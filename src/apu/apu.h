#ifndef _GB_APU_H_
#define _GB_APU_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "../internal.h"

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

#define CH1 gb->apu.ch1
#define CH2 gb->apu.ch2
#define CH3 gb->apu.ch3
#define CH4 gb->apu.ch4
#define CONTROL_CHANNEL gb->apu.control

#define CALC_CALLBACK_FREQ(freq) (4213440 / freq)

// clocked at 512hz
#define FRAME_SEQUENCER_CLOCK 512

// 4 * 1024^2 / 512
#define FRAME_SEQUENCER_STEP_RATE 8192

enum EnvelopeMode
{
    SUB = 0,
    ADD = 1
};

// defined in core/apu/apu.c
extern const bool SQUARE_DUTY_CYCLES[4][8];

// defined in core/apu/apu.c
extern const uint8_t PERIOD_TABLE[8];


GB_FORCE_INLINE uint16_t get_ch1_freq(const struct GB_Core* gb);
GB_FORCE_INLINE bool is_ch1_dac_enabled(const struct GB_Core* gb);
GB_FORCE_INLINE bool is_ch1_enabled(const struct GB_Core* gb);
GB_FORCE_INLINE void ch1_enable(struct GB_Core* gb);
GB_FORCE_INLINE void ch1_disable(struct GB_Core* gb);
GB_FORCE_INLINE int8_t sample_ch1(struct GB_Core* gb);
GB_FORCE_INLINE void clock_ch1_len(struct GB_Core* gb);
GB_FORCE_INLINE void clock_ch1_vol(struct GB_Core* gb);
GB_FORCE_INLINE void do_freq_sweep_calc(struct GB_Core* gb);
GB_FORCE_INLINE void on_ch1_sweep(struct GB_Core* gb);
GB_FORCE_INLINE void on_ch1_trigger(struct GB_Core* gb);

GB_FORCE_INLINE uint16_t get_ch2_freq(const struct GB_Core* gb);
GB_FORCE_INLINE bool is_ch2_dac_enabled(const struct GB_Core* gb);
GB_FORCE_INLINE bool is_ch2_enabled(const struct GB_Core* gb);
GB_FORCE_INLINE void ch2_enable(struct GB_Core* gb);
GB_FORCE_INLINE void ch2_disable(struct GB_Core* gb);
GB_FORCE_INLINE int8_t sample_ch2(struct GB_Core* gb);
GB_FORCE_INLINE void clock_ch2_len(struct GB_Core* gb);
GB_FORCE_INLINE void clock_ch2_vol(struct GB_Core* gb);
GB_FORCE_INLINE void on_ch2_trigger(struct GB_Core* gb);

GB_FORCE_INLINE uint16_t get_ch3_freq(const struct GB_Core* gb);
GB_FORCE_INLINE bool is_ch3_dac_enabled(const struct GB_Core* gb);
GB_FORCE_INLINE bool is_ch3_enabled(const struct GB_Core* gb);
GB_FORCE_INLINE void ch3_enable(struct GB_Core* gb);
GB_FORCE_INLINE void ch3_disable(struct GB_Core* gb);
GB_FORCE_INLINE int8_t sample_ch3(struct GB_Core* gb);
GB_FORCE_INLINE void clock_ch3_len(struct GB_Core* gb);
GB_FORCE_INLINE void advance_ch3_position_counter(struct GB_Core* gb);
GB_FORCE_INLINE void on_ch3_trigger(struct GB_Core* gb);

GB_FORCE_INLINE uint32_t get_ch4_freq(const struct GB_Core* gb);
GB_FORCE_INLINE bool is_ch4_dac_enabled(const struct GB_Core* gb);
GB_FORCE_INLINE bool is_ch4_enabled(const struct GB_Core* gb);
GB_FORCE_INLINE void ch4_enable(struct GB_Core* gb);
GB_FORCE_INLINE void ch4_disable(struct GB_Core* gb);
GB_FORCE_INLINE int8_t sample_ch4(struct GB_Core* gb);
GB_FORCE_INLINE void clock_ch4_len(struct GB_Core* gb);
GB_FORCE_INLINE void clock_ch4_vol(struct GB_Core* gb);
GB_FORCE_INLINE void step_ch4_lfsr(struct GB_Core* gb);
GB_FORCE_INLINE void on_ch4_trigger(struct GB_Core* gb);

GB_STATIC void gb_apu_on_enabled(struct GB_Core* gb);
GB_STATIC void gb_apu_on_disabled(struct GB_Core* gb);

GB_FORCE_INLINE bool gb_is_apu_enabled(const struct GB_Core* gb);
GB_FORCE_INLINE bool is_next_frame_sequencer_step_not_len(const struct GB_Core* gb);
GB_FORCE_INLINE bool is_next_frame_sequencer_step_vol(const struct GB_Core* gb);

#ifdef __cplusplus
}
#endif

#endif // _GB_APU_H_
