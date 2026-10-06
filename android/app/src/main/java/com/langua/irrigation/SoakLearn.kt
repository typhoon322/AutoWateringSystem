package com.langua.irrigation

import android.content.Context
import java.io.File
import kotlin.math.abs
import kotlin.math.ceil

/**
 * 浇完后继续看湿度。连续 3 分钟变化不超过 5 个百分点视为稳住。
 * 每盆保留最近 12 次，取偏长的一档（75%）写回灌溉器。
 */
object SoakLearn {
    private const val FILE = "soak_obs.tsv"
    private const val BAND = 5
    private const val WINDOW_MS = 3 * 60 * 1000L
    private const val SAMPLE_MS = 20_000L
    private const val MAX_WATCH_MS = 60 * 60 * 1000L
    private const val KEEP = 12

    data class Step(val logged: Boolean, val command: String?)

    private class Watch(val zone: Int, val t0: Long, val startPct: Int) {
        val samples = ArrayList<Pair<Long, Int>>()
        var lastAt = 0L
    }

    private val lock = Any()
    private val watches = HashMap<Int, Watch>()
    private val pending = LinkedHashMap<Int, Int>()
    private val paused = HashSet<Int>()
    private val lastMin = HashMap<Int, Int>()
    private val agreed = HashSet<Int>()
    private var stateReady = false
    private var wateringZone = -1
    private var lastSent = ""
    private var lastSentAt = 0L

    fun onDisconnect() {
        synchronized(lock) {
            watches.clear()
            wateringZone = -1
        }
    }

    fun counts(context: Context): Map<Int, Int> =
        load(context).groupingBy { it.zone }.eachCount()

    fun paused(context: Context): Set<Int> {
        synchronized(lock) {
            ensureState(context)
            return paused.toSet()
        }
    }

    fun restart(context: Context, zone: Int) {
        synchronized(lock) {
            ensureState(context)
            watches.remove(zone)
            pending.remove(zone)
            paused.remove(zone)
            lastMin.remove(zone)
            agreed.remove(zone)
            if (wateringZone == zone) wateringZone = -1
            write(context, load(context).filter { it.zone != zone })
            saveState(context)
            AppLog.op(context, "重新计算渗水 ${zone + 1}#")
        }
    }

    fun onStatus(context: Context, device: DeviceUi): Step {
        synchronized(lock) {
            ensureState(context)
            val watering = (device.state == 2 || device.state == 3) && device.activeValve >= 0
            if (watering) {
                wateringZone = device.activeValve
            } else if (wateringZone >= 0) {
                val zone = wateringZone
                wateringZone = -1
                val pct = device.zones.getOrNull(zone)?.takeIf { it.valid }?.pct
                if (pct != null && watches[zone] == null && !paused.contains(zone)) {
                    val watch = Watch(zone, System.currentTimeMillis(), pct)
                    watch.samples.add(0L to pct)
                    watch.lastAt = watch.t0
                    watches[zone] = watch
                }
            }

            var logged = false
            val now = System.currentTimeMillis()
            val finished = ArrayList<Int>()
            for ((zone, watch) in watches) {
                if (paused.contains(zone)) {
                    finished.add(zone)
                    continue
                }
                val ui = device.zones.getOrNull(zone) ?: continue
                if (!ui.valid) continue
                val elapsed = now - watch.t0
                if (now - watch.lastAt >= SAMPLE_MS) {
                    watch.samples.add(elapsed to ui.pct)
                    watch.lastAt = now
                }
                val stable = stable(watch.samples)
                val timeout = elapsed >= MAX_WATCH_MS
                if (!stable && !timeout) continue
                val settleSec = (elapsed / 1000L).toInt().coerceIn(1, 3600)
                val endPct = watch.samples.lastOrNull()?.second ?: ui.pct
                append(context, zone, settleSec, watch.startPct, endPct)
                logged = true
                finished.add(zone)
                val secs = load(context).filter { it.zone == zone }.map { it.settleSec }
                val minutes = recommend(secs)
                val previous = lastMin[zone]
                if (minutes != null && previous != null && secs.size >= 3 && abs(minutes - previous) < 2) {
                    agreed.add(zone)
                } else {
                    agreed.remove(zone)
                }
                if (minutes != null) lastMin[zone] = minutes
                if (minutes != null && abs(minutes - ui.soakMin) >= 2 && !paused.contains(zone)) {
                    pending[zone] = minutes
                }
                saveState(context)
            }
            finished.forEach { watches.remove(it) }
            if (markSettled(context, device)) logged = true
            for (zone in pending.keys.toList()) {
                val current = device.zones.getOrNull(zone)?.soakMin ?: continue
                val want = pending[zone] ?: continue
                if (abs(want - current) < 2) pending.remove(zone)
            }
            var command: String? = null
            val next = pending.entries.firstOrNull()
            if (next != null) {
                val line = "soak ${next.key} ${next.value}"
                if (line != lastSent || now - lastSentAt > 30_000L) {
                    command = line
                    lastSent = line
                    lastSentAt = now
                }
            }
            return Step(logged, command)
        }
    }

