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
GB_INLINE bool GB_get_bg_data_select(const struct GB_Core* gb);
GB_INLINE bool GB_get_title_data_select(const struct GB_Core* gb);
GB_INLINE bool GB_get_win_data_select(const struct GB_Core* gb);
// map selects
GB_INLINE uint16_t GB_get_bg_map_select(const struct GB_Core* gb);
GB_INLINE uint16_t GB_get_title_map_select(const struct GB_Core* gb);
GB_INLINE uint16_t GB_get_win_map_select(const struct GB_Core* gb);
GB_INLINE uint16_t GB_get_tile_offset(const struct GB_Core* gb, const uint8_t tile_num, const uint8_t sub_tile_y);

GB_STATIC bool GB_is_render_layer_enabled(const struct GB_Core* gb, enum GB_RenderLayerConfig want);
GB_INLINE uint8_t GB_vram_read(const struct GB_Core* gb, const uint16_t addr, const uint8_t bank);
GB_STATIC uint8_t GB_get_sprite_size(const struct GB_Core* gb);

// GBC stuff
GB_INLINE bool GB_is_hdma_active(const struct GB_Core* gb);
GB_INLINE uint8_t hdma_read(const struct GB_Core* gb, const uint16_t addr);
GB_STATIC void hdma_write(struct GB_Core* gb, const uint16_t addr, const uint8_t value);
GB_STATIC void perform_hdma(struct GB_Core* gb);

GB_STATIC void DMG_render_scanline(struct GB_Core* gb);
GB_STATIC void GBC_render_scanline(struct GB_Core* gb);
GB_STATIC void SGB_render_scanline(struct GB_Core* gb);

GB_INLINE void ppu_write_pixel(struct GB_Core* gb, uint32_t c, uint8_t x, uint8_t y);

#ifdef __cplusplus
}
#endif

#endif // _GB_PPU_H_
