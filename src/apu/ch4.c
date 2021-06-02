#include "../internal.h"
#include "apu.h"


// used for LFSR shifter
enum NoiseChannelShiftWidth
{
    WIDTH_15_BITS = 0,
    WIDTH_7_BITS = 1,
};

struct Sweep
{
    uint16_t freq_shadow_register;
    uint8_t internal_enable_flag;
    uint8_t shift;
    bool negate;
};

struct Timer
{
    uint32_t freq;
    int32_t timer;
    uint8_t duty;
};

struct LengthCounter
{
    uint8_t length_load;
    uint8_t counter;
    bool enable;
};

struct Envelope
{
    uint8_t period;
    uint8_t volume;
    uint8_t starting_volume;
    bool add_mode;
    bool disable;
};


#define CH1_ gb->apu.ch1
#define CH2_ gb->apu.ch2
#define CH3_ gb->apu.ch3
#define CH4_ gb->apu.ch4

#define CH(num) CH##num##_
#define IS_CHANNEL_ON(num) (CONTROL_CHANNEL.ch##num##_on)

#define ENABLE_CHANNEL(num) CONTROL_CHANNEL.ch##num##_on = true
#define DISABLE_CHANNEL(num) CONTROL_CHANNEL.ch##num##_on = false

#define CLOCK_CHANNEL_LEN(num) do { \
    if (CH(num).length_enable && CH(num).length_counter > 0) \
    { \
        --CH(num).length_counter; \
        /* disable channel if we hit zero... */ \
        if (CH(num).length_counter == 0) \
        { \
            DISABLE_CHANNEL(num); \
        } \
    } \
} while(0)


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
    return IO_NR52.ch4_on;
}

void ch4_enable(struct GB_Core* gb)
{
    IO_NR52.ch4_on = true;
}

void ch4_disable(struct GB_Core* gb)
{
    IO_NR52.ch4_on = false;
}

int8_t sample_ch4(struct GB_Core* gb)
{
    // docs say that it's bit-0 INVERTED
    const bool bit = !(CH4.lfsr & 0x1);
    if (bit == 1)
    {
        return CH4.volume;
    }

    return -CH4.volume;
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