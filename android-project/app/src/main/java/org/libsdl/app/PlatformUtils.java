package org.libsdl.app;

import android.annotation.TargetApi;
import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.NetworkInfo;
import android.net.Uri;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.BatteryManager;
import android.os.Build;
import android.os.Message;
import android.provider.Settings;
import android.util.Log;
import android.view.DisplayCutout;
import android.view.RoundedCorner;
import android.view.View;
import android.view.Window;
import android.view.WindowInsets;
import android.view.WindowManager;

public class PlatformUtils {
    public static boolean isBatterySupported() {
        Context context = SDLActivity.getContext();
        Intent batteryIntent = context.registerReceiver(null, new IntentFilter(Intent.ACTION_BATTERY_CHANGED));
        return batteryIntent != null;
    }

    public static int getBatteryLevel() {
        Context context = SDLActivity.getContext();

        Intent batteryIntent = context.registerReceiver(null, new IntentFilter(Intent.ACTION_BATTERY_CHANGED));
        if (batteryIntent == null) {
            return 0;
        }
        int level = batteryIntent.getIntExtra(BatteryManager.EXTRA_LEVEL, -1);
        int scale = batteryIntent.getIntExtra(BatteryManager.EXTRA_SCALE, -1);

        if (level >= 0 && scale > 0) {
            return (level * 100) / scale;
        }

        return 0;
    }

    public static boolean isBatteryCharging() {
        Context context = SDLActivity.getContext();

        IntentFilter filter = new IntentFilter(Intent.ACTION_BATTERY_CHANGED);
        Intent batteryStatus = context.registerReceiver(null, filter);

        int status = batteryStatus.getIntExtra(BatteryManager.EXTRA_STATUS, -1);
        return status == BatteryManager.BATTERY_STATUS_CHARGING ||
                status == BatteryManager.BATTERY_STATUS_FULL;
    }

    public static boolean isEthernetConnected() {
        Context context = SDLActivity.getContext();

        ConnectivityManager connectivityManager = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
        Network[] networks = connectivityManager.getAllNetworks();
        for (Network network : networks) {
            NetworkCapabilities capabilities = connectivityManager.getNetworkCapabilities(network);
            if (capabilities != null && capabilities.hasTransport(NetworkCapabilities.TRANSPORT_ETHERNET)) {
                return true;
            }
        }
        return false;
    }

    public static boolean isWifiSupported() {
        Context context = SDLActivity.getContext();

        WifiManager wifiManager = (WifiManager) context.getApplicationContext().getSystemService(Context.WIFI_SERVICE);
        return wifiManager != null && wifiManager.isWifiEnabled();
    }

    public static boolean isWifiConnected() {
        Context context = SDLActivity.getContext();

        ConnectivityManager connectivityManager = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo wifiInfo = connectivityManager.getNetworkInfo(ConnectivityManager.TYPE_WIFI);
        return wifiInfo != null && wifiInfo.isConnected();
    }

    public static int getWifiSignalStrength() {
        Context context = SDLActivity.getContext();

        WifiManager wifiManager = (WifiManager) context.getApplicationContext().getSystemService(Context.WIFI_SERVICE);
        WifiInfo wifiInfo = wifiManager.getConnectionInfo();
        return wifiInfo.getRssi();
    }

    public static void openBrowser(String url) {
        Context context = SDLActivity.getContext();

        Uri webpage = Uri.parse(url);
        Intent intent = new Intent(Intent.ACTION_VIEW, webpage);
        // Not resolveActivity(): from API 30 it sees no browser without a
        // <queries> entry
        try {
            context.startActivity(intent);
        } catch (ActivityNotFoundException e) {
            Log.w("PlatformUtils", "no app opens " + webpage.getScheme() + " links");
        }
    }

    public static float getSystemScreenBrightness(Context context) {
        ContentResolver contentResolver = context.getContentResolver();
        return Settings.System.getInt(contentResolver,
                Settings.System.SCREEN_BRIGHTNESS, 125) * 1.0f / 255.0f;
    }

    public static AuroreHandler auroreHandler = null;

