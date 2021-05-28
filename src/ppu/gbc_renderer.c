#include "ppu.h"
#include "../internal.h"
#include "../gb.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>


// store a 1 when bg / win writes to the screen
// this is used by obj rendering to check firstly if
// the bg always has priority.
// if it does, then it checks this buffer for a 1
// at the same xpos, if its 1, rendering that pixel is skipped.
struct PrioBuf {
    // 1 = prio, 0 = no prio
    bool prio[GB_SCREEN_WIDTH];
    // 0-3
    uint8_t colour_id[GB_SCREEN_WIDTH];
};


static inline bool is_bcps_auto_increment(const struct GB_Core* gb) {
    return (IO_BCPS & 0x80) > 0;
}

static inline bool is_ocps_auto_increment(const struct GB_Core* gb) {
    return (IO_OCPS & 0x80) > 0;
}

static inline uint8_t get_bcps_index(const struct GB_Core* gb) {
    return IO_BCPS & 0x3F;
}

static inline uint8_t get_ocps_index(const struct GB_Core* gb) {
    return IO_OCPS & 0x3F;
}

static inline void bcps_increment(struct GB_Core* gb) {
    if (is_bcps_auto_increment(gb) == true) {
        // only increment the lower 5 bits.
        // set back the increment bit after.
        IO_BCPS = ((IO_BCPS + 1) & 0x3F) | (0xC0);
    }
}

static inline void ocps_increment(struct GB_Core* gb) {
    if (is_ocps_auto_increment(gb) == true) {
        // only increment the lower 5 bits.
        // set back the increment bit after.
        IO_OCPS = ((IO_OCPS + 1) & 0x3F) | (0xC0);
    }
}

void GB_bcpd_write(struct GB_Core* gb, uint8_t value) {
    const uint8_t index = get_bcps_index(gb);

    // this is 0-7
    assert((index >> 3) <= 7);
    gb->ppu.dirty_bg[index >> 3] = true;

    gb->ppu.bg_palette[index] = value;
    bcps_increment(gb);
}

uint8_t GBC_bcpd_read(struct GB_Core* gb) {
    const uint8_t index = get_bcps_index(gb);
    return gb->ppu.bg_palette[index];
}

uint8_t GBC_ocpd_read(struct GB_Core* gb) {
    const uint8_t index = get_ocps_index(gb);
    return gb->ppu.obj_palette[index];
}

void GB_ocpd_write(struct GB_Core* gb, uint8_t value) {
    const uint8_t index = get_ocps_index(gb);

    // this is 0-7
    assert((index >> 3) <= 7);
    gb->ppu.dirty_obj[index >> 3] = true;

    gb->ppu.obj_palette[index] = value;
    ocps_increment(gb);
}

bool GB_is_hdma_active(const struct GB_Core* gb) {
    return gb->ppu.hdma_length > 0;
}

uint8_t hdma_read(const struct GB_Core* gb, const uint16_t addr) {
    const struct GB_MemMapEntry entry = gb->mmap[(addr >> 12)];
    return entry.ptr[addr & entry.mask];
}

void hdma_write(struct GB_Core* gb,
    const uint16_t addr, const uint8_t value
) {
    gb->ppu.vram[IO_VBK][(addr) & 0x1FFF] = value;
}

void perform_hdma(struct GB_Core* gb) {
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
        IO_HDMA5 = 0xFF;
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
            gb->ppu.hdma_length = 0;
            IO_HDMA5 = ((gb->ppu.hdma_length >> 4) - 1) | 0x80;

            // do not perform GDMA after, this only cancels the active
            // transfer and sets HDMA5.
            return;
        }

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
        IO_HDMA5 = 0xFF;
    }
    else {
        gb->ppu.hdma_src_addr = dma_src;
        gb->ppu.hdma_dst_addr = dma_dst;
        gb->ppu.hdma_length = dma_len;

        // set that the transfer is active.
        IO_HDMA5 = value & 0x7F;
    }
}

