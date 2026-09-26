/*
 * avr/io.h stub — replaces the AVR-libc header for Linux host compilation.
 *
 * Provides volatile register simulators for PORTB, DDRB, PINB, MCUSR and
 * the watchdog disable sequence, plus the minimal set of bit-manipulation
 * macros the firmware uses.
 */
#pragma once

#include <stdint.h>

/* -------------------------------------------------------------------------
 * Simulated I/O registers
 * Each register is a plain volatile byte that tests can read / write freely.
 * ------------------------------------------------------------------------- */
extern volatile uint8_t sim_PORTB;
extern volatile uint8_t sim_DDRB;
extern volatile uint8_t sim_PINB;
extern volatile uint8_t sim_MCUSR;

#define PORTB  sim_PORTB
#define DDRB   sim_DDRB
#define PINB   sim_PINB
#define MCUSR  sim_MCUSR

/* Watchdog-related bits used in SetupHardware */
#define WDRF   3          /* watchdog reset flag bit position */

/* AVR bit-manipulation helpers */
#ifndef _BV
#  define _BV(bit)  (1 << (bit))
#endif

/* Port B pin bit positions */
#define PB0 0
#define PB1 1
#define PB2 2
#define PB3 3
#define PB4 4
#define PB5 5
#define PB6 6
#define PB7 7
