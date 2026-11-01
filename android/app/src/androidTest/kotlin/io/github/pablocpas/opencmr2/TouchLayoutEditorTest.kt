package io.github.pablocpas.opencmr2

import android.content.Context
import android.graphics.Rect
import android.graphics.Bitmap
import android.os.SystemClock
import android.view.MotionEvent
import android.view.View
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.test.*
import androidx.compose.ui.test.junit4.createEmptyComposeRule
import androidx.lifecycle.Lifecycle
import androidx.test.core.app.ActivityScenario
import androidx.test.core.app.ApplicationProvider
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import com.swordfish.radialgamepad.library.RadialGamePad
import org.junit.After
import org.junit.Assert.*
import org.junit.Before
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith

@RunWith(AndroidJUnit4::class)
class TouchLayoutEditorTest {
    @get:Rule val compose = createEmptyComposeRule()
    private val context get() = ApplicationProvider.getApplicationContext<Context>()
    private lateinit var original: TouchLayoutSettings

    @Before fun rememberSettings() {
        original = TouchLayoutStore.load(context)
        TouchLayoutStore.save(context, TouchLayoutSettings())
    }

    @After fun restoreSettings() { TouchLayoutStore.save(context, original) }

    @Test fun oneLayoutWithArrowsConfirmAndBackOnTheLeft() {
        ActivityScenario.launch(TouchLayoutActivity::class.java).use { scenario ->
            compose.waitForIdle()
            scenario.onActivity { activity ->
                assertEquals(View.VISIBLE, pad(activity, true).visibility)
                assertEquals(View.VISIBLE, pad(activity, false).visibility)
                val left = buttonBounds(pad(activity, true))
                assertEquals(setOf(R.string.controllers_left, R.string.controllers_right, R.string.touch_ok,
                    R.string.touch_back).map(activity::getString).toSet(), left.keys)
            }
        }
    }

    @Test fun defaultLayoutPrioritizesDrivingAndKeepsConfirmOnTheLeft() {
        ActivityScenario.launch(TouchLayoutActivity::class.java).use { scenario ->
            compose.waitForIdle()
            screenshot("touch-default-layout.png")
            scenario.onActivity { activity ->
                val left = buttonBounds(pad(activity, true))
                val right = buttonBounds(pad(activity, false))
                assertTrue(left.containsKey(activity.getString(R.string.touch_ok)))
                assertTrue(left.containsKey(activity.getString(R.string.touch_back)))
                assertFalse(right.containsKey(activity.getString(R.string.touch_ok)))
                val accelerate = right.getValue(activity.getString(R.string.touch_accelerate))
                val brake = right.getValue(activity.getString(R.string.touch_brake))
                val handbrake = right.getValue(activity.getString(R.string.touch_handbrake))
                val gear = right.getValue(activity.getString(R.string.touch_gear_up))
                assertTrue(accelerate.width() > gear.width())
                assertTrue(kotlin.math.abs(accelerate.width() - brake.width()) <= 2)
                assertTrue(accelerate.centerX() > brake.centerX())
                assertTrue(kotlin.math.abs(accelerate.centerY() - brake.centerY()) <= 2)
                assertTrue(brake.centerY() > handbrake.centerY())
            }
        }
    }

    @Test fun dragIsIndependentSurvivesRecreationAndPersistsOnlyOnSave() {
        ActivityScenario.launch(TouchLayoutActivity::class.java).use { scenario ->
            compose.waitForIdle()
            var initialTop = 0
            scenario.onActivity { initialTop = pad(it, true).spacingTop }
            dragLeft()
            scenario.onActivity {
                assertTrue(pad(it, true).spacingTop < initialTop)
                assertEquals(TouchLayoutSettings(), TouchLayoutStore.load(it))
            }
            scenario.recreate()
            compose.waitForIdle()
            // Pixel positions can change with transient system-bar insets.
            // Saving after recreation must retain the edited relative layout.
            button(R.string.touch_save)
            val saved = TouchLayoutStore.load(context)
            assertTrue(saved.left.horizontal > 0f)
            assertTrue(saved.left.vertical > 0f)
            assertEquals(TouchPadSettings(), saved.right)
        }
    }

    @Test fun resetAndCancelDoNotOverwriteSavedSettings() {
        val custom = TouchLayoutSettings(left = TouchPadSettings(scale = 1.5f, horizontal = 0.3f))
        TouchLayoutStore.save(context, custom)
        ActivityScenario.launch(TouchLayoutActivity::class.java).use {
            compose.waitForIdle()
            button(R.string.touch_reset)
            button(android.R.string.cancel)
            assertEquals(custom, TouchLayoutStore.load(context))
        }
        ActivityScenario.launch(TouchLayoutActivity::class.java).use {
            compose.waitForIdle()
            button(R.string.touch_reset)
            button(R.string.touch_save)
            assertEquals(TouchLayoutSettings(), TouchLayoutStore.load(context))
        }
    }

