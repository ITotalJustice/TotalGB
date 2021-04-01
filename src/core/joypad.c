#include "core/gb.h"
#include "core/internal.h"

static inline bool GB_is_directional(const struct GB_Core* gb) {
    return !!(IO_JYP & 0x10);
}

static inline bool GB_is_button(const struct GB_Core* gb) {
    return !!(IO_JYP & 0x20);
}

static inline uint8_t GB_joypad_get_internal(const struct GB_Core* gb) {
    return ((gb->joypad.var >> (GB_is_button(gb) << 2)) & 0xF);
}

void GB_set_buttons(struct GB_Core* gb, uint8_t buttons, bool is_down) {
    gb->joypad.var = !is_down ? gb->joypad.var | buttons : gb->joypad.var & (~buttons);
}

uint8_t GB_get_buttons(const struct GB_Core* gb) {
    return gb->joypad.var;
}

bool GB_is_button_down(const struct GB_Core* gb, enum GB_Button button) {
    return (gb->joypad.var & button) > 0;
}

uint8_t GB_joypad_get(const struct GB_Core* gb) {
    return (0xC0 | (IO_JYP & 0x30) | ((gb->joypad.var >> (GB_is_button(gb) << 2)) & 0xF));
}
