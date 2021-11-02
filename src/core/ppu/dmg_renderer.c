#include "ppu.h"
#include "../internal.h"
#include "../gb.h"

#include <string.h>
#include <assert.h>


#define DMG_PPU gb->ppu.system.dmg


struct DmgPrioBuf
{
    // 0-3
    uint8_t colour_id[GB_SCREEN_WIDTH];
};

static FORCE_INLINE uint16_t calculate_col_from_palette(const uint8_t palette, const uint8_t colour)
{
    return ((palette >> (colour << 1)) & 3);
}

static void dmg_update_colours(struct GB_PalCache cache[20], uint32_t colours[20][4], bool* dirty, const uint32_t pal_colours[4], const uint8_t palette_reg)
{
    if (*dirty == false)
    {
        // reset cache so old entries don't persist
        memset(cache, 0, sizeof(struct GB_PalCache) * 20);
        return;
    }

    *dirty = false;
    uint8_t palette = palette_reg;

    for (uint8_t i = 0; i < 20; ++i)
    {
        if (cache[i].used)
        {
            // this is needed for now, see prehistorik man text intro
            *dirty = true;

            cache[i].used = false;
            palette = cache[i].pal;
        }

        for (uint8_t j = 0; j < 4; ++j)
        {
            colours[i][j] = pal_colours[calculate_col_from_palette(palette, j)];
        }
    }
}

static FORCE_INLINE void on_dmg_palette_write(const struct GB_Core* gb, struct GB_PalCache cache[20], bool* dirty, uint8_t palette, uint8_t value)
{
    *dirty |= palette != value;

    if (GB_get_status_mode(gb) == 3 && PPU.next_cycles >= 12 && PPU.next_cycles <= 172)
    {
        const uint8_t index = (172 - PPU.next_cycles) / 8;

        cache[index].used = true;
        cache[index].pal = value;
    }
}

void on_bgp_write(struct GB_Core* gb, uint8_t value)
{
    if (!GB_is_system_gbc(gb))
    {
        on_dmg_palette_write(gb, DMG_PPU.bg_cache, &PPU.dirty_bg[0], IO_BGP, value);
    }

    IO_BGP = value;
}

void on_obp0_write(struct GB_Core* gb, uint8_t value)
{
    if (!GB_is_system_gbc(gb))
    {
        on_dmg_palette_write(gb, DMG_PPU.obj_cache[0], &PPU.dirty_obj[0], IO_OBP0, value);
    }

    IO_OBP0 = value;
}

void on_obp1_write(struct GB_Core* gb, uint8_t value)
{
    if (!GB_is_system_gbc(gb))
    {
        on_dmg_palette_write(gb, DMG_PPU.obj_cache[1], &PPU.dirty_obj[1], IO_OBP1, value);
    }

    IO_OBP1 = value;
}

struct DMG_SpriteAttribute
{
    bool prio;
    bool yflip;
    bool xflip;
    uint8_t pal; // only 0,1. not set as bool as its not a flag
};

struct DMG_Sprite
{
    int16_t y;
    int16_t x;
    uint8_t i;
    struct DMG_SpriteAttribute a;
};

struct DMG_Sprites
{
    struct DMG_Sprite sprite[10];
    uint8_t count;
};

static FORCE_INLINE struct DMG_SpriteAttribute dmg_get_sprite_attr(const uint8_t v)
{
    return (struct DMG_SpriteAttribute)
    {
        .prio   = (v & 0x80) > 0,
        .yflip  = (v & 0x40) > 0,
        .xflip  = (v & 0x20) > 0,
        .pal    = (v & 0x10) > 0,
    };
}

