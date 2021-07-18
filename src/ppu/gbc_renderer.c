#include "ppu.h"
#include "../internal.h"
#include "../gb.h"

#include <assert.h>
#include <string.h>

#if GBC_ENABLE

// store a 1 when bg / win writes to the screen
// this is used by obj rendering to check firstly if
// the bg always has priority.
// if it does, then it checks this buffer for a 1
// at the same xpos, if its 1, rendering that pixel is skipped.
struct GbcPrioBuf
{
    // 1 = prio, 0 = no prio
    bool prio[GB_SCREEN_WIDTH];
    // 0-3
    uint8_t colour_id[GB_SCREEN_WIDTH];
};

static inline void gbc_update_colours(struct GB_Core* gb,
    bool dirty[8], uint32_t map[8][4], const uint8_t palette_mem[64]
) {
    if (gb->callback.colour == NULL)
    {
        memset(dirty, 0, sizeof(bool) * 8);
        return;
    }

    for (uint8_t palette = 0; palette < 8; ++palette)
    {
        if (dirty[palette] == true)
        {
            dirty[palette] = false;

            for (uint8_t colours = 0, pos = 0; colours < 4; colours++, pos += 2)
            {
                const uint8_t col_a = palette_mem[(palette * 8) + pos + 0];
                const uint8_t col_b = palette_mem[(palette * 8) + pos + 1];
             
                const uint16_t pair = (col_b << 8) | col_a;

                const uint8_t r = (pair >> 0x0) & 0x1F; 
                const uint8_t g = (pair >> 0x5) & 0x1F; 
                const uint8_t b = (pair >> 0xA) & 0x1F; 

                map[palette][colours] = gb->callback.colour(gb->callback.user_colour, GB_ColourCallbackType_GBC, r, g, b);
            }
        }
    }
}

static inline bool is_bcps_auto_increment(const struct GB_Core* gb)
{
    return (IO_BCPS & 0x80) > 0;
}

static inline bool is_ocps_auto_increment(const struct GB_Core* gb)
{
    return (IO_OCPS & 0x80) > 0;
}

static inline uint8_t get_bcps_index(const struct GB_Core* gb)
{
    return IO_BCPS & 0x3F;
}

static inline uint8_t get_ocps_index(const struct GB_Core* gb)
{
    return IO_OCPS & 0x3F;
}

static inline void bcps_increment(struct GB_Core* gb)
{
    if (is_bcps_auto_increment(gb) == true)
    {
        // only increment the lower 5 bits.
        // set back the increment bit after.
        IO_BCPS = ((IO_BCPS + 1) & 0x3F) | (0xC0);
    }
}

static inline void ocps_increment(struct GB_Core* gb)
{
    if (is_ocps_auto_increment(gb) == true)
    {
        // only increment the lower 5 bits.
        // set back the increment bit after.
        IO_OCPS = ((IO_OCPS + 1) & 0x3F) | (0xC0);
    }
}

void GBC_on_bcpd_update(struct GB_Core* gb)
{
    IO_BCPD = gb->ppu.bg_palette[get_bcps_index(gb)];
}

void GBC_on_ocpd_update(struct GB_Core* gb)
{
    IO_OCPD = gb->ppu.obj_palette[get_ocps_index(gb)];
}

void GB_bcpd_write(struct GB_Core* gb, uint8_t value)
{
    const uint8_t index = get_bcps_index(gb);

    // this is 0-7
    assert((index >> 3) <= 7);
    gb->ppu.dirty_bg[index >> 3] |= gb->ppu.bg_palette[index] != value;

    gb->ppu.bg_palette[index] = value;
    bcps_increment(gb);

    GBC_on_bcpd_update(gb);
}

