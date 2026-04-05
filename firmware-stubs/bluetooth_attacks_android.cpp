// firmware-stubs/bluetooth_attacks_android.cpp
//
// Android-native replacement for bluetooth_attacks.cpp.
// The original uses ESP32's NimBLE/Arduino BLE stack which is not portable.
// Real BLE scanning and advertising are handled in Kotlin via BluetoothLeScanner
// and BluetoothLeAdvertiser. These stubs keep the NDK build clean and log intent.

#include <android/log.h>
#define TAG "HaleHound-BLE"

extern "C" {

static void (*g_log_cb)(const char*) = nullptr;
void android_ble_set_log_cb(void (*cb)(const char*)) { g_log_cb = cb; }

static void log(const char* msg) {
    __android_log_print(ANDROID_LOG_INFO, TAG, "%s", msg);
    if (g_log_cb) g_log_cb(msg);
}

// ── Stubs matching bluetooth_attacks.h public API ─────────────────────────────

void ble_jammer_start()  { log("[BLE] Jammer — ad-flood via BluetoothLeAdvertiser (root for injection)"); }
void ble_jammer_stop()   {}

void ble_spoofer_start(const uint8_t* /*addr*/) { log("[BLE] Spoofer — clone via BluetoothLeAdvertiser"); }
void ble_spoofer_stop()  {}

void ble_beacon_start()  { log("[BLE] Beacon — custom advert via BluetoothLeAdvertiser"); }
void ble_beacon_stop()   {}

void ble_sniffer_start() { log("[BLE] Sniffer — passive scan via BluetoothLeScanner"); }
void ble_sniffer_stop()  {}

void ble_scanner_start() { log("[BLE] Scanner — BluetoothLeScanner.startScan()"); }
void ble_scanner_stop()  {}

void ble_whisperpair_start() { log("[BLE] WhisperPair CVE-2025-36911 — GATT probe via BluetoothGatt"); }
void ble_whisperpair_stop()  {}

void ble_airtag_detect_start() { log("[BLE] AirTag detect — filter Apple 0x004C in scan callback"); }
void ble_airtag_detect_stop()  {}

} // extern "C"
