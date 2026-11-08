package dev.dcpport.android;

import android.app.Activity;
import android.content.res.ColorStateList;
import android.content.res.Configuration;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.DashPathEffect;
import android.graphics.Paint;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.os.Bundle;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.SeekBar;
import android.widget.Switch;
import android.widget.TextView;

import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

/**
 * Edits user/touch_layout.ini (android/touch_layout.h): drag a control to move it; the panel sets the selected one's
 * size, opacity and whether it is shown, and turns the touchpad (the free screen area) on or off. Full screen in
 * landscape, so positions are the same fractions of the screen the game uses.
 */
public class TouchLayoutActivity extends Activity {
    /** android/touch_layout.h dcp_touch_defaults: id, x, y, radius, glyph, kind. Keep the two tables the same. */
    private static final Object[][] DEFAULTS = {
            {"cross", 0.885f, 0.80f, 0.085f, "✕", "Cross"},
            {"circle", 0.955f, 0.62f, 0.085f, "○", "Circle"},
            {"square", 0.815f, 0.62f, 0.085f, "□", "Square"},
            {"triangle", 0.885f, 0.44f, 0.085f, "△", "Triangle"},
            {"l1", 0.055f, 0.10f, 0.065f, "L1", "L1"},
            {"l2", 0.145f, 0.10f, 0.065f, "L2", "L2"},
            {"r1", 0.945f, 0.10f, 0.065f, "R1", "R1"},
            {"r2", 0.855f, 0.10f, 0.065f, "R2", "R2"},
            {"back", 0.42f, 0.07f, 0.055f, "Back", "Back (touchpad click)"},
            {"start", 0.58f, 0.07f, 0.055f, "Start", "Start (Options)"},
            {"settings", 0.28f, 0.07f, 0.045f, "⚙", "Settings button"},
            {"up", 0.105f, 0.30f, 0.050f, "▲", "D-pad up"},
            {"down", 0.105f, 0.50f, 0.050f, "▼", "D-pad down"},
            {"left", 0.050f, 0.40f, 0.050f, "◀", "D-pad left"},
            {"right", 0.160f, 0.40f, 0.050f, "▶", "D-pad right"},
            {"left_stick", 0.26f, 0.70f, 0.14f, "L", "Left stick (floats where you touch the left half)"},
            {"right_stick", 0.66f, 0.78f, 0.11f, "R", "Right stick"},
    };

    private static final class Control {
        String id, glyph, name;
        float x, y, defaultR, size = 1f, opacity = 1f;
        boolean enabled = true;
        float r() {
            return defaultR * size;
        }
    }

    private final List<Control> controls = new ArrayList<>();
    private boolean touchpad = true;
    private Control selected;
    private File file;
    private LayoutView canvas;
    private LinearLayout panel;
    private TextView panelTitle;
    private Switch panelEnabled;
    private SeekBar panelSize, panelOpacity;
    private boolean binding;
    private int primary, onPrimary, surface, onSurface, onSurfaceVariant, surfaceContainerHighest, outline;

    @Override
    protected void onCreate(Bundle state) {
        super.onCreate(state);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        boolean dark = (getResources().getConfiguration().uiMode & Configuration.UI_MODE_NIGHT_MASK)
                == Configuration.UI_MODE_NIGHT_YES;
        primary = getColor(dark ? android.R.color.system_accent1_200 : android.R.color.system_accent1_600);
        onPrimary = getColor(dark ? android.R.color.system_accent1_800 : android.R.color.system_accent1_0);
        surface = getColor(dark ? android.R.color.system_neutral2_800 : android.R.color.system_neutral2_50);
        onSurface = getColor(dark ? android.R.color.system_neutral1_100 : android.R.color.system_neutral1_900);
        onSurfaceVariant = getColor(dark ? android.R.color.system_neutral2_200 : android.R.color.system_neutral2_700);
        surfaceContainerHighest = getColor(dark ? android.R.color.system_neutral2_600 : android.R.color.system_neutral2_200);
        outline = getColor(dark ? android.R.color.system_neutral2_400 : android.R.color.system_neutral2_500);
        file = new File(GameFiles.root(this), "user/touch_layout.ini");
        for (Object[] row : DEFAULTS) {
            Control c = new Control();
            c.id = (String) row[0];
            c.x = (float) row[1];
            c.y = (float) row[2];
            c.defaultR = (float) row[3];
            c.glyph = (String) row[4];
            c.name = (String) row[5];
            controls.add(c);
        }
        load();
        FrameLayout root = new FrameLayout(this);
        root.setBackgroundColor(Color.BLACK);
        canvas = new LayoutView();
        root.addView(canvas);
        int width = Math.min(dp(340), getResources().getDisplayMetrics().widthPixels - dp(32));
        root.addView(panel(), new FrameLayout.LayoutParams(width, ViewGroup.LayoutParams.WRAP_CONTENT, Gravity.CENTER));
        setContentView(root);
        bind();
    }

