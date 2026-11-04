package io.github.pablocpas.opencmr2

import android.os.Bundle
import android.view.KeyEvent
import android.widget.FrameLayout
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.background
import androidx.compose.foundation.gestures.awaitEachGesture
import androidx.compose.foundation.gestures.awaitFirstDown
import androidx.compose.foundation.gestures.calculatePan
import androidx.compose.foundation.gestures.calculateRotation
import androidx.compose.foundation.gestures.calculateZoom
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.runtime.saveable.Saver
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.platform.LocalConfiguration
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import androidx.compose.ui.viewinterop.AndroidView
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.WindowInsetsControllerCompat
import java.util.Locale

/** The real pad views, with an isolated input sink. Editing never loads the game
 * or sends native key events. Only Save persists the draft, including Reset. */
class TouchLayoutActivity : ComponentActivity() {
    private var preview: TouchControls? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        val bars = WindowCompat.getInsetsController(window, window.decorView)
        bars.systemBarsBehavior = WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
        bars.hide(WindowInsetsCompat.Type.systemBars())
        setContent {
            SetupTheme {
                val locale = LocalConfiguration.current.locales[0]
                var draft by rememberSaveable(stateSaver = layoutSaver) { mutableStateOf(TouchLayoutStore.load(this)) }
                var left by rememberSaveable { mutableStateOf(true) }
                var testing by rememberSaveable { mutableStateOf(false) }
                var expanded by rememberSaveable { mutableStateOf(false) }
                var held by remember { mutableStateOf(emptySet<Int>()) }

                BoxWithConstraints(Modifier.fillMaxSize().background(Color(0xFF17202B))) {
                    val toolsHeight = (maxHeight - 128.dp).coerceAtLeast(80.dp)
                    AndroidView(
                        factory = { context ->
                            FrameLayout(context).also { parent ->
                                preview = TouchControls(this@TouchLayoutActivity, parent, preview = true) { key, down ->
                                    held = if (down) held + key else held - key
                                }.also { it.applyLayout(draft) }
                            }
                        },
                        update = { preview?.applyLayout(draft) },
                        modifier = Modifier.fillMaxSize(),
                    )

                    if (!testing) Box(Modifier.fillMaxSize().pointerInput(Unit) {
                        awaitEachGesture {
                            val first = awaitFirstDown()
                            val isLeft = first.position.x < size.width / 2f
                            left = isLeft
                            first.consume()
                            val before = draft
                            var completed = false
                            try {
                                do {
                                    val event = awaitPointerEvent()
                                    val place = preview?.placement(isLeft)
                                    if (place != null) {
                                        val pan = event.calculatePan()
                                        val value = draft.pad(isLeft).transform(isLeft, place,
                                            pan.x, pan.y, event.calculateZoom(), event.calculateRotation())
                                        draft = draft.withPad(isLeft, value)
                                    }
                                    event.changes.forEach { it.consume() }
                                    val pressed = event.changes.any { it.pressed }
                                    if (!pressed) completed = true
                                } while (pressed)
                            } finally {
                                // If Android cancels a drag, keep the pre-gesture draft.
                                if (!completed) draft = before
                            }
                        }
                    })

                    // Compact, collapsible tools keep the actual controls visible.
                    Column(Modifier.align(Alignment.TopCenter).safeDrawingPadding().widthIn(max = 580.dp),
                        horizontalAlignment = Alignment.CenterHorizontally) {
                        Surface(color = MaterialTheme.colorScheme.surface.copy(alpha = 0.95f)) {
                            Column(horizontalAlignment = Alignment.CenterHorizontally) {
                                Row(Modifier.fillMaxWidth(), verticalAlignment = Alignment.CenterVertically,
                                    horizontalArrangement = Arrangement.SpaceBetween) {
                                    TextButton(onClick = { finish() }) { Text(stringResource(android.R.string.cancel)) }
                                    Text(stringResource(R.string.touch_edit), style = MaterialTheme.typography.titleMedium)
                                    TextButton(onClick = {
                                        preview?.releaseAll()
                                        TouchLayoutStore.save(this@TouchLayoutActivity, draft)
                                        finish()
                                    }) { Text(stringResource(R.string.touch_save)) }
                                }
                                Row(verticalAlignment = Alignment.CenterVertically) {
                                    TextButton(onClick = {
                                        preview?.releaseAll()
                                        held = emptySet()
                                        testing = !testing
                                        expanded = false
                                    }) { Text(stringResource(if (testing) R.string.touch_edit_mode else R.string.touch_test)) }
                                    TextButton(onClick = {
                                        preview?.releaseAll()
                                        testing = false
                                        expanded = !expanded
                                    }) { Text(stringResource(R.string.touch_adjustments)) }
                                    TextButton(onClick = {
                                        preview?.releaseAll()
                                        draft = TouchLayoutSettings()
                                    }) { Text(stringResource(R.string.touch_reset)) }
                                }
                            }
                        }
                        if (expanded) Surface(color = MaterialTheme.colorScheme.surface.copy(alpha = 0.97f)) {
                            Column(Modifier.widthIn(max = 360.dp).heightIn(max = toolsHeight)
                                .verticalScroll(rememberScrollState()).padding(horizontal = 16.dp)) {
                                Row {
                                    FilterChip(selected = left, onClick = { left = true },
                                        label = { Text(stringResource(R.string.touch_left)) })
                                    Spacer(Modifier.width(12.dp))
                                    FilterChip(selected = !left, onClick = { left = false },
                                        label = { Text(stringResource(R.string.touch_right)) })
                                }
                                val pad = draft.pad(left)
                                LayoutSlider(R.string.touch_size, pad.scale, 0.8f..1.8f, percent(pad.scale, locale)) {
                                    draft = draft.withPad(left, pad.copy(scale = it))
                                }
                                LayoutSlider(R.string.touch_horizontal, pad.horizontal, 0f..1f, percent(pad.horizontal, locale)) {
                                    draft = draft.withPad(left, pad.copy(horizontal = it))
                                }
                                LayoutSlider(R.string.touch_vertical, pad.vertical, 0f..1f, percent(pad.vertical, locale)) {
                                    draft = draft.withPad(left, pad.copy(vertical = it))
                                }
                                LayoutSlider(R.string.touch_rotation, pad.rotation, -45f..45f,
                                    String.format(locale, "%.0f°", pad.rotation)) {
                                    draft = draft.withPad(left, pad.copy(rotation = it))
                                }
                                LayoutSlider(R.string.touch_opacity, pad.opacity, 0.2f..1f, percent(pad.opacity, locale)) {
                                    draft = draft.withPad(left, pad.copy(opacity = it))
                                }
                                Row(verticalAlignment = Alignment.CenterVertically) {
                                    Switch(checked = draft.avoidCutout, onCheckedChange = { draft = draft.copy(avoidCutout = it) })
                                    Text(stringResource(R.string.touch_cutout), Modifier.padding(start = 12.dp))
                                }
                            }
                        }
                    }
                    if (!expanded) Surface(Modifier.align(Alignment.Center).widthIn(max = 380.dp),
                        color = MaterialTheme.colorScheme.surface.copy(alpha = 0.85f)) {
                        Text(if (testing) {
                            if (held.isEmpty()) stringResource(R.string.touch_test_hint)
                            else held.sorted().joinToString(" · ") { keyLabel(it) }
                        } else stringResource(R.string.touch_drag_hint), Modifier.padding(12.dp),
                            style = MaterialTheme.typography.bodyMedium)
                    }
                    // Hardware Back uses the activity default: discard the draft.
                }
            }
        }
    }

    private fun keyLabel(key: Int): String = getString(when (key) {
        KeyEvent.KEYCODE_DPAD_LEFT -> R.string.controllers_left
        KeyEvent.KEYCODE_DPAD_RIGHT -> R.string.controllers_right
        KeyEvent.KEYCODE_DPAD_UP -> R.string.touch_accelerate
        KeyEvent.KEYCODE_DPAD_DOWN -> R.string.touch_brake
        KeyEvent.KEYCODE_SPACE -> R.string.touch_handbrake
        KeyEvent.KEYCODE_LEFT_BRACKET -> R.string.touch_gear_down
        KeyEvent.KEYCODE_RIGHT_BRACKET -> R.string.touch_gear_up
        KeyEvent.KEYCODE_C -> R.string.touch_camera
        KeyEvent.KEYCODE_ESCAPE -> R.string.touch_back
        else -> R.string.touch_ok
    })

    override fun onPause() {
        preview?.releaseAll()
        super.onPause()
    }

    override fun onDestroy() {
        preview?.close()
        preview = null
        super.onDestroy()
    }

    private companion object {
        val layoutSaver = Saver<TouchLayoutSettings, FloatArray>(save = { it.values() }, restore = { TouchLayoutSettings.fromValues(it) })
    }
}

@Composable
private fun LayoutSlider(label: Int, value: Float, range: ClosedFloatingPointRange<Float>,
                         display: String, onChange: (Float) -> Unit) {
    Text(stringResource(label) + " · " + display)
    Slider(value = value, onValueChange = onChange, valueRange = range)
}

private fun percent(value: Float, locale: Locale) = String.format(locale, "%.0f%%", value * 100)
