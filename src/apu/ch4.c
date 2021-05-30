#include "../internal.h"
#include "apu.h"


// used for LFSR shifter
enum NoiseChannelShiftWidth
{
    WIDTH_15_BITS = 0,
    WIDTH_7_BITS = 1,
};


uint32_t get_noise_freq(const struct GB_Core* gb)
{
    // indexed using the noise divisor code
    static const uint8_t NOISE_DIVISOR[8] = { 8, 16, 32, 48, 64, 80, 96, 112 };

    return NOISE_DIVISOR[IO_NR43.divisor_code] << IO_NR43.clock_shift;
}

bool is_noise_dac_enabled(const struct GB_Core* gb)
{
    return IO_NR42.starting_vol > 0 || IO_NR42.env_add_mode > 0;
}

bool is_noise_enabled(const struct GB_Core* gb)
{
    return IO_NR52.ch4_on;
}

void noise_enable(struct GB_Core* gb)
{
    IO_NR52.ch4_on = true;
}

void noise_disable(struct GB_Core* gb)
{
    IO_NR52.ch4_on = false;
}

int8_t sample_noise(struct GB_Core* gb)
{
    // docs say that it's bit-0 INVERTED
    const bool bit = !(NOISE_CHANNEL.lfsr & 0x1);
    if (bit == 1)
    {
        return NOISE_CHANNEL.volume;
    }

    return -NOISE_CHANNEL.volume;
}

void clock_noise_len(struct GB_Core* gb)
{
    if (IO_NR44.length_enable && NOISE_CHANNEL.length_counter > 0)
    {
        --NOISE_CHANNEL.length_counter;
        // disable channel if we hit zero...
        if (NOISE_CHANNEL.length_counter == 0)
        {
            noise_disable(gb);
        }
    }
}

void clock_noise_vol(struct GB_Core* gb)
{
    if (NOISE_CHANNEL.disable_env == false)
    {
        --NOISE_CHANNEL.volume_timer;

        if (NOISE_CHANNEL.volume_timer <= 0)
        {
            NOISE_CHANNEL.volume_timer = PERIOD_TABLE[IO_NR42.period];

            if (IO_NR42.period != 0)
            {
                uint8_t new_vol = NOISE_CHANNEL.volume;
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
                    NOISE_CHANNEL.volume = new_vol;
                }
                else
                {
                    NOISE_CHANNEL.disable_env = true;
                }
            }
        }
    }
}

void step_noise_lfsr(struct GB_Core* gb)
{
    const uint8_t bit0 = NOISE_CHANNEL.lfsr & 0x1;
    const uint8_t bit1 = (NOISE_CHANNEL.lfsr >> 1) & 0x1;
    const uint8_t result = bit1 ^ bit0;

    // now we shift the lfsr BEFORE setting the value!
    NOISE_CHANNEL.lfsr >>= 1;

    // now set bit 15
    NOISE_CHANNEL.lfsr |= (result << 14);

    // set bit-6 if the width is half-mode
    if (IO_NR43.width_mode == WIDTH_7_BITS)
    {
        // unset it first!
        NOISE_CHANNEL.lfsr &= ~(1 << 6);
        NOISE_CHANNEL.lfsr |= (result << 6);
    }
}

void on_noise_trigger(struct GB_Core* gb)
{
    noise_enable(gb);

    if (NOISE_CHANNEL.length_counter == 0)
    {
        if (IO_NR44.length_enable && is_next_frame_sequencer_step_not_len(gb))
        {
            NOISE_CHANNEL.length_counter = 63;
        }
        else
        {
            NOISE_CHANNEL.length_counter = 64;
        }
    }

    NOISE_CHANNEL.disable_env = false;

    NOISE_CHANNEL.volume_timer = PERIOD_TABLE[IO_NR42.period];
    if (is_next_frame_sequencer_step_vol(gb))
    {
        NOISE_CHANNEL.volume_timer++;
    }

    // reload the volume
    NOISE_CHANNEL.volume = IO_NR42.starting_vol;
    // set all bits of the lfsr to 1
    NOISE_CHANNEL.lfsr = 0x7FFF;

    NOISE_CHANNEL.timer = get_noise_freq(gb);

    if (is_noise_dac_enabled(gb) == false)
    {
        noise_disable(gb);
    }
}
