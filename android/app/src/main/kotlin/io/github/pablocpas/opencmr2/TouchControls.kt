package io.github.pablocpas.opencmr2

import android.app.Activity
import android.content.Context
import android.hardware.input.InputManager
import android.os.Handler
import android.os.Looper
import android.view.Gravity
import android.view.InputDevice
import android.view.KeyEvent
import android.view.MotionEvent
import android.view.View
import android.view.ViewGroup
import android.widget.FrameLayout
import androidx.core.view.ViewCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.isVisible
import com.swordfish.radialgamepad.library.RadialGamePad
import com.swordfish.radialgamepad.library.config.ButtonConfig
import com.swordfish.radialgamepad.library.config.PrimaryDialConfig
import com.swordfish.radialgamepad.library.config.RadialGamePadConfig
import com.swordfish.radialgamepad.library.config.RadialGamePadTheme
import com.swordfish.radialgamepad.library.config.SecondaryDialConfig
import com.swordfish.radialgamepad.library.event.Event
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.MainScope
import kotlinx.coroutines.cancel
import kotlinx.coroutines.launch
import org.libsdl.app.SDLActivity

/**
 * On-screen controls drawn over the game with RadialGamePad. They press the
 * keys of the game's default keyboard layout for player 1, so they work in the
 * menus and on the stages without setting up a controller: Left and Right
 * steer, accelerate and brake are Up and Down (and move through the menus),
 * Space is the handbrake, ] and [ change gear, C the camera, Enter and Escape
 * confirm and go back or pause. One layout serves the menus and the stages.
 * They hide while a gamepad is played and come back on a touch.
 */
