/** zvonilka/Android
2023, Simon Zolin */

package com.github.stsaz.zvonilka;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;
import androidx.core.app.NotificationCompat;

public class RecSvc extends Service {
	private static final String TAG = "zvonilka.RecSvc";
	private Core core;

	private String notification_channel_create(String id, String name) {
		String r = "";
		NotificationManager mgr = (NotificationManager)getSystemService(NOTIFICATION_SERVICE);
		if (mgr != null) {
			NotificationChannel chan = new NotificationChannel(id, name, NotificationManager.IMPORTANCE_LOW);
			mgr.createNotificationChannel(chan);
			r = id;
		}
		return r;
	}

	private Notification notification_create() {
		NotificationCompat.Builder nfy = new NotificationCompat.Builder(this
				, notification_channel_create("zvonilka.chan-work", "chanworkname"))
			.setContentTitle("zvonilka")
			.setContentText("Recording audio")
			;
		return nfy.build();
	}

	@Override
	public void onCreate() {
		if (BuildConfig.DEBUG)
			Log.d(TAG, String.format("onCreate"));

		core = Core.ref();

		final int ID = 2;
		startForeground(ID, notification_create());
	}

	@Override
	public int onStartCommand(Intent intent, int flags, int startId) {
		core.dbglog(TAG, "onStartCommand");
		return START_NOT_STICKY;
	}

	@Override
	public IBinder onBind(Intent intent) { return null; }

	@Override
	public void onDestroy() {
		core.dbglog(TAG, "onDestroy");
		core.unref();
	}
}
