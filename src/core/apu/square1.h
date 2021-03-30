#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"
#include  "square_common.h"
#include "../internal.h"


static inline GB_U16 get_square1_freq(const struct GB_Core* gb) {
    return (2048 - ((IO_NR14.freq_msb << 8) | IO_NR13.freq_lsb)) << 2;
}

static inline GB_BOOL is_square1_dac_enabled(const struct GB_Core* gb) {
    return IO_NR12.starting_vol > 0 || IO_NR12.env_add_mode > 0;
}

static inline GB_BOOL is_square1_enabled(const struct GB_Core* gb) {
    return IO_NR52.square1 > 0;
}

static inline void square1_enable(struct GB_Core* gb) {
    IO_NR52.square1 = 1;
}

static inline void square1_disable(struct GB_Core* gb) {
    IO_NR52.square1 = 0;
}

static inline GB_S8 sample_square1(struct GB_Core* gb) {
    if (SQUARE_DUTY_CYCLES[SQUARE1_CHANNEL.nr11.duty][SQUARE1_CHANNEL.duty_index]) {
        return SQUARE1_CHANNEL.volume;
    }
    return -SQUARE1_CHANNEL.volume;
}

static inline void clock_square1_len(struct GB_Core* gb) {
    if (IO_NR14.length_enable && SQUARE1_CHANNEL.length_counter > 0) {
        --SQUARE1_CHANNEL.length_counter;
        // disable channel if we hit zero...
        if (SQUARE1_CHANNEL.length_counter == 0) {
            square1_disable(gb);
        }   
    }
}

static inline void clock_square1_vol(struct GB_Core* gb) {
    if (SQUARE1_CHANNEL.disable_env == GB_FALSE) {
        --SQUARE1_CHANNEL.volume_timer;

        if (SQUARE1_CHANNEL.volume_timer <= 0) {
            SQUARE1_CHANNEL.volume_timer = PERIOD_TABLE[IO_NR12.period];

            if (IO_NR12.period != 0) {
                GB_U8 new_vol = SQUARE1_CHANNEL.volume;
                if (IO_NR12.env_add_mode == ADD) {
                    ++new_vol;
                } else {
                    --new_vol;
                }

                if (new_vol <= 15) {
                    SQUARE1_CHANNEL.volume = new_vol;
                } else {
                    SQUARE1_CHANNEL.disable_env = GB_TRUE;
                }
            }
        }
    }
}

static inline void do_freq_sweep_calc(struct GB_Core* gb) {
    if (SQUARE1_CHANNEL.internal_enable_flag && IO_NR10.sweep_period) {
        GB_U16 new_freq = SQUARE1_CHANNEL.freq_shadow_register >> IO_NR10.shift;
        
        if (IO_NR10.negate) {
            new_freq = SQUARE1_CHANNEL.freq_shadow_register + -new_freq;
        } else {
            new_freq = SQUARE1_CHANNEL.freq_shadow_register + new_freq;
        }

        if (new_freq <= 2047) {
            SQUARE1_CHANNEL.freq_shadow_register = new_freq;
            IO_NR13.freq_lsb = new_freq & 0xFF;
            IO_NR14.freq_msb = new_freq >> 8;
        } else { // overflow...
            square1_disable(gb);
        }
    }
}

static inline void on_square1_sweep(struct GB_Core* gb) {
    // decrement the counter, reload after
    --SQUARE1_CHANNEL.sweep_timer;

    if (SQUARE1_CHANNEL.sweep_timer <= 0) {
        // period is counted as 8 if 0...
        SQUARE1_CHANNEL.sweep_timer = PERIOD_TABLE[IO_NR10.sweep_period];
        do_freq_sweep_calc(gb);
    }
}

static inline void on_square1_trigger(struct GB_Core* gb) {
    square1_enable(gb);

    if (SQUARE1_CHANNEL.length_counter == 0) {
        SQUARE1_CHANNEL.length_counter = 64;
    }
    
    SQUARE1_CHANNEL.disable_env = GB_FALSE;
    SQUARE1_CHANNEL.volume_timer = PERIOD_TABLE[IO_NR12.period];
    // reload the volume
    SQUARE1_CHANNEL.volume = IO_NR12.starting_vol;
    // reload the timer with freq
    SQUARE1_CHANNEL.timer = get_square1_freq(gb);
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
    
    if (is_square1_dac_enabled(gb) == GB_FALSE) {
        square1_disable(gb);
    }
}

#ifdef __cplusplus
}
#endif
