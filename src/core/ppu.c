#include "gb.h"
#include "internal.h"
#include "tables/palette_table.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

enum GB_Modes {
    MODE_HBLANK = 0,
    MODE_VBLANK = 1,
    MODE_SPRITE = 2,
    MODE_TRANSFER = 3
};

enum GB_StatIntModes {
    STAT_INT_MODE_0 = 0x08,
    STAT_INT_MODE_1 = 0x10,
    STAT_INT_MODE_2 = 0x20,
    STAT_INT_MODE_COINCIDENCE = 0x40
};

static inline GB_U16 GB_calculate_col_from_palette(GB_U8 palette, GB_U8 colour) {
    return ((palette >> (colour << 1)) & 3);
}

static inline GB_U8 GB_vram_read(const struct GB_Data* gb, const GB_U16 addr, const GB_U8 bank) {
    assert(gb);
    assert(bank < 2);
    return gb->ppu.vram[bank][addr & 0x1FFF];
}

// data selects
static inline GB_BOOL GB_get_bg_data_select(const struct GB_Data* gb) {
    return (!!(IO_LCDC & 0x08));
}

static inline GB_BOOL GB_get_title_data_select(const struct GB_Data* gb) {
    return (!!(IO_LCDC & 0x10));
}

static inline GB_BOOL GB_get_win_data_select(const struct GB_Data* gb) {
    return (!!(IO_LCDC & 0x40));
}

// map selects
static inline GB_U16 GB_get_bg_map_select(const struct GB_Data* gb) {
    return GB_get_bg_data_select(gb) ? 0x9C00 : 0x9800;
}

static inline GB_U16 GB_get_title_map_select(const struct GB_Data* gb) {
    return GB_get_title_data_select(gb) ? 0x8000 : 0x9000;
}

static inline GB_U16 GB_get_win_map_select(const struct GB_Data* gb) {
    return GB_get_win_data_select(gb) ? 0x9C00 : 0x9800;
}

static inline GB_U16 GB_get_tile_offset(const struct GB_Data* gb, GB_U8 tile_num, GB_U8 sub_tile_y) {
    return (GB_get_title_map_select(gb) + (((GB_get_title_data_select(gb) ? tile_num : (GB_S8)tile_num)) << 4) + (sub_tile_y << 1));
}

static inline void GB_raise_if_enabled(struct GB_Data* gb, GB_U8 mode) {
    IO_IF |= ((!!(IO_STAT & mode)) << 1);
}

void GB_set_coincidence_flag(struct GB_Data* gb, GB_BOOL n) {
    IO_STAT = n ? IO_STAT | 0x04 : IO_STAT & ~0x04;
    // IO_STAT ^= (-(!!(n)) ^ IO_STAT) & 0x04;
}

void GB_set_status_mode(struct GB_Data* gb, GB_U8 mode) {
    IO_STAT = (IO_STAT & 252) | mode;
}

GB_U8 GB_get_status_mode(const struct GB_Data* gb) {
    return (IO_STAT & 0x03);
}

GB_BOOL GB_is_lcd_enabled(const struct GB_Data* gb) {
    return (!!(IO_LCDC & 0x80)); 
}

GB_BOOL GB_is_win_enabled(const struct GB_Data* gb) {
    return (!!(IO_LCDC & 0x20)); 
}

GB_BOOL GB_is_obj_enabled(const struct GB_Data* gb) {
    return (!!(IO_LCDC & 0x02)); 
}

GB_BOOL GB_is_bg_enabled(const struct GB_Data* gb) {
    return (!!(IO_LCDC & 0x01)); 
}

void GB_compare_LYC(struct GB_Data* gb) {
    if (UNLIKELY(IO_LY == IO_LYC)) {
        GB_set_coincidence_flag(gb, 1);
        GB_raise_if_enabled(gb, STAT_INT_MODE_COINCIDENCE);
    } else {
        GB_set_coincidence_flag(gb, 0);
    }
}

void GB_change_status_mode(struct GB_Data* gb, GB_U8 new_mode) {
    GB_set_status_mode(gb, new_mode);
    
    switch (new_mode) {
        case MODE_HBLANK: // hblank
            GB_raise_if_enabled(gb, STAT_INT_MODE_0);
            gb->ppu.next_cycles += 146;
            GB_draw_scanline(gb);
            if (gb->hblank_cb != NULL) {
                gb->hblank_cb(gb, gb->hblank_cb_user_data);
            }
            break;

        case MODE_VBLANK: // vblank
            GB_raise_if_enabled(gb, STAT_INT_MODE_1);
            GB_enable_interrupt(gb, GB_INTERRUPT_VBLANK);
            gb->ppu.next_cycles += 456;
            if (gb->vblank_cb != NULL) {
                gb->vblank_cb(gb, gb->vblank_cb_user_data);
            }
            break;

        case MODE_SPRITE: // sprite
            GB_raise_if_enabled(gb, STAT_INT_MODE_2);
            gb->ppu.next_cycles += 80;
            break;

        case MODE_TRANSFER: // transfer
            gb->ppu.next_cycles += 230;
            break;
    }
}

