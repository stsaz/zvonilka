/** zvonilka/Android
2024, Simon Zolin */

package com.github.stsaz.zvonilka;

import androidx.annotation.NonNull;

import android.content.Context;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

class Settings {
	int tcp_port;

	int a_buffer;
	int a_quality;
	int a_gain = -1;

	String target = "192.168.1.1";

	void normalize() {
		if (!(tcp_port >= 1 && tcp_port < 0xffff))
			tcp_port = 21073;
		if (!(a_buffer >= 1))
			a_buffer = 200;
		if (!(a_quality >= 1))
			a_quality = 32;
		if (!(a_gain >= 0))
			a_gain = 12;
	}

	Zvonilka.Settings zvon() {
		Zvonilka.Settings s = new Zvonilka.Settings();
		s.tcp_port = tcp_port;
		s.a_buffer = a_buffer;
		s.a_quality = a_quality;
		s.a_gain = a_gain;
		return s;
	}
}

class Core extends Util {
	private static Core instance;
	private int refcount;

	private static final String TAG = "zvonilka.Core";

	GUI gui;
	Zvonilka zvon;
	Handler tq;
	Settings settings;
	int state;
	int state2;

	Context context;

	static Core ref() {
		instance.dbglog(TAG, "ref");
		instance.refcount++;
		return instance;
	}

	static Core init_once(Context ctx) {
		if (instance == null) {
			Core c = new Core();
			c.refcount = 1;
			if (0 != c.init(ctx))
				return null;
			instance = c;
			return c;
		}
		return ref();
	}

	private int init(@NonNull Context ctx) {
		dbglog(TAG, "init");
		context = ctx;

		zvon = new Zvonilka(ctx.getApplicationInfo().nativeLibraryDir, ctx.getAssets());
		tq = new Handler(Looper.getMainLooper());
		gui = new GUI(this);
		settings = new Settings();
		settings.normalize();
		return 0;
	}

	void unref() {
		dbglog(TAG, "unref(): %d", refcount);
		refcount--;
	}

	void close() {
		dbglog(TAG, "close(): %d", refcount);
		if (--refcount != 0)
			return;
		instance = null;
		zvon.destroy();
	}

	void errlog(String mod, String fmt, Object... args) {
		Log.e(mod, String.format("%s: %s", mod, String.format(fmt, args)));
		if (gui != null)
			gui.on_error(fmt, args);
	}

	void dbglog(String mod, String fmt, Object... args) {
		if (BuildConfig.DEBUG)
			Log.d(mod, String.format("%s: %s", mod, String.format(fmt, args)));
	}
}
