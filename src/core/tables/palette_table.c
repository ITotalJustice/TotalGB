#include "core/tables/palette_table.h"
#include "core/types.h"

#include <assert.h>

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

/* transforms RGB888 to BGR555 */
/* can be made faster but i think this is calculated at compile time for the arrays */
/* shift down by (8 * byte pos) + 3 (5bits) then mask lower 5 bits, then shift up */
#define C(c) (((c >> 19) & 0x1F)) | (((c >> 11) & 0x1F) << 5) | (((c >> 3) & 0x1F) << 10)

struct GB_PaletteButtonEntry {
    uint8_t table;
    uint8_t entry;
    uint8_t buttons;
    struct GB_PalettePreviewShades preview;
};

struct GB_PaletteHashEntry {
    uint8_t table;
    uint8_t entry;
    uint8_t hash;
    uint8_t forth;
};

static const struct GB_PaletteEntry PALETTE_CUSTOM_TABLE[] = {
    [GB_CUSTOM_PALETTE_GREY] = {
        .BG =   { C(0xFFFFFF), C(0xB6B6B6), C(0x676767), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xB6B6B6), C(0x676767), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xB6B6B6), C(0x676767), C(0x000000) }
    },
    [GB_CUSTOM_PALETTE_GREEN] = {
        .BG =   { C(0xE3EEC0), C(0xAEBA89), C(0x5E6745), C(0x202020) },
        .OBJ0 = { C(0xE3EEC0), C(0xAEBA89), C(0x5E6745), C(0x202020) },
        .OBJ1 = { C(0xE3EEC0), C(0xAEBA89), C(0x5E6745), C(0x202020) }
    },
    [GB_CUSTOM_PALETTE_CREAM] = {
        .BG =   { C(0xF7E7C6), C(0xD68E49), C(0xA63725), C(0x331E50) },
        .OBJ0 = { C(0xF7E7C6), C(0xD68E49), C(0xA63725), C(0x331E50) },
        .OBJ1 = { C(0xF7E7C6), C(0xD68E49), C(0xA63725), C(0x331E50) }
    }
};

static const struct GB_PaletteEntry PALETTE_TABLE_0x0[] = {
    [0x00] = {
        .BG =   { C(0xFFFFFF), C(0xADAD84), C(0x42737B), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xADAD84), C(0x42737B), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xADAD84), C(0x42737B), C(0x000000) }
    },
    [0x01] = {
        .BG =   { C(0xFFFF9C), C(0x94B5FF), C(0x639473), C(0x003A3A) },
        .OBJ0 = { C(0xFFFF9C), C(0x94B5FF), C(0x639473), C(0x003A3A) },
        .OBJ1 = { C(0xFFFF9C), C(0x94B5FF), C(0x639473), C(0x003A3A) }
    },
    [0x02] = {
        .BG =   { C(0x6BFF00), C(0xFFFFFF), C(0xFF524A), C(0x000000) },
        .OBJ0 = { C(0x6BFF00), C(0xFFFFFF), C(0xFF524A), C(0x000000) },
        .OBJ1 = { C(0x6BFF00), C(0xFFFFFF), C(0xFF524A), C(0x000000) }
    },
    [0x03] = {
        .BG =   { C(0x52DE00), C(0xFF8400), C(0xFFFF00), C(0xFFFFFF) },
        .OBJ0 = { C(0x52DE00), C(0xFF8400), C(0xFFFF00), C(0xFFFFFF) },
        .OBJ1 = { C(0x52DE00), C(0xFF8400), C(0xFFFF00), C(0xFFFFFF) }
    },
    [0x04] = {
        .BG =   { C(0xFFFFFF), C(0x7BFF00), C(0xB57300), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0x7BFF00), C(0xB57300), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x7BFF00), C(0xB57300), C(0x000000) }
    },
    [0x05] = {
        .BG =   { C(0xFFFFFF), C(0x52FF00), C(0xFF4200), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0x52FF00), C(0xFF4200), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x52FF00), C(0xFF4200), C(0x000000) }
    },
    [0x06] = {
        .BG =   { C(0xFFFFFF), C(0xFF9C00), C(0xFF0000), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFF9C00), C(0xFF0000), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFF9C00), C(0xFF0000), C(0x000000) }
    },
    [0x07] = {
        .BG =   { C(0xFFFFFF), C(0xFFFF00), C(0xFF0000), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFFFF00), C(0xFF0000), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFFFF00), C(0xFF0000), C(0x000000) }
    },
    [0x08] = {
        .BG =   { C(0xA59CFF), C(0xFFFF00), C(0x006300), C(0x000000) },
        .OBJ0 = { C(0xA59CFF), C(0xFFFF00), C(0x006300), C(0x000000) },
        .OBJ1 = { C(0xA59CFF), C(0xFFFF00), C(0x006300), C(0x000000) }
    },
    [0x09] = {
        .BG =   { C(0xFFFFCE), C(0x63EFEF), C(0x9C8431), C(0x5A5A5A) },
        .OBJ0 = { C(0xFFFFCE), C(0x63EFEF), C(0x9C8431), C(0x5A5A5A) },
        .OBJ1 = { C(0xFFFFCE), C(0x63EFEF), C(0x9C8431), C(0x5A5A5A) }
    },
    [0x0A] = {
        .BG =   { C(0xB5B5FF), C(0xFFFF94), C(0xAD5A42), C(0x000000) },
        .OBJ0 = { C(0xB5B5FF), C(0xFFFF94), C(0xAD5A42), C(0x000000) },
        .OBJ1 = { C(0xB5B5FF), C(0xFFFF94), C(0xAD5A42), C(0x000000) }
    },
    [0x0B] = {
        .BG =   { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) }
    },
    [0x0C] = {
        .BG =   { C(0xFFFFFF), C(0x8C8CDE), C(0x52528C), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0x8C8CDE), C(0x52528C), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x8C8CDE), C(0x52528C), C(0x000000) }
    },
    [0x0D] = {
        .BG =   { C(0xFFFFFF), C(0x8C8CDE), C(0x52528C), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0x8C8CDE), C(0x52528C), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x8C8CDE), C(0x52528C), C(0x000000) }
    },
    [0x0E] = {
        .BG =   { C(0xFFFFFF), C(0x7BFF31), C(0x008400), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0x7BFF31), C(0x008400), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x7BFF31), C(0x008400), C(0x000000) }
    },
    [0x0F] = {
        .BG =   { C(0xFFFFFF), C(0xFFAD63), C(0x843100), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFFAD63), C(0x843100), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFFAD63), C(0x843100), C(0x000000) }
    },
    [0x10] = {
        .BG =   { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) }
    },
    [0x11] = {
        .BG =   { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) }
    },
    [0x12] = {
        .BG =   { C(0xFFFFFF), C(0xFFAD63), C(0x843100), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFFAD63), C(0x843100), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFFAD63), C(0x843100), C(0x000000) }
    },
    [0x13] = {
        .BG =   { C(0x000000), C(0x008484), C(0xFFDE00), C(0xFFFFFF) },
        .OBJ0 = { C(0x000000), C(0x008484), C(0xFFDE00), C(0xFFFFFF) },
        .OBJ1 = { C(0x000000), C(0x008484), C(0xFFDE00), C(0xFFFFFF) }
    },
    [0x14] = {
        .BG =   { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) }
    },
    [0x15] = {
        .BG =   { C(0xFFFFFF), C(0xADAD84), C(0x42737B), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xADAD84), C(0x42737B), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xADAD84), C(0x42737B), C(0x000000) }
    },
    [0x16] = {
        .BG =   { C(0xFFFFFF), C(0xA5A5A5), C(0x525252), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xA5A5A5), C(0x525252), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xA5A5A5), C(0x525252), C(0x000000) }
    },
    [0x17] = {
        .BG =   { C(0xFFFFA5), C(0xFF9494), C(0x9494FF), C(0x000000) },
        .OBJ0 = { C(0xFFFFA5), C(0xFF9494), C(0x9494FF), C(0x000000) },
        .OBJ1 = { C(0xFFFFA5), C(0xFF9494), C(0x9494FF), C(0x000000) }
    },
    [0x18] = {
        .BG =   { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) }
    },
    [0x19] = {
        .BG =   { C(0xFFE6C5), C(0xCE9C84), C(0x846B29), C(0x5A3108) },
        .OBJ0 = { C(0xFFE6C5), C(0xCE9C84), C(0x846B29), C(0x5A3108) },
        .OBJ1 = { C(0xFFE6C5), C(0xCE9C84), C(0x846B29), C(0x5A3108) }
    },
    [0x1A] = {
        .BG =   { C(0xFFFFFF), C(0xFFFF00), C(0x7B4A00), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFFFF00), C(0x7B4A00), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFFFF00), C(0x7B4A00), C(0x000000) }
    },
    [0x1B] = {
        .BG =   { C(0xFFFFFF), C(0xFFCE00), C(0x9C6300), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFFCE00), C(0x9C6300), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFFCE00), C(0x9C6300), C(0x000000) }
    },
    [0x1C] = {
        .BG =   { C(0xFFFFFF), C(0x7BFF31), C(0x0063C5), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0x7BFF31), C(0x0063C5), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x7BFF31), C(0x0063C5), C(0x000000) }
    }
};

