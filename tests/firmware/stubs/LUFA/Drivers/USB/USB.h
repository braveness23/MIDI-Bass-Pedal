/*
 * LUFA/Drivers/USB/USB.h stub
 *
 * Umbrella USB header.  Pulls in the MIDI class driver stub and provides
 * the USB descriptor types, constants, and macros referenced by the firmware.
 *
 * Struct layouts match the actual LUFA source so Descriptors.c compiles
 * unchanged.
 */
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "Class/MIDIClass.h"

/* =========================================================================
 * Compiler attribute macros (LUFA uses these in its own headers)
 * ========================================================================= */
#define ATTR_WARN_UNUSED_RESULT
#define ATTR_NON_NULL_PTR_ARG(...)

/* =========================================================================
 * USB spec version helper
 * VERSION_BCD(major, minor, rev) → BCD-encoded uint16_t
 * ========================================================================= */
#define VERSION_BCD(maj, min, rev) \
    ((uint16_t)(((maj) << 8) | (((min) & 0x0F) << 4) | ((rev) & 0x0F)))

/* =========================================================================
 * Descriptor type constants
 * ========================================================================= */
#define DTYPE_Device        0x01
#define DTYPE_Configuration 0x02
#define DTYPE_String        0x03
#define DTYPE_Interface     0x04
#define DTYPE_Endpoint      0x05
#define DTYPE_CSInterface   0x24
#define DTYPE_CSEndpoint    0x25

/* LUFA also defines AUDIO_DTYPE_* aliases that Descriptors.c uses */
#define AUDIO_DTYPE_CSInterface   DTYPE_CSInterface
#define AUDIO_DTYPE_CSEndpoint    DTYPE_CSEndpoint

/* =========================================================================
 * USB class / subclass / protocol codes
 * ========================================================================= */
#define USB_CSCP_NoDeviceClass      0x00
#define USB_CSCP_NoDeviceSubclass   0x00
#define USB_CSCP_NoDeviceProtocol   0x00

/* Audio class */
#define AUDIO_CSCP_AudioClass               0x01
#define AUDIO_CSCP_ControlSubclass          0x01
#define AUDIO_CSCP_ControlProtocol          0x00
#define AUDIO_CSCP_MIDIStreamingSubclass    0x03
#define AUDIO_CSCP_StreamingProtocol        0x00

/* =========================================================================
 * Audio descriptor subtypes
 * ========================================================================= */
#define AUDIO_DSUBTYPE_CSInterface_Header         0x01
#define AUDIO_DSUBTYPE_CSInterface_General        0x01
#define AUDIO_DSUBTYPE_CSInterface_InputTerminal  0x02
#define AUDIO_DSUBTYPE_CSInterface_OutputTerminal 0x03
#define AUDIO_DSUBTYPE_CSEndpoint_General         0x01

/* MIDI jack types */
#define MIDI_JACKTYPE_Embedded  0x01
#define MIDI_JACKTYPE_External  0x02

/* =========================================================================
 * Endpoint direction / type / attribute flags
 * ========================================================================= */
#define ENDPOINT_DIR_IN   0x80
#define ENDPOINT_DIR_OUT  0x00

#define EP_TYPE_BULK        0x02
#define EP_TYPE_INTERRUPT   0x03
#define EP_TYPE_ISOCHRONOUS 0x01
#define EP_TYPE_CONTROL     0x00

#define ENDPOINT_ATTR_NO_SYNC    0x00
#define ENDPOINT_USAGE_DATA      0x00

/* =========================================================================
 * Configuration descriptor attribute flags
 * ========================================================================= */
#define USB_CONFIG_ATTR_RESERVED     0x80
#define USB_CONFIG_ATTR_SELFPOWERED  0x40
#define USB_CONFIG_ATTR_REMOTEWAKEUP 0x20

#define USB_CONFIG_POWER_MA(ma)  ((ma) / 2)

/* =========================================================================
 * Language ID
 * ========================================================================= */
#define LANGUAGE_ID_ENG 0x0409

/* =========================================================================
 * NO_DESCRIPTOR sentinel
 * ========================================================================= */
#define NO_DESCRIPTOR  0

/* =========================================================================
 * Descriptor header (2-byte preamble common to every USB descriptor)
 * ========================================================================= */
typedef struct {
    uint8_t Size;
    uint8_t Type;
} USB_Descriptor_Header_t;

/* =========================================================================
 * Device descriptor
 * ========================================================================= */
typedef struct {
    USB_Descriptor_Header_t Header;

    uint16_t USBSpecification;
    uint8_t  Class;
    uint8_t  SubClass;
    uint8_t  Protocol;

    uint8_t  Endpoint0Size;

    uint16_t VendorID;
    uint16_t ProductID;
    uint16_t ReleaseNumber;

    uint8_t  ManufacturerStrIndex;
    uint8_t  ProductStrIndex;
    uint8_t  SerialNumStrIndex;

    uint8_t  NumberOfConfigurations;
} USB_Descriptor_Device_t;

/* =========================================================================
 * Configuration descriptor header
 * ========================================================================= */
