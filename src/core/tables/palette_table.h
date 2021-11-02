#ifndef PALETTE_TABLE_H
#define PALETTE_TABLE_H

#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include <stdbool.h>

/* SOURCES: */
/* Button:  https://tcrf.net/Notes:Game_Boy_Color_Bootstrap_ROM#Manual_Select_Palette_Configurations */
/* Hash:    https://tcrf.net/Notes:Game_Boy_Color_Bootstrap_ROM#Assigned_Palette_Configurations */
/* Unused:  https://tcrf.net/Game_Boy_Color_Bootstrap_ROM#Unused_Palette_Configurations */

#define PALETTE_TABLE_MIN 0x0
#define PALETTE_TABLE_MAX 0x5

#define PALETTE_ENTRY_MIN 0x00
#define PALETTE_ENTRY_MAX 0x1C

enum CustomPalette {
    CUSTOM_PALETTE_GREY,
    CUSTOM_PALETTE_GREEN,
    CUSTOM_PALETTE_CREAM,
    CUSTOM_PALETTE_KGREEN,

    CUSTOM_PALETTE_MAX = 0xFF
};

struct PaletteEntry { 
    struct {
        uint8_t r, g, b;
    } BG[4];
    struct {
        uint8_t r, g, b;
    } OBJ0[4];
    struct {
        uint8_t r, g, b;
    } OBJ1[4];
};

struct PalettePreviewShades {
    struct {
        uint8_t r, g, b;
    } shade1;
    struct {
        uint8_t r, g, b;
    } shade2;
};

bool palette_fill_from_table_entry(
    uint8_t table, uint8_t entry, /* keys */
    struct PaletteEntry* palette
);

/* hash over the header title */
/* add each title entry % 100 */
/* forth byte is the 4th byte in the title, used for hash collisions. */
bool palette_fill_from_hash(
    uint8_t hash, /* key */
    uint8_t forth_byte, /* key */
    struct PaletteEntry* palette
);

/* fill palette using buttons as the key */
bool palette_fill_from_buttons(
    uint8_t buttons, /* key */
    struct PaletteEntry* palette,
    struct PalettePreviewShades* preview /* optional (can be NULL) */
);

bool palette_fill_from_custom(
    enum CustomPalette custom, /* key */
    struct PaletteEntry* palette
);

#ifdef __cplusplus
}
#endif

#endif // PALETTE_TABLE_H
