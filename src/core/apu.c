#include "gb.h"
#include "internal.h"

#include "apu/square1.h"
#include "apu/square2.h"
#include "apu/wave.h"
#include "apu/noise.h"

#include <stdio.h>
#include <string.h>

static inline void clock_len(struct GB_Core* gb) {
    clock_square1_len(gb);
    clock_square2_len(gb);
    clock_wave_len(gb);
    clock_noise_len(gb);
}

static inline void clock_sweep(struct GB_Core* gb) {
    on_square1_sweep(gb);
}

static inline void clock_vol(struct GB_Core* gb) {
    clock_square1_vol(gb);
    clock_square2_vol(gb);
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

static void sample_channels(struct GB_Core* gb) {
    const GB_S8 square1_sample = sample_square1(gb) * is_square1_enabled(gb);
    const GB_S8 square2_sample = sample_square2(gb) * is_square2_enabled(gb);
    const GB_S8 wave_sample = sample_wave(gb) * is_wave_enabled(gb);
    const GB_S8 noise_sample = sample_noise(gb) * is_noise_enabled(gb);

// for testing, test all channels VS test everything but wave channel...
#define MODE 1
#if MODE == 0
    const GB_S8 final_sample_left = (((square1_sample * IO_NR51.square1_left) + (square2_sample * IO_NR51.square2_left) + (wave_sample * IO_NR51.wave_left) + (noise_sample * IO_NR51.noise_left)) / 4) * CONTROL_CHANNEL.nr50.left_vol;
    const GB_S8 final_sample_right = (((square1_sample * IO_NR51.square1_right) + (square2_sample * IO_NR51.square2_right) + (wave_sample * IO_NR51.wave_right) + (noise_sample * IO_NR51.noise_right)) / 4) * CONTROL_CHANNEL.nr50.right_vol;
#elif MODE == 1
    GB_UNUSED(wave_sample);
    const GB_S8 final_sample_left = (((square1_sample * IO_NR51.square1_left) + (square2_sample * IO_NR51.square2_left) + (noise_sample * IO_NR51.noise_left)) / 3) * CONTROL_CHANNEL.nr50.left_vol;
    const GB_S8 final_sample_right = (((square1_sample * IO_NR51.square1_right) + (square2_sample * IO_NR51.square2_right) + (noise_sample * IO_NR51.noise_right)) / 3) * CONTROL_CHANNEL.nr50.right_vol;
#elif MODE == 2
    GB_UNUSED(wave_sample);
    GB_UNUSED(noise_sample);
    const GB_S8 final_sample_left = (((square1_sample * IO_NR51.square1_left) + (square2_sample * IO_NR51.square2_left)) / 2) * CONTROL_CHANNEL.nr50.left_vol;
    const GB_S8 final_sample_right = (((square1_sample * IO_NR51.square1_right) + (square2_sample * IO_NR51.square2_right)) / 2) * CONTROL_CHANNEL.nr50.right_vol;
#endif
#undef MODE

    // store the samples in the buffer, remember this is stereo!
    gb->apu.samples[gb->apu.samples_count + 0] = final_sample_left;
    gb->apu.samples[gb->apu.samples_count + 1] = final_sample_right;

    // we stored 2 samples (left and right for stereo)
    gb->apu.samples_count += 2;

    // check if we filled the buffer
    if (gb->apu.samples_count >= sizeof(gb->apu.samples)) {
        gb->apu.samples_count -= sizeof(gb->apu.samples);
        
        // make sure the frontend did set a callback!
        if (gb->apu_cb != NULL) {

            // send over the data!
            struct GB_ApuCallbackData data;
            memcpy(data.samples, gb->apu.samples, sizeof(gb->apu.samples));
            gb->apu_cb(gb, gb->apu_cb_user_data, &data);
        }
    }
}

#define TEST (96000 / 2048)

#ifdef GB_SDL_AUDIO_CALLBACK
void GB_SDL_audio_callback(struct GB_Core* gb, GB_S8* buf, int len) {

    for (int i = 0; i < len; i += 2) {
        gb->apu.next_frame_sequencer_cycles += 1;
        if (gb->apu.next_frame_sequencer_cycles >= (48000)) {
            gb->apu.next_frame_sequencer_cycles -= (48000);
            step_frame_sequencer(gb);
        }

        SQUARE1_CHANNEL.timer -= 100;
        if (SQUARE1_CHANNEL.timer <= 0) {
            SQUARE1_CHANNEL.timer = get_square1_freq(gb);
            ++SQUARE1_CHANNEL.duty_index;
        }

        SQUARE2_CHANNEL.timer -= 100;
        if (SQUARE2_CHANNEL.timer <= 0) {
            SQUARE2_CHANNEL.timer = get_square2_freq(gb);
            ++SQUARE2_CHANNEL.duty_index;
        }

        WAVE_CHANNEL.timer -= 100;
        if (WAVE_CHANNEL.timer <= 0) {
            WAVE_CHANNEL.timer = get_wave_freq(gb);
            advance_wave_position_counter(gb);
        }

        NOISE_CHANNEL.timer -= 100;
        if (NOISE_CHANNEL.timer <= 0) {
            NOISE_CHANNEL.timer = get_noise_freq(gb);
            step_noise_lfsr(gb);
        }

        const GB_S8 square1_sample = sample_square1(gb) * is_square1_enabled(gb);
        const GB_S8 square2_sample = sample_square2(gb) * is_square2_enabled(gb);
        const GB_S8 wave_sample = sample_wave(gb) * is_wave_enabled(gb);
        const GB_S8 noise_sample = sample_noise(gb) * is_noise_enabled(gb);

        const GB_S8 final_sample_left = (((square1_sample * IO_NR51.square1_left) + (square2_sample * IO_NR51.square2_left) + (wave_sample * IO_NR51.wave_left) + (noise_sample * IO_NR51.noise_left))/4) * CONTROL_CHANNEL.nr50.left_vol;
        const GB_S8 final_sample_right = (((square1_sample * IO_NR51.square1_right) + (square2_sample * IO_NR51.square2_right) + (wave_sample * IO_NR51.wave_right) + (noise_sample * IO_NR51.noise_right))/4) * CONTROL_CHANNEL.nr50.right_vol;

        buf[i] = final_sample_left;
        buf[i + 1] = final_sample_right;
    }
}
#endif // GB_SDL_AUDIO_CALLBACK

void GB_apu_run(struct GB_Core* gb, GB_U16 cycles) {
    // for debug...
    static int init_start = 0;
    if (init_start == 0) {
        SQUARE1_CHANNEL.timer = 2048;
        SQUARE2_CHANNEL.timer = 2048;
        WAVE_CHANNEL.timer = 2048;
        ++init_start;
    }
    
#ifndef GB_SDL_AUDIO_CALLBACK
    // tick all the channels!

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

    NOISE_CHANNEL.timer -= cycles;
    if (NOISE_CHANNEL.timer <= 0) {
        NOISE_CHANNEL.timer = get_noise_freq(gb);
        step_noise_lfsr(gb);
    }

    // check if we need to tick the frame sequencer!
    gb->apu.next_frame_sequencer_cycles += cycles;
    if (gb->apu.next_frame_sequencer_cycles >= FRAME_SEQUENCER_STEP_RATE) {
        gb->apu.next_frame_sequencer_cycles -= FRAME_SEQUENCER_STEP_RATE;
        step_frame_sequencer(gb);
    }

    // see if it's time to create a new sample!
    gb->apu.next_sample_cycles += cycles;
    if (gb->apu.next_sample_cycles >= SAMPLE_RATE) {
        gb->apu.next_sample_cycles -= SAMPLE_RATE;
        sample_channels(gb);
    }
#endif // GB_SDL_AUDIO_CALLBACK
}

// IO read / writes
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
                on_square1_trigger(gb);
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
                on_square2_trigger(gb);
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
            IO_WAVE_TABLE[WAVE_CHANNEL.position_counter >> 1] = value;
            break;
	}
}
