cmake_minimum_required(VERSION 3.22)
project(halehound CXX)

# ── libusb-android ────────────────────────────────────────────────────────────
# Build libusb from source for Android or use prebuilt .so files.
# Prebuilt path: libs/${ANDROID_ABI}/libusb1.0.so
# Source:        https://github.com/libusb/libusb
set(LIBUSB_DIR "${CMAKE_SOURCE_DIR}/libs/${ANDROID_ABI}")

add_library(libusb SHARED IMPORTED)
set_target_properties(libusb PROPERTIES
    IMPORTED_LOCATION "${LIBUSB_DIR}/libusb1.0.so"
)

# ── Firmware source files ─────────────────────────────────────────────────────
# Copy these from the HaleHound-CYD repo into firmware-stubs/.
# See firmware-stubs/PLACE_FIRMWARE_HERE.txt for the exact list.
# wifi_attacks.cpp and bluetooth_attacks.cpp are NOT included here —
# those are replaced by the Kotlin equivalents.
file(GLOB FIRMWARE_SOURCES
    "${CMAKE_SOURCE_DIR}/firmware-stubs/*.cpp"
)

# ── Shared library: libhalehound.so ──────────────────────────────────────────
add_library(halehound SHARED
    # JNI + HAL backend
    jni/usb_hal.cpp
    jni/SPI.cpp
    jni/HardwareSerial.cpp
    jni/attack_entry.cpp

    # Original firmware source (compiled unchanged)
    ${FIRMWARE_SOURCES}
)

target_include_directories(halehound PRIVATE
    # HAL shim headers come FIRST — they shadow any Arduino SDK includes
    hal/
    # Firmware headers
    firmware-stubs/
    # libusb
    ${CMAKE_SOURCE_DIR}/include/libusb
)

target_compile_options(halehound PRIVATE
    -std=c++17
    -Wall
    -Wno-unused-parameter
    -Wno-missing-field-initializers
    # Suppress ESP-IDF / Arduino-specific warnings that are harmless here
    -Wno-deprecated-declarations
)

target_link_libraries(halehound
    libusb
    android
    log
)