static const struct GB_PaletteEntry PALETTE_TABLE_0x1[] = {
    [0x00] = {
        .BG =   { C(0xFFFFFF), C(0xADAD84), C(0x42737B), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFF7300), C(0x944200), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xADAD84), C(0x42737B), C(0x000000) }
    },
    [0x01] = {
        .BG =   { C(0xFFFF9C), C(0x94B5FF), C(0x639473), C(0x003A3A) },
        .OBJ0 = { C(0xFFC542), C(0xFFD600), C(0x943A00), C(0x4A0000) },
        .OBJ1 = { C(0xFFFF9C), C(0x94B5FF), C(0x639473), C(0x003A3A) }
    },
    [0x02] = {
        .BG =   { C(0x6BFF00), C(0xFFFFFF), C(0xFF524A), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFFFFFF), C(0x63A5FF), C(0x0000FF) },
        .OBJ1 = { C(0x6BFF00), C(0xFFFFFF), C(0xFF524A), C(0x000000) }
    },
    [0x03] = {
        .BG =   { C(0x52DE00), C(0xFF8400), C(0xFFFF00), C(0xFFFFFF) },
        .OBJ0 = { C(0xFFFFFF), C(0xFFFFFF), C(0x63A5FF), C(0x0000FF) },
        .OBJ1 = { C(0x52DE00), C(0xFF8400), C(0xFFFF00), C(0xFFFFFF) }
    },
    [0x04] = {
        .BG =   { C(0xFFFFFF), C(0x7BFF00), C(0xB57300), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x7BFF00), C(0xB57300), C(0x000000) }
    },
    [0x05] = {
        .BG =   { C(0xFFFFFF), C(0x52FF00), C(0xFF4200), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x52FF00), C(0xFF4200), C(0x000000) }
    },
    [0x06] = {
        .BG =   { C(0xFFFFFF), C(0xFF9C00), C(0xFF0000), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFF9C00), C(0xFF0000), C(0x000000) }
    },
    [0x07] = {
        .BG =   { C(0xFFFFFF), C(0xFFFF00), C(0xFF0000), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFFFF00), C(0xFF0000), C(0x000000) }
    },
    [0x08] = {
        .BG =   { C(0xA59CFF), C(0xFFFF00), C(0x006300), C(0x000000) },
        .OBJ0 = { C(0xFF6352), C(0xD60000), C(0x630000), C(0x000000) },
        .OBJ1 = { C(0xA59CFF), C(0xFFFF00), C(0x006300), C(0x000000) }
    },
    [0x09] = {
        .BG =   { C(0xFFFFCE), C(0x63EFEF), C(0x9C8431), C(0x5A5A5A) },
        .OBJ0 = { C(0xFFFFFF), C(0xFF7300), C(0x944200), C(0x000000) },
        .OBJ1 = { C(0xFFFFCE), C(0x63EFEF), C(0x9C8431), C(0x5A5A5A) }
    },
    [0x0A] = {
        .BG =   { C(0xB5B5FF), C(0xFFFF94), C(0xAD5A42), C(0x000000) },
        .OBJ0 = { C(0x000000), C(0xFFFFFF), C(0xFF8484), C(0x943A3A) },
        .OBJ1 = { C(0xB5B5FF), C(0xFFFF94), C(0xAD5A42), C(0x000000) }
    },
    [0x0B] = {
        .BG =   { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) }
    },
    [0x0C] = {
        .BG =   { C(0xFFFFFF), C(0x8C8CDE), C(0x52528C), C(0x000000) },
        .OBJ0 = { C(0xFFC542), C(0xFFD600), C(0x943A00), C(0x4A0000) },
        .OBJ1 = { C(0xFFFFFF), C(0x8C8CDE), C(0x52528C), C(0x000000) }
    },
    [0x0D] = {
        .BG =   { C(0xFFFFFF), C(0x8C8CDE), C(0x52528C), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x8C8CDE), C(0x52528C), C(0x000000) }
    },
    [0x0E] = {
        .BG =   { C(0xFFFFFF), C(0x7BFF31), C(0x008400), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x7BFF31), C(0x008400), C(0x000000) }
    },
    [0x0F] = {
        .BG =   { C(0xFFFFFF), C(0xFFAD63), C(0x843100), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFFAD63), C(0x843100), C(0x000000) }
    },
    [0x10] = {
        .BG =   { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0x7BFF31), C(0x008400), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) }
    },
    [0x11] = {
        .BG =   { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0x00FF00), C(0x318400), C(0x004A00) },
        .OBJ1 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) }
    },
    [0x12] = {
        .BG =   { C(0xFFFFFF), C(0xFFAD63), C(0x843100), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0x7BFF31), C(0x008400), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFFAD63), C(0x843100), C(0x000000) }
    },
    [0x13] = {
        .BG =   { C(0x000000), C(0x008484), C(0xFFDE00), C(0xFFFFFF) },
        .OBJ0 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) },
        .OBJ1 = { C(0x000000), C(0x008484), C(0xFFDE00), C(0xFFFFFF) }
    },
    [0x14] = {
        .BG =   { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) },
        .OBJ0 = { C(0xFFFF00), C(0xFF0000), C(0x630000), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) }
    },
    [0x15] = {
        .BG =   { C(0xFFFFFF), C(0xADAD84), C(0x42737B), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFFAD63), C(0x843100), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xADAD84), C(0x42737B), C(0x000000) }
    },
    [0x16] = {
        .BG =   { C(0xFFFFFF), C(0xA5A5A5), C(0x525252), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xA5A5A5), C(0x525252), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xA5A5A5), C(0x525252), C(0x000000) }
    },
    [0x17] = {
        .BG =   { C(0xFFFFA5), C(0xFF9494), C(0x9494FF), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0x7BFF31), C(0x008400), C(0x000000) },
        .OBJ1 = { C(0xFFFFA5), C(0xFF9494), C(0x9494FF), C(0x000000) }
    },
    [0x18] = {
        .BG =   { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) }
    },
    [0x19] = {
        .BG =   { C(0xFFE6C5), C(0xCE9C84), C(0x846B29), C(0x5A3108) },
        .OBJ0 = { C(0xFFFFFF), C(0xFFAD63), C(0x843100), C(0x000000) },
        .OBJ1 = { C(0xFFE6C5), C(0xCE9C84), C(0x846B29), C(0x5A3108) }
    },
    [0x1A] = {
        .BG =   { C(0xFFFFFF), C(0xFFFF00), C(0x7B4A00), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFFFF00), C(0x7B4A00), C(0x000000) }
    },
    [0x1B] = {
        .BG =   { C(0xFFFFFF), C(0xFFCE00), C(0x9C6300), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFFCE00), C(0x9C6300), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFFCE00), C(0x9C6300), C(0x000000) }
    },
    [0x1C] = {
        .BG =   { C(0xFFFFFF), C(0x7BFF31), C(0x0063C5), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x7BFF31), C(0x0063C5), C(0x000000) }
    },
};

