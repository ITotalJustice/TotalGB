#include "core/apu/common.h"
#include "core/apu/apu.h"
#include "core/internal.h"


uint16_t get_square1_freq(const struct GB_Core* gb) {
    return (2048 - ((IO_NR14.freq_msb << 8) | IO_NR13.freq_lsb)) << 2;
}

bool is_square1_dac_enabled(const struct GB_Core* gb) {
    return IO_NR12.starting_vol > 0 || IO_NR12.env_add_mode > 0;
}

bool is_square1_enabled(const struct GB_Core* gb) {
    return IO_NR52.square1 > 0;
}

void square1_enable(struct GB_Core* gb) {
    IO_NR52.square1 = 1;
}

void square1_disable(struct GB_Core* gb) {
    IO_NR52.square1 = 0;
}

int8_t sample_square1(struct GB_Core* gb) {
    if (SQUARE_DUTY_CYCLES[SQUARE1_CHANNEL.nr11.duty][SQUARE1_CHANNEL.duty_index]) {
        return SQUARE1_CHANNEL.volume;
    }
    return -SQUARE1_CHANNEL.volume;
}

void clock_square1_len(struct GB_Core* gb) {
    if (IO_NR14.length_enable && SQUARE1_CHANNEL.length_counter > 0) {
        --SQUARE1_CHANNEL.length_counter;
        // disable channel if we hit zero...
        if (SQUARE1_CHANNEL.length_counter == 0) {
            square1_disable(gb);
        }   
    }
}

void clock_square1_vol(struct GB_Core* gb) {
    if (SQUARE1_CHANNEL.disable_env == false) {
        --SQUARE1_CHANNEL.volume_timer;

        if (SQUARE1_CHANNEL.volume_timer <= 0) {
            SQUARE1_CHANNEL.volume_timer = PERIOD_TABLE[IO_NR12.period];

            if (IO_NR12.period != 0) {
                uint8_t new_vol = SQUARE1_CHANNEL.volume;
                if (IO_NR12.env_add_mode == ADD) {
                    ++new_vol;
                } else {
                    --new_vol;
                }

                if (new_vol <= 15) {
                    SQUARE1_CHANNEL.volume = new_vol;
                } else {
                    SQUARE1_CHANNEL.disable_env = true;
                }
            }
        }
    }
}

static uint16_t get_new_sweep_freq(const struct GB_Core* gb) {
    const uint16_t new_freq = SQUARE1_CHANNEL.freq_shadow_register >> IO_NR10.shift;
        
    if (IO_NR10.negate) {
        return SQUARE1_CHANNEL.freq_shadow_register - new_freq;
    } else {
        return SQUARE1_CHANNEL.freq_shadow_register + new_freq;
    }
}

void do_freq_sweep_calc(struct GB_Core* gb) {
    if (SQUARE1_CHANNEL.internal_enable_flag && IO_NR10.sweep_period) {
        const uint16_t new_freq = get_new_sweep_freq(gb);

        if (new_freq <= 2047) {
            SQUARE1_CHANNEL.freq_shadow_register = new_freq;
            IO_NR13.freq_lsb = new_freq & 0xFF;
            IO_NR14.freq_msb = new_freq >> 8;

            // for some reason, a second overflow check is performed...
            if (get_new_sweep_freq(gb) > 2047) {
                // overflow...
                square1_disable(gb);
            }

        } else { // overflow...
            square1_disable(gb);
        }
    }
}

void on_square1_sweep(struct GB_Core* gb) {
    // first check if sweep is enabled
    if (!SQUARE1_CHANNEL.internal_enable_flag || !is_square1_enabled(gb)) {
        return;
    }

    // decrement the counter, reload after
    --SQUARE1_CHANNEL.sweep_timer;

    if (SQUARE1_CHANNEL.sweep_timer <= 0) {
        // period is counted as 8 if 0...
        SQUARE1_CHANNEL.sweep_timer = PERIOD_TABLE[IO_NR10.sweep_period];
        do_freq_sweep_calc(gb);
    }
}

void on_square1_trigger(struct GB_Core* gb) {
    square1_enable(gb);

    if (SQUARE1_CHANNEL.length_counter == 0) {
        if (IO_NR14.length_enable && is_next_frame_suqencer_step_not_len(gb)) {
            SQUARE1_CHANNEL.length_counter = 63;
        } else {
            SQUARE1_CHANNEL.length_counter = 64;
        }
    }
    
    SQUARE1_CHANNEL.disable_env = false;
    
    SQUARE1_CHANNEL.volume_timer = PERIOD_TABLE[IO_NR12.period];
    // if the next frame sequence clocks the vol, then
    // the timer is reloaded + 1.
    if (is_next_frame_suqencer_step_vol(gb)) {
        SQUARE1_CHANNEL.volume_timer++;
    }
    
    // reload the volume
    SQUARE1_CHANNEL.volume = IO_NR12.starting_vol;
    
    // when a square channel is triggered, it's lower 2-bits are not modified!
    // SOURCE: https://gbdev.gg8.se/wiki/articles/Gameboy_sound_hardware#Obscure_Behavior
    SQUARE1_CHANNEL.timer = (SQUARE1_CHANNEL.timer & 0x3) | (get_square1_freq(gb) & ~(0x3));

    // reload sweep timer with period
    SQUARE1_CHANNEL.sweep_timer = PERIOD_TABLE[IO_NR10.sweep_period];
    // the freq is loaded into the shadow_freq_reg
    SQUARE1_CHANNEL.freq_shadow_register = (IO_NR14.freq_msb << 8) | IO_NR13.freq_lsb;
    // internal flag is set is period or shift is non zero
    SQUARE1_CHANNEL.internal_enable_flag = (IO_NR10.sweep_period != 0) || (IO_NR10.shift != 0);
    // if shift is non zero, then calc is performed...
    if (IO_NR10.shift != 0) {
        do_freq_sweep_calc(gb);
    }
    
    if (is_square1_dac_enabled(gb) == false) {
        square1_disable(gb);
    }
}
