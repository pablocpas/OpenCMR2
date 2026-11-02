package io.github.pablocpas.opencmr2

import android.app.Activity
import android.content.Context
import android.graphics.Color
import android.graphics.drawable.GradientDrawable
import android.text.Editable
import android.text.InputFilter
import android.text.InputType
import android.text.TextUtils
import android.text.TextWatcher
import android.util.TypedValue
import android.view.Gravity
import android.view.KeyEvent
import android.view.View
import android.view.ViewGroup
import android.view.WindowManager
import android.view.inputmethod.EditorInfo
import android.view.inputmethod.InputMethodManager
import android.widget.Button
import android.widget.EditText
import android.widget.FrameLayout
import android.widget.LinearLayout
import android.widget.TextView

/**
 * The game's text entries (a game's name and password, the chat line, a saved
 * game's name) without a physical keyboard: a bar at the top of the screen
 * with the entry's label, its text and the system's keyboard below. The game
 * goes on behind it. Accepting replaces the entry's text (sys.cpp types it in);
 * cancelling keeps it. While the game is still on the entry, a chip at the top
 * opens the bar again.
 */
class TextEntry(private val activity: Activity, parent: ViewGroup, private val onOpen: (Boolean) -> Unit) {
    private val density = activity.resources.displayMetrics.density
    private val root = FrameLayout(activity).apply { isVisible(false) }
    private val bar = LinearLayout(activity)
    private val label = TextView(activity)
    private val count = TextView(activity)
    private val edit = object : EditText(activity) {
        // Back closes the keyboard and the bar together.
        override fun onKeyPreIme(keyCode: Int, event: KeyEvent): Boolean {
            if (keyCode == KeyEvent.KEYCODE_BACK && event.action == KeyEvent.ACTION_UP && isOpen) {
                answer(false)
                return true
            }
            return super.onKeyPreIme(keyCode, event)
        }
    }
    private val chip = TextView(activity)
    private var maxLength = 0
    private var softInputMode = 0
    private var entryLabel = ""

    /** The bar is open (a chip alone does not count). */
    var isOpen = false
        private set

    init {
        bar.orientation = LinearLayout.HORIZONTAL
        bar.gravity = Gravity.CENTER_VERTICAL
        bar.background = GradientDrawable().apply {
            setColor(0xF0182220.toInt())
            cornerRadii = floatArrayOf(0f, 0f, 0f, 0f, dp(14), dp(14), dp(14), dp(14))
        }
        bar.elevation = dp(8)
        bar.isClickable = true

        val column = LinearLayout(activity).apply { orientation = LinearLayout.VERTICAL }
        label.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13f)
        label.setTextColor(ACCENT)
        label.isAllCaps = true
        label.letterSpacing = 0.08f
        column.addView(label)