typedef struct {
    USB_Descriptor_Header_t Header;

    uint16_t TotalConfigurationSize;
    uint8_t  TotalInterfaces;

    uint8_t  ConfigurationNumber;
    uint8_t  ConfigurationStrIndex;

    uint8_t  ConfigAttributes;
    uint8_t  MaxPowerConsumption;
} USB_Descriptor_Configuration_Header_t;

/* =========================================================================
 * Interface descriptor
 * ========================================================================= */
typedef struct {
    USB_Descriptor_Header_t Header;

    uint8_t InterfaceNumber;
    uint8_t AlternateSetting;

    uint8_t TotalEndpoints;

    uint8_t Class;
    uint8_t SubClass;
    uint8_t Protocol;

    uint8_t InterfaceStrIndex;
} USB_Descriptor_Interface_t;

/* =========================================================================
 * Audio Control interface descriptor (Class-Specific)
 * ========================================================================= */
typedef struct {
    USB_Descriptor_Header_t Header;
    uint8_t  Subtype;

    uint16_t ACSpecification;
    uint16_t TotalLength;

    uint8_t  InCollection;
    uint8_t  InterfaceNumber;
} USB_Audio_Descriptor_Interface_AC_t;

/* =========================================================================
 * MIDI Streaming interface descriptor (Class-Specific)
 * ========================================================================= */
typedef struct {
    USB_Descriptor_Header_t Header;
    uint8_t  Subtype;

    uint16_t AudioSpecification;
    uint16_t TotalLength;
} USB_MIDI_Descriptor_AudioInterface_AS_t;

/* =========================================================================
 * MIDI Input Jack descriptor
 * ========================================================================= */
typedef struct {
    USB_Descriptor_Header_t Header;
    uint8_t Subtype;

    uint8_t JackType;
    uint8_t JackID;

    uint8_t JackStrIndex;
} USB_MIDI_Descriptor_InputJack_t;

/* =========================================================================
 * MIDI Output Jack descriptor
 * ========================================================================= */
typedef struct {
    USB_Descriptor_Header_t Header;
    uint8_t Subtype;

    uint8_t JackType;
    uint8_t JackID;

    uint8_t  NumberOfPins;
    uint8_t  SourceJackID[1];
    uint8_t  SourcePinID[1];

    uint8_t JackStrIndex;
} USB_MIDI_Descriptor_OutputJack_t;

/* =========================================================================
 * Audio streaming endpoint (standard part)
 *
 * The LUFA struct wraps the core endpoint fields in a nested "Endpoint"
 * sub-struct, then adds audio-specific Refresh and SyncEndpointNumber
 * fields at the outer level.  Descriptors.c uses designator syntax like:
 *
 *   .MIDI_In_Jack_Endpoint = {
 *       .Endpoint = {
 *           .Header = { ... },
 *           .EndpointAddress = ...,
 *           ...
 *       },
 *       .Refresh = 0,
 *       .SyncEndpointNumber = 0,
 *   }
 * ========================================================================= */
typedef struct {
    USB_Descriptor_Header_t Header;

    uint8_t  EndpointAddress;
    uint8_t  Attributes;
    uint16_t EndpointSize;
    uint8_t  PollingIntervalMS;
} USB_Descriptor_Endpoint_t;

typedef struct {
    USB_Descriptor_Endpoint_t Endpoint;  /* nested core endpoint struct */

    uint8_t Refresh;
    uint8_t SyncEndpointNumber;
} USB_Audio_Descriptor_StreamEndpoint_Std_t;

/* =========================================================================
 * MIDI Jack endpoint (class-specific companion to audio endpoint)
 * ========================================================================= */
typedef struct {
    USB_Descriptor_Header_t Header;
    uint8_t Subtype;

    uint8_t TotalEmbeddedJacks;
    uint8_t AssociatedJackID[1];
} USB_MIDI_Descriptor_Jack_Endpoint_t;

/* =========================================================================
 * String descriptor helpers
 * ========================================================================= */
typedef struct {
    USB_Descriptor_Header_t Header;
    uint16_t UnicodeString[128];
} USB_Descriptor_String_t;

/* Count UTF-16 code units in a wide string literal at compile time */
#define _WSTRLEN(ws)  (sizeof(ws) / sizeof(wchar_t) - 1)

#define USB_STRING_DESCRIPTOR(ws) \
    { \
        .Header = { \
            .Size = (uint8_t)(sizeof(USB_Descriptor_Header_t) + \
                              _WSTRLEN(ws) * sizeof(uint16_t)), \
            .Type = DTYPE_String \
        } \
    }

#define USB_STRING_DESCRIPTOR_ARRAY(...) \
    { \
        .Header = { \
            .Size = (uint8_t)(sizeof(USB_Descriptor_Header_t) + \
                              sizeof((uint16_t[]){__VA_ARGS__})), \
            .Type = DTYPE_String \
        }, \
        .UnicodeString = {__VA_ARGS__} \
    }

/* =========================================================================
 * USB device / host task stubs
 * ========================================================================= */
extern int g_usb_initialized;

static inline void USB_Init(void)    { g_usb_initialized = 1; }
static inline void USB_USBTask(void) {}

/* USB device option constants */
#define USB_DEVICE_OPT_FULLSPEED  0x01
#define USB_OPT_REG_ENABLED       0x02
#define USB_OPT_AUTO_PLL          0x04