    private fun stable(samples: List<Pair<Long, Int>>): Boolean {
        if (samples.size < 4) return false
        val end = samples.last().first
        if (end < WINDOW_MS) return false
        val recent = samples.filter { it.first >= end - WINDOW_MS }
        if (recent.size < 4) return false
        if (recent.last().first - recent.first().first < WINDOW_MS - SAMPLE_MS) return false
        return recent.maxOf { it.second } - recent.minOf { it.second } <= BAND
    }

    private fun markSettled(context: Context, device: DeviceUi): Boolean {
        var changed = false
        val rows = load(context)
        for (ui in device.zones) {
            if (!agreed.contains(ui.index) || paused.contains(ui.index)) continue
            val minutes = recommend(rows.filter { it.zone == ui.index }.map { it.settleSec }) ?: continue
            if (abs(minutes - ui.soakMin) >= 2) continue
            paused.add(ui.index)
            agreed.remove(ui.index)
            pending.remove(ui.index)
            watches.remove(ui.index)
            changed = true
            AppLog.op(context, "渗水计算暂停 ${ui.index + 1}# $minutes 分钟")
        }
        if (changed) saveState(context)
        return changed
    }

    private fun ensureState(context: Context) {
        if (stateReady) return
        val prefs = context.getSharedPreferences("soak_learn", Context.MODE_PRIVATE)
        paused.clear()
        paused.addAll(prefs.getStringSet("paused", emptySet()).orEmpty().mapNotNull { it.toIntOrNull() })
        agreed.clear()
        agreed.addAll(prefs.getStringSet("agreed", emptySet()).orEmpty().mapNotNull { it.toIntOrNull() })
        lastMin.clear()
        prefs.getString("last", null)?.split(',')?.forEach { part ->
            val kv = part.split('=')
            if (kv.size != 2) return@forEach
            val zone = kv[0].toIntOrNull() ?: return@forEach
            val minutes = kv[1].toIntOrNull() ?: return@forEach
            lastMin[zone] = minutes
        }
        stateReady = true
    }

    private fun saveState(context: Context) {
        context.getSharedPreferences("soak_learn", Context.MODE_PRIVATE).edit()
            .putStringSet("paused", paused.map { it.toString() }.toSet())
            .putStringSet("agreed", agreed.map { it.toString() }.toSet())
            .putString("last", lastMin.entries.joinToString(",") { "${it.key}=${it.value}" })
            .apply()
    }

    private fun recommend(settleSec: List<Int>): Int? {
        if (settleSec.size < 2) return null
        val sorted = settleSec.sorted()
        val index = ceil(sorted.size * 0.75).toInt() - 1
        val sec = sorted[index.coerceIn(0, sorted.lastIndex)]
        return ((sec + 59) / 60).coerceIn(5, 60)
    }

    private data class Obs(val zone: Int, val settleSec: Int)

    private fun append(context: Context, zone: Int, settleSec: Int, startPct: Int, endPct: Int) {
        val rows = load(context).toMutableList()
        rows.add(Obs(zone, settleSec))
        write(context, rows.groupBy { it.zone }.values.flatMap { it.takeLast(KEEP) })
        AppLog.op(context, "渗水记录 ${zone + 1}# ${settleSec / 60} 分钟 $startPct%→$endPct%")
    }

    private fun write(context: Context, rows: List<Obs>) {
        file(context).writeText(
            rows.joinToString("\n") { "${System.currentTimeMillis() / 1000}\t${it.zone}\t${it.settleSec}" }
        )
    }

    private fun load(context: Context): List<Obs> {
        val f = file(context)
        if (!f.exists()) return emptyList()
        return f.readLines().mapNotNull { line ->
            val p = line.split('\t')
            if (p.size < 3) return@mapNotNull null
            val zone = p[1].toIntOrNull() ?: return@mapNotNull null
            val sec = p[2].toIntOrNull() ?: return@mapNotNull null
            Obs(zone, sec)
        }
    }

    private fun file(context: Context) = File(context.filesDir, FILE)
}
