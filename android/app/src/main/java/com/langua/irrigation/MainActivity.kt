package com.langua.irrigation

import android.Manifest
import android.bluetooth.BluetoothAdapter
import android.content.Intent
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import android.net.Uri
import androidx.activity.result.PickVisualMediaRequest
import androidx.activity.result.contract.ActivityResultContracts
import androidx.activity.result.contract.ActivityResultContracts.PickVisualMedia
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.sp
import androidx.compose.animation.core.LinearEasing
import androidx.compose.animation.core.animateFloat
import androidx.compose.animation.core.infiniteRepeatable
import androidx.compose.animation.core.rememberInfiniteTransition
import androidx.compose.animation.core.tween
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ColumnScope
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Build
import androidx.compose.material.icons.filled.Home
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material3.AlertDialog
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
import androidx.compose.material3.OutlinedTextFieldDefaults
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Slider
import androidx.compose.material3.SliderDefaults
import androidx.compose.material3.Snackbar
import androidx.compose.material3.SnackbarHost
import androidx.compose.material3.SnackbarHostState
import androidx.compose.material3.Switch
import androidx.compose.material3.SwitchDefaults
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.key
import androidx.compose.runtime.mutableStateMapOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.foundation.layout.width
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.window.DialogProperties
import androidx.core.content.ContextCompat
import kotlin.math.abs
import kotlin.math.roundToInt

class MainActivity : ComponentActivity() {
    private var linkText by mutableStateOf("未连接")
    private var notice by mutableStateOf<Notice?>(null)
    private var noticeSeq = 0
    private var device by mutableStateOf<DeviceUi?>(null)
    private var history by mutableStateOf<List<HistUi>>(emptyList())
    private var histLeft = 0
    private val histBuf = mutableListOf<HistUi>()
    private var wifiOn by mutableStateOf(true)
    private var wifiSsid by mutableStateOf("")
    private var wifiIp by mutableStateOf("")
    private lateinit var ble: BleClient

    private var mediaTick by mutableIntStateOf(0)
    private var photoFor = -1

    private val permissionLaunch = registerForActivityResult(
        ActivityResultContracts.RequestMultiplePermissions()
    ) { granted ->
        if (granted.values.all { it }) ble.startScan()
        else linkText = "需要蓝牙权限才能连接灌溉器"
    }

    private val pickPhoto = registerForActivityResult(PickVisualMedia()) { uri: Uri? ->
        val zone = photoFor
        if (uri != null && zone >= 0) {
            ZoneStore.savePhoto(this, zone, uri)
            AppLog.op(this, "实拍图 ${zone + 1}# 已保存")
            mediaTick += 1
        } else if (zone >= 0) {
            AppLog.op(this, "实拍图 ${zone + 1}# 取消选择")
        }
        photoFor = -1
    }

    private val enableBtLaunch = registerForActivityResult(
        ActivityResultContracts.StartActivityForResult()
    ) { result ->
        if (result.resultCode == RESULT_OK) ensurePermissionAndScan()
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        AppLog.op(this, "打开 App")
        SnapshotStore.load(this)?.let { saved ->
            device = saved.device
            history = HistoryArchive.merge(this, saved.history)
            wifiOn = saved.wifiOn
            wifiSsid = saved.wifiSsid
            wifiIp = saved.wifiIp
        }
        if (device == null) {
            history = HistoryArchive.load(this)
        }
        ble = BleClient(
            this,
            onLink = {
                linkText = it
                AppLog.op(this, "连接状态 $it")
            },
            onDevice = {
                device = it
                persistSnapshot()
                AppLog.status(this, it)
            },
            onReply = { raw ->
                val text = raw.trim()
                when {
                    text.startsWith("H ") -> {
                        histLeft = text.removePrefix("H ").toIntOrNull() ?: 0
                        histBuf.clear()
                    }
                    text.startsWith("R ") -> {
                        val p = text.removePrefix("R ").split(" ")
                        if (p.size >= 4) {
                            histBuf.add(
                                HistUi(
                                    ts = p[0].toLongOrNull() ?: 0L,
                                    zone = p[1].toIntOrNull() ?: 0,
                                    ml = p[2].toIntOrNull() ?: 0,
                                    trigger = p[3].toIntOrNull() ?: 0,
                                    outcome = p.getOrNull(4)?.toIntOrNull() ?: 0,
                                    seq = p.getOrNull(5)?.toLongOrNull() ?: 0L,
                                )
                            )
                            histLeft -= 1
                            if (histLeft <= 0) {
                                history = HistoryArchive.merge(this, histBuf.toList())
                                persistSnapshot()
                            }
                        }
                    }
                    text.startsWith("W ") -> {
                        val p = text.removePrefix("W ").split(" ", limit = 2)
                        wifiOn = p.getOrNull(0) == "1"
                        wifiIp = p.getOrNull(1)?.takeUnless { it == "-" } ?: ""
                        persistSnapshot()
                    }
                    text.startsWith("S ") -> {
                        wifiSsid = text.removePrefix("S ")
                        persistSnapshot()
                    }
                    text == "S" -> {
                        wifiSsid = ""
                        persistSnapshot()
                    }
                    else -> friendlyReply(text)?.let {
                        noticeSeq += 1
                        notice = Notice(noticeSeq, it)
                    }
                }
            },
        )
        setContent {
            LanguaTheme {
                AppShell(
                    link = linkText,
                    notice = notice,
                    device = device,
                    history = history,
                    wifiOn = wifiOn,
                    wifiSsid = wifiSsid,
                    wifiIp = wifiIp,
                    online = linkText == "已连接",
                    mediaTick = mediaTick,
                    onPickPhoto = { zone ->
                        AppLog.op(this, "选择实拍图 ${zone + 1}#")
                        photoFor = zone
                        pickPhoto.launch(PickVisualMediaRequest(PickVisualMedia.ImageOnly))
                    },
                    onLocalChange = { mediaTick += 1 },
                    onConnect = {
                        AppLog.op(this, "点击连接")
                        ensurePermissionAndScan()
                    },
                    onDisconnect = {
                        AppLog.op(this, "点击断开")
                        ble.disconnect()
                    },
                    onCommand = { cmd ->
                        if (linkText == "已连接") ble.send(cmd)
                        else AppLog.op(this, "未发送 $cmd")
                    },
                )
            }
        }
        ensurePermissionAndScan()
    }