void GB_ocpd_write(struct GB_Core* gb, uint8_t value)
{
    const uint8_t index = get_ocps_index(gb);

    // this is 0-7
    assert((index >> 3) <= 7);
    gb->ppu.dirty_obj[index >> 3] |= gb->ppu.obj_palette[index] != value;

    gb->ppu.obj_palette[index] = value;
    ocps_increment(gb);

    GBC_on_ocpd_update(gb);
}

bool GB_is_hdma_active(const struct GB_Core* gb)
{
    return gb->ppu.hdma_length > 0;
}

static FORCE_INLINE uint8_t hdma_read(const struct GB_Core* gb, const uint16_t addr)
{
    const struct GB_MemMapEntry* entry = &gb->mmap[(addr >> 12)];
    
    return entry->ptr[addr & entry->mask];
}

static FORCE_INLINE void hdma_write(struct GB_Core* gb, const uint16_t addr, const uint8_t value)
{
    gb->ppu.vram[IO_VBK][(addr) & 0x1FFF] = value;
}

void perform_hdma(struct GB_Core* gb)
{
    assert(GB_is_hdma_active(gb) == true);
    // perform 16-block transfer
    for (uint16_t i = 0; i < 0x10; ++i)
    {
        hdma_write(gb, gb->ppu.hdma_dst_addr++, hdma_read(gb, gb->ppu.hdma_src_addr++));
    }

    gb->ppu.hdma_length -= 0x10;

    --IO_HDMA5;

    // finished!
    if (gb->ppu.hdma_length == 0)
    {
        gb->ppu.hdma_length = 0;

        IO_HDMA5 = 0xFF;
    }
}

uint8_t GB_hdma5_read(const struct GB_Core* gb)
{
    return IO_HDMA5;
}

void GB_hdma5_write(struct GB_Core* gb, uint8_t value)
{
    // lower 6-bits are the length + 1 * 0x10
    const uint16_t dma_len = ((value & 0x7F) + 1) * 0x10;

    // by checking bit-7 of value, it returns the type of dma to perform.
    const uint8_t mode = value & 0x80;

    enum GB_HDMA5Mode
    {
        GDMA = 0x00,
        HDMA = 0x80
    };

    if (mode == GDMA)
    {
        // setting bit-7 = 0 whilst a HDMA is currently active
        // actually disables that transfer
        if (GB_is_hdma_active(gb) == true)
        {
            IO_HDMA5 = ((gb->ppu.hdma_length >> 4) - 1) | 0x80;
            gb->ppu.hdma_length = 0;

            // do not perform GDMA after, this only cancels the active
            // transfer and sets HDMA5.
            return;
        }

        // GDMA are performed immediately
        for (uint16_t i = 0; i < dma_len; ++i)
        {
            hdma_write(gb, gb->ppu.hdma_dst_addr++, hdma_read(gb, gb->ppu.hdma_src_addr++));
        }

        // it's unclear if all HDMA regs are set to 0xFF post transfer,
        // HDMA5 is, but not sure about the rest.
        IO_HDMA5 = 0xFF;
    }
    else
    {
        gb->ppu.hdma_length = dma_len;

        // set that the transfer is active.
        IO_HDMA5 = value & 0x7F;
    }
}

struct GBC_BgAttribute
{
    bool prio;
    bool yflip;
    bool xflip;
    uint8_t bank;
    uint8_t pal;
};

struct GBC_BgAttributes
{
    struct GBC_BgAttribute a[32];
};

static FORCE_INLINE struct GBC_BgAttribute gbc_get_bg_attr(const uint8_t v)
{
    return (struct GBC_BgAttribute)
    {
        .prio   = (v & 0x80) > 0,
        .yflip  = (v & 0x40) > 0,
        .xflip  = (v & 0x20) > 0,
        .bank   = (v & 0x08) > 0,
        .pal    = (v & 0x07),
    };
}

