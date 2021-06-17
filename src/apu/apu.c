#include "../internal.h"
#include "../gb.h"
#include "apu.h"

#include <string.h>


const int8_t SQUARE_DUTY_CYCLES[4][8] =
{
    [0] = { -1, -1, -1, -1, -1, -1, -1, +1 },
    [1] = { +1, -1, -1, -1, -1, -1, -1, +1 },
    [2] = { -1, -1, -1, -1, -1, +1, +1, +1 },
    [3] = { -1, +1, +1, +1, +1, +1, +1, -1 },
};

const uint8_t PERIOD_TABLE[8] = { 8, 1, 2, 3, 4, 5, 6, 7 };


static inline uint8_t volume_left(const struct GB_Core* gb)
{
    return (IO_NR50 >> 4) & 0x7;
}

static inline uint8_t volume_right(const struct GB_Core* gb)
{
    return (IO_NR50 >> 0) & 0x7;
}

static inline bool ch1_left_output(const struct GB_Core* gb)
{
    return (IO_NR51 >> 4) & 0x1;
}

static inline bool ch2_left_output(const struct GB_Core* gb)
{
    return (IO_NR51 >> 5) & 0x1;
}

static inline bool ch3_left_output(const struct GB_Core* gb)
{
    return (IO_NR51 >> 6) & 0x1;
}

static inline bool ch4_left_output(const struct GB_Core* gb)
{
    return (IO_NR51 >> 7) & 0x1;
}

static inline bool ch1_right_output(const struct GB_Core* gb)
{
    return (IO_NR51 >> 0) & 0x1;
}

static inline bool ch2_right_output(const struct GB_Core* gb)
{
    return (IO_NR51 >> 1) & 0x1;
}

static inline bool ch3_right_output(const struct GB_Core* gb)
{
    return (IO_NR51 >> 2) & 0x1;
}

static inline bool ch4_right_output(const struct GB_Core* gb)
{
    return (IO_NR51 >> 3) & 0x1;
}

static inline void clock_len(struct GB_Core* gb)
{
    clock_ch1_len(gb);
    clock_ch2_len(gb);
    clock_ch3_len(gb);
    clock_ch4_len(gb);
}

static inline void clock_sweep(struct GB_Core* gb)
{
    on_ch1_sweep(gb);
}

static inline void clock_vol(struct GB_Core* gb)
{
    clock_ch1_vol(gb);
    clock_ch2_vol(gb);
    clock_ch4_vol(gb);
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

    IO_NR52 &= ~0xF;

    // not sure if this should be 0xFF or 0x00 on reset
    #if 1
    memset(IO + 0x10, 0x00, 0x15);
    #else
    memset(IO + 0x10, 0xFF, 0x15);
    #endif

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

// this runs at 512hz
static inline void step_frame_sequencer(struct GB_Core* gb)
{
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
    int8_t left;
    int8_t right;
};

struct MixerData
{
    struct MixerSampleData ch1;
    struct MixerSampleData ch2;
    struct MixerSampleData ch3;
    struct MixerSampleData ch4;

    int8_t left_master, right_master;
};

static inline struct GB_ApuCallbackData mixer(const struct MixerData* data)
{
    enum { LEFT, RIGHT };

    return (struct GB_ApuCallbackData)
    {
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

static inline void sample_channels(struct GB_Core* gb)
{
    // check if we have any callbacks set, if not, avoid
    // doing all the hardwork below!
    if (gb->callback.apu == NULL)
    {
        return;
    }

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
        .left_master = volume_left(gb),
        .right_master = volume_right(gb)
    };

    struct GB_ApuCallbackData samples = mixer(&mixer_data);

    gb->callback.apu(gb->callback.user_apu, &samples);
}

void GB_apu_on_ch1_event(struct GB_Core* gb)
{
    CH1.duty_index = (CH1.duty_index + 1) % 8;
    GB_add_event(gb, GB_EventType_APU_CH1, get_ch1_freq(gb));
}

void GB_apu_on_ch2_event(struct GB_Core* gb)
{
    CH2.duty_index = (CH2.duty_index + 1) % 8;
    GB_add_event(gb, GB_EventType_APU_CH2, get_ch2_freq(gb));
}

void GB_apu_on_ch3_event(struct GB_Core* gb)
{
    advance_ch3_position_counter(gb);
    GB_add_event(gb, GB_EventType_APU_CH3, get_ch3_freq(gb));
}

void GB_apu_on_ch4_event(struct GB_Core* gb)
{
    step_ch4_lfsr(gb);
    GB_add_event(gb, GB_EventType_APU_CH4, get_ch4_freq(gb));
}

void GB_apu_on_frame_sequencer_event(struct GB_Core* gb)
{
    step_frame_sequencer(gb);
    GB_add_event(gb, GB_EventType_APU_FRAME_SEQUENCER, FRAME_SEQUENCER_STEP_RATE);
}

void GB_apu_on_sample_event(struct GB_Core* gb)
{
    sample_channels(gb);
    GB_add_event(gb, GB_EventType_APU_SAMPLE, CALC_CALLBACK_FREQ(gb->callback.apu_data.freq));
}

void GB_apu_init(struct GB_Core* gb)
{
    GB_add_event(gb, GB_EventType_APU_CH1, get_ch1_freq(gb));
    GB_add_event(gb, GB_EventType_APU_CH2, get_ch2_freq(gb));
    GB_add_event(gb, GB_EventType_APU_CH3, get_ch3_freq(gb));
    GB_add_event(gb, GB_EventType_APU_CH4, get_ch4_freq(gb));
    GB_add_event(gb, GB_EventType_APU_FRAME_SEQUENCER, FRAME_SEQUENCER_STEP_RATE);
    GB_add_event(gb, GB_EventType_APU_SAMPLE, CALC_CALLBACK_FREQ(gb->callback.apu_data.freq));
}
