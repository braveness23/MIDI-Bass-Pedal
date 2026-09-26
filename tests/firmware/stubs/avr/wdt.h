/*
 * avr/wdt.h stub — watchdog timer helpers.
 */
#pragma once

extern int g_wdt_disabled;

static inline void wdt_disable(void)  { g_wdt_disabled = 1; }
static inline void wdt_enable(uint8_t timeout) { (void)timeout; g_wdt_disabled = 0; }
