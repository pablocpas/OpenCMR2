package io.github.pablocpas.opencmr2

import android.content.Context
import android.content.SharedPreferences
import android.view.InputDevice
import android.view.KeyEvent
import android.view.MotionEvent
import androidx.core.content.edit

/** Sparse overrides of SDL's automatic layout, saved by Android device descriptor. */
object ControllerProfiles {
    private fun prefs(context: Context) = context.getSharedPreferences("controllers", Context.MODE_PRIVATE)
    fun devices() = InputDevice.getDeviceIds().toList().mapNotNull(InputDevice::getDevice).filter {
        !it.isVirtual && !it.supportsSource(InputDevice.SOURCE_TOUCHSCREEN) &&
            (it.supportsSource(InputDevice.SOURCE_GAMEPAD) || it.supportsSource(InputDevice.SOURCE_JOYSTICK) ||
                it.supportsSource(InputDevice.SOURCE_DPAD))
    }
    fun load(context: Context, device: InputDevice): Map<String, String> =
        prefs(context).getString(device.descriptor, "").orEmpty().split(',').mapNotNull {
            val colon = it.indexOf(':')
            if (colon < 0) null else it.substring(0, colon) to it.substring(colon + 1)
        }.toMap()

    fun bind(context: Context, device: InputDevice, target: String, source: String) {
        val bindings = load(context, device).toMutableMap()
        // An explicitly emptied action also suppresses its automatic SDL binding.
        for ((action, input) in bindings.toMap())
            if (input == source && action != target) bindings[action] = ""
        bindings[target] = source
        prefs(context).edit { putString(device.descriptor, bindings.entries.joinToString(",") { "${it.key}:${it.value}" }) }
    }
    fun reset(context: Context, device: InputDevice) { prefs(context).edit { remove(device.descriptor) } }
    fun touchOnly(context: Context) = prefs(context).getBoolean("touch_only", false)
    fun touchOnly(context: Context, value: Boolean) { prefs(context).edit { putBoolean("touch_only", value) } }
    fun showTouch(context: Context) = prefs(context).getBoolean("show_touch", false)
    fun showTouch(context: Context, value: Boolean) { prefs(context).edit { putBoolean("show_touch", value) } }
    fun detailedTouchLayout(context: Context) = prefs(context).getBoolean("touch_layout_detailed", false)
    fun detailedTouchLayout(context: Context, value: Boolean) {
        prefs(context).edit { putBoolean("touch_layout_detailed", value) }
    }

    // Touch layout, adjustable in ControllerActivity: plain numbers so another
    // platform's overlay can reuse the same model. Scale multiplies the pads;
    // margins are 0..1 of a quarter of the screen; opacity 0.2..1.
    fun touchScale(context: Context) = prefs(context).getFloat("touch_scale", 1f)
    fun touchScale(context: Context, value: Float) { prefs(context).edit { putFloat("touch_scale", value) } }
    fun touchMarginX(context: Context) = prefs(context).getFloat("touch_margin_x", 0f)
    fun touchMarginX(context: Context, value: Float) { prefs(context).edit { putFloat("touch_margin_x", value) } }
    fun touchMarginY(context: Context) = prefs(context).getFloat("touch_margin_y", 0f)
    fun touchMarginY(context: Context, value: Float) { prefs(context).edit { putFloat("touch_margin_y", value) } }
    fun touchOpacity(context: Context) = prefs(context).getFloat("touch_opacity", 1f)
    fun touchOpacity(context: Context, value: Float) { prefs(context).edit { putFloat("touch_opacity", value) } }
    fun touchAvoidCutout(context: Context) = prefs(context).getBoolean("touch_avoid_cutout", true)
    fun touchAvoidCutout(context: Context, value: Boolean) { prefs(context).edit { putBoolean("touch_avoid_cutout", value) } }
    fun resetTouchLayout(context: Context) {
        prefs(context).edit {
            remove("touch_scale")
            remove("touch_margin_x")
            remove("touch_margin_y")
            remove("touch_opacity")
            remove("touch_avoid_cutout")
        }
    }

