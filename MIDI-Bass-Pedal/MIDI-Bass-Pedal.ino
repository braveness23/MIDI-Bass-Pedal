// =============================================================================
// MIDI Bass Pedal — Full Implementation
// =============================================================================
//
// Hardware (Arduino Mega or similar):
//   Keys          : pins 22–34  (13 keys, wired to GND, INPUT_PULLUP)
//   Octave Down   : pin 19      (momentary button to GND, INPUT_PULLUP)
//   Octave Up     : pin 20      (momentary button to GND, INPUT_PULLUP)
//   Panic         : pin 18      (momentary button to GND, INPUT_PULLUP)
//   Mode Toggle   : pin 17      (momentary button to GND, INPUT_PULLUP — latches mono/poly)
//   Velocity Knob : A0          (10k pot: outer legs to 5V and GND, wiper to A0)
//   NeoPixels     : pin 6       (40-pixel strip, 5V power)
//
// Features:
//   - Per-key debounce
//   - Octave up/down (C0–C8), silences held notes on change
//   - Monophonic mode: last-note priority with note stack
//   - Polyphonic mode: full independent NoteOn/NoteOff per key
//   - Velocity set by potentiometer (never from key press force)
//   - Panic button: CC123 All-Notes-Off on all 16 channels
//   - NeoPixel feedback: note colour + velocity brightness
// =============================================================================

#include <MIDI.h>
#include <Adafruit_NeoPixel.h>

MIDI_CREATE_DEFAULT_INSTANCE();

// -----------------------------------------------------------------------------
// Pin assignments
// -----------------------------------------------------------------------------
static const int FIRST_KEY_PIN   = 22;
static const int KEY_COUNT       = 13;
static const int OCTAVE_DOWN_PIN = 19;
static const int OCTAVE_UP_PIN   = 20;
static const int PANIC_PIN       = 18;
static const int MODE_PIN        = 17;
static const int VELOCITY_PIN    = A0;

// -----------------------------------------------------------------------------
// NeoPixel
// -----------------------------------------------------------------------------
#define NEOPIXEL_PIN   6
#define NEOPIXEL_COUNT 40