    @Test fun realPadAcceptsSimultaneousButtonsAndReleasesThemOnPause() {
        ActivityScenario.launch(TouchLayoutActivity::class.java).use { scenario ->
            compose.waitForIdle()
            button(R.string.touch_test)
            compose.waitForIdle()
            scenario.onActivity { activity ->
                val pad = pad(activity, false)
                val accelerate = buttonCenter(pad, activity.getString(R.string.touch_accelerate))
                val handbrake = buttonCenter(pad, activity.getString(R.string.touch_handbrake))
                val now = SystemClock.uptimeMillis()
                send(pad, now, MotionEvent.ACTION_DOWN, listOf(accelerate))
                send(pad, now, MotionEvent.ACTION_POINTER_DOWN or (1 shl MotionEvent.ACTION_POINTER_INDEX_SHIFT),
                    listOf(accelerate, handbrake))
            }
            val accelerate = context.getString(R.string.touch_accelerate)
            val handbrake = context.getString(R.string.touch_handbrake)
            compose.waitUntil(5_000) {
                compose.onAllNodes(hasText(accelerate, substring = true) and hasText(handbrake, substring = true))
                    .fetchSemanticsNodes().isNotEmpty()
            }
            scenario.moveToState(Lifecycle.State.CREATED)
            scenario.moveToState(Lifecycle.State.RESUMED)
            compose.onNodeWithText(context.getString(R.string.touch_test_hint)).assertIsDisplayed()
        }
    }

    @Test fun editingGesturesNeverPressThePreviewButtons() {
        ActivityScenario.launch(TouchLayoutActivity::class.java).use {
            compose.waitForIdle()
            dragLeft()
            button(R.string.touch_test)
            compose.onNodeWithText(context.getString(R.string.touch_test_hint)).assertIsDisplayed()
        }
    }

    private fun dragLeft() {
        compose.onRoot().performTouchInput {
            swipe(Offset(width * 0.15f, height * 0.85f), Offset(width * 0.25f, height * 0.65f), 400)
        }
        compose.waitForIdle()
    }

    private fun button(string: Int) = compose.onNodeWithText(context.getString(string)).performClick()

    private fun pad(activity: TouchLayoutActivity, left: Boolean): RadialGamePad =
        activity.window.decorView.findViewWithTag(if (left) "touch-left" else "touch-right")

    private fun screenshot(name: String) {
        InstrumentationRegistry.getInstrumentation().uiAutomation.takeScreenshot()?.let { bitmap ->
            context.cacheDir.resolve(name).outputStream().use {
                bitmap.compress(Bitmap.CompressFormat.PNG, 100, it)
            }
            bitmap.recycle()
        }
    }

    private fun buttonCenter(pad: RadialGamePad, label: String): Offset {
        val rect = buttonBounds(pad)[label] ?: error("Missing pad button: $label")
        return Offset(rect.exactCenterX(), rect.exactCenterY())
    }

    private fun buttonBounds(pad: RadialGamePad): Map<String, Rect> {
        val provider = requireNotNull(pad.accessibilityNodeProvider)
        val host = requireNotNull(provider.createAccessibilityNodeInfo(View.NO_ID))
        return (0 until host.childCount).associate { id ->
            val node = requireNotNull(provider.createAccessibilityNodeInfo(id))
            val rect = Rect()
            node.getBoundsInParent(rect)
            node.contentDescription.toString() to rect
        }
    }

    private fun send(view: View, down: Long, action: Int, positions: List<Offset>) {
        val properties = positions.indices.map { id -> MotionEvent.PointerProperties().apply {
            this.id = id
            toolType = MotionEvent.TOOL_TYPE_FINGER
        } }.toTypedArray()
        val coords = positions.map { position -> MotionEvent.PointerCoords().apply {
            x = position.x; y = position.y; pressure = 1f; size = 1f
        } }.toTypedArray()
        val event = MotionEvent.obtain(down, SystemClock.uptimeMillis(), action, positions.size,
            properties, coords, 0, 0, 1f, 1f, 0, 0, android.view.InputDevice.SOURCE_TOUCHSCREEN, 0)
        view.dispatchTouchEvent(event)
        event.recycle()
    }
}
