#include "../internal.h"
#include "../gb.h"
#include "apu.h"

#include <string.h>


const bool SQUARE_DUTY_CYCLES[4][8] =
{
    [0] = { 0, 0, 0, 0, 0, 0, 0, 1 },
    [1] = { 1, 0, 0, 0, 0, 0, 0, 1 },
    [2] = { 0, 0, 0, 0, 0, 1, 1, 1 },
    [3] = { 0, 1, 1, 1, 1, 1, 1, 0 },
};

const uint8_t PERIOD_TABLE[8] = { 8, 1, 2, 3, 4, 5, 6, 7 };


static FORCE_INLINE uint8_t volume_left(const struct GB_Core* gb)
{
    return (IO_NR50 >> 4) & 0x7;
}

static FORCE_INLINE uint8_t volume_right(const struct GB_Core* gb)
{
    return (IO_NR50 >> 0) & 0x7;
}

static FORCE_INLINE bool ch1_left_output(const struct GB_Core* gb)
{
    return (IO_NR51 >> 4) & 0x1;
}

static FORCE_INLINE bool ch2_left_output(const struct GB_Core* gb)
{
    return (IO_NR51 >> 5) & 0x1;
}

static FORCE_INLINE bool ch3_left_output(const struct GB_Core* gb)
{
    return (IO_NR51 >> 6) & 0x1;
}

static FORCE_INLINE bool ch4_left_output(const struct GB_Core* gb)
{
    return (IO_NR51 >> 7) & 0x1;
}

static FORCE_INLINE bool ch1_right_output(const struct GB_Core* gb)
{
    return (IO_NR51 >> 0) & 0x1;
}

static FORCE_INLINE bool ch2_right_output(const struct GB_Core* gb)
{
    return (IO_NR51 >> 1) & 0x1;
}

static FORCE_INLINE bool ch3_right_output(const struct GB_Core* gb)
{
    return (IO_NR51 >> 2) & 0x1;
}

static FORCE_INLINE bool ch4_right_output(const struct GB_Core* gb)
{
    return (IO_NR51 >> 3) & 0x1;
}

static FORCE_INLINE void clock_len(struct GB_Core* gb)
{
    // docs say that this is always clocked, even if channel is disabled
    clock_ch1_len(gb);
    clock_ch2_len(gb);
    clock_ch3_len(gb);
    clock_ch4_len(gb);
}

static FORCE_INLINE void clock_sweep(struct GB_Core* gb)
{
    if (is_ch1_enabled(gb)) { on_ch1_sweep(gb); }
}

static FORCE_INLINE void clock_vol(struct GB_Core* gb)
{
    if (is_ch1_enabled(gb)) { clock_ch1_vol(gb); }
    if (is_ch2_enabled(gb)) { clock_ch2_vol(gb); }
    if (is_ch4_enabled(gb)) { clock_ch4_vol(gb); }
}

bool gb_is_apu_enabled(const struct GB_Core* gb)
{
    return (IO_NR52 & 0x80) > 0;
}

void gb_apu_on_enabled(struct GB_Core* gb)
{
    GB_log("[APU] enabling...\n");

    CH1.duty = 0;
    CH1.duty_index = 0;

    CH2.duty = 0;
    CH2.duty_index = 0;
}

void gb_apu_on_disabled(struct GB_Core* gb)
{
    GB_log("[APU] disabling...\n");

    gb->apu.frame_sequencer_counter = 0;

    IO_NR52 &= ~0xF;

    memset(IO + 0x10, 0x00, 0x15);

    memset(&CH1, 0, sizeof(CH1));
    memset(&CH2, 0, sizeof(CH2));
    memset(&CH3, 0, sizeof(CH3));

    memset(&IO_NR10, 0, sizeof(IO_NR10));

    memset(&IO_NR11, 0, sizeof(IO_NR11));
    memset(&IO_NR12, 0, sizeof(IO_NR12));
    memset(&IO_NR13, 0, sizeof(IO_NR13));
    memset(&IO_NR14, 0, sizeof(IO_NR14));

    memset(&IO_NR21, 0, sizeof(IO_NR21));
    memset(&IO_NR22, 0, sizeof(IO_NR22));
    memset(&IO_NR23, 0, sizeof(IO_NR23));
    memset(&IO_NR22, 0, sizeof(IO_NR22));

    memset(&IO_NR41, 0, sizeof(IO_NR41));
    memset(&IO_NR42, 0, sizeof(IO_NR42));
    memset(&IO_NR43, 0, sizeof(IO_NR43));
    memset(&IO_NR44, 0, sizeof(IO_NR44));

    memset(&IO_NR50, 0, sizeof(IO_NR50));
    memset(&IO_NR51, 0, sizeof(IO_NR51));
}

// this is used when a channel is triggered
bool is_next_frame_sequencer_step_not_len(const struct GB_Core* gb)
{
    // check if the current counter is the len clock, the next one won't be!
    return gb->apu.frame_sequencer_counter == 1 || gb->apu.frame_sequencer_counter == 3 || gb->apu.frame_sequencer_counter == 5 || gb->apu.frame_sequencer_counter == 7;
}

// this is used when channels 1,2,4 are triggered
bool is_next_frame_sequencer_step_vol(const struct GB_Core* gb)
{
    // check if the current counter is the len clock, the next one won't be!
    return gb->apu.frame_sequencer_counter == 7;
}