    override fun onDestroy() {
        ble.disconnect()
        super.onDestroy()
    }

    private fun persistSnapshot() {
        val snap = device ?: return
        SnapshotStore.save(this, snap, history.take(20), wifiOn, wifiSsid, wifiIp)
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
            onPrimary = Color(0xFF0F1419),
            error = Warn,
            onError = Color.White,
        ),
        content = content,
    )
}

@Composable
private fun AppShell(
    link: String,
    notice: Notice?,
    device: DeviceUi?,
    history: List<HistUi>,
    wifiOn: Boolean,
    wifiSsid: String,
    wifiIp: String,
    online: Boolean,
    mediaTick: Int,
    onPickPhoto: (Int) -> Unit,
    onLocalChange: () -> Unit,
    onConnect: () -> Unit,
    onDisconnect: () -> Unit,
    onCommand: (String) -> Unit,
) {
    var tab by remember { mutableStateOf(AppTab.Home) }
    val volumes = remember { mutableStateMapOf<Int, Int>() }
    val lows = remember { mutableStateMapOf<Int, String>() }
    val highs = remember { mutableStateMapOf<Int, String>() }
    val snackbar = remember { SnackbarHostState() }
    LaunchedEffect(notice?.id) {
        val text = notice?.text ?: return@LaunchedEffect
        snackbar.showSnackbar(text)
    }
    LaunchedEffect(tab, online, device?.state, device?.safety, device?.dailyMl, device != null) {
        if (online && tab == AppTab.Home && device != null) onCommand("hist")
    }
    LaunchedEffect(tab, online) {
        if (online && tab == AppTab.Settings && device != null) onCommand("wifi?")
    }
    Scaffold(
        containerColor = Bg,
        snackbarHost = {
            SnackbarHost(snackbar) { data ->
                Snackbar(
                    snackbarData = data,
                    containerColor = Color(0xFF3A1C1C),
                    contentColor = Color.White,
                )
            }
        },
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
            AppTab.Home -> HomePage(link, device, history, volumes, online, mediaTick, onCommand, Modifier.padding(padding))
            AppTab.Settings -> SettingsPage(
                link, device, lows, highs, wifiOn, wifiSsid, wifiIp, online, mediaTick,
                onPickPhoto, onLocalChange, onConnect, onDisconnect, onCommand, Modifier.padding(padding),
            )
            AppTab.Debug -> DebugPage(link, device, online, onCommand, Modifier.padding(padding))
        }
    }
}

@Composable
private fun PageColumn(
    title: String,
    link: String,
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
        Row(
            Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically,
        ) {
            Text(title, style = MaterialTheme.typography.headlineSmall, color = Color.White)
            if (link == "已连接") Text("已连接", color = Accent)
        }
        if (link != "已连接") Text(link, color = linkColor(link))
        content()
        Spacer(Modifier.height(8.dp))
    }
}

@Composable
private fun HomePage(
    link: String,
    device: DeviceUi?,
    history: List<HistUi>,
    volumes: MutableMap<Int, Int>,
    online: Boolean,
    mediaTick: Int,
    onCommand: (String) -> Unit,
    modifier: Modifier,
) {
    PageColumn("首页", link, modifier) {
        if (device == null) {
            HintCard("还没连上灌溉器", "打开 App 会自动连一次，也可以到「设置」里再连。")
        } else {
            if (!online) OfflineNote()
            val watering = if (online && (device.state == 2 || device.state == 3) && device.activeValve >= 0) {
                device.activeValve
            } else {
                -1
            }
            PotOverview(device.zones, watering, mediaTick)
            StatusCard(device, online, onCommand)
            device.zones.forEach { zone ->
                key(zone.index) {
                    HomeZoneCard(
                        zone,
                        volumes[zone.index] ?: nearestVolume(zone.volume),
                        mediaTick,
                        online,
                        onVolume = { volumes[zone.index] = it },
                        onCommand = onCommand,
                    )
                }
            }
            HistoryCard(history, mediaTick)
        }
    }
}

