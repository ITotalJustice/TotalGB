#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "core/types.h"


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


uint16_t get_square1_freq(const struct GB_Core* gb);
bool is_square1_dac_enabled(const struct GB_Core* gb);
bool is_square1_enabled(const struct GB_Core* gb);
void square1_enable(struct GB_Core* gb);
void square1_disable(struct GB_Core* gb);
int8_t sample_square1(struct GB_Core* gb);
void clock_square1_len(struct GB_Core* gb);
void clock_square1_vol(struct GB_Core* gb);
void do_freq_sweep_calc(struct GB_Core* gb);
void on_square1_sweep(struct GB_Core* gb);
void on_square1_trigger(struct GB_Core* gb);

uint16_t get_square2_freq(const struct GB_Core* gb);
bool is_square2_dac_enabled(const struct GB_Core* gb);
bool is_square2_enabled(const struct GB_Core* gb);
void square2_enable(struct GB_Core* gb);
void square2_disable(struct GB_Core* gb);
int8_t sample_square2(struct GB_Core* gb);
void clock_square2_len(struct GB_Core* gb);
void clock_square2_vol(struct GB_Core* gb);
void on_square2_trigger(struct GB_Core* gb);

uint16_t get_wave_freq(const struct GB_Core* gb);
bool is_wave_dac_enabled(const struct GB_Core* gb);
bool is_wave_enabled(const struct GB_Core* gb);
void wave_enable(struct GB_Core* gb);
void wave_disable(struct GB_Core* gb);
int8_t sample_wave(struct GB_Core* gb);
void clock_wave_len(struct GB_Core* gb);
void advance_wave_position_counter(struct GB_Core* gb);
void on_wave_trigger(struct GB_Core* gb);

uint32_t get_noise_freq(const struct GB_Core* gb);
bool is_noise_dac_enabled(const struct GB_Core* gb);
bool is_noise_enabled(const struct GB_Core* gb);
void noise_enable(struct GB_Core* gb);
void noise_disable(struct GB_Core* gb);
int8_t sample_noise(struct GB_Core* gb);
void clock_noise_len(struct GB_Core* gb);
void clock_noise_vol(struct GB_Core* gb);
void step_noise_lfsr(struct GB_Core* gb);
void on_noise_trigger(struct GB_Core* gb);


#ifdef __cplusplus
}
#endif
