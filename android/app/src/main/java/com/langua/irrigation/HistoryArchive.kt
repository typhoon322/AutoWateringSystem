package com.langua.irrigation

import android.content.Context
import java.io.File

// 手机上的全部浇水记录。灌溉器只留最近 50 条，连上后按序号并进来。
object HistoryArchive {
    private const val FILE = "watering_history.tsv"
    private val rows = mutableListOf<HistUi>()
    private val seqs = HashSet<Long>()
    private val legacy = HashSet<String>()
    private var loaded = false

    fun load(context: Context): List<HistUi> {
        ensure(context)
        return sorted()
    }

    fun clear(context: Context) {
        rows.clear()
        seqs.clear()
        legacy.clear()
        loaded = true
        File(context.filesDir, FILE).delete()
    }

    fun merge(context: Context, incoming: List<HistUi>): List<HistUi> {
        ensure(context)
        val fresh = incoming.filter { remember(it) }
        if (fresh.isNotEmpty()) {
            File(context.filesDir, FILE).appendText(fresh.joinToString("") { line(it) })
        }
        return sorted()
    }

    private fun ensure(context: Context) {
        if (loaded) return
        val file = File(context.filesDir, FILE)
        if (file.exists()) {
            file.readLines().forEach { text -> parse(text)?.let { remember(it) } }
        }
        loaded = true
    }

    private fun remember(row: HistUi): Boolean {
        if (row.seq > 0) {
            if (!seqs.add(row.seq)) return false
            val key = legacyKey(row)
            if (legacy.remove(key)) {
                rows.removeAll { it.seq == 0L && legacyKey(it) == key }
            }
        } else if (!legacy.add(legacyKey(row))) {
            return false
        }
        rows.add(row)
        return true
    }

    private fun legacyKey(row: HistUi) = "${row.ts}|${row.zone}|${row.ml}|${row.trigger}|${row.outcome}"

    private fun line(row: HistUi) =
        "${row.seq}\t${row.ts}\t${row.zone}\t${row.ml}\t${row.trigger}\t${row.outcome}\n"

    private fun parse(text: String): HistUi? {
        val p = text.split('\t')
        if (p.size < 6) return null
        return HistUi(
            ts = p[1].toLongOrNull() ?: return null,
            zone = p[2].toIntOrNull() ?: return null,
            ml = p[3].toIntOrNull() ?: return null,
            trigger = p[4].toIntOrNull() ?: return null,
            outcome = p[5].toIntOrNull() ?: 0,
            seq = p[0].toLongOrNull() ?: 0,
        )
    }

    private fun sorted(): List<HistUi> =
        rows.sortedWith(compareByDescending<HistUi> { it.seq }.thenByDescending { it.ts })
}