static inline struct DMG_Sprites dmg_sprite_fetch(const struct GB_Core* gb)
{
    struct DMG_Sprites sprites = {0};

    const uint8_t sprite_size = GB_get_sprite_size(gb);
    const uint8_t ly = IO_LY;

    for (size_t i = 0; i < ARRAY_SIZE(gb->ppu.oam); i += 4)
    {
        const int16_t sprite_y = gb->ppu.oam[i + 0] - 16;

        // check if the y is in bounds!
        if (ly >= sprite_y && ly < (sprite_y + sprite_size))
        {
            struct DMG_Sprite* sprite = &sprites.sprite[sprites.count];

            sprite->y = sprite_y;
            sprite->x = gb->ppu.oam[i + 1] - 8;
            sprite->i = gb->ppu.oam[i + 2];
            sprite->a = dmg_get_sprite_attr(gb->ppu.oam[i + 3]);

            ++sprites.count;

            // only 10 sprites per line!
            if (sprites.count == 10)
            {
                break;
            }
        }
    }

    // now we need to sort the spirtes!
    // sprites are ordered by their xpos, however, if xpos match,
    // then the conflicting sprites are sorted based on pos in oam.

    // because the sprites are already sorted via oam index, the following
    // sort preserves the index position.

    if (sprites.count > 1) // silence gcc false positive stringop-overflow
    {
        const uint8_t runfor = sprites.count - 1; // silence gcc false positive strict-overflow
        bool unsorted = true;

        while (unsorted)
        {
            unsorted = false;

            for (uint8_t i = 0; i < runfor; ++i)
            {
                if (sprites.sprite[i].x > sprites.sprite[i + 1].x)
                {
                    const struct DMG_Sprite tmp = sprites.sprite[i];

                    sprites.sprite[i + 0] = sprites.sprite[i + 1];
                    sprites.sprite[i + 1] = tmp;

                    unsorted = true;
                }
            }

        }
    }

    return sprites;
}

static void render_bg_dmg(struct GB_Core* gb, uint32_t pixels[160], struct DmgPrioBuf* prio_buf)
{
    const uint8_t scanline = IO_LY;
    const uint8_t base_tile_x = IO_SCX >> 3;
    const uint8_t sub_tile_x = (IO_SCX & 7);
    const uint8_t pixel_y = (scanline + IO_SCY);
    const uint8_t tile_y = pixel_y >> 3;
    const uint8_t sub_tile_y = (pixel_y & 7);

    const uint8_t* bit = PIXEL_BIT_SHRINK;
    /* due how internally the array is represented when NOT built with gbc */
    /* support, this needed changing to silence gcc array-bounds */
    const uint8_t* vram_map = ((const uint8_t*)gb->ppu.vram) + ((GB_get_bg_map_select(gb) + (tile_y * 32)) & 0x1FFF);

    for (uint8_t tile_x = 0; tile_x <= 20; ++tile_x)
    {
        const uint8_t map_x = ((base_tile_x + tile_x) & 31);

        const uint8_t tile_num = vram_map[map_x];
        const uint16_t offset = GB_get_tile_offset(gb, tile_num, sub_tile_y);

        const uint8_t byte_a = GB_vram_read(gb, offset + 0, 0);
        const uint8_t byte_b = GB_vram_read(gb, offset + 1, 0);

        for (uint8_t x = 0; x < 8; ++x)
        {
            const int16_t x_index = ((tile_x * 8) + x) - sub_tile_x;

            if (x_index >= GB_SCREEN_WIDTH)
            {
                break;
            }

            if (x_index < 0)
            {
                continue;
            }

            const uint8_t colour_id = ((!!(byte_b & bit[x])) << 1) | (!!(byte_a & bit[x]));

            prio_buf->colour_id[x_index] = colour_id;

            const uint32_t colour = DMG_PPU.bg_colours[x_index>>3][colour_id];

            pixels[x_index] = colour;
        }
    }
}

static void render_win_dmg(struct GB_Core* gb, uint32_t pixels[160], struct DmgPrioBuf* prio_buf)
{
    const uint8_t base_tile_x = 20 - (IO_WX >> 3);
    const int16_t sub_tile_x = IO_WX - 7;
    const uint8_t pixel_y = gb->ppu.window_line;
    const uint8_t tile_y = pixel_y >> 3;
    const uint8_t sub_tile_y = (pixel_y & 7);

    bool did_draw = false;

    const uint8_t* bit = PIXEL_BIT_SHRINK;
    const uint8_t* vram_map = ((const uint8_t*)gb->ppu.vram) + ((GB_get_win_map_select(gb) + (tile_y * 32)) & 0x1FFF);

    for (uint8_t tile_x = 0; tile_x <= base_tile_x; ++tile_x)
    {
        const uint8_t tile_num = vram_map[tile_x];
        const uint16_t offset = GB_get_tile_offset(gb, tile_num, sub_tile_y);

        const uint8_t byte_a = GB_vram_read(gb, offset + 0, 0);
        const uint8_t byte_b = GB_vram_read(gb, offset + 1, 0);

        for (uint8_t x = 0; x < 8; ++x)
        {
            const uint8_t x_index = (tile_x * 8) + x + sub_tile_x;

            if (x_index >= GB_SCREEN_WIDTH)
            {
                break;
            }

            did_draw |= true;

            const uint8_t colour_id = ((!!(byte_b & bit[x])) << 1) | (!!(byte_a & bit[x]));

            prio_buf->colour_id[x_index] = colour_id;

            const uint32_t colour = DMG_PPU.bg_colours[x_index>>3][colour_id];

            pixels[x_index] = colour;
        }
    }

    if (did_draw)
    {
        ++gb->ppu.window_line;
    }
}

