#include "gb.h"
#include "internal.h"
#include "tables/palette_table.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

static inline uint16_t GB_calculate_col_from_palette(const uint8_t palette, const uint8_t colour) {
    return ((palette >> (colour << 1)) & 3);
}

static inline uint8_t GB_vram_read(const struct GB_Core* gb, const uint16_t addr, const uint8_t bank) {
    assert(gb);
    assert(bank < 2);
    return gb->ppu.vram[bank][addr & 0x1FFF];
}

// data selects
static inline bool GB_get_bg_data_select(const struct GB_Core* gb) {
    return (!!(IO_LCDC & 0x08));
}

static inline bool GB_get_title_data_select(const struct GB_Core* gb) {
    return (!!(IO_LCDC & 0x10));
}

static inline bool GB_get_win_data_select(const struct GB_Core* gb) {
    return (!!(IO_LCDC & 0x40));
}

// map selects
static inline uint16_t GB_get_bg_map_select(const struct GB_Core* gb) {
    return GB_get_bg_data_select(gb) ? 0x9C00 : 0x9800;
}

static inline uint16_t GB_get_title_map_select(const struct GB_Core* gb) {
    return GB_get_title_data_select(gb) ? 0x8000 : 0x9000;
}

static inline uint16_t GB_get_win_map_select(const struct GB_Core* gb) {
    return GB_get_win_data_select(gb) ? 0x9C00 : 0x9800;
}

static inline uint16_t GB_get_tile_offset(const struct GB_Core* gb, const uint8_t tile_num, const uint8_t sub_tile_y) {
    return (GB_get_title_map_select(gb) + (((GB_get_title_data_select(gb) ? tile_num : (int8_t)tile_num)) << 4) + (sub_tile_y << 1));
}

static inline void GB_raise_if_enabled(struct GB_Core* gb, const uint8_t mode) {
    // IO_IF |= ((!!(IO_STAT & mode)) << 1);
    if (IO_STAT & mode) {
        GB_enable_interrupt(gb, GB_INTERRUPT_LCD_STAT);
    }
}

void GB_set_coincidence_flag(struct GB_Core* gb, const bool n) {
    IO_STAT = n ? IO_STAT | 0x04 : IO_STAT & ~0x04;
    // IO_STAT ^= (-(!!(n)) ^ IO_STAT) & 0x04;
}

void GB_set_status_mode(struct GB_Core* gb, const enum GB_StatusModes mode) {
    IO_STAT = (IO_STAT & 252) | mode;
}

enum GB_StatusModes GB_get_status_mode(const struct GB_Core* gb) {
    return (IO_STAT & 0x03);
}

bool GB_is_lcd_enabled(const struct GB_Core* gb) {
    return (!!(IO_LCDC & 0x80)); 
}

bool GB_is_win_enabled(const struct GB_Core* gb) {
    return (!!(IO_LCDC & 0x20)); 
}

bool GB_is_obj_enabled(const struct GB_Core* gb) {
    return (!!(IO_LCDC & 0x02)); 
}

bool GB_is_bg_enabled(const struct GB_Core* gb) {
    return (!!(IO_LCDC & 0x01)); 
}

static inline bool GB_is_bcps_auto_increment(const struct GB_Core* gb) {
    return (IO_BCPS & 0x80) > 0;
}

static inline bool GB_is_ocps_auto_increment(const struct GB_Core* gb) {
    return (IO_OCPS & 0x80) > 0;
}

static inline uint8_t GB_get_bcps_index(const struct GB_Core* gb) {
    return IO_BCPS & 0x3F;
}

static inline uint8_t GB_get_ocps_index(const struct GB_Core* gb) {
    return IO_OCPS & 0x3F;
}

static inline void GB_bcps_increment(struct GB_Core* gb) {
    if (GB_is_bcps_auto_increment(gb) == true) {
        // only increment the lower 5 bits.
        // set back the increment bit after.
        // printf("bcps auto inc\n");
        IO_BCPS = ((IO_BCPS + 1) & 0x3F) | (0x80);
    }
}

