/*
 * LUFA/Drivers/Board/Joystick.h stub
 *
 * Simulates a 5-direction joystick.  Tests set g_joystick_status to the
 * desired direction bitmask; Joystick_GetStatus() returns it.
 */
#pragma once
#include <stdint.h>

/* Direction bit masks — match the USBKEY board values */
#define JOY_LEFT   0x01
#define JOY_UP     0x02
#define JOY_RIGHT  0x04
#define JOY_DOWN   0x08
#define JOY_PRESS  0x10

extern uint8_t g_joystick_status;

static inline void    Joystick_Init(void)      {}
static inline uint8_t Joystick_GetStatus(void) { return g_joystick_status; }
