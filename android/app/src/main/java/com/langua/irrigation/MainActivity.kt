package com.langua.irrigation

import android.Manifest
import android.bluetooth.BluetoothAdapter
import android.content.Intent
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Surface
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateMapOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import androidx.core.content.ContextCompat

class MainActivity : ComponentActivity() {
    private var linkText by mutableStateOf("未连接")
    private var replyText by mutableStateOf("")
    private var device by mutableStateOf<DeviceUi?>(null)
    private lateinit var ble: BleClient

    private val permissionLaunch = registerForActivityResult(
        ActivityResultContracts.RequestMultiplePermissions()
    ) { granted ->
        if (granted.values.all { it }) ble.startScan()
        else linkText = "需要蓝牙权限才能连接灌溉器"
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        ble = BleClient(
            this,
            onLink = { linkText = it },
            onDevice = { device = it },
            onReply = { replyText = it },
        )
        setContent {
            LanguaTheme {
                HomeScreen(
                    link = linkText,
                    reply = replyText,
                    device = device,
                    onConnect = { ensurePermissionAndScan() },
                    onDisconnect = { ble.disconnect() },
                    onCommand = { ble.send(it) },
                )
            }
        }
    }

    override fun onDestroy() {
        ble.disconnect()
        super.onDestroy()
    }

    private fun ensurePermissionAndScan() {
        val adapter = (getSystemService(BLUETOOTH_SERVICE) as android.bluetooth.BluetoothManager).adapter
        if (adapter == null) {
            linkText = "这台手机没有蓝牙"
            return
        }
        if (!adapter.isEnabled) {
            startActivity(Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE))
            return
        }
        val needed = requiredPermissions().filter {
            ContextCompat.checkSelfPermission(this, it) != PackageManager.PERMISSION_GRANTED
        }
        if (needed.isEmpty()) ble.startScan()
        else permissionLaunch.launch(needed.toTypedArray())
    }

    private fun requiredPermissions(): List<String> {
        return if (Build.VERSION.SDK_INT >= 31) {
            listOf(Manifest.permission.BLUETOOTH_SCAN, Manifest.permission.BLUETOOTH_CONNECT)
        } else {
            listOf(
                Manifest.permission.BLUETOOTH,
                Manifest.permission.BLUETOOTH_ADMIN,
                Manifest.permission.ACCESS_FINE_LOCATION,
            )
        }
    }
}

private val Bg = Color(0xFF0F1419)
private val CardBg = Color(0xFF1A2332)
private val Accent = Color(0xFF00CC88)
private val Warn = Color(0xFFFF5555)

@Composable
private fun LanguaTheme(content: @Composable () -> Unit) {
    MaterialTheme(
        colorScheme = darkColorScheme(
            background = Bg,
            surface = CardBg,
            primary = Accent,
            error = Warn,
        ),
        content = content,
    )
}

@Composable
private fun HomeScreen(
    link: String,
    reply: String,
    device: DeviceUi?,
    onConnect: () -> Unit,
    onDisconnect: () -> Unit,
    onCommand: (String) -> Unit,
) {
    val volumes = remember { mutableStateMapOf<Int, String>() }
    Surface(modifier = Modifier.fillMaxSize(), color = Bg) {
        Column(
            modifier = Modifier
                .fillMaxSize()
                .verticalScroll(rememberScrollState())
                .padding(16.dp),
            verticalArrangement = Arrangement.spacedBy(10.dp),
        ) {
            Text("蓝瓜浇水", style = MaterialTheme.typography.headlineSmall, color = Accent)
            Text(link, color = Color(0xFF8B949E))
            Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                Button(onClick = onConnect) { Text("连接") }
                Button(onClick = onDisconnect, colors = ButtonDefaults.buttonColors(containerColor = Color(0xFF21262D))) {
                    Text("断开")
                }
            }
            if (device == null) {
                Text("连接后可查看各盆湿度、浇水、开关自动模式。设备蓝牙名是 Langua。", color = Color(0xFF8B949E))
            } else {
                StatusCard(device, onCommand)
                device.zones.forEach { zone ->
                    ZoneCard(
                        zone,
                        volumes[zone.index] ?: zone.volume.toString(),
                        onVolume = { volumes[zone.index] = it },
                        onCommand = onCommand,
                    )
                }
            }
            if (reply.isNotBlank()) Text(reply, color = Color(0xFF8B949E))
            Spacer(Modifier.height(12.dp))
        }
    }
}

@Composable
private fun StatusCard(device: DeviceUi, onCommand: (String) -> Unit) {
    Card(colors = CardDefaults.cardColors(containerColor = if (device.state == 5 || device.locked) Color(0xFF2A1515) else CardBg)) {
        Column(Modifier.padding(14.dp), verticalArrangement = Arrangement.spacedBy(6.dp)) {
            Text("今日已浇 ${device.dailyMl} ml", color = Color.White)
            Text(
                "状态 ${Protocol.stateText(device.state)} · ${Protocol.safetyText(device.safety)}",
                color = Color.White,
            )
            val valve = if (device.valveOn && device.activeValve >= 0) "阀${device.activeValve}" else "阀关"
            Text("泵${if (device.pump) "开" else "关"} · $valve · 队列 ${device.queue}", color = Color.White)
            Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                Button(
                    onClick = { onCommand("estop") },
                    colors = ButtonDefaults.buttonColors(containerColor = Warn),
                ) { Text("急停") }
                if (device.state == 5 || device.locked) {
                    Button(onClick = { onCommand("stop") }) { Text("恢复运行") }
                }
            }
        }
    }
}

@Composable
private fun ZoneCard(
    zone: ZoneUi,
    volume: String,
    onVolume: (String) -> Unit,
    onCommand: (String) -> Unit,
) {
    val tone = when {
        !zone.valid -> Color(0xFF8B949E)
        zone.pct < zone.low -> Warn
        zone.pct > zone.high -> Color(0xFF388BFD)
        else -> Accent
    }
    val label = when {
        !zone.valid -> "未接"
        zone.pct < zone.low -> "偏干"
        zone.pct > zone.high -> "偏湿"
        else -> "正常"
    }
    Card(colors = CardDefaults.cardColors(containerColor = CardBg), modifier = Modifier.fillMaxWidth()) {
        Column(Modifier.padding(14.dp), verticalArrangement = Arrangement.spacedBy(8.dp)) {
            Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween, verticalAlignment = Alignment.CenterVertically) {
                Column {
                    Text(zone.name, color = Color.White)
                    Text(label, color = tone)
                }
                Text("${if (zone.valid) zone.pct.toString() else "--"}%", color = tone, style = MaterialTheme.typography.headlineSmall)
            }
            Row(verticalAlignment = Alignment.CenterVertically, horizontalArrangement = Arrangement.SpaceBetween, modifier = Modifier.fillMaxWidth()) {
                Text("自动浇水", color = Color.White)
                Switch(checked = zone.auto, onCheckedChange = { onCommand("auto ${zone.index} ${if (it) 1 else 0}") })
            }
            OutlinedTextField(
                value = volume,
                onValueChange = { if (it.length <= 4 && it.all(Char::isDigit)) onVolume(it) },
                label = { Text("水量 ml") },
                singleLine = true,
                modifier = Modifier.fillMaxWidth(),
            )
            Button(
                onClick = {
                    val ml = volume.toIntOrNull() ?: zone.volume
                    onCommand("vol ${zone.index} $ml")
                    onCommand("water ${zone.index} $ml")
                },
                modifier = Modifier.fillMaxWidth(),
            ) { Text("浇水") }
        }
    }
}