static inline void GB_ocps_increment(struct GB_Core* gb) {
    if (GB_is_ocps_auto_increment(gb) == true) {
        // only increment the lower 5 bits.
        // set back the increment bit after.
        // printf("ocps auto inc\n");
        IO_OCPS = ((IO_OCPS + 1) & 0x3F) | (0x80);
    }
}

void GB_bcpd_write(struct GB_Core* gb, uint8_t value) {
    const uint8_t index = GB_get_bcps_index(gb);

    // this is 0-7
    assert((index >> 3) <= 7);
    gb->ppu.dirty_bg[index >> 3] = true;

    gb->ppu.bg_palette[index] = value;
    GB_bcps_increment(gb);
}

void GB_ocpd_write(struct GB_Core* gb, uint8_t value) {
    const uint8_t index = GB_get_ocps_index(gb);

    // this is 0-7
    assert((index >> 3) <= 7);
    gb->ppu.dirty_obj[index >> 3] = true;

    gb->ppu.obj_palette[index] = value;
    GB_ocps_increment(gb);
}

static inline bool GB_is_hdma_active(const struct GB_Core* gb) {
    return gb->ppu.hdma_length > 0;
}

static inline uint8_t hdma_read(const struct GB_Core* gb, const uint16_t addr) { // A000-DFF0
    // assert((addr <= 0xDFF0) || (addr >= 0xA000 && addr <= 0xDFF0));

    return gb->mmap[(addr >> 12)][addr & 0x0FFF];
}

static inline void hdma_write(struct GB_Core* gb, const uint16_t addr, const uint8_t value) { // A000-DFF0
    gb->ppu.vram[IO_VBK][(addr) & 0x1FFF] = value;
}

static inline void GB_perform_hdma(struct GB_Core* gb) {
    assert(GB_is_hdma_active(gb) == true);

    // perform 16-block transfer
    for (uint16_t i = 0; i < 0x10; ++i) {
        hdma_write(gb,
            gb->ppu.hdma_dst_addr + i, // dst
            hdma_read(gb, gb->ppu.hdma_src_addr + i) // value
        );
    }

    gb->cpu.cycles += 8;
    gb->ppu.hdma_length -= 0x10;
    gb->ppu.hdma_src_addr += 0x10;
    gb->ppu.hdma_dst_addr += 0x10;
    --IO_HDMA5;

    // finished!
    if (gb->ppu.hdma_length == 0) {
        gb->ppu.hdma_length = 0;

        IO_HDMA1 = 0xFF;
        IO_HDMA2 = 0xFF;
        IO_HDMA3 = 0xFF;
        IO_HDMA4 = 0xFF;
        IO_HDMA5 = 0x80;
    }
}

uint8_t GB_hdma5_read(const struct GB_Core* gb) {
    return IO_HDMA5;
}

void GB_hdma5_write(struct GB_Core* gb, uint8_t value) {
    // the lower 4-bits of both address are ignored
    const uint16_t dma_src = (IO_HDMA1 << 8) | (IO_HDMA2 & 0xF0);
    const uint16_t dma_dst = (IO_HDMA3 << 8) | (IO_HDMA4 & 0xF0);

    // lower 6-bits are the length + 1 * 0x10
    const uint16_t dma_len = ((value & 0x7F) + 1) << 4;

    // by checking bit-7 of value, it returns the type of dma to perform.
    const uint8_t mode = value & 0x80;

    enum GB_HDMA5Mode {
        GDMA = 0x00,
        HDMA = 0x80
    };

    if (mode == GDMA) {
        // setting bit-7 = 0 whilst a HDMA is currently active
        // actually disables that transfer
        if (GB_is_hdma_active(gb) == true) {
            // GB_throw_info(gb, "cancleing HDMA");

            gb->ppu.hdma_length = 0;
            IO_HDMA5 = ((gb->ppu.hdma_length >> 4) - 1) | 0x80;

            // do not perform GDMA after, this only cancels the active
            // transfer and sets HDMA5.
            return;
        }

        // use this for testing, it gets noises however...
        // GB_throw_info(gb, "performing GDMA");

        // GDMA are performed immediately
        for (uint16_t i = 0; i < dma_len; ++i) {
            hdma_write(gb,
                dma_dst + i, // dst
                hdma_read(gb, dma_src + i) // value
            );
        }

        gb->cpu.cycles += (value & 0x7F) + 1;

        // it's unclear if all HDMA regs are set to 0xFF post transfer,
        // HDMA5 is, but not sure about the rest.
        IO_HDMA1 = 0xFF;
        IO_HDMA2 = 0xFF;
        IO_HDMA3 = 0xFF;
        IO_HDMA4 = 0xFF;
        IO_HDMA5 = 0x80;
    }
    else {
        // GB_throw_info(gb, "performing HDMA");

        gb->ppu.hdma_src_addr = dma_src;
        gb->ppu.hdma_dst_addr = dma_dst;
        gb->ppu.hdma_length = dma_len;

        // set that the transfer is active.
        IO_HDMA5 = value & 0x7F;
    }
}

