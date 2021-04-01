#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "core/types.h"

// indexed using the square duty code and duty cycle
static const bool SQUARE_DUTY_CYCLES[4][8] = {
    [0] = { 0, 0, 0, 0, 0, 0, 0, 1 },
    [1] = { 1, 0, 0, 0, 0, 0, 0, 1 },
    [2] = { 0, 0, 0, 0, 0, 1, 1, 1 },
    [3] = { 0, 1, 1, 1, 1, 1, 1, 0 },
};

#ifdef __cplusplus
}
#endif
