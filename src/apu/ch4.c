#include "../internal.h"
#include "apu.h"


// used for LFSR shifter
enum NoiseChannelShiftWidth
{
    WIDTH_15_BITS = 0,
    WIDTH_7_BITS = 1,
};

uint32_t get_ch4_freq(const struct GB_Core* gb)
{
    // indexed using the noise divisor code
    static const uint8_t NOISE_DIVISOR[8] = { 8, 16, 32, 48, 64, 80, 96, 112 };

    return NOISE_DIVISOR[IO_NR43.divisor_code] << IO_NR43.clock_shift;
}

bool is_ch4_dac_enabled(const struct GB_Core* gb)
{
    return IO_NR42.starting_vol > 0 || IO_NR42.env_add_mode > 0;
}

bool is_ch4_enabled(const struct GB_Core* gb)
{
    return (IO_NR52 & 0x08) > 0;
}

void ch4_enable(struct GB_Core* gb)
{
    IO_NR52 |= 0x08;
}

void ch4_disable(struct GB_Core* gb)
{
    IO_NR52 &= ~0x08;
}

int8_t sample_ch4(struct GB_Core* gb)
{
    // docs say that it's bit-0 INVERTED
    const bool bit = !(CH4.lfsr & 0x1);

    return (bit ? +CH4.volume : -CH4.volume) * CH4.master;
}

void clock_ch4_len(struct GB_Core* gb)
{
    if (IO_NR44.length_enable && CH4.length_counter > 0)
    {
        --CH4.length_counter;
        // disable channel if we hit zero...
        if (CH4.length_counter == 0)
        {
            ch4_disable(gb);
        }
    }
}

void clock_ch4_vol(struct GB_Core* gb)
{
    if (CH4.disable_env == false)
    {
        --CH4.volume_timer;

        if (CH4.volume_timer <= 0)
        {
            CH4.volume_timer = PERIOD_TABLE[IO_NR42.period];

            if (IO_NR42.period != 0)
            {
                uint8_t new_vol = CH4.volume;
                if (IO_NR42.env_add_mode == ADD)
                {
                    ++new_vol;
                }
                else
                {
                    --new_vol;
                }

                if (new_vol <= 15)
                {
                    CH4.volume = new_vol;
                }
                else
                {
                    CH4.disable_env = true;
                }
            }
        }
    }
}

void step_ch4_lfsr(struct GB_Core* gb)
{
    const uint8_t bit0 = CH4.lfsr & 0x1;
    const uint8_t bit1 = (CH4.lfsr >> 1) & 0x1;
    const uint8_t result = bit1 ^ bit0;

    // now we shift the lfsr BEFORE setting the value!
    CH4.lfsr >>= 1;

    // now set bit 15
    CH4.lfsr |= (result << 14);

    // set bit-6 if the width is half-mode
    if (IO_NR43.width_mode == WIDTH_7_BITS)
    {
        // unset it first!
        CH4.lfsr &= ~(1 << 6);
        CH4.lfsr |= (result << 6);
    }
}

void on_ch4_trigger(struct GB_Core* gb)
{
    ch4_enable(gb);

    if (CH4.length_counter == 0)
    {
        if (IO_NR44.length_enable && is_next_frame_sequencer_step_not_len(gb))
        {
            CH4.length_counter = 63;
        }
        else
        {
            CH4.length_counter = 64;
        }
    }

    CH4.disable_env = false;

    CH4.volume_timer = PERIOD_TABLE[IO_NR42.period];
    if (is_next_frame_sequencer_step_vol(gb))
    {
        CH4.volume_timer++;
    }

    // reload the volume
    CH4.volume = IO_NR42.starting_vol;
    // set all bits of the lfsr to 1
    CH4.lfsr = 0x7FFF;

    CH4.timer = get_ch4_freq(gb);

    if (is_ch4_dac_enabled(gb) == false)
    {
        ch4_disable(gb);
    }
}