void GB_compare_LYC(struct GB_Core* gb) {
    if (UNLIKELY(IO_LY == IO_LYC)) {
        GB_set_coincidence_flag(gb, true);
        GB_raise_if_enabled(gb, STAT_INT_MODE_COINCIDENCE);
    } else {
        GB_set_coincidence_flag(gb, false);
    }
}

void GB_change_status_mode(struct GB_Core* gb, const uint8_t new_mode) {
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

void GB_on_lcdc_write(struct GB_Core* gb, const uint8_t value) {
    // check if the game wants to disable the ppu
    if ((value & 0x80) == 0) {
        // we need to set a few vars, LY is reset and stat mode is HBLANK
        IO_STAT &= ~(0x3);
        IO_LY = 0;
        // i think this is reset also...
        gb->ppu.next_cycles = 0;
        printf("disabling ppu...\n");
    }
    else {
        // if the value is enabling the ppu and the ppu is
        // currently disabled, something else is meant to happen
        // i think...not handled yet anyway
        if (GB_is_lcd_enabled(gb) == false) {

        }
    }

    IO_LCDC = value;
}

void GB_ppu_run(struct GB_Core* gb, uint16_t cycles) {
    if (UNLIKELY(!GB_is_lcd_enabled(gb))) {
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

        if (GB_is_hdma_active(gb) == true) {
            GB_perform_hdma(gb);
        }

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
                gb->ppu.window_line = 0;
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

void GB_DMA(struct GB_Core* gb) {
    assert(IO_DMA <= 0xF1);

    memcpy(gb->ppu.oam, gb->mmap[IO_DMA >> 4] + ((IO_DMA & 0xF) << 8), sizeof(gb->ppu.oam));
	gb->cpu.cycles += 646;
}

void GB_update_colours_gb(uint16_t colours[4], const uint16_t pal_colours[4], const uint8_t palette, bool* dirty) {
    assert(colours && pal_colours && dirty);

    if (*dirty) {
        *dirty = false;
        for (uint8_t i = 0; i < 4; ++i) {
            colours[i] = pal_colours[GB_calculate_col_from_palette(palette, i)];
        }
    }
}

void GB_update_all_colours_gb(struct GB_Core* gb) {
    assert(gb);

    // the colour palette will be auto updated next frame.
    gb->ppu.dirty_bg[0] = true;
    gb->ppu.dirty_obj[0] = true;
    gb->ppu.dirty_obj[1] = true;
}

static const uint8_t PIXEL_BIT_SHRINK[] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };
static const uint8_t PIXEL_BIT_GROW[] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };

static inline void GB_draw_bg_gb(struct GB_Core* gb) {
    const uint8_t scanline = IO_LY;
    const uint8_t base_tile_x = IO_SCX >> 3;
    const uint8_t sub_tile_x = (IO_SCX & 7);
    const uint8_t pixel_y = (scanline + IO_SCY);
    const uint8_t tile_y = pixel_y >> 3;
    const uint8_t sub_tile_y = (pixel_y & 7);

    const uint8_t* bit = PIXEL_BIT_SHRINK;
    const uint8_t *vram_map = &gb->ppu.vram[0][(GB_get_bg_map_select(gb) + (tile_y << 5)) & 0x1FFF];
    uint16_t* pixels = gb->ppu.pixles[scanline];

    for (uint8_t tile_x = 0; tile_x <= 20; ++tile_x) {
        const uint8_t map_x = ((base_tile_x + tile_x) & 31);

        const uint8_t tile_num = vram_map[map_x];
        const uint16_t offset = GB_get_tile_offset(gb, tile_num, sub_tile_y);

        const uint8_t byte_a = GB_vram_read(gb, offset + 0, 0);
        const uint8_t byte_b = GB_vram_read(gb, offset + 1, 0);

        for (uint8_t x = 0; x < 8; ++x) {
            const uint8_t pixel_x = ((tile_x << 3) + x - sub_tile_x) & 0xFF;
            if (pixel_x >= GB_SCREEN_WIDTH) {
                continue;
            }

            const uint8_t pixel = ((!!(byte_b & bit[x])) << 1) | (!!(byte_a & bit[x]));
            pixels[pixel_x] = gb->ppu.bg_colours[0][pixel];
        }
    }
}

static inline void GB_draw_win_gb(struct GB_Core* gb) {
    const uint8_t scanline = IO_LY;
    const uint8_t base_tile_x = 20 - (IO_WX >> 3);
    const int16_t sub_tile_x = IO_WX - 7;
    const uint8_t pixel_y = gb->ppu.window_line;
    const uint8_t tile_y = pixel_y >> 3;
    const uint8_t sub_tile_y = (pixel_y & 7);

    bool did_draw = false;

    const uint8_t* bit = PIXEL_BIT_SHRINK;
    const uint8_t *vram_map = &gb->ppu.vram[0][(GB_get_win_map_select(gb) + (tile_y << 5)) & 0x1FFF];
    uint16_t* pixels = gb->ppu.pixles[scanline];

    for (uint8_t tile_x = 0; tile_x <= base_tile_x; ++tile_x) {
        const uint8_t tile_num = vram_map[tile_x];
        const uint16_t offset = GB_get_tile_offset(gb, tile_num, sub_tile_y);

        const uint8_t byte_a = GB_vram_read(gb, offset + 0, 0);
        const uint8_t byte_b = GB_vram_read(gb, offset + 1, 0);

        for (uint8_t x = 0; x < 8; ++x) {
            const uint8_t pixel_x = ((tile_x << 3) + x + sub_tile_x) & 0xFF;
            if (pixel_x >= GB_SCREEN_WIDTH) {
                continue;
            }

            did_draw |= 1;

            const uint8_t pixel = ((!!(byte_b & bit[x])) << 1) | (!!(byte_a & bit[x]));
            pixels[pixel_x] = gb->ppu.bg_colours[0][pixel];
        }
    }

    
    gb->ppu.window_line += did_draw;
}

static inline uint8_t GB_get_sprite_size(const struct GB_Core* gb) {
    return ((IO_LCDC & 0x04) ? 16 : 8);
}

#include <stdlib.h>

static int sprite_comp(const void* a, const void* b) {
    const struct GB_Sprite* sprite_a = (const struct GB_Sprite*)a;
    const struct GB_Sprite* sprite_b = (const struct GB_Sprite*)b;

    // if (sprite_a->x > sprite_b->x) {
    //     return -1;
    // } else if (sprite_a->x == sprite_b->x) {
    //     return -1;
    // } else {
    //     return 1;
    // }
    return sprite_b->x - sprite_a->x;
}

static inline void GB_draw_obj(struct GB_Core* gb) {
    const uint8_t scanline = IO_LY;
    const uint8_t sprite_size = GB_get_sprite_size(gb);
    const uint16_t bg_trans_col = gb->ppu.bg_colours[0][0];
    uint16_t* pixels = gb->ppu.pixles[scanline];

    struct GB_Sprite sprites[40];
    memcpy(sprites, gb->ppu.sprites, sizeof(struct GB_Sprite) * 40);
    qsort(sprites, 40, sizeof(struct GB_Sprite), sprite_comp);
    // return;
    for (uint8_t i = 0, sprite_total = 0; i < 40 && sprite_total < 10; ++i) {
        const struct GB_Sprite sprite = sprites[i];
        const int16_t spy = (int16_t)sprite.y - 16;
        const int16_t spx = (int16_t)sprite.x - 8;

        // if (gb->ppu.sprites[i].x == 0x38 && gb->ppu.sprites[i].y == 0x68 && gb->ppu.sprites[i].num == 0x0C) {
        //     printf("addr is %u\n", i);
        //     draw = 1;
        // } else {
        //     draw = 0;
        // }
        // if (gb->ppu.oam[(0xFE4C - 0xFE00)] == 0x68 && gb->ppu.oam[(0xFE4D - 0xFE00)] == 0x38) {
        //     printf("addr is %u\n", i);
        // }

        if (scanline >= spy && scanline < (spy + (sprite_size))) {
            ++sprite_total;
            if ((spx + 8) == 0 || spx >= GB_SCREEN_WIDTH) {
                continue;
            }


            const uint8_t sprite_line = sprite.flag.yflip ? sprite_size - 1 - (scanline - spy) : scanline - spy;
            // when in 8x16 size, bit-0 is ignored of the tile_index
            const uint8_t tile_index = sprite_size == 16 ? sprite.num & 0xFE : sprite.num;
            const uint16_t offset = 0x8000 | (((sprite_line) << 1) + (tile_index << 4));

            // if (gb->ppu.sprites[i].x == 0x50 && gb->ppu.sprites[i].y == 0x68 && gb->ppu.sprites[i].num == 0x0C) {
            //     printf("addr is %u spx %d spy %d\n", i, spx, spy);
            //     draw = 1;
            // } else {
            //     draw = 0;
            // }
            
            const uint8_t byte_a = GB_vram_read(gb, offset + 0, 0);
            const uint8_t byte_b = GB_vram_read(gb, offset + 1, 0);

            const uint8_t* bit = sprite.flag.xflip ? PIXEL_BIT_GROW : PIXEL_BIT_SHRINK;

            for (uint8_t x = 0; x < 8; ++x) {
                if ((spx + x) < 0 || (sprite.flag.priority && pixels[spx + x] != bg_trans_col)) {
                    continue;
                }
                const uint8_t pixel = ((!!(byte_b & bit[x])) << 1) | (!!(byte_a & bit[x]));
                
                if (pixel == 0) {
                    continue;
                }

                // if (draw) {
                //     printf("drawing 0x%04X\n", gb->ppu.obj_colours[sprite.flag.pal_gb][pixel]);
                // }

                pixels[spx + x] = gb->ppu.obj_colours[sprite.flag.pal_gb][pixel];
            }
        }
    }
}

static inline void GB_draw_bg_gb_gbc(struct GB_Core* gb) {
    const uint8_t scanline = IO_LY;
    const uint8_t base_tile_x = IO_SCX >> 3;
    const uint8_t sub_tile_x = (IO_SCX & 7);
    const uint8_t pixel_y = (scanline + IO_SCY);
    const uint8_t tile_y = pixel_y >> 3;
    const uint8_t sub_tile_y = (pixel_y & 7);

    // array maps
    const uint8_t* vram_map = &gb->ppu.vram[0][(GB_get_bg_map_select(gb) + (tile_y << 5)) & 0x1FFF];
    const struct GB_BgAttributes* attribute_map = &gb->ppu.bg_attributes[1][(GB_get_bg_map_select(gb) + (tile_y << 5)) & 0x1FFF];
    uint16_t* pixels = gb->ppu.pixles[scanline];

    for (uint8_t tile_x = 0; tile_x <= 20; ++tile_x) {
        // calc the map index
        const uint8_t map_x = ((base_tile_x + tile_x) & 31);
        
        // fetch the tile number and attributes
        const uint8_t tile_num = vram_map[map_x];
        const struct GB_BgAttributes attributes = attribute_map[map_x];

        const uint16_t offset = GB_get_tile_offset(gb,
            tile_num,
            // check if flip the y axis
            attributes.yflip ? 7 - sub_tile_y : sub_tile_y
        );

        const uint8_t byte_a = GB_vram_read(gb, offset + 0, attributes.bank);
        const uint8_t byte_b = GB_vram_read(gb, offset + 1, attributes.bank);

        const uint8_t* bit = attributes.xflip ? PIXEL_BIT_GROW : PIXEL_BIT_SHRINK;

        for (uint8_t x = 0; x < 8; ++x) {
            const uint8_t pixel_x = ((tile_x << 3) + x - sub_tile_x) & 0xFF;
            if (pixel_x >= GB_SCREEN_WIDTH) {
                continue;
            }

            const uint8_t pixel = ((!!(byte_b & bit[x])) << 1) | (!!(byte_a & bit[x]));
            pixels[pixel_x] = gb->ppu.bg_colours[attributes.pal][pixel];
        }
    }
}

static inline void GB_draw_win_gb_gbc(struct GB_Core* gb) {
    const uint8_t scanline = IO_LY;
    const uint8_t base_tile_x = 20 - (IO_WX >> 3);
    const int16_t sub_tile_x = IO_WX - 7;
    const uint8_t pixel_y = gb->ppu.window_line;
    const uint8_t tile_y = pixel_y >> 3;
    const uint8_t sub_tile_y = (pixel_y & 7);

    bool did_draw = false;

    const uint8_t *vram_map = &gb->ppu.vram[0][(GB_get_win_map_select(gb) + (tile_y << 5)) & 0x1FFF];
    const struct GB_BgAttributes* attribute_map = &gb->ppu.bg_attributes[1][(GB_get_win_map_select(gb) + (tile_y << 5)) & 0x1FFF];
    uint16_t* pixels = gb->ppu.pixles[scanline];

    for (uint8_t tile_x = 0; tile_x <= base_tile_x; ++tile_x) {

        // fetch the tile number and attributes
        const uint8_t tile_num = vram_map[tile_x];
        const struct GB_BgAttributes attributes = attribute_map[tile_x];
        
        const uint16_t offset = GB_get_tile_offset(gb,
            tile_num,
            // check if flip the y axis
            attributes.yflip ? 7 - sub_tile_y : sub_tile_y
        );

        const uint8_t byte_a = GB_vram_read(gb, offset + 0, attributes.bank);
        const uint8_t byte_b = GB_vram_read(gb, offset + 1, attributes.bank);

        const uint8_t* bit = attributes.xflip ? PIXEL_BIT_GROW :  PIXEL_BIT_SHRINK;

        for (uint8_t x = 0; x < 8; ++x) {
            const uint8_t pixel_x = ((tile_x << 3) + x + sub_tile_x) & 0xFF;
            if (pixel_x >= GB_SCREEN_WIDTH) {
                continue;
            }

            did_draw |= 1;

            const uint8_t pixel = ((!!(byte_b & bit[x])) << 1) | (!!(byte_a & bit[x]));
            pixels[pixel_x] = gb->ppu.bg_colours[attributes.pal][pixel];
        }
    }

    gb->ppu.window_line += did_draw;
}

static inline void GB_draw_obj_gbc(struct GB_Core* gb) {
    const uint8_t scanline = IO_LY;
    const uint8_t sprite_size = GB_get_sprite_size(gb);
    const bool bg_prio = (IO_LCDC & 0x1) > 0;
    uint16_t* pixels = gb->ppu.pixles[scanline];

    for (uint8_t i = 0, sprite_total = 0; i < 40 && sprite_total < 10; ++i) {
        const struct GB_Sprite sprite = gb->ppu.sprites[i];
        const int16_t spy = (int16_t)sprite.y - 16;
        const int16_t spx = (int16_t)sprite.x - 8;

        if (scanline >= spy && scanline < (spy + (sprite_size))) {
            ++sprite_total;
            if ((spx + 8) == 0 || spx >= GB_SCREEN_WIDTH) {
                continue;
            }

            const uint8_t sprite_line = sprite.flag.yflip ? sprite_size - 1 - (scanline - spy) : scanline - spy;
            // when in 8x16 size, bit-0 is ignored of the tile_index
            const uint8_t tile_index = sprite_size == 16 ? sprite.num & 0xFE : sprite.num;
            const uint16_t offset = (((sprite_line) << 1) + (tile_index << 4));

            const uint8_t byte_a = GB_vram_read(gb, offset + 0, sprite.flag.bank);
            const uint8_t byte_b = GB_vram_read(gb, offset + 1, sprite.flag.bank);

            const uint8_t* bit = sprite.flag.xflip ? PIXEL_BIT_GROW : PIXEL_BIT_SHRINK;

            for (uint8_t x = 0; x < 8; ++x) {
                if ((spx + x) < 0 || (sprite.flag.priority && bg_prio)) {
                    continue;
                }
                const uint8_t pixel = ((!!(byte_b & bit[x])) << 1) | (!!(byte_a & bit[x]));
                
                if (pixel == 0) {
                    continue;
                }

                pixels[spx + x] = gb->ppu.obj_colours[sprite.flag.pal_gbc][pixel];
            }
        }
    }
}

static inline void GB_update_colours_gbc(bool dirty[8], uint16_t map[8][4], const uint8_t palette_mem[64]) {
    for (uint8_t palette = 0; palette < 8; ++palette) {
        if (dirty[palette] == true) {
            dirty[palette] = false;
            
            for (uint8_t colours = 0, pos = 0; colours < 4; colours++, pos += 2) {
                const uint8_t col_a = palette_mem[(palette << 3) + pos];
                const uint8_t col_b = palette_mem[(palette << 3) + pos + 1];
                const uint16_t pair = (col_b << 8) | col_a;

                map[palette & 7][colours & 3] = pair;
            }
        }
    }
}

static inline bool GB_is_render_layer_enabled(const struct GB_Core* gb, enum GB_RenderLayerConfig want) {
    return (gb->config.render_layer_config == GB_RENDER_LAYER_CONFIG_ALL) || ((gb->config.render_layer_config & want) > 0);
}

void GB_draw_scanline(struct GB_Core* gb) {
    // for now, split DMG and GBC rendering into different functions
    // whilst this does bloat the codebase a little, it makes each function
    // more readable, and less branching in a hot loop

    if (GB_is_system_gbc(gb) == true) {
        // update the gbc colour palettes
        GB_update_colours_gbc(gb->ppu.dirty_bg, gb->ppu.bg_colours, gb->ppu.bg_palette);
        GB_update_colours_gbc(gb->ppu.dirty_obj, gb->ppu.obj_colours, gb->ppu.obj_palette);

        if (LIKELY(GB_is_bg_enabled(gb))) {
            if (GB_is_render_layer_enabled(gb, GB_RENDER_LAYER_CONFIG_BG)) {
                GB_draw_bg_gb_gbc(gb);
            }
            // WX=0..166, WY=0..143
            if ((GB_is_win_enabled(gb)) && (IO_WX <= 166) && (IO_WY <= 143) && (IO_WY <= IO_LY)) {
                if (GB_is_render_layer_enabled(gb, GB_RENDER_LAYER_CONFIG_WIN)) {
                    GB_draw_win_gb_gbc(gb);
                }
            }
        }

        if (LIKELY(GB_is_obj_enabled(gb))) {
            if (GB_is_render_layer_enabled(gb, GB_RENDER_LAYER_CONFIG_OBJ)) {
                GB_draw_obj_gbc(gb);
            }
        }
    }
    
    // DMG or SGB mode...
    else {
        // update the DMG colour palettes
        GB_update_colours_gb(gb->ppu.bg_colours[0], gb->palette.BG, IO_BGP, &gb->ppu.dirty_bg[0]);
        GB_update_colours_gb(gb->ppu.obj_colours[0], gb->palette.OBJ0, IO_OBP0, &gb->ppu.dirty_obj[0]);
        GB_update_colours_gb(gb->ppu.obj_colours[1], gb->palette.OBJ1, IO_OBP1, &gb->ppu.dirty_obj[1]);

        if (LIKELY(GB_is_bg_enabled(gb))) {
            if (GB_is_render_layer_enabled(gb, GB_RENDER_LAYER_CONFIG_BG)) {
                GB_draw_bg_gb(gb);
            }

            // WX=0..166, WY=0..143
            if ((GB_is_win_enabled(gb)) && (IO_WX <= 166) && (IO_WY <= 143) && (IO_WY <= IO_LY)) {
                if (GB_is_render_layer_enabled(gb, GB_RENDER_LAYER_CONFIG_WIN)) {
                    GB_draw_win_gb(gb);
                }
            }
        }

        if (LIKELY(GB_is_obj_enabled(gb))) {
            if (GB_is_render_layer_enabled(gb, GB_RENDER_LAYER_CONFIG_OBJ)) {
                GB_draw_obj(gb);
            }
        }
    }
}
