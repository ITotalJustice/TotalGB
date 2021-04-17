#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <functional>


extern "C" {
struct GB_Core;
struct GB_ErrorData;
}


namespace mgb::platform::video {

// this header tries to be a generic interface for
// platform video / window systems.

// windows systems should handle input, display, errors etc...

enum class RenderType {
	SOFTWARE,
	ACCELERATED,
	ACCELERATED_VSYNC,
};

struct VideoInfo {
	std::string name;
	RenderType render_type;
	int x, y, w, h;
};

struct GameTextureInfo {
	int x, y, w, h;
};

struct GameTextureData {
	const uint16_t* pixels;
	int w, h;
};

struct Callbacks {
	std::function<struct GB_Core*()> GetCore;
	std::function<bool(std::string path)> LoadRom;
	std::function<void()> SaveState;
	std::function<void()> LoadState;
	std::function<void()> FilePicker;
	std::function<void()> OnQuit;
};

class Interface {
public:
	Interface(Callbacks _cb) : callback{_cb} {}
	virtual ~Interface() = default;

	virtual auto SetupVideo(VideoInfo vid_info, GameTextureInfo game_info) -> bool = 0;

	virtual auto UpdateGameTexture(GameTextureData data) -> void = 0;

	virtual auto RenderDisplay() -> void = 0;

	virtual auto PollEvents() -> void = 0;

protected:
	Callbacks callback;

	std::vector<std::uint16_t> game_pixels;
	
	bool is_video_setup = false;

private:

};


} // namespace mgb::platform::video
