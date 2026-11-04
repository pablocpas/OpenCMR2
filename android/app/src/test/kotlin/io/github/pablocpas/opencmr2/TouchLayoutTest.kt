package io.github.pablocpas.opencmr2

import org.junit.Assert.*
import org.junit.Test

class TouchLayoutTest {
    @Test fun defaultsRetainOriginalDrawingSizeAndOutsideCorners() {
        val left = TouchLayoutGeometry.place(2400, 1080, true, TouchInsets(), TouchPadSettings())
        val right = TouchLayoutGeometry.place(2400, 1080, false, TouchInsets(), TouchPadSettings())
        assertEquals(594, left.size)
        assertEquals(0, left.left)
        assertEquals(486, left.top)
        assertEquals(1806, right.left)
        assertEquals(left.top, right.top)
        // The original square now lives inside a half-screen touch view.
        assertEquals(1200, left.viewWidth)
        assertEquals(1200, right.viewWidth)
    }

    @Test fun independentEditsDoNotChangeTheOtherSide() {
        val original = TouchLayoutSettings()
        val changed = original.withPad(true, original.left.copy(scale = 1.6f, vertical = 0.3f))
        assertEquals(original.right, changed.right)
        assertEquals(1.6f, changed.left.scale, 0f)
        assertEquals(original, TouchLayoutSettings.fromValues(original.values()))
        assertEquals(changed, TouchLayoutSettings.fromValues(changed.values()))
    }

    @Test fun cutoutOnlyMovesTheAffectedSide() {
        val plain = TouchLayoutGeometry.place(2608, 1200, false, TouchInsets(), TouchPadSettings())
        val cutout = TouchInsets(left = 100, bottom = 24)
        val left = TouchLayoutGeometry.place(2608, 1200, true, cutout, TouchPadSettings())
        val right = TouchLayoutGeometry.place(2608, 1200, false, cutout, TouchPadSettings())
        assertEquals(100, left.left)
        assertEquals(plain.left, right.left)
        assertEquals(1200 - 24, left.top + left.size)
    }

    @Test fun groupsStayInTheirHalvesAcrossScreensSizesAndOffsets() {
        for ((width, height) in listOf(2608 to 1200, 1920 to 1080, 1280 to 800, 1024 to 768, 801 to 600, 640 to 360)) {
            for (scale in listOf(0.8f, 1f, 1.8f)) for (x in listOf(0f, 0.5f, 1f)) for (y in listOf(0f, 0.5f, 1f)) {
                for (isLeft in listOf(true, false)) {
                    val insets = TouchInsets(70, 12, 25, 30)
                    val p = TouchLayoutGeometry.place(width, height, isLeft, insets, TouchPadSettings(scale, x, y))
                    assertTrue(p.left >= maxOf(p.viewLeft, insets.left))
                    assertTrue(p.left + p.size <= minOf(p.viewLeft + p.viewWidth, width - insets.right))
                    assertTrue(p.top >= insets.top)
                    assertTrue(p.top + p.size <= height - insets.bottom)
                    assertEquals(p.viewWidth, p.spacingLeft + p.size + p.spacingRight)
                }
            }
        }
    }

    @Test fun proportionalSizesAreConsistentAcrossPixelDensities() {
        val small = TouchLayoutGeometry.place(800, 400, true, TouchInsets(), TouchPadSettings())
        val large = TouchLayoutGeometry.place(2400, 1200, true, TouchInsets(), TouchPadSettings())
        assertEquals(small.size * 3, large.size)
        assertEquals(small.top * 3, large.top)
    }

    @Test fun dragDirectionsMatchTheFingerOnBothSides() {
        for (isLeft in listOf(true, false)) {
            val settings = TouchPadSettings(horizontal = 0.5f, vertical = 0.5f)
            val before = TouchLayoutGeometry.place(2400, 1080, isLeft, TouchInsets(), settings)
            val moved = settings.transform(isLeft, before, 60f, -40f, 1f, 0f)
            val after = TouchLayoutGeometry.place(2400, 1080, isLeft, TouchInsets(), moved)
            assertEquals(before.left + 60, after.left)
            assertEquals(before.top - 40, after.top)
        }
    }

    @Test fun oversizeAndMalformedSettingsCannotEscapeTheViewport() {
        val invalid = TouchPadSettings(Float.NaN, Float.POSITIVE_INFINITY, -100f, 500f, -5f).normalized()
        assertEquals(1f, invalid.scale, 0f)
        assertEquals(0f, invalid.horizontal, 0f)
        assertEquals(0f, invalid.vertical, 0f)
        assertEquals(45f, invalid.rotation, 0f)
        assertEquals(0.2f, invalid.opacity, 0f)
        val p = TouchLayoutGeometry.place(640, 360, false, TouchInsets(), TouchPadSettings(scale = 200f))
        assertTrue(p.size <= p.viewWidth)
        assertTrue(p.top >= 0)
    }

    @Test fun noSpaceAndZeroTravelDoNotDivideByZero() {
        val p = TouchLayoutGeometry.place(0, 0, true, TouchInsets(500, 500, 500, 500), TouchPadSettings())
        assertEquals(0, p.size)
        val moved = TouchPadSettings().transform(true, p, 500f, 500f, 1f, 0f)
        assertEquals(TouchPadSettings(), moved)
    }
}