static inline struct GBC_BgAttributes gbc_fetch_bg_attr(const struct GB_Core* gb, uint16_t map, uint8_t tile_y)
{
    struct GBC_BgAttributes attrs = {0};

    const uint8_t* ptr = &gb->ppu.vram[1][(map + (tile_y * 32)) & 0x1FFF];

    for (size_t i = 0; i < ARRAY_SIZE(attrs.a); ++i)
    {
        attrs.a[i] = gbc_get_bg_attr(ptr[i]);
    }

    return attrs;
}

struct GBC_Sprite
{
    int16_t y;
    int16_t x;
    uint8_t i;
    // gbc sprite and bg attr have the same layout
    struct GBC_BgAttribute a;
};

struct GBC_Sprites
{
    struct GBC_Sprite sprite[10];
    uint8_t count;
};

static inline struct GBC_Sprites gbc_sprite_fetch(const struct GB_Core* gb)
{
    struct GBC_Sprites sprites = {0};
    
    const uint8_t sprite_size = GB_get_sprite_size(gb);
    const uint8_t ly = IO_LY;

    for (size_t i = 0; i < ARRAY_SIZE(gb->ppu.oam); i += 4)
    {
        const int16_t sprite_y = gb->ppu.oam[i + 0] - 16;

        // check if the y is in bounds!
        if (ly >= sprite_y && ly < (sprite_y + sprite_size))
        {
            struct GBC_Sprite* sprite = &sprites.sprite[sprites.count];

            sprite->y = sprite_y;
            sprite->x = gb->ppu.oam[i + 1] - 8;
            sprite->i = gb->ppu.oam[i + 2];
            sprite->a = gbc_get_bg_attr(gb->ppu.oam[i + 3]);

            ++sprites.count;

            // only 10 sprites per line!
            if (sprites.count == 10)
            {
                break;
            }
        }
    }

    return sprites;
}

#define RENDER_BG(pixels, prio_buf) do \
{ \
    const uint8_t scanline = IO_LY; \
    const uint8_t base_tile_x = IO_SCX >> 3; \
    const uint8_t sub_tile_x = (IO_SCX & 7); \
    const uint8_t pixel_y = (scanline + IO_SCY); \
    const uint8_t tile_y = pixel_y >> 3; \
    const uint8_t sub_tile_y = (pixel_y & 7); \
\
    /* array maps */ \
    const uint8_t* vram_map = &gb->ppu.vram[0][(GB_get_bg_map_select(gb) + (tile_y * 32)) & 0x1FFF]; \
    const struct GBC_BgAttributes attr_map = gbc_fetch_bg_attr(gb, GB_get_bg_map_select(gb), tile_y); \
\
    for (uint8_t tile_x = 0; tile_x <= 20; ++tile_x) \
    { \
        /* calc the map index */ \
        const uint8_t map_x = ((base_tile_x + tile_x) & 31); \
\
        /* fetch the tile number and attributes */ \
        const uint8_t tile_num = vram_map[map_x]; \
        const struct GBC_BgAttribute* attr = &attr_map.a[map_x]; \
\
        const uint16_t offset = GB_get_tile_offset(gb, \
            tile_num, \
            /* check if flip the y axis */ \
            attr->yflip ? 7 - sub_tile_y : sub_tile_y \
        ); \
\
        const uint8_t byte_a = GB_vram_read(gb, offset + 0, attr->bank); \
        const uint8_t byte_b = GB_vram_read(gb, offset + 1, attr->bank); \
\
        const uint8_t* bit = attr->xflip ? PIXEL_BIT_GROW : PIXEL_BIT_SHRINK; \
\
        for (uint8_t x = 0; x < 8; ++x) \
        { \
            const int16_t x_index = (tile_x * 8) + x - sub_tile_x; \
\
            if (x_index >= GB_SCREEN_WIDTH) \
            { \
                break; \
            } \
\
            if (x_index < 0) \
            { \
                continue; \
            } \
\
            const uint8_t colour_id = ((!!(byte_b & bit[x])) << 1) | (!!(byte_a & bit[x])); \
\
            /* set priority */ \
            prio_buf.prio[x_index] = attr->prio; \
            prio_buf.colour_id[x_index] = colour_id; \
\
            const uint32_t colour = gb->ppu.bg_colours[attr->pal][colour_id]; \
\
            pixels[x_index] = colour; \
        } \
    } \
} while (0)

