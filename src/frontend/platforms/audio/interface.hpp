#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <functional>


extern "C" {
struct GB_ApuCallbackData;
}


namespace mgb::platform::audio {


class Interface {
public:
	Interface() = default;
	virtual ~Interface() = default;

	virtual auto SetupAudio() -> bool = 0;

	virtual auto PushSamples(const struct GB_ApuCallbackData* data) -> void = 0;

protected:


private:


};

} // namespace mgb::platform::audio

