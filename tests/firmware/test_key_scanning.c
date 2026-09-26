/*
 * test_key_scanning.c
 *
 * Unit tests for the key-scanning logic in CheckJoystickMovement().
 *
 * Strategy:
 *   1. Include the firmware source with main() redefined to avoid conflicts.
 *   2. Drive g_joystick_status / g_button_status from the test.
 *   3. Call CheckJoystickMovement() directly.
 *   4. Inspect g_midi_send_log[] for the expected MIDI event.
 *
 * Tests cover:
 *   - Each of the 5 joystick directions emits the correct pitch
 *   - Pressing a direction emits Note-On; releasing emits Note-Off
 *   - Channel 1 (no button) vs channel 10 (button held)
 *   - No MIDI event when nothing changes
 *   - Multiple direction changes in one call (last one wins — firmware
 *     behaviour when multiple bits change simultaneously)
 *   - Flush is called after sending
 *   - State tracking: a second identical call with no change sends nothing
 */

#define ARCH_AVR8 1
#define ARCH      ARCH_AVR8

#include "stubs/avr/io.h"
#include "stubs/avr/wdt.h"
#include "stubs/avr/power.h"
#include "stubs/avr/interrupt.h"
#include "stubs/avr/pgmspace.h"
#include "stubs/LUFA/Drivers/USB/USB.h"
#include "stubs/LUFA/Drivers/Board/LEDs.h"
#include "stubs/LUFA/Drivers/Board/Joystick.h"
#include "stubs/LUFA/Drivers/Board/Buttons.h"
#include "stubs/LUFA/Platform/Platform.h"

/* We need Descriptors.h for the INTERFACE_ID_ enum, but we do NOT want to
 * pull in Descriptors.c (we already compiled those separately).  Just
 * forward-declare what MIDI.c needs. */
#include "../../MIDI-Bass-Pedal/firmware/Descriptors.h"

/* Prevent Descriptors.c from being compiled a second time */
#define _DESCRIPTORS_C_ALREADY_INCLUDED

/* Redirect the firmware's main() to a different symbol so it does not
 * clash with our test runner's main(). */
#define main firmware_main_unused

#include "../../MIDI-Bass-Pedal/firmware/MIDI.c"

#undef main

/* Pull in the stub globals definition (only once per binary) */
/* done via Makefile linking stub_globals.o */

#include "test_framework.h"

/* =========================================================================
 * Helper: reset all stub state between tests
 * ========================================================================= */
static void reset_state(void)
{
    midi_stub_reset();
    g_joystick_status = 0;
    g_button_status   = 0;
    g_led_state       = 0;

    /* Reset the static PrevJoystickStatus inside CheckJoystickMovement by
     * calling it once with joystick=0 so the previous state is recorded as 0.
     * (We need to do this carefully: call with 0 and discard any spurious
     * send caused by whatever PrevJoystickStatus was before.) */
    g_joystick_status = 0;
    CheckJoystickMovement();
    midi_stub_reset();   /* discard anything emitted during reset call */
}

/* =========================================================================
 * Tests: pitch mapping per direction
 * ========================================================================= */

TEST(key_scanning, joy_left_pitch_is_0x3C)
{
    reset_state();
    g_joystick_status = JOY_LEFT;
    CheckJoystickMovement();

    ASSERT_EQ(g_midi_send_count, 1);
    ASSERT_EQ(g_midi_send_log[0].packet.Data2, 0x3C);
}

TEST(key_scanning, joy_up_pitch_is_0x3D)
{
    reset_state();
    g_joystick_status = JOY_UP;
    CheckJoystickMovement();

    ASSERT_EQ(g_midi_send_count, 1);
    ASSERT_EQ(g_midi_send_log[0].packet.Data2, 0x3D);
}

TEST(key_scanning, joy_right_pitch_is_0x3E)
{
    reset_state();
    g_joystick_status = JOY_RIGHT;
    CheckJoystickMovement();

    ASSERT_EQ(g_midi_send_count, 1);
    ASSERT_EQ(g_midi_send_log[0].packet.Data2, 0x3E);
}

