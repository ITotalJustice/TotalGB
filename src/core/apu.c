#include "gb.h"
#include "internal.h"

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

/* square 1 */
#define NR10_SWEEP 0x70
#define NR10_NEGATE 0x08
#define NR10_SHIFT 0x07
#define NR11_DUTY 0xC0
#define NR11_LENGTH_LOAD 0x3F
#define NR12_STARTING_VOLUME 0xF0
#define NR12_ENVELOPE_ADD_MODE 0x08
#define NR12_PERIOD 0x07
#define NR13_FREQ_LSB 0xFF
#define NR14_TRIGGER 0x80
#define NR14_LNEGTH_ENABLE 0x40
#define NR14_FREQ_MSB 0x07

/* square 2 */
#define NR21_DUTY 0xC0
#define NR21_LENGTH_LOAD 0x3F
#define NR22_STARTING_VOLUME 0xF0
#define NR22_ENVELOPE_ADD_MODE 0x08
#define NR22_PERIOD 0x07
#define NR23_FREQ_LSB 0xFF
#define NR24_TRIGGER 0x80
#define NR24_LNEGTH_ENABLE 0x40
#define NR24_FREQ_MSB 0x07

/* wave */
#define NR30_DAC_POWER 0x80
#define NR31_LENGTH_LOAD 0xFF
#define NR32_VOLUME_CODE 0x60
#define NR33_FREQ_LSB 0xFF
#define NR34_TRIGGER 0x80
#define NR34_LNEGTH_ENABLE 0x40
#define NR34_FREQ_MSB 0x07

/* noise */
#define NR41_LENGTH_LOAD 0x3F
#define NR42_STARTING_VOLUME 0xF0
#define NR42_ENVELOPE_ADD_MODE 0x08
#define NR42_PERIOD 0x07
#define NR43_CLOCK_SHIFT 0xF0
#define NR43_WIDTH_MODE 0x08
#define NR43_DIVISOR_CODE 0x07
#define NR44_TRIGGER 0x80
#define NR44_LNEGTH_ENABLE 0x40

/* control / status */
#define NR50_VIN_L_ENABLE 0x80
#define NR50_LEFT_VOL 0x70
#define NR50_VIN_R_ENABLE 0x08
#define NR50_RIGHT_VOL 0x07
#define NR51_LEFT_ENABLES
#define NR51_RIGHT_ENABLES
#define NR52_STATUS 0x80
#define NR52_CHANNEL_LENGTH_STATUSES 0x0F

/* every channel has a trigger, which is bit 7 (0x80) */
/* when a channel is triggered, it usually resets it's values, such */
/* as volume and timers. */

static inline GB_U8 GB_wave_volume(GB_U8 code) {
    /* these are volume %, so 0%, 100%... */
    static const GB_U8 volumes[] = {0U, 100U, 50U, 25U};
    return volumes[code & 0x3];
}

static inline float GB_wave_volumef(GB_U8 code) {
    /* these are volume %, so 0%, 100%... */
    static const float volumes[] = {0.0f, 1.0f, 0.5f, 0.25f};
    return volumes[code & 0x3];
}

#define GET_CHANNEL_FREQ(num) \
    ((IO_NR##num##4 & NR##num##4##_FREQ_MSB) << 8) | \
    (IO_NR##num##3 & NR##num##3##_FREQ_LSB)

#define GET_SQUARE_DUTY(num) ((IO_NR##num##1 & NR##num##1##_DUTY) >> 6)
// #define GET_SQUARE_DUTY(num) ((IO_NR##num##1) >> 6)

#define CHANNEL_STARTING_VOLUME(num) ((IO_NR##num##2 & NR##num##2##_STARTING_VOLUME) >> 4)

#define CHANNEL_TRIGGER(num) ((IO_NR##num##4 & NR##num##4##_TRIGGER) > 0)

#define SOUND_ENABLED() ((IO_NR52 & NR52_STATUS) > 0)

#define CHANNEL_LENGTH_ENABLE(num) ((IO_NR##num##4 & NR##num##4##_LNEGTH_ENABLE) > 0)

#define CHANNEL_PERIOD(num) ((IO_NR##num##2 & NR##num##2##_PERIOD))

// clocked at 512hz
#define FRAME_SEQUENCER_CLOCK 512

/*static*/ const GB_BOOL SQUARE_DUTY_CYCLE_0[] = { 0, 0, 0, 0, 0, 0, 0, 1 };
/*static*/ const GB_BOOL SQUARE_DUTY_CYCLE_1[] = { 1, 0, 0, 0, 0, 0, 0, 1 };
/*static*/ const GB_BOOL SQUARE_DUTY_CYCLE_2[] = { 0, 0, 0, 0, 0, 1, 1, 1 };
/*static*/ const GB_BOOL SQUARE_DUTY_CYCLE_3[] = { 0, 1, 1, 1, 1, 1, 1, 0 };

// 4194304 / 256; // 256 Hz

//       gb = 2048 - (131072 / Hz)
//       Hz = 131072 / (2048 - gb)

// static GB_U16 GB_apu_get_noise_freq(const struct GB_Data* gb) {

// }

// Channels 1-3 can produce frequencies of 64hz-131072hz

// Channel 4 can produce bit-frequencies of 2hz-1048576hz.

// Hz = 4194304 / ((2048 - (11-bit-freq)) << 5)
