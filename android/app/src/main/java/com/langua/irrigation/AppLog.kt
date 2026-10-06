package com.langua.irrigation

import android.app.Activity
import android.content.Context
import android.content.Intent
import androidx.core.content.FileProvider
import java.io.File
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale
import java.util.concurrent.TimeUnit

object AppLog {
    private const val DIR = "logs"
    private const val KEEP_DAYS = 30L
    private val lock = Any()
    private val dayFmt = SimpleDateFormat("yyyy-MM-dd", Locale.US)
    private val timeFmt = SimpleDateFormat("yyyy-MM-dd HH:mm:ss.SSS", Locale.US)
    private var prunedDay = ""
    private var lastStat = ""
    private var lastStatAt = 0L

    fun op(context: Context, message: String) = write(context, "APP", message)

    fun ble(context: Context, message: String) = write(context, "BLE", redact(message))

    fun sent(context: Context, line: String) = write(context, "BLE>", redact(line))

    fun received(context: Context, line: String) = write(context, "BLE<", redact(line))

    fun status(context: Context, device: DeviceUi) {
        val text = statusText(device)
        val now = System.currentTimeMillis()
        if (text == lastStat && now - lastStatAt < 60_000L) return
        lastStat = text
        lastStatAt = now
        write(context, "STAT", text)
    }

    fun share(activity: Activity) {
        val export = export(activity)
        val uri = FileProvider.getUriForFile(activity, "com.langua.irrigation.files", export)
        val send = Intent(Intent.ACTION_SEND).apply {
            type = "text/plain"
            putExtra(Intent.EXTRA_STREAM, uri)
            putExtra(Intent.EXTRA_SUBJECT, "蓝瓜智控日志")
            addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
        }
        activity.startActivity(Intent.createChooser(send, "发送日志"))
    }

    private fun export(context: Context): File {
        val dir = File(context.filesDir, DIR).apply { mkdirs() }
        val out = File(context.cacheDir, "langua-log.txt")
        synchronized(lock) {
            prune(dir, System.currentTimeMillis())
            out.bufferedWriter().use { writer ->
                dir.listFiles { file -> file.name.startsWith("langua-") && file.name.endsWith(".log") }
                    ?.sortedBy { it.name }
                    ?.forEach { file ->
                        writer.append(file.readText())
                    }
            }
        }
        return out
    }

    private fun write(context: Context, tag: String, message: String) {
        val now = System.currentTimeMillis()
        val line = "${timeFmt.format(Date(now))}  $tag  ${redact(message).replace('\n', ' ')}\n"
        synchronized(lock) {
            val dir = File(context.filesDir, DIR).apply { mkdirs() }
            val day = dayFmt.format(Date(now))
            if (prunedDay != day) {
                prune(dir, now)
                prunedDay = day
            }
            File(dir, "langua-$day.log").appendText(line)
        }
    }

    private fun prune(dir: File, now: Long) {
        val cutoff = now - TimeUnit.DAYS.toMillis(KEEP_DAYS)
        dir.listFiles()?.forEach { file ->
            val day = file.name.removePrefix("langua-").removeSuffix(".log")
            val parsed = runCatching { dayFmt.parse(day)?.time }.getOrNull() ?: return@forEach
            if (parsed < cutoff) file.delete()
        }
    }

    private fun redact(line: String): String = line.replace(Regex("(?<=\\bpass )\\S+"), "***")

    private fun statusText(device: DeviceUi): String {
        val valve = if (device.valveOn && device.activeValve >= 0) "${device.activeValve + 1}#" else "关"
        val zones = device.zones.joinToString(" ") { zone ->
            val moist = if (zone.valid) "${zone.pct}%" else "--"
            "${zone.index + 1}#$moist"
        }
        return "${Protocol.stateText(device.state)} ${Protocol.safetyText(device.safety)} " +
            "泵${if (device.pump) "开" else "关"} 阀$valve 队列${device.queue} " +
            "今日${device.dailyMl} 本次${device.sessionMl} $zones"
    }
}
