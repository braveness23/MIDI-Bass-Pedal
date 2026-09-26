/*
 * LUFA/Platform/Platform.h stub
 *
 * The firmware includes this for GlobalInterruptEnable().  On Linux we
 * forward to our avr/interrupt.h stub.
 */
#pragma once

/* Use a path relative to the stubs root that works with -Istubs */
#include "../../avr/interrupt.h"
