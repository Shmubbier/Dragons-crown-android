package dev.dcpport.android;

import android.app.AlertDialog;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.os.BatteryManager;
import android.os.Bundle;
import android.provider.Settings;
import android.view.WindowManager;

import org.libsdl.app.SDLActivity;

/** SDLActivity for libmain.so, which links SDL3 statically (android/CMakeLists.txt). */
public class DcpActivity extends SDLActivity {
    /** Samsung Game Launcher: its Game Booster has "Pause USB Power Delivery" (bypass charging). */
    private static final String GAME_LAUNCHER = "com.samsung.android.game.gamehome";

    @Override
    protected String[] getLibraries() {
        return new String[] {"main"};
    }

    /** The settings gear in the touch overlay (android/touch_layout.h DCP_ANDROID_OPEN_SETTINGS). */
    private static final int OPEN_SETTINGS = 0x8001;

    @Override
    protected boolean onUnhandledMessage(int command, Object param) {
        if (command != OPEN_SETTINGS) {
            return super.onUnhandledMessage(command, param);
        }
        startActivity(new Intent(this, LauncherActivity.class).putExtra(LauncherActivity.EXTRA_FROM_GAME, true));
        return true;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        if (savedInstanceState == null) {
            showPowerNotice();
        }
    }

    /**
     * At startup: the game keeps the CPU and GPU busy, so recommend the charger, and point to bypass charging, which
     * runs the phone from the charger without charging the battery (less heat, less battery wear). Apps cannot turn
     * bypass charging on themselves: the button opens the system app where the player turns it on.
     */
    private void showPowerNotice() {
        final SharedPreferences prefs = getSharedPreferences("dcp", MODE_PRIVATE);
        if (prefs.getBoolean("hide_power_notice", false)) {
            return;
        }
        final Intent battery = registerReceiver(null, new IntentFilter(Intent.ACTION_BATTERY_CHANGED));
        final boolean plugged = battery != null && battery.getIntExtra(BatteryManager.EXTRA_PLUGGED, 0) != 0;
        final boolean samsung = getPackageManager().getLaunchIntentForPackage(GAME_LAUNCHER) != null;
        final StringBuilder text = new StringBuilder();
        text.append(plugged ? "Charger connected.\n\n" : "The phone is running on its battery.\n\n");
        text.append("This game keeps the processor and graphics chip busy. Keep the phone plugged in while you play: "
                + "on battery it drains quickly and the phone slows down as it heats up.\n\n");
        if (samsung) {
            text.append("For less heat, turn on bypass charging: Game Launcher > Game Booster > \"Pause USB Power "
                    + "Delivery\". The phone then runs from the charger without charging the battery. It needs a "
                    + "USB PD charger of 25 W or more, and only some models offer it.");
        } else {
            text.append("If your phone offers bypass charging (running from the charger without charging the "
                    + "battery), turn it on in its battery or game settings for less heat.");
        }
        new AlertDialog.Builder(this)
                .setTitle(plugged ? "Power" : "Plug in your phone")
                .setMessage(text)
                .setPositiveButton("OK", null)
                .setNeutralButton("Bypass charging", (dialog, which) -> openBypassSettings(samsung))
                .setNegativeButton("Don't show again",
                        (dialog, which) -> prefs.edit().putBoolean("hide_power_notice", true).apply())
                .show();
    }

    private void openBypassSettings(boolean samsung) {
        Intent intent = samsung ? getPackageManager().getLaunchIntentForPackage(GAME_LAUNCHER)
                                : new Intent(Intent.ACTION_POWER_USAGE_SUMMARY);
        try {
            startActivity(intent);
        } catch (RuntimeException e) {
            startActivity(new Intent(Settings.ACTION_SETTINGS));
        }
    }
}
