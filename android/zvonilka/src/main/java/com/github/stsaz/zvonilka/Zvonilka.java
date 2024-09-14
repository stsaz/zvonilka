/** zvonilka/Android
2024, Simon Zolin */

package com.github.stsaz.zvonilka;

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
		int tcp_port;
		int a_buffer;
		int a_quality;
		int a_gain;
	}
	interface Ctl {
		void open(int flags);
		void close(int flags, String msg);
		void process(int flags);
	}
	native void listen(Settings settings, Ctl ctl);
	native int call(Settings settings, Ctl ctl, String target);

	native String[] listIPAddresses();
}
