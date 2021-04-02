#include "core/gb.h"
#include "core/internal.h"
#include "core/apu/apu.h"
#include "core/apu/common.h"

#include <stdio.h>
#include <string.h>


const bool SQUARE_DUTY_CYCLES[4][8] = {
    [0] = { 0, 0, 0, 0, 0, 0, 0, 1 },
    [1] = { 1, 0, 0, 0, 0, 0, 0, 1 },
    [2] = { 0, 0, 0, 0, 0, 1, 1, 1 },
    [3] = { 0, 1, 1, 1, 1, 1, 1, 0 },
};

const uint8_t PERIOD_TABLE[8] = {
    8, 1, 2, 3, 4, 5, 6, 7,
};


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
    const int8_t square1_sample = sample_square1(gb) * is_square1_enabled(gb);
    const int8_t square2_sample = sample_square2(gb) * is_square2_enabled(gb);
    const int8_t wave_sample = sample_wave(gb) * is_wave_enabled(gb);
    const int8_t noise_sample = sample_noise(gb) * is_noise_enabled(gb);

#ifdef CHANNEL_8
    gb->apu.samples[gb->apu.samples_count + 0] = square1_sample * IO_NR51.square1_left * CONTROL_CHANNEL.nr50.left_vol;
    gb->apu.samples[gb->apu.samples_count + 1] = square1_sample * IO_NR51.square1_right * CONTROL_CHANNEL.nr50.right_vol;

    gb->apu.samples[gb->apu.samples_count + 2] = square2_sample * IO_NR51.square2_left * CONTROL_CHANNEL.nr50.left_vol;
    gb->apu.samples[gb->apu.samples_count + 3] = square2_sample * IO_NR51.square2_right * CONTROL_CHANNEL.nr50.right_vol;

    gb->apu.samples[gb->apu.samples_count + 4] = wave_sample * IO_NR51.wave_left * CONTROL_CHANNEL.nr50.left_vol;
    gb->apu.samples[gb->apu.samples_count + 5] = wave_sample * IO_NR51.wave_right * CONTROL_CHANNEL.nr50.right_vol;

    gb->apu.samples[gb->apu.samples_count + 6] = noise_sample * IO_NR51.noise_left * CONTROL_CHANNEL.nr50.left_vol;
    gb->apu.samples[gb->apu.samples_count + 7] = noise_sample * IO_NR51.noise_right * CONTROL_CHANNEL.nr50.right_vol;

    gb->apu.samples_count += 8;
#else

// for testing, test all channels VS test everything but wave channel...
#define MODE 0
#if MODE == 0
    const int8_t final_sample_left = (((square1_sample * IO_NR51.square1_left) + (square2_sample * IO_NR51.square2_left) + (wave_sample * IO_NR51.wave_left) + (noise_sample * IO_NR51.noise_left)) / 4) * CONTROL_CHANNEL.nr50.left_vol;
    const int8_t final_sample_right = (((square1_sample * IO_NR51.square1_right) + (square2_sample * IO_NR51.square2_right) + (wave_sample * IO_NR51.wave_right) + (noise_sample * IO_NR51.noise_right)) / 4) * CONTROL_CHANNEL.nr50.right_vol;
#elif MODE == 1
    GB_UNUSED(wave_sample);
    const int8_t final_sample_left = (((square1_sample * IO_NR51.square1_left) + (square2_sample * IO_NR51.square2_left) + (noise_sample * IO_NR51.noise_left)) / 3) * CONTROL_CHANNEL.nr50.left_vol;
    const int8_t final_sample_right = (((square1_sample * IO_NR51.square1_right) + (square2_sample * IO_NR51.square2_right) + (noise_sample * IO_NR51.noise_right)) / 3) * CONTROL_CHANNEL.nr50.right_vol;
#elif MODE == 2
    GB_UNUSED(wave_sample);
    GB_UNUSED(noise_sample);
    const int8_t final_sample_left = (((square1_sample * IO_NR51.square1_left) + (square2_sample * IO_NR51.square2_left)) / 2) * CONTROL_CHANNEL.nr50.left_vol;
    const int8_t final_sample_right = (((square1_sample * IO_NR51.square1_right) + (square2_sample * IO_NR51.square2_right)) / 2) * CONTROL_CHANNEL.nr50.right_vol;
