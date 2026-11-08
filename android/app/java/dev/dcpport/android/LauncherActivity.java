package dev.dcpport.android;

import android.app.Activity;
import android.app.Dialog;
import android.content.Intent;
import android.content.res.ColorStateList;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.PixelFormat;
import android.graphics.RectF;
import android.graphics.Typeface;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.GradientDrawable;
import android.graphics.drawable.RippleDrawable;
import android.net.Uri;
import android.os.Bundle;
import android.text.InputFilter;
import android.text.InputType;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.InputDevice;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.Switch;
import android.widget.TextView;

import java.io.File;
import java.io.InputStream;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

/**
 * The app's home and settings, in Material You: the dynamic colours of the system palette, tonal surfaces and rounded
 * shapes, built from framework views only (the APK is packaged without Gradle or AndroidX). Settings are the launcher's
 * own dcp.ini keys (launcher/settings.h), written as they change; the game reads them when it starts.
 */
public class LauncherActivity extends Activity {
    /** Set by DcpActivity when the in-game settings gear opens this screen: Play becomes Resume. */
    public static final String EXTRA_FROM_GAME = "from_game";

    private static final String HOME = "home", GRAPHICS = "graphics", GAMEPLAY = "gameplay", CONTROLS = "controls",
            ONLINE = "online", ABOUT = "about";
    private static final String ON = "1", OFF = "0";
    private static final String SOURCE_URL = "https://github.com/Shmubbier/dragons-crown-pro-pc/tree/android-port";

    private final ArrayDeque<String> pages = new ArrayDeque<>();
    private Ini ini;
    private File root;
    private boolean fromGame;
    private boolean dark;
    // Material 3 roles from the system's dynamic palette.
    private int primary, onPrimary, primaryContainer, onPrimaryContainer, secondaryContainer, onSecondaryContainer,
            surface, surfaceContainer, surfaceContainerHigh, surfaceContainerHighest, onSurface, onSurfaceVariant,
            outline, outlineVariant;

