/*
 * stub_globals.c
 *
 * Definitions for all extern variables declared in the stub headers.
 * Compiled once and linked into every test binary.
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* avr/io.h register simulation */
volatile uint8_t sim_PORTB = 0;
volatile uint8_t sim_DDRB  = 0;
volatile uint8_t sim_PINB  = 0;
volatile uint8_t sim_MCUSR = 0;

/* avr/interrupt.h */
int g_global_interrupts_enabled = 0;

/* avr/wdt.h */
int g_wdt_disabled = 0;

/* avr/power.h */
int g_clock_prescale = 0;   /* clock_div_1 == 0 */

/* LUFA LEDs */
uint8_t g_led_state = 0;

/* LUFA Joystick */
uint8_t g_joystick_status = 0;

/* LUFA Buttons */
uint8_t g_button_status = 0;

/* LUFA USB */
int g_usb_initialized = 0;

/* MIDI send log (defined here; declared extern in MIDIClass.h) */
#include "stubs/LUFA/Drivers/USB/Class/MIDIClass.h"

MidiSendLogEntry g_midi_send_log[MIDI_STUB_LOG_MAX];
int              g_midi_send_count  = 0;
int              g_midi_flush_count = 0;
bool             g_midi_send_ok     = true;
