// firmware-stubs/wifi_attacks_android.cpp
//
// Android-native replacement for wifi_attacks.cpp.
// The original file uses the ESP32 RF stack (esp_wifi_80211_tx, promiscuous mode, etc.)
// which cannot be cross-compiled. This stub provides the same function signatures
// and routes calls to Android's WifiManager via JNI callbacks.
//
// Raw frame injection (deauth, beacon spam) requires either:
//   a) A rooted device + Nexmon kernel patch
//   b) A USB WiFi adapter (Alfa AWUS036ACH) with monitor-mode support
//
// For now stubs return immediately so the rest of the firmware compiles cleanly.
// Replace each stub body with real Android logic as needed.

#include <android/log.h>
#define TAG "HaleHound-WiFi"

extern "C" {

// ── Callbacks into Kotlin (set by JNI on init) ───────────────────────────────
// Kotlin registers these function pointers when HalBridge.nativeInitCH341() runs.
// They forward scan results and log lines back to the UI.
static void (*g_log_cb)(const char*)        = nullptr;
static void (*g_scan_result_cb)(const char*)= nullptr;

void android_wifi_set_log_cb(void (*cb)(const char*))          { g_log_cb = cb; }
void android_wifi_set_scan_cb(void (*cb)(const char*))         { g_scan_result_cb = cb; }

static void log(const char* msg) {
    __android_log_print(ANDROID_LOG_INFO, TAG, "%s", msg);
    if (g_log_cb) g_log_cb(msg);
}

// ── Stubs matching wifi_attacks.h public API ──────────────────────────────────

void wifi_packet_monitor_start()  { log("[WiFi] Packet monitor — use Android WifiManager"); }
void wifi_packet_monitor_stop()   {}

void wifi_beacon_spammer_start()  { log("[WiFi] Beacon spam requires root + Nexmon"); }
void wifi_beacon_spammer_stop()   {}

void wifi_deauther_scan()         { log("[WiFi] Scan — trigger WifiManager.startScan() from Kotlin"); }
void wifi_deauther_start(const uint8_t* /*bssid*/, uint8_t /*ch*/) {
    log("[WiFi] Deauth requires raw 802.11 TX (root + Nexmon or USB NIC)");
}
void wifi_deauther_stop() {}

void wifi_probe_sniffer_start()   { log("[WiFi] Probe sniff requires monitor mode"); }
void wifi_probe_sniffer_stop()    {}

void wifi_captive_portal_start(const char* ssid) {
    log("[WiFi] Captive portal — start hotspot via WifiManager.startLocalOnlyHotspot()");
}
void wifi_captive_portal_stop() {}

void wifi_station_scanner_start() { log("[WiFi] Station scan — use WifiManager"); }
void wifi_station_scanner_stop()  {}

void wifi_auth_flood_start(const uint8_t* /*bssid*/, uint8_t /*ch*/) {
    log("[WiFi] Auth flood requires raw 802.11 TX (root + Nexmon)");
}
void wifi_auth_flood_stop() {}

} // extern "C"
