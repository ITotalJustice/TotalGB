#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* SOURCES: */
/* Button:  https://tcrf.net/Notes:Game_Boy_Color_Bootstrap_ROM#Manual_Select_Palette_Configurations */
/* Hash:    https://tcrf.net/Notes:Game_Boy_Color_Bootstrap_ROM#Assigned_Palette_Configurations */
/* Unused:  https://tcrf.net/Game_Boy_Color_Bootstrap_ROM#Unused_Palette_Configurations */

/* BUTTONS: */
/* enum GB_Button {
    GB_BUTTON_A = 1 << 0,
    GB_BUTTON_B = 1 << 1,
    GB_BUTTON_SELECT = 1 << 2,
    GB_BUTTON_START = 1 << 3,

    GB_BUTTON_RIGHT = 1 << 4,
    GB_BUTTON_LEFT = 1 << 5,
    GB_BUTTON_UP = 1 << 6,
    GB_BUTTON_DOWN = 1 << 7
}; */

#define GB_PALETTE_TABLE_MIN 0x0
#define GB_PALETTE_TABLE_MAX 0x5

#define GB_PALETTE_ENTRY_MIN 0x00
#define GB_PALETTE_ENTRY_MAX 0x1C

enum GB_CustomPalette {
    GB_CUSTOM_PALETTE_GREY,
    GB_CUSTOM_PALETTE_GREEN,
    GB_CUSTOM_PALETTE_CREAM,

    GB_CUSTOM_PALETTE_MAX = 0xFF
};

struct GB_PaletteEntry {
    uint16_t BG[4];
    uint16_t OBJ0[4];
    uint16_t OBJ1[4];
};

struct GB_PalettePreviewShades {
    uint16_t shade1;
    uint16_t shade2;
};

/* RETURNS: */
/* 0    = success */
/* -1   = error */

int GB_palette_fill_from_table_entry(
    uint8_t table, uint8_t entry, /* keys */
    struct GB_PaletteEntry* palette
);

/* hash over the header title */
/* add each title entry % 100 */
/* forth byte is the 4th byte in the title, used for hash collisions. */
int GB_palette_fill_from_hash(
    uint8_t hash, /* key */
    uint8_t forth_byte, /* key */
    struct GB_PaletteEntry* palette
);

/* fill palette using buttons as the key */
int GB_palette_fill_from_buttons(
    uint8_t buttons, /* key */
    struct GB_PaletteEntry* palette,
    struct GB_PalettePreviewShades* preview /* optional (can be NULL) */
);

int GB_Palette_fill_from_custom(
    enum GB_CustomPalette custom, /* key */
    struct GB_PaletteEntry* palette
);

#ifdef __cplusplus
}
#endif