static const struct GB_PaletteEntry PALETTE_TABLE_0x2[] = {
    [0x00] = {
        .BG =   { C(0xFFFFFF), C(0xADAD84), C(0x42737B), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xADAD84), C(0x42737B), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFF7300), C(0x944200), C(0x000000) }
    },
    [0x01] = {
        .BG =   { C(0xFFFF9C), C(0x94B5FF), C(0x639473), C(0x003A3A) },
        .OBJ0 = { C(0xFFFF9C), C(0x94B5FF), C(0x639473), C(0x003A3A) },
        .OBJ1 = { C(0xFFC542), C(0xFFD600), C(0x943A00), C(0x4A0000) }
    },
    [0x02] = {
        .BG =   { C(0x6BFF00), C(0xFFFFFF), C(0xFF524A), C(0x000000) },
        .OBJ0 = { C(0x6BFF00), C(0xFFFFFF), C(0xFF524A), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFFFFFF), C(0x63A5FF), C(0x0000FF) }
    },
    [0x03] = {
        .BG =   { C(0x52DE00), C(0xFF8400), C(0xFFFF00), C(0xFFFFFF) },
        .OBJ0 = { C(0x52DE00), C(0xFF8400), C(0xFFFF00), C(0xFFFFFF) },
        .OBJ1 = { C(0xFFFFFF), C(0xFFFFFF), C(0x63A5FF), C(0x0000FF) }
    },
    [0x04] = {
        .BG =   { C(0xFFFFFF), C(0x7BFF00), C(0xB57300), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0x7BFF00), C(0xB57300), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) }
    },
    [0x05] = {
        .BG =   { C(0xFFFFFF), C(0x52FF00), C(0xFF4200), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0x52FF00), C(0xFF4200), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) }
    },
    [0x06] = {
        .BG =   { C(0xFFFFFF), C(0xFF9C00), C(0xFF0000), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFF9C00), C(0xFF0000), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) }
    },
    [0x07] = {
        .BG =   { C(0xFFFFFF), C(0xFFFF00), C(0xFF0000), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFFFF00), C(0xFF0000), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) }
    },
    [0x08] = {
        .BG =   { C(0xA59CFF), C(0xFFFF00), C(0x006300), C(0x000000) },
        .OBJ0 = { C(0xA59CFF), C(0xFFFF00), C(0x006300), C(0x000000) },
        .OBJ1 = { C(0xFF6352), C(0xD60000), C(0x630000), C(0x000000) }
    },
    [0x09] = {
        .BG =   { C(0xFFFFCE), C(0x63EFEF), C(0x9C8431), C(0x5A5A5A) },
        .OBJ0 = { C(0xFFFFCE), C(0x63EFEF), C(0x9C8431), C(0x5A5A5A) },
        .OBJ1 = { C(0xFFFFFF), C(0xFF7300), C(0x944200), C(0x000000) }
    },
    [0x0A] = {
        .BG =   { C(0xB5B5FF), C(0xFFFF94), C(0xAD5A42), C(0x000000) },
        .OBJ0 = { C(0xB5B5FF), C(0xFFFF94), C(0xAD5A42), C(0x000000) },
        .OBJ1 = { C(0x000000), C(0xFFFFFF), C(0xFF8484), C(0x943A3A) }
    },
    [0x0B] = {
        .BG =   { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) }
    },
    [0x0C] = {
        .BG =   { C(0xFFFFFF), C(0x8C8CDE), C(0x52528C), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0x8C8CDE), C(0x52528C), C(0x000000) },
        .OBJ1 = { C(0xFFC542), C(0xFFD600), C(0x943A00), C(0x4A0000) }
    },
    [0x0D] = {
        .BG =   { C(0xFFFFFF), C(0x8C8CDE), C(0x52528C), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0x8C8CDE), C(0x52528C), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) }
    },
    [0x0E] = {
        .BG =   { C(0xFFFFFF), C(0x7BFF31), C(0x008400), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0x7BFF31), C(0x008400), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) }
    },
    [0x0F] = {
        .BG =   { C(0xFFFFFF), C(0xFFAD63), C(0x843100), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFFAD63), C(0x843100), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) }
    },
    [0x10] = {
        .BG =   { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x7BFF31), C(0x008400), C(0x000000) }
    },
    [0x11] = {
        .BG =   { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x00FF00), C(0x318400), C(0x004A00) }
    },
    [0x12] = {
        .BG =   { C(0xFFFFFF), C(0xFFAD63), C(0x843100), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFFAD63), C(0x843100), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x7BFF31), C(0x008400), C(0x000000) }
    },
    [0x13] = {
        .BG =   { C(0x000000), C(0x008484), C(0xFFDE00), C(0xFFFFFF) },
        .OBJ0 = { C(0x000000), C(0x008484), C(0xFFDE00), C(0xFFFFFF) },
        .OBJ1 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) }
    },
    [0x14] = {
        .BG =   { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) },
        .OBJ1 = { C(0xFFFF00), C(0xFF0000), C(0x630000), C(0x000000) }
    },
    [0x15] = {
        .BG =   { C(0xFFFFFF), C(0xADAD84), C(0x42737B), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xADAD84), C(0x42737B), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFFAD63), C(0x843100), C(0x000000) }
    },
    [0x16] = {
        .BG =   { C(0xFFFFFF), C(0xA5A5A5), C(0x525252), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xA5A5A5), C(0x525252), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xA5A5A5), C(0x525252), C(0x000000) }
    },
    [0x17] = {
        .BG =   { C(0xFFFFA5), C(0xFF9494), C(0x9494FF), C(0x000000) },
        .OBJ0 = { C(0xFFFFA5), C(0xFF9494), C(0x9494FF), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x7BFF31), C(0x008400), C(0x000000) }
    },
    [0x18] = {
        .BG =   { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) }
    },
    [0x19] = {
        .BG =   { C(0xFFE6C5), C(0xCE9C84), C(0x846B29), C(0x5A3108) },
        .OBJ0 = { C(0xFFE6C5), C(0xCE9C84), C(0x846B29), C(0x5A3108) },
        .OBJ1 = { C(0xFFFFFF), C(0xFFAD63), C(0x843100), C(0x000000) }
    },
    [0x1A] = {
        .BG =   { C(0xFFFFFF), C(0xFFFF00), C(0x7B4A00), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFFFF00), C(0x7B4A00), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) }
    },
    [0x1B] = {
        .BG =   { C(0xFFFFFF), C(0xFFCE00), C(0x9C6300), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFFCE00), C(0x9C6300), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFFCE00), C(0x9C6300), C(0x000000) }
    },
    [0x1C] = {
        .BG =   { C(0xFFFFFF), C(0x7BFF31), C(0x0063C5), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0x7BFF31), C(0x0063C5), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) }
    }
};

