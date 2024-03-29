#ifndef GB_PPU_H
#define GB_PPU_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../types.h"
#include "../internal.h"

#define PPU gb->ppu

// defined in core/ppu/ppu.c
extern const uint8_t PIXEL_BIT_SHRINK[8];
extern const uint8_t PIXEL_BIT_GROW[8];

// data selects
GB_FORCE_INLINE bool GB_get_bg_data_select(const struct GB_Core* gb);
GB_FORCE_INLINE bool GB_get_title_data_select(const struct GB_Core* gb);
GB_FORCE_INLINE bool GB_get_win_data_select(const struct GB_Core* gb);
// map selects
GB_FORCE_INLINE uint16_t GB_get_bg_map_select(const struct GB_Core* gb);
GB_FORCE_INLINE uint16_t GB_get_title_map_select(const struct GB_Core* gb);
GB_FORCE_INLINE uint16_t GB_get_win_map_select(const struct GB_Core* gb);
GB_FORCE_INLINE uint16_t GB_get_tile_offset(const struct GB_Core* gb, const uint8_t tile_num, const uint8_t sub_tile_y);

// GB_STATIC bool GB_is_render_layer_enabled(const struct GB_Core* gb, enum GB_RenderLayerConfig want);
GB_FORCE_INLINE uint8_t GB_vram_read(const struct GB_Core* gb, const uint16_t addr, const uint8_t bank);
GB_FORCE_INLINE uint8_t GB_get_sprite_size(const struct GB_Core* gb);

GB_FORCE_INLINE void on_bgp_write(struct GB_Core* gb, uint8_t value);
GB_FORCE_INLINE void on_obp0_write(struct GB_Core* gb, uint8_t value);
GB_FORCE_INLINE void on_obp1_write(struct GB_Core* gb, uint8_t value);

GB_FORCE_INLINE void write_scanline_to_frame(struct GB_Core* gb, const uint32_t scanline[160]);

// GBC stuff
#if GBC_ENABLE
    GB_FORCE_INLINE bool GB_is_hdma_active(const struct GB_Core* gb);
    GB_STATIC void perform_hdma(struct GB_Core* gb);
#endif

GB_STATIC void DMG_render_scanline(struct GB_Core* gb);
#if GBC_ENABLE
    GB_STATIC void GBC_render_scanline(struct GB_Core* gb);
#endif
#if SGB_ENABLE
    GB_STATIC void SGB_render_scanline(struct GB_Core* gb);
#endif

#ifdef __cplusplus
}
#endif

#endif // GB_PPU_H