// this is clocked by DIV
void step_frame_sequencer(struct GB_Core* gb)
{
    if (!gb_is_apu_enabled(gb))
    {
        return;
    }

    switch (gb->apu.frame_sequencer_counter)
    {
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

    gb->apu.frame_sequencer_counter = (gb->apu.frame_sequencer_counter + 1) % 8;
}

struct MixerSampleData
{
    int8_t sample;
    bool left;
    bool right;
};

struct MixerData
{
    struct MixerSampleData ch1;
    struct MixerSampleData ch2;
    struct MixerSampleData ch3;
    struct MixerSampleData ch4;

    int8_t left_amp, right_amp;
};

static FORCE_INLINE struct GB_ApuCallbackData mixer(const struct MixerData* data)
{
    enum { LEFT, RIGHT };

    return (struct GB_ApuCallbackData)
    {
        .ch1[LEFT] = data->ch1.sample * data->ch1.left,
        .ch1[RIGHT] = data->ch1.sample * data->ch1.right,
        .ch2[LEFT] = data->ch2.sample * data->ch2.left,
        .ch2[RIGHT] = data->ch2.sample * data->ch2.right,
        .ch3[LEFT] = data->ch3.sample * data->ch3.left,
        .ch3[RIGHT] = data->ch3.sample * data->ch3.right,
        .ch4[LEFT] = data->ch4.sample * data->ch4.left,
        .ch4[RIGHT] = data->ch4.sample * data->ch4.right,
        .left_amp = data->left_amp,
        .right_amp = data->right_amp,
    };
}

static FORCE_INLINE void sample_channels(struct GB_Core* gb)
{
    // build up data for the mixer!
    struct GB_ApuCallbackData samples = {0};

    if (gb_is_apu_enabled(gb))
    {
        // build up data for the mixer!
        const struct MixerData mixer_data =
        {
            .ch1 =
            {
                .sample = sample_ch1(gb) * is_ch1_enabled(gb),
                .left = ch1_left_output(gb),
                .right = ch1_right_output(gb)
            },
            .ch2 =
            {
                .sample = sample_ch2(gb) * is_ch2_enabled(gb),
                .left = ch2_left_output(gb),
                .right = ch2_right_output(gb)
            },
            .ch3 =
            {
                .sample = sample_ch3(gb) * is_ch3_enabled(gb),
                .left = ch3_left_output(gb),
                .right = ch3_right_output(gb)
            },
            .ch4 =
            {
                .sample = sample_ch4(gb) * is_ch4_enabled(gb),
                .left = ch4_left_output(gb),
                .right = ch4_right_output(gb)
            },
            .left_amp = volume_left(gb) + 1,
            .right_amp = volume_right(gb) + 1,
        };

        samples = mixer(&mixer_data);
    }

    if (LIKELY(gb->callback.apu != NULL))
    {
        gb->callback.apu(gb->callback.user_apu, &samples);
    }
}

void GB_apu_run(struct GB_Core* gb, uint16_t cycles)
{
    if (LIKELY(gb_is_apu_enabled(gb)))
    {
        // still tick samples but fill empty
        // nothing else should tick i dont think?
        // not sure if when apu is disabled, do all regs reset?
        // what happens when apu is re-enabled? do they all trigger?

        if (LIKELY(CH1.timer > 0 || get_ch1_freq(gb)))
        {
            CH1.timer -= cycles;
            if (UNLIKELY(CH1.timer <= 0))
            {
                CH1.timer += get_ch1_freq(gb);
                CH1.duty_index = (CH1.duty_index + 1) % 8;
            }
        }

        if (LIKELY(CH2.timer > 0 || get_ch2_freq(gb)))
        {
            CH2.timer -= cycles;
            if (UNLIKELY(CH2.timer <= 0))
            {
                CH2.timer += get_ch2_freq(gb);
                CH2.duty_index = (CH2.duty_index + 1) % 8;
            }
        }

        if (LIKELY(CH3.timer > 0 || get_ch3_freq(gb)))
        {
            CH3.timer -= cycles;
            if (UNLIKELY(CH3.timer <= 0))
            {
                CH3.timer += get_ch3_freq(gb);
                advance_ch3_position_counter(gb);
            }
        }

        // NOTE: ch4 lfsr is ONLY clocked if clock shift is not 14 or 15
        if (IO_NR43.clock_shift != 14 && IO_NR43.clock_shift != 15)
        {
            if (LIKELY(CH4.timer > 0 || get_ch4_freq(gb)))
            {
                CH4.timer -= cycles;
                if (UNLIKELY(CH4.timer <= 0))
                {
                    CH4.timer += get_ch4_freq(gb);
                    step_ch4_lfsr(gb);
                }
            }
        }
    }

    // we should still sample even if the apu is disabled
    // in this case, the samples are filled with 0.
    if (LIKELY(gb->callback.apu_data.freq_reload))
    {
        gb->apu.next_sample_cycles += cycles;

        if (gb->apu.next_sample_cycles >= gb->callback.apu_data.freq_reload)
        {
            gb->apu.next_sample_cycles -= gb->callback.apu_data.freq_reload;
            sample_channels(gb);
        }
    }
}
