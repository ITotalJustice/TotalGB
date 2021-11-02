#include "gb.h"
#include "internal.h"


static inline bool GB_is_directional(const struct GB_Core* gb)
{
    return !(IO_JYP & 0x10);
}

static inline bool GB_is_button(const struct GB_Core* gb)
{
    return !(IO_JYP & 0x20);
}

// [API]
void GB_set_buttons(struct GB_Core* gb, uint8_t buttons, bool is_down)
{
    // the pins go LOW when pressed!
    if (is_down)
    {
        gb->joypad.var &= ~buttons;

        // this isn't correct impl, it needs to fire when going hi to lo.
        // but it's good enough for now.
        GB_enable_interrupt(gb, GB_INTERRUPT_JOYPAD);

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
    return (gb->joypad.var & button) == 0;
}

void GB_joypad_write(struct GB_Core* gb, uint8_t value)
{
    #if SGB_ENABLE
        if (GB_get_system_type(gb) == GB_SYSTEM_TYPE_SGB)
        {
            SGB_handle_joyp_write(gb, value);
        }
    #endif // #if SGB_ENABLE

    // unset p14 and p15
    IO_JYP &= ~0x30;
    // only p14 and p15 are writable, also OR in unused bits
    IO_JYP |= 0xCF | (0x30 & value);
    
    // CREDIT: thanks to Calindro for the below code.
    // turns out that both p14 and p15 can be low (selected),
    // in which case, reading pulls from both button and directional
    // lines.

    // for example, if A is low and RIGHT is high, reading will be low.
    // this was noticed in [bomberman GB] where both p14 and p15 were low.
    // SEE: https://github.com/ITotalJustice/TotalGB/issues/41

    if ((GB_is_button(gb)))
    {
        if (GB_is_button_down(gb, GB_BUTTON_A))
        {
            IO_JYP &= ~0x01;
        }
        if (GB_is_button_down(gb, GB_BUTTON_B))
        {
            IO_JYP &= ~0x02;
        }
        if (GB_is_button_down(gb, GB_BUTTON_SELECT))
        {
            IO_JYP &= ~0x04;
        }
        if (GB_is_button_down(gb, GB_BUTTON_START))
        {
            IO_JYP &= ~0x08;
        }
    }
    
    if ((GB_is_directional(gb)))
    {
        if (GB_is_button_down(gb, GB_BUTTON_RIGHT))
        {
            IO_JYP &= ~0x01;
        }
        if (GB_is_button_down(gb, GB_BUTTON_LEFT))
        {
            IO_JYP &= ~0x02;
        }
        if (GB_is_button_down(gb, GB_BUTTON_UP))
        {
            IO_JYP &= ~0x04;
        }
        if (GB_is_button_down(gb, GB_BUTTON_DOWN))
        {
            IO_JYP &= ~0x08;
        }
    }
}
