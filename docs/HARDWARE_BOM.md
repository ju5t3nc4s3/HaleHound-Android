# Hardware bill of materials & wiring guide

## Components

| # | Item | Purpose | Notes |
|---|------|---------|-------|
| 1 | Powered USB hub (USB-A, ≥2A supply) | Carries all modules | Must be powered — NRF24+PA+LNA draws too much for bus power |
| 2 | CH341A USB-SPI board ×1 (or ×2) | SPI bridge → NRF24 + CC1101 | ~$3 each, search "CH341A programmer" |
| 3 | CP2102 USB-UART board | GPS UART bridge | Any CP2102, CH340, or FT232R board works |
| 4 | NRF24L01+PA+LNA module | 2.4 GHz attacks | The +PA+LNA version for range |
| 5 | CC1101 HW-863 red board | SubGHz 300–928 MHz | The red HW-863 board is the correct variant |
| 6 | GT-U7 or NEO-6M GPS | Wardriving | |
| 7 | 10µF electrolytic capacitor | NRF24 power stability | Solder across VCC/GND at the NRF24 module |
| 8 | USB-C OTG adapter (or micro-USB) | Phone → hub | Match your phone's port |
| 9 | Alfa AWUS036ACH (optional) | Monitor-mode WiFi | Root required for raw injection |
| 10 | USB flash drive (optional) | PCAP/log storage | |

Total cost estimate (excluding phone and Alfa): ~$20–30 USD

---

## CH341A SPI wiring

The CH341A board exposes a standard 2×5 or 2×4 ISP header.
Pins below use the common "green CH341A programmer" pinout.

### CH341A → NRF24L01+PA+LNA

| CH341A pin | NRF24 pin | Notes |
|-----------|-----------|-------|
| VCC (3.3V) | VCC | Use 3.3V — NRF24 is NOT 5V tolerant |
| GND | GND | |
| SCK | SCK | |
| MOSI | MOSI | |
| MISO | MISO | |
| CS0 | CSN | Chip select (active LOW) |
| GPIO0 | CE | Chip enable (controlled by hal_digitalWrite) |
| GPIO1 | IRQ | Interrupt (optional, polled in firmware) |

Add 10µF capacitor between VCC and GND directly at the NRF24 module.

### CH341A → CC1101 HW-863

If using two CH341A boards (recommended, cleaner CS management):

| CH341A pin | CC1101 pin | Notes |
|-----------|-----------|-------|
| VCC (3.3V) | VCC | |
| GND | GND | |
| SCK | SCK | |
| MOSI | MOSI | |
| MISO | MISO | |
| CS0 | CS | Chip select |
| GPIO0 | GDO0 | TX data line (output from host) |
| GPIO1 | GDO2 | RX data line (input to host) |

If sharing one CH341A board, use CS0 for NRF24 and CS1 for CC1101, and tie both
MOSI/MISO/SCK lines together. Only one CS is asserted at a time (handled by the
spi_manager shim).

### CP2102 → GPS (GT-U7 or NEO-6M)

| CP2102 pin | GPS pin | Notes |
|-----------|---------|-------|
| VCC (3.3V or 5V) | VCC | Check your GPS module's voltage |
| GND | GND | |
| RXD | TX | CP2102 receives GPS data |
| TXD | RX | (usually not needed — GPS sends, rarely receives) |

Default baud rate: 9600. The firmware calls `Serial2.begin(9600)`.

---

## USB OTG hub wiring overview

```
Android phone
    │
    │ USB-C OTG cable
    │
    ▼
Powered USB Hub (5V/2A+ adapter required)
    ├── Port 1 → CH341A board #1 → NRF24L01+PA+LNA
    ├── Port 2 → CH341A board #2 → CC1101 HW-863
    ├── Port 3 → CP2102 board   → GT-U7 GPS
    ├── Port 4 → Alfa AWUS036ACH (optional, root only)
    └── Port 5 → USB flash drive (optional)
```

---

## GPIO mapping (ESP32 → CH341 translation)

The `usb_hal.cpp` translates ESP32 GPIO numbers (as written in the original
firmware) to physical CH341 signal lines. This table is for reference.

| ESP32 GPIO | Original purpose | CH341 line | Direction |
|-----------|----------------|-----------|-----------|
| 18 | VSPI SCK | SCK | Output |
| 19 | VSPI MISO | MISO | Input |
| 23 | VSPI MOSI | MOSI | Output |
| 4 | NRF24 CSN | CS0 | Output |
| 16 | NRF24 CE | GPIO0 | Output |
| 17 | NRF24 IRQ | GPIO1 | Input |
| 27 | CC1101 CS | CS1 | Output |
| 22 | CC1101 GDO0 (TX) | GPIO2 | Output |
| 35 | CC1101 GDO2 (RX) | GPIO3 | Input |
| 3 | GPS RX (UART2) | CP2102 RXD | Input |

To change the mapping, edit the `switch` statements in `hal_spi_cs_assert()`,
`hal_digitalWrite()`, and `hal_digitalRead()` in `jni/usb_hal.cpp`.
