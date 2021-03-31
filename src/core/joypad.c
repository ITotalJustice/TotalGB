#include "gb.h"
#include "internal.h"


static inline bool GB_is_directional(const struct GB_Core* gb) {
    return !!(IO_JYP & 0x10);
}

static inline bool GB_is_button(const struct GB_Core* gb) {
    return !!(IO_JYP & 0x20);
}

static inline uint8_t GB_joypad_get_internal(const struct GB_Core* gb) {
    return ((gb->joypad.var >> (GB_is_button(gb) << 2)) & 0xF);
}

// [API]
void GB_set_buttons(struct GB_Core* gb, uint8_t buttons, bool is_down) {
    gb->joypad.var = !is_down ? gb->joypad.var | buttons : gb->joypad.var & (~buttons);
}

uint8_t GB_get_buttons(const struct GB_Core* gb) {
    return gb->joypad.var;
}

bool GB_is_button_down(const struct GB_Core* gb, enum GB_Button button) {
    return (gb->joypad.var & button) > 0;
}

// [core]
uint8_t GB_joypad_read(const struct GB_Core* gb) {
    if (GB_get_system_type(gb) == GB_SYSTEM_TYPE_SGB) {
        uint8_t data = 0;
        // check if we handled it
        if (SGB_handle_joyp_read(gb, &data)) {
            return data;
        }
    }
    return (0xC0 | (IO_JYP & 0x30) | ((gb->joypad.var >> (GB_is_button(gb) << 2)) & 0xF));
}

void GB_joypad_write(struct GB_Core* gb, uint8_t value) {
    if (GB_get_system_type(gb) == GB_SYSTEM_TYPE_SGB) {
        SGB_handle_joyp_write(gb, value);
    }

    IO_JYP &= ~(0x30);
    IO_JYP |= 0x30 & value;
}
