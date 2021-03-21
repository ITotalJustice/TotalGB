#include "gb.h"
#include "internal.h"
#include "tables/palette_table.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

static inline GB_U16 GB_calculate_col_from_palette(const GB_U8 palette, const GB_U8 colour) {
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

static inline GB_U16 GB_get_tile_offset(const struct GB_Data* gb, const GB_U8 tile_num, const GB_U8 sub_tile_y) {
    return (GB_get_title_map_select(gb) + (((GB_get_title_data_select(gb) ? tile_num : (GB_S8)tile_num)) << 4) + (sub_tile_y << 1));
}

static inline void GB_raise_if_enabled(struct GB_Data* gb, const GB_U8 mode) {
    // IO_IF |= ((!!(IO_STAT & mode)) << 1);
    if (IO_STAT & mode) {
        GB_enable_interrupt(gb, GB_INTERRUPT_LCD_STAT);
    }
}

void GB_set_coincidence_flag(struct GB_Data* gb, const GB_BOOL n) {
    IO_STAT = n ? IO_STAT | 0x04 : IO_STAT & ~0x04;
    // IO_STAT ^= (-(!!(n)) ^ IO_STAT) & 0x04;
}

void GB_set_status_mode(struct GB_Data* gb, const enum GB_StatusModes mode) {
    IO_STAT = (IO_STAT & 252) | mode;
}

enum GB_StatusModes GB_get_status_mode(const struct GB_Data* gb) {
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

static inline GB_BOOL GB_is_bcps_auto_increment(const struct GB_Data* gb) {
    return (IO_BCPS & 0x80) > 0;
}

static inline GB_BOOL GB_is_ocps_auto_increment(const struct GB_Data* gb) {
    return (IO_OCPS & 0x80) > 0;
}

static inline GB_U8 GB_get_bcps_index(const struct GB_Data* gb) {
    return IO_BCPS & 0x3F;
}

static inline GB_U8 GB_get_ocps_index(const struct GB_Data* gb) {
    return IO_OCPS & 0x3F;
}

static inline void GB_bcps_increment(struct GB_Data* gb) {
    if (GB_is_bcps_auto_increment(gb) == GB_TRUE) {
        // only increment the lower 5 bits.
        // set back the increment bit after.
        // printf("bcps auto inc\n");
        IO_BCPS = ((IO_BCPS + 1) & 0x3F) | (0x80);
    }
}

static inline void GB_ocps_increment(struct GB_Data* gb) {
    if (GB_is_ocps_auto_increment(gb) == GB_TRUE) {
        // only increment the lower 5 bits.
        // set back the increment bit after.
        // printf("ocps auto inc\n");
        IO_OCPS = ((IO_OCPS + 1) & 0x3F) | (0x80);
    }
}

void GB_bcpd_write(struct GB_Data* gb, GB_U8 value) {
    const GB_U8 index = GB_get_bcps_index(gb);

    // this is 0-7
    assert((index >> 3) <= 7);
    gb->ppu.dirty_bg[index >> 3] = GB_TRUE;

    gb->ppu.bg_palette[index] = value;
    GB_bcps_increment(gb);
}

void GB_ocpd_write(struct GB_Data* gb, GB_U8 value) {
    const GB_U8 index = GB_get_ocps_index(gb);

    // this is 0-7
    assert((index >> 3) <= 7);
    gb->ppu.dirty_obj[index >> 3] = GB_TRUE;

    gb->ppu.obj_palette[index] = value;
    GB_ocps_increment(gb);
}

void GB_compare_LYC(struct GB_Data* gb) {
    if (UNLIKELY(IO_LY == IO_LYC)) {
        GB_set_coincidence_flag(gb, GB_TRUE);
        GB_raise_if_enabled(gb, STAT_INT_MODE_COINCIDENCE);
    } else {
        GB_set_coincidence_flag(gb, GB_FALSE);
    }
}

void GB_change_status_mode(struct GB_Data* gb, const GB_U8 new_mode) {
    GB_set_status_mode(gb, new_mode);
    
    switch (new_mode) {
        case STATUS_MODE_HBLANK: // hblank
            GB_raise_if_enabled(gb, STAT_INT_MODE_0);
            gb->ppu.next_cycles += 146;
            GB_draw_scanline(gb);
            if (gb->hblank_cb != NULL) {
                gb->hblank_cb(gb, gb->hblank_cb_user_data);
            }
            break;

        case STATUS_MODE_VBLANK: // vblank
            GB_raise_if_enabled(gb, STAT_INT_MODE_1);
            GB_enable_interrupt(gb, GB_INTERRUPT_VBLANK);
            gb->ppu.next_cycles += 456;
            if (gb->vblank_cb != NULL) {
                gb->vblank_cb(gb, gb->vblank_cb_user_data);
            }
            break;

        case STATUS_MODE_SPRITE: // sprite
            GB_raise_if_enabled(gb, STAT_INT_MODE_2);
            gb->ppu.next_cycles += 80;
            break;

        case STATUS_MODE_TRANSFER: // transfer
            gb->ppu.next_cycles += 230;
            break;
    }
}

void GB_ppu_run(struct GB_Data* gb, GB_U16 cycles) {
    if (UNLIKELY(!GB_is_lcd_enabled(gb))) {
        GB_set_status_mode(gb, STATUS_MODE_VBLANK);
        gb->ppu.next_cycles = 456;
        return;
    }

    gb->ppu.next_cycles -= cycles;
    if (((gb->ppu.next_cycles) > 0)) {
        return;
    }

    switch (GB_get_status_mode(gb)) {
        case STATUS_MODE_HBLANK: // hblank
            ++IO_LY;
            GB_compare_LYC(gb);
            if (UNLIKELY(IO_LY == 144)) {
                GB_change_status_mode(gb, STATUS_MODE_VBLANK);
            } else {
                GB_change_status_mode(gb, STATUS_MODE_SPRITE);
            }
            break;
        
        case STATUS_MODE_VBLANK: // vblank
            ++IO_LY;
            GB_compare_LYC(gb);
            gb->ppu.next_cycles += 456;
            if (UNLIKELY(IO_LY == 153)) {
                gb->ppu.next_cycles -= 456;
                IO_LY = 0;
                GB_compare_LYC(gb); // important, this is needed for zelda intro.
                GB_change_status_mode(gb, STATUS_MODE_SPRITE);
            }
            
            break;
        
        case STATUS_MODE_SPRITE: // sprite
            GB_change_status_mode(gb, STATUS_MODE_TRANSFER);
            break;
        
        case STATUS_MODE_TRANSFER: // transfer
            GB_change_status_mode(gb, STATUS_MODE_HBLANK);
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

static inline void GB_draw_bg_gb(struct GB_Data* gb) {
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

static inline void GB_draw_win_gb(struct GB_Data* gb) {
    const GB_U8 scanline = IO_LY;
    const GB_U8 base_tile_x = 20 - (IO_WX >> 3);
    const GB_S16 sub_tile_x = IO_WX - 7;
    const GB_U8 pixel_y = (IO_LY - IO_WY);
    const GB_U8 tile_y = pixel_y >> 3;
    const GB_U8 sub_tile_y = (pixel_y & 7);

    const GB_U8* bit = PIXEL_BIT_SHRINK;
    const GB_U8 *vram_map = &gb->ppu.vram[0][(GB_get_win_map_select(gb) + (tile_y << 5)) & 0x1FFF];
    GB_U16* pixels = gb->ppu.pixles[scanline];

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

            const GB_U8 pixel = ((!!(byte_b & bit[x])) << 1) | (!!(byte_a & bit[x]));
            pixels[pixel_x] = gb->ppu.bg_colours[0][pixel];
        }
    }
}

static inline GB_BOOL GB_get_sprite_size(const struct GB_Data* gb) {
    return ((IO_LCDC & 0x04) ? 16 : 8);
}

static inline void GB_draw_obj(struct GB_Data* gb) {
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

static inline void GB_draw_bg_gb_gbc(struct GB_Data* gb) {
    const GB_U8 scanline = IO_LY;
    const GB_U8 base_tile_x = IO_SCX >> 3;
    const GB_U8 sub_tile_x = (IO_SCX & 7);
    const GB_U8 pixel_y = (scanline + IO_SCY);
    const GB_U8 tile_y = pixel_y >> 3;
    const GB_U8 sub_tile_y = (pixel_y & 7);

    // array maps
    const GB_U8* vram_map = &gb->ppu.vram[0][(GB_get_bg_map_select(gb) + (tile_y << 5)) & 0x1FFF];
    const struct GB_BgAttributes* attribute_map = &gb->ppu.bg_attributes[1][(GB_get_bg_map_select(gb) + (tile_y << 5)) & 0x1FFF];
    GB_U16* pixels = gb->ppu.pixles[scanline];

    for (GB_U8 tile_x = 0; tile_x <= 20; ++tile_x) {
        // calc the map index
        const GB_U8 map_x = ((base_tile_x + tile_x) & 31);
        
        // fetch the tile number and attributes
        const GB_U8 tile_num = vram_map[map_x];
        const struct GB_BgAttributes attributes = attribute_map[map_x];

        const GB_U16 offset = GB_get_tile_offset(gb,
            tile_num,
            // check if flip the y axis
            attributes.yflip ? 7 - sub_tile_y : sub_tile_y
        );

        const GB_U8 byte_a = GB_vram_read(gb, offset + 0, attributes.bank);
        const GB_U8 byte_b = GB_vram_read(gb, offset + 1, attributes.bank);

        const GB_U8* bit = attributes.xflip ? PIXEL_BIT_GROW : PIXEL_BIT_SHRINK;

        for (GB_U8 x = 0; x < 8; ++x) {
            const GB_U8 pixel_x = ((tile_x << 3) + x - sub_tile_x) & 0xFF;
            if (pixel_x >= GB_SCREEN_WIDTH) {
                continue;
            }

            const GB_U8 pixel = ((!!(byte_b & bit[x])) << 1) | (!!(byte_a & bit[x]));
            pixels[pixel_x] = gb->ppu.bg_colours[attributes.pal][pixel];
        }
    }
}

static inline void GB_draw_win_gb_gbc(struct GB_Data* gb) {
    const GB_U8 scanline = IO_LY;
    const GB_U8 base_tile_x = 20 - (IO_WX >> 3);
    const GB_S16 sub_tile_x = IO_WX - 7;
    const GB_U8 pixel_y = (IO_LY - IO_WY);
    const GB_U8 tile_y = pixel_y >> 3;
    const GB_U8 sub_tile_y = (pixel_y & 7);

    const GB_U8 *vram_map = &gb->ppu.vram[0][(GB_get_win_map_select(gb) + (tile_y << 5)) & 0x1FFF];
    const struct GB_BgAttributes* attribute_map = &gb->ppu.bg_attributes[1][(GB_get_win_map_select(gb) + (tile_y << 5)) & 0x1FFF];
    GB_U16* pixels = gb->ppu.pixles[scanline];

    for (GB_U8 tile_x = 0; tile_x <= base_tile_x; ++tile_x) {

        // fetch the tile number and attributes
        const GB_U8 tile_num = vram_map[tile_x];
        const struct GB_BgAttributes attributes = attribute_map[tile_x];
        
        const GB_U16 offset = GB_get_tile_offset(gb,
            tile_num,
            // check if flip the y axis
            attributes.yflip ? 7 - sub_tile_y : sub_tile_y
        );

        const GB_U8 byte_a = GB_vram_read(gb, offset + 0, attributes.bank);
        const GB_U8 byte_b = GB_vram_read(gb, offset + 1, attributes.bank);

        const GB_U8* bit = attributes.xflip ? PIXEL_BIT_GROW :  PIXEL_BIT_SHRINK;

        for (GB_U8 x = 0; x < 8; ++x) {
            const GB_U8 pixel_x = ((tile_x << 3) + x + sub_tile_x) & 0xFF;
            if (pixel_x >= GB_SCREEN_WIDTH) {
                continue;
            }

            const GB_U8 pixel = ((!!(byte_b & bit[x])) << 1) | (!!(byte_a & bit[x]));
            pixels[pixel_x] = gb->ppu.bg_colours[attributes.pal][pixel];
        }
    }
}

static inline void GB_draw_obj_gbc(struct GB_Data* gb) {
    const GB_U8 scanline = IO_LY;
    const GB_U8 sprite_size = GB_get_sprite_size(gb);
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

            const GB_U8 byte_a = GB_vram_read(gb, offset + 0, sprite.flag.bank);
            const GB_U8 byte_b = GB_vram_read(gb, offset + 1, sprite.flag.bank);

            const GB_U8* bit = sprite.flag.xflip ? PIXEL_BIT_GROW : PIXEL_BIT_SHRINK;

            for (GB_U8 x = 0; x < 8; ++x) {
                if ((spx + x) < 0 || (sprite.flag.priority)) {
                    continue;
                }
                const GB_U8 pixel = ((!!(byte_b & bit[x])) << 1) | (!!(byte_a & bit[x]));
                
                if (pixel == 0) {
                    continue;
                }

                pixels[spx + x] = gb->ppu.obj_colours[sprite.flag.pal_gbc][pixel];
            }
        }
    }
}

static inline void GB_update_colours_gbc(GB_BOOL dirty[8], GB_U16 map[8][4], const GB_U8 palette_mem[64]) {
    for (GB_U8 palette = 0; palette < 8; ++palette) {
        if (dirty[palette] == GB_TRUE) {
            dirty[palette] = GB_FALSE;
            
            for (GB_U8 colours = 0, pos = 0; colours < 4; colours++, pos += 2) {
                const GB_U8 col_a = palette_mem[(palette << 3) + pos];
                const GB_U8 col_b = palette_mem[(palette << 3) + pos + 1];
                const GB_U16 pair = (col_b << 8) | col_a;

                map[palette & 7][colours & 3] = pair;
            }
        }
    }
}

void GB_draw_scanline(struct GB_Data* gb) {
    // for now, split DMG and GBC rendering into different functions
    // whilst this does bloat the codebase a little, it makes each function
    // more readable, and less branching in a hot loop

    if (GB_is_system_gbc(gb) == GB_TRUE) {
        // update the gbc colour palettes
        GB_update_colours_gbc(gb->ppu.dirty_bg, gb->ppu.bg_colours, gb->ppu.bg_palette);
        GB_update_colours_gbc(gb->ppu.dirty_obj, gb->ppu.obj_colours, gb->ppu.obj_palette);

        if (LIKELY(GB_is_bg_enabled(gb))) {
            GB_draw_bg_gb_gbc(gb);
            // WX=0..166, WY=0..143
            if ((GB_is_win_enabled(gb)) && (IO_WX <= 166) && (IO_WY <= 143) && (IO_WY <= IO_LY)) {
                GB_draw_win_gb_gbc(gb);
            }
        }

        if (LIKELY(GB_is_obj_enabled(gb))) {
            GB_draw_obj_gbc(gb);
        }
    }
    
    // DMG or SGB mode...
    else {
        // update the DMG colour palettes
        GB_update_colours_gb(gb->ppu.bg_colours[0], gb->palette.BG, IO_BGP, &gb->ppu.dirty_bg[0]);
        GB_update_colours_gb(gb->ppu.obj_colours[0], gb->palette.OBJ0, IO_OBP0, &gb->ppu.dirty_obj[0]);
        GB_update_colours_gb(gb->ppu.obj_colours[1], gb->palette.OBJ1, IO_OBP1, &gb->ppu.dirty_obj[1]);

        if (LIKELY(GB_is_bg_enabled(gb))) {
            GB_draw_bg_gb(gb);
            // WX=0..166, WY=0..143
            if ((GB_is_win_enabled(gb)) && (IO_WX <= 166) && (IO_WY <= 143) && (IO_WY <= IO_LY)) {
                GB_draw_win_gb(gb);
            }
        }

        if (LIKELY(GB_is_obj_enabled(gb))) {
            GB_draw_obj(gb);
        }
    }
}
