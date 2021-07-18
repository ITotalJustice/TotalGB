#include "../internal.h"
#include "../gb.h"
#include "apu.h"

#include <assert.h>


static inline void on_wave_mem_write(struct GB_Core* gb, uint8_t addr, uint8_t value);

static inline void on_nr10_write(struct GB_Core* gb, uint8_t value);
static inline void on_nr11_write(struct GB_Core* gb, uint8_t value);
static inline void on_nr12_write(struct GB_Core* gb, uint8_t value);
static inline void on_nr13_write(struct GB_Core* gb, uint8_t value);
static inline void on_nr14_write(struct GB_Core* gb, uint8_t value);

static inline void on_nr21_write(struct GB_Core* gb, uint8_t value);
static inline void on_nr22_write(struct GB_Core* gb, uint8_t value);
static inline void on_nr23_write(struct GB_Core* gb, uint8_t value);
static inline void on_nr24_write(struct GB_Core* gb, uint8_t value);

static inline void on_nr30_write(struct GB_Core* gb, uint8_t value);
static inline void on_nr31_write(struct GB_Core* gb, uint8_t value);
static inline void on_nr32_write(struct GB_Core* gb, uint8_t value);
static inline void on_nr33_write(struct GB_Core* gb, uint8_t value);
static inline void on_nr34_write(struct GB_Core* gb, uint8_t value);

static inline void on_nr41_write(struct GB_Core* gb, uint8_t value);
static inline void on_nr42_write(struct GB_Core* gb, uint8_t value);
static inline void on_nr43_write(struct GB_Core* gb, uint8_t value);
static inline void on_nr44_write(struct GB_Core* gb, uint8_t value);

// static inline void on_nr50_write(struct GB_Core* gb, uint8_t value);
// static inline void on_nr51_write(struct GB_Core* gb, uint8_t value);
static inline void on_nr52_write(struct GB_Core* gb, uint8_t value);


void GB_apu_iowrite(struct GB_Core* gb, uint16_t addr, uint8_t value)
{
    addr &= 0x7F;

    // on // off reg is always writable
    if (addr == 0x26)
    {
        on_nr52_write(gb, value);
    }
    else if (UNLIKELY(gb_is_apu_enabled(gb) == false))
    {
        // on the dmg, len registers are still writable
        if (GB_get_system_type(gb) & GB_SYSTEM_TYPE_DMG)
        {
            switch (addr)
            {
                case 0x11: IO[addr] = value; on_nr11_write(gb, value); break;
                case 0x16: IO[addr] = value; on_nr21_write(gb, value); break;
                case 0x1B: IO[addr] = value; on_nr31_write(gb, value); break;
                case 0x20: IO[addr] = value; on_nr41_write(gb, value); break;
            }
        }

        if (addr >= 0x30 && addr <= 0x3F)
        {
            IO[addr] = value;
            on_wave_mem_write(gb, addr, value); 
        }
    }
    else
    {
        IO[addr] = value;

        switch (addr)
        {
            case 0x10: on_nr10_write(gb, value); break;
            case 0x11: on_nr11_write(gb, value); break;
            case 0x12: on_nr12_write(gb, value); break;
            case 0x13: on_nr13_write(gb, value); break;
            case 0x14: on_nr14_write(gb, value); break;
            
            case 0x16: on_nr21_write(gb, value); break;
            case 0x17: on_nr22_write(gb, value); break;
            case 0x18: on_nr23_write(gb, value); break;
            case 0x19: on_nr24_write(gb, value); break;

            case 0x1A: on_nr30_write(gb, value); break;
            case 0x1B: on_nr31_write(gb, value); break;
            case 0x1C: on_nr32_write(gb, value); break;
            case 0x1D: on_nr33_write(gb, value); break;
            case 0x1E: on_nr34_write(gb, value); break;

            case 0x20: on_nr41_write(gb, value); break;
            case 0x21: on_nr42_write(gb, value); break;
            case 0x22: on_nr43_write(gb, value); break;
            case 0x23: on_nr44_write(gb, value); break;

            // nr5X are already handled
            case 0x24: case 0x25: case 0x26: break;

            case 0x30: case 0x31: case 0x32: case 0x33:
            case 0x34: case 0x35: case 0x36: case 0x37:
            case 0x38: case 0x39: case 0x3A: case 0x3B:
            case 0x3C: case 0x3D: case 0x3E: case 0x3F:
                on_wave_mem_write(gb, addr, value);
                break;
        }
    }
}

void on_nr10_write(struct GB_Core* gb, uint8_t value)
{
    IO_NR10.sweep_period = (value >> 4) & 0x7;
    IO_NR10.sweep_negate = (value >> 3) & 0x1;
    IO_NR10.sweep_shift = value & 0x7;
}

