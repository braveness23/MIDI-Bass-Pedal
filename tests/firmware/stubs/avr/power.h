/*
 * avr/power.h stub — clock prescaler helpers.
 */
#pragma once
#include <stdint.h>

typedef enum {
    clock_div_1   = 0,
    clock_div_2   = 1,
    clock_div_4   = 2,
    clock_div_8   = 3,
    clock_div_16  = 4,
    clock_div_32  = 5,
    clock_div_64  = 6,
    clock_div_128 = 7,
    clock_div_256 = 8,
} clock_div_t;

extern clock_div_t g_clock_prescale;

static inline void clock_prescale_set(clock_div_t d) { g_clock_prescale = d; }
