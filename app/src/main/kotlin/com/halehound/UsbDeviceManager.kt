package com.halehound

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.hardware.usb.UsbDevice
import android.hardware.usb.UsbManager

// Listens for USB attach/detach and permission grants, then notifies the app.
class UsbDeviceManager(
    private val context: Context,
    private val onDeviceReady: (UsbDevice, String) -> Unit,
    private val onDeviceGone:  (UsbDevice) -> Unit,
    private val onStatus:      (String) -> Unit
) {
    private val receiver = object : BroadcastReceiver() {
        override fun onReceive(ctx: Context, intent: Intent) {
            val device = intent.getParcelableExtra<UsbDevice>(UsbManager.EXTRA_DEVICE)
                ?: return
            when (intent.action) {
                ACTION_USB_PERMISSION -> {
                    val granted = intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)
                    if (granted) {
                        onStatus("Permission granted for ${device.deviceName}")
                        onDeviceReady(device, labelFor(device))
                    } else {
                        onStatus("Permission DENIED for ${device.deviceName}")
                    }
                }
                UsbManager.ACTION_USB_DEVICE_ATTACHED -> {
                    onStatus("Device attached: ${labelFor(device)}")
                    onDeviceReady(device, labelFor(device))
                }
                UsbManager.ACTION_USB_DEVICE_DETACHED -> {
                    onStatus("Device detached: ${labelFor(device)}")
                    onDeviceGone(device)
                }
            }
        }
    }

    fun register() {
        val filter = IntentFilter().apply {
            addAction(ACTION_USB_PERMISSION)
            addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED)
            addAction(UsbManager.ACTION_USB_DEVICE_DETACHED)
        }
        context.registerReceiver(receiver, filter)
    }

    fun unregister() {
        runCatching { context.unregisterReceiver(receiver) }
    }

    private fun labelFor(d: UsbDevice): String = when {
        d.vendorId == 0x1A86 -> "CH341A (SPI bridge)"
        d.vendorId == 0x10C4 -> "CP2102 (GPS UART)"
        d.vendorId == 0x0BDA -> "USB WiFi dongle"
        else -> "Unknown device VID=${d.vendorId.toString(16)}"
    }
}
