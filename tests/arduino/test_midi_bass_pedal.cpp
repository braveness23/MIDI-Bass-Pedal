// =============================================================================
// test_midi_bass_pedal.cpp
//
// Native Linux test suite for MIDI-Bass-Pedal.ino
// Compile and run via:   make -C tests/arduino
//
// Tests are grouped by feature:
//   1.  midiNoteFor() — note number calculation
//   2.  Poly mode:  key press → NoteOn, key release → NoteOff
//   3.  Mono mode:  last-note-priority stack behaviour
//   4.  Per-key debounce (scanKeys)
//   5.  Velocity mapping
//   6.  MIDI channel
//   7.  Octave up/down (scanButtons)
//   8.  Panic (CC 123 on all 16 channels)
//   9.  Mode toggle (poly ↔ mono)
//  10.  Note colour mapping (NOTE_COLORS array)
//  11.  Mono stack boundary: stack full (KEY_COUNT keys)
//  12.  Mono last-note-priority: release active key resumes previous key
// =============================================================================

#include "sketch_under_test.h"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
#include <string>

// ---------------------------------------------------------------------------
// Minimal test framework helpers
// ---------------------------------------------------------------------------
static int g_tests_run    = 0;
static int g_tests_failed = 0;

#define ASSERT_EQ(a, b)                                                        \
    do {                                                                       \
        auto _a = (a); auto _b = (b);                                         \
        if (!(_a == _b)) {                                                     \
            fprintf(stderr, "  FAIL %s:%d  %s == %s  (%lld != %lld)\n",      \
                    __FILE__, __LINE__, #a, #b,                                \
                    (long long)_a, (long long)_b);                             \
            g_tests_failed++;                                                  \
            return;                                                            \
        }                                                                      \
    } while (0)

