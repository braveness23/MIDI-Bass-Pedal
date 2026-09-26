/*
 * test_descriptors.c
 *
 * Unit tests for USB descriptor correctness.  We compile a thin
 * re-implementation of Descriptors.c (descriptors_impl.c) that uses the
 * Linux stubs in place of AVR/LUFA headers, then exercise:
 *
 *   - Device descriptor: VID, PID, bcdUSB, bcdDevice, class codes,
 *     control endpoint size, number of configurations, string indices
 *   - Configuration descriptor: total interfaces, config number,
 *     self-powered attribute, max power
 *   - Audio/MIDI streaming interface class codes
 *   - MIDI jack IDs, types, pin counts
 *   - Endpoint addresses and sizes
 *   - CALLBACK_USB_GetDescriptor: correct address and size returned for
 *     device / configuration / string descriptor requests
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

/* Pull in LUFAConfig.h first so FIXED_CONTROL_ENDPOINT_SIZE etc. are defined
 * before Descriptors.c is compiled (it uses these at file scope). */
#include "../../MIDI-Bass-Pedal/firmware/Config/LUFAConfig.h"

/* Pull in the descriptor constants defined in the firmware headers */
#include "../../MIDI-Bass-Pedal/firmware/Descriptors.h"

#include "test_framework.h"

/*
 * The firmware's Descriptors.c uses PROGMEM and pgm_read_byte; both are
 * defined as no-ops in our pgmspace.h stub so we can directly #include the
 * firmware source here for compilation.
 *
 * We rename main() to avoid a collision: the firmware has no main() in
 * Descriptors.c, so we can include it directly.
 */
#include "../../MIDI-Bass-Pedal/firmware/Descriptors.c"

/* =========================================================================
 * Convenience accessors
 * ========================================================================= */

/* CALLBACK_USB_GetDescriptor wValue helpers */
#define DESC_REQUEST(type, idx)  (uint16_t)(((type) << 8) | (idx))

/* =========================================================================
 * Device Descriptor tests
 * ========================================================================= */

TEST(descriptors, device_bcdUSB_is_1_1_0)
{
    /* VERSION_BCD(1,1,0) = 0x0110 */
    ASSERT_EQ(DeviceDescriptor.USBSpecification, VERSION_BCD(1, 1, 0));
    ASSERT_EQ(DeviceDescriptor.USBSpecification, 0x0110);
}

TEST(descriptors, device_vendor_id_is_atmel)
{
    /* 0x03EB is the Atmel VID used by the LUFA demo */
    ASSERT_EQ(DeviceDescriptor.VendorID, 0x03EB);
}

TEST(descriptors, device_product_id)
{
    ASSERT_EQ(DeviceDescriptor.ProductID, 0x2048);
}

TEST(descriptors, device_release_number)
{
    /* VERSION_BCD(0,0,1) = 0x0001 */
    ASSERT_EQ(DeviceDescriptor.ReleaseNumber, VERSION_BCD(0, 0, 1));
    ASSERT_EQ(DeviceDescriptor.ReleaseNumber, 0x0001);
}

TEST(descriptors, device_class_is_no_class)
{
    ASSERT_EQ(DeviceDescriptor.Class,    USB_CSCP_NoDeviceClass);
    ASSERT_EQ(DeviceDescriptor.SubClass, USB_CSCP_NoDeviceSubclass);
    ASSERT_EQ(DeviceDescriptor.Protocol, USB_CSCP_NoDeviceProtocol);
}

TEST(descriptors, device_endpoint0_size_is_8)
{
    ASSERT_EQ(DeviceDescriptor.Endpoint0Size, FIXED_CONTROL_ENDPOINT_SIZE);
    ASSERT_EQ(DeviceDescriptor.Endpoint0Size, 8);
}

TEST(descriptors, device_one_configuration)
{
    ASSERT_EQ(DeviceDescriptor.NumberOfConfigurations, FIXED_NUM_CONFIGURATIONS);
    ASSERT_EQ(DeviceDescriptor.NumberOfConfigurations, 1);
}

TEST(descriptors, device_manufacturer_string_index)
{
    ASSERT_EQ(DeviceDescriptor.ManufacturerStrIndex, STRING_ID_Manufacturer);
}

TEST(descriptors, device_product_string_index)
{
    ASSERT_EQ(DeviceDescriptor.ProductStrIndex, STRING_ID_Product);
}