@Composable
private fun SettingsPage(
    link: String,
    device: DeviceUi?,
    lows: MutableMap<Int, String>,
    highs: MutableMap<Int, String>,
    wifiOn: Boolean,
    wifiSsid: String,
    wifiIp: String,
    online: Boolean,
    mediaTick: Int,
    onPickPhoto: (Int) -> Unit,
    onLocalChange: () -> Unit,
    onConnect: () -> Unit,
    onDisconnect: () -> Unit,
    onCommand: (String) -> Unit,
    modifier: Modifier,
) {
    PageColumn("设置", link, modifier) {
        Card(colors = CardDefaults.cardColors(containerColor = CardBg), modifier = Modifier.fillMaxWidth()) {
            Column(Modifier.padding(14.dp), verticalArrangement = Arrangement.spacedBy(8.dp)) {
                Text("蓝牙", color = Color.White, style = MaterialTheme.typography.titleMedium)
                Text("设备名 Langua。vivo 需要打开系统定位才能搜到。", color = Muted)
                Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                    Button(onClick = onConnect, modifier = Modifier.weight(1f)) { Text("连接") }
                    Button(
                        onClick = onDisconnect,
                        enabled = online,
                        modifier = Modifier.weight(1f),
                        colors = ButtonDefaults.buttonColors(
                            containerColor = Color(0xFF21262D),
                            contentColor = Color.White,
                        ),
                    ) { Text("断开") }
                }
            }
        }
        if (device == null) {
            HintCard("还没有盆的设置", "连上之后可以改每盆的湿度上下限。")
        } else {
            if (!online) OfflineNote()
            Text("低于下限自动浇，高于上限视为偏湿。定时和自动时段在每张卡片里。", color = Muted)
            RulesCard(device, online, onCommand)
            WifiCard(wifiOn, wifiSsid, wifiIp, device.wifiUp, online, onCommand)
            device.zones.forEach { zone ->
                key(zone.index) {
                    SettingsZoneCard(zone, lows, highs, mediaTick, online, onPickPhoto, onLocalChange, onCommand)
                }
            }
            ZoneCountCard(device.zones.size, online, onCommand)
        }
    }
}

@Composable
private fun DebugPage(
    link: String,
    device: DeviceUi?,
    online: Boolean,
    onCommand: (String) -> Unit,
    modifier: Modifier,
) {
    PageColumn("调试", link, modifier) {
        if (device == null) {
            HintCard("还不能调试", "连上之后可以采样、标定、开关泵阀。")
        } else {
            if (!online) OfflineNote()
            ServiceCard(device, online, onCommand)
            FlowCard(device, online, onCommand)
            device.zones.forEach { zone ->
                DebugZoneCard(
                    zone,
                    valveOpen = device.valveOn && device.activeValve == zone.index,
                    online = online,
                    onCommand = onCommand,
                )
            }
        }
        LogCard()
    }
}

@Composable
private fun LogCard() {
    val context = LocalContext.current
    Card(colors = CardDefaults.cardColors(containerColor = CardBg), modifier = Modifier.fillMaxWidth()) {
        Column(Modifier.padding(14.dp), verticalArrangement = Arrangement.spacedBy(8.dp)) {
            Text("维护日志", color = Color.White, style = MaterialTheme.typography.titleMedium)
            Text("操作和蓝牙往来存在这台手机，保留最近 30 天。WiFi 密码不会写入。", color = Muted)
            Button(
                onClick = { (context as? android.app.Activity)?.let { AppLog.share(it) } },
                modifier = Modifier.fillMaxWidth(),
            ) { Text("分享日志") }
        }
    }
}

@Composable
private fun OfflineNote() {
    Text("未连接，下面是断开前的状态，连上后才能操作。", color = Muted)
}

