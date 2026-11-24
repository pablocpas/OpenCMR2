package io.github.pablocpas.opencmr2

import android.app.Activity
import android.content.ActivityNotFoundException
import android.content.Context
import android.content.Intent
import android.database.Cursor
import android.database.MatrixCursor
import android.os.CancellationSignal
import android.os.ParcelFileDescriptor
import android.provider.DocumentsContract
import android.provider.DocumentsContract.Document
import android.provider.DocumentsContract.Root
import android.provider.DocumentsProvider
import android.webkit.MimeTypeMap
import android.widget.Toast
import java.io.File
import java.io.FileNotFoundException

/**
 * The game's user folder (SDL's pref path, the app's internal storage: saved
 * games, settings, controls, the log) in the system Files app and any app that
 * picks files or folders through it, to back the files up, sync them or bring
 * them over from a PC. Android gives no other app access to that folder.
 */
class UserFilesProvider : DocumentsProvider() {
    private lateinit var base: File

    override fun onCreate(): Boolean {
        base = userDir(context!!)
        return true
    }

    override fun queryRoots(projection: Array<out String>?): Cursor {
        val context = context!!
        return MatrixCursor(projection ?: ROOT_COLUMNS).apply {
            newRow().apply {
                add(Root.COLUMN_ROOT_ID, ROOT)
                add(Root.COLUMN_DOCUMENT_ID, ROOT)
                add(Root.COLUMN_TITLE, context.getString(R.string.app_name))
                add(Root.COLUMN_SUMMARY, context.getString(R.string.user_files_summary))
                add(Root.COLUMN_ICON, R.mipmap.ic_launcher)
                add(Root.COLUMN_FLAGS, Root.FLAG_SUPPORTS_CREATE or Root.FLAG_SUPPORTS_IS_CHILD or Root.FLAG_LOCAL_ONLY)
                add(Root.COLUMN_AVAILABLE_BYTES, base.freeSpace)
            }
        }
    }

    override fun queryDocument(documentId: String, projection: Array<out String>?): Cursor =
        MatrixCursor(projection ?: DOCUMENT_COLUMNS).apply { addFile(this, documentId, file(documentId)) }

    override fun queryChildDocuments(parentId: String, projection: Array<out String>?, sortOrder: String?): Cursor {
        val dir = file(parentId)
        return MatrixCursor(projection ?: DOCUMENT_COLUMNS).apply {
            for (child in dir.listFiles().orEmpty().sortedBy { it.name.lowercase() })
                addFile(this, id(child), child)
            setNotificationUri(context!!.contentResolver, DocumentsContract.buildChildDocumentsUri(authority(context!!), parentId))
        }
    }

    override fun openDocument(documentId: String, mode: String, signal: CancellationSignal?): ParcelFileDescriptor {
        val file = file(documentId)
        if (file.isDirectory) throw FileNotFoundException("$documentId is a folder")
        return ParcelFileDescriptor.open(file, ParcelFileDescriptor.parseMode(mode))
    }

    override fun createDocument(parentId: String, mimeType: String, displayName: String): String {
        val parent = file(parentId)
        if ('/' in displayName) throw FileNotFoundException(displayName)
        var target = File(parent, displayName)
        // The system picker asks for a unique name only after creating; follow
        // its "name (1)" convention instead of failing.
        var n = 1
        while (target.exists()) {
            val dot = displayName.lastIndexOf('.').takeIf { it > 0 && mimeType != Document.MIME_TYPE_DIR }
            target = File(parent, if (dot == null) "$displayName ($n)" else
                "${displayName.substring(0, dot)} ($n)${displayName.substring(dot)}")
            n++
        }
        check(target)
        val ok = if (mimeType == Document.MIME_TYPE_DIR) target.mkdir() else target.createNewFile()
        if (!ok) throw FileNotFoundException("Cannot create $displayName")
        changed(parentId)
        return id(target)
    }

    override fun deleteDocument(documentId: String) {
        if (documentId == ROOT) throw UnsupportedOperationException("The user folder itself stays")
        val file = file(documentId)
        if (!file.deleteRecursively()) throw FileNotFoundException("Cannot delete $documentId")
        changed(id(file.parentFile!!))
    }

    override fun renameDocument(documentId: String, displayName: String): String {
        if (documentId == ROOT) throw UnsupportedOperationException("The user folder itself keeps its name")
        val file = file(documentId)
        if ('/' in displayName) throw FileNotFoundException(displayName)
        val target = File(file.parentFile, displayName)
        check(target)
        if (target.exists() || !file.renameTo(target)) throw FileNotFoundException("Cannot rename to $displayName")
        changed(id(file.parentFile!!))
        return id(target)
    }

    override fun isChildDocument(parentId: String, documentId: String): Boolean =
        documentId == parentId || documentId.startsWith(if (parentId == ROOT) "$ROOT/" else "$parentId/")

