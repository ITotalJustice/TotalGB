#include "../internal.h"
#include "../gb.h"
#include "apu.h"

#include <assert.h>


void GB_apu_iowrite(struct GB_Core* gb, uint16_t addr, uint8_t value)
{
    addr &= 0x7F;

    // on // off reg is always writable
    if (addr == 0x26)
    {
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
    else if (UNLIKELY(gb_is_apu_enabled(gb) == false))
    {
        return;
    }
    else
    {
        IO[addr] = value;

        switch (addr)
        {
            case 0x10:
                IO_NR10.sweep_period = (value >> 4) & 0x7;
                IO_NR10.sweep_negate = (value >> 3) & 0x1;
                IO_NR10.sweep_shift = value & 0x7;
                break;

            case 0x11:
                IO_NR11.duty = value >> 6;
                IO_NR11.length_load = value & 0x3F;
                CH1.length_counter = 64 - IO_NR11.length_load;
                break;

            case 0x12:
                IO_NR12.starting_vol = value >> 4;
                IO_NR12.env_add_mode = (value >> 3) & 0x1;
                IO_NR12.period = value & 0x7;
                if (is_ch1_dac_enabled(gb) == false)
                {
                    ch1_disable(gb);
                }
                break;

            case 0x13:
                IO_NR13.freq_lsb = value;
                break;

            case 0x14:
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
                break;

            case 0x16:
                IO_NR21.duty = value >> 6;
                IO_NR21.length_load = value & 0x3F;
                CH2.length_counter = 64 - IO_NR21.length_load;
                break;

            case 0x17:
                IO_NR22.starting_vol = value >> 4;
                IO_NR22.env_add_mode = (value >> 3) & 0x1;
                IO_NR22.period = value & 0x7;
                if (is_ch2_dac_enabled(gb) == false)
                {
                    ch2_disable(gb);
                }
                break;

            case 0x18:
                IO_NR23.freq_lsb = value;
                break;

            case 0x19:
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
                break;

            case 0x1A:
                IO_NR30.dac_power = value >> 7;
                if (is_ch3_dac_enabled(gb) == false)
                {
                    ch3_disable(gb);
                }
                break;

            case 0x1B:
                IO_NR31.length_load = value;
                CH3.length_counter = 256 - IO_NR31.length_load;
                break;

            case 0x1C:
                IO_NR32.vol_code = (value >> 5) & 0x3;
                break;

            case 0x1D:
                IO_NR33.freq_lsb = value;
                break;

            case 0x1E:
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
                break;

            case 0x20:
                IO_NR41.length_load = value & 0x3F;
                CH4.length_counter = 64 - IO_NR41.length_load;
                break;

            case 0x21:
                IO_NR42.starting_vol = value >> 4;
                IO_NR42.env_add_mode = (value >> 3) & 0x1;
                IO_NR42.period = value & 0x7;
                if (is_ch4_dac_enabled(gb) == false)
                {
                    ch4_disable(gb);
                }
                break;

            case 0x22:
                IO_NR43.clock_shift = value >> 4;
                IO_NR43.width_mode = (value >> 3) & 0x1;
                IO_NR43.divisor_code = value & 0x7;
                break;

            case 0x23:
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
                break;

            case 0x24:
                // handled above
                break;

            case 0x25:
                // handled above
                break;

            case 0x26:
                // this is handled above in the if, else
                assert(0 && "somehow writing to nr52 in switch!");
                break;

            case 0x30: case 0x31: case 0x32: case 0x33:
            case 0x34: case 0x35: case 0x36: case 0x37:
            case 0x38: case 0x39: case 0x3A: case 0x3B:
            case 0x3C: case 0x3D: case 0x3E: case 0x3F:
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
                break;
        }
    }
}
