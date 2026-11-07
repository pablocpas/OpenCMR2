package io.github.pablocpas.opencmr2

import android.app.Activity
import android.content.Context
import android.content.Intent
import android.os.Build
import androidx.core.content.FileProvider
import java.io.File

/**
 * DIAG issue 46: the launcher's test switches and the log it shares. The game
 * writes opencmr2.log in its user directory each session and keeps the one
 * before as opencmr2.prev.log; both go out as one text file.
 */
object DiagnosticLog {
    private const val PREFS = "diagnostics"
    private const val FADE = "distance_fade"

    fun distanceFade(context: Context): Boolean =
        context.getSharedPreferences(PREFS, Context.MODE_PRIVATE).getBoolean(FADE, true)

    fun setDistanceFade(context: Context, on: Boolean) =
        context.getSharedPreferences(PREFS, Context.MODE_PRIVATE).edit().putBoolean(FADE, on).apply()

    /** Arguments for the game: the settings the switches change. */
    fun gameArguments(context: Context): Array<String> =
        if (distanceFade(context)) emptyArray() else arrayOf("--set", "video.distance_fade=0")

    // SDL_GetPrefPath is the internal storage; the other places cover older layouts.
    private fun logDirectories(context: Context): List<File> = listOfNotNull(
        context.filesDir, File(context.filesDir, "OpenCMR2"), context.getExternalFilesDir(null),
    )

    private fun header(context: Context): String {
        val version = runCatching {
            context.packageManager.getPackageInfo(context.packageName, 0).versionName
        }.getOrNull() ?: "?"
        val soc = if (Build.VERSION.SDK_INT >= 31) "${Build.SOC_MANUFACTURER} ${Build.SOC_MODEL}" else Build.HARDWARE
        return buildString {
            appendLine("OpenCMR2 $version (diagnostic build for issue 46)")
            appendLine("Device: ${Build.MANUFACTURER} ${Build.MODEL}, SoC $soc, Android ${Build.VERSION.RELEASE} (API ${Build.VERSION.SDK_INT})")
            appendLine("Distance fade: ${if (distanceFade(context)) "on" else "off"}")
        }
    }

    /** Opens the share sheet with the logs, or returns false when there are none. */
    fun share(activity: Activity): Boolean {
        val logs = logDirectories(activity).flatMap { dir ->
            listOf("opencmr2.prev.log", "opencmr2.log").map { File(dir, it) }
        }.filter { it.isFile }
        if (logs.isEmpty())
            return false
        val dir = File(activity.cacheDir, "diag").apply { mkdirs() }
        val out = File(dir, "opencmr2-logs.txt")
        out.writeText(buildString {
            append(header(activity))
            for (log in logs) {
                appendLine()
                appendLine("===== ${log.name} (${log.length()} bytes) =====")
                append(log.readText())
            }
        })
        val uri = FileProvider.getUriForFile(activity, "${activity.packageName}.logs", out)
        val send = Intent(Intent.ACTION_SEND).apply {
            type = "text/plain"
            putExtra(Intent.EXTRA_STREAM, uri)
            putExtra(Intent.EXTRA_SUBJECT, "OpenCMR2 logs (issue 46)")
            addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
        }
        activity.startActivity(Intent.createChooser(send, "Share OpenCMR2 logs"))
        return true
    }
}
