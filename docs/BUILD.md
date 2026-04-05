# Build instructions

## Prerequisites

| Tool | Version | Install |
|------|---------|---------|
| Android Studio | Hedgehog (2023.1+) | https://developer.android.com/studio |
| Android NDK | r25c or later | SDK Manager → SDK Tools → NDK |
| CMake | 3.22+ | SDK Manager → SDK Tools → CMake |
| libusb-android | latest | See step 2 below |

---

## Step 1 — Clone this repo

```bash
git clone https://github.com/YOUR_FORK/HaleHound-Android
cd HaleHound-Android
```

## Step 2 — Download libusb-android prebuilt

```bash
# Download the prebuilt AAR/zip from:
# https://github.com/esrlabs/libusb-android/releases
# Extract so the folder structure looks like:
#
# libusb-android/
#   include/libusb.h
#   libs/
#     arm64-v8a/libusb1.0.so
#     armeabi-v7a/libusb1.0.so
#     x86_64/libusb1.0.so

# Place the libusb-android/ folder at the same level as HaleHound-Android/
ls ..
# Should show: HaleHound-Android/   libusb-android/
```

## Step 3 — Copy original firmware sources

```bash
# Clone the original firmware
git clone https://github.com/JesseCHale/HaleHound-CYD firmware-src

# Copy all .cpp and .h files into firmware/
cp firmware-src/*.cpp firmware-src/*.h firmware/

# Remove the two files replaced by Android stubs
rm firmware/wifi_attacks.cpp
rm firmware/bluetooth_attacks.cpp

# Also remove CYD-specific files with no Android equivalent
rm -f firmware/touch_buttons.cpp \
      firmware/CYD28_TouchscreenR.cpp \
      firmware/firmware_update.cpp \
      firmware/HaleHound-CYD.ino
```

## Step 4 — Open in Android Studio

1. Open Android Studio
2. File → Open → select the `HaleHound-Android/` folder
3. Let Gradle sync complete
4. SDK Manager → confirm NDK r25c and CMake 3.22+ are installed

## Step 5 — Build

```bash
# From project root (or use Android Studio Build menu)
./gradlew assembleDebug
```

The NDK build compiles `libhalehound.so` for each ABI (arm64-v8a, armeabi-v7a).
Expect compiler warnings about unused parameters — these come from the firmware
source and are suppressed in CMakeLists.txt.

## Step 6 — Install and run

```bash
# Enable USB debugging on your Android device
# Connect via USB and run:
adb install -r app/build/outputs/apk/debug/app-debug.apk
adb logcat -s HaleHound HaleHound-USB HaleHound-BLE HaleHound-WiFi
```

---

## Troubleshooting

**`libusb.h not found`**
— Check that `libusb-android/include/libusb.h` exists relative to the project root.
— Verify the path in `CMakeLists.txt` `LIBUSB_DIR` matches your folder name.

**`undefined reference to hal_spi_transfer`**
— The shim headers are not on the include path before Arduino SDK headers.
— Confirm `hal/` is the first entry in `target_include_directories()`.

**App installs but shows "No USB hardware"**
— Check `adb logcat` for `HaleHound-USB` tag messages.
— Make sure you granted USB permission on the popup dialog.
— Try unplugging and replugging the OTG hub.

**NRF24/CC1101 init fails**
— Run the Radio Self-Test from the app menu.
— Check wiring against the GPIO mapping table in README.md.
— Add a 10µF capacitor across VCC/GND on the NRF24+PA+LNA module.

**GPS not reading**
— Confirm CP2102 VID/PID matches the list in `HalBridge.kt`.
— The firmware calls `Serial2.begin(9600)` — confirm your GPS module uses 9600 baud.

---

## Adding a new attack module

1. Add the `.cpp` source to `firmware/`
2. Add it to `FIRMWARE_SRC` in `CMakeLists.txt`
3. Add a new `ATTACK_*` constant in `HalBridge.kt`
4. Add a `case` for it in the `nativeRunAttack()` switch in `usb_hal.cpp`
5. Add an `AttackItem` entry in `ATTACK_MENU` in `MainActivity.kt`
