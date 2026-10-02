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
import androidx.compose.foundation.layout.ColumnScope
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Build
import androidx.compose.material.icons.filled.Home
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.NavigationBar
import androidx.compose.material3.NavigationBarItem
import androidx.compose.material3.NavigationBarItemDefaults
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Slider
import androidx.compose.material3.SliderDefaults
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.key
import androidx.compose.runtime.mutableStateMapOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.unit.dp
import androidx.core.content.ContextCompat
import kotlin.math.abs
import kotlin.math.roundToInt

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

    private val enableBtLaunch = registerForActivityResult(
        ActivityResultContracts.StartActivityForResult()
    ) { result ->
        if (result.resultCode == RESULT_OK) ensurePermissionAndScan()
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
                AppShell(
                    link = linkText,
                    reply = replyText,
                    device = device,
                    onConnect = { ensurePermissionAndScan() },
                    onDisconnect = { ble.disconnect() },
                    onCommand = { ble.send(it) },
                )
            }
        }
        ensurePermissionAndScan()
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
            enableBtLaunch.launch(Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE))
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
            listOf(
                Manifest.permission.BLUETOOTH_SCAN,
                Manifest.permission.BLUETOOTH_CONNECT,
                Manifest.permission.ACCESS_FINE_LOCATION,
            )
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
private val Muted = Color(0xFF8B949E)

private enum class AppTab(val label: String, val icon: ImageVector) {
    Home("首页", Icons.Filled.Home),
    Settings("设置", Icons.Filled.Settings),
    Debug("调试", Icons.Filled.Build),
}

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
private fun AppShell(
    link: String,
    reply: String,
    device: DeviceUi?,
    onConnect: () -> Unit,
    onDisconnect: () -> Unit,
    onCommand: (String) -> Unit,
) {
    var tab by remember { mutableStateOf(AppTab.Home) }
    val volumes = remember { mutableStateMapOf<Int, Int>() }
    val lows = remember { mutableStateMapOf<Int, String>() }
    val highs = remember { mutableStateMapOf<Int, String>() }
    Scaffold(
        containerColor = Bg,
        bottomBar = {
            NavigationBar(containerColor = Color(0xFF12181F)) {
                AppTab.entries.forEach { item ->
                    NavigationBarItem(
                        selected = tab == item,
                        onClick = { tab = item },
                        icon = { Icon(item.icon, contentDescription = item.label) },
                        label = { Text(item.label) },
                        colors = NavigationBarItemDefaults.colors(
                            selectedIconColor = Color(0xFF0F1419),
                            selectedTextColor = Accent,
                            indicatorColor = Accent,
                            unselectedIconColor = Muted,
                            unselectedTextColor = Muted,
                        ),
                    )
                }
            }
        },
    ) { padding ->
        when (tab) {
            AppTab.Home -> HomePage(link, reply, device, volumes, onCommand, Modifier.padding(padding))
            AppTab.Settings -> SettingsPage(
                link, reply, device, lows, highs, onConnect, onDisconnect, onCommand, Modifier.padding(padding),
            )
            AppTab.Debug -> DebugPage(link, reply, device, onCommand, Modifier.padding(padding))
        }
    }
}

@Composable
private fun PageColumn(
    title: String,
    link: String,
    reply: String,
    modifier: Modifier,
    content: @Composable ColumnScope.() -> Unit,
) {
    Column(
        modifier
            .fillMaxSize()
            .verticalScroll(rememberScrollState())
            .padding(16.dp),
        verticalArrangement = Arrangement.spacedBy(10.dp),
    ) {
        Text(title, style = MaterialTheme.typography.headlineSmall, color = Accent)
        Text(link, color = Muted)
        content()
        if (reply.isNotBlank()) Text(reply, color = Muted)
        Spacer(Modifier.height(12.dp))
    }
}

@Composable
private fun HomePage(
    link: String,
    reply: String,
    device: DeviceUi?,
    volumes: MutableMap<Int, Int>,
    onCommand: (String) -> Unit,
    modifier: Modifier,
) {
    PageColumn("首页", link, reply, modifier) {
        if (device == null) {
            Text("还没连上灌溉器。打开 App 会自动连一次，也可以到「设置」里再连。", color = Muted)
        } else {
            StatusCard(device, onCommand)
            device.zones.forEach { zone ->
                key(zone.index) {
                    HomeZoneCard(
                        zone,
                        volumes[zone.index] ?: nearestVolume(zone.volume),
                        onVolume = { volumes[zone.index] = it },
                        onCommand = onCommand,
                    )
                }
            }
        }
    }
}