TEST(descriptors, device_serial_is_no_descriptor)
{
    ASSERT_EQ(DeviceDescriptor.SerialNumStrIndex, NO_DESCRIPTOR);
}

TEST(descriptors, device_descriptor_type_byte)
{
    ASSERT_EQ(DeviceDescriptor.Header.Type, DTYPE_Device);
}

TEST(descriptors, device_descriptor_size_field)
{
    ASSERT_EQ(DeviceDescriptor.Header.Size, sizeof(USB_Descriptor_Device_t));
}

/* =========================================================================
 * Configuration Descriptor tests
 * ========================================================================= */

TEST(descriptors, config_number_of_interfaces_is_2)
{
    ASSERT_EQ(ConfigurationDescriptor.Config.TotalInterfaces, 2);
}

TEST(descriptors, config_configuration_number_is_1)
{
    ASSERT_EQ(ConfigurationDescriptor.Config.ConfigurationNumber, 1);
}

TEST(descriptors, config_is_self_powered)
{
    ASSERT_TRUE(ConfigurationDescriptor.Config.ConfigAttributes &
                USB_CONFIG_ATTR_SELFPOWERED);
}

TEST(descriptors, config_max_power_100ma)
{
    /* USB_CONFIG_POWER_MA(100) = 50 */
    ASSERT_EQ(ConfigurationDescriptor.Config.MaxPowerConsumption,
              USB_CONFIG_POWER_MA(100));
    ASSERT_EQ(ConfigurationDescriptor.Config.MaxPowerConsumption, 50);
}

TEST(descriptors, config_total_size_matches_struct)
{
    ASSERT_EQ(ConfigurationDescriptor.Config.TotalConfigurationSize,
              sizeof(USB_Descriptor_Configuration_t));
}

/* =========================================================================
 * Audio Control Interface
 * ========================================================================= */

TEST(descriptors, audio_control_interface_number_is_0)
{
    ASSERT_EQ(ConfigurationDescriptor.Audio_ControlInterface.InterfaceNumber,
              INTERFACE_ID_AudioControl);
    ASSERT_EQ(ConfigurationDescriptor.Audio_ControlInterface.InterfaceNumber, 0);
}

TEST(descriptors, audio_control_has_no_endpoints)
{
    ASSERT_EQ(ConfigurationDescriptor.Audio_ControlInterface.TotalEndpoints, 0);
}

TEST(descriptors, audio_control_class_codes)
{
    ASSERT_EQ(ConfigurationDescriptor.Audio_ControlInterface.Class,
              AUDIO_CSCP_AudioClass);
    ASSERT_EQ(ConfigurationDescriptor.Audio_ControlInterface.SubClass,
              AUDIO_CSCP_ControlSubclass);
}

/* =========================================================================
 * MIDI Streaming Interface
 * ========================================================================= */

TEST(descriptors, audio_stream_interface_number_is_1)
{
    ASSERT_EQ(ConfigurationDescriptor.Audio_StreamInterface.InterfaceNumber,
              INTERFACE_ID_AudioStream);
    ASSERT_EQ(ConfigurationDescriptor.Audio_StreamInterface.InterfaceNumber, 1);
}

TEST(descriptors, audio_stream_has_two_endpoints)
{
    ASSERT_EQ(ConfigurationDescriptor.Audio_StreamInterface.TotalEndpoints, 2);
}

TEST(descriptors, audio_stream_class_is_midi_streaming)
{
    ASSERT_EQ(ConfigurationDescriptor.Audio_StreamInterface.Class,
              AUDIO_CSCP_AudioClass);
    ASSERT_EQ(ConfigurationDescriptor.Audio_StreamInterface.SubClass,
              AUDIO_CSCP_MIDIStreamingSubclass);
}

/* =========================================================================
 * MIDI Jack descriptors
 * ========================================================================= */

TEST(descriptors, midi_in_jack_emb_type_and_id)
{
    ASSERT_EQ(ConfigurationDescriptor.MIDI_In_Jack_Emb.JackType,
              MIDI_JACKTYPE_Embedded);
    ASSERT_EQ(ConfigurationDescriptor.MIDI_In_Jack_Emb.JackID, 0x01);
}

