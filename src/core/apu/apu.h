#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "core/types.h"


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