static const struct GB_PaletteEntry PALETTE_TABLE_0x3[] = {
    [0x00] = {
        .BG =   { C(0xFFFFFF), C(0xADAD84), C(0x42737B), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFF7300), C(0x944200), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFF7300), C(0x944200), C(0x000000) }
    },
    [0x01] = {
        .BG =   { C(0xFFFF9C), C(0x94B5FF), C(0x639473), C(0x003A3A) },
        .OBJ0 = { C(0xFFC542), C(0xFFD600), C(0x943A00), C(0x4A0000) },
        .OBJ1 = { C(0xFFC542), C(0xFFD600), C(0x943A00), C(0x4A0000) }
    },
    [0x02] = {
        .BG =   { C(0x6BFF00), C(0xFFFFFF), C(0xFF524A), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFFFFFF), C(0x63A5FF), C(0x0000FF) },
        .OBJ1 = { C(0xFFFFFF), C(0xFFFFFF), C(0x63A5FF), C(0x0000FF) }
    },
    [0x03] = {
        .BG =   { C(0x52DE00), C(0xFF8400), C(0xFFFF00), C(0xFFFFFF) },
        .OBJ0 = { C(0xFFFFFF), C(0xFFFFFF), C(0x63A5FF), C(0x0000FF) },
        .OBJ1 = { C(0xFFFFFF), C(0xFFFFFF), C(0x63A5FF), C(0x0000FF) }
    },
    [0x04] = {
        .BG =   { C(0xFFFFFF), C(0x7BFF00), C(0xB57300), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) }
    },
    [0x05] = {
        .BG =   { C(0xFFFFFF), C(0x52FF00), C(0xFF4200), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) }
    },
    [0x06] = {
        .BG =   { C(0xFFFFFF), C(0xFF9C00), C(0xFF0000), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) }
    },
    [0x07] = {
        .BG =   { C(0xFFFFFF), C(0xFFFF00), C(0xFF0000), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) }
    },
    [0x08] = {
        .BG =   { C(0xA59CFF), C(0xFFFF00), C(0x006300), C(0x000000) },
        .OBJ0 = { C(0xFF6352), C(0xD60000), C(0x630000), C(0x000000) },
        .OBJ1 = { C(0xFF6352), C(0xD60000), C(0x630000), C(0x000000) }
    },
    [0x09] = {
        .BG =   { C(0xFFFFCE), C(0x63EFEF), C(0x9C8431), C(0x5A5A5A) },
        .OBJ0 = { C(0xFFFFFF), C(0xFF7300), C(0x944200), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFF7300), C(0x944200), C(0x000000) }
    },
    [0x0A] = {
        .BG =   { C(0xB5B5FF), C(0xFFFF94), C(0xAD5A42), C(0x000000) },
        .OBJ0 = { C(0x000000), C(0xFFFFFF), C(0xFF8484), C(0x943A3A) },
        .OBJ1 = { C(0x000000), C(0xFFFFFF), C(0xFF8484), C(0x943A3A) }
    },
    [0x0B] = {
        .BG =   { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) }
    },
    [0x0C] = {
        .BG =   { C(0xFFFFFF), C(0x8C8CDE), C(0x52528C), C(0x000000) },
        .OBJ0 = { C(0xFFC542), C(0xFFD600), C(0x943A00), C(0x4A0000) },
        .OBJ1 = { C(0xFFC542), C(0xFFD600), C(0x943A00), C(0x4A0000) }
    },
    [0x0D] = {
        .BG =   { C(0xFFFFFF), C(0x8C8CDE), C(0x52528C), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) }
    },
    [0x0E] = {
        .BG =   { C(0xFFFFFF), C(0x7BFF31), C(0x008400), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) }
    },
    [0x0F] = {
        .BG =   { C(0xFFFFFF), C(0xFFAD63), C(0x843100), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) }
    },
    [0x10] = {
        .BG =   { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0x7BFF31), C(0x008400), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x7BFF31), C(0x008400), C(0x000000) }
    },
    [0x11] = {
        .BG =   { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0x00FF00), C(0x318400), C(0x004A00) },
        .OBJ1 = { C(0xFFFFFF), C(0x00FF00), C(0x318400), C(0x004A00) }
    },
    [0x12] = {
        .BG =   { C(0xFFFFFF), C(0xFFAD63), C(0x843100), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0x7BFF31), C(0x008400), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x7BFF31), C(0x008400), C(0x000000) }
    },
    [0x13] = {
        .BG =   { C(0x000000), C(0x008484), C(0xFFDE00), C(0xFFFFFF) },
        .OBJ0 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) }
    },
    [0x14] = {
        .BG =   { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) },
        .OBJ0 = { C(0xFFFF00), C(0xFF0000), C(0x630000), C(0x000000) },
        .OBJ1 = { C(0xFFFF00), C(0xFF0000), C(0x630000), C(0x000000) }
    },
    [0x15] = {
        .BG =   { C(0xFFFFFF), C(0xADAD84), C(0x42737B), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFFAD63), C(0x843100), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFFAD63), C(0x843100), C(0x000000) }
    },
    [0x16] = {
        .BG =   { C(0xFFFFFF), C(0xA5A5A5), C(0x525252), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xA5A5A5), C(0x525252), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xA5A5A5), C(0x525252), C(0x000000) }
    },
    [0x17] = {
        .BG =   { C(0xFFFFA5), C(0xFF9494), C(0x9494FF), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0x7BFF31), C(0x008400), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x7BFF31), C(0x008400), C(0x000000) }
    },
    [0x18] = {
        .BG =   { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) }
    },
    [0x19] = {
        .BG =   { C(0xFFE6C5), C(0xCE9C84), C(0x846B29), C(0x5A3108) },
        .OBJ0 = { C(0xFFFFFF), C(0xFFAD63), C(0x843100), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFFAD63), C(0x843100), C(0x000000) }
    },
    [0x1A] = {
        .BG =   { C(0xFFFFFF), C(0xFFFF00), C(0x7B4A00), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) }
    },
    [0x1B] = {
        .BG =   { C(0xFFFFFF), C(0xFFCE00), C(0x9C6300), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFFCE00), C(0x9C6300), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFFCE00), C(0x9C6300), C(0x000000) }
    },
    [0x1C] = {
        .BG =   { C(0xFFFFFF), C(0x7BFF31), C(0x0063C5), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) }
    }
};

static const struct GB_PaletteEntry PALETTE_TABLE_0x4[] = {
    [0x00] = {
        .BG =   { C(0xFFFFFF), C(0xADAD84), C(0x42737B), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xADAD84), C(0x42737B), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x5ABDFF), C(0xFF0000), C(0x0000FF) }
    },
    [0x01] = {
        .BG =   { C(0xFFFF9C), C(0x94B5FF), C(0x639473), C(0x003A3A) },
        .OBJ0 = { C(0xFFFF9C), C(0x94B5FF), C(0x639473), C(0x003A3A) },
        .OBJ1 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) }
    },
    [0x02] = {
        .BG =   { C(0x6BFF00), C(0xFFFFFF), C(0xFF524A), C(0x000000) },
        .OBJ0 = { C(0x6BFF00), C(0xFFFFFF), C(0xFF524A), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFFAD63), C(0x843100), C(0x000000) }
    },
    [0x03] = {
        .BG =   { C(0x52DE00), C(0xFF8400), C(0xFFFF00), C(0xFFFFFF) },
        .OBJ0 = { C(0x52DE00), C(0xFF8400), C(0xFFFF00), C(0xFFFFFF) },
        .OBJ1 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) }
    },
    [0x04] = {
        .BG =   { C(0xFFFFFF), C(0x7BFF00), C(0xB57300), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0x7BFF00), C(0xB57300), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) }
    },
    [0x05] = {
        .BG =   { C(0xFFFFFF), C(0x52FF00), C(0xFF4200), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0x52FF00), C(0xFF4200), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x5ABDFF), C(0xFF0000), C(0x0000FF) }
    },
    [0x06] = {
        .BG =   { C(0xFFFFFF), C(0xFF9C00), C(0xFF0000), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFF9C00), C(0xFF0000), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x5ABDFF), C(0xFF0000), C(0x0000FF) }
    },
    [0x07] = {
        .BG =   { C(0xFFFFFF), C(0xFFFF00), C(0xFF0000), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFFFF00), C(0xFF0000), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x5ABDFF), C(0xFF0000), C(0x0000FF) }
    },
    [0x08] = {
        .BG =   { C(0xA59CFF), C(0xFFFF00), C(0x006300), C(0x000000) },
        .OBJ0 = { C(0xA59CFF), C(0xFFFF00), C(0x006300), C(0x000000) },
        .OBJ1 = { C(0x0000FF), C(0xFFFFFF), C(0xFFFF7B), C(0x0084FF) }
    },
    [0x09] = {
        .BG =   { C(0xFFFFCE), C(0x63EFEF), C(0x9C8431), C(0x5A5A5A) },
        .OBJ0 = { C(0xFFFFCE), C(0x63EFEF), C(0x9C8431), C(0x5A5A5A) },
        .OBJ1 = { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) }
    },
    [0x0A] = {
        .BG =   { C(0xB5B5FF), C(0xFFFF94), C(0xAD5A42), C(0x000000) },
        .OBJ0 = { C(0xB5B5FF), C(0xFFFF94), C(0xAD5A42), C(0x000000) },
        .OBJ1 = { C(0x000000), C(0xFFFFFF), C(0xFF8484), C(0x943A3A) }
    },
    [0x0B] = {
        .BG =   { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFFFF7B), C(0x0084FF), C(0xFF0000) }
    },
    [0x0C] = {
        .BG =   { C(0xFFFFFF), C(0x8C8CDE), C(0x52528C), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0x8C8CDE), C(0x52528C), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x5ABDFF), C(0xFF0000), C(0x0000FF) }
    },
    [0x0D] = {
        .BG =   { C(0xFFFFFF), C(0x8C8CDE), C(0x52528C), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0x8C8CDE), C(0x52528C), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFFAD63), C(0x843100), C(0x000000) }
    },
    [0x0E] = {
        .BG =   { C(0xFFFFFF), C(0x7BFF31), C(0x008400), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0x7BFF31), C(0x008400), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) }
    },
    [0x0F] = {
        .BG =   { C(0xFFFFFF), C(0xFFAD63), C(0x843100), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFFAD63), C(0x843100), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x7BFF31), C(0x008400), C(0x000000) }
    },
    [0x10] = {
        .BG =   { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) }
    },
    [0x11] = {
        .BG =   { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) }
    },
    [0x12] = {
        .BG =   { C(0xFFFFFF), C(0xFFAD63), C(0x843100), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFFAD63), C(0x843100), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) }
    },
    [0x13] = {
        .BG =   { C(0x000000), C(0x008484), C(0xFFDE00), C(0xFFFFFF) },
        .OBJ0 = { C(0x000000), C(0x008484), C(0xFFDE00), C(0xFFFFFF) },
        .OBJ1 = { C(0xFFFFFF), C(0x7BFF31), C(0x008400), C(0x000000) }
    },
    [0x14] = {
        .BG =   { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x7BFF31), C(0x008400), C(0x000000) }
    },
    [0x15] = {
        .BG =   { C(0xFFFFFF), C(0xADAD84), C(0x42737B), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xADAD84), C(0x42737B), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) }
    },
    [0x16] = {
        .BG =   { C(0xFFFFFF), C(0xA5A5A5), C(0x525252), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xA5A5A5), C(0x525252), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xA5A5A5), C(0x525252), C(0x000000) }
    },
    [0x17] = {
        .BG =   { C(0xFFFFA5), C(0xFF9494), C(0x9494FF), C(0x000000) },
        .OBJ0 = { C(0xFFFFA5), C(0xFF9494), C(0x9494FF), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) }
    },
    [0x18] = {
        .BG =   { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x7BFF31), C(0x008400), C(0x000000) }
    },
    [0x19] = {
        .BG =   { C(0xFFE6C5), C(0xCE9C84), C(0x846B29), C(0x5A3108) },
        .OBJ0 = { C(0xFFE6C5), C(0xCE9C84), C(0x846B29), C(0x5A3108) },
        .OBJ1 = { C(0xFFFFFF), C(0xFFAD63), C(0x843100), C(0x000000) }
    },
    [0x1A] = {
        .BG =   { C(0xFFFFFF), C(0xFFFF00), C(0x7B4A00), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFFFF00), C(0x7B4A00), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x7BFF31), C(0x008400), C(0x000000) }
    },
    [0x1B] = {
        .BG =   { C(0xFFFFFF), C(0xFFCE00), C(0x9C6300), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFFCE00), C(0x9C6300), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFFCE00), C(0x9C6300), C(0x000000) }
    },
    [0x1C] = {
        .BG =   { C(0xFFFFFF), C(0x7BFF31), C(0x0063C5), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0x7BFF31), C(0x0063C5), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) }
    }
};