TEST(descriptors, midi_in_jack_ext_type_and_id)
{
    ASSERT_EQ(ConfigurationDescriptor.MIDI_In_Jack_Ext.JackType,
              MIDI_JACKTYPE_External);
    ASSERT_EQ(ConfigurationDescriptor.MIDI_In_Jack_Ext.JackID, 0x02);
}

TEST(descriptors, midi_out_jack_emb_type_and_id)
{
    ASSERT_EQ(ConfigurationDescriptor.MIDI_Out_Jack_Emb.JackType,
              MIDI_JACKTYPE_Embedded);
    ASSERT_EQ(ConfigurationDescriptor.MIDI_Out_Jack_Emb.JackID, 0x03);
}

TEST(descriptors, midi_out_jack_ext_type_and_id)
{
    ASSERT_EQ(ConfigurationDescriptor.MIDI_Out_Jack_Ext.JackType,
              MIDI_JACKTYPE_External);
    ASSERT_EQ(ConfigurationDescriptor.MIDI_Out_Jack_Ext.JackID, 0x04);
}

TEST(descriptors, midi_out_jack_emb_source_is_ext_in)
{
    /* Embedded OUT jack pulls from External IN jack (ID 0x02) */
    ASSERT_EQ(ConfigurationDescriptor.MIDI_Out_Jack_Emb.NumberOfPins, 1);
    ASSERT_EQ(ConfigurationDescriptor.MIDI_Out_Jack_Emb.SourceJackID[0], 0x02);
    ASSERT_EQ(ConfigurationDescriptor.MIDI_Out_Jack_Emb.SourcePinID[0], 0x01);
}

TEST(descriptors, midi_out_jack_ext_source_is_emb_in)
{
    /* External OUT jack pulls from Embedded IN jack (ID 0x01) */
    ASSERT_EQ(ConfigurationDescriptor.MIDI_Out_Jack_Ext.NumberOfPins, 1);
    ASSERT_EQ(ConfigurationDescriptor.MIDI_Out_Jack_Ext.SourceJackID[0], 0x01);
    ASSERT_EQ(ConfigurationDescriptor.MIDI_Out_Jack_Ext.SourcePinID[0], 0x01);
}

/* =========================================================================
 * Endpoint tests
 * ========================================================================= */

TEST(descriptors, midi_out_endpoint_address)
{
    /* MIDI_STREAM_OUT_EPADDR = ENDPOINT_DIR_OUT | 1 = 0x01 */
    ASSERT_EQ(ConfigurationDescriptor.MIDI_In_Jack_Endpoint.Endpoint.EndpointAddress,
              MIDI_STREAM_OUT_EPADDR);
    ASSERT_EQ(ConfigurationDescriptor.MIDI_In_Jack_Endpoint.Endpoint.EndpointAddress,
              (ENDPOINT_DIR_OUT | 1));
}

TEST(descriptors, midi_in_endpoint_address)
{
    /* MIDI_STREAM_IN_EPADDR = ENDPOINT_DIR_IN | 2 = 0x82 */
    ASSERT_EQ(ConfigurationDescriptor.MIDI_Out_Jack_Endpoint.Endpoint.EndpointAddress,
              MIDI_STREAM_IN_EPADDR);
    ASSERT_EQ(ConfigurationDescriptor.MIDI_Out_Jack_Endpoint.Endpoint.EndpointAddress,
              (ENDPOINT_DIR_IN | 2));
}

TEST(descriptors, endpoint_size_is_64)
{
    ASSERT_EQ(ConfigurationDescriptor.MIDI_In_Jack_Endpoint.Endpoint.EndpointSize,
              MIDI_STREAM_EPSIZE);
    ASSERT_EQ(ConfigurationDescriptor.MIDI_In_Jack_Endpoint.Endpoint.EndpointSize,
              64);
}

TEST(descriptors, endpoints_are_bulk)
{
    ASSERT_TRUE(ConfigurationDescriptor.MIDI_In_Jack_Endpoint.Endpoint.Attributes
                & EP_TYPE_BULK);
    ASSERT_TRUE(ConfigurationDescriptor.MIDI_Out_Jack_Endpoint.Endpoint.Attributes
                & EP_TYPE_BULK);
}

/* =========================================================================
 * Jack endpoint SPC (class-specific endpoint companion)
 * ========================================================================= */

TEST(descriptors, in_jack_endpoint_spc_embedded_jack_id)
{
    ASSERT_EQ(ConfigurationDescriptor.MIDI_In_Jack_Endpoint_SPC.TotalEmbeddedJacks,
              0x01);
    ASSERT_EQ(ConfigurationDescriptor.MIDI_In_Jack_Endpoint_SPC.AssociatedJackID[0],
              0x01);
}

