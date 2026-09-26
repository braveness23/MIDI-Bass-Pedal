/*
 * util/delay.h stub — busy-wait delay helpers.
 * On Linux these are no-ops; tests do not need actual wall-clock delays.
 */
#pragma once

static inline void _delay_ms(double ms)   { (void)ms; }
static inline void _delay_us(double us)   { (void)us; }
