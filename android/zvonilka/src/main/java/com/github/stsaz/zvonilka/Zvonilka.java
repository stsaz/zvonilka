/** zvonilka/Android
2024, Simon Zolin */

package com.github.stsaz.zvonilka;

import java.util.Arrays;

class Zvonilka {
	Zvonilka(String libdir, Object asset_mgr) {
		System.load(String.format("%s/libzvon.so", libdir));
		init(libdir, asset_mgr);
	}
	private native void init(String libdir, Object asset_mgr);
	native void destroy();
	private static boolean lib_load(String filename) {
		System.load(filename);
		return true;
	}

	native String version();

	native void disconnect();

	static class Settings {
		int relay_port;
		int a_buffer;
		int a_quality;
		int a_gain;
	}
	static final int
		CF_INCALL = 1,

		CF_CON = 2,

		CF_ERR = 1,
		CF_INTR = 2
		;
	interface Ctl {
		void open(int flags);
		void close(int flags, String msg);
		void process(int flags);
	}
	native void listen(Settings settings, Ctl ctl, String relay_ip, String name);
	native int call(Settings settings, Ctl ctl, String relay_ip, String name, String target);
}

class Conf {
	private int[] fields; // {off, len, number}...
	private byte[] data;
	String value(int i) {
		return new String(Arrays.copyOfRange(data, fields[i*3], fields[i*3] + fields[i*3 + 1]));
	}
	int number(int i) { return fields[i*3 + 2]; }
	boolean enabled(int i) { return fields[i*3 + 2] == 1; }
	void reset() {
		fields = null;
		data = null;
	}

	static final int
		A_BUFFER	= 0,
		A_GAIN		= A_BUFFER + 1,
		A_QUALITY	= A_GAIN + 1,
		NAME		= A_QUALITY + 1,
		RELAY_IP	= NAME + 1,
		RELAY_PORT	= RELAY_IP + 1,
		TARGET		= RELAY_PORT + 1
		;
	native boolean confRead(String filepath);
	native boolean confWrite(String filepath, byte[] data);
}
