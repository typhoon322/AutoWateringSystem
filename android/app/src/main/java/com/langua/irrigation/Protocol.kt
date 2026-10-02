package com.langua.irrigation

data class ZoneUi(
    val index: Int,
    val name: String,
    val pct: Int,
    val low: Int,
    val high: Int,
    val auto: Boolean,
    val valid: Boolean,
    val volume: Int,
)

data class DeviceUi(
    val state: Int,
    val safety: Int,
    val pump: Boolean,
    val valveOn: Boolean,
    val activeValve: Int,
    val queue: Int,
    val locked: Boolean,
    val dailyMl: Int,
    val zones: List<ZoneUi>,
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

    fun parse(payload: ByteArray): DeviceUi? {
        if (payload.size < 12 || payload[0].toInt() != 1) return null
        val zoneCount = payload[6].toInt() and 0xFF
        if (payload.size < 12 + zoneCount * 16) return null
        val flags = payload[3].toInt() and 0xFF
        val active = payload[4].toInt() and 0xFF
        val zones = ArrayList<ZoneUi>(zoneCount)
        for (i in 0 until zoneCount) {
            val o = 12 + i * 16
            val zf = payload[o + 3].toInt() and 0xFF
            val nameBytes = payload.copyOfRange(o + 8, o + 16)
            val end = nameBytes.indexOf(0).let { if (it < 0) nameBytes.size else it }
            val name = nameBytes.copyOf(end).toString(Charsets.UTF_8).ifBlank { "盆$i" }
            zones.add(
                ZoneUi(
                    index = i,
                    name = name,
                    pct = payload[o].toInt() and 0xFF,
                    low = payload[o + 1].toInt() and 0xFF,
                    high = payload[o + 2].toInt() and 0xFF,
                    auto = zf and 0x01 != 0,
                    valid = zf and 0x02 != 0,
                    volume = u16(payload, o + 4),
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
            dailyMl = u16(payload, 8),
            zones = zones,
        )
    }

    private fun u16(b: ByteArray, i: Int): Int =
        (b[i].toInt() and 0xFF) or ((b[i + 1].toInt() and 0xFF) shl 8)
}
