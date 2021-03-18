#pragma once

#include <cstdint>
#include <vector>
#include <deque>
#include <optional>

extern "C" {
struct GB_CoreState; // opaque
}

namespace mgb {

using State = GB_CoreState;
using PackedState = std::vector<std::uint8_t>;

auto packState(const State& state) -> PackedState;
auto unpackState(const PackedState& packed) -> State;

class Rewinder final {
public:
    // move the packed keyframe on init
    Rewinder(PackedState&& _keyframe);
    Rewinder() = delete;

    // the packedstate will be delta checked against the current keyframe
    // then after delta, it will be further compressed using either ztsd, zlib or lz4
    auto Push(PackedState&& packed) -> bool;
    // fills packedstate with the uncompressed state.
    auto Pop() -> std::optional<PackedState>;
    // called by the RewindManager when frame count is full.
    auto PopFront(void) -> bool;
    // compresses the keyframe. used when pushing a new keyframe (hence store)
    auto Store(void) -> void;
    // uncompresses the keyframe. used when rewinding.
    auto Restore(void) -> void;

    auto WantedNewKeyframe(void) const noexcept {
        return want_new_keyframe;
    }
    auto IsKeyframeCompressed(void) const noexcept {
        return this->keyframe_compressed;
    }
    auto MaxFrameSize(void) const noexcept {
        return this->keyframe.size();
    }
    auto Size(void) const noexcept {
        return this->states.size();
    }
    auto AverageSize(void) const noexcept {
        return this->avg_size;
    }

private:
    PackedState keyframe; // uncompressed keyframe state.
    std::deque<PackedState> states{}; // compressed states.
    const std::size_t keyframe_size;
    std::size_t avg_size{0};
    std::size_t total_size{0}; // size of all data after compression.
    bool want_new_keyframe{false};
    bool keyframe_compressed{false};
    
};

class RewindManager final {
public:
    auto Reset(std::size_t size = 0) -> void;
    auto Resize(std::size_t size) -> void;

    auto Push(State&& state) -> bool;
    auto Pop() -> std::optional<State>;

    auto Size(void) const noexcept {
        return this->rewinders.size();
    }
    auto Capacity(void) const noexcept {
        return this->max_states;
    }
    auto StateCount(void) const noexcept {
        return this->states_count;
    }

private:
    std::deque<Rewinder> rewinders{};
    std::size_t max_states{0};
    std::size_t states_count{0};
    bool smart_mode{true}; // not very smart yet
};

} // namespace mbg
