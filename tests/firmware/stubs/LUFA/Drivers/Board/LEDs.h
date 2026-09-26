/*
 * LUFA/Drivers/Board/LEDs.h stub
 *
 * Provides LED mask constants and a simulated LED state variable.
 * Tests can read g_led_state to assert on LED changes.
 */
#pragma once
#include <stdint.h>

#define LEDS_NO_LEDS  0x00
#define LEDS_LED1     0x01
#define LEDS_LED2     0x02
#define LEDS_LED3     0x04
#define LEDS_LED4     0x08

extern uint8_t g_led_state;

static inline void LEDs_Init(void) { g_led_state = LEDS_NO_LEDS; }
static inline void LEDs_SetAllLEDs(uint8_t mask) { g_led_state = mask; }
static inline uint8_t LEDs_GetLEDs(void) { return g_led_state; }