#define RENDER_WIN(pixels, prio_buf) do \
{ \
    const uint8_t base_tile_x = 20 - (IO_WX >> 3); \
    const int16_t sub_tile_x = IO_WX - 7; \
    const uint8_t pixel_y = gb->ppu.window_line; \
    const uint8_t tile_y = pixel_y >> 3; \
    const uint8_t sub_tile_y = (pixel_y & 7); \
\
    bool did_draw = false; \
\
    const uint8_t *vram_map = &gb->ppu.vram[0][(GB_get_win_map_select(gb) + (tile_y * 32)) & 0x1FFF]; \
    const struct GBC_BgAttributes attr_map = gbc_fetch_bg_attr(gb, GB_get_win_map_select(gb), tile_y); \
\
    for (uint8_t tile_x = 0; tile_x <= base_tile_x; ++tile_x) \
    { \
        /* fetch the tile number and attributes */ \
        const uint8_t tile_num = vram_map[tile_x]; \
        const struct GBC_BgAttribute attr = attr_map.a[tile_x]; \
\
        const uint16_t offset = GB_get_tile_offset(gb, \
            tile_num, \
            attr.yflip ? 7 - sub_tile_y : sub_tile_y \
        ); \
\
        const uint8_t byte_a = GB_vram_read(gb, offset + 0, attr.bank); \
        const uint8_t byte_b = GB_vram_read(gb, offset + 1, attr.bank); \
\
        const uint8_t* bit = attr.xflip ? PIXEL_BIT_GROW : PIXEL_BIT_SHRINK; \
\
        for (uint8_t x = 0; x < 8; ++x) \
        { \
            const uint8_t x_index = (tile_x * 8) + x + sub_tile_x; \
\
            if (x_index >= GB_SCREEN_WIDTH) \
            { \
                break; \
            } \
\
            did_draw |= true; \
\
            const uint8_t colour_id = ((!!(byte_b & bit[x])) << 1) | (!!(byte_a & bit[x])); \
\
            /* set priority */ \
            prio_buf.prio[x_index] = attr.prio; \
            prio_buf.colour_id[x_index] = colour_id; \
\
            const uint32_t colour = gb->ppu.bg_colours[attr.pal][colour_id]; \
\
            pixels[x_index] = colour; \
        } \
    } \
\
    if (did_draw) \
    { \
        ++gb->ppu.window_line; \
    } \
} while(0)