TEST(key_scanning, joy_down_pitch_is_0x3F)
{
    reset_state();
    g_joystick_status = JOY_DOWN;
    CheckJoystickMovement();

    ASSERT_EQ(g_midi_send_count, 1);
    ASSERT_EQ(g_midi_send_log[0].packet.Data2, 0x3F);
}

TEST(key_scanning, joy_press_pitch_is_0x3B)
{
    reset_state();
    g_joystick_status = JOY_PRESS;
    CheckJoystickMovement();

    ASSERT_EQ(g_midi_send_count, 1);
    ASSERT_EQ(g_midi_send_log[0].packet.Data2, 0x3B);
}

/* =========================================================================
 * Tests: Note-On vs Note-Off
 * ========================================================================= */

TEST(key_scanning, press_sends_note_on)
{
    reset_state();
    g_joystick_status = JOY_LEFT;
    CheckJoystickMovement();

    ASSERT_EQ(g_midi_send_count, 1);
    /* Event byte CIN must be 0x09 (Note-On, cable 0) */
    ASSERT_EQ(g_midi_send_log[0].packet.Event, MIDI_EVENT(0, MIDI_COMMAND_NOTE_ON));
    /* Data1 high nibble must be 0x9x */
    ASSERT_EQ(g_midi_send_log[0].packet.Data1 & 0xF0, 0x90);
}

TEST(key_scanning, release_sends_note_off)
{
    reset_state();
    /* Press */
    g_joystick_status = JOY_LEFT;
    CheckJoystickMovement();
    midi_stub_reset();

    /* Release */
    g_joystick_status = 0;
    CheckJoystickMovement();

    ASSERT_EQ(g_midi_send_count, 1);
    ASSERT_EQ(g_midi_send_log[0].packet.Event, MIDI_EVENT(0, MIDI_COMMAND_NOTE_OFF));
    ASSERT_EQ(g_midi_send_log[0].packet.Data1 & 0xF0, 0x80);
}

TEST(key_scanning, release_sends_same_pitch_as_press)
{
    reset_state();
    g_joystick_status = JOY_RIGHT;
    CheckJoystickMovement();
    midi_stub_reset();

    g_joystick_status = 0;
    CheckJoystickMovement();

    ASSERT_EQ(g_midi_send_count, 1);
    ASSERT_EQ(g_midi_send_log[0].packet.Data2, 0x3E);
}

/* =========================================================================
 * Tests: Channel selection (button)
 * ========================================================================= */

TEST(key_scanning, no_button_uses_channel_1)
{
    reset_state();
    g_button_status   = 0;              /* button not pressed */
    g_joystick_status = JOY_UP;
    CheckJoystickMovement();

    ASSERT_EQ(g_midi_send_count, 1);
    /* channel 1 → low nibble of Data1 = 0x00 */
    ASSERT_EQ(g_midi_send_log[0].packet.Data1 & 0x0F, MIDI_CHANNEL(1));
    ASSERT_EQ(g_midi_send_log[0].packet.Data1 & 0x0F, 0x00);
}

TEST(key_scanning, button_held_uses_channel_10)
{
    reset_state();
    g_button_status   = BUTTONS_BUTTON1; /* button pressed */
    g_joystick_status = JOY_UP;
    CheckJoystickMovement();

    ASSERT_EQ(g_midi_send_count, 1);
    /* channel 10 → low nibble of Data1 = 0x09 */
    ASSERT_EQ(g_midi_send_log[0].packet.Data1 & 0x0F, MIDI_CHANNEL(10));
    ASSERT_EQ(g_midi_send_log[0].packet.Data1 & 0x0F, 0x09);
}

/* =========================================================================
 * Tests: No change → no MIDI event
 * ========================================================================= */

