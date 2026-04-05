#pragma once
// HAL shim: pins_arduino.h
// ESP32 GPIO numbers used by the firmware.
// Physical mapping to CH341 lines is in jni/usb_hal.cpp.

// VSPI bus (shared: SD, CC1101, NRF24)
#define PIN_SPI_SCK   18
#define PIN_SPI_MISO  19
#define PIN_SPI_MOSI  23
#define PIN_SPI_SS     5   // SD card CS

// NRF24L01
#define PIN_NRF24_CSN  4
#define PIN_NRF24_CE   16
#define PIN_NRF24_IRQ  17

// CC1101
#define PIN_CC1101_CS   27
#define PIN_CC1101_GDO0 22   // TX data line
#define PIN_CC1101_GDO2 35   // RX data line (input-only on ESP32)

// GPS
#define PIN_GPS_RX  3
#define PIN_GPS_TX  1

// Misc
#define PIN_BOOT    0    // BOOT button (GPIO 0)
#define PIN_LDR    34    // Light-dependent resistor / battery ADC

// Not available on Android — defined to avoid compile errors
#define LED_BUILTIN 2
