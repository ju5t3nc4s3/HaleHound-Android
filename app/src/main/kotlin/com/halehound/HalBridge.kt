package com.halehound

import android.content.Context
import android.hardware.usb.*
import android.app.PendingIntent
import android.content.Intent
import kotlinx.coroutines.*

// USB vendor/product IDs
private const val VID_CH341   = 0x1A86
private const val PID_CH341A  = 0x5512
private const val PID_CH341B  = 0x5523
private const val VID_CP2102  = 0x10C4
private const val PID_CP2102  = 0xEA60

const val ACTION_USB_PERMISSION = "com.halehound.USB_PERMISSION"

// Attack IDs — keep in sync with jni/attack_entry.cpp
enum class AttackId(val id: Int) {
    SUBGHZ_REPLAY(10),
    SUBGHZ_JAM(11),
    SUBGHZ_BRUTE(12),
    NRF24_SCAN(20),
    NRF24_JAM(21),
    GPS_START(30),
    KARMA(40),
    STOP(99)
}

class HalBridge(private val context: Context) {

    private val usbManager = context.getSystemService(Context.USB_SERVICE) as UsbManager
    private val scope = CoroutineScope(Dispatchers.IO + SupervisorJob())

    // Tracks open connections so we can close them on teardown
    private val openConnections = mutableListOf<UsbDeviceConnection>()

    // ── JNI declarations ──────────────────────────────────────────────────────
    external fun nativeInitCH341NRF24(fd: Int)
    external fun nativeInitCH341CC1101(fd: Int)
    external fun nativeInitCP2102(fd: Int)
    external fun nativeRunAttack(attackId: Int, params: String): String
    external fun nativeRadioTest(): String

    companion object {
        init { System.loadLibrary("halehound") }
    }

    // ── Device detection ──────────────────────────────────────────────────────
    private fun isCH341(d: UsbDevice)  = d.vendorId == VID_CH341 &&
            (d.productId == PID_CH341A || d.productId == PID_CH341B)
    private fun isCP2102(d: UsbDevice) = d.vendorId == VID_CP2102 && d.productId == PID_CP2102

    // ── Open all recognised USB OTG devices ───────────────────────────────────
    // Call this after receiving USB_DEVICE_ATTACHED or on app start.
    fun openDevices(onStatus: (String) -> Unit) {
        val devices = usbManager.deviceList.values.toList()
        val ch341s  = devices.filter { isCH341(it) }
        val cp2102s = devices.filter { isCP2102(it) }

        onStatus("Found: ${ch341s.size} CH341A, ${cp2102s.size} CP2102")

        // First CH341A -> NRF24, second -> CC1101
        ch341s.getOrNull(0)?.let { openAndInit(it, "CH341A #1 (NRF24)",  ::nativeInitCH341NRF24,  onStatus) }
        ch341s.getOrNull(1)?.let { openAndInit(it, "CH341A #2 (CC1101)", ::nativeInitCH341CC1101, onStatus) }
        cp2102s.getOrNull(0)?.let { openAndInit(it, "CP2102 (GPS)",       ::nativeInitCP2102,      onStatus) }
    }

    private fun openAndInit(
        device: UsbDevice,
        label: String,
        initFn: (Int) -> Unit,
        onStatus: (String) -> Unit
    ) {
        if (!usbManager.hasPermission(device)) {
            onStatus("Requesting permission for $label…")
            val pi = PendingIntent.getBroadcast(
                context, 0,
                Intent(ACTION_USB_PERMISSION),
                PendingIntent.FLAG_IMMUTABLE
            )
            usbManager.requestPermission(device, pi)
            return
        }
        val conn = usbManager.openDevice(device)
        if (conn == null) {
            onStatus("ERROR: Could not open $label")
            return
        }
        openConnections += conn
        initFn(conn.fileDescriptor)
        onStatus("$label ready (fd=${conn.fileDescriptor})")
    }

    // ── Run an attack ─────────────────────────────────────────────────────────
    fun runAttack(
        attack: AttackId,
        params: String = "",
        onResult: (String) -> Unit
    ) {
        scope.launch {
            val result = nativeRunAttack(attack.id, params)
            withContext(Dispatchers.Main) { onResult(result) }
        }
    }

    fun stopAll(onResult: (String) -> Unit) = runAttack(AttackId.STOP, "", onResult)

    // ── Radio hardware test ───────────────────────────────────────────────────
    fun radioTest(onResult: (String) -> Unit) {
        scope.launch {
            val result = nativeRadioTest()
            withContext(Dispatchers.Main) { onResult(result) }
        }
    }

    // ── Cleanup ───────────────────────────────────────────────────────────────
    fun close() {
        runAttack(AttackId.STOP, "") {}
        openConnections.forEach { it.close() }
        openConnections.clear()
        scope.cancel()
    }
}
