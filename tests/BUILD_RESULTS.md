# Build Results

Date: 2026-03-25

## Summary

| Target | Status |
|--------|--------|
| Arduino sketch (`MIDI-Bass-Pedal.ino`) | PASS |
| LUFA firmware (`firmware/`) | PASS |

---

## 1. Arduino Sketch

**File:** `MIDI-Bass-Pedal/MIDI-Bass-Pedal.ino`
**Board:** Arduino Leonardo (`arduino:avr:leonardo`, ATmega32u4)
**Tool:** arduino-cli 1.4.1

### Tooling installed

- `arduino-cli` 1.4.1 — downloaded from `https://downloads.arduino.cc/arduino-cli/` and installed to `~/.local/bin/`
- Arduino AVR core `arduino:avr@1.8.7` — installed via `arduino-cli core install arduino:avr`
  - Bundles `avr-gcc 7.3.0-atmel3.6.1-arduino7`
- Libraries installed via `arduino-cli lib install`:
  - `MIDI Library@5.0.2`
  - `Adafruit NeoPixel@1.15.4`

### Compile command

```
arduino-cli compile --fqbn arduino:avr:leonardo MIDI-Bass-Pedal/MIDI-Bass-Pedal.ino
```

### Result: SUCCESS

```
Sketch uses 10898 bytes (38%) of program storage space. Maximum is 28672 bytes.
Global variables use 771 bytes (30%) of dynamic memory, leaving 1789 bytes for local variables. Maximum is 2560 bytes.
```

No errors or warnings.

---

## 2. LUFA Firmware

**Directory:** `MIDI-Bass-Pedal/firmware/`
**Target MCU:** `at90usb1287` (ATmega family, USB-capable)
**LUFA version:** 210130

### Tooling

- `avr-gcc 7.3.0` — from the Arduino AVR core bundle (`~/.arduino15/packages/arduino/tools/avr-gcc/7.3.0-atmel3.6.1-arduino7/bin/`), added to PATH for make
- LUFA 210130 — downloaded from `https://github.com/abcminiuser/lufa/archive/refs/tags/LUFA-210130.tar.gz` and extracted to `MIDI-Bass-Pedal/LUFA/LUFA/` (the path expected by `LUFA_PATH = ../LUFA/LUFA` in the makefile)

### Fix applied to `firmware/Descriptors.c`

LUFA 210130 renamed the generic `DTYPE_CSInterface` and `DTYPE_CSEndpoint` constants to class-specific variants. The descriptor type constants in `Descriptors.c` were updated:

- `DTYPE_CSInterface` → `AUDIO_DTYPE_CSInterface` (8 occurrences, lines 104–175)
- `DTYPE_CSEndpoint` → `AUDIO_DTYPE_CSEndpoint` (2 occurrences, lines 203, 225)

These constants are now defined in `LUFA/Drivers/USB/Class/Common/AudioClassCommon.h`.

### Build command

```
cd MIDI-Bass-Pedal/firmware && make
```

### Result: SUCCESS

```
AVR Memory Usage
----------------
Device: at90usb1287

Program:    3566 bytes (2.7% Full)
(.text + .data + .bootloader)

Data:         26 bytes (0.3% Full)
(.data + .bss + .noinit)
```

### Output files produced

| File | Description |
|------|-------------|
| `firmware/MIDI.elf` | Linked ELF binary |
| `firmware/MIDI.hex` | Intel HEX file for flashing |
| `firmware/MIDI.bin` | Raw binary |
| `firmware/MIDI.eep` | EEPROM data |
| `firmware/MIDI.lss` | Disassembly listing |
| `firmware/MIDI.sym` | Symbol table |
| `firmware/MIDI.map` | Linker map |

---

## Notes

- `apt-get` was not available without root/sudo; all tooling was installed to user-local paths.
- The Arduino avr-gcc bundle served double duty as the AVR toolchain for the LUFA make build.
- LUFA is not a git submodule in this repo; it was fetched separately and placed at `MIDI-Bass-Pedal/LUFA/LUFA/` to satisfy the makefile's `LUFA_PATH = ../LUFA/LUFA`.
