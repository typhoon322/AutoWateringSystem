package com.langua.irrigation

import android.content.Context
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.net.Uri
import java.io.File

object ZoneStore {
    private const val PREF = "zone_local"

    fun name(context: Context, zone: Int): String =
        context.getSharedPreferences(PREF, Context.MODE_PRIVATE).getString(nameKey(zone), "").orEmpty()

    fun setName(context: Context, zone: Int, name: String) {
        context.getSharedPreferences(PREF, Context.MODE_PRIVATE).edit()
            .putString(nameKey(zone), name.trim())
            .apply()
    }

    fun photoFile(context: Context, zone: Int): File =
        File(context.filesDir, "zone_$zone.jpg")

    fun hasPhoto(context: Context, zone: Int): Boolean = photoFile(context, zone).exists()

    fun savePhoto(context: Context, zone: Int, uri: Uri) {
        val dest = photoFile(context, zone)
        context.contentResolver.openInputStream(uri)?.use { input ->
            dest.outputStream().use { output -> input.copyTo(output) }
        }
    }

    fun clearPhoto(context: Context, zone: Int) {
        photoFile(context, zone).delete()
    }

    fun bitmap(context: Context, zone: Int): Bitmap? {
        val file = photoFile(context, zone)
        if (!file.exists()) return null
        val bounds = BitmapFactory.Options().apply { inJustDecodeBounds = true }
        BitmapFactory.decodeFile(file.absolutePath, bounds)
        var sample = 1
        while (bounds.outWidth / sample > 480 && bounds.outWidth / sample > 0) sample *= 2
        val opts = BitmapFactory.Options().apply { inSampleSize = sample }
        return BitmapFactory.decodeFile(file.absolutePath, opts)
    }

    private fun nameKey(zone: Int) = "name_$zone"
}
