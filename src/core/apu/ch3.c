#include "../internal.h"
#include "apu.h"


uint16_t get_ch3_freq(const struct GB_Core* gb)
{
    return (2048 - ((IO_NR34.freq_msb << 8) | IO_NR33.freq_lsb)) << 1;
}

bool is_ch3_dac_enabled(const struct GB_Core* gb)
{
    return IO_NR30.dac_power;
}

bool is_ch3_enabled(const struct GB_Core* gb)
{
    return (IO_NR52 & 0x04) > 0;
}

void ch3_enable(struct GB_Core* gb)
{
    if ((IO_NR52 & 0x04) == 0x00)
    {
        GB_log("[APU] ch3 enable\n");
    }
    IO_NR52 |= 0x04;
}

void ch3_disable(struct GB_Core* gb)
{
    if ((IO_NR52 & 0x04) == 0x04)
    {
        GB_log("[APU] ch3 disable\n");
    }
    IO_NR52 &= ~0x04;
}

uint8_t sample_ch3(struct GB_Core* gb)
{
    // indexed using the ch3 shift.
    // the values in this table are used to right-shift the sample
    static const uint8_t CH3_SHIFT[4] = { 4, 0, 1, 2 };

    // check if we need the access the low of high nibble
    const uint8_t sample = (CH3.position_counter & 1) ? CH3.sample_buffer & 0xF : CH3.sample_buffer >> 4;

    return (sample >> CH3_SHIFT[IO_NR32.vol_code]) * is_ch3_enabled(gb);
}

void clock_ch3_len(struct GB_Core* gb)
{
    if (IO_NR34.length_enable && CH3.length_counter > 0)
    {
        --CH3.length_counter;
        // disable channel if we hit zero...
        if (CH3.length_counter == 0)
        {
            ch3_disable(gb);
        }
    }
}

void advance_ch3_position_counter(struct GB_Core* gb)
{
    CH3.position_counter = (CH3.position_counter + 1) % 32;
    CH3.sample_buffer = IO_WAVE_TABLE[CH3.position_counter >> 1];
}

void on_ch3_trigger(struct GB_Core* gb)
{
    ch3_enable(gb);

    if (CH3.length_counter == 0)
    {
        CH3.length_counter = 256;

        if (IO_NR34.length_enable && is_next_frame_sequencer_step_not_len(gb))
        {
            CH3.length_counter--;
        }
    }

    // reset position counter
    CH3.position_counter = 0;
    // ch3 sample is NOT reloaded
    // SOURCE: https://gbdev.gg8.se/wiki/articles/Gameboy_sound_hardware#Obscure_Behavior

    // SOURCE: https://forums.nesdev.org/viewtopic.php?t=13730
    // SOURCE: https://github.com/kode54/Game_Music_Emu/blob/master/gme/Gb_Oscs.cpp#L303
    // binji mentions here that blargss source (for some reason) adds
    // 6 extra cycles when triggering the wave channel.
    // this allows me to pass audio test 09.
    CH3.timer = get_ch3_freq(gb) + 6;

    if (is_ch3_dac_enabled(gb) == false)
    {
        ch3_disable(gb);
    }
}