    public static void setAppScreenBrightness(Activity activity, float value) {
        Message message = Message.obtain();
        message.obj = activity;
        message.arg1 = (int)(value * 255);
        message.what = 0;
        if(auroreHandler != null) auroreHandler.sendMessage(message);
    }

    public static float getAppScreenBrightness(Activity activity) {
        Window window = activity.getWindow();
        WindowManager.LayoutParams lp = window.getAttributes();
        if (lp.screenBrightness < 0) return getSystemScreenBrightness(activity);
        return lp.screenBrightness;
    }

    // The safe area over the SDL surface, in pixels: top, right, bottom,
    // left. The system bars and the display cutout, not the keyboard; and the
    // rounded corners. Called from the SDL thread, as the other getters.
    public static int[] getSafeAreaInsets(Activity activity) {
        int[] result = new int[4];
        if (activity == null || Build.VERSION.SDK_INT < Build.VERSION_CODES.M) return result;
        Window window = activity.getWindow();
        if (window == null) return result;
        View decor = window.getDecorView();
        WindowInsets insets = decor.getRootWindowInsets();
        if (insets == null) return result;

        int top, right, bottom, left;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            android.graphics.Insets bars = insets.getInsets(
                    WindowInsets.Type.systemBars() | WindowInsets.Type.displayCutout());
            top = bars.top;
            right = bars.right;
            bottom = bars.bottom;
            left = bars.left;
        } else {
            // The system window's insets have the keyboard in them, the
            // stable ones the bars even hidden: the smaller of the two
            top = Math.min(insets.getSystemWindowInsetTop(), insets.getStableInsetTop());
            right = Math.min(insets.getSystemWindowInsetRight(), insets.getStableInsetRight());
            bottom = Math.min(insets.getSystemWindowInsetBottom(), insets.getStableInsetBottom());
            left = Math.min(insets.getSystemWindowInsetLeft(), insets.getStableInsetLeft());
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
                DisplayCutout cutout = insets.getDisplayCutout();
                if (cutout != null) {
                    top = Math.max(top, cutout.getSafeInsetTop());
                    right = Math.max(right, cutout.getSafeInsetRight());
                    bottom = Math.max(bottom, cutout.getSafeInsetBottom());
                    left = Math.max(left, cutout.getSafeInsetLeft());
                }
            }
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            int[] corners = roundedCornerInsets(insets);
            top = Math.max(top, Math.max(corners[0], corners[1]));
            right = Math.max(right, Math.max(corners[1], corners[2]));
            bottom = Math.max(bottom, Math.max(corners[2], corners[3]));
            left = Math.max(left, Math.max(corners[3], corners[0]));
        }

        // The insets are the window's: over the SDL surface only
        int width = decor.getWidth(), height = decor.getHeight();
        int x = 0, y = 0, w = width, h = height;
        View content = SDLActivity.getContentView();
        if (content != null && content.getWidth() > 0 && content.getHeight() > 0) {
            int[] at = new int[2];
            content.getLocationInWindow(at);
            x = at[0];
            y = at[1];
            w = content.getWidth();
            h = content.getHeight();
        }
        result[0] = Math.max(0, top - y);
        result[1] = Math.max(0, right - (width - (x + w)));
        result[2] = Math.max(0, bottom - (height - (y + h)));
        result[3] = Math.max(0, left - x);
        return result;
    }

    // Top left, top right, bottom right, bottom left: how far in from both
    // of its edges a rectangle's corner stays inside a rounded corner of
    // radius r, r (1 - 1/sqrt 2)
    @TargetApi(Build.VERSION_CODES.S)
    private static int[] roundedCornerInsets(WindowInsets insets) {
        int[] positions = { RoundedCorner.POSITION_TOP_LEFT, RoundedCorner.POSITION_TOP_RIGHT,
                RoundedCorner.POSITION_BOTTOM_RIGHT, RoundedCorner.POSITION_BOTTOM_LEFT };
        int[] result = new int[4];
        for (int i = 0; i < 4; i++) {
            RoundedCorner corner = insets.getRoundedCorner(positions[i]);
            if (corner != null)
                result[i] = (int) Math.ceil(corner.getRadius() * (1.0 - Math.sqrt(0.5)));
        }
        return result;
    }
}