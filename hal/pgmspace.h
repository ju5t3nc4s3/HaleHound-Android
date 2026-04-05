#pragma once
// HAL shim: pgmspace.h
// On AVR/ESP32, PROGMEM stores constants in flash.
// On Android everything is in RAM — these are all no-ops.

#ifndef PROGMEM
#define PROGMEM
#endif

#define PGM_P          const char*
#define PSTR(s)        (s)

#define pgm_read_byte(addr)   (*(const uint8_t*)(addr))
#define pgm_read_word(addr)   (*(const uint16_t*)(addr))
#define pgm_read_dword(addr)  (*(const uint32_t*)(addr))
#define pgm_read_float(addr)  (*(const float*)(addr))
#define pgm_read_ptr(addr)    (*(const void* const*)(addr))

#define strlen_P(s)    strlen(s)
#define strcpy_P(d,s)  strcpy(d,s)
#define strcat_P(d,s)  strcat(d,s)
#define strcmp_P(a,b)  strcmp(a,b)
#define memcpy_P(d,s,n) memcpy(d,s,n)

typedef const char* __FlashStringHelper;
#define F(s) (s)
