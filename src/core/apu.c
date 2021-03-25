#include "gb.h"
#include "internal.h"

#include <string.h>

//        Square 1
// NR10 FF10 -PPP NSSS Sweep period, negate, shift
// NR11 FF11 DDLL LLLL Duty, Length load (64-L)
// NR12 FF12 VVVV APPP Starting volume, Envelope add mode, period
// NR13 FF13 FFFF FFFF Frequency LSB
// NR14 FF14 TL-- -FFF Trigger, Length enable, Frequency MSB

//        Square 2
//      FF15 ---- ---- Not used
// NR21 FF16 DDLL LLLL Duty, Length load (64-L)
// NR22 FF17 VVVV APPP Starting volume, Envelope add mode, period
// NR23 FF18 FFFF FFFF Frequency LSB
// NR24 FF19 TL-- -FFF Trigger, Length enable, Frequency MSB

//        Wave
// NR30 FF1A E--- ---- DAC power
// NR31 FF1B LLLL LLLL Length load (256-L)
// NR32 FF1C -VV- ---- Volume code (00=0%, 01=100%, 10=50%, 11=25%)
// NR33 FF1D FFFF FFFF Frequency LSB
// NR34 FF1E TL-- -FFF Trigger, Length enable, Frequency MSB

//        Noise
//      FF1F ---- ---- Not used
// NR41 FF20 --LL LLLL Length load (64-L)
// NR42 FF21 VVVV APPP Starting volume, Envelope add mode, period
// NR43 FF22 SSSS WDDD Clock shift, Width mode of LFSR, Divisor code
// NR44 FF23 TL-- ---- Trigger, Length enable

//        Control/Status
// NR50 FF24 ALLL BRRR Vin L enable, Left vol, Vin R enable, Right vol
// NR51 FF25 NW21 NW21 Left enables, Right enables
// NR52 FF26 P--- NW21 Power control/status, Channel length statuses

#define SQUARE1_CHANNEL gb->apu.square1
#define SQUARE2_CHANNEL gb->apu.square2
#define WAVE_CHANNEL gb->apu.wave
#define NOISE_CHANNEL gb->apu.noise
#define CONTROL_CHANNEL gb->apu.control

#define SAMPLE_RATE (4194304 / 48000)

/* every channel has a trigger, which is bit 7 (0x80) */
/* when a channel is triggered, it usually resets it's values, such */
/* as volume and timers. */

enum NoiseChannelShiftWidth {
    WIDTH_15_BITS = 0,
    WIDTH_7_BITS = 1,
};

static inline GB_U8 GB_wave_volume(GB_U8 code) {
    /* these are volume %, so 0%, 100%... */
    static const GB_U8 volumes[] = {0U, 100U, 50U, 25U};
    return volumes[code & 0x3];
}

static inline GB_U16 get_square1_freq(const struct GB_Core* gb) { return (IO_NR14.freq_msb << 8) | IO_NR13.freq_lsb; }
static inline GB_U16 get_square2_freq(const struct GB_Core* gb) { return (IO_NR24.freq_msb << 8) | IO_NR23.freq_lsb; }
static inline GB_U16 get_wave_freq(const struct GB_Core* gb) { return (IO_NR34.freq_msb << 8) | IO_NR33.freq_lsb; }

static inline GB_BOOL is_square1_dac_enabled(const struct GB_Core* gb) { return IO_NR12.starting_vol > 0 || IO_NR12.env_add_mode > 0; }
static inline GB_BOOL is_square2_dac_enabled(const struct GB_Core* gb) { return IO_NR22.starting_vol > 0 || IO_NR22.env_add_mode > 0; }
static inline GB_BOOL is_wave_dac_enabled(const struct GB_Core* gb) { return IO_NR30.DAC_power > 0; }
static inline GB_BOOL is_noise_dac_enabled(const struct GB_Core* gb) { return IO_NR42.starting_vol > 0 || IO_NR42.env_add_mode > 0; }

static inline GB_BOOL is_square1_enabled(const struct GB_Core* gb) { return IO_NR52.square1 > 0; }
static inline GB_BOOL is_square2_enabled(const struct GB_Core* gb) { return IO_NR52.square2 > 0; }
static inline GB_BOOL is_wave_enabled(const struct GB_Core* gb) { return IO_NR52.wave > 0; }
static inline GB_BOOL is_noise_enabled(const struct GB_Core* gb) { return IO_NR52.noise > 0; }

