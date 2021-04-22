#include "frontend/platforms/video/sdl2/sdl2_ttf.hpp"


#include <cstdint>
#include <vector>


namespace mgb::platform::video::sdl2::ttf {


auto is_valid_char(const char c) -> bool {
	return c >= ' ' && c <= '~';
}

Font::Font(SDL_Renderer* renderer, const char* filename, float font_size) {
	this->OpenFont(renderer, filename, font_size);
}

Font::~Font() {
	this->CloseFont();
}

auto Font::CloseFont() -> void {
	if (this->atlas) {
		SDL_DestroyTexture(this->atlas);
		this->atlas = nullptr;
	}

	this->open_font = false;
}

auto Font::OpenFontRW(SDL_Renderer* renderer, SDL_RWops* rw, float font_size) -> bool {
	// if this is already open, close it, then re-open new font!
	if (this->open_font) {
		this->CloseFont();
	}

	const auto file_size = SDL_RWsize(rw);
	std::vector<std::uint8_t> buffer(file_size);

	if (SDL_RWread(rw, buffer.data(), file_size, 1) != 1) {
		return false;
	}

	SDL_RWclose(rw);

	if (stbtt_InitFont(&this->info, buffer.data(), 0) == 0) {
		this->CloseFont();
		return false;
	}

	// fill bitmap atlas with packed characters
	std::vector<uint8_t> bitmap;
	this->texture_size = 32;
	
	for (;;) {
		bitmap.resize(this->texture_size * this->texture_size);
		stbtt_pack_context pack_context;

		stbtt_PackBegin(
			&pack_context, bitmap.data(),
			this->texture_size, this->texture_size,
			0, 1, 0
		);

		stbtt_PackSetOversampling(&pack_context, 1, 1);
		
		if (!stbtt_PackFontRange(&pack_context, buffer.data(), 0, font_size, 32, 95, this->chars)) {
			// too small
			stbtt_PackEnd(&pack_context);
			this->texture_size *= 2;
		} else {
			stbtt_PackEnd(&pack_context);
			break;
		}
	}

	// convert bitmap to texture
	this->atlas = SDL_CreateTexture(
		renderer,
		SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC,
		this->texture_size, this->texture_size
	);

	SDL_SetTextureBlendMode(this->atlas, SDL_BLENDMODE_BLEND);


	std::vector<uint32_t> pixels(
		this->texture_size * this->texture_size * sizeof(Uint32)
	);

	// todo: don't make this static!
	static SDL_PixelFormat* format = NULL;
	
	if (format == NULL) {
		format = SDL_AllocFormat(SDL_PIXELFORMAT_ARGB8888);
	}

	for (int i = 0; i < this->texture_size * this->texture_size; i++) {
		pixels[i] = SDL_MapRGBA(
			format, 0xFF, 0xFF, 0xFF, bitmap[i]
		);
	}

	SDL_UpdateTexture(
		this->atlas, NULL, pixels.data(),
		this->texture_size * sizeof(Uint32)
	);

	// setup additional info
 	this->scale = stbtt_ScaleForPixelHeight(&this->info, font_size);
	stbtt_GetFontVMetrics(&this->info, &this->ascent, 0, 0);
 	this->baseline = (int) (this->ascent * this->scale);

 	this->open_font = true;

	return true;
}

auto Font::OpenFont(SDL_Renderer* renderer, const char* filename, float font_size) -> bool {
	auto rw = SDL_RWFromFile(filename, "rb");
	if (rw == NULL) {
		return false;
	}

	return this->OpenFontRW(renderer, rw, font_size);
}

auto Font::GetHeight() const -> float {
	return this->baseline;
}

auto Font::GetWidth(std::string_view text) const -> float {
	float width = 0.f;

	for (auto c : text) {
		if (is_valid_char(c)) {

			const auto char_info = this->chars[c - 32];
			width += char_info.xadvance;
		}
	}

	return width;
}

auto Font::SetColour(Colour c) -> void {
	SDL_SetTextureColorMod(this->atlas, c.r, c.g, c.b);
	SDL_SetTextureAlphaMod(this->atlas, c.a);
}

auto Font::DrawText(
	SDL_Renderer* renderer,
	float x, float y,
	std::string_view text
) -> void {
	for (auto c : text) {
		if (is_valid_char(c)) {

			const auto char_info = this->chars[c - 32];
			
			const SDL_Rect src_rect = {
				char_info.x0,
				char_info.y0,
				char_info.x1 - char_info.x0,
				char_info.y1 - char_info.y0
			};

			const SDL_Rect dst_rect = {
				static_cast<int>(x + char_info.xoff),
				static_cast<int>(y + char_info.yoff),
				char_info.x1 - char_info.x0,
				char_info.y1 - char_info.y0
			};

			SDL_RenderCopy(renderer, this->atlas, &src_rect, &dst_rect);
			x += char_info.xadvance;
		}
	}
}

auto Font::DrawTextBox(
	SDL_Renderer* renderer,
	float x, float y, // pos of the BOX
	float pad_x, float pad_y, // padding for the inner box before text
	Colour box_col, // colours
	std::string_view text
) -> void {

	const auto text_width = this->GetWidth(text);
	const auto text_height = this->GetHeight();

	const SDL_Rect box_rect = {
		static_cast<int>(x),
		static_cast<int>(y),
		static_cast<int>(text_width + (pad_x * 2)),
		static_cast<int>(text_height + (pad_y * 2))		
	};

	SDL_SetRenderDrawColor(renderer, box_col.r, box_col.g, box_col.b, box_col.a);
	SDL_RenderFillRect(renderer, &box_rect);

	auto text_y = y + pad_y;

	// avoid divide by zero...
	if (box_rect.h != 0) {
		text_y += static_cast<float>(box_rect.h / 2);
	}

	this->DrawText(renderer,
		x + pad_x, text_y,
		text
	);
}

} // namespace mgb::platform::video::sdl2::ttf
