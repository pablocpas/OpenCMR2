import java.net.URI
import javax.inject.Inject

plugins {
    alias(libs.plugins.android.application)
    alias(libs.plugins.kotlin.compose)
}

// The SDL3 release the desktop builds fetch (GIT_TAG in the top CMakeLists.txt).
// Its source gives both libSDL3.so, built with the game, and SDLActivity.
val sdlVersion: String = providers.fileContents(rootProject.layout.projectDirectory.file("../CMakeLists.txt"))
    .asText.map { Regex("""GIT_TAG release-([0-9.]+)""").find(it)!!.groupValues[1] }.get()

abstract class FetchSdl : DefaultTask() {
    @get:Input abstract val version: Property<String>
    @get:OutputDirectory abstract val sourceDir: DirectoryProperty
    @get:OutputDirectory abstract val javaDir: DirectoryProperty
    @get:Inject abstract val archives: ArchiveOperations
    @get:Inject abstract val files: FileSystemOperations

    @TaskAction
    fun fetch() {
        val v = version.get()
        val archive = temporaryDir.resolve("SDL3-$v.tar.gz")
        URI("https://github.com/libsdl-org/SDL/releases/download/release-$v/SDL3-$v.tar.gz").toURL()
            .openStream().use { input -> archive.outputStream().use { input.copyTo(it) } }
        files.sync {
            from(archives.tarTree(archives.gzip(archive)))
            into(sourceDir)
            // Drop the archive's top directory, SDL3-<version>.
            eachFile { relativePath = RelativePath(true, *relativePath.segments.drop(1).toTypedArray()) }
            includeEmptyDirs = false
        }
        files.sync {
            from(sourceDir.dir("android-project/app/src/main/java"))
            into(javaDir)
        }
        archive.delete()
    }
}

val fetchSdl = tasks.register<FetchSdl>("fetchSdl") {
    version = sdlVersion
    sourceDir = layout.buildDirectory.dir("sdl/source")
    javaDir = layout.buildDirectory.dir("sdl/java")
}

// Version from git: the tag (v1.0.2-14-gabcdef) and the number of commits.
fun git(vararg args: String): String =
    providers.exec { commandLine("git", *args); isIgnoreExitValue = true }.standardOutput.asText.get().trim()

android {
    namespace = "io.github.pablocpas.opencmr2"
    compileSdk = 37
    ndkVersion = "30.0.16248370"

    defaultConfig {
        applicationId = "io.github.pablocpas.opencmr2"
        // Android 10: Vulkan 1.1 on every 64-bit device.
        minSdk = 29
        targetSdk = 37
        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
        // Test build for #46: the same code as the diagnostic build the tester
        // has (215), so it installs over it and 1.8.7 can still install over it.
        versionCode = 215
        versionName = git("describe", "--tags", "--always").removePrefix("v").ifEmpty { "dev" }

        ndk {
            abiFilters += listOf("arm64-v8a", "x86_64")
        }
        externalNativeBuild {
            cmake {
                arguments += listOf(
                    "-DFETCHCONTENT_SOURCE_DIR_SDL3=${layout.buildDirectory.dir("sdl/source").get().asFile.absolutePath}",
                    "-DANDROID_STL=c++_static",
                )
                targets += listOf("opencmr2", "SDL3-shared")
            }
        }
    }

    externalNativeBuild {
        cmake {
            path = file("../../CMakeLists.txt")
            version = "4.1.2"
        }
    }

    // Release signing from the environment (CI secrets) or ~/.gradle/gradle.properties;
    // without a keystore the release is signed with the debug key, so it still installs.
    signingConfigs {
        create("release") {
            val keystore = providers.gradleProperty("opencmr2.keystore")
                .orElse(providers.environmentVariable("OPENCMR2_KEYSTORE")).orNull
            if (keystore != null) {
                storeFile = file(keystore)
                storePassword = providers.gradleProperty("opencmr2.keystore.password")
                    .orElse(providers.environmentVariable("OPENCMR2_KEYSTORE_PASSWORD")).orNull
                keyAlias = providers.gradleProperty("opencmr2.key.alias")
                    .orElse(providers.environmentVariable("OPENCMR2_KEY_ALIAS")).orNull
                keyPassword = providers.gradleProperty("opencmr2.key.password")
                    .orElse(providers.environmentVariable("OPENCMR2_KEY_PASSWORD")).orNull
            }
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = true
            isShrinkResources = true
            proguardFiles(getDefaultProguardFile("proguard-android-optimize.txt"), "proguard-rules.pro")
            val release = signingConfigs.getByName("release")
            signingConfig = if (release.storeFile != null) release else signingConfigs.getByName("debug")
        }
    }

    buildFeatures {
        compose = true
    }

    // The app follows the system language, and Android 13+ lists these
    // languages in its per-app language settings (from res/values-*).
    // Library strings in other languages are left out, so a screen never
    // mixes languages.
    androidResources {
        generateLocaleConfig = true
        localeFilters += listOf("en", "de", "es", "fr", "it", "pl")
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
}

androidComponents {
    onVariants { variant ->
        variant.sources.java?.addGeneratedSourceDirectory(fetchSdl, FetchSdl::javaDir)
    }
}

// CMake reads SDL from the fetched source.
tasks.configureEach {
    if (name.startsWith("configureCMake") || name.startsWith("buildCMake"))
        dependsOn(fetchSdl)
}

dependencies {
    implementation(libs.androidx.core.ktx)
    implementation(libs.androidx.activity.compose)
    implementation(libs.androidx.lifecycle.viewmodel.compose)
    implementation(platform(libs.androidx.compose.bom))
    implementation(libs.androidx.compose.ui)
    implementation(libs.androidx.compose.material3)
    implementation(libs.androidx.compose.ui.tooling.preview)
    debugImplementation(libs.androidx.compose.ui.tooling)
    implementation(libs.kotlinx.coroutines.android)
    implementation(libs.radialgamepad) {
        // Only its build used the old Android extensions plugin.
        exclude(group = "org.jetbrains.kotlin", module = "kotlin-android-extensions-runtime")
    }
    testImplementation(libs.junit)
    androidTestImplementation(libs.androidx.test.runner)
    androidTestImplementation(libs.androidx.test.junit)
    androidTestImplementation(platform(libs.androidx.compose.bom))
    androidTestImplementation(libs.androidx.compose.ui.test)
}