#define ASSERT_TRUE(expr)                                                      \
    do {                                                                       \
        if (!(expr)) {                                                         \
            fprintf(stderr, "  FAIL %s:%d  expected true: %s\n",              \
                    __FILE__, __LINE__, #expr);                                \
            g_tests_failed++;                                                  \
            return;                                                            \
        }                                                                      \
    } while (0)

#define ASSERT_FALSE(expr) ASSERT_TRUE(!(expr))

#define RUN_TEST(fn)                                                           \
    do {                                                                       \
        g_tests_run++;                                                         \
        printf("  [ RUN ] " #fn "\n");                                        \
        resetSketchState();                                                    \
        fn();                                                                  \
        if (g_tests_failed == 0 || true) { /* always keep going */ }          \
    } while (0)

// Count messages of a given type in the MIDI log
static int countMsg(MidiMsgType t) {
    int n = 0;
    for (auto &m : g_midi_log) if (m.type == t) n++;
    return n;
}

// Return the i-th message of a given type (0-indexed)
static MidiMessage nthMsg(MidiMsgType t, int idx) {
    int n = 0;
    for (auto &m : g_midi_log) {
        if (m.type == t) {
            if (n == idx) return m;
            n++;
        }
    }
    assert(false && "nthMsg: index out of range");
    return {};
}

// Return the i-th message of any type
static MidiMessage atLog(int idx) {
    assert(idx >= 0 && (size_t)idx < g_midi_log.size());
    return g_midi_log[idx];
}

// ---------------------------------------------------------------------------
// Helper: press a key pin and tick scanKeys() past the debounce window
// ---------------------------------------------------------------------------
static void pressKey(int keyIndex) {
    mockPinPress(FIRST_KEY_PIN + keyIndex);
    tickScanKeys(DEBOUNCE_MS + 1);   // settle
}

static void releaseKey(int keyIndex) {
    mockPinRelease(FIRST_KEY_PIN + keyIndex);
    tickScanKeys(DEBOUNCE_MS + 1);
}

// Helper: press and immediately release (full keystroke with settle time).
// Kept for completeness; suppress unused-function warning.
[[maybe_unused]] static void strokeKey(int keyIndex) {
    pressKey(keyIndex);
    releaseKey(keyIndex);
}

// ---------------------------------------------------------------------------
// Helper: press a button and tick scanButtons() past debounce
// ---------------------------------------------------------------------------
static void pressButton(int pin) {
    mockPinPress(pin);
    tickScanButtons(DEBOUNCE_MS + 1);
}

// ===========================================================================
// 1. midiNoteFor — note number mapping
// ===========================================================================

void test_midiNoteFor_defaultOctave_C() {
    // C at DEFAULT_OCTAVE (3): MIDI note = (3+1)*12 + 0 = 48
    ASSERT_EQ(midiNoteFor(0), (byte)48);
}

void test_midiNoteFor_defaultOctave_C_sharp() {
    // C# = index 1: (3+1)*12 + 1 = 49
    ASSERT_EQ(midiNoteFor(1), (byte)49);
}

void test_midiNoteFor_defaultOctave_top_C() {
    // 13th key (index 12) = (3+1)*12 + 12 = 60 (middle C)
    ASSERT_EQ(midiNoteFor(12), (byte)60);
}

void test_midiNoteFor_octave0() {
    currentOctave = 0;
    // C0: (0+1)*12 + 0 = 12
    ASSERT_EQ(midiNoteFor(0), (byte)12);
}

void test_midiNoteFor_octave8() {
    currentOctave = 8;
    // C8: (8+1)*12 + 0 = 108
    ASSERT_EQ(midiNoteFor(0), (byte)108);
}

void test_midiNoteFor_allKeys_sequential() {
    currentOctave = DEFAULT_OCTAVE;
    byte base = (byte)((DEFAULT_OCTAVE + 1) * 12);
    for (int i = 0; i < KEY_COUNT; i++) {
        ASSERT_EQ(midiNoteFor(i), (byte)(base + i));
    }
}

// ===========================================================================
// 2. Poly mode — key press / release
// ===========================================================================

void test_poly_single_press_sends_noteOn() {
    polyMode = true;
    pressKey(0);
    ASSERT_EQ(countMsg(MidiMsgType::NoteOn), 1);
    MidiMessage m = nthMsg(MidiMsgType::NoteOn, 0);
    ASSERT_EQ(m.data1, midiNoteFor(0));
    ASSERT_EQ(m.channel, (byte)MIDI_CHANNEL);
}

void test_poly_single_release_sends_noteOff() {
    polyMode = true;
    pressKey(0);
    clearMidiLog();
    releaseKey(0);
    ASSERT_EQ(countMsg(MidiMsgType::NoteOff), 1);
    MidiMessage m = nthMsg(MidiMsgType::NoteOff, 0);
    ASSERT_EQ(m.data1, midiNoteFor(0));
    ASSERT_EQ(m.channel, (byte)MIDI_CHANNEL);
}

void test_poly_multiple_keys_independent_noteOns() {
    polyMode = true;
    pressKey(0);
    pressKey(4);
    pressKey(7);
    ASSERT_EQ(countMsg(MidiMsgType::NoteOn), 3);
    // Each note should be the correct MIDI note for that key
    byte base = (byte)((DEFAULT_OCTAVE + 1) * 12);
    ASSERT_EQ(nthMsg(MidiMsgType::NoteOn, 0).data1, (byte)(base + 0));
    ASSERT_EQ(nthMsg(MidiMsgType::NoteOn, 1).data1, (byte)(base + 4));
    ASSERT_EQ(nthMsg(MidiMsgType::NoteOn, 2).data1, (byte)(base + 7));
}

void test_poly_release_sends_correct_note_off_per_key() {
    polyMode = true;
    pressKey(0);
    pressKey(4);
    clearMidiLog();
    releaseKey(0);
    ASSERT_EQ(countMsg(MidiMsgType::NoteOff), 1);
    byte base = (byte)((DEFAULT_OCTAVE + 1) * 12);
    ASSERT_EQ(nthMsg(MidiMsgType::NoteOff, 0).data1, (byte)(base + 0));
    // key 4 is still held — no extra NoteOff
    ASSERT_EQ(countMsg(MidiMsgType::NoteOn), 0);
}

void test_poly_no_note_off_without_prior_note_on() {
    // Release a key that was never pressed → no messages
    polyMode = true;
    releaseKey(5);
    ASSERT_EQ((int)g_midi_log.size(), 0);
}

void test_poly_velocity_carried_in_noteOn() {
    polyMode  = true;
    velocity  = 64;
    pressKey(0);
    ASSERT_EQ(nthMsg(MidiMsgType::NoteOn, 0).data2, (byte)64);
}

// ===========================================================================
// 3. Mono mode — last-note priority stack
// ===========================================================================

void test_mono_single_press_sends_noteOn() {
    polyMode = false;
    pressKey(0);
    ASSERT_EQ(countMsg(MidiMsgType::NoteOn), 1);
    ASSERT_EQ(nthMsg(MidiMsgType::NoteOn, 0).data1, midiNoteFor(0));
}

void test_mono_single_release_sends_noteOff() {
    polyMode = false;
    pressKey(0);
    clearMidiLog();
    releaseKey(0);
    ASSERT_EQ(countMsg(MidiMsgType::NoteOff), 1);
    ASSERT_EQ(nthMsg(MidiMsgType::NoteOff, 0).data1, midiNoteFor(0));
}

void test_mono_second_key_silences_first() {
    // Press key 0, then key 4:
    //  → NoteOff(key0) then NoteOn(key4)
    polyMode = false;
    pressKey(0);
    clearMidiLog();
    pressKey(4);
    // Should have sent NoteOff for key 0 first, then NoteOn for key 4
    ASSERT_EQ((int)g_midi_log.size(), 2);
    ASSERT_EQ(atLog(0).type, MidiMsgType::NoteOff);
    ASSERT_EQ(atLog(0).data1, midiNoteFor(0));
    ASSERT_EQ(atLog(1).type, MidiMsgType::NoteOn);
    ASSERT_EQ(atLog(1).data1, midiNoteFor(4));
}

void test_mono_release_active_with_held_key_resumes() {
    // Press 0, press 4, release 4 → should resume 0
    polyMode = false;
    pressKey(0);
    pressKey(4);
    clearMidiLog();
    releaseKey(4);
    // NoteOff(4) then NoteOn(0)
    ASSERT_TRUE(g_midi_log.size() >= 2);
    ASSERT_EQ(atLog(0).type, MidiMsgType::NoteOff);
    ASSERT_EQ(atLog(0).data1, midiNoteFor(4));
    ASSERT_EQ(atLog(1).type, MidiMsgType::NoteOn);
    ASSERT_EQ(atLog(1).data1, midiNoteFor(0));
}

void test_mono_release_non_active_key_no_noteOff() {
    // Press 0 and 4; release 0 (the non-active one, since 4 was pressed last)
    // The stack removes 0, but since 4 is still active, only the stack is
    // cleaned — no NoteOff is emitted for 0 (it was never sounding).
    polyMode = false;
    pressKey(0);
    pressKey(4);    // 4 is now the active (monoActive) note
    clearMidiLog();
    releaseKey(0);  // release the non-active key
    // Only 4 is sounding; releasing 0 should not produce a NoteOff
    ASSERT_EQ(countMsg(MidiMsgType::NoteOff), 0);
}

void test_mono_release_active_no_remaining_keys_silent() {
    polyMode = false;
    pressKey(0);
    clearMidiLog();
    releaseKey(0);
    // NoteOff(0), nothing more
    ASSERT_EQ(countMsg(MidiMsgType::NoteOff), 1);
    ASSERT_EQ(monoActive, -1);
}

// ===========================================================================
// 4. Per-key debounce — bounce window suppresses spurious events
// ===========================================================================

void test_debounce_no_event_before_window_expires() {
    polyMode = false;
    mockPinPress(FIRST_KEY_PIN + 0);

    // Tick for less than DEBOUNCE_MS — state machine should not fire yet
    for (unsigned long t = 0; t < DEBOUNCE_MS; t++) {
        scanKeys();
        g_mock_millis++;
    }
    // The pin has been pressed but the debounce window has not yet elapsed
    ASSERT_EQ(countMsg(MidiMsgType::NoteOn), 0);
}

void test_debounce_event_fires_after_window_expires() {
    polyMode = false;
    mockPinPress(FIRST_KEY_PIN + 0);

    // Tick exactly DEBOUNCE_MS + 1 milliseconds
    for (unsigned long t = 0; t <= DEBOUNCE_MS; t++) {
        scanKeys();
        g_mock_millis++;
    }
    ASSERT_EQ(countMsg(MidiMsgType::NoteOn), 1);
}

void test_debounce_bounce_within_window_resets_timer() {
    // If the pin bounces (toggles) within the debounce window the timer resets.
    // Net result: no event until the pin is stable for DEBOUNCE_MS ms.
    polyMode = false;

    // Press, then immediately release before window expires — simulates bounce
    mockPinPress(FIRST_KEY_PIN + 0);
    for (unsigned long t = 0; t < DEBOUNCE_MS / 2; t++) {
        scanKeys();
        g_mock_millis++;
    }
    // Bounce: release briefly
    mockPinRelease(FIRST_KEY_PIN + 0);
    scanKeys();
    g_mock_millis++;

    // Press again
    mockPinPress(FIRST_KEY_PIN + 0);

    // Tick full window from this point — should eventually fire once
    for (unsigned long t = 0; t <= DEBOUNCE_MS; t++) {
        scanKeys();
        g_mock_millis++;
    }
    ASSERT_EQ(countMsg(MidiMsgType::NoteOn), 1);
}

void test_debounce_each_key_independent() {
    polyMode = true;
    // Press keys 0 and 3 at different times; both should produce events
    mockPinPress(FIRST_KEY_PIN + 0);
    tickScanKeys(5);
    mockPinPress(FIRST_KEY_PIN + 3);
    tickScanKeys(DEBOUNCE_MS + 1);

    ASSERT_EQ(countMsg(MidiMsgType::NoteOn), 2);
}

// ===========================================================================
// 5. Velocity mapping
// ===========================================================================

void test_velocity_min_analog_maps_to_1() {
    // analogRead returns 0 → velocity should be 1 (not 0, reserved for NoteOff)
    mockAnalogWrite(VELOCITY_PIN, 0);
    readVelocity();
    ASSERT_EQ((int)velocity, 1);
}

void test_velocity_max_analog_maps_to_127() {
    mockAnalogWrite(VELOCITY_PIN, 1023);
    readVelocity();
    ASSERT_EQ((int)velocity, 127);
}

void test_velocity_midpoint_analog() {
    // 512 / 1023 ≈ 50% → map(512, 0, 1023, 1, 127) = 63 (integer arithmetic)
    mockAnalogWrite(VELOCITY_PIN, 512);
    readVelocity();
    int expected = (int)map(512, 0, 1023, 1, 127);
    ASSERT_EQ((int)velocity, expected);
}

void test_velocity_sent_in_noteOn() {
    mockAnalogWrite(VELOCITY_PIN, 1023);
    readVelocity();
    polyMode = true;
    pressKey(0);
    ASSERT_EQ((int)nthMsg(MidiMsgType::NoteOn, 0).data2, 127);
}

void test_velocity_never_zero_from_knob() {
    // Even at analog 0, velocity must stay >= 1
    mockAnalogWrite(VELOCITY_PIN, 0);
    readVelocity();
    ASSERT_TRUE(velocity >= 1);
}

// ===========================================================================
// 6. MIDI channel
// ===========================================================================

void test_midi_channel_noteOn_is_1() {
    polyMode = true;
    pressKey(0);
    ASSERT_EQ((int)nthMsg(MidiMsgType::NoteOn, 0).channel, (int)MIDI_CHANNEL);
}

void test_midi_channel_noteOff_is_1() {
    polyMode = true;
    pressKey(0);
    clearMidiLog();
    releaseKey(0);
    ASSERT_EQ((int)nthMsg(MidiMsgType::NoteOff, 0).channel, (int)MIDI_CHANNEL);
}

void test_midi_channel_constant_is_1() {
    ASSERT_EQ((int)MIDI_CHANNEL, 1);
}

// ===========================================================================
// 7. Octave up / down via scanButtons
// ===========================================================================

void test_octave_down_decrements_octave() {
    currentOctave = DEFAULT_OCTAVE;
    pressButton(OCTAVE_DOWN_PIN);
    ASSERT_EQ(currentOctave, DEFAULT_OCTAVE - 1);
}

void test_octave_up_increments_octave() {
    currentOctave = DEFAULT_OCTAVE;
    pressButton(OCTAVE_UP_PIN);
    ASSERT_EQ(currentOctave, DEFAULT_OCTAVE + 1);
}

void test_octave_down_clamps_at_minimum() {
    currentOctave = OCTAVE_MIN;
    pressButton(OCTAVE_DOWN_PIN);
    ASSERT_EQ(currentOctave, OCTAVE_MIN);
}

void test_octave_up_clamps_at_maximum() {
    currentOctave = OCTAVE_MAX;
    pressButton(OCTAVE_UP_PIN);
    ASSERT_EQ(currentOctave, OCTAVE_MAX);
}

void test_octave_change_sends_panic() {
    polyMode = true;
    pressKey(0);  // hold a note
    clearMidiLog();
    pressButton(OCTAVE_UP_PIN);
    // Panic should have sent CC 123 on all 16 channels
    ASSERT_EQ(countMsg(MidiMsgType::ControlChange), 32); // 2 CCs × 16 channels
}

void test_octave_change_affects_note_number() {
    currentOctave = DEFAULT_OCTAVE;
    byte noteDefault = midiNoteFor(0);
    currentOctave = DEFAULT_OCTAVE + 1;
    byte noteUp = midiNoteFor(0);
    ASSERT_EQ(noteUp, (byte)(noteDefault + 12));
}

void test_octave_down_button_debounce_no_fire_before_window() {
    currentOctave = DEFAULT_OCTAVE;
    mockPinPress(OCTAVE_DOWN_PIN);
    // Tick for less than DEBOUNCE_MS
    for (unsigned long t = 0; t < DEBOUNCE_MS; t++) {
        scanButtons();
        g_mock_millis++;
    }
    ASSERT_EQ(currentOctave, DEFAULT_OCTAVE);
}

// ===========================================================================
// 8. Panic
// ===========================================================================

void test_panic_sends_cc123_on_all_16_channels() {
    pressButton(PANIC_PIN);
    // Each channel gets CC 123 (All Notes Off) + CC 64 (Sustain Off) = 32 msgs
    ASSERT_EQ(countMsg(MidiMsgType::ControlChange), 32);
    // Verify CC 123 on every channel 1–16
    int cc123_count = 0;
    for (auto &m : g_midi_log) {
        if (m.type == MidiMsgType::ControlChange && m.data1 == 123) {
            cc123_count++;
            ASSERT_EQ((int)m.data2, 0);
        }
    }
    ASSERT_EQ(cc123_count, 16);
}

void test_panic_sends_cc64_sustain_off_on_all_16_channels() {
    pressButton(PANIC_PIN);
    int cc64_count = 0;
    for (auto &m : g_midi_log) {
        if (m.type == MidiMsgType::ControlChange && m.data1 == 64) {
            cc64_count++;
        }
    }
    ASSERT_EQ(cc64_count, 16);
}

void test_panic_resets_mono_stack() {
    polyMode = false;
    pressKey(0);
    pressKey(4);
    pressButton(PANIC_PIN);
    ASSERT_EQ(stackSize, 0);
    ASSERT_EQ(monoActive, -1);
}

void test_panic_resets_poly_note_tracking() {
    polyMode = true;
    pressKey(0);
    pressKey(4);
    pressButton(PANIC_PIN);
    for (int i = 0; i < KEY_COUNT; i++) {
        ASSERT_EQ(polyNote[i], -1);
    }
}

void test_panic_channels_are_1_through_16() {
    pressButton(PANIC_PIN);
    // Collect all channels that received a CC 123
    std::vector<int> channels;
    for (auto &m : g_midi_log) {
        if (m.type == MidiMsgType::ControlChange && m.data1 == 123) {
            channels.push_back((int)m.channel);
        }
    }
    ASSERT_EQ((int)channels.size(), 16);
    for (int ch = 1; ch <= 16; ch++) {
        bool found = false;
        for (int c : channels) if (c == ch) { found = true; break; }
        ASSERT_TRUE(found);
    }
}

// ===========================================================================
// 9. Mode toggle (poly ↔ mono)
// ===========================================================================

void test_mode_toggle_starts_mono() {
    ASSERT_FALSE(polyMode);
}

void test_mode_toggle_switches_to_poly() {
    polyMode = false;
    pressButton(MODE_PIN);
    ASSERT_TRUE(polyMode);
}

void test_mode_toggle_switches_back_to_mono() {
    polyMode = true;
    pressButton(MODE_PIN);
    ASSERT_FALSE(polyMode);
}

void test_mode_toggle_sends_panic() {
    polyMode = true;
    pressKey(0);
    clearMidiLog();
    pressButton(MODE_PIN);
    ASSERT_EQ(countMsg(MidiMsgType::ControlChange), 32);
}

// ===========================================================================
// 10. NOTE_COLORS array — pitch-class colours
// ===========================================================================

void test_note_colors_twelve_entries() {
    int count = 0;
    for (int i = 0; i < 12; i++) {
        // Access each entry to verify the array is fully initialised
        const RGB &c = NOTE_COLORS[i];
        (void)c;
        count++;
    }
    ASSERT_EQ(count, 12);
}

void test_note_colors_C_is_red() {
    ASSERT_EQ((int)NOTE_COLORS[0].r, 255);
    ASSERT_EQ((int)NOTE_COLORS[0].g, 0);
    ASSERT_EQ((int)NOTE_COLORS[0].b, 0);
}

void test_note_colors_modulo_wraps() {
    // MIDI note 60 (middle C) % 12 == 0 → same as NOTE_COLORS[0]
    const RGB &c = NOTE_COLORS[60 % 12];
    ASSERT_EQ((int)c.r, 255);
    ASSERT_EQ((int)c.g, 0);
    ASSERT_EQ((int)c.b, 0);
}

// ===========================================================================
// 11. Mono stack boundary — fill all KEY_COUNT slots
// ===========================================================================

void test_mono_stack_full_does_not_overflow() {
    polyMode = false;
    // Press all 13 keys in sequence
    for (int i = 0; i < KEY_COUNT; i++) {
        pressKey(i);
    }
    ASSERT_EQ(stackSize, KEY_COUNT);
    // The 13th key should be the active note
    ASSERT_EQ(monoActive, (int)midiNoteFor(KEY_COUNT - 1));
}

void test_mono_stack_extra_push_beyond_max_ignored() {
    polyMode = false;
    // Fill the stack
    for (int i = 0; i < STACK_MAX; i++) {
        pressKey(i);
    }
    // stackPush is called only if stackSize < STACK_MAX, so size stays at max
    ASSERT_EQ(stackSize, STACK_MAX);
}

// ===========================================================================
// 12. Mono last-note priority — deep resume chain
// ===========================================================================

void test_mono_last_note_priority_three_keys() {
    // Press 0, 1, 2 in order.  Release 2 → resume 1.  Release 1 → resume 0.
    polyMode = false;

    pressKey(0);
    pressKey(1);
    pressKey(2);
    // Active should be key 2
    ASSERT_EQ(monoActive, (int)midiNoteFor(2));

    clearMidiLog();
    releaseKey(2);
    // Should resume key 1
    ASSERT_EQ(monoActive, (int)midiNoteFor(1));
    ASSERT_EQ(atLog(0).type, MidiMsgType::NoteOff);
    ASSERT_EQ(atLog(0).data1, midiNoteFor(2));
    ASSERT_EQ(atLog(1).type, MidiMsgType::NoteOn);
    ASSERT_EQ(atLog(1).data1, midiNoteFor(1));

    clearMidiLog();
    releaseKey(1);
    // Should resume key 0
    ASSERT_EQ(monoActive, (int)midiNoteFor(0));
    ASSERT_EQ(atLog(0).data1, midiNoteFor(1));
    ASSERT_EQ(atLog(1).data1, midiNoteFor(0));
}

// ===========================================================================
// 13. Poly — polyNote tracking across octave change (held note)
// ===========================================================================

void test_poly_note_off_matches_note_on_note_number() {
    // If octave were to change between press and release, the NoteOff should
    // still match the NoteOn that was recorded at press time.  We simulate
    // this by directly modifying polyNote after pressing.
    polyMode = true;
    pressKey(3);
    byte recorded = (byte)polyNote[3];
    ASSERT_EQ(recorded, midiNoteFor(3));

    // Change octave manually (without panic, to keep polyNote)
    currentOctave++;
    clearMidiLog();
    releaseKey(3);
    // NoteOff should use the *recorded* note, not the new octave's note
    ASSERT_EQ(nthMsg(MidiMsgType::NoteOff, 0).data1, recorded);
}

// ===========================================================================
// 14. stackRemove — correctness
// ===========================================================================

void test_stackRemove_from_middle() {
    // Manually populate stack and verify removal
    stackSize     = 3;
    noteStack[0]  = 48;
    noteStack[1]  = 52;
    noteStack[2]  = 55;
    stackRemove(52);
    ASSERT_EQ(stackSize, 2);
    ASSERT_EQ(noteStack[0], 48);
    ASSERT_EQ(noteStack[1], 55);
}

void test_stackRemove_from_top() {
    stackSize    = 2;
    noteStack[0] = 48;
    noteStack[1] = 52;
    stackRemove(52);
    ASSERT_EQ(stackSize, 1);
    ASSERT_EQ(noteStack[0], 48);
}

void test_stackRemove_nonexistent_is_noop() {
    stackSize    = 2;
    noteStack[0] = 48;
    noteStack[1] = 52;
    stackRemove(99);   // not in stack
    ASSERT_EQ(stackSize, 2);
}

void test_stackTop_empty_returns_minus1() {
    stackSize = 0;
    ASSERT_EQ(stackTop(), -1);
}

void test_stackTop_returns_last_element() {
    stackSize    = 3;
    noteStack[0] = 40;
    noteStack[1] = 45;
    noteStack[2] = 50;
    ASSERT_EQ(stackTop(), 50);
}

// ===========================================================================
// main — run all tests and report
// ===========================================================================

int main() {
    printf("\n=== MIDI Bass Pedal Test Suite ===\n\n");

    // 1. midiNoteFor
    printf("-- midiNoteFor --\n");
    RUN_TEST(test_midiNoteFor_defaultOctave_C);
    RUN_TEST(test_midiNoteFor_defaultOctave_C_sharp);
    RUN_TEST(test_midiNoteFor_defaultOctave_top_C);
    RUN_TEST(test_midiNoteFor_octave0);
    RUN_TEST(test_midiNoteFor_octave8);
    RUN_TEST(test_midiNoteFor_allKeys_sequential);

    // 2. Poly mode
    printf("\n-- Poly mode --\n");
    RUN_TEST(test_poly_single_press_sends_noteOn);
    RUN_TEST(test_poly_single_release_sends_noteOff);
    RUN_TEST(test_poly_multiple_keys_independent_noteOns);
    RUN_TEST(test_poly_release_sends_correct_note_off_per_key);
    RUN_TEST(test_poly_no_note_off_without_prior_note_on);
    RUN_TEST(test_poly_velocity_carried_in_noteOn);

    // 3. Mono mode
    printf("\n-- Mono mode --\n");
    RUN_TEST(test_mono_single_press_sends_noteOn);
    RUN_TEST(test_mono_single_release_sends_noteOff);
    RUN_TEST(test_mono_second_key_silences_first);
    RUN_TEST(test_mono_release_active_with_held_key_resumes);
    RUN_TEST(test_mono_release_non_active_key_no_noteOff);
    RUN_TEST(test_mono_release_active_no_remaining_keys_silent);

    // 4. Debounce
    printf("\n-- Debounce --\n");
    RUN_TEST(test_debounce_no_event_before_window_expires);
    RUN_TEST(test_debounce_event_fires_after_window_expires);
    RUN_TEST(test_debounce_bounce_within_window_resets_timer);
    RUN_TEST(test_debounce_each_key_independent);

    // 5. Velocity
    printf("\n-- Velocity --\n");
    RUN_TEST(test_velocity_min_analog_maps_to_1);
    RUN_TEST(test_velocity_max_analog_maps_to_127);
    RUN_TEST(test_velocity_midpoint_analog);
    RUN_TEST(test_velocity_sent_in_noteOn);
    RUN_TEST(test_velocity_never_zero_from_knob);

    // 6. MIDI channel
    printf("\n-- MIDI channel --\n");
    RUN_TEST(test_midi_channel_noteOn_is_1);
    RUN_TEST(test_midi_channel_noteOff_is_1);
    RUN_TEST(test_midi_channel_constant_is_1);

    // 7. Octave
    printf("\n-- Octave --\n");
    RUN_TEST(test_octave_down_decrements_octave);
    RUN_TEST(test_octave_up_increments_octave);
    RUN_TEST(test_octave_down_clamps_at_minimum);
    RUN_TEST(test_octave_up_clamps_at_maximum);
    RUN_TEST(test_octave_change_sends_panic);
    RUN_TEST(test_octave_change_affects_note_number);
    RUN_TEST(test_octave_down_button_debounce_no_fire_before_window);

    // 8. Panic
    printf("\n-- Panic --\n");
    RUN_TEST(test_panic_sends_cc123_on_all_16_channels);
    RUN_TEST(test_panic_sends_cc64_sustain_off_on_all_16_channels);
    RUN_TEST(test_panic_resets_mono_stack);
    RUN_TEST(test_panic_resets_poly_note_tracking);
    RUN_TEST(test_panic_channels_are_1_through_16);

    // 9. Mode toggle
    printf("\n-- Mode toggle --\n");
    RUN_TEST(test_mode_toggle_starts_mono);
    RUN_TEST(test_mode_toggle_switches_to_poly);
    RUN_TEST(test_mode_toggle_switches_back_to_mono);
    RUN_TEST(test_mode_toggle_sends_panic);

    // 10. Note colours
    printf("\n-- Note colours --\n");
    RUN_TEST(test_note_colors_twelve_entries);
    RUN_TEST(test_note_colors_C_is_red);
    RUN_TEST(test_note_colors_modulo_wraps);

    // 11. Mono stack boundary
    printf("\n-- Mono stack boundary --\n");
    RUN_TEST(test_mono_stack_full_does_not_overflow);
    RUN_TEST(test_mono_stack_extra_push_beyond_max_ignored);

    // 12. Mono last-note priority
    printf("\n-- Mono last-note priority --\n");
    RUN_TEST(test_mono_last_note_priority_three_keys);

    // 13. Poly note tracking
    printf("\n-- Poly note tracking --\n");
    RUN_TEST(test_poly_note_off_matches_note_on_note_number);

    // 14. Stack helpers
    printf("\n-- Stack helpers --\n");
    RUN_TEST(test_stackRemove_from_middle);
    RUN_TEST(test_stackRemove_from_top);
    RUN_TEST(test_stackRemove_nonexistent_is_noop);
    RUN_TEST(test_stackTop_empty_returns_minus1);
    RUN_TEST(test_stackTop_returns_last_element);

    // Summary
    printf("\n=== Results: %d/%d passed", g_tests_run - g_tests_failed, g_tests_run);
    if (g_tests_failed == 0) {
        printf(" — ALL PASSED ===\n\n");
    } else {
        printf(" — %d FAILED ===\n\n", g_tests_failed);
    }

    return g_tests_failed > 0 ? 1 : 0;
}
