#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"
#include "../internal.h"

// used for LFSR shifter
enum NoiseChannelShiftWidth {
    WIDTH_15_BITS = 0,
    WIDTH_7_BITS = 1,
};

// indexed using the noise divisor code
static const GB_U8 NOISE_DIVISOR[8] = { 8, 16, 32, 48, 64, 80, 96, 112 };

/*
SOURCE: http://belogic.com/gba/registers.shtml#REG_SOUND4CNT_H
A Clock divider controlled by bits 0-2 divides the CPU frequency.
Its output is then fed into the counter's pre-scaler (controlled by bits 4-7)
which divides further more the frequency.
*/
static inline GB_U16 get_noise_freq(const struct GB_Core* gb) {
    // divide the cpu clock by the divisor
    const GB_U16 divided_clock = DMG_CPU_CLOCK / NOISE_DIVISOR[IO_NR43.divisor_code];
    // feed this result to the pre-scaler, which futher divides the clock
    const GB_U16 pre_scaler_result = divided_clock >> IO_NR43.clock_shift;
    // we now have the new timer!
    return pre_scaler_result;
}

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
    // docs say that it's bit-0 INVERTED
    const GB_S8 bit = !(NOISE_CHANNEL.LFSR & 0x1);
    return bit * NOISE_CHANNEL.volume;
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
    if (NOISE_CHANNEL.disable_env == GB_FALSE) {
        --NOISE_CHANNEL.volume_timer;

        if (NOISE_CHANNEL.volume_timer <= 0) {
            NOISE_CHANNEL.volume_timer = IO_NR42.period == 0 ? 8 : IO_NR42.period;

            if (IO_NR42.period != 0) {
                GB_U8 new_vol = NOISE_CHANNEL.volume;
                if (IO_NR42.env_add_mode == ADD) {
                    ++new_vol;
                } else {
                    --new_vol;
                }

                if (new_vol <= 15) {
                    NOISE_CHANNEL.volume = new_vol;
                } else {
                    NOISE_CHANNEL.disable_env = GB_TRUE;
                }
            }
        }
    }
}

static inline void step_noise_lfsr(struct GB_Core* gb) {
    // this is explicit...
    const GB_U8 bit0 = NOISE_CHANNEL.LFSR & 0x1;
    const GB_U8 bit1 = (NOISE_CHANNEL.LFSR >> 1) & 0x1;
    const GB_U8 result = bit1 ^ bit0;

    // now we shift the lfsr BEFORE setting the value!
    NOISE_CHANNEL.LFSR >>= 1;

    // now set bit 15
    NOISE_CHANNEL.LFSR |= (result << 14);

    // set bit-6 if the width is half-mode
    if (IO_NR43.width_mode == WIDTH_7_BITS) {
        // unset it first!
        NOISE_CHANNEL.LFSR &= ~(1 << 5);
        NOISE_CHANNEL.LFSR |= (result << 5);
    }
}

static inline void on_noise_trigger(struct GB_Core* gb) {
    noise_enable(gb);

    if (NOISE_CHANNEL.length_counter == 0) {
        NOISE_CHANNEL.length_counter = 64 - NOISE_CHANNEL.nr41.length_load;
    }
    
    NOISE_CHANNEL.disable_env = GB_FALSE;
    NOISE_CHANNEL.volume_timer = IO_NR42.period == 0 ? 8 : IO_NR42.period;
    // reload the volume
    NOISE_CHANNEL.volume = IO_NR42.starting_vol;
    // set all bits of the lfsr to 1
    NOISE_CHANNEL.LFSR = 0x7FFF;

    NOISE_CHANNEL.timer = get_noise_freq(gb);
    
    if (is_noise_dac_enabled(gb) == GB_FALSE) {
        noise_disable(gb);
    }
}

#ifdef __cplusplus
}
#endif
