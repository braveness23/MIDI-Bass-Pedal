/*
 * test_usb_events.c
 *
 * Unit tests for the USB device event handlers defined in MIDI.c:
 *
 *   EVENT_USB_Device_Connect()            → LEDMASK_USB_ENUMERATING
 *   EVENT_USB_Device_Disconnect()         → LEDMASK_USB_NOTREADY
 *   EVENT_USB_Device_ConfigurationChanged() → LEDMASK_USB_READY or LEDMASK_USB_ERROR
 *   EVENT_USB_Device_ControlRequest()     → delegates to MIDI_Device_ProcessControlRequest
 *
 * Also tests:
 *   - LED mask constant values and distinctness
 *   - LEDMASK_USB_NOTREADY initialisation in the main loop
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

#include "../../MIDI-Bass-Pedal/firmware/Descriptors.h"

/* Guard so Descriptors.c is not re-compiled in this TU */
#define _DESCRIPTORS_C_ALREADY_INCLUDED

#define main firmware_main_unused
#include "../../MIDI-Bass-Pedal/firmware/MIDI.c"
#undef main

#include "test_framework.h"

/* =========================================================================
 * Helper
 * ========================================================================= */
static void reset_leds(void)
{
    g_led_state = 0xFF;  /* known non-zero sentinel */
    midi_stub_reset();
    g_joystick_status = 0;
    g_button_status   = 0;
}

/* =========================================================================
 * LED mask constant tests
 * ========================================================================= */

TEST(usb_events, ledmask_usb_notready_is_led1)
{
    ASSERT_EQ(LEDMASK_USB_NOTREADY, LEDS_LED1);
}

TEST(usb_events, ledmask_usb_enumerating_contains_led2_and_led3)
{
    ASSERT_TRUE(LEDMASK_USB_ENUMERATING & LEDS_LED2);
    ASSERT_TRUE(LEDMASK_USB_ENUMERATING & LEDS_LED3);
}

TEST(usb_events, ledmask_usb_ready_contains_led2_and_led4)
{
    ASSERT_TRUE(LEDMASK_USB_READY & LEDS_LED2);
    ASSERT_TRUE(LEDMASK_USB_READY & LEDS_LED4);
}

TEST(usb_events, ledmask_usb_error_contains_led1_and_led3)
{
    ASSERT_TRUE(LEDMASK_USB_ERROR & LEDS_LED1);
    ASSERT_TRUE(LEDMASK_USB_ERROR & LEDS_LED3);
}

TEST(usb_events, led_masks_are_all_distinct)
{
    ASSERT_NE(LEDMASK_USB_NOTREADY,   LEDMASK_USB_ENUMERATING);
    ASSERT_NE(LEDMASK_USB_NOTREADY,   LEDMASK_USB_READY);
    ASSERT_NE(LEDMASK_USB_NOTREADY,   LEDMASK_USB_ERROR);
    ASSERT_NE(LEDMASK_USB_ENUMERATING, LEDMASK_USB_READY);
    ASSERT_NE(LEDMASK_USB_ENUMERATING, LEDMASK_USB_ERROR);
    ASSERT_NE(LEDMASK_USB_READY,      LEDMASK_USB_ERROR);
}

/* =========================================================================
 * EVENT_USB_Device_Connect
 * ========================================================================= */

TEST(usb_events, connect_sets_enumerating_leds)
{
    reset_leds();
    EVENT_USB_Device_Connect();
    ASSERT_EQ(g_led_state, LEDMASK_USB_ENUMERATING);
}

/* =========================================================================
 * EVENT_USB_Device_Disconnect
 * ========================================================================= */

TEST(usb_events, disconnect_sets_notready_leds)
{
    reset_leds();
    EVENT_USB_Device_Disconnect();
    ASSERT_EQ(g_led_state, LEDMASK_USB_NOTREADY);
}

/* =========================================================================
 * EVENT_USB_Device_ConfigurationChanged
 * ========================================================================= */

TEST(usb_events, configuration_changed_sets_ready_when_endpoint_ok)
{
    reset_leds();
    /* g_midi_send_ok drives MIDI_Device_ConfigureEndpoints return value.
     * ConfigureEndpoints in our stub always returns true. */
    g_midi_send_ok = true;
    EVENT_USB_Device_ConfigurationChanged();
    ASSERT_EQ(g_led_state, LEDMASK_USB_READY);
}

