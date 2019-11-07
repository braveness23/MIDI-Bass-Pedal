
#include <MIDI.h>

MIDI_CREATE_DEFAULT_INSTANCE();

#define DEFAULT_OCTAVE 5
#define NUMBER_OF_KEYS 13
#define NOTE_VELOCITY 127

#define BUTTON_OCTAVE_DOWN D19
#define BUTTON_OCTAVE_UP D20
#define BUTTON_FIRST_KEY D21

boolean midiMode = false; // if midiMode = false, the Arduino will send on and off messages via the serial monitor.
                          // if midiMode = true, the Arduino will act as a native MIDI device.
int CurrentOctave = DEFAULT_OCTAVE;

void handleNoteOn(byte channel, byte pitch, byte velocity)
{
  // Do whatever you want when a note is pressed.
}

void handleNoteOff(byte channel, byte pitch, byte velocity)
{
  // Do something when the note is released.
  // Note that NoteOn messages with 0 velocity are interpreted as NoteOffs.
}

void setup()
{
  if (midiMode)
  {
    Serial.begin(31250); // this is the standard communication baudrate for MIDI devices, do not change this!
  }
  else
  {
    Serial.begin(9600);
  }

  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.begin(MIDI_CHANNEL_OMNI);
}

void loop()
{

  MIDI.read();
}

void handleKeyDown()
{
  MIDI.sendNoteOn(42, 127, 1);
}

void handKeyUp()
{
  MIDI.sendNoteOff(42, 0, 1);
}
