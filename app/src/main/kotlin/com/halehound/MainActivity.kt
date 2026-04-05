package com.halehound

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

// Minimal Jetpack Compose UI shell.
// Expand each section into a full screen as you port each attack module.
class MainActivity : ComponentActivity() {

    private lateinit var halBridge: HalBridge
    private lateinit var usbManager: UsbDeviceManager
    private lateinit var wifiAttacks: WifiAttacks
    private lateinit var bleAttacks: BleAttacks

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        halBridge   = HalBridge(this)
        wifiAttacks = WifiAttacks(this)
        bleAttacks  = BleAttacks(this)

        usbManager = UsbDeviceManager(
            context      = this,
            onDeviceReady = { _, label -> /* re-open devices */ },
            onDeviceGone  = { _ -> },
            onStatus      = { msg -> /* log */ }
        )
        usbManager.register()

        setContent { HaleHoundApp(halBridge, wifiAttacks, bleAttacks) }
    }

    override fun onDestroy() {
        super.onDestroy()
        halBridge.close()
        bleAttacks.close()
        usbManager.unregister()
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun HaleHoundApp(hal: HalBridge, wifi: WifiAttacks, ble: BleAttacks) {
    val log = remember { mutableStateListOf<String>() }
    var statusText by remember { mutableStateOf("Idle — connect USB OTG hub") }

    fun appendLog(msg: String) { log.add(0, msg) }

    MaterialTheme(colorScheme = darkColorScheme()) {
        Scaffold(
            topBar = { TopAppBar(title = { Text("HaleHound", fontSize = 20.sp) }) }
        ) { padding ->
            Column(
                modifier = Modifier
                    .padding(padding)
                    .padding(16.dp)
                    .fillMaxSize(),
                verticalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                // Status bar
                Text(statusText, color = MaterialTheme.colorScheme.onSurfaceVariant, fontSize = 13.sp)
                Divider()

                // USB init button
                Button(onClick = {
                    hal.openDevices { msg ->
                        statusText = msg
                        appendLog(msg)
                    }
                }, modifier = Modifier.fillMaxWidth()) {
                    Text("Init USB OTG devices")
                }

                // Radio self-test
                Button(onClick = {
                    hal.radioTest { appendLog("Radio test: $it") }
                }, modifier = Modifier.fillMaxWidth()) {
                    Text("Radio self-test (NRF24 + CC1101)")
                }

                Divider()
                Text("SubGHz / CC1101", style = MaterialTheme.typography.labelMedium)

                Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                    Button(onClick = {
                        hal.runAttack(AttackId.SUBGHZ_REPLAY, "433.92") { appendLog(it) }
                    }, modifier = Modifier.weight(1f)) { Text("Replay 433 MHz") }

                    Button(onClick = {
                        hal.runAttack(AttackId.SUBGHZ_JAM, "433.92") { appendLog(it) }
                    }, modifier = Modifier.weight(1f)) { Text("Jam 433 MHz") }
                }

                Divider()
                Text("NRF24 2.4 GHz", style = MaterialTheme.typography.labelMedium)

                Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                    Button(onClick = {
                        hal.runAttack(AttackId.NRF24_SCAN) { appendLog(it) }
                    }, modifier = Modifier.weight(1f)) { Text("Channel scan") }

                    Button(onClick = {
                        hal.runAttack(AttackId.NRF24_JAM) { appendLog(it) }
                    }, modifier = Modifier.weight(1f)) { Text("WLAN jam") }
                }

                Divider()
                Text("SIGINT", style = MaterialTheme.typography.labelMedium)

                Button(onClick = {
                    hal.runAttack(AttackId.KARMA) { appendLog(it) }
                }, modifier = Modifier.fillMaxWidth()) { Text("Karma attack") }

                Divider()

                // Stop all
                Button(
                    onClick = { hal.stopAll { appendLog("Stopped: $it") } },
                    colors  = ButtonDefaults.buttonColors(containerColor = MaterialTheme.colorScheme.error),
                    modifier = Modifier.fillMaxWidth()
                ) { Text("STOP ALL") }

                Divider()
                Text("Log", style = MaterialTheme.typography.labelMedium)

                LazyColumn(modifier = Modifier.fillMaxSize()) {
                    items(log) { entry ->
                        Text(entry, fontSize = 11.sp,
                            color = MaterialTheme.colorScheme.onSurfaceVariant,
                            modifier = Modifier.padding(vertical = 1.dp))
                    }
                }
            }
        }
    }
}