static inline void square1_enable(struct GB_Core* gb) { IO_NR52.square1 = 1; }
static inline void square2_enable(struct GB_Core* gb) { IO_NR52.square2 = 1; }
static inline void wave_enable(struct GB_Core* gb) { IO_NR52.wave = 1; }
static inline void noise_enable(struct GB_Core* gb) { IO_NR52.noise = 1; }

static inline void square1_disable(struct GB_Core* gb) { IO_NR52.square1 = 0; }
static inline void square2_disable(struct GB_Core* gb) { IO_NR52.square2 = 0; }
static inline void wave_disable(struct GB_Core* gb) { IO_NR52.wave = 0; }
static inline void noise_disable(struct GB_Core* gb) { IO_NR52.noise = 0; }

// clocked at 512hz
#define FRAME_SEQUENCER_CLOCK 512

static const GB_BOOL SQUARE_DUTY_CYCLE_0[8] = { 0, 0, 0, 0, 0, 0, 0, 1 };
static const GB_BOOL SQUARE_DUTY_CYCLE_1[8] = { 1, 0, 0, 0, 0, 0, 0, 1 };
static const GB_BOOL SQUARE_DUTY_CYCLE_2[8] = { 0, 0, 0, 0, 0, 1, 1, 1 };
static const GB_BOOL SQUARE_DUTY_CYCLE_3[8] = { 0, 1, 1, 1, 1, 1, 1, 0 };
static const GB_BOOL* const SQUARE_DUTY_CYCLES[4] = {
    SQUARE_DUTY_CYCLE_0,
    SQUARE_DUTY_CYCLE_1,
    SQUARE_DUTY_CYCLE_2,
    SQUARE_DUTY_CYCLE_3
};

// 4194304 / 256; // 256 Hz

//       gb = 2048 - (131072 / Hz)
//       Hz = 131072 / (2048 - gb)

// Channels 1-3 can produce frequencies of 64hz-131072hz

// Channel 4 can produce bit-frequencies of 2hz-1048576hz.

// Hz = 4194304 / ((2048 - (11-bit-freq)) << 5)

// 4 * 1024^2 / 512
#define FRAME_SEQUENCER_STEP_RATE 8192

static void clock_square_1_vol(struct GB_Core* gb) {

}

static void clock_len(struct GB_Core* gb) {

}

static void clock_sweep(struct GB_Core* gb) {

}

static void clock_vol(struct GB_Core* gb) {

}

// this runs at 512hz
static void step_frame_sequencer(struct GB_Core* gb, GB_U8 sequence) {
    switch (sequence & 7) {
        case 0: // len
            clock_len(gb);
            break;

        case 1:
            break;

        case 2: // len, sweep
            clock_len(gb);
            clock_sweep(gb);
            break;

        case 3:
            break;

        case 4: // len
            clock_len(gb);
            break;

        case 5:
            break;

        case 6: // len, sweep
            clock_len(gb);
            clock_sweep(gb);
            break;

        case 7: // vol
            clock_vol(gb);
            break;      
    }

    ++sequence;
}

static GB_U16 get_channel_1_frequency(const struct GB_Core* gb) {
    // get the hi and low bits.
    const GB_U16 x = ((IO_NR14.freq_msb) << 8) | IO_NR13.freq_lsb;
    // return 131072 / (2048 - x);
    return (2048 - x) * 4;
}

static void on_square_1_trigger(struct GB_Core* gb) {
    square1_enable(gb);

    if (SQUARE1_CHANNEL.nr11.length_load == 0) {
        SQUARE1_CHANNEL.nr11.length_load = 0x3F;
    }
    // todo:
    // - freq timer
    // - vol env timer
    // - channel vol
    
    if (is_square1_dac_enabled(gb) == GB_FALSE) {
        square1_disable(gb);
    }
}

static void on_square_2_trigger(struct GB_Core* gb) {
    square2_enable(gb);

    if (SQUARE2_CHANNEL.nr21.length_load == 0) {
        SQUARE2_CHANNEL.nr21.length_load = 0x3F;
    }
    // todo:
    // - freq timer
    // - vol env timer
    // - channel vol
    
    if (is_square2_dac_enabled(gb) == GB_FALSE) {
        square2_disable(gb);
    }
}

