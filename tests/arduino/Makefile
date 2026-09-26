# =============================================================================
# Makefile — MIDI Bass Pedal native Linux test suite
#
# Usage:
#   make          build and run
#   make build    build only
#   make run      run only (must have built first)
#   make clean    remove build artefacts
#
# Requirements:
#   g++ with C++14 support (standard on any modern Linux distro)
# =============================================================================

CXX      := g++
# -std=c++14: needed for initialiser-list and some STL features
# -x c++:     treat the .ino as C++ source
# -I.:        so #include "mock_arduino.h" etc. resolve
# -Wall -Wextra: catch common issues without being too noisy
# -Wno-unused-parameter: Arduino callbacks have unused params by design
CXXFLAGS := -std=c++14 -Wall -Wextra -Wno-unused-parameter -I.

# The test binary
TARGET   := run_tests

# Source file
SRCS     := test_midi_bass_pedal.cpp

# The .ino is pulled in via sketch_under_test.h → no need to list it separately
# (it is #included directly, not compiled as a separate translation unit)

.PHONY: all build run clean

all: build run

build: $(TARGET)

$(TARGET): $(SRCS) sketch_under_test.h mock_arduino.h
	$(CXX) $(CXXFLAGS) -o $@ $(SRCS)

run: $(TARGET)
	@echo ""
	@./$(TARGET)

clean:
	rm -f $(TARGET)
