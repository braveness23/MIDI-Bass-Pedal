// =============================================================================
// mock_arduino.h — Stub headers for Arduino + MIDI + NeoPixel APIs
//
// Allows MIDI-Bass-Pedal.ino logic to be compiled and tested on Linux
// with g++ without any real hardware or Arduino SDK installed.
// =============================================================================
#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <string>

// ---------------------------------------------------------------------------
// Basic Arduino types
// ---------------------------------------------------------------------------
typedef uint8_t  byte;
typedef uint16_t word;
typedef bool     boolean;

// ---------------------------------------------------------------------------
// Arduino constants
// ---------------------------------------------------------------------------
static const int INPUT        = 0;
static const int OUTPUT       = 1;
static const int INPUT_PULLUP = 2;
static const int LOW          = 0;
static const int HIGH         = 1;
static const int A0           = 100;   // any sentinel value beyond real pins

// ---------------------------------------------------------------------------
// Simulated time (millis)
// ---------------------------------------------------------------------------
static unsigned long g_mock_millis = 0;

inline unsigned long millis() { return g_mock_millis; }

inline void advanceMillis(unsigned long ms) { g_mock_millis += ms; }

// ---------------------------------------------------------------------------
// GPIO simulation
// ---------------------------------------------------------------------------
// 256-pin GPIO table; tests write pin values here; code reads from here.
static int g_pin_values[256] = {};   // default HIGH (pulled up)

// Initialise all pins to HIGH (unpressed, because INPUT_PULLUP)
struct PinTableInit {
    PinTableInit() { for (int i = 0; i < 256; i++) g_pin_values[i] = HIGH; }
};
static PinTableInit _pinInit;

inline void pinMode(int pin, int mode) { (void)pin; (void)mode; }

inline int digitalRead(int pin) {
    if (pin < 0 || pin >= 256) return HIGH;
    return g_pin_values[pin];
}

inline void digitalWrite(int pin, int val) {
    if (pin >= 0 && pin < 256) g_pin_values[pin] = val;
}

// Simulate pressing / releasing a pin (active-low, INPUT_PULLUP)
inline void mockPinPress(int pin)   { g_pin_values[pin] = LOW;  }
inline void mockPinRelease(int pin) { g_pin_values[pin] = HIGH; }

// ---------------------------------------------------------------------------
// analogRead simulation
// ---------------------------------------------------------------------------
static int g_analog_values[256] = {};

inline int analogRead(int pin) {
    if (pin < 0 || pin >= 256) return 0;
    return g_analog_values[pin];
}

inline void mockAnalogWrite(int pin, int val) {
    if (pin >= 0 && pin < 256) g_analog_values[pin] = val;
}

// ---------------------------------------------------------------------------
// map() — identical to Arduino's implementation
// ---------------------------------------------------------------------------
inline long map(long x, long in_min, long in_max, long out_min, long out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// ---------------------------------------------------------------------------
// delay / delayMicroseconds stubs
// ---------------------------------------------------------------------------
inline void delay(unsigned long ms) { g_mock_millis += ms; }
inline void delayMicroseconds(unsigned int us) { (void)us; }

// ---------------------------------------------------------------------------
// Serial stub
// ---------------------------------------------------------------------------
struct SerialClass {
    void begin(long) {}
    void print(const char *) {}
    void println(const char *) {}
    void print(int) {}
    void println(int) {}
};
[[maybe_unused]] static SerialClass Serial;

// ---------------------------------------------------------------------------
// MIDI message log — every sent message is appended here so tests can assert
// on the exact sequence of MIDI events.
// ---------------------------------------------------------------------------
enum class MidiMsgType { NoteOn, NoteOff, ControlChange };

struct MidiMessage {
    MidiMsgType type;
    byte        data1;   // note / CC number
    byte        data2;   // velocity / CC value
    byte        channel;
};

static std::vector<MidiMessage> g_midi_log;

inline void clearMidiLog() { g_midi_log.clear(); }

// ---------------------------------------------------------------------------
// Stub MIDI library — thin wrapper that records messages
// ---------------------------------------------------------------------------
struct MidiInterface {
    // Callbacks registered by the sketch
    void (*onNoteOn)(byte, byte, byte)   = nullptr;
    void (*onNoteOff)(byte, byte, byte)  = nullptr;

    void sendNoteOn(byte note, byte vel, byte ch) {
        g_midi_log.push_back({MidiMsgType::NoteOn, note, vel, ch});
    }
    void sendNoteOff(byte note, byte vel, byte ch) {
        g_midi_log.push_back({MidiMsgType::NoteOff, note, vel, ch});
    }
    void sendControlChange(byte cc, byte val, byte ch) {
        g_midi_log.push_back({MidiMsgType::ControlChange, cc, val, ch});
    }
    void setHandleNoteOn(void (*cb)(byte, byte, byte))  { onNoteOn  = cb; }
    void setHandleNoteOff(void (*cb)(byte, byte, byte)) { onNoteOff = cb; }
    void begin(byte) {}
    bool read() { return false; }
};

static MidiInterface MIDI;

// Macro the sketch uses to create its MIDI instance — we've already created it
#define MIDI_CREATE_DEFAULT_INSTANCE()  /* no-op; MIDI is the global above */
#define MIDI_CHANNEL_OMNI 0

// ---------------------------------------------------------------------------
// Adafruit NeoPixel stub
// ---------------------------------------------------------------------------
#define NEO_GRB   0x06
#define NEO_KHZ800 0x00

struct Adafruit_NeoPixel {
    int  numPixels;
    int  pin;
    int  flags;
    uint8_t brightness = 0;

    Adafruit_NeoPixel(int n, int p, int f) : numPixels(n), pin(p), flags(f) {}

    void begin()   {}
    void clear()   {}
    void show()    {}
    void setBrightness(uint8_t b) { brightness = b; }
    void setPixelColor(int, uint32_t) {}
    uint32_t Color(uint8_t r, uint8_t g, uint8_t b) {
        return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
    }
};
