# =============================================================================
# tests/firmware/Makefile
#
# Compiles and runs the LUFA USB MIDI firmware unit tests on Linux (no AVR
# hardware required).
#
# Usage:
#   make          — build and run all tests
#   make build    — build only
#   make run      — run already-built tests
#   make clean    — remove build artefacts
#
# Each test binary is self-contained; stub globals are linked in from
# stub_globals.o.  All binaries end up in build/.
# =============================================================================

CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -Wno-unused-function \
           -Wno-unused-parameter -Wno-missing-field-initializers \
           -g \
           -I. \
           -Istubs \
           -I../../MIDI-Bass-Pedal/firmware \
           -DARCH_AVR8=1 \
           -DARCH=ARCH_AVR8 \
           -DUSE_LUFA_CONFIG_HEADER \
           -IConfig

# Directory for build artefacts
BUILD := build

# Stub globals object (compiled once, linked into every test binary)
STUB_GLOBALS_SRC := stub_globals.c
STUB_GLOBALS_OBJ := $(BUILD)/stub_globals.o

# Each test file becomes its own executable
TEST_SRCS := test_midi_packet.c \
             test_midi_macros.c \
             test_descriptors.c \
             test_key_scanning.c \
             test_usb_events.c

TEST_BINS := $(patsubst %.c,$(BUILD)/%,$(TEST_SRCS))

# =============================================================================
# Default target: build + run
# =============================================================================
.PHONY: all
all: build run

# =============================================================================
# Build target
# =============================================================================
.PHONY: build
build: $(TEST_BINS)

# Compile stub globals (mkdir ensures the build dir exists first)
$(STUB_GLOBALS_OBJ): $(STUB_GLOBALS_SRC)
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

# Pattern rule: compile each test source into its own binary.
# Each TU includes the firmware source directly; stub_globals.o
# provides the simulated hardware register definitions.
$(BUILD)/test_%: test_%.c $(STUB_GLOBALS_OBJ)
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $< $(STUB_GLOBALS_OBJ) -o $@

# =============================================================================
# Run target
# =============================================================================
.PHONY: run
run: $(TEST_BINS)
	@echo ""
	@echo "============================================================"
	@echo " Running firmware unit tests"
	@echo "============================================================"
	@PASS=0; FAIL=0; \
	for bin in $(TEST_BINS); do \
	    echo ""; \
	    echo "--- $$bin ---"; \
	    if $$bin; then \
	        PASS=$$((PASS + 1)); \
	    else \
	        FAIL=$$((FAIL + 1)); \
	    fi; \
	done; \
	echo ""; \
	echo "============================================================"; \
	echo " Test suites: $$((PASS + FAIL))  passed: $$PASS  failed: $$FAIL"; \
	echo "============================================================"; \
	test $$FAIL -eq 0

# =============================================================================
# Clean
# =============================================================================
.PHONY: clean
clean:
	rm -rf $(BUILD)