    @Override
    protected void onCreate(Bundle state) {
        super.onCreate(state);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        dark = (getResources().getConfiguration().uiMode & Configuration.UI_MODE_NIGHT_MASK)
                == Configuration.UI_MODE_NIGHT_YES;
        loadPalette();
        getWindow().setStatusBarColor(surface);
        getWindow().setNavigationBarColor(surface);
        root = GameFiles.root(this);
        ini = new Ini(new File(root, "user/dcp.ini"));
        fromGame = getIntent().getBooleanExtra(EXTRA_FROM_GAME, false);
        show(HOME);
    }

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        setIntent(intent);
        fromGame = intent.getBooleanExtra(EXTRA_FROM_GAME, false);
        pages.clear();
        show(HOME);
    }

    @Override
    public void onBackPressed() {
        if (pages.size() > 1) {
            pages.pop();
            render(pages.peek());
        } else {
            super.onBackPressed();
        }
    }

    private void show(String page) {
        pages.push(page);
        render(page);
    }

    private void render(String page) {
        LinearLayout column = new LinearLayout(this);
        column.setOrientation(LinearLayout.VERTICAL);
        column.setPadding(dp(16), dp(8), dp(16), dp(24));
        switch (page) {
            case GRAPHICS: graphics(column); break;
            case GAMEPLAY: gameplay(column); break;
            case CONTROLS: controls(column); break;
            case ONLINE: online(column); break;
            case ABOUT: about(column); break;
            default: home(column); break;
        }
        // One readable column on wide screens.
        FrameLayout frame = new FrameLayout(this);
        frame.addView(column, new FrameLayout.LayoutParams(Math.min(dp(720), widthPx()),
                ViewGroup.LayoutParams.WRAP_CONTENT, Gravity.CENTER_HORIZONTAL));
        ScrollView scroll = new ScrollView(this);
        scroll.setBackgroundColor(surface);
        scroll.setFitsSystemWindows(true);
        scroll.addView(frame);
        setContentView(scroll);
    }

    // ---- pages ----

    private void home(LinearLayout c) {
        TextView title = text("Dragon's Crown Pro", 32, onSurface, true);
        title.setPadding(dp(4), dp(24), 0, 0);
        c.addView(title);
        TextView subtitle = text("Android port · based on Dragon's Crown Pro PC " + portVersion(), 14, onSurfaceVariant,
                false);
        subtitle.setPadding(dp(4), dp(4), 0, dp(16));
        c.addView(subtitle);
        c.addView(hero());
        c.addView(label("Settings"));
        LinearLayout list = card();
        list.addView(item(Icon.DISPLAY, "Graphics", "Render resolution, aspect ratio, filtering", () -> show(GRAPHICS)));
        list.addView(divider());
        list.addView(item(Icon.SWORD, "Gameplay", "Language, frame rate, skip offers", () -> show(GAMEPLAY)));
        list.addView(divider());
        list.addView(item(Icon.GAMEPAD, "Controls", "On-screen controls, controller for each player",
                () -> show(CONTROLS)));
        list.addView(divider());
        list.addView(item(Icon.GLOBE, "Online", ini.get("online_enabled", OFF).equals(ON)
                ? "On as " + ini.get("online_name", "Player") : "Off", () -> show(ONLINE)));
        list.addView(divider());
        list.addView(item(Icon.INFO, "About", "Version, source code and licences", () -> show(ABOUT)));
        c.addView(list);
    }

    private View hero() {
        LinearLayout hero = new LinearLayout(this);
        hero.setOrientation(LinearLayout.VERTICAL);
        hero.setBackground(rounded(primaryContainer, dp(28)));
        hero.setClipToOutline(true);
        Bitmap art = headerArt();
        if (art != null) {
            // The whole flourish across the card's top edge, at its own proportions.
            ImageView image = new ImageView(this);
            image.setImageBitmap(art);
            image.setAdjustViewBounds(true);
            image.setScaleType(ImageView.ScaleType.FIT_CENTER);
            image.setAlpha(dark ? 0.95f : 0.8f);
            image.setPadding(dp(16), dp(12), dp(16), 0);
            hero.addView(image, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT));
        }
        LinearLayout body = new LinearLayout(this);
        body.setOrientation(LinearLayout.VERTICAL);
        body.setPadding(dp(24), dp(16), dp(24), dp(24));
        boolean installed = GameFiles.installed(root);
        body.addView(text(installed ? "Ready to play" : "No game installed", 22, onPrimaryContainer, true));
        TextView detail = text(installed
                ? "Settings apply the next time the game starts."
                : "Copy your prepared game into the app's files folder (docs/ANDROID.md). Importing a package "
                        + "from the phone is coming.", 14, onPrimaryContainer, false);
        detail.setAlpha(0.85f);
        detail.setPadding(0, dp(4), 0, dp(20));
        body.addView(detail);
        TextView play = button(fromGame ? "Resume game" : "Play", primary, onPrimary);
        play.setEnabled(installed);
        play.setAlpha(installed ? 1f : 0.38f);
        play.setOnClickListener(v -> play());
        body.addView(play, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, dp(56)));
        hero.addView(body);
        return hero;
    }

    private void graphics(LinearLayout c) {
        header(c, "Graphics");
        LinearLayout list = card();
        list.addView(choice("Render resolution", "The size the game draws at before it is scaled to the screen. "
                + "720p keeps 60 fps on most phones; higher costs heat and battery.", "render_resolution", "720p",
                new String[][] {{"native", "Native (1080p)"}, {"480p", "480p"}, {"720p", "720p"},
                        {"1440p", "1440p"}, {"1800p", "1800p"}, {"4K", "4K"}}));
        list.addView(divider());
        list.addView(choice("Aspect ratio", "How the picture fits a screen of another shape.", "aspect_ratio", "16:9",
                new String[][] {{"16:9", "16:9 Original"}, {"16:10", "16:10"}, {"21:9", "21:9 Ultrawide"},
                        {"32:9", "32:9 Super Ultrawide"}, {"4:3", "4:3"}, {"5:4", "5:4"}}));
        list.addView(divider());
        list.addView(toggle("Anti-aliasing", "SMAA smooths jagged edges.", "smaa", false));
        list.addView(divider());
        list.addView(choice("Texture filtering", "Sharper textures at an angle.", "anisotropy", "0",
                new String[][] {{"0", "Game"}, {"2", "2x"}, {"4", "4x"}, {"8", "8x"}, {"16", "16x"}}));
        list.addView(divider());
        list.addView(toggle("Upscaling", "FSR 1 upscales a lower render resolution to the screen.", "fsr1", false));
        list.addView(divider());
        list.addView(toggle("Sharpening", "RCAS sharpens the scaled picture.", "rcas", true));
        c.addView(list);
        note(c);
    }

    private void gameplay(LinearLayout c) {
        header(c, "Gameplay");
        LinearLayout list = card();
        list.addView(choice("Menu & Subtitles", "The language of the menus and subtitles. Voices are chosen in the game.",
                "text_language", "english",
                new String[][] {{"english", "English"}, {"french", "French"}, {"german", "German"},
                        {"italian", "Italian"}, {"brazilian_portuguese", "Portuguese (Brazil)"},
                        {"spanish", "Spanish"}}));
        list.addView(divider());
        list.addView(choice("Frame rate", "The game's frame rate limit.", "frame_rate", "60",
                new String[][] {{"30", "30 fps"}, {"60", "60 fps"}, {"90", "90 fps"}, {"120", "120 fps"}}));
        list.addView(divider());
        list.addView(toggle("Frame counter", "Shows the frame rate in a corner of the game.", "show_fps", false));
        list.addView(divider());
        list.addView(toggle("Story Skip Offer", "New characters are asked whether to skip the opening story.",
                "story_skip_offer", false));
        list.addView(divider());
        list.addView(toggle("Tutorial Skip Offer", "New characters are asked whether to skip the tutorial.",
                "tutorial_skip_offer", false));
        c.addView(list);
        note(c);
    }

    private void controls(LinearLayout c) {
        header(c, "Controls");
        c.addView(label("On screen"));
        LinearLayout screen = card();
        screen.addView(choice("On-screen controls", "Automatic hides them while a controller plays player 1. The "
                + "settings button stays on screen either way.", "touch_controls", "auto",
                new String[][] {{"auto", "Automatic"}, {"off", "Off"}}));
        screen.addView(divider());
        screen.addView(item(Icon.GAMEPAD, "Edit layout", "Move, resize, fade or hide each control; the right stick "
                + "and the touchpad", () -> startActivity(new Intent(this, TouchLayoutActivity.class))));
        screen.addView(divider());
        screen.addView(toggle("Japanese Menu Buttons", "Circle confirms and Cross cancels in menus, as in the "
                + "Japanese release.", "japanese_menu_buttons", false));
        c.addView(screen);
        c.addView(label("Players"));
        LinearLayout players = card();
        for (int player = 1; player <= 4; ++player) {
            if (player > 1) {
                players.addView(divider());
            }
            players.addView(choice("Player " + player, player == 1
                    ? "A new controller takes the first free player set to it, else the first free player set "
                            + "to Any controller."
                    : null, "player" + player + "_controller", "", controllerOptions(player)));
        }
        c.addView(players);
        note(c);
    }

    private void online(LinearLayout c) {
        header(c, "Online");
        LinearLayout list = card();
        list.addView(toggle("Online play", "Play with PC and phone players through the port's online service. "
                + "Cheats are locked while it is on.", "online_enabled", false));
        list.addView(divider());
        list.addView(textSetting("Online name", "What other players see: up to 16 letters, digits, - or _.",
                "online_name", "Player", 16));
        list.addView(divider());
        list.addView(toggle("Ping display", "Shows the ping to the other players while playing online.",
                "show_ping", false));
        list.addView(divider());
        list.addView(toggle("Show me in the public lobby", "Lists you, while online, for players looking for a room.",
                "show_in_lobby", false));
        c.addView(list);
        TextView more = text("Friend codes and requests are coming to the phone. Random rooms and join codes work "
                + "now.", 13, onSurfaceVariant, false);
        more.setPadding(dp(16), dp(12), dp(16), 0);
        c.addView(more);
        note(c);
    }

    private void about(LinearLayout c) {
        header(c, "About");
        LinearLayout list = card();
        list.addView(info("Version", appVersion()));
        list.addView(divider());
        list.addView(info("Based on", "Dragon's Crown Pro PC " + portVersion() + " by saulob"));
        list.addView(divider());
        list.addView(item(Icon.CODE, "Source code", "GPL-3.0-or-later in this build", () -> open(SOURCE_URL)));
        list.addView(divider());
        list.addView(info("Game files", "Not included. Dragon's Crown Pro and its assets belong to their owners."));
        c.addView(list);
    }

    // ---- controls ----

    private String[][] controllerOptions(int player) {
        List<String[]> options = new ArrayList<>();
        options.add(new String[] {"", "Any controller"});
        if (player == 1) {
            options.add(new String[] {"none", "On-screen controls only"});
        }
        for (int id : InputDevice.getDeviceIds()) {
            InputDevice device = InputDevice.getDevice(id);
            if (device == null || device.isVirtual()) {
                continue;
            }
            int sources = device.getSources();
            if ((sources & InputDevice.SOURCE_GAMEPAD) != InputDevice.SOURCE_GAMEPAD
                    && (sources & InputDevice.SOURCE_JOYSTICK) != InputDevice.SOURCE_JOYSTICK) {
                continue;
            }
            String model = String.format(Locale.ROOT, "%04x:%04x", device.getVendorId(), device.getProductId());
            if (!listed(options, model)) {
                options.add(new String[] {model, device.getName()});
            }
        }
        String saved = ini.get("player" + player + "_controller", "");
        if (!saved.isEmpty() && !listed(options, saved)) {
            options.add(new String[] {saved, "Saved controller (not connected)"});
        }
        return options.toArray(new String[0][]);
    }

    private static boolean listed(List<String[]> options, String value) {
        for (String[] option : options) {
            if (option[0].equals(value)) {
                return true;
            }
        }
        return false;
    }

    // ---- actions ----

    /** Starts the game, or brings the running one back (DcpActivity is singleInstance: no second copy). */
    private void play() {
        startActivity(new Intent(this, DcpActivity.class));
        finish();
    }

    private void open(String url) {
        try {
            startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(url)));
        } catch (RuntimeException ignored) {
            // No browser: nothing to open it with.
        }
    }

    // ---- rows ----

    private View toggle(String title, String help, String key, boolean def) {
        LinearLayout row = row(title, help);
        Switch control = new Switch(this);
        control.setChecked(ini.getBool(key, def));
        int[][] states = {{android.R.attr.state_checked}, {}};
        control.setThumbTintList(new ColorStateList(states, new int[] {onPrimary, outline}));
        control.setTrackTintList(new ColorStateList(states, new int[] {primary, surfaceContainerHighest}));
        control.setOnCheckedChangeListener((view, checked) -> ini.set(key, checked ? ON : OFF));
        row.addView(control);
        row.setOnClickListener(v -> control.toggle());
        return row;
    }

    private View choice(String title, String help, String key, String def, String[][] options) {
        LinearLayout row = row(title, help);
        TextView value = text(labelOf(options, ini.get(key, def)), 14, primary, true);
        value.setPadding(dp(12), 0, 0, 0);
        row.addView(value);
        row.setOnClickListener(v -> {
            String current = ini.get(key, def);
            LinearLayout list = new LinearLayout(this);
            list.setOrientation(LinearLayout.VERTICAL);
            Dialog dialog = dialog(title, list, null, null);
            for (String[] option : options) {
                list.addView(radio(option[1], option[0].equals(current), () -> {
                    ini.set(key, option[0]);
                    value.setText(option[1]);
                    dialog.dismiss();
                }));
            }
            dialog.show();
        });
        return row;
    }

    private View textSetting(String title, String help, String key, String def, int maxLength) {
        LinearLayout row = row(title, help);
        TextView value = text(ini.get(key, def), 14, primary, true);
        value.setPadding(dp(12), 0, 0, 0);
        row.addView(value);
        row.setOnClickListener(v -> {
            EditText field = new EditText(this);
            field.setText(ini.get(key, def));
            field.setSingleLine(true);
            field.setTextColor(onSurface);
            field.setTextSize(TypedValue.COMPLEX_UNIT_SP, 16);
            field.setBackgroundTintList(ColorStateList.valueOf(primary));
            field.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS);
            field.setFilters(new InputFilter[] {new InputFilter.LengthFilter(maxLength)});
            dialog(title, field, "Save", () -> {
                String text = field.getText().toString().replaceAll("[^A-Za-z0-9_-]", "");
                if (!text.isEmpty()) {
                    ini.set(key, text);
                    value.setText(text);
                }
            }).show();
        });
        return row;
    }

    /** A Material 3 basic dialog in the app's palette: headline, content, and Cancel plus an optional action. */
    private Dialog dialog(String title, View content, String action, Runnable onAction) {
        Dialog dialog = new Dialog(this);
        dialog.requestWindowFeature(Window.FEATURE_NO_TITLE);
        LinearLayout box = new LinearLayout(this);
        box.setOrientation(LinearLayout.VERTICAL);
        box.setPadding(dp(24), dp(24), dp(24), dp(16));
        box.setBackground(rounded(surfaceContainerHigh, dp(28)));
        TextView headline = text(title, 24, onSurface, false);
        headline.setPadding(0, 0, 0, dp(16));
        box.addView(headline);
        box.addView(content);
        LinearLayout buttons = new LinearLayout(this);
        buttons.setGravity(Gravity.END);
        buttons.setPadding(0, dp(16), 0, 0);
        TextView cancel = textButton("Cancel");
        cancel.setOnClickListener(v -> dialog.dismiss());
        buttons.addView(cancel);
        if (action != null) {
            TextView ok = textButton(action);
            ok.setOnClickListener(v -> {
                onAction.run();
                dialog.dismiss();
            });
            buttons.addView(ok);
        }
        box.addView(buttons);
        dialog.setContentView(box);
        Window window = dialog.getWindow();
        window.setBackgroundDrawable(new GradientDrawable());
        window.setLayout(Math.min(dp(400), widthPx() - dp(48)), ViewGroup.LayoutParams.WRAP_CONTENT);
        return dialog;
    }

    private View radio(String label, boolean checked, Runnable onPick) {
        LinearLayout row = new LinearLayout(this);
        row.setGravity(Gravity.CENTER_VERTICAL);
        row.setMinimumHeight(dp(52));
        View mark = new View(this);
        mark.setBackground(new RadioDrawable(checked));
        row.addView(mark, new LinearLayout.LayoutParams(dp(20), dp(20)));
        TextView name = text(label, 16, onSurface, false);
        name.setPadding(dp(16), 0, 0, 0);
        row.addView(name);
        row.setBackground(ripple());
        row.setOnClickListener(v -> onPick.run());
        return row;
    }

    private TextView textButton(String label) {
        TextView button = text(label, 14, primary, true);
        button.setGravity(Gravity.CENTER);
        button.setPadding(dp(16), 0, dp(16), 0);
        button.setMinHeight(dp(40));
        button.setBackground(new RippleDrawable(ColorStateList.valueOf(withAlpha(primary, 0.12f)), null,
                rounded(0xFFFFFFFF, dp(20))));
        return button;
    }

    private final class RadioDrawable extends Drawable {
        private final boolean checked;
        private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);

        RadioDrawable(boolean checked) {
            this.checked = checked;
        }

        @Override
        public void draw(Canvas canvas) {
            float cx = getBounds().exactCenterX(), cy = getBounds().exactCenterY(), r = getBounds().width() / 2f;
            paint.setStyle(Paint.Style.STROKE);
            paint.setStrokeWidth(dp(2));
            paint.setColor(checked ? primary : onSurfaceVariant);
            canvas.drawCircle(cx, cy, r - dp(1), paint);
            if (checked) {
                paint.setStyle(Paint.Style.FILL);
                canvas.drawCircle(cx, cy, r * 0.5f, paint);
            }
        }

        @Override
        public void setAlpha(int alpha) {}

        @Override
        public void setColorFilter(ColorFilter filter) {}

        @Override
        public int getOpacity() {
            return PixelFormat.TRANSLUCENT;
        }
    }

    private View info(String title, String detail) {
        return row(title, detail);
    }

    private View item(Icon icon, String title, String detail, Runnable action) {
        LinearLayout row = new LinearLayout(this);
        row.setGravity(Gravity.CENTER_VERTICAL);
        row.setPadding(dp(16), dp(12), dp(16), dp(12));
        row.setMinimumHeight(dp(72));
        View badge = new View(this);
        badge.setBackground(new IconDrawable(icon, secondaryContainer, onSecondaryContainer));
        row.addView(badge, new LinearLayout.LayoutParams(dp(40), dp(40)));
        LinearLayout words = words(title, detail);
        words.setPadding(dp(16), 0, dp(8), 0);
        row.addView(words, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1));
        row.addView(text("›", 22, onSurfaceVariant, false));
        row.setBackground(ripple());
        row.setOnClickListener(v -> action.run());
        return row;
    }

    private LinearLayout row(String title, String help) {
        LinearLayout row = new LinearLayout(this);
        row.setGravity(Gravity.CENTER_VERTICAL);
        row.setPadding(dp(20), dp(14), dp(16), dp(14));
        row.setMinimumHeight(dp(64));
        row.addView(words(title, help), new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1));
        row.setBackground(ripple());
        return row;
    }

    private LinearLayout words(String title, String detail) {
        LinearLayout words = new LinearLayout(this);
        words.setOrientation(LinearLayout.VERTICAL);
        words.addView(text(title, 16, onSurface, false));
        if (detail != null) {
            TextView small = text(detail, 13, onSurfaceVariant, false);
            small.setPadding(0, dp(2), 0, 0);
            words.addView(small);
        }
        return words;
    }

    // ---- building blocks ----

    private void header(LinearLayout c, String title) {
        LinearLayout bar = new LinearLayout(this);
        bar.setGravity(Gravity.CENTER_VERTICAL);
        bar.setPadding(0, dp(8), 0, dp(12));
        View back = new View(this);
        back.setBackground(new IconDrawable(Icon.BACK, 0, onSurface));
        back.setForeground(rippleOval());
        back.setOnClickListener(v -> onBackPressed());
        back.setContentDescription("Back");
        bar.addView(back, new LinearLayout.LayoutParams(dp(48), dp(48)));
        TextView name = text(title, 28, onSurface, false);
        name.setPadding(dp(8), 0, 0, 0);
        bar.addView(name);
        c.addView(bar);
    }

    private TextView label(String title) {
        TextView label = text(title, 14, primary, true);
        label.setPadding(dp(16), dp(24), 0, dp(8));
        return label;
    }

    private void note(LinearLayout c) {
        TextView note = text("Changes apply the next time the game starts.", 13, onSurfaceVariant, false);
        note.setPadding(dp(16), dp(16), dp(16), 0);
        c.addView(note);
    }

    private LinearLayout card() {
        LinearLayout card = new LinearLayout(this);
        card.setOrientation(LinearLayout.VERTICAL);
        card.setBackground(rounded(surfaceContainer, dp(24)));
        card.setClipToOutline(true);
        return card;
    }

    private View divider() {
        View line = new View(this);
        line.setBackgroundColor(outlineVariant);
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 1);
        params.setMargins(dp(20), 0, dp(20), 0);
        line.setLayoutParams(params);
        return line;
    }

    private TextView button(String label, int background, int foreground) {
        TextView button = text(label, 16, foreground, true);
        button.setGravity(Gravity.CENTER);
        button.setPadding(dp(32), 0, dp(32), 0);
        button.setBackground(new RippleDrawable(ColorStateList.valueOf(withAlpha(foreground, 0.2f)),
                rounded(background, dp(16)), null));
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

    private RippleDrawable ripple() {
        return new RippleDrawable(ColorStateList.valueOf(withAlpha(onSurface, 0.12f)), null,
                new GradientDrawable());
    }

    private RippleDrawable rippleOval() {
        GradientDrawable mask = new GradientDrawable();
        mask.setShape(GradientDrawable.OVAL);
        mask.setColor(0xFFFFFFFF);
        return new RippleDrawable(ColorStateList.valueOf(withAlpha(onSurface, 0.12f)), null, mask);
    }

    private static GradientDrawable rounded(int color, float radius) {
        GradientDrawable shape = new GradientDrawable();
        shape.setColor(color);
        shape.setCornerRadius(radius);
        return shape;
    }

    private static String labelOf(String[][] options, String value) {
        for (String[] option : options) {
            if (option[0].equals(value)) {
                return option[1];
            }
        }
        return value.isEmpty() ? options[0][1] : value;
    }

    private Bitmap headerArt() {
        try (InputStream in = getAssets().open("header-art.png")) {
            return BitmapFactory.decodeStream(in);
        } catch (Exception e) {
            return null;
        }
    }

    private String appVersion() {
        try {
            return getPackageManager().getPackageInfo(getPackageName(), 0).versionName;
        } catch (Exception e) {
            return "unknown";
        }
    }

    private String portVersion() {
        String version = appVersion();
        int dash = version.indexOf('-');
        return dash > 0 ? "v" + version.substring(0, dash) : "v" + version;
    }


    private void loadPalette() {
        // Material 3 colour roles on the Android 12+ system palette (tone 0-1000 steps of each tonal palette).
        primary = color(dark ? android.R.color.system_accent1_200 : android.R.color.system_accent1_600);
        onPrimary = color(dark ? android.R.color.system_accent1_800 : android.R.color.system_accent1_0);
        primaryContainer = color(dark ? android.R.color.system_accent1_700 : android.R.color.system_accent1_100);
        onPrimaryContainer = color(dark ? android.R.color.system_accent1_100 : android.R.color.system_accent1_900);
        secondaryContainer = color(dark ? android.R.color.system_accent2_700 : android.R.color.system_accent2_100);
        onSecondaryContainer = color(dark ? android.R.color.system_accent2_100 : android.R.color.system_accent2_900);
        surface = color(dark ? android.R.color.system_neutral1_900 : android.R.color.system_neutral1_10);
        surfaceContainer = color(dark ? android.R.color.system_neutral2_800 : android.R.color.system_neutral2_50);
        surfaceContainerHigh = color(dark ? android.R.color.system_neutral2_700 : android.R.color.system_neutral2_100);
        surfaceContainerHighest = color(dark ? android.R.color.system_neutral2_600 : android.R.color.system_neutral2_200);
        onSurface = color(dark ? android.R.color.system_neutral1_100 : android.R.color.system_neutral1_900);
        onSurfaceVariant = color(dark ? android.R.color.system_neutral2_200 : android.R.color.system_neutral2_700);
        outline = color(dark ? android.R.color.system_neutral2_400 : android.R.color.system_neutral2_500);
        outlineVariant = color(dark ? android.R.color.system_neutral2_700 : android.R.color.system_neutral2_200);
    }

    private int color(int id) {
        return getColor(id);
    }

    private static int withAlpha(int color, float alpha) {
        return (Math.round(alpha * 255) << 24) | (color & 0xFFFFFF);
    }

    private int dp(float value) {
        return Math.round(TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, value,
                getResources().getDisplayMetrics()));
    }

    private int widthPx() {
        return getResources().getDisplayMetrics().widthPixels;
    }

    // ---- icons: simple line drawings in the Material Symbols spirit, no icon font or resources ----

    private enum Icon { DISPLAY, SWORD, GAMEPAD, GLOBE, INFO, CODE, BACK }

    private final class IconDrawable extends Drawable {
        private final Icon icon;
        private final int background;
        private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);

        IconDrawable(Icon icon, int background, int ink) {
            this.icon = icon;
            this.background = background;
            paint.setColor(ink);
            paint.setStyle(Paint.Style.STROKE);
            paint.setStrokeCap(Paint.Cap.ROUND);
            paint.setStrokeJoin(Paint.Join.ROUND);
        }

        @Override
        public void draw(Canvas canvas) {
            float w = getBounds().width(), h = getBounds().height(), cx = w / 2, cy = h / 2, s = Math.min(w, h) / 40f;
            if (background != 0) {
                Paint fill = new Paint(Paint.ANTI_ALIAS_FLAG);
                fill.setColor(background);
                canvas.drawCircle(cx, cy, Math.min(w, h) / 2, fill);
            }
            paint.setStrokeWidth(2f * s);
            Path p = new Path();
            switch (icon) {
                case DISPLAY:
                    canvas.drawRoundRect(new RectF(cx - 10 * s, cy - 8 * s, cx + 10 * s, cy + 5 * s), 2 * s, 2 * s, paint);
                    canvas.drawLine(cx - 4 * s, cy + 9 * s, cx + 4 * s, cy + 9 * s, paint);
                    break;
                case SWORD:
                    // Blade to a point, crossguard, grip and pommel, on the diagonal.
                    p.moveTo(cx - 3 * s, cy + 3 * s);
                    p.lineTo(cx + 7 * s, cy - 7 * s);
                    p.lineTo(cx + 9 * s, cy - 9 * s);
                    p.moveTo(cx - 7 * s, cy - 1 * s);
                    p.lineTo(cx + 1 * s, cy + 7 * s);
                    p.moveTo(cx - 3 * s, cy + 3 * s);
                    p.lineTo(cx - 7 * s, cy + 7 * s);
                    canvas.drawPath(p, paint);
                    canvas.drawCircle(cx - 8.5f * s, cy + 8.5f * s, 1.5f * s, paint);
                    break;
                case GAMEPAD:
                    canvas.drawRoundRect(new RectF(cx - 11 * s, cy - 6 * s, cx + 11 * s, cy + 7 * s), 6 * s, 6 * s, paint);
                    canvas.drawLine(cx - 7 * s, cy, cx - 3 * s, cy, paint);
                    canvas.drawLine(cx - 5 * s, cy - 2 * s, cx - 5 * s, cy + 2 * s, paint);
                    canvas.drawPoint(cx + 4 * s, cy - 1 * s, paint);
                    canvas.drawPoint(cx + 7 * s, cy + 2 * s, paint);
                    break;
                case GLOBE:
                    canvas.drawCircle(cx, cy, 9 * s, paint);
                    canvas.drawOval(new RectF(cx - 4 * s, cy - 9 * s, cx + 4 * s, cy + 9 * s), paint);
                    canvas.drawLine(cx - 9 * s, cy, cx + 9 * s, cy, paint);
                    break;
                case INFO:
                    canvas.drawCircle(cx, cy, 9 * s, paint);
                    canvas.drawLine(cx, cy - 1 * s, cx, cy + 5 * s, paint);
                    canvas.drawPoint(cx, cy - 5 * s, paint);
                    break;
                case CODE:
                    p.moveTo(cx - 3 * s, cy - 7 * s);
                    p.lineTo(cx - 10 * s, cy);
                    p.lineTo(cx - 3 * s, cy + 7 * s);
                    p.moveTo(cx + 3 * s, cy - 7 * s);
                    p.lineTo(cx + 10 * s, cy);
                    p.lineTo(cx + 3 * s, cy + 7 * s);
                    canvas.drawPath(p, paint);
                    break;
                case BACK:
                    p.moveTo(cx + 8 * s, cy);
                    p.lineTo(cx - 8 * s, cy);
                    p.moveTo(cx - 2 * s, cy - 6 * s);
                    p.lineTo(cx - 8 * s, cy);
                    p.lineTo(cx - 2 * s, cy + 6 * s);
                    canvas.drawPath(p, paint);
                    break;
            }
        }

        @Override
        public void setAlpha(int alpha) {
            paint.setAlpha(alpha);
        }

        @Override
        public void setColorFilter(ColorFilter filter) {
            paint.setColorFilter(filter);
        }

        @Override
        public int getOpacity() {
            return PixelFormat.TRANSLUCENT;
        }
    }
}
