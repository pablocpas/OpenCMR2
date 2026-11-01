package io.github.pablocpas.opencmr2

import android.content.Context
import androidx.core.content.edit

/** Separate from controller mappings: a draft editor session never changes play. */
object TouchLayoutStore {
    private fun prefs(context: Context) = context.getSharedPreferences("touch_layout", Context.MODE_PRIVATE)

    fun load(context: Context): TouchLayoutSettings {
        val p = prefs(context)
        fun pad(prefix: String) = TouchPadSettings(
            p.getFloat("${prefix}_scale", 1f), p.getFloat("${prefix}_x", 0f),
            p.getFloat("${prefix}_y", 0f), p.getFloat("${prefix}_rotation", 0f),
            p.getFloat("${prefix}_opacity", 1f),
        )
        return TouchLayoutSettings(pad("left"), pad("right"), p.getBoolean("avoid_cutout", true)).normalized()
    }

    fun save(context: Context, settings: TouchLayoutSettings) {
        val layout = settings.normalized()
        prefs(context).edit {
            for ((prefix, pad) in listOf("left" to layout.left, "right" to layout.right)) {
                putFloat("${prefix}_scale", pad.scale)
                putFloat("${prefix}_x", pad.horizontal)
                putFloat("${prefix}_y", pad.vertical)
                putFloat("${prefix}_rotation", pad.rotation)
                putFloat("${prefix}_opacity", pad.opacity)
            }
            putBoolean("avoid_cutout", layout.avoidCutout)
        }
    }
}
