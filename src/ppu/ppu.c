#include "ppu.h"
#include "../internal.h"
#include "../gb.h"

#include <string.h>
#include <assert.h>


// these are extern, used for dmg / gbc / sgb render functions
const uint8_t PIXEL_BIT_SHRINK[] =
{
    0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01
};

const uint8_t PIXEL_BIT_GROW[] =
{
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80
};


void ppu_write_pixel(struct GB_Core* gb, uint32_t c, uint8_t x, uint8_t y)
{
    switch (gb->bpp)
    {
        case 8:
            ((uint8_t*)gb->pixels)[gb->stride * y + x] = c;
            break;

        case 15:
        case 16:
            ((uint16_t*)gb->pixels)[gb->stride * y + x] = c;
            break;

        case 24:
        case 32:
            ((uint32_t*)gb->pixels)[gb->stride * y + x] = c;
            break;
    }
}

uint8_t GB_vram_read(const struct GB_Core* gb,
    const uint16_t addr, const uint8_t bank
)
{
    assert(bank < 2);
    return gb->ppu.vram[bank][addr & 0x1FFF];
}

// data selects
bool GB_get_bg_data_select(const struct GB_Core* gb)
{
    return (!!(IO_LCDC & 0x08));
}

bool GB_get_title_data_select(const struct GB_Core* gb)
{
    return (!!(IO_LCDC & 0x10));
}

bool GB_get_win_data_select(const struct GB_Core* gb)
{
    return (!!(IO_LCDC & 0x40));
}

// map selects
uint16_t GB_get_bg_map_select(const struct GB_Core* gb)
{
    return GB_get_bg_data_select(gb) ? 0x9C00 : 0x9800;
}

uint16_t GB_get_title_map_select(const struct GB_Core* gb)
{
    return GB_get_title_data_select(gb) ? 0x8000 : 0x9000;
}

uint16_t GB_get_win_map_select(const struct GB_Core* gb)
{
    return GB_get_win_data_select(gb) ? 0x9C00 : 0x9800;
}

uint16_t GB_get_tile_offset(const struct GB_Core* gb,
    const uint8_t tile_num, const uint8_t sub_tile_y
) {
    return (GB_get_title_map_select(gb) + (((GB_get_title_data_select(gb) ? tile_num : (int8_t)tile_num)) * 16) + (sub_tile_y << 1));
}

uint8_t GB_get_sprite_size(const struct GB_Core* gb)
{
    return ((IO_LCDC & 0x04) ? 16 : 8);
}

static inline void GB_raise_if_enabled(struct GB_Core* gb, const uint8_t mode)
{
    if (IO_STAT & mode)
    {
        GB_enable_interrupt(gb, GB_INTERRUPT_LCD_STAT);
    }
}

void GB_set_coincidence_flag(struct GB_Core* gb, const bool n)
{
    IO_STAT = n ? IO_STAT | 0x04 : IO_STAT & ~0x04;
}

void GB_set_status_mode(struct GB_Core* gb, enum GB_StatusModes mode)
{
    IO_STAT &= ~0x3;
    IO_STAT |= mode;
}

enum GB_StatusModes GB_get_status_mode(const struct GB_Core* gb)
{
    return (IO_STAT & 0x03);
}

bool GB_is_lcd_enabled(const struct GB_Core* gb)
{
    return (!!(IO_LCDC & 0x80));
}

bool GB_is_win_enabled(const struct GB_Core* gb)
{
    return (!!(IO_LCDC & 0x20));
}

bool GB_is_obj_enabled(const struct GB_Core* gb)
{
    return (!!(IO_LCDC & 0x02));
}

bool GB_is_bg_enabled(const struct GB_Core* gb)
{
    return (!!(IO_LCDC & 0x01));
}

void GB_compare_LYC(struct GB_Core* gb)
{
    if (UNLIKELY(IO_LY == IO_LYC))
    {
        GB_set_coincidence_flag(gb, true);
        GB_raise_if_enabled(gb, STAT_INT_MODE_COINCIDENCE);
    }

    else
    {
        GB_set_coincidence_flag(gb, false);
    }
}

void GB_change_status_mode(struct GB_Core* gb, const uint8_t new_mode)
{
    GB_set_status_mode(gb, new_mode);

    // TODO: check what the timing should actually be for ppu modes!
    switch (new_mode)
    {
        case STATUS_MODE_HBLANK:
            GB_raise_if_enabled(gb, STAT_INT_MODE_0);
            gb->ppu.next_cycles += 204;
            GB_draw_scanline(gb);

            if (gb->callback.hblank != NULL)
            {
                gb->callback.hblank(gb->callback.user_data);
            }
            break;

        case STATUS_MODE_VBLANK:
            GB_raise_if_enabled(gb, STAT_INT_MODE_1);
            GB_enable_interrupt(gb, GB_INTERRUPT_VBLANK);
            gb->ppu.next_cycles += 456;

            if (gb->callback.vblank != NULL)
            {
                gb->callback.vblank(gb->callback.user_data);
            }
            break;

        case STATUS_MODE_SPRITE:
            GB_raise_if_enabled(gb, STAT_INT_MODE_2);
            gb->ppu.next_cycles += 80;
            break;

        case STATUS_MODE_TRANSFER:
            gb->ppu.next_cycles += 172;
            break;
    }
}