static const struct GB_PaletteEntry PALETTE_TABLE_0x5[] = {
    [0x00] = {
        .BG =   { C(0xFFFFFF), C(0xADAD84), C(0x42737B), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFF7300), C(0x944200), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x5ABDFF), C(0xFF0000), C(0x0000FF) }
    },
    [0x01] = {
        .BG =   { C(0xFFFF9C), C(0x94B5FF), C(0x639473), C(0x003A3A) },
        .OBJ0 = { C(0xFFC542), C(0xFFD600), C(0x943A00), C(0x4A0000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) }
    },
    [0x02] = {
        .BG =   { C(0x6BFF00), C(0xFFFFFF), C(0xFF524A), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFFFFFF), C(0x63A5FF), C(0x0000FF) },
        .OBJ1 = { C(0xFFFFFF), C(0xFFAD63), C(0x843100), C(0x000000) }
    },
    [0x03] = {
        .BG =   { C(0x52DE00), C(0xFF8400), C(0xFFFF00), C(0xFFFFFF) },
        .OBJ0 = { C(0xFFFFFF), C(0xFFFFFF), C(0x63A5FF), C(0x0000FF) },
        .OBJ1 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) }
    },
    [0x04] = {
        .BG =   { C(0xFFFFFF), C(0x7BFF00), C(0xB57300), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) }
    },
    [0x05] = {
        .BG =   { C(0xFFFFFF), C(0x52FF00), C(0xFF4200), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x5ABDFF), C(0xFF0000), C(0x0000FF) }
    },
    [0x06] = {
        .BG =   { C(0xFFFFFF), C(0xFF9C00), C(0xFF0000), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x5ABDFF), C(0xFF0000), C(0x0000FF) }
    },
    [0x07] = {
        .BG =   { C(0xFFFFFF), C(0xFFFF00), C(0xFF0000), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x5ABDFF), C(0xFF0000), C(0x0000FF) }
    },
    [0x08] = {
        .BG =   { C(0xA59CFF), C(0xFFFF00), C(0x006300), C(0x000000) },
        .OBJ0 = { C(0xFF6352), C(0xD60000), C(0x630000), C(0x000000) },
        .OBJ1 = { C(0x0000FF), C(0xFFFFFF), C(0xFFFF7B), C(0x0084FF) }
    },
    [0x09] = {
        .BG =   { C(0xFFFFCE), C(0x63EFEF), C(0x9C8431), C(0x5A5A5A) },
        .OBJ0 = { C(0xFFFFFF), C(0xFF7300), C(0x944200), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) }
    },
    [0x0A] = {
        .BG =   { C(0xB5B5FF), C(0xFFFF94), C(0xAD5A42), C(0x000000) },
        .OBJ0 = { C(0x000000), C(0xFFFFFF), C(0xFF8484), C(0x943A3A) },
        .OBJ1 = { C(0x000000), C(0xFFFFFF), C(0xFF8484), C(0x943A3A) }
    },
    [0x0B] = {
        .BG =   { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFFFF7B), C(0x0084FF), C(0xFF0000) }
    },
    [0x0C] = {
        .BG =   { C(0xFFFFFF), C(0x8C8CDE), C(0x52528C), C(0x000000) },
        .OBJ0 = { C(0xFFC542), C(0xFFD600), C(0x943A00), C(0x4A0000) },
        .OBJ1 = { C(0xFFFFFF), C(0x5ABDFF), C(0xFF0000), C(0x0000FF) }
    },
    [0x0D] = {
        .BG =   { C(0xFFFFFF), C(0x8C8CDE), C(0x52528C), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFFAD63), C(0x843100), C(0x000000) }
    },
    [0x0E] = {
        .BG =   { C(0xFFFFFF), C(0x7BFF31), C(0x008400), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) }
    },
    [0x0F] = {
        .BG =   { C(0xFFFFFF), C(0xFFAD63), C(0x843100), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x7BFF31), C(0x008400), C(0x000000) }
    },
    [0x10] = {
        .BG =   { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0x7BFF31), C(0x008400), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) }
    },
    [0x11] = {
        .BG =   { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0x00FF00), C(0x318400), C(0x004A00) },
        .OBJ1 = { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) }
    },
    [0x12] = {
        .BG =   { C(0xFFFFFF), C(0xFFAD63), C(0x843100), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0x7BFF31), C(0x008400), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) }
    },
    [0x13] = {
        .BG =   { C(0x000000), C(0x008484), C(0xFFDE00), C(0xFFFFFF) },
        .OBJ0 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x7BFF31), C(0x008400), C(0x000000) }
    },
    [0x14] = {
        .BG =   { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) },
        .OBJ0 = { C(0xFFFF00), C(0xFF0000), C(0x630000), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x7BFF31), C(0x008400), C(0x000000) }
    },
    [0x15] = {
        .BG =   { C(0xFFFFFF), C(0xADAD84), C(0x42737B), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFFAD63), C(0x843100), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) }
    },
    [0x16] = {
        .BG =   { C(0xFFFFFF), C(0xA5A5A5), C(0x525252), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xA5A5A5), C(0x525252), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xA5A5A5), C(0x525252), C(0x000000) }
    },
    [0x17] = {
        .BG =   { C(0xFFFFA5), C(0xFF9494), C(0x9494FF), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0x7BFF31), C(0x008400), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) }
    },
    [0x18] = {
        .BG =   { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x7BFF31), C(0x008400), C(0x000000) }
    },
    [0x19] = {
        .BG =   { C(0xFFE6C5), C(0xCE9C84), C(0x846B29), C(0x5A3108) },
        .OBJ0 = { C(0xFFFFFF), C(0xFFAD63), C(0x843100), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFFAD63), C(0x843100), C(0x000000) }
    },
    [0x1A] = {
        .BG =   { C(0xFFFFFF), C(0xFFFF00), C(0x7B4A00), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x7BFF31), C(0x008400), C(0x000000) }
    },
    [0x1B] = {
        .BG =   { C(0xFFFFFF), C(0xFFCE00), C(0x9C6300), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFFCE00), C(0x9C6300), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0xFFCE00), C(0x9C6300), C(0x000000) }
    },
    [0x1C] = {
        .BG =   { C(0xFFFFFF), C(0x7BFF31), C(0x0063C5), C(0x000000) },
        .OBJ0 = { C(0xFFFFFF), C(0xFF8484), C(0x943A3A), C(0x000000) },
        .OBJ1 = { C(0xFFFFFF), C(0x63A5FF), C(0x0000FF), C(0x000000) }
    }
};

