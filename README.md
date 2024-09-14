# zvonilka (beta)

Free, fast, minimalistic peer-to-peer voice chat app for Windows, Linux & Android.
Based on [phiola](https://github.com/stsaz/phiola).
The current functionality is **very limited**, it requires ideal conditions (i.e. no hardware/network delays) and works best with headphones (as there is no echo cancellation).

Contents:

* Usage Example
* Build


## Usage Example

To establish a call between two devices, the first one (server) must listen for incoming calls, and the second one (client) must connect to it directly by server's IP address.

### Linux

> Note: although PulseAudio is used by default, know that ALSA module provides much lower latency.

Server:

```
$ zvonilka listen -audio alsa
Listening...
Incoming call from 192.168.1.2
Speak
```

Client:

```
$ zvonilka call -audio alsa 192.168.1.1
Calling...
Speak
```

### Android

On Android either tap on `Listen` button for server mode, or type in the target IP address and tap on `Call` button for client mode.
Tap `Disconnect` to interrupt the call.


## Build

Download the source code:

```sh
mkdir zvonilka-src
cd zvonilka-src
git clone https://github.com/stsaz/zvonilka
git clone https://github.com/stsaz/phiola
git clone https://github.com/stsaz/netmill
git clone https://github.com/stsaz/avpack
git clone https://github.com/stsaz/ffaudio
git clone https://github.com/stsaz/ffsys
git clone https://github.com/stsaz/ffbase
cd zvonilka
```

* Cross-build for Linux/AMD64:

	```sh
	bash xbuild-debianbullseye.sh
	```

* Cross-build for Windows/AMD64:

	```sh
	bash xbuild-win64.sh
	```

* Cross-build for Android/ARM64:

	```sh
	ANDROID_CLT_URL=https://dl.google.com/android/repository/commandlinetools-linux-11076708_latest.zip \
	ANDROID_HOME=/home/USER/Android \
	ANDROID_BT_VER=33.0.0 \
	ANDROID_PF_VER=33 \
	ANDROID_NDK_VER=25.1.8937393 \
	GRADLE_DIR=/home/USER/.gradle \
	CPU=arm64 \
	bash xbuild-android.sh
	```

### Build Parameters

| Parameter | Description |
| --- | --- |
| `DEBUG=1`         | Developer build (no optimization; no strip; all assertions) |
| `ASAN=1`          | Enable ASAN |
| `CFLAGS_USER=...` | Additional C/C++ compiler flags |


## External Libraries

[libopus](https://github.com/xiph/opus),
libsoxr.


## License

BSD-2