void GB_on_lcdc_write(struct GB_Core* gb, const uint8_t value)
{
    // check if the game wants to disable the ppu
    // this *should* only happen in vblank!
    if (GB_is_lcd_enabled(gb) && (value & 0x80) == 0)
    {
        if (GB_get_status_mode(gb) != STATUS_MODE_VBLANK)
        {
            GB_log("[PPU-WARN] game is disabling lcd outside vblank: 0x%0X\n", GB_get_status_mode(gb));
        }

        IO_LY = 0;
        IO_STAT &= ~(0x3);
        GB_log("disabling ppu...\n");
    }

    // check if the game wants to re-enable the lcd
    else if (!GB_is_lcd_enabled(gb) && (value & 0x80))
    {
        gb->ppu.next_cycles = 0;
        GB_set_status_mode(gb, STATUS_MODE_TRANSFER);
        GB_compare_LYC(gb);
        GB_log("enabling ppu!\n");
    }

    IO_LCDC = value;
}

void GB_ppu_run(struct GB_Core* gb, uint16_t cycles)
{
    if (UNLIKELY(!GB_is_lcd_enabled(gb)))
    {
        return;
    }

    gb->ppu.next_cycles -= cycles;
    
    if (UNLIKELY(((gb->ppu.next_cycles) > 0)))
    {
        return;
    }

    switch (GB_get_status_mode(gb))
    {
        case STATUS_MODE_HBLANK:
            ++IO_LY;
            GB_compare_LYC(gb);

            if (GB_is_hdma_active(gb))
            {
                perform_hdma(gb);
            }

            if (UNLIKELY(IO_LY == 144))
            {
                GB_change_status_mode(gb, STATUS_MODE_VBLANK);
            }
            else
            {
                GB_change_status_mode(gb, STATUS_MODE_SPRITE);
            }
            break;

        case STATUS_MODE_VBLANK:
            ++IO_LY;
            
            // there is a bug in which on line 153, LY=153 only lasts
            // for 4-Tcycles.
            if (IO_LY == 153)
            {
                gb->ppu.next_cycles += 4;
                GB_compare_LYC(gb);
            }
            else if (IO_LY == 154)
            {
                gb->ppu.next_cycles += 452;
                IO_LY = 0;
                gb->ppu.window_line = 0;
                GB_compare_LYC(gb);
            }
            else if (IO_LY == 1)
            {
                IO_LY = 0;
                GB_change_status_mode(gb, STATUS_MODE_SPRITE);
            }
            else
            {
                gb->ppu.next_cycles += 456;
                GB_compare_LYC(gb);
            }
            break;

        case STATUS_MODE_SPRITE:
            GB_change_status_mode(gb, STATUS_MODE_TRANSFER);
            break;

        case STATUS_MODE_TRANSFER:
            GB_change_status_mode(gb, STATUS_MODE_HBLANK);
            break;
    }
}

void GB_DMA(struct GB_Core* gb)
{
    assert(IO_DMA <= 0xDF);

    // because it's possible for the index to be
    // cart ram, which may be invalid or RTC reg,
    // this first checks that the mask is zero,
    // if it is, then just memset the entire area
    // else, a normal copy happens.
    const struct GB_MemMapEntry entry = gb->mmap[IO_DMA >> 4];

    if (entry.mask == 0)
    {
        memset(gb->ppu.oam, entry.ptr[0], sizeof(gb->ppu.oam));
    }
    else
    {
        // TODO: check the math to see if this can go OOB for
        // mbc2-ram!!!
        memcpy(gb->ppu.oam, entry.ptr + ((IO_DMA & 0xF) << 8), sizeof(gb->ppu.oam));
    }
}

bool GB_is_render_layer_enabled(const struct GB_Core* gb,
    enum GB_RenderLayerConfig want
) {
    return (gb->config.render_layer_config == GB_RENDER_LAYER_CONFIG_ALL) || ((gb->config.render_layer_config & want) > 0);
}

void GB_draw_scanline(struct GB_Core* gb)
{
    // check if the user has set any pixels, if not, skip rendering!
    if (!gb->pixels || !gb->stride)
    {
        return;
    }

    if (gb->skip_next_frame)
    {
        // return;
    }

    switch (GB_get_system_type(gb))
    {
        case GB_SYSTEM_TYPE_DMG:
            DMG_render_scanline(gb);
            break;

         case GB_SYSTEM_TYPE_GBC:
            GBC_render_scanline(gb);
            break;

        case GB_SYSTEM_TYPE_SGB:
            SGB_render_scanline(gb);
            break;
    }
}
