package io.github.pablocpas.opencmr2

import android.content.ContentResolver
import android.content.Context
import android.net.Uri
import android.os.StatFs
import android.provider.DocumentsContract
import android.provider.DocumentsContract.Document
import kotlinx.coroutines.ensureActive
import java.io.File
import java.io.IOException
import kotlin.coroutines.coroutineContext

/**
 * The copy of the PC game the app plays: an installation copied into the
 * app's storage, read by the game with ordinary file access. Android gives
 * apps no such access to a folder the player chooses, so the import copies it
 * from there once (Storage Access Framework), as other ports of PC games do.
 */
object GameData {
    // The check MissingGameData does in src/platform/sys.cpp, which the game
    // repeats on start: files every PC edition has, and the folders with the
    // text, speech and loading screens, which some copies leave out.
    private val REQUIRED_FILES = listOf("Game/Bigfiles/common.bfl", "FrontEnd/common.bfl", "Sounds/surface.bfl")
    private val REQUIRED_FOLDERS = listOf("CountrySpecific", "SetupRep")
    private val REQUIRED_COUNT = REQUIRED_FILES.size + REQUIRED_FOLDERS.size

    /** A file or folder of a tree being checked; [id] finds its children. */
    internal data class Node<T>(val name: String, val isDirectory: Boolean, val size: Long, val id: T)

    /**
     * What the folder [root] lacks of an installation: the required paths it
     * does not have, folders with a trailing slash. Names match in any case,
     * as on the PC.
     */
    internal fun <T> missingIn(root: T, children: (T) -> List<Node<T>>): List<String> {
        fun find(path: String): Node<T>? {
            var parent = root
            var node: Node<T>? = null
            for (name in path.split('/')) {
                node = children(parent).firstOrNull { it.name.equals(name, ignoreCase = true) } ?: return null
                parent = node.id
            }
            return node
        }
        val files = REQUIRED_FILES.filter { path -> find(path).let { it == null || it.isDirectory || it.size <= 0 } }
        val folders = REQUIRED_FOLDERS.filter { path ->
            find(path).let { it == null || !it.isDirectory || children(it.id).isEmpty() }
        }
        return files + folders.map { "$it/" }
    }

    internal fun missingIn(dir: File): List<String> = missingIn(dir) { parent ->
        parent.listFiles()?.map { Node(it.name, it.isDirectory, it.length(), it) } ?: emptyList()
    }

    private fun storage(context: Context): File = context.getExternalFilesDir(null) ?: context.filesDir

    /** Where the game data lives; passed to the game with --data. */
    fun directory(context: Context): File = File(storage(context), "game")

    sealed interface Installation {
        data object None : Installation
        /** A copy without some required files: an import of an incomplete folder, or files deleted since. */
        data object Incomplete : Installation
        data object Complete : Installation
    }

    fun installation(context: Context): Installation {
        val dir = directory(context)
        if (!dir.isDirectory) return Installation.None
        return if (missingIn(dir).isEmpty()) Installation.Complete else Installation.Incomplete
    }

    sealed interface Progress {
        data object Searching : Progress
        data class Copying(val copied: Long, val total: Long) : Progress
    }

    sealed class ImportException(message: String) : IOException(message) {
        /** Nothing of an installation in or under the chosen folder. */
        class NotFound : ImportException("No installation found")
        /**
         * Part of an installation in or under the chosen folder. What it lacks
         * stays out of the player's message: a list of paths invites copying
         * just those, as the old message naming three files did (#65).
         */
        class Incomplete(val folder: String, val missing: List<String>) :
            ImportException("Incomplete installation in ${folder.ifEmpty { "/" }}: ${missing.joinToString()}")
        class Ambiguous(val folders: List<String>) : ImportException("More than one installation found")
        class NoSpace(val needed: Long, val available: Long) : ImportException("Not enough space")
    }