static void render_obj_dmg(struct GB_Core* gb, uint32_t pixels[160], const struct DmgPrioBuf* prio_buf)
{
    const uint8_t scanline = IO_LY;
    const uint8_t sprite_size = GB_get_sprite_size(gb);

    /* keep track of when an oam entry has already been written. */
    bool oam_priority[GB_SCREEN_WIDTH] = {0};

    const struct DMG_Sprites sprites = dmg_sprite_fetch(gb);

    for (uint8_t i = 0; i < sprites.count; ++i)
    {
        const struct DMG_Sprite* sprite = &sprites.sprite[i];

        /* check if the sprite has a chance of being on screen */
        /* + 8 because thats the width of each sprite (8 pixels) */
        if (sprite->x == -8 || sprite->x >= GB_SCREEN_WIDTH)
        {
            continue;
        }

        const uint8_t sprite_line = sprite->a.yflip ? sprite_size - 1 - (scanline - sprite->y) : scanline - sprite->y;
        /* when in 8x16 size, bit-0 is ignored of the tile_index */
        const uint8_t tile_index = sprite_size == 16 ? sprite->i & 0xFE : sprite->i;
        const uint16_t offset = (sprite_line << 1) + (tile_index << 4);

        const uint8_t byte_a = GB_vram_read(gb, offset + 0, 0);
        const uint8_t byte_b = GB_vram_read(gb, offset + 1, 0);

        const uint8_t* bit = sprite->a.xflip ? PIXEL_BIT_GROW : PIXEL_BIT_SHRINK;

        for (int8_t x = 0; x < 8; ++x)
        {
            const int16_t x_index = sprite->x + x;

            /* sprite is offscreen, exit loop now */
            if (x_index >= GB_SCREEN_WIDTH)
            {
                break;
            }

            /* ensure that we are in bounds */
            if (x_index < 0)
            {
                continue;
            }

            if (oam_priority[x_index])
            {
                continue;
            }

            const uint8_t colour_id = ((!!(byte_b & bit[x])) << 1) | (!!(byte_a & bit[x]));

            if (colour_id == 0)
            {
                continue;
            }

            /* handle prio */
            if (sprite->a.prio && prio_buf->colour_id[x_index])
            {
                continue;
            }

            /* keep track of the sprite pixel that was written (so we don't overlap it!) */
            oam_priority[x_index] = true;

            const uint32_t colour = DMG_PPU.obj_colours[sprite->a.pal][x_index>>3][colour_id];

            pixels[x_index] = colour;
        }
    }
}

void DMG_render_scanline(struct GB_Core* gb)
{
    struct DmgPrioBuf prio_buf = {0};
    uint32_t scanline[160] = {0};

    // update the DMG colour palettes
    dmg_update_colours(DMG_PPU.bg_cache, DMG_PPU.bg_colours, &PPU.dirty_bg[0], gb->palette.BG, IO_BGP);
    dmg_update_colours(DMG_PPU.obj_cache[0], DMG_PPU.obj_colours[0], &PPU.dirty_obj[0], gb->palette.OBJ0, IO_OBP0);
    dmg_update_colours(DMG_PPU.obj_cache[1], DMG_PPU.obj_colours[1], &PPU.dirty_obj[1], gb->palette.OBJ1, IO_OBP1);

    if (LIKELY(GB_is_bg_enabled(gb)))
    {
        render_bg_dmg(gb, scanline, &prio_buf);

        /* WX=0..166, WY=0..143 */
        if ((GB_is_win_enabled(gb)) && (IO_WX <= 166) && (IO_WY <= 143) && (IO_WY <= IO_LY))
        {
            render_win_dmg(gb, scanline, &prio_buf);
        }

        if (LIKELY(GB_is_obj_enabled(gb)))
        {
            render_obj_dmg(gb, scanline, &prio_buf);
        }
    }

    write_scanline_to_frame(gb, scanline);
}

#undef DMG_PPU
