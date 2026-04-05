This not compleate ,, and I wont copleat it ,, 
A port of the Halehound firmware to a full android APK,  using OTG out to usb hub for all radio moduals.


# HaleHound Android — HAL Shim Port

Ports HaleHound-CYD (https://github.com/JesseCHale/HaleHound-CYD) ESP32 firmware to Android
by cross-compiling the original C++ attack modules via the Android NDK, replacing the ESP32
hardware layer with a thin shim that redirects GPIO/SPI/UART calls to real USB OTG hardware.

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  HOW IT WORKS

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  Original firmware .cpp files  (unchanged — dropped into firmware-stubs/)
           |  #includes Arduino.h, SPI.h, HardwareSerial.h
           v
  HAL shim headers              (hal/)
           |  SPI.transfer()  -->  USB bulk transfer to CH341
           |  digitalWrite()  -->  CH341 GPIO pin
           |  Serial.read()   -->  CP2102 USB UART byte
           v
  NDK / JNI backend             (jni/)
           |  libusb wraps Android UsbDeviceConnection file descriptors
           v
  USB OTG powered hub
    +-- CH341A board #1  -->  NRF24L01+PA+LNA  (SPI)
    +-- CH341A board #2  -->  CC1101 HW-863    (SPI)
    +-- CP2102 board     -->  GPS GT-U7        (UART 9600 baud)
    +-- Alfa AWUS036ACH  -->  Monitor-mode WiFi (root only, optional)

  WiFi and Bluetooth attacks use Android native APIs (WifiManager, BluetoothLeScanner).
  The ESP32 RF stack is not portable, so wifi_attacks.cpp and bluetooth_attacks.cpp are
  replaced by Kotlin equivalents in app/src/main/kotlin/com/halehound/.

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  HARDWARE REQUIRED

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  Item                         Purpose                      Notes
  ───────────────────────────  ───────────────────────────  ─────────────────────────────────
  Powered USB-C OTG hub        Connect all modules to phone Must supply its own power —
                                                            NRF24+PA+LNA exceeds bus limit
  CH341A breakout x2           USB -> SPI bridge            VID 0x1A86  PID 0x5512 or 0x5523
  CP2102 breakout              USB -> UART for GPS          VID 0x10C4  PID 0xEA60
  NRF24L01+PA+LNA              2.4 GHz radio                Add 10uF cap across VCC/GND
  CC1101 HW-863 (red board)    SubGHz 300-928 MHz           setPA(12) = max TX power
  GT-U7 or NEO-6M              GPS module                   UART 9600 baud
  Alfa AWUS036ACH (optional)   Monitor-mode WiFi            Needs rooted device

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  WIRING

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  NRF24L01 --> CH341A board #1
    VCC  -> 3.3V
    GND  -> GND
    SCK  -> SCK
    MOSI -> MOSI
    MISO -> MISO
    CSN  -> CS0    (maps to ESP32 GPIO 4)
    CE   -> GPIO0  (maps to ESP32 GPIO 16)
    IRQ  -> GPIO1  (maps to ESP32 GPIO 17, input)

  CC1101 --> CH341A board #2
    VCC  -> 3.3V
    GND  -> GND
    SCK  -> SCK
    MOSI -> MOSI
    MISO -> MISO
    CS   -> CS0    (maps to ESP32 GPIO 27)
    GDO0 -> GPIO2  (maps to ESP32 GPIO 22, data TX to radio)
    GDO2 -> GPIO3  (maps to ESP32 GPIO 35, data RX, input only)

  GPS GT-U7 --> CP2102
    VCC  -> 3.3V
    GND  -> GND
    TX   -> RXD
    RX   -> TXD   (optional — GPS is mostly one-way)

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  GPIO PIN MAPPING  (ESP32 GPIO -> CH341 USB line)

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  ESP32 GPIO   Firmware role       CH341 line
  ──────────   ─────────────────   ───────────────────
  GPIO 4       NRF24 CSN           CS0  (CH341 device #1)
  GPIO 16      NRF24 CE            GPIO0
  GPIO 17      NRF24 IRQ           GPIO1  (input)
  GPIO 27      CC1101 CS           CS0  (CH341 device #2)
  GPIO 22      CC1101 GDO0 (TX)    GPIO2
  GPIO 35      CC1101 GDO2 (RX)    GPIO3  (input)
  GPIO 18      VSPI SCK            SCK
  GPIO 19      VSPI MISO           MISO
  GPIO 23      VSPI MOSI           MOSI

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  PROJECT STRUCTURE

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  HaleHound-Android/
  +-- README.md                         This file
  +-- CMakeLists.txt                    NDK build — compiles firmware + shims -> libhalehound.so
  |
  +-- hal/                              HAL shim headers (fake Arduino / ESP-IDF API surface)
  |   +-- Arduino.h                     delay(), millis(), digitalWrite(), String, etc.
  |   +-- SPI.h                         SPIClass — transfer() dispatches to USB bulk transfer
  |   +-- HardwareSerial.h              Serial / Serial2 — logcat output + USB UART reads
  |
  +-- jni/                              NDK C++ implementation files
  |   +-- usb_hal.cpp                   libusb backend: CH341 SPI ops, CP2102 UART, GPIO
  |   +-- SPI.cpp                       SPIClass singleton + CH341A init sequence
  |   +-- HardwareSerial.cpp            Serial / Serial2 singleton instances
  |   +-- attack_entry.cpp              JNI entry points called from Kotlin HalBridge
  |
  +-- firmware-stubs/                   Drop original HaleHound-CYD .cpp files here
  |   +-- PLACE_FIRMWARE_HERE.txt       Exact list of files to copy and which to skip
  |
  +-- app/src/main/
      +-- kotlin/com/halehound/
      |   +-- HalBridge.kt              Opens USB devices, loads JNI lib, launches attacks
      |   +-- UsbDeviceManager.kt       UsbManager wrapper, permission flow, hot-plug
      |   +-- WifiAttacks.kt            WiFi deauth/beacon/scan via Android WifiManager
      |   +-- BleAttacks.kt             BLE scan/advertise via Android BLE API
      |   +-- MainActivity.kt           Jetpack Compose UI shell
      +-- res/xml/
          +-- device_filter.xml         USB device whitelist (CH341A, CP2102 VID/PID)

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  BUILD INSTRUCTIONS

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  Prerequisites:
    - Android Studio Hedgehog or newer
    - NDK r25c or newer (set ndk.dir in local.properties)
    - libusb-android prebuilt .so files for arm64-v8a and armeabi-v7a
      Build from: https://github.com/libusb/libusb
      Place at:   libs/arm64-v8a/libusb1.0.so
                  libs/armeabi-v7a/libusb1.0.so

  Steps:
    1. Clone HaleHound-CYD firmware:
         git clone https://github.com/JesseCHale/HaleHound-CYD

    2. Copy firmware source into firmware-stubs/ — see PLACE_FIRMWARE_HERE.txt
       Do NOT copy wifi_attacks.cpp or bluetooth_attacks.cpp (replaced by Kotlin)

    3. Open project in Android Studio, wait for Gradle sync

    4. Build:
         ./gradlew assembleDebug

    5. Install:
         adb install app/build/outputs/apk/debug/app-debug.apk

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  KNOWN LIMITATIONS

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  - WiFi raw frame injection (deauth, beacon spam) requires a rooted device or a USB
    WiFi adapter (e.g. Alfa AWUS036ACH) with monitor mode enabled. Android's WifiManager
    does not expose raw 802.11 TX to non-root apps.

  - BLE jamming/injection requires CAP_NET_RAW capability — root only. Passive BLE
    scanning and advertising work fine without root.

  - CC1101 replay timing: USB introduces ~1 ms jitter from the host stack. This is
    acceptable for fixed-code systems (garage doors, gate openers). Rolling-code targets
    may require a timing-accurate microcontroller intermediary between CH341 and CC1101.

  - The TFT display, touchscreen, RGB LED, speaker, and SD card from the CYD hardware
    are not ported — Android provides native equivalents for all of these.

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  LICENSE

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  HAL shim and JNI glue code in this repository: MIT
  HaleHound-CYD firmware source: see original repo license at
  https://github.com/JesseCHale/HaleHound-CYD/blob/main/LICENSE

  This project is for authorized penetration testing and educational use only.
  Ensure you have explicit written permission before testing on any network or
  device you do not own.