@Composable
private fun StatusCard(device: DeviceUi, online: Boolean, onCommand: (String) -> Unit) {
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
                    if (device.sessionMl > 0) {
                        Text("本次 ${device.sessionMl} ml", color = Muted)
                    }
                }
                Column(horizontalAlignment = Alignment.End) {
                    Text("${device.dailyMl} ml", color = tone, style = MaterialTheme.typography.headlineSmall)
                    Text("限额 ${device.dailyLimit}", color = Muted)
                }
            }
            Row(Modifier.fillMaxWidth()) {
                SummaryStat("水泵", if (device.pump) "开" else "关", Modifier.weight(1f))
                SummaryStat("阀门", valve, Modifier.weight(1f))
                SummaryStat("队列", device.queue.toString(), Modifier.weight(1f))
            }
            Button(
                onClick = { onCommand("detect") },
                enabled = online,
                modifier = Modifier.fillMaxWidth(),
            ) { Text("检测并浇水") }
            Button(
                onClick = { onCommand("estop") },
                enabled = online,
                modifier = Modifier.fillMaxWidth(),
                colors = ButtonDefaults.buttonColors(
                    containerColor = Warn,
                    contentColor = Color.White,
                    disabledContainerColor = Color(0xFF2A3140),
                    disabledContentColor = Muted,
                ),
            ) { Text("急停") }
            if (fault) {
                Button(
                    onClick = { onCommand("stop") },
                    enabled = online,
                    modifier = Modifier.fillMaxWidth(),
                    colors = ButtonDefaults.buttonColors(
                        containerColor = Color(0xFF21262D),
                        contentColor = Color.White,
                        disabledContainerColor = Color(0xFF2A3140),
                        disabledContentColor = Muted,
                    ),
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
private fun ServiceCard(device: DeviceUi, online: Boolean, onCommand: (String) -> Unit) {
    Card(colors = CardDefaults.cardColors(containerColor = CardBg), modifier = Modifier.fillMaxWidth()) {
        Column(Modifier.padding(14.dp), verticalArrangement = Arrangement.spacedBy(8.dp)) {
            Text("现场调试", color = Color.White, style = MaterialTheme.typography.titleMedium)
            Text("直接开关泵和阀，不经过浇水保护。标定会先采样再写入当前 ADC。", color = Muted)
            Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                Button(onClick = { onCommand("sample") }, enabled = online, modifier = Modifier.weight(1f)) { Text("采样") }
                Button(
                    onClick = { onCommand(if (device.pump) "pump 0" else "pump 1") },
                    enabled = online,
                    modifier = Modifier.weight(1f),
                    colors = ButtonDefaults.buttonColors(
                        containerColor = if (device.pump) Warn else Accent,
                        contentColor = if (device.pump) Color.White else Color(0xFF0F1419),
                        disabledContainerColor = Color(0xFF2A3140),
                        disabledContentColor = Muted,
                    ),
                ) { Text(if (device.pump) "关泵" else "开泵") }
            }
        }
    }
}

@Composable
private fun HomeZoneCard(
    zone: ZoneUi,
    volume: Int,
    mediaTick: Int,
    online: Boolean,
    onVolume: (Int) -> Unit,
    onCommand: (String) -> Unit,
) {
    val tone = zoneTone(zone)
    val ml = nearestVolume(volume)
    val index = VolumeStops.indexOf(ml).coerceAtLeast(0)
    val alias = localName(zone.index, mediaTick)
    Card(colors = CardDefaults.cardColors(containerColor = CardBg), modifier = Modifier.fillMaxWidth()) {
        Column(Modifier.padding(14.dp), verticalArrangement = Arrangement.spacedBy(8.dp)) {
            Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween, verticalAlignment = Alignment.CenterVertically) {
                Column {
                    Text(alias.ifBlank { "${zone.index + 1}#" }, color = Color.White, style = MaterialTheme.typography.titleMedium)
                    Text(
                        zoneLabel(zone) + if (zone.schedule) " · 定时 %02d:%02d".format(zone.hour, zone.minute) else "",
                        color = tone,
                    )
                }
                Text("${if (zone.valid) zone.pct.toString() else "--"}%", color = tone, style = MaterialTheme.typography.headlineSmall)
            }
            Row(verticalAlignment = Alignment.CenterVertically, horizontalArrangement = Arrangement.SpaceBetween, modifier = Modifier.fillMaxWidth()) {
                Text("自动浇水", color = Color.White)
                Switch(
                    checked = zone.auto,
                    enabled = online,
                    onCheckedChange = { onCommand("auto ${zone.index} ${if (it) 1 else 0}") },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = Color.White,
                        checkedTrackColor = Accent,
                        uncheckedThumbColor = Muted,
                        uncheckedTrackColor = Color(0xFF2C3544),
                        uncheckedBorderColor = Color(0xFF2C3544),
                    ),
                )
            }
            Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween) {
                Text("水量", color = Color.White)
                Text("$ml ml", color = Accent)
            }
            Column(verticalArrangement = Arrangement.spacedBy(0.dp)) {
                Slider(
                    value = index.toFloat(),
                    enabled = online,
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
                    Text("100 ml", color = Muted, style = MaterialTheme.typography.bodySmall)
                    Text("1000 ml", color = Muted, style = MaterialTheme.typography.bodySmall)
                }
            }
            Button(
                onClick = {
                    onCommand("vol ${zone.index} $ml")
                    onCommand("water ${zone.index} $ml")
                },
                enabled = online,
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
    mediaTick: Int,
    online: Boolean,
    onPickPhoto: (Int) -> Unit,
    onLocalChange: () -> Unit,
    onCommand: (String) -> Unit,
) {
    val context = LocalContext.current
    val low = lows[zone.index] ?: zone.low.toString()
    val high = highs[zone.index] ?: zone.high.toString()
    var name by remember(zone.index, mediaTick) { mutableStateOf(ZoneStore.name(context, zone.index)) }
    var schOn by remember(zone.schedule) { mutableStateOf(zone.schedule) }
    var hour by remember(zone.hour) { mutableStateOf(zone.hour.coerceIn(0, 23)) }
    var minute by remember(zone.minute) { mutableStateOf(zone.minute.coerceIn(0, 59)) }
    val photo = remember(zone.index, mediaTick) { ZoneStore.bitmap(context, zone.index) }
    Card(colors = CardDefaults.cardColors(containerColor = CardBg), modifier = Modifier.fillMaxWidth()) {
        Column(Modifier.padding(14.dp), verticalArrangement = Arrangement.spacedBy(8.dp)) {
            Text("${zone.index + 1}#", color = Muted)
            Row(horizontalArrangement = Arrangement.spacedBy(12.dp), verticalAlignment = Alignment.CenterVertically) {
                Box(
                    Modifier
                        .height(72.dp)
                        .aspectRatio(1f)
                        .clip(RoundedCornerShape(12.dp))
                        .background(Color(0xFF143028)),
                ) {
                    PotPicture(photo, "${zone.index + 1}#", Modifier.fillMaxSize())
                }
                Column(Modifier.weight(1f), verticalArrangement = Arrangement.spacedBy(8.dp)) {
                    Button(onClick = { onPickPhoto(zone.index) }, modifier = Modifier.fillMaxWidth()) { Text("实拍图") }
                    if (photo != null) {
                        DarkButton("去掉图片") {
                            ZoneStore.clearPhoto(context, zone.index)
                            AppLog.op(context, "去掉实拍图 ${zone.index + 1}#")
                            onLocalChange()
                        }
                    }
                }
            }
            OutlinedTextField(
                value = name,
                onValueChange = { if (it.length <= 12) name = it },
                label = { Text("别名，只存在这台手机") },
                singleLine = true,
                modifier = Modifier.fillMaxWidth(),
                colors = fieldColors(),
            )
            Button(
                onClick = {
                    ZoneStore.setName(context, zone.index, name)
                    AppLog.op(context, "别名 ${zone.index + 1}# ${name.trim()}")
                    onLocalChange()
                },
                modifier = Modifier.fillMaxWidth(),
            ) { Text("保存别名") }
            Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                OutlinedTextField(
                    value = low,
                    onValueChange = { if (it.length <= 3 && it.all(Char::isDigit)) lows[zone.index] = it },
                    enabled = online,
                    label = { Text("下限 %") },
                    singleLine = true,
                    modifier = Modifier.weight(1f),
                    colors = fieldColors(),
                )
                OutlinedTextField(
                    value = high,
                    onValueChange = { if (it.length <= 3 && it.all(Char::isDigit)) highs[zone.index] = it },
                    enabled = online,
                    label = { Text("上限 %") },
                    singleLine = true,
                    modifier = Modifier.weight(1f),
                    colors = fieldColors(),
                )
            }
            DarkButton("保存阈值", online) {
                val lo = low.toIntOrNull() ?: zone.low
                val hi = high.toIntOrNull() ?: zone.high
                onCommand("th ${zone.index} $lo $hi")
            }
            Row(
                Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically,
            ) {
                Text("定时浇水", color = Color.White)
                Switch(
                    checked = schOn,
                    enabled = online,
                    onCheckedChange = { schOn = it },
                    colors = switchColors(),
                )
            }
            if (schOn) {
                TimeRow("时刻", hour, minute, online, { hour = it }, { minute = it })
            }
            DarkButton("保存定时", online) {
                if (schOn) onCommand("sch ${zone.index} $hour $minute")
                else onCommand("sch ${zone.index} off")
            }
        }
    }
}

