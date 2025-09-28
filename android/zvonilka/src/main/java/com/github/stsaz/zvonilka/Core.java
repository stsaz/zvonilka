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
	int relay_port;

	int a_buffer;
	int a_quality;
	int a_gain = -1;

	String relay = "192.168.1.1";
	String name = "Callee";
	String target = "Callee";

	void normalize() {
		if (!(relay_port >= 1 && relay_port < 0xffff))
			relay_port = 21073;
		if (!(a_buffer >= 1))
			a_buffer = 200;
		if (!(a_quality >= 1))
			a_quality = 32;
		if (!(a_gain >= 0))
			a_gain = 12;
	}

	Zvonilka.Settings zvon() {
		Zvonilka.Settings s = new Zvonilka.Settings();
		s.relay_port = relay_port;
		s.a_buffer = a_buffer;
		s.a_quality = a_quality;
		s.a_gain = a_gain;
		return s;
	}

	void load(Conf c) {
		relay = c.value(Conf.RELAY_IP);
		relay_port = c.number(Conf.RELAY_PORT);
		name = c.value(Conf.NAME);
		target = c.value(Conf.TARGET);
		a_buffer = c.number(Conf.A_BUFFER);
		a_quality = c.number(Conf.A_QUALITY);
		a_gain = c.number(Conf.A_GAIN);
	}

	String conf_write() {
		return String.format(
			"relay_ip %s\n"
			+ "relay_port %d\n"
			+ "name %s\n"
			+ "target %s\n"
			+ "a_buffer %d\n"
			+ "a_quality %d\n"
			+ "a_gain %d\n"

			, relay
			, relay_port
			, name
			, target
			, a_buffer
			, a_quality
			, a_gain
			);
	}
}

class Core extends Util {
	private static Core instance;
	private int refcount;

	private static final String TAG = "zvonilka.Core";

	private String work_dir;
	private Conf conf;
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
		work_dir = ctx.getFilesDir().getPath();

		zvon = new Zvonilka(ctx.getApplicationInfo().nativeLibraryDir, ctx.getAssets());
		tq = new Handler(Looper.getMainLooper());
		gui = new GUI(this);
		settings = new Settings();

		conf = new Conf();
		if (conf.confRead(conf_file_name()))
			settings.load(conf);

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

	private String conf_file_name() { return this.work_dir + "/zvonilka-user.conf"; }

	void fin() {
		conf.confWrite(conf_file_name(), settings.conf_write().getBytes());
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