static const struct GB_PaletteEntry* const PALETTE_TABLES[] = {
    PALETTE_TABLE_0x0,
    PALETTE_TABLE_0x1,
    PALETTE_TABLE_0x2,
    PALETTE_TABLE_0x3,
    PALETTE_TABLE_0x4,
    PALETTE_TABLE_0x5
};

static const struct GB_PaletteHashEntry PALETTE_HASH_ENTRIES[] = {
    /* Balloon Kid (USA, Europe) */
    /* Tetris Blast (USA, Europe) */
    {
        .table = 0x0,
        .entry = 0x06,
        .hash = 0x71
    },
    {
        .table = 0x0,
        .entry = 0x06,
        .hash = 0xFF
    },
    /* Pocket Monsters - Pikachu (Japan) (Rev 0A) */
    /* Pocket Monsters - Pikachu (Japan) (Rev B) */
    /* Pocket Monsters - Pikachu (Japan) (Rev C) */
    /* Pocket Monsters - Pikachu (Japan) (Rev D) */
    /* Tetris (World) */
    /* Tetris (World) (Rev A) */
    {
        .table = 0x0,
        .entry = 0x07,
        .hash = 0x15
    },
    {
        .table = 0x0,
        .entry = 0x07,
        .hash = 0xDB
    },
    /* Alleyway (World) */
    {
        .table = 0x0,
        .entry = 0x08,
        .hash = 0x88
    },
    /* F-1 Race (World) */
    /* F-1 Race (World) (Rev A) */
    /* Game Boy Gallery (Europe) */
    /* Kirby no Kirakira Kids (Japan) */
    /* Kirby's Star Stacker (USA, Europe) */
    /* Mario no Picross (Japan) */
    /* Mario's Picross (USA, Europe) */
    /* Nigel Mansell's World Championship Racing (Europe) */
    /* Picross 2 (Japan) */
    /* Yakuman (Japan) */
    /* Yakuman (Japan) (Rev A) */
    {
        .table = 0x0,
        .entry = 0x12,
        .hash = 0x0C
    },
    {
        .table = 0x0,
        .entry = 0x12,
        .hash = 0x16
    },
    {
        .table = 0x0,
        .entry = 0x12,
        .hash = 0x35
    },
    {
        .table = 0x0,
        .entry = 0x12,
        .hash = 0x67
    },
    {
        .table = 0x0,
        .entry = 0x12,
        .hash = 0x75
    },
    {
        .table = 0x0,
        .entry = 0x12,
        .hash = 0x92
    },
    {
        .table = 0x0,
        .entry = 0x12,
        .hash = 0x99
    },
    {
        .table = 0x0,
        .entry = 0x12,
        .hash = 0xB7
    },
    /* Arcade Classic No. 3 - Galaga & Galaxian (USA) */
    /* Solar Striker (World) */
    /* Space Invaders (Europe) */
    /* Space Invaders (USA) */
    {
        .table = 0x0,
        .entry = 0x13,
        .hash = 0x28
    },
    {
        .table = 0x0,
        .entry = 0x13,
        .hash = 0xA5,
        .forth = 0X41
    },
    {
        .table = 0x0,
        .entry = 0x13,
        .hash = 0xE8,
        .forth = 0X41
    },
    /* X (Japan) */
    {
        .table = 0x0,
        .entry = 0x16,
        .hash = 0x58
    },
    /* Pocket Camera (Japan) (Rev A) */
    {
        .table = 0x0,
        .entry = 0x1B,
        .hash = 0x6F
    },
    /* Radar Mission (Japan) */
    /* Radar Mission (USA, Europe) */
    {
        .table = 0x1,
        .entry = 0x00,
        .hash = 0x8C
    },
    /* Pocket Monsters Ao (Japan) */
    /* Pokemon - Blaue Edition (Germany) */
    /* Pokemon - Blue Version (USA, Europe) */
    /* Pokemon - Edicion Azul (Spain) */
    /* Pokemon - Version Bleue (France) */
    /* Pokemon - Versione Blu (Italy) */
    {
        .table = 0x1,
        .entry = 0x0B,
        .hash = 0x61
    },
    /* Kaeru no Tame ni Kane wa Naru (Japan) */
    {
        .table = 0x1,
        .entry = 0x0D,
        .hash = 0xD3,
        .forth = 0X52
    },
    /* Game Boy Camera Gold (USA) */
    /* Pocket Monsters Aka (Japan) */
    /* Pocket Monsters Aka (Japan) (Rev A) */
    /* Pokemon - Edicion Roja (Spain) */
    /* Pokemon - Red Version (USA, Europe) */
    /* Pokemon - Rote Edition (Germany) */
    /* Pokemon - Version Rouge (France) */
    /* Pokemon - Versione Rossa (Italy) */
    {
        .table = 0x1,
        .entry = 0x10,
        .hash = 0x14
    },
    /* James Bond 007 (USA, Europe) */
    /* Pocket Monsters Midori (Japan) */
    /* Pocket Monsters Midori (Japan) (Rev A) */
    {
        .table = 0x1,
        .entry = 0x1C,
        .hash = 0xAA
    },
    /* Dr. Mario (World) */
    /* Dr. Mario (World) (Rev A) */
    {
        .table = 0x2,
        .entry = 0x0B,
        .hash = 0x3C
    },
    /* Pinocchio (Europe) */
    {
        .table = 0x2,
        .entry = 0x0C,
        .hash = 0x9C
    },
    /* Moguranya (Japan) */
    /* Mole Mania (USA, Europe) */
    {
        .table = 0x3,
        .entry = 0x00,
        .hash = 0xB3,
        .forth = 0X55
    },
    /* Game & Watch Gallery (Europe) */
    /* Game & Watch Gallery (USA) */
    /* Game & Watch Gallery (USA) (Rev A) */
    /* Game Boy Gallery (Japan) */
    /* Game Boy Gallery 2 (Australia) */
    /* Game Boy Gallery 2 (Japan) */
    {
        .table = 0x3,
        .entry = 0x04,
        .hash = 0x34
    },
    {
        .table = 0x3,
        .entry = 0x04,
        .hash = 0x66,
        .forth = 0X45
    },
    {
        .table = 0x3,
        .entry = 0x04,
        .hash = 0xF4,
        .forth = 0X20
    },
    /* Mario & Yoshi (Europe) */
    /* Yoshi (USA) */
    /* Yoshi no Tamago (Japan) */
    {
        .table = 0x3,
        .entry = 0x05,
        .hash = 0x3D
    },
    {
        .table = 0x3,
        .entry = 0x05,
        .hash = 0x6A,
        .forth = 0X49
    },
    /* Donkey Kong (World) */
    /* Donkey Kong (World) (Rev A) */
    {
        .table = 0x3,
        .entry = 0x06,
        .hash = 0x19
    },
    /* Kirby no Pinball (Japan) */
    /* Kirby's Pinball Land (USA, Europe) */
    {
        .table = 0x3,
        .entry = 0x08,
        .hash = 0x1D
    },
    /* Super Mario Land (World) */
    /* Super Mario Land (World) (Rev A) */
    {
        .table = 0x3,
        .entry = 0x0A,
        .hash = 0x46,
        .forth = 0X45
    },
    /* Pocket Bomberman (Europe) */
    {
        .table = 0x3,
        .entry = 0x0C,
        .hash = 0x0D,
        .forth = 0X45
    },
    /* Kid Icarus - Of Myths and Monsters (USA, Europe) */
    {
        .table = 0x3,
        .entry = 0x0D,
        .hash = 0xBF,
        .forth = 0X20
    },
    /* Arcade Classic No. 1 - Asteroids & Missile Command (USA, Europe) */
    /* Golf (World) */
    /* Nintendo World Cup (USA, Europe) */
    /* Play Action Football (USA) */
    /* Toy Story (Europe) */
    {
        .table = 0x3,
        .entry = 0x0E,
        .hash = 0x28,
        .forth = 0X46
    },
    {
        .table = 0x3,
        .entry = 0x0E,
        .hash = 0x4B
    },
    {
        .table = 0x3,
        .entry = 0x0E,
        .hash = 0x90
    },
    {
        .table = 0x3,
        .entry = 0x0E,
        .hash = 0x9A
    },
    {
        .table = 0x3,
        .entry = 0x0E,
        .hash = 0xBD
    },
    /* Chessmaster, The (Europe) */
    /* Dynablaster (Europe) */
    /* King of the Zoo (Europe) */
    {
        .table = 0x3,
        .entry = 0x0F,
        .hash = 0x39
    },
    {
        .table = 0x3,
        .entry = 0x0F,
        .hash = 0x43
    },
    {
        .table = 0x3,
        .entry = 0x0F,
        .hash = 0x97
    },
    /* Battletoads in Ragnarok's World (Europe) */
    {
        .table = 0x3,
        .entry = 0x12,
        .hash = 0xA5,
        .forth = 0X52
    },
    /* Arcade Classic No. 2 - Centipede & Millipede (USA, Europe) */
    /* Ken Griffey Jr. presents Major League Baseball (USA, Europe) */
    /* Tetris Plus (USA, Europe) */
    /* Wario Blast Featuring Bomberman! (USA, Europe) */
    {
        .table = 0x3,
        .entry = 0x1C,
        .hash = 0x00
    },
    {
        .table = 0x3,
        .entry = 0x1C,
        .hash = 0x18,
        .forth = 0X49
    },
    {
        .table = 0x3,
        .entry = 0x1C,
        .hash = 0x3F
    },
    {
        .table = 0x3,
        .entry = 0x1C,
        .hash = 0x66,
        .forth = 0X4C
    },
    {
        .table = 0x3,
        .entry = 0x1C,
        .hash = 0xC6,
        .forth = 0X20
    },
    /* Tetris Attack (USA) */
    /* Tetris Attack (USA, Europe) (Rev A) */
    /* Yoshi no Panepon (Japan) */
    {
        .table = 0x4,
        .entry = 0x05,
        .hash = 0x95
    },
    {
        .table = 0x4,
        .entry = 0x05,
        .hash = 0xB3,
        .forth = 0X52
    },
    /* Yoshi no Cookie (Japan) */
    /* Yoshi's Cookie (USA, Europe) */
    {
        .table = 0x4,
        .entry = 0x06,
        .hash = 0x3E
    },
    {
        .table = 0x4,
        .entry = 0x06,
        .hash = 0xE0
    },
    /* Qix (World) */
    /* Tetris 2 (Europe) */
    /* Tetris 2 (USA) */
    /* Tetris Flash (Japan) */
    {
        .table = 0x4,
        .entry = 0x07,
        .hash = 0x0D,
        .forth = 0X52
    },
    {
        .table = 0x4,
        .entry = 0x07,
        .hash = 0x69 // ayyyy
    },
    {
        .table = 0x4,
        .entry = 0x07,
        .hash = 0xF2
    },
    /* Game Boy Wars (Japan) */
    /* Wario Land - Super Mario Land 3 (World) */
    {
        .table = 0x5,
        .entry = 0x00,
        .hash = 0x59
    },
    {
        .table = 0x5,
        .entry = 0x00,
        .hash = 0xC6,
        .forth = 0X41
    },
    /* Donkey Kong Land (USA, Europe) */
    /* Super Donkey Kong GB (Japan) */
    {
        .table = 0x5,
        .entry = 0x01,
        .hash = 0x86
    },
    {
        .table = 0x5,
        .entry = 0x01,
        .hash = 0xA8
    },
    /* Soccer (Europe) (En,Fr,De) */
    /* Tennis (World) */
    /* Top Rank Tennis (USA) */
    /* Top Ranking Tennis (Europe) */
    {
        .table = 0x5,
        .entry = 0x02,
        .hash = 0xBF,
        .forth = 0X43
    },
    {
        .table = 0x5,
        .entry = 0x02,
        .hash = 0xCE
    },
    {
        .table = 0x5,
        .entry = 0x02,
        .hash = 0xD1
    },
    {
        .table = 0x5,
        .entry = 0x02,
        .hash = 0xF0
    },
    /* Baseball (World) */
    {
        .table = 0x5,
        .entry = 0x03,
        .hash = 0x36
    },
    /* Hoshi no Kirby (Japan) */
    /* Hoshi no Kirby (Japan) (Rev A) */
    /* Hoshi no Kirby 2 (Japan) */
    /* Kirby no Block Ball (Japan) */
    /* Kirby's Block Ball (USA, Europe) */
    /* Kirby's Dream Land (USA, Europe) */
    /* Kirby's Dream Land 2 (USA, Europe) */
    {
        .table = 0x5,
        .entry = 0x08,
        .hash = 0x27,
        .forth = 0X42
    },
    {
        .table = 0x5,
        .entry = 0x08,
        .hash = 0x49
    },
    {
        .table = 0x5,
        .entry = 0x08,
        .hash = 0x5C
    },
    {
        .table = 0x5,
        .entry = 0x08,
        .hash = 0xB3,
        .forth = 0X42
    },
    /* Super Mario Land 2 - 6 Golden Coins (USA, Europe) */
    /* Super Mario Land 2 - 6 Golden Coins (USA, Europe) (Rev A) */
    /* Super Mario Land 2 - 6 Golden Coins (USA, Europe) (Rev B) */
    /* Super Mario Land 2 - 6-tsu no Kinka (Japan) */
    /* Super Mario Land 2 - 6-tsu no Kinka (Japan) (Rev B) */
    {
        .table = 0x5,
        .entry = 0x09,
        .hash = 0xC9
    },
    /* Wave Race (USA, Europe) */
    {
        .table = 0x5,
        .entry = 0x0B,
        .hash = 0x4E
    },
    /* Donkey Kong Land (Japan) */
    /* Donkey Kong Land 2 (USA, Europe) */
    /* Donkey Kong Land III (USA, Europe) */
    /* Donkey Kong Land III (USA, Europe) (Rev A) */
    {
        .table = 0x5,
        .entry = 0x0C,
        .hash = 0x18,
        .forth = 0X4B
    },
    {
        .table = 0x5,
        .entry = 0x0C,
        .hash = 0x6A,
        .forth = 0X4B
    },
    {
        .table = 0x5,
        .entry = 0x0C,
        .hash = 0x6B
    },
    /* Killer Instinct (USA, Europe) */
    {
        .table = 0x5,
        .entry = 0x0D,
        .hash = 0x9D
    },
    /* Magnetic Soccer (Europe) */
    /* Mystic Quest (Europe) */
    /* Mystic Quest (France) */
    /* Mystic Quest (Germany) */
    /* Othello (Europe) */
    /* Vegas Stakes (USA, Europe) */
    {
        .table = 0x5,
        .entry = 0x0E,
        .hash = 0x17
    },
    {
        .table = 0x5,
        .entry = 0x0E,
        .hash = 0x27,
        .forth = 0X4E
    },
    {
        .table = 0x5,
        .entry = 0x0E,
        .hash = 0x61,
        .forth = 0X41
    },
    {
        .table = 0x5,
        .entry = 0x0E,
        .hash = 0x8B
    },
    /* Adventures of Lolo (Europe) */
    /* Arcade Classic No. 4 - Defender & Joust (USA, Europe) */
    /* Battle Arena Toshinden (USA) */
    /* King of Fighters '95, The (USA) */
    /* Mega Man - Dr. Wily's Revenge (Europe) */
    /* Mega Man II (Europe) */
    /* Mega Man III (Europe) */
    /* Street Fighter II (USA, Europe) (Rev A) */
    /* Super R.C. Pro-Am (USA, Europe) */
    {
        .table = 0x5,
        .entry = 0x0F,
        .hash = 0x01
    },
    {
        .table = 0x5,
        .entry = 0x0F,
        .hash = 0x10
    },
    {
        .table = 0x5,
        .entry = 0x0F,
        .hash = 0x29
    },
    {
        .table = 0x5,
        .entry = 0x0F,
        .hash = 0x52
    },
    {
        .table = 0x5,
        .entry = 0x0F,
        .hash = 0x5D
    },
    {
        .table = 0x5,
        .entry = 0x0F,
        .hash = 0x68
    },
    {
        .table = 0x5,
        .entry = 0x0F,
        .hash = 0x6D
    },
    {
        .table = 0x5,
        .entry = 0x0F,
        .hash = 0xF6
    },
    /* Legend of Zelda, The - Link's Awakening (France) */
    /* Legend of Zelda, The - Link's Awakening (Germany) */
    /* Legend of Zelda, The - Link's Awakening (USA, Europe) */
    /* Legend of Zelda, The - Link's Awakening (USA, Europe) (Rev A) */
    /* Legend of Zelda, The - Link's Awakening (USA, Europe) (Rev B) */
    /* Zelda no Densetsu - Yume o Miru Shima (Japan) */
    /* Zelda no Densetsu - Yume o Miru Shima (Japan) (Rev A) */
    {
        .table = 0x5,
        .entry = 0x11,
        .hash = 0x70
    },
    /* Boy and His Blob in the Rescue of Princess Blobette, A (Europe) */
    /* Star Wars (USA, Europe) (Rev A) */
    {
        .table = 0x5,
        .entry = 0x12,
        .hash = 0xA2
    },
    {
        .table = 0x5,
        .entry = 0x12,
        .hash = 0xF7
    },
    /* Metroid II - Return of Samus (World) */
    {
        .table = 0x5,
        .entry = 0x14,
        .hash = 0x46,
        .forth = 0X52
    },
    /* Wario Land II (USA, Europe) */
    {
        .table = 0x5,
        .entry = 0x15,
        .hash = 0xD3,
        .forth = 0X49
    },
    /* Pac-In-Time (USA) */
    {
        .table = 0x5,
        .entry = 0x1C,
        .hash = 0xF4,
        .forth = 0X2D
    },
};

