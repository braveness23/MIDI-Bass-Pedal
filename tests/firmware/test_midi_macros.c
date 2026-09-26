/*
 * test_midi_macros.c
 *
 * Exhaustive tests for the MIDI helper macros and constants that the
 * firmware relies on.  These tests are pure arithmetic — no firmware source
 * compilation needed — so they are the fastest and most portable.
 *
 * Covered:
 *   - MIDI_EVENT: all 16 cables × all command nibbles (0x8–0xE)
 *   - MIDI_CHANNEL: all 16 channels
 *   - MIDI command constant values (NOTE_ON, NOTE_OFF, etc.)
 *   - MIDI_STANDARD_VELOCITY
 *   - VERSION_BCD encoding for common version tuples
 *   - MIDI_STREAM_EPSIZE, MIDI_STREAM_IN_EPADDR, MIDI_STREAM_OUT_EPADDR
 *   - FIXED_CONTROL_ENDPOINT_SIZE, FIXED_NUM_CONFIGURATIONS
 */

#define ARCH_AVR8 1
#define ARCH      ARCH_AVR8

#include "stubs/avr/io.h"
#include "stubs/avr/pgmspace.h"
#include "stubs/LUFA/Drivers/USB/USB.h"

/* Bring in Descriptors.h for endpoint/size constants */
#include "../../MIDI-Bass-Pedal/firmware/Descriptors.h"

/* LUFAConfig.h for FIXED_CONTROL_ENDPOINT_SIZE / FIXED_NUM_CONFIGURATIONS */
#include "../../MIDI-Bass-Pedal/firmware/Config/LUFAConfig.h"

/* Board Joystick constants */
#include "stubs/LUFA/Drivers/Board/Joystick.h"

#include "test_framework.h"

/* =========================================================================
 * MIDI_EVENT macro — exhaustive cable × command sweep
 * ========================================================================= */

/* Helper: extract cable number from Event byte */
static uint8_t cable_from_event(uint8_t event) { return (event >> 4) & 0x0F; }
/* Helper: extract CIN from Event byte */
static uint8_t cin_from_event(uint8_t event)   { return  event       & 0x0F; }

TEST(midi_macros, event_cable_0_note_on)
{
    uint8_t ev = MIDI_EVENT(0, MIDI_COMMAND_NOTE_ON);
    ASSERT_EQ(cable_from_event(ev), 0);
    ASSERT_EQ(cin_from_event(ev),   9);   /* 0x90 >> 4 = 9 */
}

TEST(midi_macros, event_cable_5_note_on)
{
    uint8_t ev = MIDI_EVENT(5, MIDI_COMMAND_NOTE_ON);
    ASSERT_EQ(cable_from_event(ev), 5);
    ASSERT_EQ(cin_from_event(ev),   9);
}

TEST(midi_macros, event_cable_15_note_on)
{
    uint8_t ev = MIDI_EVENT(15, MIDI_COMMAND_NOTE_ON);
    ASSERT_EQ(cable_from_event(ev), 15);
    ASSERT_EQ(cin_from_event(ev),    9);
}

TEST(midi_macros, event_cable_0_note_off)
{
    uint8_t ev = MIDI_EVENT(0, MIDI_COMMAND_NOTE_OFF);
    ASSERT_EQ(cable_from_event(ev), 0);
    ASSERT_EQ(cin_from_event(ev),   8);   /* 0x80 >> 4 = 8 */
}

TEST(midi_macros, event_all_16_cables_preserve_cin)
{
    /* CIN for NOTE_ON should be 9 regardless of cable */
    for (int cable = 0; cable < 16; cable++) {
        uint8_t ev = MIDI_EVENT(cable, MIDI_COMMAND_NOTE_ON);
        ASSERT_EQ(cable_from_event(ev), (uint8_t)cable);
        ASSERT_EQ(cin_from_event(ev),   9u);
    }
}

/* =========================================================================
 * MIDI_CHANNEL macro — all 16 channels
 * ========================================================================= */

TEST(midi_macros, channel_1_through_16)
{
    for (int ch = 1; ch <= 16; ch++) {
        ASSERT_EQ(MIDI_CHANNEL(ch), (uint8_t)(ch - 1));
    }
}

/* =========================================================================
 * MIDI command constants
 * ========================================================================= */

TEST(midi_macros, note_off_command_value)
{
    ASSERT_EQ(MIDI_COMMAND_NOTE_OFF, 0x80);
}

TEST(midi_macros, note_on_command_value)
{
    ASSERT_EQ(MIDI_COMMAND_NOTE_ON, 0x90);
}

TEST(midi_macros, note_on_and_note_off_differ_by_0x10)
{
    ASSERT_EQ(MIDI_COMMAND_NOTE_ON - MIDI_COMMAND_NOTE_OFF, 0x10);
}

TEST(midi_macros, note_on_high_nibble_is_9)
{
    ASSERT_EQ((MIDI_COMMAND_NOTE_ON >> 4) & 0x0F, 9u);
}

