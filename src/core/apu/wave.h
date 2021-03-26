#pragma once

#include "common.h"
#include "../internal.h"


// indexed using the wave shift.
// the values in this table are used to right-shift the sample from wave_table
static const GB_U8 WAVE_SHIFT[4] = { 4, 0, 1, 2 };


static inline GB_U16 get_wave_freq(const struct GB_Core* gb) {
    return (2048 - ((IO_NR34.freq_msb << 8) | IO_NR33.freq_lsb)) << 1;
}

static inline GB_BOOL is_wave_dac_enabled(const struct GB_Core* gb) {
    return IO_NR30.DAC_power > 0;
}

static inline GB_BOOL is_wave_enabled(const struct GB_Core* gb) {
    return IO_NR52.wave > 0;
}

static inline void wave_enable(struct GB_Core* gb) {
    IO_NR52.wave = 1;
}

static inline void wave_disable(struct GB_Core* gb) {
    IO_NR52.wave = 0;
}

static inline GB_S8 sample_wave(struct GB_Core* gb) {
    // this now uses the sample buffer, rather than indexing the array
    // which is *correct*, but still sounds very bad
    GB_S8 sample = (WAVE_CHANNEL.position_counter & 1) ? WAVE_CHANNEL.sample_buffer & 0xF : WAVE_CHANNEL.sample_buffer >> 4;

    // next, we have to do this shift thing.
    GB_U8 t = WAVE_SHIFT[IO_NR32.vol_code];
    // this is a hack, because i have not fixed the volumes
    // the wave channel is very loud at 100%.
    // so for now, 0-75% vol.
    if (t == 0) {
        t = 1;
    }

    return sample >> 1;//WAVE_SHIFT[IO_NR32.vol_code];
}

static inline void clock_wave_len(struct GB_Core* gb) {
    if (IO_NR34.length_enable && WAVE_CHANNEL.length_counter > 0) {
        --WAVE_CHANNEL.length_counter;
        // disable channel if we hit zero...
        if (WAVE_CHANNEL.length_counter == 0) {
            wave_disable(gb);
        }   
    }
}

static inline void advance_wave_position_counter(struct GB_Core* gb) {
    ++WAVE_CHANNEL.position_counter;
    // check if we need to wrap around
    if (WAVE_CHANNEL.position_counter >= 32) {
        WAVE_CHANNEL.position_counter = 0;
    }

    WAVE_CHANNEL.sample_buffer = IO_WAVE_TABLE[WAVE_CHANNEL.position_counter >> 1];
}

static inline void on_wave_trigger(struct GB_Core* gb) {
    wave_enable(gb);

    if (WAVE_CHANNEL.length_counter == 0) {
        WAVE_CHANNEL.length_counter = 256 - WAVE_CHANNEL.nr31.length_load;
    }

    // reset position counter
    WAVE_CHANNEL.position_counter = 0;

    WAVE_CHANNEL.timer = get_wave_freq(gb);
    
    if (is_wave_dac_enabled(gb) == GB_FALSE) {
        wave_disable(gb);
    }
}
