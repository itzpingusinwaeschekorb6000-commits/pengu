package app.pengu.island;

import android.Manifest;
import android.app.Activity;
import android.app.AlarmManager;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.os.Build;

import org.json.JSONArray;
import org.json.JSONObject;

// Pengu's reminders on Android. The C++ side (src/reminders.cpp) decides what
// to remind about and when, for the coming days; this class hands those
// times to the AlarmManager so they arrive even when Pengu is closed, and
// keeps them in SharedPreferences to plan them again after a reboot.
public final class Reminders {
    static final String CHANNEL = "reminders";
    private static final String PREFS = "pengu_reminders";
    private static final String KEY_PLAN = "plan";
    private static final String KEY_COUNT = "count";

    private Reminders() {}

    // Called from C++ with a JSON array of {at: epoch ms, title, body}.
    public static void schedule(Context context, String json) {
        SharedPreferences prefs = context.getSharedPreferences(PREFS, Context.MODE_PRIVATE);
        prefs.edit().putString(KEY_PLAN, json).apply();
        apply(context, json);
    }

    // After a reboot alarms are gone; plan them again from the saved list.
    static void restore(Context context) {
        String json = context.getSharedPreferences(PREFS, Context.MODE_PRIVATE).getString(KEY_PLAN, "[]");
        apply(context, json);
    }

    private static void apply(Context context, String json) {
        AlarmManager alarms = (AlarmManager) context.getSystemService(Context.ALARM_SERVICE);
        SharedPreferences prefs = context.getSharedPreferences(PREFS, Context.MODE_PRIVATE);
        // Cancel what the previous plan set up; request codes are 0..count-1.
        int previous = prefs.getInt(KEY_COUNT, 0);
        for (int i = 0; i < previous; i++) {
            PendingIntent old = PendingIntent.getBroadcast(context, i, new Intent(context, ReminderReceiver.class),
                    PendingIntent.FLAG_NO_CREATE | PendingIntent.FLAG_IMMUTABLE);
            if (old != null) {
                alarms.cancel(old);
                old.cancel();
            }
        }
        int count = 0;
        try {
            JSONArray items = new JSONArray(json);
            long now = System.currentTimeMillis();
            for (int i = 0; i < items.length(); i++) {
                JSONObject item = items.getJSONObject(i);
                long at = item.getLong("at");
                if (at <= now)
                    continue;
                Intent intent = new Intent(context, ReminderReceiver.class)
                        .putExtra("title", item.optString("title"))
                        .putExtra("body", item.optString("body"))
                        .putExtra("id", (int) (at / 60000));
                PendingIntent pending = PendingIntent.getBroadcast(context, count, intent,
                        PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE);
                // Inexact but allowed while the phone dozes; needs no special
                // permission, and a few minutes late is fine for a reminder.
                alarms.setAndAllowWhileIdle(AlarmManager.RTC_WAKEUP, at, pending);
                count++;
            }
        } catch (Exception e) {
            // A broken plan schedules nothing; the next change plans again.
        }
        prefs.edit().putInt(KEY_COUNT, count).apply();
    }

    static void ensureChannel(Context context) {
        if (Build.VERSION.SDK_INT < 26)
            return;
        NotificationManager manager = context.getSystemService(NotificationManager.class);
        if (manager.getNotificationChannel(CHANNEL) == null)
            manager.createNotificationChannel(new NotificationChannel(CHANNEL, "Erinnerungen",
                    NotificationManager.IMPORTANCE_DEFAULT));
    }

    // Shows a notification right away (the "notifications are on" test).
    public static void show(Context context, String title, String body) {
        ReminderReceiver.notify(context, title, body, (int) (System.currentTimeMillis() / 1000));
    }

    // Android 13+ asks the user once before an app may post notifications.
    public static void requestPermission(Context context) {
        if (Build.VERSION.SDK_INT < 33 || !(context instanceof Activity))
            return;
        Activity activity = (Activity) context;
        if (activity.checkSelfPermission(Manifest.permission.POST_NOTIFICATIONS) != PackageManager.PERMISSION_GRANTED)
            activity.runOnUiThread(() -> activity.requestPermissions(
                    new String[] { Manifest.permission.POST_NOTIFICATIONS }, 7001));
    }
}
