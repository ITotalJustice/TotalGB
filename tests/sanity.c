// simple test to see if it links okay
// and that all functions are visibile!
#include "gb.h"


int main() {
    static struct GB_Core gb;

    GB_init(&gb);

    GB_loadrom(&gb, NULL, 0);

    GB_has_save(&gb);
    GB_has_rtc(&gb);
    GB_calculate_savedata_size(&gb);
    // NOT IMPLEMENTED
    // GB_calculate_state_size(&gb);

    GB_savegame(&gb, NULL);
    GB_loadsave(&gb, NULL);
    GB_savestate(&gb, NULL);
    GB_loadstate(&gb, NULL);
    GB_savestate2(&gb, NULL);
    GB_loadstate2(&gb, NULL);


    // GB_set_rtc(&gb, const struct GB_Rtc rtc);
    GB_set_rtc_update_config(&gb, 0);
    GB_has_mbc_flags(&gb, 1);

    GB_run_step(&gb);
    GB_run_frame(&gb);

    GB_set_render_palette_layer_config(&gb, 0);

    GB_get_system_type(&gb);
    GB_is_system_gbc(&gb);

    GB_get_rom_header(&gb, NULL);
    GB_get_rom_header_ptr(&gb);
    GB_get_rom_info(&gb, NULL);
    GB_get_rom_name(&gb, NULL);
    GB_get_rom_name_from_header(NULL, NULL);

    GB_set_apu_callback(&gb, NULL);
    GB_set_vblank_callback(&gb, NULL, NULL);
    GB_set_hblank_callback(&gb, NULL, NULL);
    GB_set_dma_callback(&gb, NULL, NULL);
    GB_set_halt_callback(&gb, NULL, NULL);
    GB_set_stop_callback(&gb, NULL, NULL);
    GB_set_error_callback(&gb, NULL, NULL);
    GB_connect_link_cable(&gb, NULL, NULL);
    GB_connect_link_cable_builtin(&gb, NULL);

    GB_get_rom_palette_hash_from_header(NULL, NULL, NULL);
    GB_get_rom_palette_hash(&gb, NULL, NULL);
    // NOT IMPLEMENTED
    // GB_set_palette_from_table_entry(&gb, 0, 0);
    // NOT IMPLEMENTED
    // GB_set_palette_from_hash(&gb, 0);
    // NOT IMPLEMENTED
    // GB_set_palette_from_buttons(&gb, 0);
    GB_set_palette_from_palette(&gb, NULL);

    GB_set_buttons(&gb, 1, true);
    GB_get_buttons(&gb);
    GB_is_button_down(&gb, 1);

    GB_cpu_set_flag(&gb, 1, true);
    GB_cpu_get_flag(&gb, 1);

    GB_cpu_set_register(&gb, 0, 0);
    GB_cpu_get_register(&gb, 0);

    GB_cpu_set_register_pair(&gb, 0, 0);
    GB_cpu_get_register_pair(&gb, 0);

    GB_reset(&gb);
    GB_quit(&gb);

    return 0;
}
