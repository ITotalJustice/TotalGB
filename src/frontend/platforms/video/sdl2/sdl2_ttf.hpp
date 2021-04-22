// SOURCE: https://gist.github.com/benob/92ee64d9ffcaa5d3be95edbf4ded55f2

#pragma once


#include "stb/stb_rect_pack.h"
#include "stb/stb_truetype.h"

#include <SDL2/SDL.h>
#include <cstdint>
#include <string_view>


namespace mgb::platform::video::sdl2::ttf {

struct Colour {
	using u8 = std::uint8_t;

	constexpr Colour() = default;
	constexpr Colour(u8 _r, u8 _g, u8 _b) : r{_r}, g{_g}, b{_b}, a{0xFF} {}
	constexpr Colour(u8 _r, u8 _g, u8 _b, u8 _a) : r{_r}, g{_g}, b{_b}, a{_a} {}

	u8 r{0xFF}, g{0xFF}, b{0xFF}, a{0xFF};
};

class Font final {
public:
	Font() = default;
	Font(SDL_Renderer* renderer, const char* filename, float font_size);
	~Font();

	/* Release the memory and textures associated with a font */
	void CloseFont();

	/* Open a TTF font given a SDL abstract IO handler, for a given renderer and a given font size.
	 * Returns NULL on failure. The font must be deallocated with STBTTF_CloseFont when not used anymore.
	 * This function creates a texture atlas with prerendered ASCII characters (32-128).
	 */ 
	bool OpenFontRW(SDL_Renderer* renderer, SDL_RWops* rw, float font_size);

	/* Open a TTF font given a filename, for a given renderer and a given font size.
	 * Convinience function which calls STBTTF_OpenFontRW.
	 */
	bool OpenFont(SDL_Renderer* renderer, const char* filename, float font_size);

	/*  returns font->baseline. */
	float GetHeight() const;

	/* Return the length in pixels of a text. */
	float GetWidth(std::string_view text) const;

	auto IsOpen() const {
		return this->open_font;
	}

	auto SetColour(Colour colour) -> void;

	auto DrawText(
		SDL_Renderer* renderer,
		float x, float y,
		std::string_view text
	) -> void;

	auto DrawTextBox(
		SDL_Renderer* renderer,
		float x, float y, // pos of the BOX
		float pad_x, float pad_y, // padding for the inner box before text
		Colour box_col, // colours
		std::string_view text
	) -> void;

private:
	SDL_Texture* atlas = nullptr;
	stbtt_packedchar chars[96]{};
	stbtt_fontinfo info{};

	float scale;
	float size;
	int texture_size;

	int ascent;
	int baseline;

	bool open_font{false};
};

} // namespace mgb::platform::video::sdl2::ttf