void on_nr11_write(struct GB_Core* gb, uint8_t value)
{
    // writes to NRx1 on DMG are allowed even if the apu is disabled.
    // however, near mentiones that only the length is changed!
    // SOURCE: https://forums.nesdev.com/viewtopic.php?t=13730
    if (gb_is_apu_enabled(gb))
    {
        IO_NR11.duty = value >> 6;
    }
    CH1.length_counter = 64 - (value & 0x3F);
}

void on_nr12_write(struct GB_Core* gb, uint8_t value)
{
    const uint8_t starting_vol = value >> 4;
    const bool env_add_mode = (value >> 3) & 0x1;
    const uint8_t period = value & 0x7;

    if (is_ch1_enabled(gb))
    {
        if (IO_NR12.period == 0 && CH1.disable_env == false)
        {
            if (IO_NR12.env_add_mode)
            {
                CH1.volume += 1;
            }
            else
            {
                CH1.volume += 2;
            }
        }

        if (IO_NR12.env_add_mode != env_add_mode)
        {
            CH1.volume = 16 - CH1.volume;
        }

        CH1.volume &= 0xF;
    }

    if (starting_vol == 0)
    {
        CH1.master = false;
    }
    else
    {
        CH1.master = true;
    }

    IO_NR12.starting_vol = starting_vol;
    IO_NR12.env_add_mode = env_add_mode;
    IO_NR12.period = period;

    if (is_ch1_dac_enabled(gb) == false)
    {
        ch1_disable(gb);
    }
}

void on_nr13_write(struct GB_Core* gb, uint8_t value)
{
    IO_NR13.freq_lsb = value;
}

void on_nr14_write(struct GB_Core* gb, uint8_t value)
{
    // if next is not len and len is NOW enabled, it is clocked
    if (is_next_frame_sequencer_step_not_len(gb) && CH1.length_counter && !IO_NR14.length_enable && (value >> 6) & 0x1)
    {
        --CH1.length_counter;

        GB_log("APU: edge case: extra len clock!\n");
        // if this makes the result 0, and trigger is clear, disbale channel
        if (!CH1.length_counter && !(value & 0x80))
        {
            ch1_disable(gb);
        }
    }

    IO_NR14.length_enable = (value >> 6) & 0x1;
    IO_NR14.freq_msb = value & 0x7;

    if (value & 0x80)
    {
        on_ch1_trigger(gb);
    }
}


void on_nr21_write(struct GB_Core* gb, uint8_t value)
{
    if (gb_is_apu_enabled(gb))
    {
        IO_NR21.duty = value >> 6;
    }
    CH2.length_counter = 64 - (value & 0x3F);
}

void on_nr22_write(struct GB_Core* gb, uint8_t value)
{
    const uint8_t starting_vol = value >> 4;
    const bool env_add_mode = (value >> 3) & 0x1;
    const uint8_t period = value & 0x7;

    if (is_ch2_enabled(gb))
    {
        if (IO_NR22.period == 0 && CH2.disable_env == false)
        {
            if (IO_NR22.env_add_mode)
            {
                CH2.volume += 1;
            }
            else
            {
                CH2.volume += 2;
            }
        }

        if (IO_NR22.env_add_mode != env_add_mode)
        {
            CH2.volume = 16 - CH2.volume;
        }

        CH2.volume &= 0xF;
    }

    if (starting_vol == 0)
    {
        CH2.master = false;
    }
    else
    {
        CH2.master = true;
    }

    IO_NR22.starting_vol = starting_vol;
    IO_NR22.env_add_mode = env_add_mode;
    IO_NR22.period = period;

    if (is_ch2_dac_enabled(gb) == false)
    {
        ch2_disable(gb);
    }
}

void on_nr23_write(struct GB_Core* gb, uint8_t value)
{
    IO_NR23.freq_lsb = value;
}

void on_nr24_write(struct GB_Core* gb, uint8_t value)
{
    // if next is not len and len is NOW enabled, it is clocked
    if (is_next_frame_sequencer_step_not_len(gb) && CH2.length_counter && !IO_NR24.length_enable && (value >> 6) & 0x1)
    {
        --CH2.length_counter;

        GB_log("APU - edge case: extra len clock!\n");
        // if this makes the result 0, and trigger is clear, disbale channel
        if (!CH2.length_counter && !(value & 0x80))
        {
            ch2_disable(gb);
        }
    }

    IO_NR24.length_enable = (value >> 6) & 0x1;
    IO_NR24.freq_msb = value & 0x7;

    if (value & 0x80)
    {
        on_ch2_trigger(gb);
    }
}


