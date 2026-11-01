package io.github.pablocpas.opencmr2

import kotlin.math.roundToInt

/** Positions are fractions of the space a pad can move through, measured from
 * its outside/bottom edges. Sizes retain the original layout at scale 1. */
data class TouchPadSettings(
    val scale: Float = 1f,
    val horizontal: Float = 0f,
    val vertical: Float = 0f,
    val rotation: Float = 0f,
    val opacity: Float = 1f,
) {
    fun normalized() = TouchPadSettings(
        scale.bounded(0.8f, 1.8f, 1f),
        horizontal.bounded(0f, 1f, 0f),
        vertical.bounded(0f, 1f, 0f),
        rotation.bounded(-45f, 45f, 0f),
        opacity.bounded(0.2f, 1f, 1f),
    )

    fun transform(left: Boolean, placement: TouchPadPlacement, dx: Float, dy: Float,
                  zoom: Float, turn: Float): TouchPadSettings = copy(
        scale = scale * zoom,
        horizontal = horizontal + if (placement.travelX > 0)
            dx * (if (left) 1 else -1) / placement.travelX else 0f,
        vertical = vertical - if (placement.travelY > 0) dy / placement.travelY else 0f,
        rotation = rotation + turn,
    ).normalized()
}

data class TouchLayoutSettings(
    val left: TouchPadSettings = TouchPadSettings(),
    val right: TouchPadSettings = TouchPadSettings(),
    val avoidCutout: Boolean = true,
) {
    fun normalized() = copy(left = left.normalized(), right = right.normalized())
    fun pad(isLeft: Boolean) = if (isLeft) left else right
    fun withPad(isLeft: Boolean, pad: TouchPadSettings) =
        if (isLeft) copy(left = pad.normalized()) else copy(right = pad.normalized())

    // Bundle-compatible editor draft; never written to preferences until Save.
    fun values() = floatArrayOf(left.scale, left.horizontal, left.vertical, left.rotation, left.opacity,
        right.scale, right.horizontal, right.vertical, right.rotation, right.opacity, if (avoidCutout) 1f else 0f)

    companion object {
        fun fromValues(values: FloatArray): TouchLayoutSettings {
            if (values.size != 11) return TouchLayoutSettings()
            return TouchLayoutSettings(
                TouchPadSettings(values[0], values[1], values[2], values[3], values[4]),
                TouchPadSettings(values[5], values[6], values[7], values[8], values[9]),
                values[10] != 0f,
            ).normalized()
        }
    }
}

data class TouchInsets(val left: Int = 0, val top: Int = 0, val right: Int = 0, val bottom: Int = 0) {
    fun union(other: TouchInsets) = TouchInsets(maxOf(left, other.left), maxOf(top, other.top),
        maxOf(right, other.right), maxOf(bottom, other.bottom))
}

/** Drawing square inside a full half-screen touch view. Empty space remains
 * inside RadialGamePad so its expanded hit regions keep receiving pointers. */
data class TouchPadPlacement(
    val viewLeft: Int, val viewWidth: Int, val left: Int, val top: Int, val size: Int,
    val travelX: Int, val travelY: Int,
) {
    val spacingLeft get() = left - viewLeft
    val spacingRight get() = viewWidth - spacingLeft - size
}

object TouchLayoutGeometry {
    fun place(width: Int, height: Int, isLeft: Boolean, insets: TouchInsets,
              settings: TouchPadSettings): TouchPadPlacement {
        val w = width.coerceAtLeast(0)
        val h = height.coerceAtLeast(0)
        val viewLeft = if (isLeft) 0 else w / 2
        val viewWidth = if (isLeft) w / 2 else w - viewLeft
        val start = insets.left.coerceIn(viewLeft, viewLeft + viewWidth)
        val end = (w - insets.right.coerceAtLeast(0)).coerceIn(start, viewLeft + viewWidth)
        val top = insets.top.coerceIn(0, h)
        val bottom = (h - insets.bottom.coerceAtLeast(0)).coerceIn(top, h)
        val pad = settings.normalized()
        val size = (minOf(w, h) * 0.55f * pad.scale).roundToInt().coerceIn(0, minOf(end - start, bottom - top))
        val travelX = end - start - size
        val travelY = bottom - top - size
        val x = if (isLeft) start + (travelX * pad.horizontal).roundToInt()
                else end - size - (travelX * pad.horizontal).roundToInt()
        val y = bottom - size - (travelY * pad.vertical).roundToInt()
        return TouchPadPlacement(viewLeft, viewWidth, x, y, size, travelX, travelY)
    }
}

private fun Float.bounded(min: Float, max: Float, fallback: Float) =
    if (isFinite()) coerceIn(min, max) else fallback