@Composable
private fun DebugZoneCard(
    zone: ZoneUi,
    valveOpen: Boolean,
    online: Boolean,
    onCommand: (String) -> Unit,
) {
    Card(colors = CardDefaults.cardColors(containerColor = CardBg), modifier = Modifier.fillMaxWidth()) {
        Column(Modifier.padding(14.dp), verticalArrangement = Arrangement.spacedBy(8.dp)) {
            Text(localName(zone.index, 0).ifBlank { "${zone.index + 1}#" }, color = Color.White, style = MaterialTheme.typography.titleMedium)
            Text("ADC ${zone.adc}", color = Muted)
            Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                Button(
                    onClick = { onCommand("cal ${zone.index} dry") },
                    enabled = online,
                    modifier = Modifier.weight(1f),
                    contentPadding = PaddingValues(horizontal = 8.dp, vertical = 10.dp),
                ) { Text("标定干") }
                Button(
                    onClick = { onCommand("cal ${zone.index} wet") },
                    enabled = online,
                    modifier = Modifier.weight(1f),
                    contentPadding = PaddingValues(horizontal = 8.dp, vertical = 10.dp),
                ) { Text("标定湿") }
                Button(
                    onClick = { onCommand(if (valveOpen) "valve off" else "valve ${zone.index}") },
                    enabled = online,
                    modifier = Modifier.weight(1f),
                    contentPadding = PaddingValues(horizontal = 8.dp, vertical = 10.dp),
                    colors = ButtonDefaults.buttonColors(
                        containerColor = if (valveOpen) Warn else Accent,
                        contentColor = if (valveOpen) Color.White else Color(0xFF0F1419),
                        disabledContainerColor = Color(0xFF2A3140),
                        disabledContentColor = Muted,
                    ),
                ) { Text(if (valveOpen) "关阀" else "开阀") }
            }
        }
    }
}

private val LimitStops = intArrayOf(500, 1000, 1500, 2000, 3000, 4000, 5000, 8000, 10000)

private fun nearestLimit(ml: Long): Int = LimitStops.minBy { abs(it - ml.toInt()) }

@Composable
private fun switchColors() = SwitchDefaults.colors(
    checkedThumbColor = Color.White,
    checkedTrackColor = Accent,
    uncheckedThumbColor = Muted,
    uncheckedTrackColor = Color(0xFF2C3544),
    uncheckedBorderColor = Color(0xFF2C3544),
)

@Composable
private fun DarkButton(text: String, enabled: Boolean = true, onClick: () -> Unit) {
    Button(
        onClick = onClick,
        enabled = enabled,
        modifier = Modifier.fillMaxWidth(),
        colors = ButtonDefaults.buttonColors(
            containerColor = Color(0xFF21262D),
            contentColor = Color.White,
            disabledContainerColor = Color(0xFF2A3140),
            disabledContentColor = Muted,
        ),
    ) { Text(text) }
}

@Composable
private fun TimeRow(
    label: String,
    hour: Int,
    minute: Int,
    enabled: Boolean,
    onHour: (Int) -> Unit,
    onMinute: (Int) -> Unit,
) {
    Row(
        Modifier.fillMaxWidth(),
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.SpaceBetween,
    ) {
        Text(label, color = Color.White)
        Stepper(hour, 0, 23, 1, enabled, onHour)
        Text(":", color = Muted)
        Stepper(minute, 0, 59, 5, enabled, onMinute)
    }
}

@Composable
private fun Stepper(value: Int, min: Int, max: Int, step: Int, enabled: Boolean, onChange: (Int) -> Unit) {
    Row(verticalAlignment = Alignment.CenterVertically) {
        Button(
            onClick = { onChange((value - step).coerceIn(min, max)) },
            enabled = enabled,
            contentPadding = PaddingValues(horizontal = 10.dp, vertical = 4.dp),
            colors = ButtonDefaults.buttonColors(
                containerColor = Color(0xFF21262D),
                contentColor = Color.White,
                disabledContainerColor = Color(0xFF2A3140),
                disabledContentColor = Muted,
            ),
        ) { Text("−") }
        Text(
            "%02d".format(value),
            color = Color.White,
            modifier = Modifier.width(36.dp),
            textAlign = TextAlign.Center,
        )
        Button(
            onClick = { onChange((value + step).coerceIn(min, max)) },
            enabled = enabled,
            contentPadding = PaddingValues(horizontal = 10.dp, vertical = 4.dp),
            colors = ButtonDefaults.buttonColors(
                containerColor = Color(0xFF21262D),
                contentColor = Color.White,
                disabledContainerColor = Color(0xFF2A3140),
                disabledContentColor = Muted,
            ),
        ) { Text("+") }
    }
}

