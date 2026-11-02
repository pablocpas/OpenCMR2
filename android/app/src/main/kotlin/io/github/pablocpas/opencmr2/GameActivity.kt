package io.github.pablocpas.opencmr2

import android.content.res.Configuration
import android.os.Bundle
import android.view.KeyEvent
import android.view.MotionEvent
import androidx.annotation.Keep
import org.libsdl.app.SDLActivity
import org.libsdl.app.SDLControllerManager

/** The game: SDL runs SDL_main from libmain.so, with the on-screen controls over it. */
class GameActivity : SDLActivity() {
    private var controls: TouchControls? = null
    private var textEntry: TextEntry? = null

    // Called by input_sdl.cpp before opening a controller.
    @Keep
    fun getControllerMapping(name: String, vendor: Int, product: Int): String =
        ControllerProfiles.mapping(this, name, vendor, product)

    @Keep
    fun useTouchControlsOnly(): Boolean = ControllerProfiles.touchOnly(this)

    // Called by input_sdl.cpp: the controls pages list the keyboard players
    // only with a physical keyboard; the touch controls are edited here.
    @Keep
    fun hasHardwareKeyboard(): Boolean = resources.configuration.keyboard == Configuration.KEYBOARD_QWERTY

    // Called by sys.cpp, on the game's thread: the game starts editing a text
    // entry without a physical keyboard, or the entry's chip asked for it again.
    @Keep
    fun openTextEntry(label: String, text: String, maxLength: Int) =
        runOnUiThread { textEntry?.open(label, text, maxLength) }

    // Called by sys.cpp: the game left the entry.
    @Keep
    fun endTextEntry() = runOnUiThread { textEntry?.end() }

    override fun getLibraries() = arrayOf("SDL3", "main")

    override fun getArguments() = arrayOf("--data", GameData.directory(this).absolutePath)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        // SDLActivity shows its own error when the libraries do not load.
        if (!mBrokenLibraries && mLayout != null) {
            controls = TouchControls(this, mLayout)
            textEntry = TextEntry(this, mLayout) { open -> controls?.setSuspended(open) }
        }
    }

    // Back (the button or the gesture) is the game's Escape. Android sends its
    // press and release together; the game reads the keyboard once a frame,
    // so the release is held back long enough for it to see the press.
    override fun dispatchKeyEvent(event: KeyEvent): Boolean {
        controls?.onInput(event.source)
        if (textEntry?.onKey(event) == true)
            return true
        if (event.keyCode != KeyEvent.KEYCODE_BACK || mBrokenLibraries ||
            SDLControllerManager.isDeviceSDLJoystick(event.deviceId))
            return super.dispatchKeyEvent(event)
        if (event.action == KeyEvent.ACTION_DOWN && event.repeatCount == 0) {
            onNativeKeyDown(KeyEvent.KEYCODE_ESCAPE)
            window.decorView.postDelayed({ onNativeKeyUp(KeyEvent.KEYCODE_ESCAPE) }, 100)
        }
        return true
    }

    override fun dispatchTouchEvent(event: MotionEvent): Boolean {
        controls?.onInput(event.source)
        return super.dispatchTouchEvent(event)
    }

    override fun dispatchGenericMotionEvent(event: MotionEvent): Boolean {
        controls?.onInput(event.source)
        return super.dispatchGenericMotionEvent(event)
    }

    override fun onPause() {
        controls?.releaseAll()
        super.onPause()
    }

    override fun onResume() {
        super.onResume()
        controls?.reloadLayout()
    }

    override fun onDestroy() {
        controls?.close()
        controls = null
        super.onDestroy()
    }
}