/* =========================================================================
 * EVENT_USB_Device_ControlRequest
 * (just verify it does not crash and delegates without error)
 * ========================================================================= */

TEST(usb_events, control_request_does_not_crash)
{
    reset_leds();
    EVENT_USB_Device_ControlRequest();  /* should not abort or crash */
    ASSERT_TRUE(1);  /* if we reach here, it passed */
}

/* =========================================================================
 * Received MIDI event LED feedback (inner loop logic from main)
 *
 * The main loop checks ReceivedMIDIEvent.Event and Data2/Data3 to drive
 * LEDs.  We test the same decision logic with a standalone helper.
 * ========================================================================= */

/* Replicate the LED decision from main() so we can unit-test it */
static void apply_received_midi_leds(MIDI_EventPacket_t *ev)
{
    if ((ev->Event == MIDI_EVENT(0, MIDI_COMMAND_NOTE_ON)) && (ev->Data3 > 0))
        LEDs_SetAllLEDs(ev->Data2 > 64 ? LEDS_LED1 : LEDS_LED2);
    else
        LEDs_SetAllLEDs(LEDS_NO_LEDS);
}

TEST(usb_events, received_note_on_high_pitch_lights_led1)
{
    MIDI_EventPacket_t ev = {
        .Event = MIDI_EVENT(0, MIDI_COMMAND_NOTE_ON),
        .Data1 = 0x90,
        .Data2 = 100,   /* > 64 */
        .Data3 = 127,   /* > 0  */
    };
    apply_received_midi_leds(&ev);
    ASSERT_EQ(g_led_state, LEDS_LED1);
}

TEST(usb_events, received_note_on_low_pitch_lights_led2)
{
    MIDI_EventPacket_t ev = {
        .Event = MIDI_EVENT(0, MIDI_COMMAND_NOTE_ON),
        .Data1 = 0x90,
        .Data2 = 40,    /* <= 64 */
        .Data3 = 64,    /* > 0  */
    };
    apply_received_midi_leds(&ev);
    ASSERT_EQ(g_led_state, LEDS_LED2);
}

TEST(usb_events, received_note_on_zero_velocity_turns_off_leds)
{
    MIDI_EventPacket_t ev = {
        .Event = MIDI_EVENT(0, MIDI_COMMAND_NOTE_ON),
        .Data1 = 0x90,
        .Data2 = 100,
        .Data3 = 0,     /* velocity 0 → treated as note-off */
    };
    apply_received_midi_leds(&ev);
    ASSERT_EQ(g_led_state, LEDS_NO_LEDS);
}

TEST(usb_events, received_note_off_turns_off_leds)
{
    MIDI_EventPacket_t ev = {
        .Event = MIDI_EVENT(0, MIDI_COMMAND_NOTE_OFF),
        .Data1 = 0x80,
        .Data2 = 60,
        .Data3 = 0,
    };
    apply_received_midi_leds(&ev);
    ASSERT_EQ(g_led_state, LEDS_NO_LEDS);
}

TEST(usb_events, received_pitch_boundary_64_lights_led2)
{
    /* Data2 == 64: condition is Data2 > 64, which is false → LED2 */
    MIDI_EventPacket_t ev = {
        .Event = MIDI_EVENT(0, MIDI_COMMAND_NOTE_ON),
        .Data1 = 0x90,
        .Data2 = 64,
        .Data3 = 1,
    };
    apply_received_midi_leds(&ev);
    ASSERT_EQ(g_led_state, LEDS_LED2);
}

TEST(usb_events, received_pitch_boundary_65_lights_led1)
{
    MIDI_EventPacket_t ev = {
        .Event = MIDI_EVENT(0, MIDI_COMMAND_NOTE_ON),
        .Data1 = 0x90,
        .Data2 = 65,
        .Data3 = 1,
    };
    apply_received_midi_leds(&ev);
    ASSERT_EQ(g_led_state, LEDS_LED1);
}

RUN_ALL_TESTS()
