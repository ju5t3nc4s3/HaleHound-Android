#pragma once
// HardwareSerial.h — HAL shim for Android NDK build
// Serial  -> Android Logcat (debug output from firmware)
// Serial2 -> CP2102 USB-UART bridge (GPS NMEA at 9600 baud)

#include "Arduino.h"
#include <android/log.h>
#include <cstdio>
#include <cstdarg>

#define LOG_TAG "HaleHound"

// USB UART backend — implemented in jni/usb_hal.cpp
extern "C" {
    void hal_uart_begin(int port, uint32_t baud);
    void hal_uart_end(int port);
    int  hal_uart_available(int port);
    int  hal_uart_read(int port);
    void hal_uart_write(int port, uint8_t b);
    void hal_uart_flush(int port);
}

class HardwareSerial {
    int _port;      // 0 = Logcat only, 1 = USB UART port index
    bool _useUsb;
public:
    explicit HardwareSerial(int port) : _port(port), _useUsb(port > 0) {}

    void begin(uint32_t baud) {
        if (_useUsb) hal_uart_begin(_port, baud);
    }
    void begin(uint32_t baud, uint32_t /*config*/) { begin(baud); }
    void end() {
        if (_useUsb) hal_uart_end(_port);
    }

    // ── Output — always mirrors to Logcat; USB port gets raw bytes too ────────
    void print(const char* s) {
        __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, "%s", s);
        if (_useUsb) for(const char* p=s; *p; ++p) hal_uart_write(_port,(uint8_t)*p);
    }
    void println(const char* s) {
        __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, "%s", s);
        if (_useUsb) {
            for(const char* p=s; *p; ++p) hal_uart_write(_port,(uint8_t)*p);
            hal_uart_write(_port,'\r'); hal_uart_write(_port,'\n');
        }
    }
    void print(int v)           { char b[32]; snprintf(b,32,"%d",v);  print(b); }
    void println(int v)         { char b[32]; snprintf(b,32,"%d",v);  println(b); }
    void print(unsigned int v)  { char b[32]; snprintf(b,32,"%u",v);  print(b); }
    void println(unsigned int v){ char b[32]; snprintf(b,32,"%u",v);  println(b); }
    void print(long v)          { char b[32]; snprintf(b,32,"%ld",v); print(b); }
    void println(long v)        { char b[32]; snprintf(b,32,"%ld",v); println(b); }
    void print(float v, int d=2){ char b[32]; snprintf(b,32,"%.*f",d,(double)v); print(b); }
    void println(float v,int d=2){char b[32]; snprintf(b,32,"%.*f",d,(double)v); println(b);}
    void print(const String& s)  { print(s.c_str()); }
    void println(const String& s){ println(s.c_str()); }
    void println()               { println(""); }

    void printf(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        char buf[512];
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        print(buf);
    }

    void write(uint8_t b) {
        if (_useUsb) hal_uart_write(_port, b);
    }
    void write(const uint8_t* buf, size_t n) {
        if (_useUsb) for(size_t i=0;i<n;i++) hal_uart_write(_port, buf[i]);
    }

    // ── Input — reads from USB UART (GPS port) ────────────────────────────────
    int  available() { return _useUsb ? hal_uart_available(_port) : 0; }
    int  read()      { return _useUsb ? hal_uart_read(_port) : -1; }
    int  peek()      { return -1; }
    void flush()     { if (_useUsb) hal_uart_flush(_port); }

    operator bool() const { return true; }
};

// Singletons — defined in jni/HardwareSerial.cpp
extern HardwareSerial Serial;   // port 0: Logcat output only
extern HardwareSerial Serial2;  // port 1: CP2102 USB-UART (GPS)
