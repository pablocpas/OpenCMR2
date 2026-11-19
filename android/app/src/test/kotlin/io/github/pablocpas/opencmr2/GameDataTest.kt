package io.github.pablocpas.opencmr2

import org.junit.Assert.assertEquals
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder
import java.io.File

class GameDataTest {
    @get:Rule val temporary = TemporaryFolder()

    private fun write(root: File, path: String) = File(root, path).apply { parentFile!!.mkdirs(); writeText("data") }

    private fun installation(): File = temporary.newFolder("cmr2").also { root ->
        write(root, "GAME/Bigfiles/common.bfl")
        write(root, "FRONTEND/common.bfl")
        write(root, "SOUNDS/surface.bfl")
        write(root, "COUNTRYSPECIFIC/Europe/Text.bfl")
        write(root, "SETUPREP/1024/common.bfl")
    }

    @Test fun completeInstallationInAnyCase() {
        assertEquals(emptyList<String>(), GameData.missingIn(installation()))
    }

    @Test fun theThreeArchivesAloneAreIncomplete() {
        // What issue #65 imported: the files the setup text used to name.
        val root = temporary.newFolder("partial")
        write(root, "Game/Bigfiles/common.bfl")
        write(root, "FrontEnd/common.bfl")
        write(root, "Sounds/surface.bfl")
        assertEquals(listOf("CountrySpecific/", "SetupRep/"), GameData.missingIn(root))
    }

    @Test fun emptyFilesAndFoldersDoNotCount() {
        val root = installation()
        File(root, "SOUNDS/surface.bfl").writeText("")
        File(root, "SETUPREP").deleteRecursively()
        File(root, "SETUPREP").mkdirs()
        assertEquals(listOf("Sounds/surface.bfl", "SetupRep/"), GameData.missingIn(root))
    }

    @Test fun nothingOfAnInstallation() {
        assertEquals(
            listOf("Game/Bigfiles/common.bfl", "FrontEnd/common.bfl", "Sounds/surface.bfl", "CountrySpecific/", "SetupRep/"),
            GameData.missingIn(temporary.newFolder("empty")),
        )
    }
}