@Composable
private fun RulesCard(device: DeviceUi, online: Boolean, onCommand: (String) -> Unit) {
    val allAuto = device.zones.isNotEmpty() && device.zones.all { it.auto }
    var winOn by remember(device.winOn) { mutableStateOf(device.winOn) }
    var sh by remember(device.winSh) { mutableStateOf(device.winSh.coerceIn(0, 23)) }
    var sm by remember(device.winSm) { mutableStateOf(device.winSm.coerceIn(0, 59)) }
    var eh by remember(device.winEh) { mutableStateOf(device.winEh.coerceIn(0, 23)) }
    var em by remember(device.winEm) { mutableStateOf(device.winEm.coerceIn(0, 59)) }
    val limit = nearestLimit(device.dailyLimit)
    var limitIndex by remember(limit) { mutableStateOf(LimitStops.indexOf(limit).coerceAtLeast(0)) }
    Card(colors = CardDefaults.cardColors(containerColor = CardBg), modifier = Modifier.fillMaxWidth()) {
        Column(Modifier.padding(14.dp), verticalArrangement = Arrangement.spacedBy(8.dp)) {
            Text("浇水规则", color = Color.White, style = MaterialTheme.typography.titleMedium)
            Row(
                Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically,
            ) {
                Text("全部自动", color = Color.White)
                Switch(
                    checked = allAuto,
                    enabled = online,
                    onCheckedChange = { onCommand("auto all ${if (it) 1 else 0}") },
                    colors = switchColors(),
                )
            }
            Row(
                Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically,
            ) {
                Column(Modifier.weight(1f)) {
                    Text("自动时段", color = Color.White)
                    Text("起止相同表示全天。窗口外不自动浇。", color = Muted)
                }
                Switch(checked = winOn, enabled = online, onCheckedChange = { winOn = it }, colors = switchColors())
            }
            if (winOn) {
                TimeRow("开始", sh, sm, online, { sh = it }, { sm = it })
                TimeRow("结束", eh, em, online, { eh = it }, { em = it })
            }
            DarkButton("保存时段", online) { onCommand("win ${if (winOn) 1 else 0} $sh $sm $eh $em") }
            Text("日限额 ${LimitStops[limitIndex]} ml", color = Color.White)
            Slider(
                value = limitIndex.toFloat(),
                enabled = online,
                onValueChange = { limitIndex = it.roundToInt().coerceIn(0, LimitStops.lastIndex) },
                valueRange = 0f..LimitStops.lastIndex.toFloat(),
                steps = LimitStops.size - 2,
                colors = SliderDefaults.colors(
                    thumbColor = Accent,
                    activeTrackColor = Accent,
                    inactiveTrackColor = Color(0xFF2C3544),
                    disabledThumbColor = Muted,
                    disabledActiveTrackColor = Color(0xFF2C3544),
                ),
            )
            DarkButton("保存限额", online) { onCommand("limit ${LimitStops[limitIndex]}") }
        }
    }
}

@Composable
private fun ZoneCountCard(current: Int, online: Boolean, onCommand: (String) -> Unit) {
    var draft by remember(current) { mutableStateOf(current.coerceIn(1, 10)) }
    var pending by remember { mutableStateOf<Int?>(null) }
    val changed = draft != current
    Card(colors = CardDefaults.cardColors(containerColor = CardBg), modifier = Modifier.fillMaxWidth()) {
        Column(Modifier.padding(14.dp), verticalArrangement = Arrangement.spacedBy(8.dp)) {
            Text("盆数", color = Color.White, style = MaterialTheme.typography.titleMedium)
            Text("当前 $current 盆。这是少用的设置，改完要再确认一次才会写入灌溉器。", color = Muted)
            Row(
                Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically,
            ) {
                Text(if (changed) "改为 $draft 盆" else "保持 $current 盆", color = Color.White)
                Stepper(draft, 1, 10, 1, online) { draft = it }
            }
            DarkButton("应用盆数", enabled = online && changed) { pending = draft }
        }
    }
    val next = pending
    if (next != null) {
        val adding = next > current
        val pots = potSpan(minOf(current, next) + 1, maxOf(current, next))
        AlertDialog(
            onDismissRequest = { pending = null },
            properties = DialogProperties(dismissOnClickOutside = false),
            containerColor = CardBg,
            titleContentColor = Color.White,
            textContentColor = Color.White,
            title = { Text(if (adding) "确认加到 $next 盆" else "确认减到 $next 盆") },
            text = {
                Text(
                    if (adding) {
                        "将启用 $pots。请先把湿度和阀门接到对应针脚。保存后可以手动浇水；自动浇水要先在调试页做干、湿标定。"
                    } else {
                        "将停用 $pots。这些盆不再采样，也不会再浇水。标定仍留在灌溉器里，以后加回来还能用。"
                    }
                )
            },
            confirmButton = {
                TextButton(onClick = {
                    pending = null
                    onCommand("zones $next")
                }) {
                    Text(if (adding) "确认增加" else "确认减少", color = if (adding) Accent else Warn)
                }
            },
            dismissButton = {
                TextButton(onClick = { pending = null }) { Text("取消", color = Muted) }
            },
        )
    }
}

