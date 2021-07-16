#include "../internal.h"
#include "apu.h"


uint16_t get_ch1_freq(const struct GB_Core* gb)
{
    return (2048 - ((IO_NR14.freq_msb << 8) | IO_NR13.freq_lsb)) << 2;
}

bool is_ch1_dac_enabled(const struct GB_Core* gb)
{
    return IO_NR12.starting_vol > 0 || IO_NR12.env_add_mode > 0;
}

bool is_ch1_enabled(const struct GB_Core* gb)
{
    return (IO_NR52 & 0x01) > 0;
}

void ch1_enable(struct GB_Core* gb)
{
    IO_NR52 |= 0x01;
}

void ch1_disable(struct GB_Core* gb)
{
    IO_NR52 &= ~0x01;
}

int8_t sample_ch1(struct GB_Core* gb)
{
    const bool duty = SQUARE_DUTY_CYCLES[IO_NR11.duty][CH1.duty_index];
    return (duty ? +CH1.volume : -CH1.volume) * CH1.master;
}

void clock_ch1_len(struct GB_Core* gb)
{
    if (IO_NR14.length_enable && CH1.length_counter > 0)
    {
        --CH1.length_counter;
        // disable channel if we hit zero...
        if (CH1.length_counter == 0)
        {
            ch1_disable(gb);
        }
    }
}

void clock_ch1_vol(struct GB_Core* gb)
{
    if (CH1.disable_env == false)
    {
        --CH1.volume_timer;

        if (CH1.volume_timer <= 0)
        {
            CH1.volume_timer = PERIOD_TABLE[IO_NR12.period];

            if (IO_NR12.period != 0)
            {
                uint8_t new_vol = CH1.volume;
                if (IO_NR12.env_add_mode == ADD)
                {
                    ++new_vol;
                }
                else
                {
                    --new_vol;
                }

                if (new_vol <= 15)
                {
                    CH1.volume = new_vol;
                }
                else
                {
                    CH1.disable_env = true;
                }
            }
            else
            {
                CH1.disable_env = true;
            }
        }
    }
}

static uint16_t get_new_sweep_freq(const struct GB_Core* gb)
{
    const uint16_t new_freq = CH1.freq_shadow_register >> IO_NR10.sweep_shift;

    if (IO_NR10.sweep_negate)
    {
        return CH1.freq_shadow_register - new_freq;
    }
    else
    {
        return CH1.freq_shadow_register + new_freq;
    }
}

void do_freq_sweep_calc(struct GB_Core* gb)
{
    if (CH1.internal_enable_flag && IO_NR10.sweep_period)
    {
        const uint16_t new_freq = get_new_sweep_freq(gb);

        if (new_freq <= 2047)
        {
            CH1.freq_shadow_register = new_freq;
            IO_NR13.freq_lsb = new_freq & 0xFF;
            IO_NR14.freq_msb = new_freq >> 8;

            // for some reason, a second overflow check is performed...
            if (get_new_sweep_freq(gb) > 2047)
            {
                // overflow...
                ch1_disable(gb);
            }

        }
        else
        { // overflow...
            ch1_disable(gb);
        }
    }
}

void on_ch1_sweep(struct GB_Core* gb)
{
    // first check if sweep is enabled
    if (!CH1.internal_enable_flag || !is_ch1_enabled(gb))
    {
        return;
    }

    // decrement the counter, reload after
    --CH1.sweep_timer;

    if (CH1.sweep_timer <= 0)
    {
        // period is counted as 8 if 0...
        CH1.sweep_timer = PERIOD_TABLE[IO_NR10.sweep_period];
        do_freq_sweep_calc(gb);
    }
}

void on_ch1_trigger(struct GB_Core* gb)
{
    ch1_enable(gb);

    if (CH1.length_counter == 0)
    {
        if (IO_NR14.length_enable && is_next_frame_sequencer_step_not_len(gb))
        {
            CH1.length_counter = 63;
        }
        else
        {
            CH1.length_counter = 64;
        }
    }

    CH1.disable_env = false;

    CH1.volume_timer = PERIOD_TABLE[IO_NR12.period];
    // if the next frame sequence clocks the vol, then
    // the timer is reloaded + 1.
    if (is_next_frame_sequencer_step_vol(gb))
    {
        CH1.volume_timer++;
    }

    // reload the volume
    CH1.volume = IO_NR12.starting_vol;

    // when a square channel is triggered, it's lower 2-bits are not modified!
    // SOURCE: https://gbdev.gg8.se/wiki/articles/Gameboy_sound_hardware#Obscure_Behavior
    CH1.timer = (CH1.timer & 0x3) | (get_ch1_freq(gb) & ~(0x3));

    // reload sweep timer with period
    CH1.sweep_timer = PERIOD_TABLE[IO_NR10.sweep_period];
    // the freq is loaded into the shadow_freq_reg
    CH1.freq_shadow_register = (IO_NR14.freq_msb << 8) | IO_NR13.freq_lsb;
    // internal flag is set is period or shift is non zero
    CH1.internal_enable_flag = (IO_NR10.sweep_period != 0) || (IO_NR10.sweep_shift != 0);
    // if shift is non zero, then calc is performed...
    if (IO_NR10.sweep_shift != 0)
    {
        do_freq_sweep_calc(gb);
    }

    if (is_ch1_dac_enabled(gb) == false)
    {
        ch1_disable(gb);
    }
}