void on_nr30_write(struct GB_Core* gb, uint8_t value)
{
    IO_NR30.dac_power = value >> 7;
    
    if (is_ch3_dac_enabled(gb) == false)
    {
        ch3_disable(gb);
    }
}

void on_nr31_write(struct GB_Core* gb, uint8_t value)
{
    CH3.length_counter = 256 - value;
}

void on_nr32_write(struct GB_Core* gb, uint8_t value)
{
    IO_NR32.vol_code = (value >> 5) & 0x3;
}

void on_nr33_write(struct GB_Core* gb, uint8_t value)
{
    IO_NR33.freq_lsb = value;
}

void on_nr34_write(struct GB_Core* gb, uint8_t value)
{
    // if next is not len and len is NOW enabled, it is clocked
    if (is_next_frame_sequencer_step_not_len(gb) && CH3.length_counter && !IO_NR34.length_enable && (value >> 6) & 0x1)
    {
        --CH3.length_counter;

        GB_log("APU: edge case: extra len clock!\n");
        // if this makes the result 0, and trigger is clear, disbale channel
        if (!CH3.length_counter && !(value & 0x80))
        {
            ch3_disable(gb);
        }
    }

    IO_NR34.length_enable = (value >> 6) & 0x1;
    IO_NR34.freq_msb = value & 0x7;

    if (value & 0x80)
    {
        on_ch3_trigger(gb);
    }
}


void on_nr41_write(struct GB_Core* gb, uint8_t value)
{
    CH4.length_counter = 64 - (value & 0x3F);
}

void on_nr42_write(struct GB_Core* gb, uint8_t value)
{
    const uint8_t starting_vol = value >> 4;
    const bool env_add_mode = (value >> 3) & 0x1;
    const uint8_t period = value & 0x7;

    if (is_ch4_enabled(gb))
    {
        if (IO_NR42.period == 0 && CH4.disable_env == false)
        {
            if (env_add_mode)
            {
                CH4.volume += 1;
            }
            else
            {
                CH4.volume += 2;
            }
        }

        if (IO_NR42.env_add_mode != env_add_mode)
        {
            CH4.volume = 16 - CH4.volume;
        }

        CH4.volume &= 0xF;
    }

    if (starting_vol == 0)
    {
        CH4.master = false;
    }
    else
    {
        CH4.master = true;
    }

    IO_NR42.starting_vol = starting_vol;
    IO_NR42.env_add_mode = env_add_mode;
    IO_NR42.period = period;

    if (is_ch4_dac_enabled(gb) == false)
    {
        ch4_disable(gb);
    }
}

void on_nr43_write(struct GB_Core* gb, uint8_t value)
{
    IO_NR43.clock_shift = value >> 4;
    IO_NR43.width_mode = (value >> 3) & 0x1;
    IO_NR43.divisor_code = value & 0x7;
}

void on_nr44_write(struct GB_Core* gb, uint8_t value)
{
    // if next is not len and len is NOW enabled, it is clocked
    if (is_next_frame_sequencer_step_not_len(gb) && CH4.length_counter && !IO_NR44.length_enable && (value >> 6) & 0x1)
    {
        --CH4.length_counter;

        GB_log("APU: edge case: extra len clock!\n");
        // if this makes the result 0, and trigger is clear, disbale channel
        if (!CH4.length_counter && !(value & 0x80))
        {
            ch4_disable(gb);
        }
    }

    IO_NR44.length_enable = (value >> 6) & 0x1;

    if (value & 0x80)
    {
        on_ch4_trigger(gb);
    }
}


void on_nr52_write(struct GB_Core* gb, uint8_t value)
{
    // on the dmg, the len is writeable!
    if (IO_NR52 & 0x80)
    {
        if ((value & 0x80) == 0)
        {
            gb_apu_on_disabled(gb);
        }
    }
    else
    {
        if (value & 0x80)
        {
            gb_apu_on_enabled(gb);
        }
    }

    // todo: should reset the apu reg values here i think, maybe even
    // trigger some registers???
    IO_NR52 &= ~0x80;
    IO_NR52 |= value & 0x80;
}


void on_wave_mem_write(struct GB_Core* gb, uint8_t addr, uint8_t value)
{
    // wave ram should only be accessed whilst the channel
    // is disabled.
    // otherwise, for the first few clocks after the last
    // time the wave channel accessed this wave ram, it will read
    // and write to the current index.
    // otherwise, writes are ignored and reads return 0xFF.

    // for now, all reads / writes that happen whilst the channel is
    // disabled only!

    if (!is_ch3_enabled(gb))
    {
        IO_WAVE_TABLE[addr & 0xF] = value;
    }
}