    /** Layout changes apply while the game runs: TouchControls listens. */
    fun watch(context: Context, listener: SharedPreferences.OnSharedPreferenceChangeListener) =
        prefs(context).registerOnSharedPreferenceChangeListener(listener)
    fun unwatch(context: Context, listener: SharedPreferences.OnSharedPreferenceChangeListener) =
        prefs(context).unregisterOnSharedPreferenceChangeListener(listener)

    // SDL's Android driver normalizes the device name for some models. Match
    // an exact name first, then VID/PID only when there is a single candidate.
    // Ambiguous identical devices keep their automatic layout.
    fun mapping(context: Context, name: String, vendor: Int, product: Int): String {
        val candidates = devices().filter { it.vendorId == vendor && it.productId == product }
        val device = candidates.singleOrNull { it.name == name } ?: candidates.singleOrNull() ?: return ""
        return load(context, device).entries.joinToString(",") { "${it.key}:${it.value}" }
    }

    // SDL 3.4 Android raw joystick button indices (SDL_sysjoystick.c), not
    // Android key codes. Generic BUTTON_1..16 are useful on handheld firmware.
    fun button(key: Int, scanCode: Int = 0): Int? = when (key) {
        KeyEvent.KEYCODE_BUTTON_A, KeyEvent.KEYCODE_DPAD_CENTER -> 0
        KeyEvent.KEYCODE_BUTTON_B -> 1
        KeyEvent.KEYCODE_BUTTON_X -> 2
        KeyEvent.KEYCODE_BUTTON_Y -> 3
        KeyEvent.KEYCODE_BACK, KeyEvent.KEYCODE_BUTTON_SELECT -> 4
        KeyEvent.KEYCODE_BUTTON_MODE -> 5
        KeyEvent.KEYCODE_MENU, KeyEvent.KEYCODE_BUTTON_START -> 6
        KeyEvent.KEYCODE_BUTTON_THUMBL -> 7
        KeyEvent.KEYCODE_BUTTON_THUMBR -> 8
        KeyEvent.KEYCODE_BUTTON_L1 -> 9
        KeyEvent.KEYCODE_BUTTON_R1 -> 10
        KeyEvent.KEYCODE_DPAD_UP -> 11
        KeyEvent.KEYCODE_DPAD_DOWN -> 12
        KeyEvent.KEYCODE_DPAD_LEFT -> 13
        KeyEvent.KEYCODE_DPAD_RIGHT -> 14
        KeyEvent.KEYCODE_BUTTON_L2, KeyEvent.KEYCODE_MEDIA_RECORD -> 15
        KeyEvent.KEYCODE_BUTTON_R2 -> 16
        KeyEvent.KEYCODE_BUTTON_C -> 17
        KeyEvent.KEYCODE_BUTTON_Z -> 18
        in KeyEvent.KEYCODE_BUTTON_1..KeyEvent.KEYCODE_BUTTON_16 -> 20 + key - KeyEvent.KEYCODE_BUTTON_1
        else -> when (scanCode) {
            // Linux BTN_DPAD_*: SDL also supports left Joy-Con scan codes.
            0x220 -> 11
            0x221 -> 12
            0x222 -> 13
            0x223 -> 14
            else -> null
        }
    }

    // Keep the same order as SDLControllerManager.RangeComparator: Z follows
    // RX/RY, GAS and BRAKE swap. Hats are emitted as D-pad buttons by SDL.
    fun axisOrder(axis: Int): Int {
        val swapped = when (axis) {
            MotionEvent.AXIS_GAS -> MotionEvent.AXIS_BRAKE
            MotionEvent.AXIS_BRAKE -> MotionEvent.AXIS_GAS
            else -> axis
        }
        return when {
            swapped == MotionEvent.AXIS_Z -> MotionEvent.AXIS_RZ - 1
            swapped > MotionEvent.AXIS_Z && swapped < MotionEvent.AXIS_RZ -> swapped - 1
            else -> swapped
        }
    }
    fun axes(device: InputDevice) = device.motionRanges.filter {
        (it.source and InputDevice.SOURCE_CLASS_JOYSTICK) != 0 &&
            it.axis != MotionEvent.AXIS_HAT_X && it.axis != MotionEvent.AXIS_HAT_Y
    }.distinctBy { it.axis }.sortedBy { axisOrder(it.axis) }
}