static const struct GB_PaletteButtonEntry PALETTE_BUTTON_ENTRIES[] = {
    {
        .table = 0x0,
        .entry = 0x05,
        .buttons = GB_BUTTON_RIGHT,
        .preview = {
            .shade1 = C(0x7BFF31),
            .shade2 = C(0xFF0000)
        }
    },
    {
        .table = 0x0,
        .entry = 0x07,
        .buttons = GB_BUTTON_A | GB_BUTTON_DOWN,
        .preview = {
            .shade1 = C(0xFFFF00),
            .shade2 = C(0xFF0000)
        }
    },
    {
        .table = 0x0,
        .entry = 0x12,
        .buttons = GB_BUTTON_UP,
        .preview = {
            .shade1 = C(0xF7C5A5),
            .shade2 = C(0x843100)
        }
    },
    {
        .table = 0x0,
        .entry = 0x13,
        .buttons = GB_BUTTON_B | GB_BUTTON_RIGHT,
        .preview = {
            .shade1 = C(0x000000),
            .shade2 = C(0xFFFF00)
        }
    },
    {
        .table = 0x0,
        .entry = 0x16,
        .buttons = GB_BUTTON_B | GB_BUTTON_LEFT,
        .preview = {
            .shade1 = C(0x949494),
            .shade2 = C(0x000000)
        }
    },
    {
        .table = 0x0,
        .entry = 0x17,
        .buttons = GB_BUTTON_DOWN,
        .preview = {
            .shade1 = C(0xFFFFA5),
            .shade2 = C(0xFF00FF)
        }
    },
    {
        .table = 0x3,
        .entry = 0x19,
        .buttons = GB_BUTTON_B | GB_BUTTON_UP,
        .preview = {
            .shade1 = C(0xA58452),
            .shade2 = C(0x6B5231)
        }
    },
    {
        .table = 0x3,
        .entry = 0x1C,
        .buttons = GB_BUTTON_A | GB_BUTTON_RIGHT,
        .preview = {
            .shade1 = C(0x7BFF31),
            .shade2 = C(0x0000FF)
        }
    },
    {
        .table = 0x5,
        .entry = 0x0D,
        .buttons = GB_BUTTON_A | GB_BUTTON_LEFT,
        .preview = {
            .shade1 = C(0x8C8CDE),
            .shade2 = C(0x52528C)
        }
    },
    {
        .table = 0x5,
        .entry = 0x10,
        .buttons = GB_BUTTON_A | GB_BUTTON_UP,
        .preview = {
            .shade1 = C(0xFF8484),
            .shade2 = C(0xE60000)
        }
    },
    {
        .table = 0x5,
        .entry = 0x18,
        .buttons = GB_BUTTON_LEFT,
        .preview = {
            .shade1 = C(0x63A5FF),
            .shade2 = C(0x0000FF)
        }
    },
    {
        .table = 0x5,
        .entry = 0x1A,
        .buttons = GB_BUTTON_B | GB_BUTTON_DOWN,
        .preview = {
            .shade1 = C(0xFFFF3A),
            .shade2 = C(0x3A2900)
        }
    }
};

