#include "../internal.h"
#include "apu.h"



uint16_t get_wave_freq(const struct GB_Core* gb)
{
    return (2048 - ((IO_NR34.freq_msb << 8) | IO_NR33.freq_lsb)) << 1;
}

bool is_wave_dac_enabled(const struct GB_Core* gb)
{
    return IO_NR30.dac_power > 0;
}

bool is_wave_enabled(const struct GB_Core* gb)
{
    return IO_NR52.ch3_on;
}

void wave_enable(struct GB_Core* gb)
{
    IO_NR52.ch3_on = true;
}

void wave_disable(struct GB_Core* gb)
{
    IO_NR52.ch3_on = false;
}

int8_t sample_wave(struct GB_Core* gb)
{
    static const int8_t test_table[16] =
    {
        -7, -6, -5, -4, -3, -2, -1,
        -0, +0,
        +1, +2, +3, +4, +5, +6, +7
    };

    // indexed using the wave shift.
    // the values in this table are used to right-shift the sample from wave_table
    static const uint8_t WAVE_SHIFT[4] = { 4, 0, 1, 2 };

    // this now uses the sample buffer, rather than indexing the array
    // which is *correct*, but still sounds very bad
    uint8_t sample = (WAVE_CHANNEL.position_counter & 1) ? WAVE_CHANNEL.sample_buffer & 0xF : WAVE_CHANNEL.sample_buffer >> 4;

    return test_table[(sample >> WAVE_SHIFT[IO_NR32.vol_code])] * 2;
}

void clock_wave_len(struct GB_Core* gb)
{
    if (IO_NR34.length_enable && WAVE_CHANNEL.length_counter > 0)
    {
        --WAVE_CHANNEL.length_counter;
        // disable channel if we hit zero...
        if (WAVE_CHANNEL.length_counter == 0)
        {
            wave_disable(gb);
        }
    }
}

void advance_wave_position_counter(struct GB_Core* gb)
{
    ++WAVE_CHANNEL.position_counter;
    // check if we need to wrap around
    if (WAVE_CHANNEL.position_counter >= 32)
    {
        WAVE_CHANNEL.position_counter = 0;
    }

    WAVE_CHANNEL.sample_buffer = IO_WAVE_TABLE[WAVE_CHANNEL.position_counter >> 1];
}

void on_wave_trigger(struct GB_Core* gb)
{
    wave_enable(gb);

    if (WAVE_CHANNEL.length_counter == 0)
    {
        // TODO: this fails blarggs audio test2, so diabling for now...
        // if (IO_NR34.length_enable && is_next_frame_sequencer_step_not_len(gb))
        // {
        //     WAVE_CHANNEL.length_counter = 255;
        // }
        // else
        {
            WAVE_CHANNEL.length_counter = 256;
        }
    }

    // reset position counter
    WAVE_CHANNEL.position_counter = 0;
    // wave sample is NOT reloaded
    // SOURCE: https://gbdev.gg8.se/wiki/articles/Gameboy_sound_hardware#Obscure_Behavior

    WAVE_CHANNEL.timer = get_wave_freq(gb);

    if (is_wave_dac_enabled(gb) == false)
    {
        wave_disable(gb);
    }
}

#ifdef __cplusplus
}
#endif
