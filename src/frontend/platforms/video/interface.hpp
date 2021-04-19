#pragma once

#include "frontend/types.hpp"

#include <string>
#include <vector>
#include <functional>



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
	// checks to see if all functions are set!
	auto Validate() const {
		if (!this->OnFileDrop) {
			return false;
		}
		if (!this->OnAction) {
			return false;
		}
		if (!this->OnQuit) {
			return false;
		}
		return true;
	}

	// this is called when a file is dragged onto the
	// display window.
	std::function<bool(std::string path)> OnFileDrop;
	std::function<void(Action action, bool down)> OnAction;
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

	virtual auto ToggleFullscreen() -> void = 0;
	virtual auto SetWindowName(const std::string& name) -> void = 0;

protected:
	Callbacks callback;

	std::vector<std::uint16_t> game_pixels;
	
	bool is_video_setup = false;

private:

};


} // namespace mgb::platform::video