@Composable
private fun SettingsPage(
    link: String,
    reply: String,
    device: DeviceUi?,
    lows: MutableMap<Int, String>,
    highs: MutableMap<Int, String>,
    onConnect: () -> Unit,
    onDisconnect: () -> Unit,
    onCommand: (String) -> Unit,
    modifier: Modifier,
) {
    PageColumn("设置", link, reply, modifier) {
        Card(colors = CardDefaults.cardColors(containerColor = CardBg), modifier = Modifier.fillMaxWidth()) {
            Column(Modifier.padding(14.dp), verticalArrangement = Arrangement.spacedBy(8.dp)) {
                Text("蓝牙", color = Color.White)
                Text("设备名 Langua。vivo 需要打开系统定位才能搜到。", color = Muted)
                Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                    Button(onClick = onConnect) { Text("连接") }
                    Button(
                        onClick = onDisconnect,
                        colors = ButtonDefaults.buttonColors(containerColor = Color(0xFF21262D)),
                    ) { Text("断开") }
                }
            }
        }
        if (device == null) {
            Text("连上之后可以改每盆的湿度上下限。", color = Muted)
        } else {
            device.zones.forEach { zone ->
                key(zone.index) {
                    SettingsZoneCard(zone, lows, highs, onCommand)
                }
            }
        }
    }
}

@Composable
private fun DebugPage(
    link: String,
    reply: String,
    device: DeviceUi?,
    onCommand: (String) -> Unit,
    modifier: Modifier,
) {
    PageColumn("调试", link, reply, modifier) {
        if (device == null) {
            Text("连上之后可以采样、标定、开关泵阀。", color = Muted)
        } else {
            ServiceCard(device, onCommand)
            device.zones.forEach { zone ->
                DebugZoneCard(
                    zone,
                    valveOpen = device.valveOn && device.activeValve == zone.index,
                    onCommand = onCommand,
                )
            }
        }
    }
}

@Composable
private fun StatusCard(device: DeviceUi, onCommand: (String) -> Unit) {
    val fault = device.state == 5 || device.locked
    val tone = if (fault) Warn else Accent
    val valve = if (device.valveOn && device.activeValve >= 0) "${device.activeValve + 1}#" else "关"
    Card(colors = CardDefaults.cardColors(containerColor = CardBg), modifier = Modifier.fillMaxWidth()) {
        Column(Modifier.padding(14.dp), verticalArrangement = Arrangement.spacedBy(8.dp)) {
            Row(
                Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically,
            ) {
                Column {
                    Text("今日浇水", color = Color.White, style = MaterialTheme.typography.titleMedium)
                    Text("${Protocol.stateText(device.state)} · ${Protocol.safetyText(device.safety)}", color = tone)
                }
                Text("${device.dailyMl} ml", color = tone, style = MaterialTheme.typography.headlineSmall)
            }
            Row(Modifier.fillMaxWidth()) {
                SummaryStat("水泵", if (device.pump) "开" else "关", Modifier.weight(1f))
                SummaryStat("阀门", valve, Modifier.weight(1f))
                SummaryStat("队列", device.queue.toString(), Modifier.weight(1f))
            }
            Button(
                onClick = { onCommand("estop") },
                modifier = Modifier.fillMaxWidth(),
                colors = ButtonDefaults.buttonColors(containerColor = Warn),
            ) { Text("急停") }
            if (fault) {
                Button(
                    onClick = { onCommand("stop") },
                    modifier = Modifier.fillMaxWidth(),
                    colors = ButtonDefaults.buttonColors(containerColor = Color(0xFF21262D)),
                ) { Text("恢复运行") }
            }
        }
    }
}

@Composable
private fun SummaryStat(label: String, value: String, modifier: Modifier = Modifier) {
    Column(modifier, horizontalAlignment = Alignment.CenterHorizontally) {
        Text(value, color = Color.White, style = MaterialTheme.typography.titleMedium)
        Text(label, color = Muted)
    }
}

@Composable
private fun ServiceCard(device: DeviceUi, onCommand: (String) -> Unit) {
    Card(colors = CardDefaults.cardColors(containerColor = CardBg), modifier = Modifier.fillMaxWidth()) {
        Column(Modifier.padding(14.dp), verticalArrangement = Arrangement.spacedBy(8.dp)) {
            Text("现场调试", color = Color.White)
            Text("直接开关泵和阀，不经过浇水保护。标定会先采样再写入当前 ADC。", color = Muted)
            Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                Button(onClick = { onCommand("sample") }) { Text("采样") }
                Button(
                    onClick = { onCommand(if (device.pump) "pump 0" else "pump 1") },
                    colors = ButtonDefaults.buttonColors(containerColor = if (device.pump) Warn else Accent),
                ) { Text(if (device.pump) "关泵" else "开泵") }
            }
        }
    }
}

