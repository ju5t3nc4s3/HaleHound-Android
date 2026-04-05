#pragma once
// Arduino.h — HAL shim for Android NDK build
// Replaces the ESP32 Arduino SDK. All GPIO calls route to jni/usb_hal.cpp
// which forwards them to the CH341A USB-SPI/GPIO bridge over OTG.

#include <cstdint>
#include <cstdarg>
#include <cstring>
#include <cstdio>
#include <cctype>
#include <chrono>
#include <thread>
#include <android/log.h>

#define HIGH         1
#define LOW          0
#define INPUT        0
#define OUTPUT       1
#define INPUT_PULLUP 2
#define LSBFIRST     0
#define MSBFIRST     1
#define SPI_MODE0    0
#define SPI_MODE1    1
#define SPI_MODE2    2
#define SPI_MODE3    3

typedef unsigned char  byte;
typedef bool           boolean;

// ── Timing ───────────────────────────────────────────────────────────────────
inline void delay(uint32_t ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
inline void delayMicroseconds(uint32_t us) {
    std::this_thread::sleep_for(std::chrono::microseconds(us));
}
inline uint32_t millis() {
    static auto t0 = std::chrono::steady_clock::now();
    return (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0).count();
}
inline uint32_t micros() {
    static auto t0 = std::chrono::steady_clock::now();
    return (uint32_t)std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - t0).count();
}

// ── GPIO — backed by CH341A USB GPIO in jni/usb_hal.cpp ──────────────────────
extern "C" {
    void    hal_digitalWrite(uint8_t pin, uint8_t val);
    uint8_t hal_digitalRead(uint8_t pin);
    void    hal_pinMode(uint8_t pin, uint8_t mode);
}
inline void    digitalWrite(uint8_t p, uint8_t v) { hal_digitalWrite(p, v); }
inline uint8_t digitalRead(uint8_t p)             { return hal_digitalRead(p); }
inline void    pinMode(uint8_t p, uint8_t m)      { hal_pinMode(p, m); }

// ── Math helpers ─────────────────────────────────────────────────────────────
template<typename T> inline T min(T a, T b)        { return a < b ? a : b; }
template<typename T> inline T max(T a, T b)        { return a > b ? a : b; }
template<typename T> inline T constrain(T x, T l, T h) { return x<l?l:(x>h?h:x); }
inline long map(long x, long a, long b, long c, long d) {
    return (x - a) * (d - c) / (b - a) + c;
}
inline long abs(long x) { return x < 0 ? -x : x; }

// ── Minimal String class ─────────────────────────────────────────────────────
class String {
    char buf[512];
public:
    String()                  { buf[0] = '\0'; }
    String(const char* s)     { strncpy(buf, s ? s : "", 511); buf[511] = '\0'; }
    String(int v)             { snprintf(buf, 512, "%d", v); }
    String(unsigned int v)    { snprintf(buf, 512, "%u", v); }
    String(long v)            { snprintf(buf, 512, "%ld", v); }
    String(float v, int d=2)  { snprintf(buf, 512, "%.*f", d, (double)v); }
    String(char c)            { buf[0]=c; buf[1]='\0'; }

    const char* c_str()  const { return buf; }
    int         length() const { return (int)strlen(buf); }
    bool        isEmpty()const { return buf[0]=='\0'; }

    String operator+(const String& o) const {
        String r(*this);
        strncat(r.buf, o.buf, 511 - strlen(r.buf));
        return r;
    }
    String& operator+=(const String& o) { *this = *this + o; return *this; }
    String& operator+=(char c)  { int n=length(); if(n<511){buf[n]=c;buf[n+1]='\0';} return *this; }
    bool operator==(const String& o) const { return strcmp(buf, o.buf)==0; }
    bool operator==(const char*   s) const { return strcmp(buf, s ? s : "")==0; }
    bool operator!=(const String& o) const { return !(*this==o); }
    char operator[](int i)           const { return buf[i]; }

    int     indexOf(char c, int from=0) const {
        const char* p = strchr(buf+from, c);
        return p ? (int)(p-buf) : -1;
    }
    String  substring(int s, int e=-1) const {
        String r;
        int len = (e<0) ? (int)strlen(buf)-s : e-s;
        if(len>0) { strncpy(r.buf, buf+s, len); r.buf[len]='\0'; }
        return r;
    }
    String  toUpperCase() const {
        String r(*this);
        for(char* p=r.buf; *p; ++p) *p=(char)toupper((unsigned char)*p);
        return r;
    }
    String  toLowerCase() const {
        String r(*this);
        for(char* p=r.buf; *p; ++p) *p=(char)tolower((unsigned char)*p);
        return r;
    }
    void    trim() {
        int s=0, e=(int)strlen(buf)-1;
        while(buf[s]==' ') s++;
        while(e>s && buf[e]==' ') e--;
        memmove(buf, buf+s, e-s+1);
        buf[e-s+1]='\0';
    }
    int     toInt()    const { return atoi(buf); }
    float   toFloat()  const { return (float)atof(buf); }
    String  operator+(const char* s) const { return *this + String(s); }
    String  operator+(char c)        const { return *this + String(c); }
};
inline String operator+(const char* a, const String& b) { return String(a) + b; }

// ── No-ops for ESP32 interrupt/yield calls ────────────────────────────────────
inline void yield()         {}
inline void interrupts()    {}
inline void noInterrupts()  {}

// ── pgmspace stubs (AVR compat, not needed on Cortex/NDK but sometimes included) ──
#define PROGMEM
#define pgm_read_byte(addr)   (*(const uint8_t*)(addr))
#define pgm_read_word(addr)   (*(const uint16_t*)(addr))
#define pgm_read_dword(addr)  (*(const uint32_t*)(addr))
#define F(s) (s)
