#include "frontend/platforms/video/interface.hpp"


namespace mgb::platform::video {


Interface::TextPopup::TextPopup(std::string _text)
	: text{std::move(_text)} {

}

auto Interface::TextPopup::GetText() const -> const std::string& {
	return this->text;
}

auto Interface::TextPopup::Tick() -> TickResult {
	++this->counter;

	if (this->counter >= this->COUNTER_MAX) {
		return TickResult::POPME;
	} else {
		return TickResult::OK;
	}
}

auto Interface::RenderDisplay() -> void {
	// tick the popups
	for (std::size_t i = 0; i < this->text_popups.size(); ++i) {
		auto result = this->text_popups[i].Tick();

		switch (result) {
			case Interface::TextPopup::TickResult::OK:
				break;

			case Interface::TextPopup::TickResult::POPME:
				this->text_popups.erase(this->text_popups.begin() + i);
		}
	}

	this->RenderDisplayInternal();
}
	
auto Interface::SetupVideo(VideoInfo vid_info, GameTextureInfo game_info) -> bool {
	return this->SetupVideoInternal(vid_info, game_info);
}

auto Interface::PushTextPopup(const std::string& text) -> void {
	this->text_popups.emplace_front(text);

	// no more than 30, remove old popups
	if (this->text_popups.size() >= 30) {
		this->text_popups.pop_back();
	}
}

auto Interface::ClearTextPopups() -> void {
	this->text_popups.clear();
}

auto Interface::GetTextPopupCount() const -> std::size_t {
	return this->text_popups.size();
}

} // namespace mgb::platform::video