static inline void render_scanline_bg(struct GB_Core* gb,
    struct PrioBuf* prio_buffer
) {
    const uint8_t scanline = IO_LY;
    const uint8_t base_tile_x = IO_SCX >> 3;
    const uint8_t sub_tile_x = (IO_SCX & 7);
    const uint8_t pixel_y = (scanline + IO_SCY);
    const uint8_t tile_y = pixel_y >> 3;
    const uint8_t sub_tile_y = (pixel_y & 7);

    // array maps
    const uint8_t* vram_map = &gb->ppu.vram[0][(GB_get_bg_map_select(gb) + (tile_y << 5)) & 0x1FFF];
    const struct GB_BgAttributes* attribute_map = (struct GB_BgAttributes*)&gb->ppu.vram[1][(GB_get_bg_map_select(gb) + (tile_y << 5)) & 0x1FFF];
    
    struct GB_Pixels pixels = get_pixels_at_scanline(gb, scanline);


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

            const uint8_t colour_id = ((!!(byte_b & bit[x])) << 1) | (!!(byte_a & bit[x]));

            // set priority
            prio_buffer->prio[pixel_x] = attributes.priority;
            prio_buffer->colour_id[pixel_x] = colour_id;

            pixels.p[pixel_x] = gb->ppu.bg_colours[attributes.pal][colour_id];
        }
    }
}

static inline void render_scanline_win(struct GB_Core* gb,
    struct PrioBuf* prio_buffer
) {
    const uint8_t scanline = IO_LY;
    const uint8_t base_tile_x = 20 - (IO_WX >> 3);
    const int16_t sub_tile_x = IO_WX - 7;
    const uint8_t pixel_y = gb->ppu.window_line;
    const uint8_t tile_y = pixel_y >> 3;
    const uint8_t sub_tile_y = (pixel_y & 7);

    bool did_draw = false;

    const uint8_t *vram_map = &gb->ppu.vram[0][(GB_get_win_map_select(gb) + (tile_y << 5)) & 0x1FFF];
    const struct GB_BgAttributes* attribute_map = (struct GB_BgAttributes*)&gb->ppu.vram[1][(GB_get_win_map_select(gb) + (tile_y << 5)) & 0x1FFF];

    struct GB_Pixels pixels = get_pixels_at_scanline(gb, scanline);


    for (uint8_t tile_x = 0; tile_x <= base_tile_x; ++tile_x) {

        // fetch the tile number and attributes
        const uint8_t tile_num = vram_map[tile_x];
        const struct GB_BgAttributes attributes = attribute_map[tile_x];

        const uint16_t offset = GB_get_tile_offset(gb,
            tile_num,
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

            did_draw |= true;

            const uint8_t colour_id = ((!!(byte_b & bit[x])) << 1) | (!!(byte_a & bit[x]));

            // set priority
            prio_buffer->prio[pixel_x] = attributes.priority;
            prio_buffer->colour_id[pixel_x] = colour_id;

            pixels.p[pixel_x] = gb->ppu.bg_colours[attributes.pal][colour_id];
        }
    }

    if (did_draw) {
        ++gb->ppu.window_line;
    }
}