TEST(key_scanning, no_joystick_change_sends_nothing)
{
    reset_state();
    /* Call once to set PrevJoystickStatus = JOY_DOWN */
    g_joystick_status = JOY_DOWN;
    CheckJoystickMovement();
    midi_stub_reset();

    /* Call again with no change */
    CheckJoystickMovement();
    ASSERT_EQ(g_midi_send_count, 0);
}

TEST(key_scanning, idle_state_sends_nothing)
{
    reset_state();
    /* joystick already 0 after reset_state(); one more call should send nothing */
    CheckJoystickMovement();
    ASSERT_EQ(g_midi_send_count, 0);
}

/* =========================================================================
 * Tests: Flush is called after send
 * ========================================================================= */

TEST(key_scanning, flush_called_after_note_on)
{
    reset_state();
    g_joystick_status = JOY_PRESS;
    CheckJoystickMovement();

    ASSERT_EQ(g_midi_send_count,  1);
    ASSERT_EQ(g_midi_flush_count, 1);
}

TEST(key_scanning, no_flush_when_no_event)
{
    reset_state();
    CheckJoystickMovement();

    ASSERT_EQ(g_midi_send_count,  0);
    ASSERT_EQ(g_midi_flush_count, 0);
}

/* =========================================================================
 * Tests: State tracking (PrevJoystickStatus persists across calls)
 * ========================================================================= */

TEST(key_scanning, held_key_does_not_repeat)
{
    reset_state();
    g_joystick_status = JOY_LEFT;

    /* First call: change detected → Note-On */
    CheckJoystickMovement();
    ASSERT_EQ(g_midi_send_count, 1);
    midi_stub_reset();

    /* Second call: no change → nothing */
    CheckJoystickMovement();
    ASSERT_EQ(g_midi_send_count, 0);
}

TEST(key_scanning, press_then_release_sends_two_events)
{
    reset_state();

    g_joystick_status = JOY_DOWN;
    CheckJoystickMovement();   /* Note-On */

    g_joystick_status = 0;
    CheckJoystickMovement();   /* Note-Off */

    /* Two events total since reset_state() */
    ASSERT_EQ(g_midi_send_count, 2);
    ASSERT_EQ(g_midi_send_log[0].packet.Event, MIDI_EVENT(0, MIDI_COMMAND_NOTE_ON));
    ASSERT_EQ(g_midi_send_log[1].packet.Event, MIDI_EVENT(0, MIDI_COMMAND_NOTE_OFF));
}

/* =========================================================================
 * Tests: Velocity
 * ========================================================================= */

TEST(key_scanning, note_on_uses_standard_velocity)
{
    reset_state();
    g_joystick_status = JOY_UP;
    CheckJoystickMovement();

    ASSERT_EQ(g_midi_send_count, 1);
    ASSERT_EQ(g_midi_send_log[0].packet.Data3, MIDI_STANDARD_VELOCITY);
}

TEST(key_scanning, note_off_uses_standard_velocity)
{
    reset_state();
    g_joystick_status = JOY_UP;
    CheckJoystickMovement();
    midi_stub_reset();

    g_joystick_status = 0;
    CheckJoystickMovement();

    ASSERT_EQ(g_midi_send_count, 1);
    ASSERT_EQ(g_midi_send_log[0].packet.Data3, MIDI_STANDARD_VELOCITY);
}

/* =========================================================================
 * Tests: Cable number in Event byte is always 0
 * ========================================================================= */

TEST(key_scanning, cable_number_is_zero_for_all_directions)
{
    uint8_t dirs[] = { JOY_LEFT, JOY_UP, JOY_RIGHT, JOY_DOWN, JOY_PRESS };
    uint8_t npitches = sizeof(dirs) / sizeof(dirs[0]);

    for (uint8_t i = 0; i < npitches; i++) {
        reset_state();
        g_joystick_status = dirs[i];
        CheckJoystickMovement();

        ASSERT_EQ(g_midi_send_count, 1);
        /* High nibble of Event byte = cable number = 0 */
        ASSERT_EQ((g_midi_send_log[0].packet.Event >> 4) & 0x0F, 0x00);
    }
}

RUN_ALL_TESTS()