    @Override
    protected void onResume() {
        super.onResume();
        WindowInsetsController insets = getWindow().getInsetsController();
        if (insets != null) {
            insets.hide(WindowInsets.Type.systemBars());
            insets.setSystemBarsBehavior(WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
        }
    }

    // ---- file ----

    private void load() {
        try {
            for (String line : Files.readAllLines(file.toPath(), StandardCharsets.UTF_8)) {
                String[] parts = line.replace("=", " ").trim().split("\\s+");
                if (parts.length != 6 || line.trim().startsWith("#")) {
                    continue;
                }
                if (parts[0].equals("touchpad")) {
                    touchpad = !parts[5].equals("0");
                    continue;
                }
                for (Control c : controls) {
                    if (c.id.equals(parts[0])) {
                        c.x = Float.parseFloat(parts[1]);
                        c.y = Float.parseFloat(parts[2]);
                        c.size = Float.parseFloat(parts[3]);
                        c.opacity = Float.parseFloat(parts[4]);
                        c.enabled = !parts[5].equals("0") || c.id.equals("settings");
                    }
                }
            }
        } catch (IOException | NumberFormatException ignored) {
            // No layout yet, or a damaged one: the defaults.
        }
    }

    private void save() {
        List<String> lines = new ArrayList<>();
        lines.add("# On-screen controls: <id> = <x> <y> <size> <opacity> <enabled> (android/touch_layout.h)");
        for (Control c : controls) {
            lines.add(String.format(Locale.ROOT, "%s = %.4f %.4f %.3f %.3f %d", c.id, c.x, c.y, c.size, c.opacity,
                    c.enabled ? 1 : 0));
        }
        lines.add(String.format(Locale.ROOT, "touchpad = 0 0 1 1 %d", touchpad ? 1 : 0));
        try {
            file.getParentFile().mkdirs();
            Files.write(file.toPath(), lines, StandardCharsets.UTF_8);
        } catch (IOException ignored) {
            // Read-only storage: the game keeps the previous layout.
        }
    }

    // ---- panel ----

    private View panel() {
        panel = new LinearLayout(this);
        panel.setOrientation(LinearLayout.VERTICAL);
        panel.setPadding(dp(20), dp(16), dp(20), dp(12));
        GradientDrawable background = new GradientDrawable();
        background.setColor((surface & 0xFFFFFF) | 0xEE000000);
        background.setCornerRadius(dp(28));
        panel.setBackground(background);
        panelTitle = text("", 18, onSurface, true);
        panel.addView(panelTitle);
        panelEnabled = new Switch(this);
        panelEnabled.setText("Shown");
        panelEnabled.setTextColor(onSurface);
        tint(panelEnabled);
        panelEnabled.setOnCheckedChangeListener((v, checked) -> {
            if (binding) {
                return;
            }
            if (selected == null) {
                touchpad = checked;
            } else {
                selected.enabled = checked || selected.id.equals("settings");
            }
            canvas.invalidate();
        });
        panel.addView(panelEnabled);
        panelSize = slider("Size", 25, 300, value -> {
            if (selected != null) {
                selected.size = value / 100f;
            }
        });
        panelOpacity = slider("Opacity", 10, 100, value -> {
            if (selected != null) {
                selected.opacity = value / 100f;
            }
        });
        LinearLayout row = new LinearLayout(this);
        row.setGravity(Gravity.END);
        row.setPadding(0, dp(8), 0, 0);
        row.addView(textButton("Reset", v -> {
            if (selected != null) {
                Control fresh = defaults(selected.id);
                selected.x = fresh.x;
                selected.y = fresh.y;
                selected.size = 1f;
                selected.opacity = 1f;
                selected.enabled = true;
            } else {
                for (Control c : controls) {
                    Control fresh = defaults(c.id);
                    c.x = fresh.x;
                    c.y = fresh.y;
                    c.size = 1f;
                    c.opacity = 1f;
                    c.enabled = true;
                }
                touchpad = true;
            }
            bind();
        }));
        row.addView(filledButton("Save", v -> {
            save();
            finish();
        }));
        panel.addView(row);
        return panel;
    }

    /** Shows the selected control in the panel, or the layout-wide choices when none is selected. */
    private void bind() {
        binding = true;
        boolean any = selected != null;
        panelTitle.setText(any ? selected.name : "Drag a control to move it, tap one to change it");
        panelEnabled.setText(any ? "Shown" : "Touch the free screen area as the PS4 touchpad");
        panelEnabled.setChecked(any ? selected.enabled : touchpad);
        panelEnabled.setEnabled(!any || !selected.id.equals("settings"));
        ((View) panelSize.getParent()).setVisibility(any ? View.VISIBLE : View.GONE);
        ((View) panelOpacity.getParent()).setVisibility(any ? View.VISIBLE : View.GONE);
        if (any) {
            panelSize.setProgress(Math.round(selected.size * 100) - 25);
            panelOpacity.setProgress(Math.round(selected.opacity * 100) - 10);
        }
        // Out of the way of the selected control: the edge across from it.
        FrameLayout.LayoutParams place = (FrameLayout.LayoutParams) panel.getLayoutParams();
        place.gravity = !any ? Gravity.CENTER
                : (selected.y > 0.5f ? Gravity.TOP : Gravity.BOTTOM) | Gravity.CENTER_HORIZONTAL;
        panel.setLayoutParams(place);
        binding = false;
        canvas.invalidate();
    }

    private interface IntChange {
        void set(int value);
    }

    private SeekBar slider(String label, int min, int max, IntChange change) {
        LinearLayout row = new LinearLayout(this);
        row.setGravity(Gravity.CENTER_VERTICAL);
        TextView name = text(label, 14, onSurfaceVariant, false);
        row.addView(name, new LinearLayout.LayoutParams(dp(64), ViewGroup.LayoutParams.WRAP_CONTENT));
        SeekBar bar = new SeekBar(this);
        bar.setMax(max - min);
        bar.setProgressTintList(ColorStateList.valueOf(primary));
        bar.setThumbTintList(ColorStateList.valueOf(primary));
        bar.setProgressBackgroundTintList(ColorStateList.valueOf(surfaceContainerHighest));
        bar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                if (fromUser && !binding) {
                    change.set(progress + min);
                    canvas.invalidate();
                }
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {}

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {}
        });
        row.addView(bar, new LinearLayout.LayoutParams(0, dp(40), 1));
        panel.addView(row);
        return bar;
    }

    private Control defaults(String id) {
        for (Object[] row : DEFAULTS) {
            if (row[0].equals(id)) {
                Control c = new Control();
                c.x = (float) row[1];
                c.y = (float) row[2];
                return c;
            }
        }
        return new Control();
    }

    private void tint(Switch control) {
        int[][] states = {{android.R.attr.state_checked}, {}};
        control.setThumbTintList(new ColorStateList(states, new int[] {onPrimary, outline}));
        control.setTrackTintList(new ColorStateList(states, new int[] {primary, surfaceContainerHighest}));
    }

    private TextView textButton(String label, View.OnClickListener click) {
        TextView button = text(label, 14, primary, true);
        button.setPadding(dp(16), dp(10), dp(16), dp(10));
        button.setOnClickListener(click);
        return button;
    }

    private TextView filledButton(String label, View.OnClickListener click) {
        TextView button = text(label, 14, onPrimary, true);
        button.setPadding(dp(24), dp(10), dp(24), dp(10));
        GradientDrawable shape = new GradientDrawable();
        shape.setColor(primary);
        shape.setCornerRadius(dp(20));
        button.setBackground(shape);
        button.setOnClickListener(click);
        return button;
    }

    private TextView text(String value, int sp, int color, boolean medium) {
        TextView view = new TextView(this);
        view.setText(value);
        view.setTextSize(TypedValue.COMPLEX_UNIT_SP, sp);
        view.setTextColor(color);
        if (medium) {
            view.setTypeface(Typeface.create("sans-serif-medium", Typeface.NORMAL));
        }
        return view;
    }

    private int dp(float value) {
        return Math.round(TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, value,
                getResources().getDisplayMetrics()));
    }

    // ---- the layout ----

    private final class LayoutView extends View {
        private final Paint fill = new Paint(Paint.ANTI_ALIAS_FLAG), ring = new Paint(Paint.ANTI_ALIAS_FLAG),
                glyph = new Paint(Paint.ANTI_ALIAS_FLAG);
        private Control dragging;
        private float lastX, lastY, downX, downY;

        LayoutView() {
            super(TouchLayoutActivity.this);
            ring.setStyle(Paint.Style.STROKE);
            glyph.setTextAlign(Paint.Align.CENTER);
            glyph.setTypeface(Typeface.create("sans-serif-medium", Typeface.NORMAL));
        }

        @Override
        protected void onDraw(Canvas c) {
            float w = getWidth(), h = getHeight();
            // The game's 16:9 picture, for reference.
            float gameH = Math.min(h, w * 9 / 16), top = (h - gameH) / 2;
            fill.setColor(0xFF1C1B1F);
            c.drawRect(0, top, w, top + gameH, fill);
            for (Control control : controls) {
                float cx = control.x * w, cy = control.y * h, r = control.r() * h;
                int alpha = Math.round(255 * control.opacity * (control.enabled ? 1f : 0.35f));
                fill.setColor(Color.argb(Math.round(alpha * 0.35f), 0, 0, 0));
                c.drawCircle(cx, cy, r, fill);
                boolean picked = control == selected;
                ring.setColor(picked ? primary : Color.argb(alpha, 255, 255, 255));
                ring.setStrokeWidth(picked ? dp(3) : dp(1.5f));
                ring.setPathEffect(control.enabled ? null : new DashPathEffect(new float[] {dp(6), dp(4)}, 0));
                c.drawCircle(cx, cy, r, ring);
                glyph.setColor(Color.argb(alpha, 255, 255, 255));
                glyph.setTextSize(Math.min(r * (control.glyph.length() > 2 ? 0.55f : 0.8f), dp(28)));
                c.drawText(control.glyph, cx, cy - (glyph.descent() + glyph.ascent()) / 2, glyph);
            }
        }

        @Override
        public boolean onTouchEvent(MotionEvent e) {
            float w = getWidth(), h = getHeight();
            switch (e.getActionMasked()) {
                case MotionEvent.ACTION_DOWN:
                    dragging = null;
                    float best = Float.MAX_VALUE;
                    for (Control control : controls) {
                        float dx = e.getX() - control.x * w, dy = e.getY() - control.y * h;
                        float distance = dx * dx + dy * dy, reach = control.r() * h * 1.2f;
                        if (distance <= reach * reach && distance < best) {
                            best = distance;
                            dragging = control;
                        }
                    }
                    lastX = downX = e.getX();
                    lastY = downY = e.getY();
                    if (dragging != null) {
                        panel.setAlpha(0.3f);
                    }
                    return true;
                case MotionEvent.ACTION_MOVE:
                    if (dragging != null) {
                        dragging.x = Math.max(0f, Math.min(1f, dragging.x + (e.getX() - lastX) / w));
                        dragging.y = Math.max(0f, Math.min(1f, dragging.y + (e.getY() - lastY) / h));
                        lastX = e.getX();
                        lastY = e.getY();
                        invalidate();
                    }
                    return true;
                case MotionEvent.ACTION_UP:
                    panel.setAlpha(1f);
                    boolean tap = Math.abs(e.getX() - downX) < dp(8) && Math.abs(e.getY() - downY) < dp(8);
                    if (dragging != null || tap) {
                        selected = dragging;
                        bind();
                    }
                    dragging = null;
                    return true;
                default:
                    return true;
            }
        }
    }
}
