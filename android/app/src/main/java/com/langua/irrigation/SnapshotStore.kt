package com.langua.irrigation

import android.content.Context
import org.json.JSONArray
import org.json.JSONObject
import java.io.File

object SnapshotStore {
    private const val FILE = "last_snapshot.json"

    data class Saved(
        val device: DeviceUi,
        val history: List<HistUi>,
        val wifiOn: Boolean,
        val wifiSsid: String,
        val wifiIp: String,
    )

    fun load(context: Context): Saved? {
        val file = File(context.filesDir, FILE)
        if (!file.exists()) return null
        return try {
            val root = JSONObject(file.readText())
            Saved(
                device = readDevice(root.getJSONObject("device")),
                history = readHistory(root.optJSONArray("history")),
                wifiOn = root.optBoolean("wifiOn", true),
                wifiSsid = root.optString("wifiSsid"),
                wifiIp = root.optString("wifiIp"),
            )
        } catch (_: Exception) {
            null
        }
    }

    fun save(
        context: Context,
        device: DeviceUi,
        history: List<HistUi>,
        wifiOn: Boolean,
        wifiSsid: String,
        wifiIp: String,
    ) {
        val root = JSONObject()
        root.put("device", writeDevice(device))
        root.put("history", writeHistory(history))
        root.put("wifiOn", wifiOn)
        root.put("wifiSsid", wifiSsid)
        root.put("wifiIp", wifiIp)
        File(context.filesDir, FILE).writeText(root.toString())
    }

    private fun writeDevice(device: DeviceUi) = JSONObject().apply {
        put("state", device.state)
        put("safety", device.safety)
        put("pump", device.pump)
        put("valveOn", device.valveOn)
        put("activeValve", device.activeValve)
        put("queue", device.queue)
        put("locked", device.locked)
        put("purge", device.purge)
        put("wifiUp", device.wifiUp)
        put("dailyMl", device.dailyMl)
        put("sessionMl", device.sessionMl)
        put("dailyLimit", device.dailyLimit)
        put("winOn", device.winOn)
        put("winSh", device.winSh)
        put("winSm", device.winSm)
        put("winEh", device.winEh)
        put("winEm", device.winEm)
        put("ppl", device.ppl)
        put("flowMl", device.flowMl)
        put("pulses", device.pulses)
        put("zones", JSONArray().apply {
            device.zones.forEach { zone ->
                put(JSONObject().apply {
                    put("index", zone.index)
                    put("name", zone.name)
                    put("pct", zone.pct)
                    put("low", zone.low)
                    put("high", zone.high)
                    put("auto", zone.auto)
                    put("valid", zone.valid)
                    put("schedule", zone.schedule)
                    put("hour", zone.hour)
                    put("minute", zone.minute)
                    put("volume", zone.volume)
                    put("adc", zone.adc)
                })
            }
        })
    }

    private fun readDevice(obj: JSONObject): DeviceUi {
        val zonesJson = obj.getJSONArray("zones")
        val zones = ArrayList<ZoneUi>(zonesJson.length())
        for (i in 0 until zonesJson.length()) {
            val zone = zonesJson.getJSONObject(i)
            zones.add(
                ZoneUi(
                    index = zone.getInt("index"),
                    name = zone.optString("name"),
                    pct = zone.getInt("pct"),
                    low = zone.getInt("low"),
                    high = zone.getInt("high"),
                    auto = zone.getBoolean("auto"),
                    valid = zone.getBoolean("valid"),
                    schedule = zone.getBoolean("schedule"),
                    hour = zone.getInt("hour"),
                    minute = zone.getInt("minute"),
                    volume = zone.getInt("volume"),
                    adc = zone.getInt("adc"),
                )
            )
        }
        return DeviceUi(
            state = obj.getInt("state"),
            safety = obj.getInt("safety"),
            pump = obj.getBoolean("pump"),
            valveOn = obj.getBoolean("valveOn"),
            activeValve = obj.getInt("activeValve"),
            queue = obj.getInt("queue"),
            locked = obj.getBoolean("locked"),
            purge = obj.getBoolean("purge"),
            wifiUp = obj.getBoolean("wifiUp"),
            dailyMl = obj.getInt("dailyMl"),
            sessionMl = obj.getInt("sessionMl"),
            dailyLimit = obj.getLong("dailyLimit"),
            winOn = obj.getBoolean("winOn"),
            winSh = obj.getInt("winSh"),
            winSm = obj.getInt("winSm"),
            winEh = obj.getInt("winEh"),
            winEm = obj.getInt("winEm"),
            ppl = obj.getInt("ppl"),
            flowMl = obj.getInt("flowMl"),
            pulses = obj.getLong("pulses"),
            zones = zones,
        )
    }

    private fun writeHistory(history: List<HistUi>) = JSONArray().apply {
        history.forEach { row ->
            put(JSONObject().apply {
                put("ts", row.ts)
                put("zone", row.zone)
                put("ml", row.ml)
                put("trigger", row.trigger)
            })
        }
    }

    private fun readHistory(array: JSONArray?): List<HistUi> {
        if (array == null) return emptyList()
        val rows = ArrayList<HistUi>(array.length())
        for (i in 0 until array.length()) {
            val row = array.getJSONObject(i)
            rows.add(
                HistUi(
                    ts = row.getLong("ts"),
                    zone = row.getInt("zone"),
                    ml = row.getInt("ml"),
                    trigger = row.getInt("trigger"),
                )
            )
        }
        return rows
    }
}
