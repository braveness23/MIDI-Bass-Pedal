/*
 * LUFA/Drivers/Board/Buttons.h stub
 *
 * Simulates the single board button.  Tests set g_button_status to
 * BUTTONS_BUTTON1 when the button is pressed, or 0 when released.
 */
#pragma once
#include <stdint.h>

#define BUTTONS_BUTTON1  0x01

extern uint8_t g_button_status;

static inline void    Buttons_Init(void)      {}
static inline uint8_t Buttons_GetStatus(void) { return g_button_status; }