#undef C

int GB_palette_fill_from_table_entry(
    uint8_t table, uint8_t entry, /* keys */
    struct GB_PaletteEntry* palette
) {
    assert(palette);
    assert(table <= GB_PALETTE_TABLE_MAX);
    assert(entry <= GB_PALETTE_ENTRY_MAX);

    if (!palette) {
        return -1;
    }

    if (table > GB_PALETTE_TABLE_MAX || entry > GB_PALETTE_ENTRY_MAX) {
        return -1;
    }

    *palette = PALETTE_TABLES[table][entry];

    return 0;
}

int GB_palette_fill_from_hash(
    uint8_t hash, /* key */
    uint8_t forth_byte, /* key */
    struct GB_PaletteEntry* palette
) {
    assert(palette);

    if (!palette) {
        return -1;
    }

    for (size_t i = 0; i < ARRAY_SIZE(PALETTE_HASH_ENTRIES); ++i) {
        if (hash == PALETTE_HASH_ENTRIES[i].hash) {
            /* some hashes collide, so the 4th byte of the title is used */
            /* luckily, all entries that collide has a non-zero 4th byte */
            if (PALETTE_HASH_ENTRIES[i].forth > 0 &&
                forth_byte != PALETTE_HASH_ENTRIES[i].forth) {
                continue;
            }

            GB_palette_fill_from_table_entry(
                PALETTE_HASH_ENTRIES[i].table,
                PALETTE_HASH_ENTRIES[i].entry,
                palette
            );

            return 0;
        }
    }

    return -1;
}

int GB_palette_fill_from_buttons(
    uint8_t buttons, /* key */
    struct GB_PaletteEntry* palette,
    struct GB_PalettePreviewShades* preview /* optional (can be NULL) */
) {
    assert(palette);

    if (!palette) {
        return -1;
    }

    for (size_t i = 0; i < ARRAY_SIZE(PALETTE_BUTTON_ENTRIES); ++i) {
        if (buttons == PALETTE_BUTTON_ENTRIES[i].buttons) {
            if (preview != NULL) {
                *preview = PALETTE_BUTTON_ENTRIES[i].preview;
            }

            GB_palette_fill_from_table_entry(
                PALETTE_BUTTON_ENTRIES[i].table,
                PALETTE_BUTTON_ENTRIES[i].entry,
                palette
            );

            return 0;
        }
    }

    return -1;
}

int GB_Palette_fill_from_custom(
    enum GB_CustomPalette custom, /* key */
    struct GB_PaletteEntry* palette
) {
    assert(palette);
    assert(custom < GB_CUSTOM_PALETTE_MAX && custom > 0);

    if (custom >= GB_CUSTOM_PALETTE_MAX || custom < 0) {
        return -1;
    }

    if (!palette) {
        return -1;
    }

    *palette = PALETTE_CUSTOM_TABLE[custom];

    return 0;
}
