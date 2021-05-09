#include "../internal.h"
#include "../gb.h"
#include "apu.h"

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
bool is_next_frame_sequencer_step_not_len(const struct GB_Core* gb) {
    // check if the current counter is the len clock, the next one won't be!
    return gb->apu.frame_sequencer_counter == 0 || gb->apu.frame_sequencer_counter == 2 || gb->apu.frame_sequencer_counter == 4 || gb->apu.frame_sequencer_counter == 6;
}

// this is used when channels 1,2,4 are triggered
bool is_next_frame_sequencer_step_vol(const struct GB_Core* gb) {
    // check if the current counter is the len clock, the next one won't be!
    return gb->apu.frame_sequencer_counter == 6;
}

// this runs at 512hz
static inline void step_frame_sequencer(struct GB_Core* gb) {
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

struct MixerResult {
    int8_t ch1[2];
    int8_t ch2[2];
    int8_t ch3[2];
    int8_t ch4[2];
};

struct MixerSampleData {
    int8_t sample;
    int8_t left;
    int8_t right;
};

struct MixerData {
    struct MixerSampleData ch1;
    struct MixerSampleData ch2;
    struct MixerSampleData ch3;
    struct MixerSampleData ch4;

    int8_t left_master, right_master;
};

static inline struct MixerResult mixer(const struct MixerData* data) {
    enum { LEFT, RIGHT };

    return (struct MixerResult){
        .ch1[LEFT] = data->ch1.sample * data->ch1.left * data->left_master,
        .ch1[RIGHT] = data->ch1.sample * data->ch1.right * data->right_master,
        .ch2[LEFT] = data->ch2.sample * data->ch2.left * data->left_master,
        .ch2[RIGHT] = data->ch2.sample * data->ch2.right * data->right_master,
        .ch3[LEFT] = data->ch3.sample * data->ch3.left * data->left_master,
        .ch3[RIGHT] = data->ch3.sample * data->ch3.right * data->right_master,
        .ch4[LEFT] = data->ch4.sample * data->ch4.left * data->left_master,
        .ch4[RIGHT] = data->ch4.sample * data->ch4.right * data->right_master,
    };
}

static inline void sample_channels(struct GB_Core* gb) {
    // check if we have any callbacks set, if not, avoid
    // doing all the hardwork below!
    if (gb->apu_cb == NULL) {
        return;
    }

    // build up data for the mixer!
    const struct MixerData mixer_data = {
        .ch1 = {
            .sample = sample_square1(gb) * is_square1_enabled(gb),
            .left = IO_NR51.square1_left,
            .right = IO_NR51.square1_right
        },
        .ch2 = {
            .sample = sample_square2(gb) * is_square2_enabled(gb),
            .left = IO_NR51.square2_left,
            .right = IO_NR51.square2_right
        },
        .ch3 = {
            .sample = sample_wave(gb) * is_wave_enabled(gb),
            .left = IO_NR51.wave_left,
            .right = IO_NR51.wave_right
        },
        .ch4 = {
            .sample = sample_noise(gb) * is_noise_enabled(gb),
            .left = IO_NR51.noise_left,
            .right = IO_NR51.noise_right
        },
        .left_master = CONTROL_CHANNEL.nr50.left_vol,
        .right_master = CONTROL_CHANNEL.nr50.right_vol
    };

    const struct MixerResult r = mixer(&mixer_data);

    struct GB_ApuCallbackData* samples = &gb->apu.samples;
    const uint32_t sample_count = gb->apu.samples_count;

    samples->data.buffers.ch1[sample_count + 0] = r.ch1[0];
    samples->data.buffers.ch1[sample_count + 1] = r.ch1[1];
    samples->data.buffers.ch2[sample_count + 0] = r.ch2[0];
    samples->data.buffers.ch2[sample_count + 1] = r.ch2[1];
    samples->data.buffers.ch3[sample_count + 0] = r.ch3[0];
    samples->data.buffers.ch3[sample_count + 1] = r.ch3[1];
    samples->data.buffers.ch4[sample_count + 0] = r.ch4[0];
    samples->data.buffers.ch4[sample_count + 1] = r.ch4[1];

    // fill the samples based on the mode!
    switch (gb->apu.sample_mode) {
        case AUDIO_CALLBACK_FILL_SAMPLES:
            gb->apu.samples_count += 2;

            // check if we filled the buffer
            if (gb->apu.samples_count >= 512*2) {
                gb->apu.samples_count = 0;
                gb->apu_cb(gb, gb->apu_cb_user_data, samples);
            }
            break;

        case AUDIO_CALLBACK_PUSH_ALL:
            gb->apu_cb(gb, gb->apu_cb_user_data, samples);
            break;
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
    }

    // we should still sample even if the apu is disabled
    // in this case, the samples are filled with 0.

    // this can slightly optimised by just filling that sample with
    // a fixed silence value, rather than fake-creating samples for
    // no reason.

    switch (gb->apu.sample_mode) {
        case AUDIO_CALLBACK_FILL_SAMPLES:
            gb->apu.next_sample_cycles += cycles;
            while (gb->apu.next_sample_cycles >= SAMPLE_RATE) {
                gb->apu.next_sample_cycles -= SAMPLE_RATE;
                sample_channels(gb);
            }
            break;

        case AUDIO_CALLBACK_PUSH_ALL:
            gb->apu.next_sample_cycles += cycles;
            while (gb->apu.next_sample_cycles >= 8) {
                gb->apu.next_sample_cycles -= 8;
                sample_channels(gb);
            }
            break;
    }
}
