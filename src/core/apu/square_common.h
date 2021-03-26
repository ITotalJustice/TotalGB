#pragma once

#include "../types.h"

// indexed using the square duty code and duty cycle
static const GB_BOOL SQUARE_DUTY_CYCLE_0[8] = { 0, 0, 0, 0, 0, 0, 0, 1 };
static const GB_BOOL SQUARE_DUTY_CYCLE_1[8] = { 1, 0, 0, 0, 0, 0, 0, 1 };
static const GB_BOOL SQUARE_DUTY_CYCLE_2[8] = { 0, 0, 0, 0, 0, 1, 1, 1 };
static const GB_BOOL SQUARE_DUTY_CYCLE_3[8] = { 0, 1, 1, 1, 1, 1, 1, 0 };
static const GB_BOOL* const SQUARE_DUTY_CYCLES[4] = {
    SQUARE_DUTY_CYCLE_0,
    SQUARE_DUTY_CYCLE_1,
    SQUARE_DUTY_CYCLE_2,
    SQUARE_DUTY_CYCLE_3
};