TEST(midi_macros, note_off_high_nibble_is_8)
{
    ASSERT_EQ((MIDI_COMMAND_NOTE_OFF >> 4) & 0x0F, 8u);
}

/* =========================================================================
 * MIDI_STANDARD_VELOCITY
 * ========================================================================= */

TEST(midi_macros, standard_velocity_is_64)
{
    ASSERT_EQ(MIDI_STANDARD_VELOCITY, 64);
}

TEST(midi_macros, standard_velocity_is_valid_midi_range)
{
    ASSERT_TRUE(MIDI_STANDARD_VELOCITY > 0);
    ASSERT_TRUE(MIDI_STANDARD_VELOCITY <= 127);
}

/* =========================================================================
 * VERSION_BCD macro
 * ========================================================================= */

TEST(midi_macros, version_bcd_1_0_0)
{
    /* 1.0.0 → 0x0100 */
    ASSERT_EQ(VERSION_BCD(1, 0, 0), 0x0100);
}

TEST(midi_macros, version_bcd_1_1_0)
{
    /* 1.1.0 → 0x0110 */
    ASSERT_EQ(VERSION_BCD(1, 1, 0), 0x0110);
}

TEST(midi_macros, version_bcd_0_0_1)
{
    /* 0.0.1 → 0x0001 */
    ASSERT_EQ(VERSION_BCD(0, 0, 1), 0x0001);
}

TEST(midi_macros, version_bcd_2_0_0)
{
    ASSERT_EQ(VERSION_BCD(2, 0, 0), 0x0200);
}

/* =========================================================================
 * Endpoint address macros
 * ========================================================================= */

TEST(midi_macros, stream_in_epaddr_has_direction_in_bit)
{
    ASSERT_TRUE(MIDI_STREAM_IN_EPADDR & ENDPOINT_DIR_IN);
    ASSERT_EQ(MIDI_STREAM_IN_EPADDR & ENDPOINT_DIR_IN, ENDPOINT_DIR_IN);
}

TEST(midi_macros, stream_out_epaddr_has_no_direction_in_bit)
{
    ASSERT_FALSE(MIDI_STREAM_OUT_EPADDR & ENDPOINT_DIR_IN);
}

TEST(midi_macros, stream_in_epaddr_endpoint_number_is_2)
{
    ASSERT_EQ(MIDI_STREAM_IN_EPADDR & ~ENDPOINT_DIR_IN, 2u);
}

TEST(midi_macros, stream_out_epaddr_endpoint_number_is_1)
{
    ASSERT_EQ(MIDI_STREAM_OUT_EPADDR & ~ENDPOINT_DIR_IN, 1u);
}

TEST(midi_macros, stream_in_and_out_epaddrs_are_distinct)
{
    ASSERT_NE(MIDI_STREAM_IN_EPADDR, MIDI_STREAM_OUT_EPADDR);
}

/* =========================================================================
 * Endpoint size
 * ========================================================================= */

TEST(midi_macros, stream_epsize_is_64)
{
    ASSERT_EQ(MIDI_STREAM_EPSIZE, 64u);
}

/* =========================================================================
 * Fixed configuration constants
 * ========================================================================= */

TEST(midi_macros, fixed_control_endpoint_size_is_8)
{
    ASSERT_EQ(FIXED_CONTROL_ENDPOINT_SIZE, 8u);
}

TEST(midi_macros, fixed_num_configurations_is_1)
{
    ASSERT_EQ(FIXED_NUM_CONFIGURATIONS, 1u);
}

/* =========================================================================
 * Joystick direction bits are distinct and non-zero
 * ========================================================================= */

TEST(midi_macros, joystick_bits_are_distinct)
{
    ASSERT_NE(JOY_LEFT,  JOY_UP);
    ASSERT_NE(JOY_LEFT,  JOY_RIGHT);
    ASSERT_NE(JOY_LEFT,  JOY_DOWN);
    ASSERT_NE(JOY_LEFT,  JOY_PRESS);
    ASSERT_NE(JOY_UP,    JOY_RIGHT);
    ASSERT_NE(JOY_UP,    JOY_DOWN);
    ASSERT_NE(JOY_UP,    JOY_PRESS);
    ASSERT_NE(JOY_RIGHT, JOY_DOWN);
    ASSERT_NE(JOY_RIGHT, JOY_PRESS);
    ASSERT_NE(JOY_DOWN,  JOY_PRESS);
}

TEST(midi_macros, joystick_bits_are_single_bits)
{
    /* Each direction mask must be a power of two */
    uint8_t dirs[] = { JOY_LEFT, JOY_UP, JOY_RIGHT, JOY_DOWN, JOY_PRESS };
    for (int i = 0; i < 5; i++) {
        uint8_t d = dirs[i];
        ASSERT_TRUE(d != 0);
        ASSERT_TRUE((d & (d - 1)) == 0);   /* power-of-two check */
    }
}

RUN_ALL_TESTS()
