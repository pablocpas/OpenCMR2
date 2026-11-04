package io.github.pablocpas.opencmr2

import android.content.Intent
import android.hardware.input.InputManager
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.view.InputDevice
import android.view.KeyEvent
import android.view.MotionEvent
import android.view.Window
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import kotlin.math.abs

/** Runs without SDL or the game: touch always works even with an unmapped pad. */
class ControllerActivity : ComponentActivity(), InputManager.InputDeviceListener {
    private val inputManager by lazy { getSystemService(INPUT_SERVICE) as InputManager }
    private var devices by mutableStateOf(emptyList<InputDevice>())
    private var selected by mutableStateOf<Int?>(null)
    private var bindings by mutableStateOf(emptyMap<String, String>())
    private var waiting by mutableStateOf<String?>(null)
    private var lastInput by mutableStateOf("")
    private var touch by mutableStateOf(false)
    private var touchOnly by mutableStateOf(false)
    private var touchScale by mutableStateOf(1f)
    private var touchMarginX by mutableStateOf(0f)
    private var touchMarginY by mutableStateOf(0f)
    private var touchOpacity by mutableStateOf(1f)
    private var touchCutout by mutableStateOf(true)
    private var detailedTouchLayout by mutableStateOf(false)
    private val lastAxes = mutableMapOf<Pair<Int, Int>, Float>()
    private var baseline = emptyMap<Pair<Int, Int>, Float>()
    private val device get() = devices.firstOrNull { it.id == selected }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        // Intercept through the framework Window callback before Compose can
        // use a captured button for focus/navigation.
        val callback = window.callback
        window.callback = object : Window.Callback by callback {
            override fun dispatchKeyEvent(event: KeyEvent): Boolean =
                handleControllerKey(event) || callback.dispatchKeyEvent(event)
        }
        enableEdgeToEdge()
        touch = ControllerProfiles.showTouch(this)
        touchOnly = ControllerProfiles.touchOnly(this)
        touchScale = ControllerProfiles.touchScale(this)
        touchMarginX = ControllerProfiles.touchMarginX(this)
        touchMarginY = ControllerProfiles.touchMarginY(this)
        touchOpacity = ControllerProfiles.touchOpacity(this)
        touchCutout = ControllerProfiles.touchAvoidCutout(this)
        detailedTouchLayout = ControllerProfiles.detailedTouchLayout(this)
        refresh()
        setContent {
            SetupTheme {
                Box(Modifier.fillMaxSize()) {
                    Column(Modifier.fillMaxSize().safeDrawingPadding().verticalScroll(rememberScrollState()).padding(24.dp),
                        verticalArrangement = Arrangement.spacedBy(12.dp)) {
                        Text(stringResource(R.string.controllers_title), style = MaterialTheme.typography.headlineMedium)
                        Text(stringResource(R.string.controllers_intro))
                        Row(verticalAlignment = Alignment.CenterVertically) {
                            Switch(checked = touchOnly, onCheckedChange = { touchOnly = it; ControllerProfiles.touchOnly(this@ControllerActivity, it) })
                            Text(stringResource(R.string.controllers_touch_only), Modifier.padding(start = 12.dp))
                        }
                        Text(stringResource(R.string.touch_layout_method), style = MaterialTheme.typography.titleMedium)
                        Row(verticalAlignment = Alignment.CenterVertically) {
                            RadioButton(selected = !detailedTouchLayout, onClick = {
                                detailedTouchLayout = false
                                ControllerProfiles.detailedTouchLayout(this@ControllerActivity, false)
                            })
                            Text(stringResource(R.string.touch_layout_quick))
                            RadioButton(selected = detailedTouchLayout, onClick = {
                                detailedTouchLayout = true
                                ControllerProfiles.detailedTouchLayout(this@ControllerActivity, true)
                            })
                            Text(stringResource(R.string.touch_layout_editor))
                        }
                        Row(verticalAlignment = Alignment.CenterVertically) {
                            Switch(checked = touch, onCheckedChange = { touch = it; ControllerProfiles.showTouch(this@ControllerActivity, it) })
                            Text(stringResource(R.string.controllers_touch), Modifier.padding(start = 12.dp))
                        }
                        Text(stringResource(R.string.touch_layout_title),
                            style = MaterialTheme.typography.titleMedium, modifier = Modifier.padding(top = 12.dp))
                        SettingSlider(R.string.touch_size, touchScale, 0.8f..1.6f, { "%.0f%%".format(it * 100) }) {
                            touchScale = it; ControllerProfiles.touchScale(this@ControllerActivity, it)
                        }
                        SettingSlider(R.string.touch_edge, touchMarginX, 0f..1f, { "%.0f%%".format(it * 100) }) {
                            touchMarginX = it; ControllerProfiles.touchMarginX(this@ControllerActivity, it)
                        }
                        SettingSlider(R.string.touch_height, touchMarginY, 0f..1f, { "%.0f%%".format(it * 100) }) {
                            touchMarginY = it; ControllerProfiles.touchMarginY(this@ControllerActivity, it)
                        }
                        SettingSlider(R.string.touch_opacity, touchOpacity, 0.2f..1f, { "%.0f%%".format(it * 100) }) {
                            touchOpacity = it; ControllerProfiles.touchOpacity(this@ControllerActivity, it)
                        }
                        Row(verticalAlignment = Alignment.CenterVertically) {
                            Switch(checked = touchCutout, onCheckedChange = { touchCutout = it; ControllerProfiles.touchAvoidCutout(this@ControllerActivity, it) })
                            Text(stringResource(R.string.touch_cutout), Modifier.padding(start = 12.dp))
                        }
                        TextButton(onClick = {
                            ControllerProfiles.resetTouchLayout(this@ControllerActivity)
                            touchScale = 1f; touchMarginX = 0f; touchMarginY = 0f; touchOpacity = 1f; touchCutout = true
                        }) { Text(stringResource(R.string.touch_reset)) }
                        OutlinedButton(onClick = {
                            detailedTouchLayout = true
                            ControllerProfiles.detailedTouchLayout(this@ControllerActivity, true)
                            startActivity(Intent(this@ControllerActivity, TouchLayoutActivity::class.java))
                        }) { Text(stringResource(R.string.touch_edit)) }
                        if (devices.isEmpty()) Text(stringResource(R.string.controllers_connect))
                        for (pad in devices) {
                            OutlinedButton(onClick = { selected = pad.id; waiting = null; reload() }, modifier = Modifier.fillMaxWidth()) {
                                Text((if (selected == pad.id) "✓ " else "") + pad.name)
                            }
                        }
                        if (device != null) {
                            Text(lastInput.ifEmpty { getString(R.string.controllers_test) })
                            for ((target, label) in ACTIONS) {
                                OutlinedButton(onClick = { baseline = lastAxes.toMap(); waiting = target }, modifier = Modifier.fillMaxWidth()) {
                                    Text(stringResource(label) + " · " + (bindings[target]?.ifEmpty { "—" }
                                        ?: stringResource(R.string.controllers_automatic)))
                                }
                            }
                            TextButton(onClick = { device?.let { ControllerProfiles.reset(this@ControllerActivity, it) }; reload() }) {
                                Text(stringResource(R.string.controllers_reset))
                            }
                        }
                        Button(onClick = { finish() }) { Text(stringResource(R.string.controllers_done)) }
                    }
                    waiting?.let { target ->
                        // Stay in the activity window: an Android Dialog would
                        // intercept the very key/motion events being captured.
                        Surface(modifier = Modifier.fillMaxSize(), color = MaterialTheme.colorScheme.background) {
                            Column(Modifier.fillMaxSize().safeDrawingPadding().padding(24.dp),
                                verticalArrangement = Arrangement.Center, horizontalAlignment = Alignment.CenterHorizontally) {
                                Text(stringResource(ACTIONS.first { it.first == target }.second), style = MaterialTheme.typography.headlineSmall)
                                Text(stringResource(if (target == "leftx" || target == "lefty")
                                    R.string.controllers_move_stick else R.string.controllers_press), Modifier.padding(vertical = 16.dp))
                                Text(lastInput)
                                TextButton(onClick = { waiting = null }) { Text(stringResource(android.R.string.cancel)) }
                            }
                        }
                    }
                }
            }
        }
    }

    private fun reload() { bindings = device?.let { ControllerProfiles.load(this, it) }.orEmpty() }
    private fun refresh() {
        devices = ControllerProfiles.devices()
        if (devices.none { it.id == selected }) { selected = devices.firstOrNull()?.id; waiting = null }
        reload()
    }
    private fun capture(source: String) {
        val target = waiting ?: return
        val pad = device ?: return
        ControllerProfiles.bind(this, pad, target, source)
        waiting = null
        reload()
    }

    private fun handleControllerKey(event: KeyEvent): Boolean {
        if (event.deviceId != selected) return false
        lastInput = KeyEvent.keyCodeToString(event.keyCode) + if (event.action == KeyEvent.ACTION_DOWN) " ↓" else " ↑"
        if (waiting == null) return false
        // Capture on release so the release cannot click the newly focused UI.
        if (event.action == KeyEvent.ACTION_UP) {
            val button = ControllerProfiles.button(event.keyCode, event.scanCode)
            if (button == null) lastInput = getString(R.string.controllers_unsupported, event.keyCode)
            else if (waiting != "leftx" && waiting != "lefty") capture("b$button")
        }
        return true
    }

    override fun dispatchGenericMotionEvent(event: MotionEvent): Boolean {
        val pad = device
        if (pad == null || event.deviceId != pad.id || !event.isFromSource(InputDevice.SOURCE_JOYSTICK))
            return super.dispatchGenericMotionEvent(event)
        val axes = ControllerProfiles.axes(pad)
        val target = waiting
        for (range in pad.motionRanges.filter { (it.source and InputDevice.SOURCE_CLASS_JOYSTICK) != 0 }.distinctBy { it.axis }) {
            val value = event.getAxisValue(range.axis)
            val key = pad.id to range.axis
            val rest = baseline[key] ?: if (range.min >= 0f && range.axis != MotionEvent.AXIS_HAT_X && range.axis != MotionEvent.AXIS_HAT_Y) range.min else 0f
            lastAxes[key] = value
            if (abs(value - rest) < maxOf(range.flat * 2, range.range * 0.3f)) continue
            lastInput = "${MotionEvent.axisToString(range.axis)}: %.2f".format(value)
            if (target == null) continue
            val positive = value > rest
            val hat = when (range.axis) {
                MotionEvent.AXIS_HAT_X -> if (positive) 14 else 13
                MotionEvent.AXIS_HAT_Y -> if (positive) 12 else 11
                else -> null
            }
            val stick = target == "leftx" || target == "lefty"
            if (hat != null) {
                if (!stick) capture("b$hat")
            } else {
                val index = axes.indexOfFirst { it.axis == range.axis }
                if (index < 0) continue
                val fullAxis = stick || target == "lefttrigger" || target == "righttrigger"
                capture(if (fullAxis) "a$index" + if (positive) "" else "~" else (if (positive) "+a" else "-a") + index)
            }
            break
        }
        return true
    }

    override fun onResume() {
        super.onResume()
        inputManager.registerInputDeviceListener(this, Handler(Looper.getMainLooper()))
        refresh()
    }
    override fun onPause() {
        inputManager.unregisterInputDeviceListener(this)
        waiting = null
        super.onPause()
    }
    override fun onInputDeviceAdded(deviceId: Int) = refresh()
    override fun onInputDeviceRemoved(deviceId: Int) = refresh()
    override fun onInputDeviceChanged(deviceId: Int) = refresh()

    private companion object {
        val ACTIONS = listOf(
            "a" to R.string.controllers_a, "b" to R.string.controllers_b,
            "x" to R.string.controllers_x, "y" to R.string.controllers_y,
            "leftshoulder" to R.string.controllers_lb, "rightshoulder" to R.string.controllers_rb,
            "start" to R.string.touch_pause, "back" to R.string.controllers_select,
            "dpup" to R.string.controllers_up, "dpdown" to R.string.controllers_down,
            "dpleft" to R.string.controllers_left, "dpright" to R.string.controllers_right,
            "leftx" to R.string.controllers_stick_right, "lefty" to R.string.controllers_stick_down,
            "righttrigger" to R.string.touch_accelerate, "lefttrigger" to R.string.touch_brake,
        )
    }
}

/** A labelled settings slider; the label shows the current value as text. */
@Composable
private fun SettingSlider(label: Int, value: Float, range: ClosedFloatingPointRange<Float>,
    display: (Float) -> String, onChange: (Float) -> Unit) {
    Column {
        Text(stringResource(label) + " · " + display(value))
        Slider(value = value, onValueChange = onChange, valueRange = range)
    }
}
