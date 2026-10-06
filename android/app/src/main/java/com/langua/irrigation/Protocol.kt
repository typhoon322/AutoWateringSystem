package com.langua.irrigation

data class ZoneUi(
    val index: Int,
    val name: String,
    val pct: Int,
    val low: Int,
    val high: Int,
    val auto: Boolean,
    val valid: Boolean,
    val schedule: Boolean,
    val hour: Int,
    val minute: Int,
    val volume: Int,
    val adc: Int,
    val soaking: Boolean = false,
    val soakMin: Int = 10,
    val soakCount: Int = 0,
    val soakPaused: Boolean = false,
    val soakWatching: Boolean = false,
)

data class DeviceUi(
    val state: Int,
    val safety: Int,
    val pump: Boolean,
    val valveOn: Boolean,
    val activeValve: Int,
    val queue: Int,
    val locked: Boolean,
    val purge: Boolean,
    val wifiUp: Boolean,
    val dailyMl: Int,
    val sessionMl: Int,
    val dailyLimit: Long,
    val winOn: Boolean,
    val winSh: Int,
    val winSm: Int,
    val winEh: Int,
    val winEm: Int,
    val ppl: Int,
    val flowMl: Int,
    val pulses: Long,
    val zones: List<ZoneUi>,
    val clockReady: Boolean = false,
)

data class HistUi(
    val ts: Long,
    val zone: Int,
    val ml: Int,
    val trigger: Int,
    val outcome: Int = 0,
    val seq: Long = 0,
)

object Protocol {
    const val SERVICE = "8a1f0001-4b2c-4d5e-9f10-112233445501"
    const val STATUS = "8a1f0002-4b2c-4d5e-9f10-112233445502"
    const val CMD = "8a1f0003-4b2c-4d5e-9f10-112233445503"
    const val REPLY = "8a1f0004-4b2c-4d5e-9f10-112233445504"
    const val DEVICE_NAME = "Langua"

    fun stateText(state: Int): String = when (state) {
        0 -> "空闲"
        1 -> "检测"
        2 -> "开阀"
        3 -> "浇水"
        4 -> "完成"
        5 -> "故障"
        else -> "未知"
    }

    fun safetyText(safety: Int): String = when (safety) {
        0 -> "正常"
        1 -> "超时"
        2 -> "干转"
        3 -> "日限额"
        4 -> "锁定"
        else -> "未知"
    }

    fun triggerText(trigger: Int): String = when (trigger) {
        1 -> "自动"
        2 -> "定时"
        3 -> "手动"
        4 -> "测试"
        else -> "浇水"
    }

    fun outcomeText(outcome: Int): String = when (outcome) {
        1 -> "干转"
        2 -> "超时"
        3 -> "急停"
        4 -> "故障"
        else -> ""
    }

    fun parse(payload: ByteArray): DeviceUi? {
        if (payload.size < 28 || payload[0].toInt() != 2) return null
        val zoneCount = payload[6].toInt() and 0xFF
        if (payload.size < 28 + zoneCount * 24) return null
        val flags = payload[3].toInt() and 0xFF
        val active = payload[4].toInt() and 0xFF
        val zones = ArrayList<ZoneUi>(zoneCount)
        val soakAt = 28 + zoneCount * 24
        val metaAt = soakAt + zoneCount
        val hasSoak = payload.size >= soakAt + zoneCount
        val hasMeta = payload.size >= metaAt + zoneCount
        for (i in 0 until zoneCount) {
            val o = 28 + i * 24
            val zf = payload[o + 3].toInt() and 0xFF
            val soakRaw = if (hasSoak) payload[soakAt + i].toInt() and 0xFF else 10
            val meta = if (hasMeta) payload[metaAt + i].toInt() and 0xFF else 0
            val nameBytes = payload.copyOfRange(o + 10, o + 24)
            val end = nameBytes.indexOf(0).let { if (it < 0) nameBytes.size else it }
            val name = nameBytes.copyOf(end).toString(Charsets.UTF_8).ifBlank { "${i + 1}# 盆" }
            zones.add(
                ZoneUi(
                    index = i,
                    name = name,
                    pct = payload[o].toInt() and 0xFF,
                    low = payload[o + 1].toInt() and 0xFF,
                    high = payload[o + 2].toInt() and 0xFF,
                    auto = zf and 0x01 != 0,
                    valid = zf and 0x02 != 0,
                    schedule = zf and 0x04 != 0,
                    hour = payload[o + 8].toInt() and 0xFF,
                    minute = payload[o + 9].toInt() and 0xFF,
                    volume = u16(payload, o + 4),
                    adc = u16(payload, o + 6),
                    soaking = zf and 0x08 != 0,
                    soakMin = if (soakRaw in 5..60) soakRaw else 10,
                    soakCount = meta and 0x1F,
                    soakPaused = meta and 0x80 != 0,
                    soakWatching = meta and 0x40 != 0,
                )
            )
        }
        return DeviceUi(
            state = payload[1].toInt() and 0xFF,
            safety = payload[2].toInt() and 0xFF,
            pump = flags and 0x01 != 0,
            valveOn = flags and 0x02 != 0,
            activeValve = if (active == 255) -1 else active,
            queue = payload[5].toInt() and 0xFF,
            locked = flags and 0x04 != 0,
            purge = flags and 0x08 != 0,
            wifiUp = flags and 0x10 != 0,
            dailyMl = u16(payload, 8),
            sessionMl = u16(payload, 10),
            dailyLimit = u32(payload, 12),
            winOn = payload[7].toInt() != 0,
            winSh = payload[16].toInt() and 0xFF,
            winSm = payload[17].toInt() and 0xFF,
            winEh = payload[18].toInt() and 0xFF,
            winEm = payload[19].toInt() and 0xFF,
            ppl = u16(payload, 20),
            flowMl = u16(payload, 22),
            pulses = u32(payload, 24),
            zones = zones,
            clockReady = flags and 0x20 != 0,
        )
    }

    private fun u16(b: ByteArray, i: Int): Int =
        (b[i].toInt() and 0xFF) or ((b[i + 1].toInt() and 0xFF) shl 8)

    private fun u32(b: ByteArray, i: Int): Long =
        (b[i].toLong() and 0xFF) or
            ((b[i + 1].toLong() and 0xFF) shl 8) or
            ((b[i + 2].toLong() and 0xFF) shl 16) or
            ((b[i + 3].toLong() and 0xFF) shl 24)
}