#elif MODE == 3
    GB_UNUSED(square1_sample);
    GB_UNUSED(square2_sample);
    GB_UNUSED(noise_sample);
    const int8_t final_sample_left = (wave_sample * IO_NR51.wave_left) * CONTROL_CHANNEL.nr50.left_vol;
    const int8_t final_sample_right = (wave_sample * IO_NR51.wave_right) * CONTROL_CHANNEL.nr50.right_vol;
#endif
#undef MODE

    // store the samples in the buffer, remember this is stereo!
    gb->apu.samples[gb->apu.samples_count + 0] = final_sample_left;
    gb->apu.samples[gb->apu.samples_count + 1] = final_sample_right;

    // we stored 2 samples (left and right for stereo)
    gb->apu.samples_count += 2;

#endif // #ifdef CHANNEL_8

    // check if we filled the buffer
    if (gb->apu.samples_count >= sizeof(gb->apu.samples)) {
        gb->apu.samples_count =0;//-= sizeof(gb->apu.samples);
        
        // make sure the frontend did set a callback!
        if (gb->apu_cb != NULL) {

            // send over the data!
            struct GB_ApuCallbackData data;
            // we don't really have to memcpy here
            // can just pass a const pointer and size instead...
            memcpy(data.samples, gb->apu.samples, sizeof(gb->apu.samples));
            gb->apu_cb(gb, gb->apu_cb_user_data, &data);
        }
    }
}

void GB_apu_run(struct GB_Core* gb, uint16_t cycles) {
    // todo: handle if the apu is disabled!
    if (IO_NR52.power != 0) {
        // still tick samples but fill empty
        // nothing else should tick i dont think?
        // not sure if when apu is disabled, do all regs reset?
        // what happens when apu is re-enabled? do they all trigger?
        SQUARE1_CHANNEL.timer -= cycles;
        while (SQUARE1_CHANNEL.timer <= 0) {
            SQUARE1_CHANNEL.timer += get_square1_freq(gb);
            ++SQUARE1_CHANNEL.duty_index;
        }

        SQUARE2_CHANNEL.timer -= cycles;
        while (SQUARE2_CHANNEL.timer <= 0) {
            SQUARE2_CHANNEL.timer += get_square2_freq(gb);
            ++SQUARE2_CHANNEL.duty_index;
        }

        WAVE_CHANNEL.timer -= cycles;
        while (WAVE_CHANNEL.timer <= 0) {
            WAVE_CHANNEL.timer += get_wave_freq(gb);
            advance_wave_position_counter(gb);
        }

        NOISE_CHANNEL.timer -= cycles;
        while (NOISE_CHANNEL.timer <= 0) {
            NOISE_CHANNEL.timer += get_noise_freq(gb);
            step_noise_lfsr(gb);
        }

        // check if we need to tick the frame sequencer!
        gb->apu.next_frame_sequencer_cycles += cycles;
        while (gb->apu.next_frame_sequencer_cycles >= FRAME_SEQUENCER_STEP_RATE) {
            gb->apu.next_frame_sequencer_cycles -= FRAME_SEQUENCER_STEP_RATE;
            step_frame_sequencer(gb);
        }

        // see if it's time to create a new sample!
        gb->apu.next_sample_cycles += cycles;
        while (gb->apu.next_sample_cycles >= SAMPLE_RATE) {
            gb->apu.next_sample_cycles -= SAMPLE_RATE;
            sample_channels(gb);
        }
    }
    
    else {
        // we should still sample even if the apu is disabled
        // in this case, the samples are filled with 0.

        // this can slightly optimised by just filling that sample with
        // a fixed silence value, rather than fake-creating samples for
        // no reason.
        gb->apu.next_sample_cycles += cycles;
        while (gb->apu.next_sample_cycles >= SAMPLE_RATE) {
            gb->apu.next_sample_cycles -= SAMPLE_RATE;
            sample_channels(gb);
        }
    }
}