void GB_ppu_run(struct GB_Data* gb, GB_U16 cycles) {
    if (UNLIKELY(!GB_is_lcd_enabled(gb))) {
        GB_set_status_mode(gb, MODE_VBLANK);
        gb->ppu.next_cycles = 456;
        return;
    }

    gb->ppu.next_cycles -= cycles;
    if (((gb->ppu.next_cycles) > 0)) {
        return;
    }

    switch (GB_get_status_mode(gb)) {
        case MODE_HBLANK: // hblank
            ++IO_LY;
            GB_compare_LYC(gb);
            if (UNLIKELY(IO_LY == 144)) {
                GB_change_status_mode(gb, MODE_VBLANK);
            } else {
                GB_change_status_mode(gb, MODE_SPRITE);
            }
            break;
        
        case MODE_VBLANK: // vblank
            ++IO_LY;
            GB_compare_LYC(gb);
            gb->ppu.next_cycles += 456;
            if (UNLIKELY(IO_LY == 153)) {
                gb->ppu.next_cycles -= 456;
                IO_LY = 0;
                gb->ppu.line_counter = 0;
                GB_compare_LYC(gb); // important, this is needed for zelda intro.
                GB_change_status_mode(gb, MODE_SPRITE);
            }
            break;
        
        case MODE_SPRITE: // sprite
            GB_change_status_mode(gb, MODE_TRANSFER);
            break;
        
        case MODE_TRANSFER: // transfer
            GB_change_status_mode(gb, MODE_HBLANK);
            break;
    }
}

void GB_DMA(struct GB_Data* gb) {
    assert(IO_DMA <= 0xF1);

    memcpy(gb->ppu.oam, gb->mmap[IO_DMA >> 4] + ((IO_DMA & 0xF) << 8), sizeof(gb->ppu.oam));
	gb->cpu.cycles += 646;
}

void GB_update_colours_gb(GB_U16 colours[4], const GB_U16 pal_colours[4], const GB_U8 palette, GB_BOOL* dirty) {
    assert(colours && pal_colours && dirty);

    if (*dirty) {
        *dirty = GB_FALSE;
        for (GB_U8 i = 0; i < 4; ++i) {
            colours[i] = pal_colours[GB_calculate_col_from_palette(palette, i)];
        }
    }
}

void GB_update_all_colours_gb(struct GB_Data* gb) {
    assert(gb);

    // the colour palette will be auto updated next frame.
    gb->ppu.dirty_bg[0] = GB_TRUE;
    gb->ppu.dirty_obj[0] = GB_TRUE;
    gb->ppu.dirty_obj[1] = GB_TRUE;
}

static const GB_U8 PIXEL_BIT_SHRINK[] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };
static const GB_U8 PIXEL_BIT_GROW[] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };

void GB_draw_bg_gb(struct GB_Data* gb) {
    const GB_U8 scanline = IO_LY;
    const GB_U8 base_tile_x = IO_SCX >> 3;
    const GB_U8 sub_tile_x = (IO_SCX & 7);
    const GB_U8 pixel_y = (scanline + IO_SCY);
    const GB_U8 tile_y = pixel_y >> 3;
    const GB_U8 sub_tile_y = (pixel_y & 7);
    const GB_U8* bit = PIXEL_BIT_SHRINK;
    const GB_U8 *vram_map = &gb->ppu.vram[0][(GB_get_bg_map_select(gb) + (tile_y << 5)) & 0x1FFF];
    GB_U16* pixels = gb->ppu.pixles[scanline];

    for (GB_U8 tile_x = 0; tile_x <= 20; ++tile_x) {
        const GB_U8 map_x = ((base_tile_x + tile_x) & 31);
        const GB_U8 tile_num = vram_map[map_x];
        const GB_U16 offset = GB_get_tile_offset(gb, tile_num, sub_tile_y);

        const GB_U8 byte_a = GB_vram_read(gb, offset + 0, 0);
        const GB_U8 byte_b = GB_vram_read(gb, offset + 1, 0);

        for (GB_U8 x = 0; x < 8; ++x) {
            const GB_U8 pixel_x = ((tile_x << 3) + x - sub_tile_x) & 0xFF;
            if (pixel_x >= GB_SCREEN_WIDTH) {
                continue;
            }

            const GB_U8 pixel = ((!!(byte_b & bit[x])) << 1) | (!!(byte_a & bit[x]));
            pixels[pixel_x] = gb->ppu.bg_colours[0][pixel];
        }
    }
}

