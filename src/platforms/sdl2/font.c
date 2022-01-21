#include <SDL.h>

#include "font.h"


#include "stb_rect_pack.h"
#include "stb_truetype.h"

/* STBTTF: A quick and dirty SDL2 text renderer based on stb_truetype and stdb_rect_pack.
 * Benoit Favre 2019
 *
 * This header-only addon to the stb_truetype library allows to draw text with SDL2 from
 * TTF fonts with a similar API to SDL_TTF without the bloat.
 * The renderer is however limited by the integral positioning of SDL blit functions.
 * It also does not parse utf8 text and only prints ASCII characters.
 *
 * This code is public domain.
 */

typedef struct {
	stbtt_fontinfo* info;
	stbtt_packedchar* chars;
	SDL_Texture* atlas;
	int texture_size;
	float size;
	float scale;
	int ascent;
	int baseline;
} STBTTF_Font;

/* Release the memory and textures associated with a font */
void STBTTF_CloseFont(STBTTF_Font* font);

/* Open a TTF font given a SDL abstract IO handler, for a given renderer and a given font size.
 * Returns NULL on failure. The font must be deallocated with STBTTF_CloseFont when not used anymore.
 * This function creates a texture atlas with prerendered ASCII characters (32-128).
 */
STBTTF_Font* STBTTF_OpenFontRW(SDL_Renderer* renderer, SDL_RWops* rw, float size);

/* Open a TTF font given a filename, for a given renderer and a given font size.
 * Convinience function which calls STBTTF_OpenFontRW.
 */
STBTTF_Font* STBTTF_OpenFont(SDL_Renderer* renderer, const char* filename, float size);

/* Draw some text using the renderer draw color at location (x, y).
 * Characters are copied from the texture atlas using the renderer SDL_RenderCopy function.
 * Since that function only supports integral coordinates, the result is not great.
 * Only ASCII characters (32 <= c < 128) are supported. Anything outside this range is ignored.
 */
void STBTTF_RenderText(SDL_Renderer* renderer, STBTTF_Font* font, float x, float y, const char *text);

/* Return the length in pixels of a text.
 * You can get the height of a line by using font->baseline.
 */
float STBTTF_MeasureText(STBTTF_Font* font, const char *text);

// impl
void STBTTF_CloseFont(STBTTF_Font* font) {
	if(font->atlas) SDL_DestroyTexture(font->atlas);
	if(font->info) free(font->info);
	if(font->chars) free(font->chars);
	free(font);
}

STBTTF_Font* STBTTF_OpenFontRW(SDL_Renderer* renderer, SDL_RWops* rw, float size) {
	Sint64 file_size = SDL_RWsize(rw);
	unsigned char* buffer = malloc(file_size);
	if(SDL_RWread(rw, buffer, file_size, 1) != 1) return NULL;
	SDL_RWclose(rw);

	STBTTF_Font* font = calloc(sizeof(STBTTF_Font), 1);
	font->info = malloc(sizeof(stbtt_fontinfo));
	font->chars = malloc(sizeof(stbtt_packedchar) * 96);

	if(stbtt_InitFont(font->info, buffer, 0) == 0) {
		free(buffer);
		STBTTF_CloseFont(font);
		return NULL;
	}

	// fill bitmap atlas with packed characters
	unsigned char* bitmap = NULL;
	font->texture_size = 32;
	while(1) {
		bitmap = malloc(font->texture_size * font->texture_size);
		stbtt_pack_context pack_context;
		stbtt_PackBegin(&pack_context, bitmap, font->texture_size, font->texture_size, 0, 1, 0);
		stbtt_PackSetOversampling(&pack_context, 1, 1);
		if(!stbtt_PackFontRange(&pack_context, buffer, 0, size, 32, 95, font->chars)) {
			// too small
			free(bitmap);
			stbtt_PackEnd(&pack_context);
			font->texture_size *= 2;
		} else {
			stbtt_PackEnd(&pack_context);
			break;
		}
	}

	// convert bitmap to texture
	font->atlas = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, font->texture_size, font->texture_size);
	SDL_SetTextureBlendMode(font->atlas, SDL_BLENDMODE_BLEND);

	Uint32* pixels = malloc(font->texture_size * font->texture_size * sizeof(Uint32));
	static SDL_PixelFormat* format = NULL;
	if(format == NULL) format = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA32);
	for(int i = 0; i < font->texture_size * font->texture_size; i++) {
		pixels[i] = SDL_MapRGBA(format, 0xff, 0xff, 0xff, bitmap[i]);
	}
	SDL_UpdateTexture(font->atlas, NULL, pixels, font->texture_size * sizeof(Uint32));
	free(pixels);
	free(bitmap);

	// setup additional info
  font->scale = stbtt_ScaleForPixelHeight(font->info, size);
	stbtt_GetFontVMetrics(font->info, &font->ascent, 0, 0);
  font->baseline = (int) (font->ascent * font->scale);

	free(buffer);

	return font;
}

STBTTF_Font* STBTTF_OpenFont(SDL_Renderer* renderer, const char* filename, float size) {
	SDL_RWops *rw = SDL_RWFromFile(filename, "rb");
	if(rw == NULL) return NULL;
	return STBTTF_OpenFontRW(renderer, rw, size);
}

void STBTTF_RenderText(SDL_Renderer* renderer, STBTTF_Font* font, float x, float y, const char *text) {
	Uint8 r, g, b, a;
	SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a);
	SDL_SetTextureColorMod(font->atlas, r, g, b);
	SDL_SetTextureAlphaMod(font->atlas, a);
	for(int i = 0; text[i]; i++) {
		if (text[i] >= 32 && text[i] < 128) {
			//if(i > 0) x += stbtt_GetCodepointKernAdvance(font->info, text[i - 1], text[i]) * font->scale;

			stbtt_packedchar* info = &font->chars[text[i] - 32];
			SDL_Rect src_rect = {info->x0, info->y0, info->x1 - info->x0, info->y1 - info->y0};
			SDL_Rect dst_rect = {x + info->xoff, y + info->yoff, info->x1 - info->x0, info->y1 - info->y0};

			SDL_RenderCopy(renderer, font->atlas, &src_rect, &dst_rect);
			x += info->xadvance;
		}
	}
}

float STBTTF_MeasureText(STBTTF_Font* font, const char *text) {
	float width = 0;
	for(int i = 0; text[i]; i++) {
		if (text[i] >= 32 && text[i] < 128) {
			//if(i > 0) width += stbtt_GetCodepointKernAdvance(font->info, text[i - 1], text[i]) * font->scale;

			stbtt_packedchar* info = &font->chars[text[i] - 32];
			width += info->xadvance;
		}
	}
	return width;
}
