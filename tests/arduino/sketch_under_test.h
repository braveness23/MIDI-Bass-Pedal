// =============================================================================
// sketch_under_test.h
//
// Pull in the mock Arduino environment, then include the sketch source so it
// becomes part of this translation unit.  The tests include THIS header, not
// the .ino directly, so they get all the sketch globals and helper functions.
//
// The sketch's setup() / loop() are declared here as well, so test files can
// call them when needed.
// =============================================================================
#pragma once

// 1. Mocks must come first — they define Arduino.h surface before the sketch
//    includes anything.
#include "mock_arduino.h"

// 2. The sketch includes <MIDI.h> and <Adafruit_NeoPixel.h>.  We intercept
//    those with our own stubs (already defined in mock_arduino.h).  Guard the
//    fake include paths with these macros so the sketch's #include lines
//    resolve to our stubs.
#define MIDI_h            // prevent a real MIDI.h from loading
#define Adafruit_NeoPixel_h  // same for NeoPixel

// 3. Redirect the sketch's library #include statements to our stubs.
//    Because we defined the header-guard macros above, the real files (if
//    installed) won't be included.  The structs/macros they export are already
//    in mock_arduino.h, so we don't need separate shim files.

// 4. Include the sketch.  We rename the extension to .ino at the source but
//    g++ treats it as plain C++ when given explicitly.
#include "../../MIDI-Bass-Pedal/MIDI-Bass-Pedal.ino"

// ---------------------------------------------------------------------------
// Test-reset helper: restores all sketch-level state to its initial values
// so each test begins from a clean slate without relaunching the process.
// ---------------------------------------------------------------------------
inline void resetSketchState() {
    // Time
    g_mock_millis = 0;

    // GPIO — all pins released (HIGH = not pressed)
    for (int i = 0; i < 256; i++) g_pin_values[i] = HIGH;
    for (int i = 0; i < 256; i++) g_analog_values[i] = 0;

    // MIDI log
    clearMidiLog();

    // Key debounce arrays
    for (int i = 0; i < KEY_COUNT; i++) {
        keyStable[i] = false;
        keyRaw[i]    = false;
        keyTimer[i]  = 0;
        polyNote[i]  = -1;
    }

    // Buttons
    btnOctaveDown = { OCTAVE_DOWN_PIN, false, false, 0 };
    btnOctaveUp   = { OCTAVE_UP_PIN,   false, false, 0 };
    btnPanic      = { PANIC_PIN,       false, false, 0 };
    btnMode       = { MODE_PIN,        false, false, 0 };

    // Voice state
    currentOctave = DEFAULT_OCTAVE;
    polyMode      = false;
    velocity      = 100;

    // Mono stack
    stackSize  = 0;
    monoActive = -1;
    memset(noteStack, 0, sizeof(noteStack));
}

// ---------------------------------------------------------------------------
// Simulate enough scan cycles for debounce to settle.
// 'holdMs' is how many milliseconds the pin is held pressed/released.
// After calling this the scanKeys()/scanButtons() state machines will have
// processed the leading and trailing edges fully.
// ---------------------------------------------------------------------------
inline void tickScanKeys(unsigned long ms) {
    unsigned long target = g_mock_millis + ms;
    while (g_mock_millis < target) {
        scanKeys();
        g_mock_millis++;
    }
}

inline void tickScanButtons(unsigned long ms) {
    unsigned long target = g_mock_millis + ms;
    while (g_mock_millis < target) {
        scanButtons();
        g_mock_millis++;
    }
}