void GB_draw_win_gb(struct GB_Data* gb) {
    const GB_U8 scanline = IO_LY;
    const GB_U8 base_tile_x = 20 - (IO_WX >> 3);
    const GB_S16 sub_tile_x = IO_WX - 7;
    const GB_U8 pixel_y = (IO_LY - IO_WY);
    const GB_U8 tile_y = pixel_y >> 3;
    const GB_U8 sub_tile_y = (pixel_y & 7);
    const GB_U8* bit = PIXEL_BIT_SHRINK;
    const GB_U8 *vram_map = &gb->ppu.vram[0][(GB_get_win_map_select(gb) + (tile_y << 5)) & 0x1FFF];
    GB_U16* pixels = gb->ppu.pixles[scanline];
    GB_BOOL inc_it = GB_FALSE;

    for (GB_U8 tile_x = 0; tile_x <= base_tile_x; ++tile_x) {
        const GB_U8 tile_num = vram_map[tile_x];
        const GB_U16 offset = GB_get_tile_offset(gb, tile_num, sub_tile_y);

        const GB_U8 byte_a = GB_vram_read(gb, offset + 0, 0);
        const GB_U8 byte_b = GB_vram_read(gb, offset + 1, 0);

        for (GB_U8 x = 0; x < 8; ++x) {
            const GB_U8 pixel_x = ((tile_x << 3) + x + sub_tile_x) & 0xFF;
            if (pixel_x >= GB_SCREEN_WIDTH) {
                continue;
            }

            inc_it |= GB_FALSE;
            const GB_U8 pixel = ((!!(byte_b & bit[x])) << 1) | (!!(byte_a & bit[x]));
            pixels[pixel_x] = gb->ppu.bg_colours[0][pixel];
        }
    }
    gb->ppu.line_counter += inc_it;
}

static inline GB_BOOL GB_get_sprite_size(const struct GB_Data* gb) {
    return ((IO_LCDC & 0x04) ? 16 : 8);
}

void GB_draw_obj(struct GB_Data* gb) {
    const GB_U8 scanline = IO_LY;
    const GB_U8 sprite_size = GB_get_sprite_size(gb);
    const GB_U16 bg_trans_col = gb->ppu.bg_colours[0][0];
    GB_U16* pixels = gb->ppu.pixles[scanline];

    for (GB_U8 i = 0, sprite_total = 0; i < 40 && sprite_total < 10; ++i) {
        const struct GB_Sprite sprite = gb->ppu.sprites[i];
        const GB_S16 spy = (GB_S16)sprite.y - 16;
        const GB_S16 spx = (GB_S16)sprite.x - 8;

        if (scanline >= spy && scanline < (spy + (sprite.flag.yflip ? -sprite_size: sprite_size))) {
            ++sprite_total;
            if ((spx + 8) == 0 || spx >= GB_SCREEN_WIDTH) {
                continue;
            }

            const GB_U8 sprite_line = sprite.flag.yflip ? sprite_size - 1 - (scanline - spy) : scanline - spy;
            const GB_U16 offset = 0x8000 | (((sprite_line) << 1) + (sprite.num << 4));

            const GB_U8 byte_a = GB_vram_read(gb, offset + 0, 0);
            const GB_U8 byte_b = GB_vram_read(gb, offset + 1, 0);

            const GB_U8* bit = sprite.flag.xflip ? PIXEL_BIT_GROW : PIXEL_BIT_SHRINK;

            for (GB_U8 x = 0; x < 8; ++x) {
                if ((spx + x) < 0 || (sprite.flag.priority && pixels[spx + x] != bg_trans_col)) {
                    continue;
                }
                const GB_U8 pixel = ((!!(byte_b & bit[x])) << 1) | (!!(byte_a & bit[x]));
                
                if (pixel == 0) {
                    continue;
                }

                pixels[spx + x] = gb->ppu.obj_colours[sprite.flag.pal_gb][pixel];
            }
        }
    }
}

void GB_draw_scanline(struct GB_Data* gb) {
    GB_update_colours_gb(gb->ppu.bg_colours[0], gb->palette.BG, IO_BGP, &gb->ppu.dirty_bg[0]);
    GB_update_colours_gb(gb->ppu.obj_colours[0], gb->palette.OBJ0, IO_OBP0, &gb->ppu.dirty_obj[0]);
    GB_update_colours_gb(gb->ppu.obj_colours[1], gb->palette.OBJ1, IO_OBP1, &gb->ppu.dirty_obj[1]);

    if (LIKELY(GB_is_bg_enabled(gb))) {
        GB_draw_bg_gb(gb);
        // WX=0..166, WY=0..143
        if ((GB_is_win_enabled(gb)) && (IO_WX <= 166) && (IO_WY <= 143) && (IO_WY <= IO_LY)) {
            GB_draw_win_gb(gb);
            ++gb->ppu.line_counter;
        }
    }

    if (LIKELY(GB_is_obj_enabled(gb))) {
        GB_draw_obj(gb);
    }
}