    override fun getDocumentType(documentId: String): String = mimeType(file(documentId))

    private fun addFile(cursor: MatrixCursor, id: String, file: File) {
        if (!file.exists()) throw FileNotFoundException(id)
        var flags = 0
        if (file.isDirectory) flags = flags or Document.FLAG_DIR_SUPPORTS_CREATE
        else flags = flags or Document.FLAG_SUPPORTS_WRITE
        if (id != ROOT) flags = flags or Document.FLAG_SUPPORTS_DELETE or Document.FLAG_SUPPORTS_RENAME
        cursor.newRow().apply {
            add(Document.COLUMN_DOCUMENT_ID, id)
            add(Document.COLUMN_DISPLAY_NAME, if (id == ROOT) context!!.getString(R.string.app_name) else file.name)
            add(Document.COLUMN_MIME_TYPE, mimeType(file))
            add(Document.COLUMN_SIZE, if (file.isDirectory) null else file.length())
            add(Document.COLUMN_LAST_MODIFIED, file.lastModified())
            add(Document.COLUMN_FLAGS, flags)
        }
    }

    private fun changed(parentId: String) = context!!.contentResolver.notifyChange(
        DocumentsContract.buildChildDocumentsUri(authority(context!!), parentId), null)

    private fun id(file: File): String {
        val path = file.canonicalFile.toRelativeString(base.canonicalFile)
        return if (path.isEmpty()) ROOT else "$ROOT/$path"
    }

    private fun file(id: String): File {
        val file = when {
            id == ROOT -> base
            id.startsWith("$ROOT/") -> File(base, id.removePrefix("$ROOT/"))
            else -> throw FileNotFoundException(id)
        }
        check(file)
        return file
    }

    // Names come from other apps: nothing may reach outside the folder.
    private fun check(file: File) {
        if (file.name.isEmpty() || file.name == "." || file.name == "..") throw FileNotFoundException(file.path)
        val root = base.canonicalFile
        val canonical = file.canonicalFile
        if (canonical != root && !canonical.startsWith(root)) throw FileNotFoundException(file.path)
    }

    private fun mimeType(file: File): String {
        if (file.isDirectory) return Document.MIME_TYPE_DIR
        val extension = file.extension.lowercase()
        return MimeTypeMap.getSingleton().getMimeTypeFromExtension(extension)
            ?: if (extension == "ini" || extension == "log" || extension == "txt") "text/plain" else "application/octet-stream"
    }

    companion object {
        private const val ROOT = "user"
        private val ROOT_COLUMNS = arrayOf(Root.COLUMN_ROOT_ID, Root.COLUMN_DOCUMENT_ID, Root.COLUMN_TITLE,
            Root.COLUMN_SUMMARY, Root.COLUMN_ICON, Root.COLUMN_FLAGS, Root.COLUMN_AVAILABLE_BYTES)
        private val DOCUMENT_COLUMNS = arrayOf(Document.COLUMN_DOCUMENT_ID, Document.COLUMN_DISPLAY_NAME,
            Document.COLUMN_MIME_TYPE, Document.COLUMN_SIZE, Document.COLUMN_LAST_MODIFIED, Document.COLUMN_FLAGS)

        private fun authority(context: Context) = "${context.packageName}.user"

        /** SDL_GetPrefPath on Android: the game's user folder. */
        fun userDir(context: Context): File = context.filesDir

        /**
         * Opens the folder in the system Files app. Its activities are not
         * public API, so this tries the usual ways in turn, as Dolphin does.
         */
        fun open(activity: Activity) {
            val root = DocumentsContract.buildRootUri(authority(activity), ROOT)
            val grants = Intent.FLAG_GRANT_READ_URI_PERMISSION or Intent.FLAG_GRANT_WRITE_URI_PERMISSION or
                Intent.FLAG_GRANT_PREFIX_URI_PERMISSION
            val intents = listOf(
                Intent(Intent.ACTION_VIEW, root).addCategory(Intent.CATEGORY_DEFAULT).addFlags(grants),
                Intent("android.provider.action.BROWSE", root).addCategory(Intent.CATEGORY_DEFAULT).addFlags(grants),
                Intent(Intent.ACTION_MAIN).setClassName("com.google.android.documentsui",
                    "com.android.documentsui.files.FilesActivity"),
                Intent(Intent.ACTION_MAIN).setClassName("com.android.documentsui",
                    "com.android.documentsui.files.FilesActivity"),
            )
            for (intent in intents) {
                try {
                    activity.startActivity(intent)
                    return
                } catch (_: ActivityNotFoundException) {
                } catch (_: SecurityException) {
                }
            }
            Toast.makeText(activity, R.string.user_files_no_manager, Toast.LENGTH_LONG).show()
        }
    }
}
