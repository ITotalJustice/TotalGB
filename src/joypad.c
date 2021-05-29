#include "gb.h"
#include "internal.h"


static inline bool GB_is_button(const struct GB_Core* gb)
{
    return !!(IO_JYP & 0x20);
}

// [API]
void GB_set_buttons(struct GB_Core* gb, uint8_t buttons, bool is_down)
{
    // the pins go LO when pressed!
    if (is_down)
    {
        gb->joypad.var &= ~buttons;
    }
    else
    {
        gb->joypad.var |= buttons;
    }

    // the direction keys were made so that the opposite key could not
    // be pressed at the same time, ie, left and right cannot both be pressed.
    // allowing this probably doesnt break any games, but does cause strange
    // effects in some games, such as zelda, pressing up and down will cause
    // link to walk in-place whilst holding a shield, even if link doesn't
    // yet have the shield...

    // this can be better optimised at some point
    if (is_down && (buttons & GB_BUTTON_DIRECTIONAL))
    {
        if (buttons & GB_BUTTON_RIGHT)
        {
            gb->joypad.var |= GB_BUTTON_LEFT;
        }
        if (buttons & GB_BUTTON_LEFT)
        {
            gb->joypad.var |= GB_BUTTON_RIGHT;
        }
        if (buttons & GB_BUTTON_UP)
        {
            gb->joypad.var |= GB_BUTTON_DOWN;
        }
        if (buttons & GB_BUTTON_DOWN)
        {
            gb->joypad.var |= GB_BUTTON_UP;
        }
    }
}

uint8_t GB_get_buttons(const struct GB_Core* gb)
{
    return gb->joypad.var;
}

bool GB_is_button_down(const struct GB_Core* gb, enum GB_Button button)
{
    return (gb->joypad.var & button) > 0;
}

// [core]
uint8_t GB_joypad_read(const struct GB_Core* gb)
{
    if (GB_get_system_type(gb) == GB_SYSTEM_TYPE_SGB)
    {
        uint8_t data = 0;
        // check if we handled it
        if (SGB_handle_joyp_read(gb, &data))
        {
            return data;
        }
    }
    return (0xC0 | (IO_JYP & 0x30) | ((gb->joypad.var >> (GB_is_button(gb) << 2)) & 0xF));
}

void GB_joypad_write(struct GB_Core* gb, uint8_t value)
{
    if (GB_get_system_type(gb) == GB_SYSTEM_TYPE_SGB)
    {
        SGB_handle_joyp_write(gb, value);
    }

    IO_JYP &= ~(0x30);
    IO_JYP |= 0x30 & value;
}
