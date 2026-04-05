#pragma once
// SPI.h — HAL shim for Android NDK build
// SPIClass.transfer() dispatches to the CH341A USB-SPI bridge via jni/usb_hal.cpp
// CS (chip-select) toggling uses hal_spi_cs_low/high which map ESP32 GPIO numbers
// to the correct CH341A device and CS line index.

#include "Arduino.h"

// ── SPI settings container ────────────────────────────────────────────────────
struct SPISettings {
    uint32_t clock;
    uint8_t  bitOrder;
    uint8_t  dataMode;
    SPISettings() : clock(4000000), bitOrder(MSBFIRST), dataMode(SPI_MODE0) {}
    SPISettings(uint32_t c, uint8_t b, uint8_t m) : clock(c), bitOrder(b), dataMode(m) {}
};

// ── USB SPI backend — implemented in jni/usb_hal.cpp ─────────────────────────
extern "C" {
    // Transfer one byte; returns received byte
    uint8_t hal_spi_transfer(uint8_t data);
    // Transfer a buffer in-place (tx->rx, can be same pointer)
    void    hal_spi_transfer_buf(const uint8_t* tx, uint8_t* rx, size_t len);
    // CS pin control — pin numbers match ESP32 GPIO numbering in the firmware
    void    hal_spi_cs_low(uint8_t csPin);
    void    hal_spi_cs_high(uint8_t csPin);
    // Reconfigure CH341A clock speed / mode between devices
    void    hal_spi_set_config(uint32_t clockHz, uint8_t mode);
}

// ── SPIClass ──────────────────────────────────────────────────────────────────
class SPIClass {
    SPISettings _settings;
public:
    // Initialise — no pin args needed (CH341A manages its own pins)
    void begin()                                      {}
    void begin(int sck, int miso, int mosi, int ss)   {}
    void end()                                        {}

    void beginTransaction(SPISettings s) {
        _settings = s;
        hal_spi_set_config(s.clock, s.dataMode);
    }
    void endTransaction() {}

    // Single-byte transfer
    uint8_t transfer(uint8_t data) {
        return hal_spi_transfer(data);
    }

    // 16-bit transfer MSB first
    uint16_t transfer16(uint16_t data) {
        uint8_t hi = hal_spi_transfer((uint8_t)(data >> 8));
        uint8_t lo = hal_spi_transfer((uint8_t)(data & 0xFF));
        return ((uint16_t)hi << 8) | lo;
    }

    // Buffer transfer (in-place)
    void transfer(void* buf, size_t count) {
        hal_spi_transfer_buf((const uint8_t*)buf, (uint8_t*)buf, count);
    }

    // Some libraries call these directly
    void setFrequency(uint32_t hz)  { hal_spi_set_config(hz, _settings.dataMode); }
    void setDataMode(uint8_t mode)  { hal_spi_set_config(_settings.clock, mode); }
    void setBitOrder(uint8_t order) { /* CH341A is always MSB-first */ }
    void setClockDivider(uint8_t)   {}
};

// Singleton — defined in jni/SPI.cpp
extern SPIClass SPI;
extern SPIClass SPI1;  // alias for VSPI — both map to the same CH341A bus
