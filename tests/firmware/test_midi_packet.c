/*
 * test_midi_packet.c
 *
 * Unit tests for USB MIDI event packet construction:
 *   - MIDI_EVENT macro (CIN encoding)
 *   - MIDI_CHANNEL macro (channel encoding)
 *   - Note-on and note-off packet field values
 *   - Cable number encoding in the Event byte
 *   - MIDI_STANDARD_VELOCITY constant
 *   - Packet struct layout (field offsets and sizes)
 */

/* Pull in stubs first so the firmware source can compile on Linux */
#define ARCH_AVR8 1
#define ARCH      ARCH_AVR8

/* Redirect AVR / LUFA headers to our stubs */
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

#include "test_framework.h"

/* =========================================================================
 * Helper: build a MIDI event packet the same way MIDI.c does
 * ========================================================================= */
static MIDI_EventPacket_t make_note_packet(uint8_t command,
                                           uint8_t channel_1based,
                                           uint8_t pitch,
                                           uint8_t velocity)
{
    uint8_t channel = MIDI_CHANNEL(channel_1based);
    MIDI_EventPacket_t pkt = {
        .Event = MIDI_EVENT(0, command),
        .Data1 = command | channel,
        .Data2 = pitch,
        .Data3 = velocity,
    };
    return pkt;
}

/* =========================================================================
 * Tests
 * ========================================================================= */

/* --- MIDI_EVENT macro ---------------------------------------------------- */

TEST(midi_packet, event_macro_note_on_cable_0)
{
    /* MIDI_EVENT(cable=0, MIDI_COMMAND_NOTE_ON=0x90)
     * CIN for 3-byte messages with status 0x9x is 0x09.
     * Event byte = (cable << 4) | (cmd >> 4) = 0 | 9 = 0x09 */
    uint8_t ev = MIDI_EVENT(0, MIDI_COMMAND_NOTE_ON);
    ASSERT_EQ(ev, 0x09);
}

TEST(midi_packet, event_macro_note_off_cable_0)
{
    /* MIDI_COMMAND_NOTE_OFF = 0x80 → CIN = 0x08 */
    uint8_t ev = MIDI_EVENT(0, MIDI_COMMAND_NOTE_OFF);
    ASSERT_EQ(ev, 0x08);
}

TEST(midi_packet, event_macro_note_on_cable_1)
{
    /* cable 1: high nibble = 1, low nibble = CIN 9 → 0x19 */
    uint8_t ev = MIDI_EVENT(1, MIDI_COMMAND_NOTE_ON);
    ASSERT_EQ(ev, 0x19);
}

TEST(midi_packet, event_macro_note_on_cable_15)
{
    /* cable 15: high nibble = 0xF, low = 9 → 0xF9 */
    uint8_t ev = MIDI_EVENT(15, MIDI_COMMAND_NOTE_ON);
    ASSERT_EQ(ev, 0xF9);
}

/* --- MIDI_CHANNEL macro -------------------------------------------------- */

TEST(midi_packet, channel_macro_ch1_is_zero)
{
    /* MIDI channel 1 maps to protocol value 0 */
    ASSERT_EQ(MIDI_CHANNEL(1), 0x00);
}

TEST(midi_packet, channel_macro_ch10_is_nine)
{
    /* MIDI channel 10 (percussion) maps to protocol value 9 */
    ASSERT_EQ(MIDI_CHANNEL(10), 0x09);
}

TEST(midi_packet, channel_macro_ch16_is_fifteen)
{
    ASSERT_EQ(MIDI_CHANNEL(16), 0x0F);
}

/* --- Note-on packet fields ----------------------------------------------- */

TEST(midi_packet, note_on_event_byte)
{
    MIDI_EventPacket_t pkt = make_note_packet(MIDI_COMMAND_NOTE_ON, 1, 0x3C, 64);
    /* Cable 0, Note-On → 0x09 */
    ASSERT_EQ(pkt.Event, 0x09);
}

TEST(midi_packet, note_on_data1_status_byte_ch1)
{
    MIDI_EventPacket_t pkt = make_note_packet(MIDI_COMMAND_NOTE_ON, 1, 0x3C, 64);
    /* 0x90 | 0x00 = 0x90 */
    ASSERT_EQ(pkt.Data1, 0x90);
}

