/*
 * avr/interrupt.h stub — replaces AVR-libc interrupt header.
 *
 * GlobalInterruptEnable() is the only AVR interrupt macro the firmware calls
 * directly; we stub it to a no-op and track whether it was called.
 */
#pragma once

extern int g_global_interrupts_enabled;

static inline void GlobalInterruptEnable(void)
{
    g_global_interrupts_enabled = 1;
}

static inline void GlobalInterruptDisable(void)
{
    g_global_interrupts_enabled = 0;
}

/* ISR macro — firmware may declare ISR bodies; on Linux they compile to
 * ordinary functions prefixed with "isr_". */
#define ISR(vec)  void isr_##vec(void)