        edit.setTextSize(TypedValue.COMPLEX_UNIT_SP, 22f)
        edit.setTextColor(Color.WHITE)
        edit.setHintTextColor(0x80FFFFFF.toInt())
        edit.backgroundTintList = android.content.res.ColorStateList.valueOf(ACCENT)
        edit.isSingleLine = true
        edit.inputType = InputType.TYPE_CLASS_TEXT or InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS
        edit.imeOptions = EditorInfo.IME_ACTION_DONE or EditorInfo.IME_FLAG_NO_EXTRACT_UI or
            EditorInfo.IME_FLAG_NO_FULLSCREEN
        edit.setOnEditorActionListener { _, action, _ ->
            if (action != EditorInfo.IME_ACTION_DONE) return@setOnEditorActionListener false
            answer(true)
            true
        }
        edit.addTextChangedListener(object : TextWatcher {
            override fun beforeTextChanged(s: CharSequence?, start: Int, count: Int, after: Int) {}
            override fun onTextChanged(s: CharSequence?, start: Int, before: Int, count: Int) {}
            override fun afterTextChanged(s: Editable?) = updateCount()
        })
        column.addView(edit)
        bar.addView(column, LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f))

        count.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13f)
        count.setTextColor(0xB0FFFFFF.toInt())
        count.setPadding(dp(12).toInt(), 0, dp(12).toInt(), 0)
        bar.addView(count)

        bar.addView(button(activity.getString(android.R.string.cancel), filled = false) { answer(false) })
        bar.addView(button(activity.getString(android.R.string.ok), filled = true) { answer(true) },
            LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT).apply {
                marginStart = dp(8).toInt()
            })
        root.addView(bar, FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
            ViewGroup.LayoutParams.WRAP_CONTENT, Gravity.TOP))

        chip.setTextSize(TypedValue.COMPLEX_UNIT_SP, 15f)
        chip.setTextColor(Color.WHITE)
        chip.background = GradientDrawable().apply {
            setColor(0xE0182220.toInt())
            cornerRadius = dp(20)
            setStroke(dp(1).toInt(), ACCENT)
        }
        chip.setPadding(dp(18).toInt(), dp(10).toInt(), dp(18).toInt(), dp(10).toInt())
        chip.elevation = dp(6)
        chip.setOnClickListener { nativeReopen() }
        root.addView(chip, FrameLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT,
            ViewGroup.LayoutParams.WRAP_CONTENT, Gravity.TOP or Gravity.CENTER_HORIZONTAL).apply {
            topMargin = dp(12).toInt()
        })

        // Over the game and the touch controls; only the bar and the chip
        // take touches.
        parent.addView(root, ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
            ViewGroup.LayoutParams.MATCH_PARENT))
        root.setOnApplyWindowInsetsListener { _, insets ->
            val cutout = insets.displayCutout
            val side = dp(20).toInt()
            bar.setPadding(maxOf(side, cutout?.safeInsetLeft ?: 0) , dp(10).toInt(),
                maxOf(side, cutout?.safeInsetRight ?: 0), dp(12).toInt())
            insets
        }
        root.isClickable = false
    }

    /** The game starts editing an entry, or the chip asked for it again. */
    fun open(entryLabel: String, text: String, maxLength: Int) {
        this.entryLabel = entryLabel
        this.maxLength = maxLength
        label.text = entryLabel
        edit.filters = arrayOf(InputFilter.LengthFilter(maxLength))
        edit.setText(text)
        edit.setSelection(edit.text.length)
        updateCount()
        if (!isOpen) {
            softInputMode = activity.window.attributes.softInputMode
            // The bar is at the top: the game stays as it is under the keyboard.
            activity.window.setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_NOTHING)
        }
        isOpen = true
        root.isVisible(true)
        bar.visibility = View.VISIBLE
        chip.visibility = View.GONE
        onOpen(true)
        edit.requestFocus()
        edit.post {
            val input = activity.getSystemService(Context.INPUT_METHOD_SERVICE) as InputMethodManager
            input.showSoftInput(edit, InputMethodManager.SHOW_IMPLICIT)
        }
    }

    /** The game left the entry. */
    fun end() {
        if (isOpen) close()
        root.isVisible(false)
    }

    /** A gamepad's A accepts and B cancels while the bar is open. */
    fun onKey(event: KeyEvent): Boolean {
        if (!isOpen) return false
        val accept = event.keyCode == KeyEvent.KEYCODE_BUTTON_A || event.keyCode == KeyEvent.KEYCODE_BUTTON_START
        val cancel = event.keyCode == KeyEvent.KEYCODE_BUTTON_B || event.keyCode == KeyEvent.KEYCODE_BACK
        if (!accept && !cancel) return false
        if (event.action == KeyEvent.ACTION_UP) answer(accept)
        return true
    }

    private fun answer(accepted: Boolean) {
        if (!isOpen) return
        nativeTextEntered(edit.text.toString(), accepted)
        close()
        chip.text = "✎  " + entryLabel
        chip.visibility = if (TextUtils.isEmpty(entryLabel)) View.GONE else View.VISIBLE
    }

    private fun close() {
        isOpen = false
        val input = activity.getSystemService(Context.INPUT_METHOD_SERVICE) as InputMethodManager
        input.hideSoftInputFromWindow(edit.windowToken, 0)
        edit.clearFocus()
        bar.visibility = View.GONE
        activity.window.setSoftInputMode(softInputMode)
        onOpen(false)
    }

    private fun updateCount() {
        count.text = "${edit.text.length}/$maxLength"
    }

    private fun button(text: String, filled: Boolean, onClick: () -> Unit) = Button(activity).apply {
        this.text = text
        isAllCaps = false
        setTextSize(TypedValue.COMPLEX_UNIT_SP, 16f)
        setTextColor(if (filled) 0xFF102018.toInt() else Color.WHITE)
        background = GradientDrawable().apply {
            cornerRadius = dp(22)
            if (filled) setColor(ACCENT) else setStroke(dp(1).toInt(), 0x80FFFFFF.toInt())
        }
        minHeight = dp(44).toInt()
        minimumHeight = dp(44).toInt()
        setPadding(dp(20).toInt(), 0, dp(20).toInt(), 0)
        stateListAnimator = null
        setOnClickListener { onClick() }
    }

    private fun View.isVisible(visible: Boolean) {
        visibility = if (visible) View.VISIBLE else View.GONE
    }

    private fun dp(value: Int) = value * density

    companion object {
        // The grey green of the game's menus.
        private const val ACCENT = 0xFFA9C2B8.toInt()

        @JvmStatic
        private external fun nativeTextEntered(text: String, accepted: Boolean)

        @JvmStatic
        private external fun nativeReopen()
    }
}