#define RENDER_OBJ(pixels, prio_buf) do \
{ \
    const uint8_t scanline = IO_LY; \
    const uint8_t sprite_size = GB_get_sprite_size(gb); \
\
    /* check if the bg always has prio over obj */ \
    const bool bg_prio = (IO_LCDC & 0x1) > 0; \
\
    const struct GBC_Sprites sprites = gbc_sprite_fetch(gb); \
\
    /* gbc uses oam prio rather than x-pos */ \
    /* so we need to keep track if a obj has already */ \
    /* been written to from a previous oam entry! */ \
    bool oam_priority[GB_SCREEN_WIDTH] = {0}; \
\
    for (uint8_t i = 0; i < sprites.count; ++i) \
    { \
        const struct GBC_Sprite* sprite = &sprites.sprite[i]; \
\
        /* check if the sprite has a chance of being on screen */ \
        /* + 8 because thats the width of each sprite (8 pixels) */ \
        if (sprite->x == -8 || sprite->x >= GB_SCREEN_WIDTH) \
        { \
            continue; \
        } \
\
        const uint8_t sprite_line = sprite->a.yflip ? sprite_size - 1 - (scanline - sprite->y) : scanline - sprite->y; \
        /* when in 8x16 size, bit-0 is ignored of the tile_index */ \
        const uint8_t tile_index = sprite_size == 16 ? sprite->i & 0xFE : sprite->i; \
        const uint16_t offset = (((sprite_line) << 1) + (tile_index << 4)); \
\
        const uint8_t byte_a = GB_vram_read(gb, offset + 0, sprite->a.bank); \
        const uint8_t byte_b = GB_vram_read(gb, offset + 1, sprite->a.bank); \
\
        const uint8_t* bit = sprite->a.xflip ? PIXEL_BIT_GROW : PIXEL_BIT_SHRINK; \
\
        for (int8_t x = 0; x < 8; ++x) \
        { \
            const int16_t x_index = sprite->x + x; \
\
            /* sprite is offscreen, exit loop now */ \
            if (x_index >= GB_SCREEN_WIDTH) \
            { \
                break; \
            } \
\
            /* ensure that we are in bounds */ \
            if (x_index < 0) \
            { \
                continue; \
            } \
\
            /* check if this has already been written to */ \
            /* from a previous oam entry */ \
            if (oam_priority[x_index]) \
            { \
                continue; \
            } \
\
            const uint8_t colour_id = ((!!(byte_b & bit[x])) << 1) | (!!(byte_a & bit[x])); \
\
            /* this tests if the obj is transparrent */ \
            if (colour_id == 0) \
            { \
                continue; \
            } \
\
            if (bg_prio == 1) \
            { \
                /* this tests if bg always has priority over obj */ \
                if (prio_buf.prio[x_index] && prio_buf.colour_id[x_index]) \
                { \
                    continue; \
                } \
\
                /* this tests if bg col 1-3 has priority, */ \
                /* then checks if the col is non-zero, if yes, skip */ \
                if (sprite->a.prio && prio_buf.colour_id[x_index]) \
                { \
                    continue; \
                } \
            } \
\
            /* save that we have already written to this xpos so the next */ \
            /* oam entries cannot overwrite this pixel! */ \
            oam_priority[x_index] = true; \
\
            const uint32_t colour = gb->ppu.obj_colours[sprite->a.pal][colour_id]; \
\
            pixels[x_index] = colour; \
        } \
    } \
} while(0)

#define RENDER(bpp, prio_buf) do \
{ \
    uint##bpp##_t* pixels = ((uint##bpp##_t*)gb->pixels) + (gb->stride * IO_LY); \
\
    RENDER_BG(pixels, prio_buf); \
\
    /* WX=0..166, WY=0..143 */ \
    if ((GB_is_win_enabled(gb)) && (IO_WX <= 166) && (IO_WY <= 143) && (IO_WY <= IO_LY)) \
    { \
        RENDER_WIN(pixels, prio_buf); \
    } \
\
    if (LIKELY(GB_is_obj_enabled(gb))) \
    { \
        RENDER_OBJ(pixels, prio_buf); \
    } \
} while(0)

void GBC_render_scanline(struct GB_Core* gb)
{
    struct GbcPrioBuf prio_buf = {0};

    // update the bg colour palettes
    gbc_update_colours(gb, gb->ppu.dirty_bg, gb->ppu.bg_colours, gb->ppu.bg_palette);
    // update the obj colour palettes
    gbc_update_colours(gb, gb->ppu.dirty_obj, gb->ppu.obj_colours, gb->ppu.obj_palette);

    switch (gb->bpp)
    {
        case 8:
            RENDER(8, prio_buf);
            break;

        case 15:
        case 16:
            RENDER(16, prio_buf);
            break;

        case 24:
        case 32:
            RENDER(32, prio_buf);
            break;
    }
}

#undef RENDER_BG
#undef RENDER_WIN
#undef RENDER_OBJ
#undef RENDER

#endif // #if GBC_ENABLE