    /**
     * Copies the installation in or under [tree] (a folder from
     * ACTION_OPEN_DOCUMENT_TREE) to [directory]. The copy goes to a temporary
     * folder first, so an interrupted or incomplete import leaves the earlier
     * copy, if any, in place.
     */
    suspend fun import(context: Context, tree: Uri, onProgress: (Progress) -> Unit) {
        onProgress(Progress.Searching)
        val source = DocumentTree(context.contentResolver, tree)
        val search = source.findInstallations()
        val root = when (search.found.size) {
            0 -> throw search.closest?.let { ImportException.Incomplete(it.first.path, it.second) }
                ?: ImportException.NotFound()
            1 -> search.found.single()
            else -> throw ImportException.Ambiguous(search.found.map { it.path })
        }
        val files = source.listFiles(root.documentId)
        val total = files.sumOf { it.second.size }

        val target = directory(context)
        val temporary = File(storage(context), "game.import")
        temporary.deleteRecursively()
        // The space of an earlier copy is freed when this one replaces it.
        val available = StatFs(storage(context).path).availableBytes + target.walkBottomUp().sumOf { it.length() }
        val margin = 64L shl 20
        if (available < total + margin)
            throw ImportException.NoSpace(total + margin, available)

        try {
            var copied = 0L
            var reported = 0L
            val buffer = ByteArray(1 shl 20)
            onProgress(Progress.Copying(0, total))
            for ((path, entry) in files) {
                val file = File(temporary, path)
                file.parentFile!!.mkdirs()
                source.open(entry.documentId).use { input ->
                    file.outputStream().use { output ->
                        while (true) {
                            coroutineContext.ensureActive()
                            val n = input.read(buffer)
                            if (n < 0) break
                            output.write(buffer, 0, n)
                            copied += n
                            if (copied - reported >= (4 shl 20)) {
                                onProgress(Progress.Copying(copied, total))
                                reported = copied
                            }
                        }
                    }
                }
            }
            onProgress(Progress.Copying(total, total))
            // Some providers report sizes they do not deliver (a cloud file
            // not downloaded): check the copy as the game will.
            val missing = missingIn(temporary)
            if (missing.isNotEmpty())
                throw ImportException.Incomplete(root.path, missing)
            target.deleteRecursively()
            if (!temporary.renameTo(target))
                throw IOException("Cannot move the copied files to $target")
        } finally {
            temporary.deleteRecursively()
        }
    }

    private class DocumentTree(private val resolver: ContentResolver, private val tree: Uri) {
        data class Entry(val documentId: String, val name: String, val isDirectory: Boolean, val size: Long)
        data class Folder(val documentId: String, val path: String)
        class Search(val found: List<Folder>, val closest: Pair<Folder, List<String>>?)

        fun children(documentId: String): List<Entry> {
            val uri = DocumentsContract.buildChildDocumentsUriUsingTree(tree, documentId)
            val projection = arrayOf(
                Document.COLUMN_DOCUMENT_ID, Document.COLUMN_DISPLAY_NAME, Document.COLUMN_MIME_TYPE, Document.COLUMN_SIZE,
            )
            val entries = mutableListOf<Entry>()
            resolver.query(uri, projection, null, null, null)?.use { cursor ->
                while (cursor.moveToNext()) {
                    entries += Entry(
                        documentId = cursor.getString(0),
                        name = cursor.getString(1) ?: continue,
                        isDirectory = cursor.getString(2) == Document.MIME_TYPE_DIR,
                        size = if (cursor.isNull(3)) 0 else cursor.getLong(3),
                    )
                }
            }
            return entries
        }

        fun open(documentId: String) =
            resolver.openInputStream(DocumentsContract.buildDocumentUriUsingTree(tree, documentId))
                ?: throw IOException("Cannot open $documentId")

        private fun missing(documentId: String): List<String> = missingIn(documentId) { id ->
            children(id).map { Node(it.name, it.isDirectory, it.size, it.documentId) }
        }

        /**
         * The chosen folder when it is an installation, otherwise the ones
         * found up to three levels below it (a download folder around it, as
         * on the desktop). Found installations are not searched further. With
         * none, the folder with the most of one, to say what it lacks.
         */
        fun findInstallations(): Search {
            val pending = ArrayDeque(listOf(Folder(DocumentsContract.getTreeDocumentId(tree), "") to 0))
            val found = mutableListOf<Folder>()
            var closest: Pair<Folder, List<String>>? = null
            var visited = 0
            while (pending.isNotEmpty() && visited++ < 512) {
                val (folder, depth) = pending.removeFirst()
                val missing = missing(folder.documentId)
                if (missing.isEmpty()) {
                    found += folder
                    continue
                }
                if (missing.size < (closest?.second?.size ?: REQUIRED_COUNT))
                    closest = folder to missing
                if (depth == 3) continue
                for (child in children(folder.documentId)) {
                    if (child.isDirectory && !child.name.startsWith('.'))
                        pending += Folder(child.documentId, folder.path + "/" + child.name) to depth + 1
                }
            }
            return Search(found, closest)
        }

        /** Every file under the folder, with its path relative to it. */
        fun listFiles(documentId: String): List<Pair<String, Entry>> {
            val files = mutableListOf<Pair<String, Entry>>()
            fun walk(id: String, prefix: String) {
                for (child in children(id)) {
                    if (child.isDirectory) walk(child.documentId, prefix + child.name + "/")
                    else files += prefix + child.name to child
                }
            }
            walk(documentId, "")
            return files
        }
    }
}
