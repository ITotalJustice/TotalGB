#include "../internal.h"
#include "apu.h"



uint16_t get_ch2_freq(const struct GB_Core* gb)
{
    return (2048 - ((IO_NR24.freq_msb << 8) | IO_NR23.freq_lsb)) << 2;
}

bool is_ch2_dac_enabled(const struct GB_Core* gb)
{
    return IO_NR22.starting_vol > 0 || IO_NR22.env_add_mode > 0;
}

bool is_ch2_enabled(const struct GB_Core* gb)
{
    return IO_NR52.ch2_on;
}

void ch2_enable(struct GB_Core* gb)
{
    IO_NR52.ch2_on = true;
}

void ch2_disable(struct GB_Core* gb)
{
    IO_NR52.ch2_on = false;
}

int8_t sample_ch2(struct GB_Core* gb)
{
    const int8_t duty = SQUARE_DUTY_CYCLES[IO_NR21.duty][CH2.duty_index];
    
    return CH2.volume * duty;
}

void clock_ch2_len(struct GB_Core* gb)
{
    if (IO_NR24.length_enable && CH2.length_counter > 0)
    {
        --CH2.length_counter;
        // disable channel if we hit zero...
        if (CH2.length_counter == 0)
        {
            ch2_disable(gb);
        }
    }
}

void clock_ch2_vol(struct GB_Core* gb)
{
    if (CH2.disable_env == false)
    {
        --CH2.volume_timer;

        if (CH2.volume_timer <= 0)
        {
            CH2.volume_timer = PERIOD_TABLE[IO_NR22.period];

            if (IO_NR22.period != 0)
            {
                uint8_t new_vol = CH2.volume;
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
                    CH2.volume = new_vol;
                }
                else
                {
                    CH2.disable_env = true;
                }
            }
        }
    }
}

void on_ch2_trigger(struct GB_Core* gb)
{
    ch2_enable(gb);

    if (CH2.length_counter == 0)
    {
        if (IO_NR24.length_enable && is_next_frame_sequencer_step_not_len(gb))
        {
            CH2.length_counter = 63;
        }
        else
        {
            CH2.length_counter = 64;
        }
    }

    CH2.disable_env = false;

    CH2.volume_timer = PERIOD_TABLE[IO_NR22.period];
    if (is_next_frame_sequencer_step_vol(gb))
    {
        CH2.volume_timer++;
    }

    // reload the volume
    CH2.volume = IO_NR22.starting_vol;

    // when a square channel is triggered, it's lower 2-bits are not modified!
    // SOURCE: https://gbdev.gg8.se/wiki/articles/Gameboy_sound_hardware#Obscure_Behavior
    CH2.timer = (CH2.timer & 0x3) | (get_ch2_freq(gb) & ~(0x3));

    if (is_ch2_dac_enabled(gb) == false)
    {
        ch2_disable(gb);
    }
}