@Composable
private fun HomeZoneCard(
    zone: ZoneUi,
    volume: Int,
    onVolume: (Int) -> Unit,
    onCommand: (String) -> Unit,
) {
    val tone = zoneTone(zone)
    val ml = nearestVolume(volume)
    val index = VolumeStops.indexOf(ml).coerceAtLeast(0)
    Card(colors = CardDefaults.cardColors(containerColor = CardBg), modifier = Modifier.fillMaxWidth()) {
        Column(Modifier.padding(14.dp), verticalArrangement = Arrangement.spacedBy(8.dp)) {
            Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween, verticalAlignment = Alignment.CenterVertically) {
                Column {
                    Text(zone.name, color = Color.White, style = MaterialTheme.typography.titleMedium)
                    Text(zoneLabel(zone), color = tone)
                }
                Text("${if (zone.valid) zone.pct.toString() else "--"}%", color = tone, style = MaterialTheme.typography.headlineSmall)
            }
            Row(verticalAlignment = Alignment.CenterVertically, horizontalArrangement = Arrangement.SpaceBetween, modifier = Modifier.fillMaxWidth()) {
                Text("自动浇水", color = Color.White)
                Switch(checked = zone.auto, onCheckedChange = { onCommand("auto ${zone.index} ${if (it) 1 else 0}") })
            }
            Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween) {
                Text("水量", color = Color.White)
                Text("$ml ml", color = Accent)
            }
            Slider(
                value = index.toFloat(),
                onValueChange = { raw ->
                    val i = raw.roundToInt().coerceIn(0, VolumeStops.lastIndex)
                    onVolume(VolumeStops[i])
                },
                valueRange = 0f..VolumeStops.lastIndex.toFloat(),
                steps = VolumeStops.size - 2,
                colors = SliderDefaults.colors(
                    thumbColor = Accent,
                    activeTrackColor = Accent,
                    inactiveTrackColor = Color(0xFF2C3544),
                ),
            )
            Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween) {
                Text("100", color = Muted)
                Text("1000", color = Muted)
            }
            Button(
                onClick = {
                    onCommand("vol ${zone.index} $ml")
                    onCommand("water ${zone.index} $ml")
                },
                modifier = Modifier.fillMaxWidth(),
            ) { Text("浇水") }
        }
    }
}

@Composable
private fun SettingsZoneCard(
    zone: ZoneUi,
    lows: MutableMap<Int, String>,
    highs: MutableMap<Int, String>,
    onCommand: (String) -> Unit,
) {
    val low = lows[zone.index] ?: zone.low.toString()
    val high = highs[zone.index] ?: zone.high.toString()
    Card(colors = CardDefaults.cardColors(containerColor = CardBg), modifier = Modifier.fillMaxWidth()) {
        Column(Modifier.padding(14.dp), verticalArrangement = Arrangement.spacedBy(8.dp)) {
            Text(zone.name, color = Color.White, style = MaterialTheme.typography.titleMedium)
            Text("低于下限自动浇，高于上限视为偏湿", color = Muted)
            Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                OutlinedTextField(
                    value = low,
                    onValueChange = { if (it.length <= 3 && it.all(Char::isDigit)) lows[zone.index] = it },
                    label = { Text("下限 %") },
                    singleLine = true,
                    modifier = Modifier.weight(1f),
                )
                OutlinedTextField(
                    value = high,
                    onValueChange = { if (it.length <= 3 && it.all(Char::isDigit)) highs[zone.index] = it },
                    label = { Text("上限 %") },
                    singleLine = true,
                    modifier = Modifier.weight(1f),
                )
            }
            Button(
                onClick = {
                    val lo = low.toIntOrNull() ?: zone.low
                    val hi = high.toIntOrNull() ?: zone.high
                    onCommand("th ${zone.index} $lo $hi")
                },
                modifier = Modifier.fillMaxWidth(),
                colors = ButtonDefaults.buttonColors(containerColor = Color(0xFF21262D)),
            ) { Text("保存阈值") }
        }
    }
}

@Composable
private fun DebugZoneCard(
    zone: ZoneUi,
    valveOpen: Boolean,
    onCommand: (String) -> Unit,
) {
    Card(colors = CardDefaults.cardColors(containerColor = CardBg), modifier = Modifier.fillMaxWidth()) {
        Column(Modifier.padding(14.dp), verticalArrangement = Arrangement.spacedBy(8.dp)) {
            Text("${zone.name} · ADC ${zone.adc}", color = Color.White)
            Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                Button(onClick = { onCommand("cal ${zone.index} dry") }) { Text("标定干") }
                Button(onClick = { onCommand("cal ${zone.index} wet") }) { Text("标定湿") }
                Button(
                    onClick = { onCommand(if (valveOpen) "valve off" else "valve ${zone.index}") },
                    colors = ButtonDefaults.buttonColors(containerColor = if (valveOpen) Warn else Accent),
                ) { Text(if (valveOpen) "关阀" else "开阀") }
            }
        }
    }
}

private val VolumeStops = intArrayOf(100, 200, 300, 400, 500, 600, 700, 800, 1000)

private fun nearestVolume(ml: Int): Int = VolumeStops.minBy { abs(it - ml) }

private fun zoneTone(zone: ZoneUi): Color = when {
    !zone.valid -> Muted
    zone.pct < zone.low -> Warn
    zone.pct > zone.high -> Color(0xFF388BFD)
    else -> Accent
}

private fun zoneLabel(zone: ZoneUi): String = when {
    !zone.valid -> "未接"
    zone.pct < zone.low -> "偏干"
    zone.pct > zone.high -> "偏湿"
    else -> "正常"
}
