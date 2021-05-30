#include "../internal.h"
#include "apu.h"



uint16_t get_square2_freq(const struct GB_Core* gb)
{
    return (2048 - ((IO_NR24.freq_msb << 8) | IO_NR23.freq_lsb)) << 2;
}

bool is_square2_dac_enabled(const struct GB_Core* gb)
{
    return IO_NR22.starting_vol > 0 || IO_NR22.env_add_mode > 0;
}

bool is_square2_enabled(const struct GB_Core* gb)
{
    return IO_NR52.ch2_on;
}

void square2_enable(struct GB_Core* gb)
{
    IO_NR52.ch2_on = true;
}

void square2_disable(struct GB_Core* gb)
{
    IO_NR52.ch2_on = false;
}

int8_t sample_square2(struct GB_Core* gb)
{
    if (SQUARE_DUTY_CYCLES[IO_NR21.duty][SQUARE2_CHANNEL.duty_index])
    {
        return SQUARE2_CHANNEL.volume;
    }
    return -SQUARE2_CHANNEL.volume;
}

void clock_square2_len(struct GB_Core* gb)
{
    if (IO_NR24.length_enable && SQUARE2_CHANNEL.length_counter > 0)
    {
        --SQUARE2_CHANNEL.length_counter;
        // disable channel if we hit zero...
        if (SQUARE2_CHANNEL.length_counter == 0)
        {
            square2_disable(gb);
        }
    }
}

void clock_square2_vol(struct GB_Core* gb)
{
    if (SQUARE2_CHANNEL.disable_env == false)
    {
        --SQUARE2_CHANNEL.volume_timer;

        if (SQUARE2_CHANNEL.volume_timer <= 0)
        {
            SQUARE2_CHANNEL.volume_timer = PERIOD_TABLE[IO_NR22.period];

            if (IO_NR22.period != 0)
            {
                uint8_t new_vol = SQUARE2_CHANNEL.volume;
                if (IO_NR22.env_add_mode == ADD)
                {
                    ++new_vol;
                }
                else
                {
                    --new_vol;
                }

                if (new_vol <= 15)
                {
                    SQUARE2_CHANNEL.volume = new_vol;
                }
                else
                {
                    SQUARE2_CHANNEL.disable_env = true;
                }
            }
        }
    }
}

void on_square2_trigger(struct GB_Core* gb)
{
    square2_enable(gb);

    if (SQUARE2_CHANNEL.length_counter == 0)
    {
        if (IO_NR24.length_enable && is_next_frame_sequencer_step_not_len(gb))
        {
            SQUARE2_CHANNEL.length_counter = 63;
        }
        else
        {
            SQUARE2_CHANNEL.length_counter = 64;
        }
    }

    SQUARE2_CHANNEL.disable_env = false;

    SQUARE2_CHANNEL.volume_timer = PERIOD_TABLE[IO_NR22.period];
    if (is_next_frame_sequencer_step_vol(gb))
    {
        SQUARE2_CHANNEL.volume_timer++;
    }

    // reload the volume
    SQUARE2_CHANNEL.volume = IO_NR22.starting_vol;

    // when a square channel is triggered, it's lower 2-bits are not modified!
    // SOURCE: https://gbdev.gg8.se/wiki/articles/Gameboy_sound_hardware#Obscure_Behavior
    SQUARE2_CHANNEL.timer = (SQUARE2_CHANNEL.timer & 0x3) | (get_square2_freq(gb) & ~(0x3));

    if (is_square2_dac_enabled(gb) == false)
    {
        square2_disable(gb);
    }
}
