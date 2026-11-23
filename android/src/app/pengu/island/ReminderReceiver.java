package app.pengu.island;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Build;

// Receives Pengu's alarms and shows the reminder; after a reboot it plans
// the saved reminders again.
public class ReminderReceiver extends BroadcastReceiver {
    @Override
    public void onReceive(Context context, Intent intent) {
        if (Intent.ACTION_BOOT_COMPLETED.equals(intent.getAction())
                || Intent.ACTION_MY_PACKAGE_REPLACED.equals(intent.getAction())) {
            Reminders.restore(context);
            return;
        }
        notify(context, intent.getStringExtra("title"), intent.getStringExtra("body"), intent.getIntExtra("id", 0));
    }

    static void notify(Context context, String title, String body, int id) {
        Reminders.ensureChannel(context);
        // Tapping the reminder opens Pengu.
        Intent launch = context.getPackageManager().getLaunchIntentForPackage(context.getPackageName());
        PendingIntent open = launch == null ? null
                : PendingIntent.getActivity(context, 0, launch, PendingIntent.FLAG_IMMUTABLE);
        Notification.Builder builder = Build.VERSION.SDK_INT >= 26
                ? new Notification.Builder(context, Reminders.CHANNEL)
                : new Notification.Builder(context);
        builder.setSmallIcon(R.drawable.ic_notification)
               .setContentTitle(title)
               .setContentText(body)
               .setStyle(new Notification.BigTextStyle().bigText(body))
               .setAutoCancel(true);
        if (open != null)
            builder.setContentIntent(open);
        NotificationManager manager = (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
        try {
            manager.notify(id, builder.build());
        } catch (SecurityException e) {
            // Notifications not allowed: nothing to show.
        }
    }
}