TEST(descriptors, out_jack_endpoint_spc_embedded_jack_id)
{
    ASSERT_EQ(ConfigurationDescriptor.MIDI_Out_Jack_Endpoint_SPC.TotalEmbeddedJacks,
              0x01);
    ASSERT_EQ(ConfigurationDescriptor.MIDI_Out_Jack_Endpoint_SPC.AssociatedJackID[0],
              0x03);
}

/* =========================================================================
 * String ID enum values
 * ========================================================================= */

TEST(descriptors, string_id_language_is_zero)
{
    ASSERT_EQ(STRING_ID_Language, 0);
}

TEST(descriptors, string_id_manufacturer_is_one)
{
    ASSERT_EQ(STRING_ID_Manufacturer, 1);
}

TEST(descriptors, string_id_product_is_two)
{
    ASSERT_EQ(STRING_ID_Product, 2);
}

/* =========================================================================
 * Interface ID enum values
 * ========================================================================= */

TEST(descriptors, interface_id_audio_control_is_zero)
{
    ASSERT_EQ(INTERFACE_ID_AudioControl, 0);
}

TEST(descriptors, interface_id_audio_stream_is_one)
{
    ASSERT_EQ(INTERFACE_ID_AudioStream, 1);
}

/* =========================================================================
 * CALLBACK_USB_GetDescriptor tests
 * ========================================================================= */

TEST(descriptors, get_descriptor_device_returns_correct_address)
{
    const void *addr = NULL;
    uint16_t size = CALLBACK_USB_GetDescriptor(
        DESC_REQUEST(DTYPE_Device, 0), 0, &addr);
    ASSERT_TRUE(addr == &DeviceDescriptor);
    ASSERT_EQ(size, sizeof(USB_Descriptor_Device_t));
}

TEST(descriptors, get_descriptor_configuration_returns_correct_address)
{
    const void *addr = NULL;
    uint16_t size = CALLBACK_USB_GetDescriptor(
        DESC_REQUEST(DTYPE_Configuration, 0), 0, &addr);
    ASSERT_TRUE(addr == &ConfigurationDescriptor);
    ASSERT_EQ(size, sizeof(USB_Descriptor_Configuration_t));
}

TEST(descriptors, get_descriptor_language_string)
{
    const void *addr = NULL;
    uint16_t size = CALLBACK_USB_GetDescriptor(
        DESC_REQUEST(DTYPE_String, STRING_ID_Language), 0, &addr);
    ASSERT_TRUE(addr == &LanguageString);
    ASSERT_NE(size, 0);
}

TEST(descriptors, get_descriptor_manufacturer_string)
{
    const void *addr = NULL;
    uint16_t size = CALLBACK_USB_GetDescriptor(
        DESC_REQUEST(DTYPE_String, STRING_ID_Manufacturer), 0, &addr);
    ASSERT_TRUE(addr == &ManufacturerString);
    ASSERT_NE(size, 0);
}

TEST(descriptors, get_descriptor_product_string)
{
    const void *addr = NULL;
    uint16_t size = CALLBACK_USB_GetDescriptor(
        DESC_REQUEST(DTYPE_String, STRING_ID_Product), 0, &addr);
    ASSERT_TRUE(addr == &ProductString);
    ASSERT_NE(size, 0);
}

TEST(descriptors, get_descriptor_unknown_type_returns_no_descriptor)
{
    const void *addr = (void *)0xDEAD;
    uint16_t size = CALLBACK_USB_GetDescriptor(
        DESC_REQUEST(0xFF, 0), 0, &addr);
    ASSERT_EQ(size, NO_DESCRIPTOR);
    ASSERT_TRUE(addr == NULL);
}

TEST(descriptors, get_descriptor_unknown_string_index_returns_no_descriptor)
{
    const void *addr = (void *)0xDEAD;
    uint16_t size = CALLBACK_USB_GetDescriptor(
        DESC_REQUEST(DTYPE_String, 0xFF), 0, &addr);
    ASSERT_EQ(size, NO_DESCRIPTOR);
    /* addr set to NULL before switch falls through */
    ASSERT_TRUE(addr == NULL);
}

RUN_ALL_TESTS()
