/*
 * avr/pgmspace.h stub — Flash (PROGMEM) attribute and access helpers.
 *
 * On Linux all data lives in RAM, so PROGMEM and pgm_read_* collapse to
 * ordinary memory accesses.
 */
#pragma once
#include <stdint.h>
#include <string.h>

/* Flash attribute — ignored on Linux */
#define PROGMEM

/* pgm_read_* helpers — on Linux these are plain derefs */
#define pgm_read_byte(addr)   (*(const uint8_t *)(addr))
#define pgm_read_word(addr)   (*(const uint16_t *)(addr))
#define pgm_read_dword(addr)  (*(const uint32_t *)(addr))
#define pgm_read_ptr(addr)    (*(const void * const *)(addr))

/* memcpy_P / strcpy_P — identical to their RAM counterparts on Linux */
#define memcpy_P(dst, src, n)  memcpy((dst), (src), (n))
#define strcpy_P(dst, src)     strcpy((dst), (src))
#define strlen_P(s)            strlen(s)

/* PSTR — on Linux just returns the string literal as-is */
#define PSTR(s)  (s)
