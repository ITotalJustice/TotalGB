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

static inline GB_U16 get_square1_freq(const struct GB_Core* gb) { return (2048 - ((IO_NR14.freq_msb << 8) | IO_NR13.freq_lsb)) << 2; }
static inline GB_U16 get_square2_freq(const struct GB_Core* gb) { return (2048 - ((IO_NR24.freq_msb << 8) | IO_NR23.freq_lsb)) << 2; }
static inline GB_U16 get_wave_freq(const struct GB_Core* gb) { return (2048 - ((IO_NR34.freq_msb << 8) | IO_NR33.freq_lsb)) << 1; }

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

// indexed using the square duty code and duty cycle
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

// indexed using the noise divisor code
static const GB_U8 NOISE_DIVISOR[8] = { 8, 16, 32, 48, 64, 80, 96, 112 };

// indexed using the wave shift.
// the values in this table are used to right-shift the sample from wave_table
static const GB_U8 WAVE_SHIFT[4] = { 4, 0, 1, 2 };

// 4 * 1024^2 / 512
#define FRAME_SEQUENCER_STEP_RATE 8192

static void do_freq_sweep_calc(struct GB_Core* gb) {
    if (SQUARE1_CHANNEL.internal_enable_flag && IO_NR10.sweep_period) {
        GB_U16 new_freq = SQUARE1_CHANNEL.freq_shadow_register >> IO_NR10.shift;
        
        if (IO_NR10.negate) {
            new_freq = SQUARE1_CHANNEL.freq_shadow_register - -new_freq;
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

static inline void advance_wave_position_counter(struct GB_Core* gb) {
    ++WAVE_CHANNEL.position_counter;
    // check if we need to wrap around
    if (WAVE_CHANNEL.position_counter >= 32) {
        WAVE_CHANNEL.position_counter = 0;
    }

    WAVE_CHANNEL.sample_buffer = IO_WAVE_TABLE[WAVE_CHANNEL.position_counter >> 1];
}

enum EnvelopeMode {
    SUB = 0,
    ADD = 1
};

static inline void clock_square_1_len(struct GB_Core* gb) {
    if (IO_NR14.length_enable && SQUARE1_CHANNEL.length_counter > 0) {
        --SQUARE1_CHANNEL.length_counter;
        // disable channel if we hit zero...
        if (SQUARE1_CHANNEL.length_counter == 0) {
            square1_disable(gb);
        }   
    }
}

static inline void clock_square_2_len(struct GB_Core* gb) {
    if (IO_NR24.length_enable && SQUARE2_CHANNEL.length_counter > 0) {
        --SQUARE2_CHANNEL.length_counter;
        // disable channel if we hit zero...
        if (SQUARE2_CHANNEL.length_counter == 0) {
            square2_disable(gb);
        }   
    }
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

static inline void clock_noise_len(struct GB_Core* gb) {
    if (IO_NR44.length_enable && NOISE_CHANNEL.length_counter > 0) {
        --NOISE_CHANNEL.length_counter;
        // disable channel if we hit zero...
        if (NOISE_CHANNEL.length_counter == 0) {
            noise_disable(gb);
        }   
    }
}

static inline void clock_square_1_vol(struct GB_Core* gb) {
    if (IO_NR12.period) {
        if (IO_NR12.env_add_mode == ADD) {
            if (SQUARE1_CHANNEL.volume_counter + 1 <= 15) {
                ++SQUARE1_CHANNEL.volume_counter;
            }
        } else {
            if (SQUARE1_CHANNEL.volume_counter - 1 >= 0) {
                --SQUARE1_CHANNEL.volume_counter;
            }
        }
    }
}

static inline void clock_square_2_vol(struct GB_Core* gb) {
    if (IO_NR22.period) {
        if (IO_NR22.env_add_mode == ADD) {
            if (SQUARE2_CHANNEL.volume_counter + 1 <= 15) {
                ++SQUARE2_CHANNEL.volume_counter;
            }
        } else {
            if (SQUARE2_CHANNEL.volume_counter - 1 >= 0) {
                --SQUARE2_CHANNEL.volume_counter;
            }
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

static inline void clock_len(struct GB_Core* gb) {
    clock_square_1_len(gb);
    clock_square_2_len(gb);
    clock_wave_len(gb);
    clock_noise_len(gb);
}

static inline void clock_sweep(struct GB_Core* gb) {
    do_freq_sweep_calc(gb);
}

static inline void clock_vol(struct GB_Core* gb) {
    clock_square_1_vol(gb);
    clock_square_2_vol(gb);
    clock_noise_vol(gb);
}

// this runs at 512hz
static void step_frame_sequencer(struct GB_Core* gb) {
    switch (gb->apu.frame_sequencer_counter) {
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

    ++gb->apu.frame_sequencer_counter;
}

static void on_square_1_trigger(struct GB_Core* gb) {
    square1_enable(gb);

    if (SQUARE1_CHANNEL.length_counter == 0) {
        SQUARE1_CHANNEL.length_counter = 64 - SQUARE1_CHANNEL.nr11.length_load;
    }
    
    // reload the volume
    SQUARE1_CHANNEL.volume_counter = IO_NR12.starting_vol;

    SQUARE1_CHANNEL.timer = get_square1_freq(gb);

    // the freq is loaded into the shadow_freq_reg
    SQUARE1_CHANNEL.freq_shadow_register = (IO_NR14.freq_msb << 8) | IO_NR13.freq_lsb;
    // internal flag is set is period or shift is non zero
    SQUARE1_CHANNEL.internal_enable_flag = (IO_NR10.sweep_period != 0) || (IO_NR10.shift != 0);
    // if shift is non zero, then calc is performed...
    if (IO_NR10.shift != 0) {
        do_freq_sweep_calc(gb);
    }
    // todo:
    // - freq timer
    // - vol env timer
    
    if (is_square1_dac_enabled(gb) == GB_FALSE) {
        square1_disable(gb);
    }
}

static void on_square_2_trigger(struct GB_Core* gb) {
    square2_enable(gb);

    if (SQUARE2_CHANNEL.length_counter == 0) {
        SQUARE2_CHANNEL.length_counter = 64 - SQUARE2_CHANNEL.nr21.length_load;
    }

    // reload the volume
    SQUARE2_CHANNEL.volume_counter = IO_NR22.starting_vol;
    SQUARE2_CHANNEL.timer = get_square2_freq(gb);
    // todo:
    // - freq timer
    // - vol env timer
    
    if (is_square2_dac_enabled(gb) == GB_FALSE) {
        square2_disable(gb);
    }
}

static void on_wave_trigger(struct GB_Core* gb) {
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

static void on_noise_trigger(struct GB_Core* gb) {
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
    // todo:
    // - freq timer
    // - vol env timer
    
    if (is_noise_dac_enabled(gb) == GB_FALSE) {
        noise_disable(gb);
    }
}

#include <stdlib.h>
#include <stdio.h>

static GB_S8 sample_square1(struct GB_Core* gb) {
    return SQUARE_DUTY_CYCLES[SQUARE1_CHANNEL.nr11.duty][SQUARE1_CHANNEL.duty_index] ? SQUARE1_CHANNEL.volume_counter: 0;
}

static GB_S8 sample_square2(struct GB_Core* gb) {
    return SQUARE_DUTY_CYCLES[SQUARE2_CHANNEL.nr21.duty][SQUARE2_CHANNEL.duty_index] ? SQUARE2_CHANNEL.volume_counter : 0;
}

static GB_S8 sample_wave(struct GB_Core* gb) {
    // const GB_U8 index = WAVE_CHANNEL.position_counter >> 1;
    // GB_U8 wave_sample_4bit = WAVE_CHANNEL.wave_ram[index];
    // // check if we want hi or lo nibble by seeing if number is odd
    // if (index & 0x1) {
    //     wave_sample_4bit &= 0xF;
    // } else {
    //     wave_sample_4bit >>= 1;
    // }

    // // next, we have to do this shift thing.
    // return wave_sample_4bit >> WAVE_SHIFT[IO_NR32.vol_code];

    // this now uses the sample buffer, rather than indexing the array
    // which is *correct*, but still sounds very bad
    GB_S8 sample = (WAVE_CHANNEL.position_counter & 1) ? WAVE_CHANNEL.sample_buffer & 0xF : WAVE_CHANNEL.sample_buffer >> 4;

    // next, we have to do this shift thing.
    GB_U8 t = WAVE_SHIFT[IO_NR32.vol_code];
    if (t == 0) t = 1;
    return sample >> 1;//WAVE_SHIFT[IO_NR32.vol_code];
}

static GB_S8 sample_noise(struct GB_Core* gb) {
    return (rand() & 1) * NOISE_CHANNEL.volume_counter;
}

static void sample_channels(struct GB_Core* gb) {
    GB_U8 enabled_count = is_square1_enabled(gb) + is_square2_enabled(gb) + is_wave_enabled(gb) + is_noise_enabled(gb);

    const GB_S8 square1_sample = sample_square1(gb) * is_square1_enabled(gb);
    const GB_S8 square2_sample = sample_square2(gb) * is_square2_enabled(gb);
    const GB_S8 wave_sample = sample_wave(gb) * is_wave_enabled(gb);
    const GB_S8 noise_sample = sample_noise(gb) * is_noise_enabled(gb);

    // GB_S8 final_sample = 0;
    // if (enabled_count) {
    //     final_sample = (square1_sample + square2_sample + wave_sample + noise_sample) / 4;//enabled_count;
    // }
    const GB_S8 final_sample_left = (((square1_sample * IO_NR51.square1_left) + (square2_sample * IO_NR51.square2_left) + (wave_sample * IO_NR51.wave_left) + (noise_sample * IO_NR51.noise_left)) / 4) * CONTROL_CHANNEL.nr50.left_vol;
    const GB_S8 final_sample_right = (((square1_sample * IO_NR51.square1_right) + (square2_sample * IO_NR51.square2_right) + (wave_sample * IO_NR51.wave_right) + (noise_sample * IO_NR51.noise_right)) / 4) * CONTROL_CHANNEL.nr50.right_vol;
    gb->apu.samples[gb->apu.samples_count + 0] = final_sample_left;
    gb->apu.samples[gb->apu.samples_count + 1] = final_sample_right;

    // const GB_S8 final_sample = (square1_sample + square2_sample + noise_sample) / 3;
    // const GB_S8 final_sample = (square2_sample);
    // const GB_S8 final_sample = (wave_sample);
    // gb->apu.samples[gb->apu.samples_count + 0] = final_sample * ((GB_S8)CONTROL_CHANNEL.nr50.left_vol);
    // gb->apu.samples[gb->apu.samples_count + 1] = final_sample * ((GB_S8)CONTROL_CHANNEL.nr50.right_vol);

    gb->apu.samples_count += 2;

    if (gb->apu.samples_count >= 1024) {
        gb->apu.samples_count -= 1024;
        
        if (gb->apu_cb != NULL) {
            struct GB_ApuCallbackData data;
            memcpy(data.samples, gb->apu.samples, sizeof(gb->apu.samples));
            gb->apu_cb(gb, gb->apu_cb_user_data, &data);
        }
    }
}

static int init_start = 0;

GB_U8 GB_apu_ioread(const struct GB_Core* gb, const GB_U16 addr) {
	switch (addr & 0x7F) {
		case 0x10:
			return 0x80 | (IO_NR10.sweep_period << 4) | (IO_NR10.negate << 3) | IO_NR10.shift;

		case 0x11:
			return 0x3F | (IO_NR11.duty << 6);

		case 0x12:
			return (IO_NR12.starting_vol << 4) | (IO_NR12.env_add_mode << 3) | IO_NR12.period;

		case 0x14:
			return 0xBF | (IO_NR14.length_enable << 6);

		case 0x16:
			return 0x3F | (IO_NR21.duty << 6);

		case 0x17:
			return (IO_NR22.starting_vol << 4) | (IO_NR22.env_add_mode << 3) | IO_NR22.period;

		case 0x19:
			return 0xBF | (IO_NR24.length_enable << 6);

		case 0x1A:
			return 0x7F |  (IO_NR30.DAC_power << 7);

		case 0x1B:
			return IO_NR31.length_load;

		case 0x1C:
			return 0xBF | (IO_NR32.vol_code << 5);

		case 0x1E:
			return 0xBF | (IO_NR34.length_enable << 6);

		case 0x20:
			return 0xC0 | IO_NR41.length_load;

		case 0x21:
			return (IO_NR42.starting_vol << 4) | (IO_NR42.env_add_mode << 3) | IO_NR42.period;

		case 0x22:
			return (IO_NR43.clock_shift << 4) | (IO_NR43.width_mode << 3) | IO_NR43.divisor_code;

		case 0x23:
			return 0xBF | (IO_NR44.length_enable << 6);

		case 0x24:
			return (IO_NR50.vin_l << 7) | (IO_NR50.left_vol << 4) | (IO_NR50.vin_r << 3) | IO_NR50.right_vol;

		case 0x25:
			return (IO_NR51.noise_left << 7) | (IO_NR51.wave_left << 6) | (IO_NR51.square1_left << 5) | (IO_NR51.square2_left << 4) | (IO_NR51.noise_right << 3) | (IO_NR51.wave_right << 2) | (IO_NR51.square1_right << 1) | IO_NR51.square2_right;

		case 0x26:
			return 0x7F | (IO_NR52.power << 7);
	
        case 0x30: case 0x31: case 0x32: case 0x33:
        case 0x34: case 0x35: case 0x36: case 0x37:
        case 0x38: case 0x39: case 0x3A: case 0x3B:
        case 0x3C: case 0x3D: case 0x3E: case 0x3F:
            // return IO_WAVE_TABLE[addr & 0xF];
            return IO_WAVE_TABLE[WAVE_CHANNEL.position_counter >> 1];

        default:
            return 0xFF;
    }
}

void GB_apu_iowrite(struct GB_Core* gb, const GB_U16 addr, const GB_U8 value) {
	switch (addr & 0x7F) {
		case 0x10:
			IO_NR10.sweep_period = (value >> 4) & 0x7;
			IO_NR10.negate = (value >> 3) & 0x1;
			IO_NR10.shift = value & 0x7;
			break;

		case 0x11:
			IO_NR11.duty = value >> 6;
			IO_NR11.length_load = value & 0x3F;
			break;

		case 0x12:
			IO_NR12.starting_vol = value >> 4;
			IO_NR12.env_add_mode = (value >> 3) & 0x1;
			IO_NR12.period = value & 0x7;
            if (is_square1_dac_enabled(gb) == GB_FALSE) {
                square1_disable(gb);
            }
			break;

		case 0x13:
			IO_NR13.freq_lsb = value;
			break;

		case 0x14:
			IO_NR14.trigger = (value >> 7) & 0x1;
			IO_NR14.length_enable = (value >> 6) & 0x1;
			IO_NR14.freq_msb = value & 0x7;

			if (IO_NR14.trigger) {
                on_square_1_trigger(gb);
			}
			break;

		case 0x16:
			IO_NR21.duty = value >> 6;
			IO_NR21.length_load = value & 0x3F;
			break;

		case 0x17:
			IO_NR22.starting_vol = value >> 4;
			IO_NR22.env_add_mode = (value >> 3) & 0x1;
			IO_NR22.period = value & 0x7;
            if (is_square2_dac_enabled(gb) == GB_FALSE) {
                square2_disable(gb);
            }
			break;

		case 0x18:
			IO_NR23.freq_lsb = value;
			break;

		case 0x19:
			IO_NR24.trigger = (value >> 7) & 0x1;
			IO_NR24.length_enable = (value >> 6) & 0x1;
			IO_NR24.freq_msb = value & 0x7;

			if (IO_NR24.trigger) {
                on_square_2_trigger(gb);
			}
			break;

		case 0x1A:
			IO_NR30.DAC_power = value >> 7;
            if (is_wave_dac_enabled(gb) == GB_FALSE) {
                wave_disable(gb);
            }
			break;

		case 0x1B:
			IO_NR31.length_load = value;
			break;

		case 0x1C:
			IO_NR32.vol_code = (value >> 5) & 0x3;
			break;

		case 0x1D:
			IO_NR33.freq_lsb = value;
			break;

		case 0x1E:
			IO_NR34.trigger = (value >> 7) & 0x1;
			IO_NR34.length_enable = (value >> 6) & 0x1;
			IO_NR34.freq_msb = value & 0x7;

            if (IO_NR34.trigger) {
                on_wave_trigger(gb);
			}
			break;

		case 0x20:
			IO_NR41.length_load = value & 0x3F;
			break;

		case 0x21:
			IO_NR42.starting_vol = value >> 4;
			IO_NR42.env_add_mode = (value >> 3) & 0x1;
			IO_NR42.period = value & 0x7;
            if (is_noise_dac_enabled(gb) == GB_FALSE) {
                noise_disable(gb);
            }
			break;

		case 0x22:
			IO_NR43.clock_shift = value >> 4;
			IO_NR43.width_mode = (value >> 3) & 0x1;
			IO_NR43.divisor_code = value & 0x7;
			break;

		case 0x23:
			IO_NR44.trigger = (value >> 7) & 0x1;
			IO_NR44.length_enable = (value >> 6) & 0x1;
			
            if (IO_NR44.trigger) {
                on_noise_trigger(gb);
			}
            break;

		case 0x24:
			IO_NR50.vin_l = value >> 7;
			IO_NR50.left_vol = (value >> 4) & 0x7;
			IO_NR50.vin_r = (value >> 3) & 0x1;
			IO_NR50.right_vol = value & 0x7;
			break;

		case 0x25:
			IO_NR51.noise_left = value >> 7;
			IO_NR51.wave_left = (value >> 6) & 0x1;
			IO_NR51.square2_left = (value >> 5) & 0x1;
			IO_NR51.square1_left = (value >> 4) & 0x1;
			IO_NR51.noise_right = (value >> 3) & 0x1;
			IO_NR51.wave_right = (value >> 2) & 0x1;
			IO_NR51.square2_right = (value >> 1) & 0x1;
			IO_NR51.square1_right = value & 0x1;
			break;

		case 0x26:
			IO_NR52.power = value >> 7;
			break;

        case 0x30: case 0x31: case 0x32: case 0x33:
        case 0x34: case 0x35: case 0x36: case 0x37:
        case 0x38: case 0x39: case 0x3A: case 0x3B:
        case 0x3C: case 0x3D: case 0x3E: case 0x3F:
            // IO_WAVE_TABLE[addr & 0xF] = value;
            IO_WAVE_TABLE[WAVE_CHANNEL.position_counter >> 1] = value;
            break;
	}
}

void GB_apu_run(struct GB_Core* gb, GB_U16 cycles) {
    // for debug...
    if (init_start == 0) {
        SQUARE1_CHANNEL.timer = 2048;
        SQUARE2_CHANNEL.timer = 2048;
        WAVE_CHANNEL.timer = 2048;
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

    WAVE_CHANNEL.timer -= cycles;
    if (WAVE_CHANNEL.timer <= 0) {
        WAVE_CHANNEL.timer = get_wave_freq(gb);
        advance_wave_position_counter(gb);
    }

    gb->apu.next_frame_sequencer_cycles += cycles;
    gb->apu.next_sample_cycles += cycles;

    if (gb->apu.next_frame_sequencer_cycles >= FRAME_SEQUENCER_STEP_RATE) {
        gb->apu.next_frame_sequencer_cycles -= FRAME_SEQUENCER_STEP_RATE;
        step_frame_sequencer(gb);
    }

    if (gb->apu.next_sample_cycles >= SAMPLE_RATE) {
        gb->apu.next_sample_cycles -= SAMPLE_RATE;
        sample_channels(gb);
    }
}