Adafruit_NeoPixel pixels(NEOPIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// -----------------------------------------------------------------------------
// MIDI
// -----------------------------------------------------------------------------
static const byte MIDI_CHANNEL  = 1;
static const int  DEFAULT_OCTAVE = 3;   // C3–C4 covers typical bass range
static const int  OCTAVE_MIN    = 0;
static const int  OCTAVE_MAX    = 8;

// -----------------------------------------------------------------------------
// Timing
// -----------------------------------------------------------------------------
static const unsigned long DEBOUNCE_MS = 20;

// -----------------------------------------------------------------------------
// Note colours (one per pitch class, C through B)
// -----------------------------------------------------------------------------
struct RGB { uint8_t r, g, b; };

static const RGB NOTE_COLORS[12] = {
    {255,   0,   0},  // C  — Red
    {  0,   0, 255},  // C# — Blue
    {255, 255, 255},  // D  — White
    { 14, 194, 114},  // D# — Apple Green
    {254, 241,  28},  // E  — Canary Yellow
    {240, 129,  70},  // F  — Pumpkin Orange
    {180,   0, 255},  // F# — Purple
    {  0, 255, 128},  // G  — Mint
    {255, 100, 100},  // G# — Pink
    {  0, 200, 255},  // A  — Cyan
    {255, 165,   0},  // A# — Orange
    {200, 255,   0},  // B  — Lime
};

// -----------------------------------------------------------------------------
// Debounced button helper
// -----------------------------------------------------------------------------
struct Button {
    int           pin;
    bool          stable;   // debounced state (true = pressed)
    bool          raw;      // last raw reading
    unsigned long timer;
};

static Button btnOctaveDown = { OCTAVE_DOWN_PIN, false, false, 0 };
static Button btnOctaveUp   = { OCTAVE_UP_PIN,   false, false, 0 };
static Button btnPanic      = { PANIC_PIN,        false, false, 0 };
static Button btnMode       = { MODE_PIN,         false, false, 0 };

// Returns true once on the leading edge of a press.
bool updateButton(Button &btn) {
    bool reading = (digitalRead(btn.pin) == LOW);
    if (reading != btn.raw) {
        btn.raw   = reading;
        btn.timer = millis();
    }
    if ((millis() - btn.timer) >= DEBOUNCE_MS) {
        bool prev   = btn.stable;
        btn.stable  = btn.raw;
        if (btn.stable && !prev) {
            return true;  // press edge
        }
    }
    return false;
}

// -----------------------------------------------------------------------------
// Key debounce state
// -----------------------------------------------------------------------------
static bool          keyStable[KEY_COUNT];
static bool          keyRaw[KEY_COUNT];
static unsigned long keyTimer[KEY_COUNT];

// -----------------------------------------------------------------------------
// Voice state
// -----------------------------------------------------------------------------
static int  currentOctave = DEFAULT_OCTAVE;
static bool polyMode      = false;
static byte velocity      = 100;

// Mono mode: last-note-priority stack.
// Holds MIDI note numbers for all physically pressed keys, in press order.
static const int STACK_MAX = KEY_COUNT;
static int  noteStack[STACK_MAX];
static int  stackSize    = 0;
static int  monoActive   = -1;   // note currently sounding (-1 = silent)

// Poly mode: track the note number sent for each key slot so that NoteOff
// always matches the NoteOn even if the octave changed while held.
static int  polyNote[KEY_COUNT];  // -1 = not playing

// -----------------------------------------------------------------------------
// MIDI note number for a key index at the current octave.
// MIDI standard: C-1 = 0, C0 = 12, C4 (middle C) = 60.
// -----------------------------------------------------------------------------
static inline byte midiNoteFor(int keyIndex) {
    return (byte)((currentOctave + 1) * 12 + keyIndex);
}

// -----------------------------------------------------------------------------
// NeoPixel helpers
// -----------------------------------------------------------------------------
void showNoteColor(byte note, byte vel) {
    const RGB &c = NOTE_COLORS[note % 12];
    pixels.setBrightness((uint8_t)map(vel, 1, 127, 10, 255));
    for (int i = 0; i < NEOPIXEL_COUNT; i++) {
        pixels.setPixelColor(i, pixels.Color(c.r, c.g, c.b));
    }
    pixels.show();
}

void showBlack() {
    pixels.setBrightness(0);
    pixels.clear();
    pixels.show();
}

// Refresh pixel display: show colour for highest-priority active note, or black.
void refreshPixels() {
    if (polyMode) {
        // Show colour for the first active key found
        for (int i = 0; i < KEY_COUNT; i++) {
            if (polyNote[i] >= 0) {
                showNoteColor((byte)polyNote[i], velocity);
                return;
            }
        }
        showBlack();
    } else {
        if (monoActive >= 0) {
            showNoteColor((byte)monoActive, velocity);
        } else {
            showBlack();
        }
    }
}

// -----------------------------------------------------------------------------
// Panic — CC 123 (All Notes Off) on every channel
// -----------------------------------------------------------------------------
void sendPanic() {
    for (byte ch = 1; ch <= 16; ch++) {
        MIDI.sendControlChange(123, 0, ch);  // All Notes Off
        MIDI.sendControlChange(64,  0, ch);  // Sustain off (safety)
    }
    // Reset tracking state
    stackSize  = 0;
    monoActive = -1;
    for (int i = 0; i < KEY_COUNT; i++) {
        polyNote[i] = -1;
    }
    showBlack();
}

// -----------------------------------------------------------------------------
// Mono note stack operations
// -----------------------------------------------------------------------------
void stackPush(int note) {
    if (stackSize < STACK_MAX) {
        noteStack[stackSize++] = note;
    }
}

void stackRemove(int note) {
    for (int i = 0; i < stackSize; i++) {
        if (noteStack[i] == note) {
            for (int j = i; j < stackSize - 1; j++) {
                noteStack[j] = noteStack[j + 1];
            }
            stackSize--;
            return;
        }
    }
}

int stackTop() {
    return (stackSize > 0) ? noteStack[stackSize - 1] : -1;
}

// -----------------------------------------------------------------------------
// Key event handlers
// -----------------------------------------------------------------------------
void onKeyPressed(int idx) {
    byte note = midiNoteFor(idx);

    if (polyMode) {
        polyNote[idx] = note;
        MIDI.sendNoteOn(note, velocity, MIDI_CHANNEL);
    } else {
        // Silence current note, push new one, sound it
        if (monoActive >= 0) {
            MIDI.sendNoteOff((byte)monoActive, 0, MIDI_CHANNEL);
        }
        stackPush(note);
        MIDI.sendNoteOn(note, velocity, MIDI_CHANNEL);
        monoActive = note;
    }
    refreshPixels();
}

void onKeyReleased(int idx) {
    if (polyMode) {
        int note = polyNote[idx];
        if (note >= 0) {
            MIDI.sendNoteOff((byte)note, 0, MIDI_CHANNEL);
            polyNote[idx] = -1;
        }
    } else {
        byte note = midiNoteFor(idx);
        stackRemove(note);

        if (note == (byte)monoActive) {
            // Active note released — see if another key is still held
            MIDI.sendNoteOff(note, 0, MIDI_CHANNEL);
            int next = stackTop();
            if (next >= 0) {
                MIDI.sendNoteOn((byte)next, velocity, MIDI_CHANNEL);
                monoActive = next;
            } else {
                monoActive = -1;
            }
        }
        // If a non-active key was released, it was already removed from the
        // stack above; no NoteOff needed (it was never sounded).
    }
    refreshPixels();
}

// -----------------------------------------------------------------------------
// Velocity knob — read and map pot to MIDI range [1, 127]
// Velocity 0 is reserved as NoteOff; exclude it.
// -----------------------------------------------------------------------------
void readVelocity() {
    velocity = (byte)map(analogRead(VELOCITY_PIN), 0, 1023, 1, 127);
}

// -----------------------------------------------------------------------------
// Key scanning with per-key debounce
// -----------------------------------------------------------------------------
void scanKeys() {
    unsigned long now = millis();
    for (int i = 0; i < KEY_COUNT; i++) {
        bool reading = (digitalRead(FIRST_KEY_PIN + i) == LOW);
        if (reading != keyRaw[i]) {
            keyRaw[i]   = reading;
            keyTimer[i] = now;
        }
        if ((now - keyTimer[i]) >= DEBOUNCE_MS) {
            bool prev      = keyStable[i];
            keyStable[i]   = keyRaw[i];
            if (keyStable[i] && !prev) {
                onKeyPressed(i);
            } else if (!keyStable[i] && prev) {
                onKeyReleased(i);
            }
        }
    }
}

// -----------------------------------------------------------------------------
// Control button scanning
// -----------------------------------------------------------------------------
void scanButtons() {
    if (updateButton(btnOctaveDown)) {
        if (currentOctave > OCTAVE_MIN) {
            currentOctave--;
            sendPanic();  // avoid stuck notes at the old octave
        }
    }

    if (updateButton(btnOctaveUp)) {
        if (currentOctave < OCTAVE_MAX) {
            currentOctave++;
            sendPanic();
        }
    }

    if (updateButton(btnPanic)) {
        sendPanic();
    }

    if (updateButton(btnMode)) {
        polyMode = !polyMode;
        sendPanic();  // clean voice state on mode switch
    }
}

// -----------------------------------------------------------------------------
// Setup
// -----------------------------------------------------------------------------
void setup() {
    // Key pins
    for (int i = 0; i < KEY_COUNT; i++) {
        pinMode(FIRST_KEY_PIN + i, INPUT_PULLUP);
        keyStable[i] = false;
        keyRaw[i]    = false;
        keyTimer[i]  = 0;
        polyNote[i]  = -1;
    }

    // Control buttons
    pinMode(OCTAVE_DOWN_PIN, INPUT_PULLUP);
    pinMode(OCTAVE_UP_PIN,   INPUT_PULLUP);
    pinMode(PANIC_PIN,       INPUT_PULLUP);
    pinMode(MODE_PIN,        INPUT_PULLUP);

    // NeoPixels
    pixels.begin();
    pixels.clear();
    pixels.show();

    // MIDI — listen on all channels so incoming MIDI is processed if needed
    MIDI.begin(MIDI_CHANNEL_OMNI);

    // Startup animation: flash each note colour in sequence
    for (int i = 0; i < 12; i++) {
        showNoteColor(i, 100);
        delay(80);
    }
    showBlack();
}

// -----------------------------------------------------------------------------
// Main loop
// -----------------------------------------------------------------------------
void loop() {
    readVelocity();
    scanKeys();
    scanButtons();
    MIDI.read();   // process any incoming MIDI
}