static inline void render_scanline_obj(struct GB_Core* gb,
    const struct PrioBuf* prio_buffer
) {
    const uint8_t scanline = IO_LY;
    const uint8_t sprite_size = GB_get_sprite_size(gb);

    // check if the bg always has prio over obj
    const bool bg_prio = (IO_LCDC & 0x1) > 0;

    //
    struct GB_Pixels pixels = get_pixels_at_scanline(gb, scanline);

    const struct GB_Sprite* sprites = (const struct GB_Sprite*)gb->ppu.oam;

    // gbc uses oam prio rather than x-pos
    // so we need to keep track if a obj has already
    // been written to from a previous oam entry!
    bool oam_priority[GB_SCREEN_WIDTH] = {0};

    for (uint8_t i = 0, sprite_total = 0; i < 40 && sprite_total < 10; ++i) {
        const int16_t spy = (int16_t)sprites[i].y - 16;
        const int16_t spx = (int16_t)sprites[i].x - 8;

        if (scanline >= spy && scanline < (spy + (sprite_size))) {
            ++sprite_total;

            if ((spx + 8) == 0 || spx >= GB_SCREEN_WIDTH) {
                continue;
            }

            const uint8_t sprite_line = sprites[i].flag.yflip ? sprite_size - 1 - (scanline - spy) : scanline - spy;
            // when in 8x16 size, bit-0 is ignored of the tile_index
            const uint8_t tile_index = sprite_size == 16 ? sprites[i].num & 0xFE : sprites[i].num;
            const uint16_t offset = (((sprite_line) << 1) + (tile_index << 4));

            const uint8_t byte_a = GB_vram_read(gb, offset + 0, sprites[i].flag.bank);
            const uint8_t byte_b = GB_vram_read(gb, offset + 1, sprites[i].flag.bank);

            const uint8_t* bit = sprites[i].flag.xflip ? PIXEL_BIT_GROW : PIXEL_BIT_SHRINK;

            for (int8_t x = 0; x < 8; ++x) {
                const int16_t x_index = spx + x;

                // ensure that we are in bounds
                if (x_index < 0 || x_index >= GB_SCREEN_WIDTH) {
                    continue;
                }

                // check if this has already been written to
                // from a previous oam entry
                if (oam_priority[x_index]) {
                    continue;
                }

                const uint8_t colour_id = ((!!(byte_b & bit[x])) << 1) | (!!(byte_a & bit[x]));

                // this tests if the obj is transparrent
                if (colour_id == 0) {
                    continue;
                }

                if (bg_prio == 1) {
                    // this tests if bg always has priority over obj
                    if (prio_buffer->prio[x_index] && prio_buffer->colour_id[x_index]) {
                        continue;
                    }

                    // this tests if bg col 1-3 has priority,
                    // then checks if the col is non-zero, if yes, skip
                    if (sprites[i].flag.priority && prio_buffer->colour_id[x_index]) {
                        continue;
                    }
                }

                // save that we have already written to this xpos so the next
                // oam entries cannot overwrite this pixel!
                oam_priority[x_index] = true;

                // write the pixel (finally!)
                pixels.p[x_index] = gb->ppu.obj_colours[sprites[i].flag.pal_gbc][colour_id];
            }
        }
    }
}

static inline void update_colours(
    bool dirty[8], uint32_t map[8][4], const uint8_t palette_mem[64]
) {
    for (uint8_t palette = 0; palette < 8; ++palette) {
        if (dirty[palette] == true) {
            dirty[palette] = false;

            for (uint8_t colours = 0, pos = 0; colours < 4; colours++, pos += 2) {

                const uint8_t col_a = palette_mem[(palette << 3) + pos];
                const uint8_t col_b = palette_mem[(palette << 3) + pos + 1];
                const uint16_t pair = (col_b << 8) | col_a;

                #if 1
                map[palette][colours] = pair;
                #else
                // needed for when using
                // GL_BGBA AND GL_UNSIGNED_SHORT_5_5_5_1,
                // this also probably depends on the sys endian
                map[palette][colours] = (pair << 1) | 1;
                #endif
            }
        }
    }
}


void GBC_render_scanline(struct GB_Core* gb) {
    struct PrioBuf prio_buffer = {0};

    // update the bg colour palettes
    update_colours(gb->ppu.dirty_bg, gb->ppu.bg_colours, gb->ppu.bg_palette);
    // update the obj colour palettes
    update_colours(gb->ppu.dirty_obj, gb->ppu.obj_colours, gb->ppu.obj_palette);

    if (LIKELY(GB_is_render_layer_enabled(gb, GB_RENDER_LAYER_CONFIG_BG))) {
        render_scanline_bg(gb, &prio_buffer);
    }

    // WX=0..166, WY=0..143
    if ((GB_is_win_enabled(gb)) && (IO_WX <= 166) && (IO_WY <= 143) && (IO_WY <= IO_LY)) {
        if (LIKELY(GB_is_render_layer_enabled(gb, GB_RENDER_LAYER_CONFIG_WIN))) {
            render_scanline_win(gb, &prio_buffer);
        }
    }

    if (LIKELY(GB_is_obj_enabled(gb))) {
        if (LIKELY(GB_is_render_layer_enabled(gb, GB_RENDER_LAYER_CONFIG_OBJ))) {
            render_scanline_obj(gb, &prio_buffer);
        }
    }
}