private fun potSpan(from: Int, to: Int): String = when {
    from >= to -> "${to}#"
    from + 1 == to -> "${from}# 和 ${to}#"
    else -> "${from}# 到 ${to}#"
}

@Composable
private fun WifiCard(
    enabled: Boolean,
    ssid: String,
    ip: String,
    up: Boolean,
    online: Boolean,
    onCommand: (String) -> Unit,
) {
    var on by remember(enabled) { mutableStateOf(enabled) }
    var name by remember(ssid) { mutableStateOf(ssid) }
    var pass by remember { mutableStateOf("") }
    Card(colors = CardDefaults.cardColors(containerColor = CardBg), modifier = Modifier.fillMaxWidth()) {
        Column(Modifier.padding(14.dp), verticalArrangement = Arrangement.spacedBy(8.dp)) {
            Text("WiFi", color = Color.White, style = MaterialTheme.typography.titleMedium)
            Text(if (up && ip.isNotBlank()) "已连接 $ip" else "未连接路由器", color = Muted)
            Row(
                Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically,
            ) {
                Text("启用", color = Color.White)
                Switch(checked = on, enabled = online, onCheckedChange = { on = it }, colors = switchColors())
            }
            OutlinedTextField(
                value = name,
                onValueChange = { if (it.encodeToByteArray().size <= 32) name = it },
                enabled = online,
                label = { Text("名称") },
                singleLine = true,
                modifier = Modifier.fillMaxWidth(),
                colors = fieldColors(),
            )
            OutlinedTextField(
                value = pass,
                onValueChange = { if (it.length <= 64) pass = it },
                enabled = online,
                label = { Text("密码，留空则不改") },
                singleLine = true,
                modifier = Modifier.fillMaxWidth(),
                colors = fieldColors(),
            )
            DarkButton("保存网络", online) {
                if (name.isNotBlank()) onCommand("ssid $name")
                if (pass.isNotEmpty()) onCommand("pass $pass")
                onCommand("wifi ${if (on) 1 else 0}")
            }
        }
    }
}

@Composable
private fun FlowCard(device: DeviceUi, online: Boolean, onCommand: (String) -> Unit) {
    var ppl by remember(device.ppl) { mutableStateOf(device.ppl.toString()) }
    Card(colors = CardDefaults.cardColors(containerColor = CardBg), modifier = Modifier.fillMaxWidth()) {
        Column(Modifier.padding(14.dp), verticalArrangement = Arrangement.spacedBy(8.dp)) {
            Text("流量与排气", color = Color.White, style = MaterialTheme.typography.titleMedium)
            Text("${device.flowMl} ml · ${device.pulses} 脉冲", color = Muted)
            OutlinedTextField(
                value = ppl,
                onValueChange = { if (it.length <= 5 && it.all(Char::isDigit)) ppl = it },
                enabled = online,
                label = { Text("每升脉冲") },
                singleLine = true,
                modifier = Modifier.fillMaxWidth(),
                colors = fieldColors(),
            )
            Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                Button(
                    onClick = { onCommand("ppl ${ppl.toIntOrNull() ?: device.ppl}") },
                    enabled = online,
                    modifier = Modifier.weight(1f),
                ) {
                    Text("保存")
                }
                Button(
                    onClick = { onCommand("flow 0") },
                    enabled = online,
                    modifier = Modifier.weight(1f),
                    colors = ButtonDefaults.buttonColors(
                        containerColor = Color(0xFF21262D),
                        contentColor = Color.White,
                        disabledContainerColor = Color(0xFF2A3140),
                        disabledContentColor = Muted,
                    ),
                ) { Text("清零") }
            }
            Button(
                onClick = { onCommand(if (device.purge) "purge 0" else "purge 1") },
                enabled = online,
                modifier = Modifier.fillMaxWidth(),
                colors = ButtonDefaults.buttonColors(
                    containerColor = if (device.purge) Warn else Accent,
                    contentColor = if (device.purge) Color.White else Color(0xFF0F1419),
                    disabledContainerColor = Color(0xFF2A3140),
                    disabledContentColor = Muted,
                ),
            ) { Text(if (device.purge) "停止排气" else "排气") }
        }
    }
}

@Composable
private fun localName(zone: Int, tick: Int): String {
    val context = LocalContext.current
    return remember(zone, tick) { ZoneStore.name(context, zone) }
}

@Composable
private fun PotOverview(zones: List<ZoneUi>, watering: Int, mediaTick: Int) {
    Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
        zones.chunked(4).forEach { row ->
            Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                row.forEach { zone ->
                    PotTile(
                        zone,
                        watering == zone.index,
                        mediaTick,
                        Modifier.weight(1f),
                    )
                }
                repeat(4 - row.size) { Spacer(Modifier.weight(1f)) }
            }
        }
    }
}

@Composable
private fun PotPicture(photo: android.graphics.Bitmap?, description: String, modifier: Modifier) {
    if (photo != null) {
        Image(photo.asImageBitmap(), description, modifier, contentScale = ContentScale.Crop)
    } else {
        Image(
            painterResource(R.drawable.ic_launcher_foreground),
            description,
            modifier,
            contentScale = ContentScale.Crop,
        )
    }
}