TEST(midi_packet, note_on_data1_status_byte_ch10)
{
    MIDI_EventPacket_t pkt = make_note_packet(MIDI_COMMAND_NOTE_ON, 10, 0x3C, 64);
    /* 0x90 | 0x09 = 0x99 */
    ASSERT_EQ(pkt.Data1, 0x99);
}

TEST(midi_packet, note_on_pitch_stored_in_data2)
{
    MIDI_EventPacket_t pkt = make_note_packet(MIDI_COMMAND_NOTE_ON, 1, 0x3C, 64);
    ASSERT_EQ(pkt.Data2, 0x3C);
}

TEST(midi_packet, note_on_velocity_stored_in_data3)
{
    MIDI_EventPacket_t pkt = make_note_packet(MIDI_COMMAND_NOTE_ON, 1, 0x3C, 127);
    ASSERT_EQ(pkt.Data3, 127);
}

/* --- Note-off packet fields ---------------------------------------------- */

TEST(midi_packet, note_off_event_byte)
{
    MIDI_EventPacket_t pkt = make_note_packet(MIDI_COMMAND_NOTE_OFF, 1, 0x3C, 0);
    /* CIN for Note-Off: 0x08 */
    ASSERT_EQ(pkt.Event, 0x08);
}

TEST(midi_packet, note_off_data1_status_byte_ch1)
{
    MIDI_EventPacket_t pkt = make_note_packet(MIDI_COMMAND_NOTE_OFF, 1, 0x3C, 0);
    /* 0x80 | 0x00 = 0x80 */
    ASSERT_EQ(pkt.Data1, 0x80);
}

TEST(midi_packet, note_off_pitch_in_data2)
{
    MIDI_EventPacket_t pkt = make_note_packet(MIDI_COMMAND_NOTE_OFF, 1, 0x3F, 0);
    ASSERT_EQ(pkt.Data2, 0x3F);
}

TEST(midi_packet, note_off_velocity_zero)
{
    MIDI_EventPacket_t pkt = make_note_packet(MIDI_COMMAND_NOTE_OFF, 1, 0x3C, 0);
    ASSERT_EQ(pkt.Data3, 0);
}

/* --- MIDI_STANDARD_VELOCITY --------------------------------------------- */

TEST(midi_packet, standard_velocity_is_64)
{
    ASSERT_EQ(MIDI_STANDARD_VELOCITY, 64);
}

/* --- Packet struct layout ------------------------------------------------ */

TEST(midi_packet, packet_is_four_bytes)
{
    ASSERT_EQ(sizeof(MIDI_EventPacket_t), 4u);
}

TEST(midi_packet, event_field_is_first_byte)
{
    ASSERT_EQ(offsetof(MIDI_EventPacket_t, Event), 0u);
}

TEST(midi_packet, data1_is_second_byte)
{
    ASSERT_EQ(offsetof(MIDI_EventPacket_t, Data1), 1u);
}

TEST(midi_packet, data2_is_third_byte)
{
    ASSERT_EQ(offsetof(MIDI_EventPacket_t, Data2), 2u);
}

TEST(midi_packet, data3_is_fourth_byte)
{
    ASSERT_EQ(offsetof(MIDI_EventPacket_t, Data3), 3u);
}

/* --- Event byte distinctness -------------------------------------------- */

TEST(midi_packet, note_on_and_note_off_events_are_distinct)
{
    uint8_t on_ev  = MIDI_EVENT(0, MIDI_COMMAND_NOTE_ON);
    uint8_t off_ev = MIDI_EVENT(0, MIDI_COMMAND_NOTE_OFF);
    ASSERT_NE(on_ev, off_ev);
}

TEST(midi_packet, note_on_cin_is_9)
{
    /* Low nibble of MIDI_EVENT(0, NOTE_ON) must be 9 per USB MIDI spec */
    uint8_t ev = MIDI_EVENT(0, MIDI_COMMAND_NOTE_ON);
    ASSERT_EQ(ev & 0x0F, 0x09);
}

TEST(midi_packet, note_off_cin_is_8)
{
    uint8_t ev = MIDI_EVENT(0, MIDI_COMMAND_NOTE_OFF);
    ASSERT_EQ(ev & 0x0F, 0x08);
}

RUN_ALL_TESTS()
