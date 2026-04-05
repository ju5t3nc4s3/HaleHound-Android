# GPIO mapping reference

This file documents how every ESP32 GPIO number used in the original
HaleHound-CYD firmware maps to a physical signal line on the CH341A
USB-SPI bridge connected via OTG.

The translation happens in `jni/usb_hal.cpp`.

---

## SPI bus (VSPI → CH341A)

| ESP32 GPIO | Signal | CH341A pin | Notes |
|-----------|--------|-----------|-------|
| 18 | SCK | SCK | Shared by NRF24, CC1101, SD |
| 19 | MISO | MISO | Shared |
| 23 | MOSI | MOSI | Shared |

Only one CS is asserted at a time. `spi_manager.cpp` handles exclusion.

---

## NRF24L01 CS and control

| ESP32 GPIO | Signal | CH341A mapping | Direction |
|-----------|--------|---------------|-----------|
| 4 | CSN (chip select) | CS0 (UIO bit 0) | Output |
| 16 | CE (chip enable) | GPIO0 (UIO bit 4) | Output |
| 17 | IRQ | GPIO1 (UIO bit 5) | Input |

---

## CC1101 CS and data lines

| ESP32 GPIO | Signal | CH341A mapping | Direction |
|-----------|--------|---------------|-----------|
| 27 | CS | CS1 (UIO bit 1) | Output |
| 22 | GDO0 (TX data) | GPIO2 (UIO bit 6) | Output |
| 35 | GDO2 (RX data) | GPIO3 (UIO bit 7) | Input |

Note: GPIO 35 is input-only on the ESP32, correct for RX.

---

## GPS / UART

| ESP32 GPIO | Signal | USB bridge | Notes |
|-----------|--------|-----------|-------|
| 3 | UART2 RX (GPS TX) | CP2102 RXD | 9600 baud default |
| 1 | UART2 TX | CP2102 TXD | Rarely used — GPS mostly sends |

---

## UIO byte structure (CH341A)

The CH341A's UIO (Universal I/O) command byte packs CS and GPIO together:

```
Bit 7  Bit 6  Bit 5  Bit 4  Bit 3  Bit 2  Bit 1  Bit 0
GDO2   GDO0   IRQ    CE     (rsv)  CS_SD  CS_CC  CS_NRF
(in)   (out)  (in)   (out)          (out)  (out)  (out)
```

`g_cs_state` tracks bits 0–2 (CS lines, active LOW).
`g_gpio_state` tracks bits 4–7 (GPIO, active HIGH).
Both are combined into a single UIO_STM_OUT byte on each write.
