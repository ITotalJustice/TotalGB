#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "../types.h"

// indexed using the square duty code and duty cycle
static const bool SQUARE_DUTY_CYCLE_0[8] = { 0, 0, 0, 0, 0, 0, 0, 1 };
static const bool SQUARE_DUTY_CYCLE_1[8] = { 1, 0, 0, 0, 0, 0, 0, 1 };
static const bool SQUARE_DUTY_CYCLE_2[8] = { 0, 0, 0, 0, 0, 1, 1, 1 };
static const bool SQUARE_DUTY_CYCLE_3[8] = { 0, 1, 1, 1, 1, 1, 1, 0 };
static const bool* const SQUARE_DUTY_CYCLES[4] = {
    SQUARE_DUTY_CYCLE_0,
    SQUARE_DUTY_CYCLE_1,
    SQUARE_DUTY_CYCLE_2,
    SQUARE_DUTY_CYCLE_3
};

#ifdef __cplusplus
}
#endif
