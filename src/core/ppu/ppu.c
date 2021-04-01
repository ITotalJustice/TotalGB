#include "core/gb.h"
#include "core/internal.h"
#include "core/ppu/ppu.h"
#include "core/tables/palette_table.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>


// these are extern, used for dmg / gbc / sgb render functions
const uint8_t PIXEL_BIT_SHRINK[] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };
const uint8_t PIXEL_BIT_GROW[] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };


uint8_t GB_vram_read(const struct GB_Core* gb, const uint16_t addr, const uint8_t bank) {
    assert(bank < 2);
    return gb->ppu.vram[bank][addr & 0x1FFF];
}

// data selects
bool GB_get_bg_data_select(const struct GB_Core* gb) {
    return (!!(IO_LCDC & 0x08));
}

bool GB_get_title_data_select(const struct GB_Core* gb) {
    return (!!(IO_LCDC & 0x10));
}

bool GB_get_win_data_select(const struct GB_Core* gb) {
    return (!!(IO_LCDC & 0x40));
}

// map selects
uint16_t GB_get_bg_map_select(const struct GB_Core* gb) {
    return GB_get_bg_data_select(gb) ? 0x9C00 : 0x9800;
}

uint16_t GB_get_title_map_select(const struct GB_Core* gb) {
    return GB_get_title_data_select(gb) ? 0x8000 : 0x9000;
}

uint16_t GB_get_win_map_select(const struct GB_Core* gb) {
    return GB_get_win_data_select(gb) ? 0x9C00 : 0x9800;
}

uint16_t GB_get_tile_offset(const struct GB_Core* gb, const uint8_t tile_num, const uint8_t sub_tile_y) {
    return (GB_get_title_map_select(gb) + (((GB_get_title_data_select(gb) ? tile_num : (int8_t)tile_num)) << 4) + (sub_tile_y << 1));
}

uint8_t GB_get_sprite_size(const struct GB_Core* gb) {
    return ((IO_LCDC & 0x04) ? 16 : 8);
}

static inline void GB_raise_if_enabled(struct GB_Core* gb, const uint8_t mode) {
    if (IO_STAT & mode) {
        GB_enable_interrupt(gb, GB_INTERRUPT_LCD_STAT);
    }
}

void GB_set_coincidence_flag(struct GB_Core* gb, const bool n) {
    IO_STAT = n ? IO_STAT | 0x04 : IO_STAT & ~0x04;
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

void GB_compare_LYC(struct GB_Core* gb) {
    if (UNLIKELY(IO_LY == IO_LYC)) {
        GB_set_coincidence_flag(gb, true);
        GB_raise_if_enabled(gb, STAT_INT_MODE_COINCIDENCE);
    }
    
    else {
        GB_set_coincidence_flag(gb, false);
    }
}

void GB_change_status_mode(struct GB_Core* gb, const uint8_t new_mode) {
    GB_set_status_mode(gb, new_mode);
    
    switch (new_mode) {
        case STATUS_MODE_HBLANK:
            GB_raise_if_enabled(gb, STAT_INT_MODE_0);
            gb->ppu.next_cycles += 146;
            GB_draw_scanline(gb);

            if (gb->hblank_cb != NULL) {
                gb->hblank_cb(gb, gb->hblank_cb_user_data);
            }
            break;

        case STATUS_MODE_VBLANK:
            GB_raise_if_enabled(gb, STAT_INT_MODE_1);
            GB_enable_interrupt(gb, GB_INTERRUPT_VBLANK);
            gb->ppu.next_cycles += 456;

            if (gb->vblank_cb != NULL) {
                gb->vblank_cb(gb, gb->vblank_cb_user_data);
            }
            break;

        case STATUS_MODE_SPRITE:
            GB_raise_if_enabled(gb, STAT_INT_MODE_2);
            gb->ppu.next_cycles += 80;
            break;

        case STATUS_MODE_TRANSFER:
            gb->ppu.next_cycles += 230;
            break;
    }
}

/*
FF41 - STAT - LCDC Status (R/W)

  Bit 6 - LYC=LY Coincidence Interrupt (1=Enable) (Read/Write)
  Bit 5 - Mode 2 OAM Interrupt         (1=Enable) (Read/Write)
  Bit 4 - Mode 1 V-Blank Interrupt     (1=Enable) (Read/Write)
  Bit 3 - Mode 0 H-Blank Interrupt     (1=Enable) (Read/Write)
  Bit 2 - Coincidence Flag  (0:LYC<>LY, 1:LYC=LY) (Read Only)
  Bit 1-0 - Mode Flag       (Mode 0-3, see below) (Read Only)
            0: During H-Blank
            1: During V-Blank
            2: During Searching OAM-RAM
            3: During Transfering Data to LCD Driver

*/
void GB_on_lcdc_write(struct GB_Core* gb, const uint8_t value) {
    // check if the game wants to disable the ppu
    // this *should* only happen in vblank!
    if (GB_is_lcd_enabled(gb) && (value & 0x80) == 0 && GB_get_status_mode(gb) == STATUS_MODE_VBLANK) {
        IO_LY = 0;
        IO_STAT &= ~(0x7);
        printf("disabling ppu...\n");
    }

    // check if the game wants to re-enable the lcd
    else if (!GB_is_lcd_enabled(gb) && (value & 0x80)) {
        // i think the ppu starts again in vblank
        IO_STAT |= 0x1;
        // i'm not sure on this...
        GB_compare_LYC(gb);
        printf("enabling ppu!\n");
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
        case STATUS_MODE_HBLANK:
            ++IO_LY;
            GB_compare_LYC(gb);

            if (GB_is_hdma_active(gb)) {
                perform_hdma(gb);
            }

            if (UNLIKELY(IO_LY == 144)) {
                GB_change_status_mode(gb, STATUS_MODE_VBLANK);
            } else {
                GB_change_status_mode(gb, STATUS_MODE_SPRITE);
            }
            break;
        
        case STATUS_MODE_VBLANK:
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
        
        case STATUS_MODE_SPRITE:
            GB_change_status_mode(gb, STATUS_MODE_TRANSFER);
            break;
        
        case STATUS_MODE_TRANSFER:
            GB_change_status_mode(gb, STATUS_MODE_HBLANK);
            break;
    }
}

void GB_DMA(struct GB_Core* gb) {
    assert(IO_DMA <= 0xF1);

    memcpy(gb->ppu.oam, gb->mmap[IO_DMA >> 4] + ((IO_DMA & 0xF) << 8), sizeof(gb->ppu.oam));
	gb->cpu.cycles += 646;
}

bool GB_is_render_layer_enabled(const struct GB_Core* gb, enum GB_RenderLayerConfig want) {
    return (gb->config.render_layer_config == GB_RENDER_LAYER_CONFIG_ALL) || ((gb->config.render_layer_config & want) > 0);
}

void GB_draw_scanline(struct GB_Core* gb) {
    switch (GB_get_system_type(gb)) {
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
