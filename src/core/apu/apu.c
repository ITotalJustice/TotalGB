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

// this is used when a channel is triggered
bool is_next_frame_suqencer_step_not_len(const struct GB_Core* gb) {
    // check if the current counter is the len clock, the next one won't be!
    return gb->apu.frame_sequencer_counter == 0 || gb->apu.frame_sequencer_counter == 2 || gb->apu.frame_sequencer_counter == 4 || gb->apu.frame_sequencer_counter == 6;
}

// this is used when channels 1,2,4 are triggered
bool is_next_frame_suqencer_step_vol(const struct GB_Core* gb) {
    // check if the current counter is the len clock, the next one won't be!
    return gb->apu.frame_sequencer_counter == 6;
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

static inline struct GB_MixerResult builtin_mixer(const struct GB_MixerData* data) {
    return (struct GB_MixerResult){
        .left = ((
            (data->square1.sample * data->square1.left) +
            (data->square2.sample * data->square2.left) +
            (data->wave.sample * data->wave.left) +
            (data->noise.sample * data->noise.left)
        ) / 4) * data->left_master,

        .right = ((
            (data->square1.sample * data->square1.right) +
            (data->square2.sample * data->square2.right) +
            (data->wave.sample * data->wave.right) +
            (data->noise.sample * data->noise.right)
        ) / 4) * data->right_master,
    };
}

static void sample_channels(struct GB_Core* gb) {

    // build up data for the mixer!
    const struct GB_MixerData mixer_data = {
        .square1 = {
            .sample = sample_square1(gb) * is_square1_enabled(gb),
            .left = IO_NR51.square1_left,
            .right = IO_NR51.square1_right
        },
        .square2 = {
            .sample = sample_square2(gb) * is_square2_enabled(gb),
            .left = IO_NR51.square2_left,
            .right = IO_NR51.square2_right
        },
        .wave = {
            .sample = sample_wave(gb) * is_wave_enabled(gb),
            .left = IO_NR51.wave_left,
            .right = IO_NR51.wave_right
        },
        .noise = {
            .sample = sample_noise(gb) * is_noise_enabled(gb),
            .left = IO_NR51.noise_left,
            .right = IO_NR51.noise_right
        },
        .left_master = CONTROL_CHANNEL.nr50.left_vol,
        .right_master = CONTROL_CHANNEL.nr50.right_vol
    };

    if (gb->mixer_cb != NULL) {
        const struct GB_MixerResult r = gb->mixer_cb(gb, gb->mixer_cb_user_data, &mixer_data);
        gb->apu.samples.samples[gb->apu.samples_count + 0] = r.left;
        gb->apu.samples.samples[gb->apu.samples_count + 1] = r.right;
    }

    else {
        const struct GB_MixerResult r = builtin_mixer(&mixer_data);
        gb->apu.samples.samples[gb->apu.samples_count + 0] = r.left;
        gb->apu.samples.samples[gb->apu.samples_count + 1] = r.right;
    }

    // we stored 2 samples (left and right for stereo)
    gb->apu.samples_count += 2;

    // check if we filled the buffer
    if (gb->apu.samples_count >= sizeof(gb->apu.samples.samples)) {
        gb->apu.samples_count = 0;

        // make sure the frontend did set a callback!
        if (gb->apu_cb != NULL) {
            gb->apu_cb(gb, gb->apu_cb_user_data, &gb->apu.samples);
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

        // NOTE: noise lfsr is ONLY clocked if clock shift is not 14 or 15
        if (IO_NR43.clock_shift != 14 && IO_NR43.clock_shift != 15) {
            NOISE_CHANNEL.timer -= cycles;
            while (NOISE_CHANNEL.timer <= 0) {
                NOISE_CHANNEL.timer += get_noise_freq(gb);
                step_noise_lfsr(gb);
            }
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