@Composable
private fun PotTile(zone: ZoneUi, watering: Boolean, mediaTick: Int, modifier: Modifier) {
    val context = LocalContext.current
    val alias = localName(zone.index, mediaTick)
    val photo = remember(zone.index, mediaTick) { ZoneStore.bitmap(context, zone.index) }
    val tone = zoneTone(zone)
    Box(
        modifier
            .aspectRatio(1f)
            .clip(RoundedCornerShape(16.dp))
            .background(Color(0xFF143028)),
    ) {
        PotPicture(photo, alias.ifBlank { "${zone.index + 1}#" }, Modifier.fillMaxSize())
        if (watering) WateringOverlay()
        Box(
            Modifier
                .align(Alignment.BottomCenter)
                .fillMaxWidth()
                .background(Brush.verticalGradient(listOf(Color.Transparent, Color(0xE60F1419))))
                .padding(horizontal = 6.dp, vertical = 6.dp),
        ) {
            Column {
                if (alias.isNotBlank()) {
                    Text(alias, color = Color.White, maxLines = 1, fontSize = 11.sp, fontWeight = FontWeight.Medium)
                }
                Text(
                    if (zone.valid) "${zone.pct}%" else "--",
                    color = if (watering) Color(0xFF7AD7FF) else tone,
                    fontSize = 16.sp,
                    fontWeight = FontWeight.Bold,
                )
            }
        }
    }
}

@Composable
private fun WateringOverlay() {
    val shift = rememberInfiniteTransition(label = "water")
    val fall by shift.animateFloat(
        initialValue = 0f,
        targetValue = 1f,
        animationSpec = infiniteRepeatable(tween(1100, easing = LinearEasing)),
        label = "fall",
    )
    Canvas(Modifier.fillMaxSize()) {
        drawRect(Color(0x3327B4FF), size = Size(size.width, size.height))
        repeat(3) { i ->
            val y = ((fall + i * 0.33f) % 1f) * size.height
            val x = size.width * (0.22f + i * 0.28f)
            drawCircle(Color(0xCCB7E8FF), radius = size.minDimension * 0.07f, center = Offset(x, y))
        }
    }
}

@Composable
private fun HistoryCard(history: List<HistUi>, mediaTick: Int) {
    Card(colors = CardDefaults.cardColors(containerColor = CardBg), modifier = Modifier.fillMaxWidth()) {
        Column(Modifier.padding(14.dp), verticalArrangement = Arrangement.spacedBy(6.dp)) {
            Text("最近浇水", color = Color.White, style = MaterialTheme.typography.titleMedium)
            Text("手机保存全部 ${history.size} 条，下面是最近 8 条。", color = Muted)
            if (history.isEmpty()) {
                Text("还没有记录", color = Muted)
            } else {
                history.take(8).forEach { row ->
                    val name = localName(row.zone, mediaTick).ifBlank { "${row.zone + 1}#" }
                    val outcome = Protocol.outcomeText(row.outcome)
                    val detail = if (outcome.isEmpty()) {
                        Protocol.triggerText(row.trigger)
                    } else {
                        "${Protocol.triggerText(row.trigger)} · $outcome"
                    }
                    Text(
                        "${histClock(row.ts)}  $name  ${row.ml} ml  $detail",
                        color = if (row.outcome == 0) Color.White else Warn,
                    )
                }
            }
        }
    }
}

private fun histClock(ts: Long): String {
    if (ts <= 0L) return "--:--"
    val cal = java.util.Calendar.getInstance()
    cal.timeInMillis = ts * 1000L
    return "%02d-%02d %02d:%02d".format(
        cal.get(java.util.Calendar.MONTH) + 1,
        cal.get(java.util.Calendar.DAY_OF_MONTH),
        cal.get(java.util.Calendar.HOUR_OF_DAY),
        cal.get(java.util.Calendar.MINUTE),
    )
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

private data class Notice(val id: Int, val text: String)

@Composable
private fun HintCard(title: String, body: String) {
    Card(colors = CardDefaults.cardColors(containerColor = CardBg), modifier = Modifier.fillMaxWidth()) {
        Column(Modifier.padding(14.dp), verticalArrangement = Arrangement.spacedBy(6.dp)) {
            Text(title, color = Color.White, style = MaterialTheme.typography.titleMedium)
            Text(body, color = Muted)
        }
    }
}

@Composable
private fun fieldColors() = OutlinedTextFieldDefaults.colors(
    focusedBorderColor = Accent,
    unfocusedBorderColor = Color(0xFF2C3544),
    focusedLabelColor = Accent,
    unfocusedLabelColor = Muted,
    cursorColor = Accent,
    focusedTextColor = Color.White,
    unfocusedTextColor = Color.White,
)

private fun linkColor(link: String): Color = when {
    link.contains("失败") || link.contains("没有") || link.contains("请") || link.contains("断开") -> Warn
    link.startsWith("正在") -> Accent
    else -> Muted
}

private fun friendlyReply(raw: String): String? {
    val text = raw.trim()
    if (text.isEmpty() || text.equals("OK", ignoreCase = true)) return null
    val queued = Regex("^OK (\\d+)$").matchEntire(text)
    if (queued != null) return "已排队 ${queued.groupValues[1]} 盆"
    return when {
        text == "OK none" -> "没有偏干的盆"
        text.contains("busy") -> "正在浇水，请稍后再试"
        text.contains("selfcheck") -> "自检未通过，暂时不能操作"
        text.contains("lock") -> "故障锁定，请先恢复运行"
        text.contains("cal") -> "请先标定"
        text.contains("time") -> "对时失败"
        text.contains("valve") -> "阀门操作失败"
        text.contains("zone") -> "数值不对，请检查盆号和上下限"
        text.contains("cmd") -> "无法识别这次操作"
        text.startsWith("ERR") -> "操作失败"
        else -> text
    }
}
