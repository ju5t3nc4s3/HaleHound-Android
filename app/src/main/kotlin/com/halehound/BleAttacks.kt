package com.halehound

import android.bluetooth.*
import android.bluetooth.le.*
import android.content.Context
import android.os.ParcelUuid
import kotlinx.coroutines.*
import java.util.UUID

// BleAttacks — replaces bluetooth_attacks.cpp from the original firmware.
// Uses Android BLE API. Passive scanning works without root.
// Active jamming/injection requires CAP_NET_RAW (root only).
class BleAttacks(private val context: Context) {

    private val btManager  = context.getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
    private val btAdapter  = btManager.adapter
    private val bleScanner = btAdapter.bluetoothLeScanner
    private val bleAdvert  = btAdapter.bluetoothLeAdvertiser

    private var scanCallback: ScanCallback? = null
    private var advertCallback: AdvertiseCallback? = null

    // ── Passive BLE scan ──────────────────────────────────────────────────────
    fun startScan(onDevice: (ScanResult) -> Unit) {
        scanCallback = object : ScanCallback() {
            override fun onScanResult(callbackType: Int, result: ScanResult) {
                onDevice(result)
            }
        }
        val settings = ScanSettings.Builder()
            .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
            .build()
        bleScanner.startScan(null, settings, scanCallback)
    }

    fun stopScan() {
        scanCallback?.let { bleScanner.stopScan(it) }
        scanCallback = null
    }

    // ── AirTag / FindMy tracker detection ────────────────────────────────────
    // Filters for Apple manufacturer data (company ID 0x004C) with FindMy type bytes
    fun startAirTagDetect(onFound: (ScanResult, String) -> Unit) {
        scanCallback = object : ScanCallback() {
            override fun onScanResult(callbackType: Int, result: ScanResult) {
                val mfr = result.scanRecord?.manufacturerSpecificData ?: return
                val appleData = mfr.get(0x004C) ?: return
                if (appleData.size >= 2 && (appleData[0] == 0x12.toByte() || appleData[0] == 0x19.toByte())) {
                    val status = "FindMy tracker — RSSI ${result.rssi} dBm"
                    onFound(result, status)
                }
            }
        }
        bleScanner.startScan(null,
            ScanSettings.Builder().setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY).build(),
            scanCallback)
    }

    // ── BLE beacon advertising ────────────────────────────────────────────────
    fun startBeacon(serviceUuid: String, onStarted: (Boolean) -> Unit) {
        val settings = AdvertiseSettings.Builder()
            .setAdvertiseMode(AdvertiseSettings.ADVERTISE_MODE_LOW_LATENCY)
            .setTxPowerLevel(AdvertiseSettings.ADVERTISE_TX_POWER_HIGH)
            .setConnectable(false)
            .build()

        val data = AdvertiseData.Builder()
            .setIncludeDeviceName(false)
            .addServiceUuid(ParcelUuid(UUID.fromString(serviceUuid)))
            .build()

        advertCallback = object : AdvertiseCallback() {
            override fun onStartSuccess(params: AdvertiseSettings) { onStarted(true) }
            override fun onStartFailure(errorCode: Int)            { onStarted(false) }
        }
        bleAdvert.startAdvertising(settings, data, advertCallback)
    }

    fun stopBeacon() {
        advertCallback?.let { bleAdvert.stopAdvertising(it) }
        advertCallback = null
    }

    fun close() {
        stopScan()
        stopBeacon()
    }
}
