package io.github.pablocpas.opencmr2

import android.app.AlertDialog
import android.content.res.Configuration
import android.os.Bundle
import android.text.InputFilter
import android.text.InputType
import android.view.KeyEvent
import android.view.MotionEvent
import android.view.WindowManager
import android.view.inputmethod.EditorInfo
import android.widget.EditText
import android.widget.FrameLayout
import androidx.annotation.Keep
import org.libsdl.app.SDLActivity
import org.libsdl.app.SDLControllerManager

/** The game: SDL runs SDL_main from libmain.so, with the on-screen controls over it. */
class GameActivity : SDLActivity() {
    private var controls: TouchControls? = null

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

    // Called by sys.cpp as the game starts a text entry (a game's name, the
    // chat line, a saved game's name) without a physical keyboard: a text box
    // with the system's keyboard. The text accepted replaces the entry's.
    @Keep
    fun editText(text: String, maxLength: Int) {
        runOnUiThread {
            val edit = EditText(this).apply {
                setText(text)
                setSelection(text.length)
                filters = arrayOf(InputFilter.LengthFilter(maxLength))
                isSingleLine = true
                inputType = InputType.TYPE_CLASS_TEXT or InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS
                imeOptions = EditorInfo.IME_ACTION_DONE or EditorInfo.IME_FLAG_NO_EXTRACT_UI
            }
            val padding = (20 * resources.displayMetrics.density).toInt()
            val frame = FrameLayout(this).apply {
                setPadding(padding, padding / 2, padding, 0)
                addView(edit)
            }
            var answered = false
            fun answer(accepted: Boolean) {
                if (!answered) {
                    answered = true
                    nativeTextEntered(edit.text.toString(), accepted)
                }
            }
            val dialog = AlertDialog.Builder(this)
                .setView(frame)
                .setPositiveButton(android.R.string.ok) { _, _ -> answer(true) }
                .setNegativeButton(android.R.string.cancel) { _, _ -> answer(false) }
                .setOnDismissListener { answer(false) }
                .create()
            edit.setOnEditorActionListener { _, action, _ ->
                if (action != EditorInfo.IME_ACTION_DONE) return@setOnEditorActionListener false
                answer(true)
                dialog.dismiss()
                true
            }
            dialog.window?.setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_VISIBLE)
            dialog.show()
            edit.requestFocus()
        }
    }

    private external fun nativeTextEntered(text: String, accepted: Boolean)

    override fun getLibraries() = arrayOf("SDL3", "main")

    override fun getArguments() = arrayOf("--data", GameData.directory(this).absolutePath)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        // SDLActivity shows its own error when the libraries do not load.
        if (!mBrokenLibraries && mLayout != null)
            controls = TouchControls(this, mLayout)
    }

    // Back (the button or the gesture) is the game's Escape. Android sends its
    // press and release together; the game reads the keyboard once a frame,
    // so the release is held back long enough for it to see the press.
    override fun dispatchKeyEvent(event: KeyEvent): Boolean {
        controls?.onInput(event.source)
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
