#pragma once

#include "common.h"
#include "../internal.h"

// used for rand().
// TODO: remove this when using proper LFSR method instead!!!
#include <stdlib.h>


// used for LFSR shifter
enum NoiseChannelShiftWidth {
    WIDTH_15_BITS = 0,
    WIDTH_7_BITS = 1,
};


// indexed using the noise divisor code
static const GB_U8 NOISE_DIVISOR[8] = { 8, 16, 32, 48, 64, 80, 96, 112 };

static inline GB_BOOL is_noise_dac_enabled(const struct GB_Core* gb) {
    return IO_NR42.starting_vol > 0 || IO_NR42.env_add_mode > 0;
}

static inline GB_BOOL is_noise_enabled(const struct GB_Core* gb) {
    return IO_NR52.noise > 0;
}

static inline void noise_enable(struct GB_Core* gb) {
    IO_NR52.noise = 1;
}

static inline void noise_disable(struct GB_Core* gb) {
    IO_NR52.noise = 0;
}

static inline GB_S8 sample_noise(struct GB_Core* gb) {
    return (rand() & 1) * NOISE_CHANNEL.volume_counter;
}

static inline void clock_noise_len(struct GB_Core* gb) {
    if (IO_NR44.length_enable && NOISE_CHANNEL.length_counter > 0) {
        --NOISE_CHANNEL.length_counter;
        // disable channel if we hit zero...
        if (NOISE_CHANNEL.length_counter == 0) {
            noise_disable(gb);
        }   
    }
}

static inline void clock_noise_vol(struct GB_Core* gb) {
    if (IO_NR42.period) {
        if (IO_NR42.env_add_mode == ADD) {
            if (NOISE_CHANNEL.volume_counter + 1 <= 15) {
                ++NOISE_CHANNEL.volume_counter;
            }
        } else {
            if (NOISE_CHANNEL.volume_counter - 1 >= 0) {
                --NOISE_CHANNEL.volume_counter;
            }
        }
    }
}

static inline void on_noise_trigger(struct GB_Core* gb) {
    noise_enable(gb);

    if (NOISE_CHANNEL.length_counter == 0) {
        NOISE_CHANNEL.length_counter = 64 - NOISE_CHANNEL.nr41.length_load;
    }
    
    // reload the volume
    NOISE_CHANNEL.volume_counter = IO_NR42.starting_vol;
    // set all bits of the lfsr to 1
    NOISE_CHANNEL.LFSR = 0xFFFF;

    // NOISE_CHANNEL.nr43.clock_shift = 0x0F;
    // NOISE_CHANNEL.nr43.width_mode = 0x01;
    // NOISE_CHANNEL.nr43.divisor_code = 0x07;
    
    if (is_noise_dac_enabled(gb) == GB_FALSE) {
        noise_disable(gb);
    }
}
