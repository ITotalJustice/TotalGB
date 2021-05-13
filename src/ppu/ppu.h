#ifndef _GB_PPU_H_
#define _GB_PPU_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "../types.h"


// defined in core/ppu/ppu.c
extern const uint8_t PIXEL_BIT_SHRINK[8];
extern const uint8_t PIXEL_BIT_GROW[8];

// data selects
bool GB_get_bg_data_select(const struct GB_Core* gb);
bool GB_get_title_data_select(const struct GB_Core* gb);
bool GB_get_win_data_select(const struct GB_Core* gb);
// map selects
uint16_t GB_get_bg_map_select(const struct GB_Core* gb);
uint16_t GB_get_title_map_select(const struct GB_Core* gb);
uint16_t GB_get_win_map_select(const struct GB_Core* gb);
uint16_t GB_get_tile_offset(const struct GB_Core* gb, const uint8_t tile_num, const uint8_t sub_tile_y);

bool GB_is_render_layer_enabled(const struct GB_Core* gb, enum GB_RenderLayerConfig want);
uint8_t GB_vram_read(const struct GB_Core* gb, const uint16_t addr, const uint8_t bank);
uint8_t GB_get_sprite_size(const struct GB_Core* gb);

// GBC stuff
bool GB_is_hdma_active(const struct GB_Core* gb);
uint8_t hdma_read(const struct GB_Core* gb, const uint16_t addr);
void hdma_write(struct GB_Core* gb, const uint16_t addr, const uint8_t value);
void perform_hdma(struct GB_Core* gb);

void DMG_render_scanline(struct GB_Core* gb);
void GBC_render_scanline(struct GB_Core* gb);
void SGB_render_scanline(struct GB_Core* gb);

struct GB_Pixels {
#if GB_PIXEL_STRIDE == 8
    uint8_t* p;
#elif GB_PIXEL_STRIDE == 16
    uint16_t* p;
#elif GB_PIXEL_STRIDE == 32
    uint32_t* p;
#else
    #error "invalid GB_PIXEL_STRIDE! Use 8,16,32"
#endif
};

struct GB_Pixels get_pixels_at_scanline(struct GB_Core* gb, uint8_t ly);

#ifdef __cplusplus
}
#endif

#endif // _GB_PPU_H_
