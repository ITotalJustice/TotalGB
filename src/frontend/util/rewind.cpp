#include "rewind.hpp"
#include "log.hpp"
#include "mem.hpp"
#include "../../core/gb.h"

#include <cstring>

namespace mgb {

constexpr auto MAX_STATES = 120;

// TODO: check if we get RTO (copy ellision)
auto packState(const State& state) -> PackedState {
    PackedState packed(sizeof(State));
    std::memcpy(packed.data(), &state, sizeof(State));
    return packed;
}

auto unpackState(const PackedState& packed) -> State {
    State unpacked;
    std::memcpy(&unpacked, packed.data(), sizeof(State));
    return unpacked;
}

/// REWINDER ///
Rewinder::Rewinder(PackedState&& _keyframe)
: keyframe{_keyframe}
, keyframe_size(_keyframe.size()) { 
    this->Push(std::forward<PackedState>(_keyframe));
}

auto Rewinder::Push(PackedState&& packed) -> bool{
    const auto original_size = packed.size();
    const auto [result, new_size] = mem::Delta(
        this->keyframe, packed,
        mem::Mode::DEFLATE, mem::CompressionTypes::ZSTD
    );
    
    if (!result) {
        return log::errReturn(false, "failed to delta compress\n");
    }

    // keep keyframe if 90% or more match.
    // trash keyframe is less than 75% match or 2 seconds passed.
    const float same = (static_cast<float>(original_size - new_size)) / static_cast<float>(original_size) * 100.f;
    this->want_new_keyframe = (same < 90.f) && ((same < 75.f) || (this->Size() >= MAX_STATES));
    this->states.emplace_back(packed);

    return true;
}

auto Rewinder::Pop() -> std::optional<PackedState> {
    if (!this->Size()) {
        return log::errReturn(std::nullopt, "failed to delta compress\n");
    }
    
    auto packed = this->states.back();
    this->states.pop_back();

    const auto [result, new_size] = mem::Delta(
        this->keyframe, packed,
        mem::Mode::INFLATE, mem::CompressionTypes::ZSTD
    );
    
    if (!result) {
        return {};
    }

    return packed;
}

auto Rewinder::PopFront(void) -> bool {
    if (!this->Size()) {
        return log::errReturn(false, "trying to pop from front\n");
    }

    this->states.pop_front();
    return true;
}

auto Rewinder::Store(void) -> void {
    this->keyframe_compressed = true;
    PackedState compressed(this->keyframe_size);
    mem::Zstd(compressed, this->keyframe, mem::Mode::DEFLATE);
    this->keyframe = std::move(compressed);
}

// uncompresses the keyframe. used when rewinding.
auto Rewinder::Restore(void) -> void {
    this->keyframe_compressed = false;
    PackedState uncompressed(this->keyframe_size);
    mem::Zstd(uncompressed, this->keyframe, mem::Mode::INFLATE);
    this->keyframe = std::move(uncompressed);
}

/// REWIND MANAGER ///
auto RewindManager::Reset(std::size_t size) -> void {
    this->rewinders.clear();
    this->states_count = 0;
    this->max_states = size;
}

auto RewindManager::Push(State&& state) -> bool {
    if (!this->max_states) {
        return log::errReturn(false, "trying to push state with max_states set to 0!\n");
    }

    // this is error prone if the size of the 
    if (this->states_count >= this->max_states) {
        // check if rewinder is empty, if so, pop it
        if (!this->rewinders.front().Size()) {
            this->rewinders.pop_front();
        }

        if (this->rewinders.size()) {
            this->rewinders.front().PopFront();
        }
        --this->states_count;
    }

    if (!rewinders.size()) {
        this->rewinders.emplace_back(packState(state));
    } else if ((this->smart_mode && rewinders.back().WantedNewKeyframe()) ||
        (!this->smart_mode && rewinders.back().Size() >= MAX_STATES)) {
        this->rewinders.back().Store();
        this->rewinders.emplace_back(packState(state));
    } else {
        this->rewinders.back().Push(packState(state));
    }

    ++this->states_count;

    return true;   
}

auto RewindManager::Pop() -> std::optional<State> {
    if (!this->Size()) {
        return {};
    }

    if (!this->rewinders.back().Size()) {
        this->rewinders.pop_back();
        if (!this->rewinders.size()) {
            return {};
        }
        this->rewinders.back().Restore();
    }

    if (auto packed = this->rewinders.back().Pop()) {
        --this->states_count;
        return unpackState(*packed);
    }

    return log::errReturn(std::nullopt, "failed to pop rewind when size is > 1!\n");
}

// todo: change this so it doesn't yeet all frames.
auto RewindManager::Resize(std::size_t size) -> void {
    this->Reset(size);
}

} // namespace mgb
