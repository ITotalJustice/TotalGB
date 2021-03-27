#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"
#include "square_common.h"
#include "../internal.h"


static inline GB_U16 get_square2_freq(const struct GB_Core* gb) {
    return (2048 - ((IO_NR24.freq_msb << 8) | IO_NR23.freq_lsb)) << 2;
}

static inline GB_BOOL is_square2_dac_enabled(const struct GB_Core* gb) {
    return IO_NR22.starting_vol > 0 || IO_NR22.env_add_mode > 0;
}

static inline GB_BOOL is_square2_enabled(const struct GB_Core* gb) {
    return IO_NR52.square2 > 0;
}

static inline void square2_enable(struct GB_Core* gb) {
    IO_NR52.square2 = 1;
}

static inline void square2_disable(struct GB_Core* gb) {
    IO_NR52.square2 = 0;
}

static inline GB_S8 sample_square2(struct GB_Core* gb) {
    if (SQUARE_DUTY_CYCLES[SQUARE2_CHANNEL.nr21.duty][SQUARE2_CHANNEL.duty_index]) {
        return SQUARE2_CHANNEL.volume;
    }
    return 0;
}

static inline void clock_square2_len(struct GB_Core* gb) {
    if (IO_NR24.length_enable && SQUARE2_CHANNEL.length_counter > 0) {
        --SQUARE2_CHANNEL.length_counter;
        // disable channel if we hit zero...
        if (SQUARE2_CHANNEL.length_counter == 0) {
            square2_disable(gb);
        }   
    }
}

static inline void clock_square2_vol(struct GB_Core* gb) {
    if (SQUARE2_CHANNEL.disable_env == GB_FALSE) {
        --SQUARE2_CHANNEL.volume_timer;

        if (SQUARE2_CHANNEL.volume_timer <= 0) {
            SQUARE2_CHANNEL.volume_timer = IO_NR22.period == 0 ? 8 : IO_NR22.period;

            if (IO_NR22.period != 0) {
                GB_U8 new_vol = SQUARE2_CHANNEL.volume;
                if (IO_NR22.env_add_mode == ADD) {
                    ++new_vol;
                } else {
                    --new_vol;
                }

                if (new_vol <= 15) {
                    SQUARE2_CHANNEL.volume = new_vol;
                } else {
                    SQUARE2_CHANNEL.disable_env = GB_TRUE;
                }
            }
        }
    }
}

static inline void on_square2_trigger(struct GB_Core* gb) {
    square2_enable(gb);

    if (SQUARE2_CHANNEL.length_counter == 0) {
        SQUARE2_CHANNEL.length_counter = 64 - SQUARE2_CHANNEL.nr21.length_load;
    }

    SQUARE2_CHANNEL.disable_env = GB_FALSE;
    SQUARE2_CHANNEL.volume_timer = IO_NR22.period == 0 ? 8 : IO_NR22.period;
    // reload the volume
    SQUARE2_CHANNEL.volume = IO_NR22.starting_vol;
    SQUARE2_CHANNEL.timer = get_square2_freq(gb);
    // todo:
    // - freq timer
    // - vol env timer
    
    if (is_square2_dac_enabled(gb) == GB_FALSE) {
        square2_disable(gb);
    }
}

#ifdef __cplusplus
}
#endif
