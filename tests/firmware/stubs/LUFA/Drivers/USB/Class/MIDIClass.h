/*
 * LUFA/Drivers/USB/Class/MIDIClass.h stub
 *
 * Provides the MIDI_EventPacket_t structure, USB_ClassInfo_MIDI_Device_t,
 * MIDI command constants, MIDI_EVENT / MIDI_CHANNEL macros, and the
 * function stubs that record calls so tests can assert on them.
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

/* =========================================================================
 * MIDI command byte constants (upper nibble of status byte)
 * ========================================================================= */
#define MIDI_COMMAND_NOTE_OFF   0x80
#define MIDI_COMMAND_NOTE_ON    0x90
#define MIDI_COMMAND_AFTERTOUCH 0xA0
#define MIDI_COMMAND_CC         0xB0
#define MIDI_COMMAND_PROG       0xC0
#define MIDI_COMMAND_PRESSURE   0xD0
#define MIDI_COMMAND_PITCHBEND  0xE0

/* Standard velocity used by the firmware */
#define MIDI_STANDARD_VELOCITY  64

/* =========================================================================
 * MIDI_CHANNEL(n) — converts 1-based channel to 0-based low nibble
 * MIDI_EVENT(cable, cmd) — builds the Code Index Number (CIN) byte
 * ========================================================================= */
#define MIDI_CHANNEL(n)         ((uint8_t)((n) - 1))
#define MIDI_EVENT(cable, cmd)  ((uint8_t)(((cable) << 4) | ((cmd) >> 4)))

/* =========================================================================
 * USB MIDI event packet (4 bytes per the USB MIDI 1.0 spec)
 * ========================================================================= */
typedef struct {
    uint8_t Event;   /* CIN in low nibble; cable number in high nibble */
    uint8_t Data1;   /* status byte  (command | channel) */
    uint8_t Data2;   /* note / controller number         */
    uint8_t Data3;   /* velocity / value                 */
} MIDI_EventPacket_t;

/* =========================================================================
 * Endpoint config stub
 * ========================================================================= */
typedef struct {
    uint8_t  Address;
    uint16_t Size;
    uint8_t  Banks;
} USB_Endpoint_Table_t;

typedef struct {
    uint8_t              StreamingInterfaceNumber;
    USB_Endpoint_Table_t DataINEndpoint;
    USB_Endpoint_Table_t DataOUTEndpoint;
} MIDI_Device_Config_t;

typedef struct {
    MIDI_Device_Config_t Config;
    /* State — unused in stubs but must exist for initialiser syntax */
    struct { int dummy; } State;
} USB_ClassInfo_MIDI_Device_t;

/* =========================================================================
 * Stub call log — every sent packet is appended here
 * ========================================================================= */
#define MIDI_STUB_LOG_MAX 256

typedef struct {
    MIDI_EventPacket_t packet;
} MidiSendLogEntry;

extern MidiSendLogEntry g_midi_send_log[MIDI_STUB_LOG_MAX];
extern int              g_midi_send_count;
extern int              g_midi_flush_count;
extern bool             g_midi_send_ok;      /* return value to simulate */

static inline void midi_stub_reset(void)
{
    g_midi_send_count  = 0;
    g_midi_flush_count = 0;
    g_midi_send_ok     = true;
    memset(g_midi_send_log, 0, sizeof(g_midi_send_log));
}

/* =========================================================================
 * LUFA MIDI Device API stubs
 * ========================================================================= */
static inline bool MIDI_Device_SendEventPacket(USB_ClassInfo_MIDI_Device_t *iface,
                                               MIDI_EventPacket_t *pkt)
{
    (void)iface;
    if (g_midi_send_count < MIDI_STUB_LOG_MAX) {
        g_midi_send_log[g_midi_send_count].packet = *pkt;
        g_midi_send_count++;
    }
    return g_midi_send_ok;
}

static inline bool MIDI_Device_Flush(USB_ClassInfo_MIDI_Device_t *iface)
{
    (void)iface;
    g_midi_flush_count++;
    return true;
}

static inline bool MIDI_Device_ReceiveEventPacket(USB_ClassInfo_MIDI_Device_t *iface,
                                                  MIDI_EventPacket_t *pkt)
{
    (void)iface;
    (void)pkt;
    return false;   /* no incoming packets in tests */
}

static inline bool MIDI_Device_ConfigureEndpoints(USB_ClassInfo_MIDI_Device_t *iface)
{
    (void)iface;
    return true;
}

static inline void MIDI_Device_USBTask(USB_ClassInfo_MIDI_Device_t *iface)
{
    (void)iface;
}

static inline void MIDI_Device_ProcessControlRequest(USB_ClassInfo_MIDI_Device_t *iface)
{
    (void)iface;
}