class TouchControls(
    private val activity: Activity,
    parent: ViewGroup,
    private val preview: Boolean = false,
    private val onKey: (Int, Boolean) -> Unit = { key, down ->
        if (down) SDLActivity.onNativeKeyDown(key) else SDLActivity.onNativeKeyUp(key)
    },
) : InputManager.InputDeviceListener {
    private val scope: CoroutineScope = MainScope()
    private val inputManager = activity.getSystemService(Context.INPUT_SERVICE) as InputManager
    private val pressed = mutableSetOf<Int>()
    // Steering and pedals have physical left/right positions in every locale.
    private val overlay = FrameLayout(activity).apply { layoutDirection = View.LAYOUT_DIRECTION_LTR }
    private val pads = mutableListOf<Pair<RadialGamePad, Boolean>>()
    private var settings = TouchLayoutStore.load(activity)
    private var cutout = TouchInsets()
    private var systemInsets = TouchInsets()

    init {
        val theme = RadialGamePadTheme(
            normalColor = 0x50FFFFFF,
            pressedColor = 0xA0FFFFFF.toInt(),
            textColor = 0xE0FFFFFF.toInt(),
            backgroundColor = 0x28000000,
            lightColor = 0x30FFFFFF,
        )
        val left = RadialGamePad(leftConfig(theme), 8f, activity)
        val right = RadialGamePad(rightConfig(theme), 8f, activity)
        for ((pad, isLeft) in listOf(left to true, right to false)) {
            pads += pad to isLeft
            pad.tag = if (isLeft) "touch-left" else "touch-right"
            overlay.addView(pad, FrameLayout.LayoutParams(0, 0, Gravity.TOP or Gravity.START))
            scope.launch { pad.events().collect { event ->
                // A queued press must not re-hold a key after the controls
                // were hidden and releaseAll cleared it.
                if (pad.isShown) handle(event)
            } }
        }
        overlay.addOnLayoutChangeListener { _, l, t, r, b, oldL, oldT, oldR, oldB ->
            if (r - l != oldR - oldL || b - t != oldB - oldT) {
                releaseAll()
                layoutPads()
            }
        }
        ViewCompat.setOnApplyWindowInsetsListener(overlay) { _, insets ->
            val c = insets.getInsets(WindowInsetsCompat.Type.displayCutout())
            // Gesture insets can remain even with the navigation bar hidden.
            // Only visible bars should limit placement in immersive mode.
            val s = insets.getInsets(WindowInsetsCompat.Type.systemBars())
            val nextCutout = TouchInsets(c.left, c.top, c.right, c.bottom)
            val nextSystem = TouchInsets(s.left, s.top, s.right, s.bottom)
            if (cutout != nextCutout || systemInsets != nextSystem) {
                releaseAll()
                cutout = nextCutout
                systemInsets = nextSystem
                layoutPads()
            }
            insets
        }
        parent.addView(overlay, ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT))
        ViewCompat.requestApplyInsets(overlay)

        inputManager.registerInputDeviceListener(this, Handler(Looper.getMainLooper()))
        updateVisibility()
    }

    fun applyLayout(value: TouchLayoutSettings) {
        val next = value.normalized()
        if (next == settings) return
        releaseAll()
        settings = next
        layoutPads()
    }

    fun reloadLayout() = applyLayout(TouchLayoutStore.load(activity))

    fun placement(isLeft: Boolean): TouchPadPlacement = TouchLayoutGeometry.place(
        overlay.width, overlay.height, isLeft,
        systemInsets.union(if (settings.avoidCutout) cutout else TouchInsets()), settings.pad(isLeft),
    )

    private fun layoutPads() {
        if (overlay.width <= 0 || overlay.height <= 0) return
        for ((pad, isLeft) in pads) {
            val place = placement(isLeft)
            val params = pad.layoutParams as FrameLayout.LayoutParams
            // Each view keeps the whole half-screen. Move its drawing area
            // with the library's spacing API, preserving expanded hit regions.
            if (params.width != place.viewWidth || params.height != overlay.height || params.leftMargin != place.viewLeft) {
                params.width = place.viewWidth
                params.height = overlay.height
                params.leftMargin = place.viewLeft
                pad.layoutParams = params
            }
            if (pad.spacingLeft != place.spacingLeft) pad.spacingLeft = place.spacingLeft
            if (pad.spacingRight != place.spacingRight) pad.spacingRight = place.spacingRight
            if (pad.spacingTop != place.top) pad.spacingTop = place.top
            val bottom = overlay.height - place.top - place.size
            if (pad.spacingBottom != bottom) pad.spacingBottom = bottom
            val config = settings.pad(isLeft)
            if (pad.secondaryDialRotation != config.rotation) pad.secondaryDialRotation = config.rotation
            pad.alpha = config.opacity
            pad.visibility = if (place.size > 0) View.VISIBLE else View.GONE
        }
    }

    private fun leftConfig(theme: RadialGamePadTheme) = RadialGamePadConfig(
        sockets = 12,
        // Steering and menu Left/Right. Up and Down are the pedals on the right.
        primaryDial = PrimaryDialConfig.PrimaryButtons(
            dials = listOf(
                ButtonConfig(KeyEvent.KEYCODE_DPAD_RIGHT, iconId = R.drawable.ic_touch_right,
                    contentDescription = activity.getString(R.string.controllers_right)),
                ButtonConfig(KeyEvent.KEYCODE_DPAD_LEFT, iconId = R.drawable.ic_touch_left,
                    contentDescription = activity.getString(R.string.controllers_left)),
            ),
            allowMultiplePressesSingleFinger = false,
        ),
        secondaryDials = listOf(
            // Confirm and Back sit above the arrows, away from the pedals.
            button(index = 1, KeyEvent.KEYCODE_ENTER, R.drawable.ic_touch_ok, R.string.touch_ok, 0.8f),
            button(index = 2, KeyEvent.KEYCODE_ESCAPE, R.drawable.ic_touch_back, R.string.touch_back, 0.8f),
            SecondaryDialConfig.Empty(index = 3, spread = 1, scale = 0.8f, distance = 0f),
        ),
        theme = theme,
    )

    private fun rightConfig(theme: RadialGamePadTheme) = RadialGamePadConfig(
        sockets = 12,
        // Pedals share a lower row: brake left, throttle right, handbrake above.
        // All three share the primary dial so the thumb can slide between them
        // without lifting.
        primaryDial = PrimaryDialConfig.PrimaryButtons(
            dials = listOf(
                ButtonConfig(KeyEvent.KEYCODE_DPAD_UP, iconId = R.drawable.ic_touch_accelerate,
                    contentDescription = activity.getString(R.string.touch_accelerate)),
                ButtonConfig(KeyEvent.KEYCODE_SPACE, iconId = R.drawable.ic_touch_handbrake,
                    contentDescription = activity.getString(R.string.touch_handbrake)),
                ButtonConfig(KeyEvent.KEYCODE_DPAD_DOWN, iconId = R.drawable.ic_touch_brake,
                    contentDescription = activity.getString(R.string.touch_brake)),
            ),
            rotationInDegrees = -30f,
            allowMultiplePressesSingleFinger = false,
        ),
        secondaryDials = listOf(
            button(index = 3, KeyEvent.KEYCODE_RIGHT_BRACKET, R.drawable.ic_touch_gear_up, R.string.touch_gear_up, 0.75f),
            button(index = 4, KeyEvent.KEYCODE_LEFT_BRACKET, R.drawable.ic_touch_gear_down, R.string.touch_gear_down, 0.75f),
            button(index = 5, KeyEvent.KEYCODE_C, R.drawable.ic_touch_camera, R.string.touch_camera, 0.75f),
        ),
        theme = theme,
    )

    private fun button(index: Int, keyCode: Int, icon: Int, description: Int, scale: Float) = SecondaryDialConfig.SingleButton(
        index = index,
        scale = scale,
        distance = 0f,
        buttonConfig = ButtonConfig(keyCode, iconId = icon, contentDescription = activity.getString(description)),
    )

    private fun handle(event: Event) {
        if (event is Event.Button) set(event.id, event.action == KeyEvent.ACTION_DOWN)
    }

    private fun set(keyCode: Int, down: Boolean) {
        if (down == keyCode in pressed) return
        if (down) pressed += keyCode else pressed -= keyCode
        onKey(keyCode, down)
    }

    /** Lets go of every key, when the game pauses or the controls hide. */
    fun releaseAll() {
        // Cancel RadialGamePad's pointer ownership too: after a pause/hide,
        // no finger from a previous gesture may resume a held accelerator.
        for ((pad, _) in pads) {
            val now = android.os.SystemClock.uptimeMillis()
            val cancel = MotionEvent.obtain(now, now, MotionEvent.ACTION_CANCEL, 0f, 0f, 0)
            pad.dispatchTouchEvent(cancel)
            cancel.recycle()
        }
        for (id in pressed.toList()) set(id, false)
    }

    // A controller, external or built into a handheld, has gamepad buttons.
    // Phones list other built-in devices with joystick sources too (the
    // OnePlus 12's touch panel), but those have no gamepad buttons.
    private fun isGamepad(device: InputDevice?) = device != null && !device.isVirtual &&
        device.supportsSource(InputDevice.SOURCE_GAMEPAD) && !device.supportsSource(InputDevice.SOURCE_TOUCHSCREEN)

    private var suspended = false
    private var shown = true

    private fun show(visible: Boolean) {
        shown = preview || visible || ControllerProfiles.showTouch(activity) || ControllerProfiles.touchOnly(activity)
        if (!shown || suspended) releaseAll()
        overlay.isVisible = shown && !suspended
    }

    /** Hidden while a text entry's bar is open, back as they were after. */
    fun setSuspended(value: Boolean) {
        suspended = value
        if (suspended) releaseAll()
        overlay.isVisible = shown && !suspended
    }

    private fun updateVisibility() =
        show(inputManager.inputDeviceIds.none { isGamepad(inputManager.getInputDevice(it)) })

    /**
     * Follows what is being played, by the source of each event: a gamepad
     * button or stick hides the controls, a touch shows them again.
     */
    fun onInput(source: Int) {
        if ((source and InputDevice.SOURCE_TOUCHSCREEN) == InputDevice.SOURCE_TOUCHSCREEN) {
            if (!shown) show(true)
        } else if (overlay.isVisible && ((source and InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD ||
                (source and InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK)) {
            show(false)
        }
    }

    override fun onInputDeviceAdded(deviceId: Int) = updateVisibility()
    override fun onInputDeviceRemoved(deviceId: Int) = updateVisibility()
    override fun onInputDeviceChanged(deviceId: Int) {}

    fun close() {
        releaseAll()
        inputManager.unregisterInputDeviceListener(this)
        scope.cancel()
        (overlay.parent as? ViewGroup)?.removeView(overlay)
    }
}