static void on_wave_trigger(struct GB_Core* gb) {
    wave_enable(gb);

    if (WAVE_CHANNEL.nr31.length_load == 0) {
        WAVE_CHANNEL.nr31.length_load = 0xFF;
    }
    // todo:
    // - freq timer
    // - vol env timer
    // - channel vol
    
    if (is_wave_dac_enabled(gb) == GB_FALSE) {
        wave_disable(gb);
    }
}

static void on_noise_trigger(struct GB_Core* gb) {
    noise_enable(gb);

    if (NOISE_CHANNEL.nr41.length_load == 0) {
        NOISE_CHANNEL.nr41.length_load = 0x3F;
    }
    NOISE_CHANNEL.nr43.clock_shift = 0x0F;
    NOISE_CHANNEL.nr43.width_mode = 0x01;
    NOISE_CHANNEL.nr43.divisor_code = 0x07;
    // todo:
    // - freq timer
    // - vol env timer
    // - channel vol
    
    if (is_noise_dac_enabled(gb) == GB_FALSE) {
        noise_disable(gb);
    }
}

#include <stdlib.h>
#include <stdio.h>

// this is wrong but
static GB_S8 sample_square1(struct GB_Core* gb) {
    return SQUARE_DUTY_CYCLES[SQUARE1_CHANNEL.nr11.duty][SQUARE1_CHANNEL.duty_index] ? (SQUARE1_CHANNEL.nr12.starting_vol) : 0;
}

static GB_S8 sample_square2(struct GB_Core* gb) {
    return SQUARE_DUTY_CYCLES[SQUARE2_CHANNEL.nr21.duty][SQUARE2_CHANNEL.duty_index] ? (SQUARE2_CHANNEL.nr22.starting_vol) : 0;
}

static GB_S8 sample_wave(struct GB_Core* gb) {
    return 0x0000;
}

static GB_S8 sample_noise(struct GB_Core* gb) {
    return 0x0000;
}

static void sample_channels(struct GB_Core* gb) {
    const GB_S8 square1_sample = sample_square1(gb);
    const GB_S8 square2_sample = sample_square2(gb);
    const GB_S8 wave_sample = sample_wave(gb);
    const GB_S8 noise_sample = sample_noise(gb);

    const GB_S8 final_sample = (square1_sample + square2_sample) / 2;
    gb->apu.samples[gb->apu.samples_count + 0] = final_sample;
    gb->apu.samples[gb->apu.samples_count + 1] = final_sample;

    gb->apu.samples_count += 2;

    if (gb->apu.samples_count >= 1024) {
        // printf("ye index %u sizeof %llu\n", gb->apu.samples_count, sizeof(gb->apu.samples));
        gb->apu.samples_count -= 1024;
        
        if (gb->apu_cb != NULL) {
            struct GB_ApuCallbackData data;
            memcpy(data.samples, gb->apu.samples, sizeof(gb->apu.samples));
            gb->apu_cb(gb, gb->apu_cb_user_data, &data);
            // printf("sending...\n");
        }
    }
}

static int init_start = 0;

void GB_apu_run(struct GB_Core* gb, GB_U16 cycles) {
    // for debug...
    if (init_start == 0) {
        SQUARE1_CHANNEL.timer = 2048;
        SQUARE2_CHANNEL.timer = 2048;
        ++init_start;
    }
    
    SQUARE1_CHANNEL.timer -= cycles;
    if (SQUARE1_CHANNEL.timer <= 0) {
        SQUARE1_CHANNEL.timer = get_square1_freq(gb);
        ++SQUARE1_CHANNEL.duty_index;
    }

    SQUARE2_CHANNEL.timer -= cycles;
    if (SQUARE2_CHANNEL.timer <= 0) {
        SQUARE2_CHANNEL.timer = get_square2_freq(gb);
        ++SQUARE2_CHANNEL.duty_index;
    }

    gb->apu.next_frame_sequencer_cycles += cycles;
    gb->apu.next_sample_cycles += cycles;

    if (gb->apu.next_frame_sequencer_cycles >= FRAME_SEQUENCER_STEP_RATE) {
        gb->apu.next_frame_sequencer_cycles -= FRAME_SEQUENCER_STEP_RATE;
        step_frame_sequencer(gb, 0);
    }

    if (gb->apu.next_sample_cycles >= SAMPLE_RATE) {
        gb->apu.next_sample_cycles -= SAMPLE_RATE;
        sample_channels(gb);
    }
}
