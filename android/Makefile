# Android project makefile

# Undefine built-in rules, suffixes and variables
MAKEFLAGS += -Rr

# Set utils
SUBMAKE := +$(MAKE) -f $(firstword $(MAKEFILE_LIST))
MKDIR := mkdir -p
RM := rm -rf
CP := cp -au
LS := ls -l

ANDROID_HOME ?= /Android
SDK_API := 33.0.0
NDK_VER := 25.1.8937393
NDK_DIR := $(ANDROID_HOME)/ndk/$(NDK_VER)
# ADB_FLAGS := -d

CPU := amd64
LIB_DIR_CPU := x86_64
ifeq "$(CPU)" "arm64"
	LIB_DIR_CPU := arm64-v8a
else ifeq "$(CPU)" "arm"
	LIB_DIR_CPU := armeabi-v7a
endif
APK_ID := com.github.stsaz.zvonilka
APK := zvonilka/build/outputs/apk/release/zvonilka-release-unsigned.apk
ifeq "$(DEBUG)" "1"
	APK_ID := com.github.stsaz.zvonilka.debug
	APK := zvonilka/build/outputs/apk/debug/zvonilka-debug.apk
endif

default: libs
	$(SUBMAKE) apk


# Build .apk

GRADLE := ./gradlew
GRADLE_BUILD := assembleRelease
ifeq "$(DEBUG)" "1"
	GRADLE_BUILD := assembleDebug
endif

apk:
	cd $(ZVONILKA)/android && $(GRADLE) $(GRADLE_BUILD)

clean-java:
	cd $(ZVONILKA)/android && $(GRADLE) clean

apk-list:
	unzip -l $(APK)


# Sign .apk

APK_KEY_STORE :=
APK_KEY_PASS :=
APK_VER := test
APK_SIGNED := zvonilka-$(APK_VER)-android-$(CPU).apk

sign:
	cd $(ZVONILKA)/android && $(ANDROID_HOME)/build-tools/$(SDK_API)/zipalign -v -f 4 $(APK) $(APK_SIGNED)
	cd $(ZVONILKA)/android && $(ANDROID_HOME)/build-tools/$(SDK_API)/apksigner sign \
		--ks $(APK_KEY_STORE) \
		--ks-pass pass:$(APK_KEY_PASS) \
		$(APK_SIGNED)
	cd $(ZVONILKA)/android && $(ANDROID_HOME)/build-tools/$(SDK_API)/apksigner verify $(APK_SIGNED)

install-signed:
	$(ANDROID_HOME)/platform-tools/adb $(ADB_FLAGS) install -r $(APK_SIGNED)


# Upload and install .apk on Android device

install:
	$(ANDROID_HOME)/platform-tools/adb $(ADB_FLAGS) install -r $(APK)

uninstall:
	$(ANDROID_HOME)/platform-tools/adb $(ADB_FLAGS) uninstall $(APK_ID)

build-install: build
	$(MAKE) install


# Show logs

log:
	$(ANDROID_HOME)/platform-tools/adb $(ADB_FLAGS) logcat -v time | grep --color=auto --line-buffered '/zvonilka'

log-error:
	$(ANDROID_HOME)/platform-tools/adb $(ADB_FLAGS) logcat -v time | grep -E --color=auto --line-buffered '/zvonilka|W/|E/'

log-clear:
	$(ANDROID_HOME)/platform-tools/adb $(ADB_FLAGS) logcat -c

log-bt:
	$(ANDROID_HOME)/platform-tools/adb $(ADB_FLAGS) logcat -v time | $(NDK_DIR)/ndk-stack -sym zvonilka/libs/$(LIB_DIR_CPU)


# Directories
ROOT_DIR := ../..
ZVONILKA := $(ROOT_DIR)/zvonilka
PHIOLA := $(ROOT_DIR)/phiola
FFBASE := $(ROOT_DIR)/ffbase
FFSYS := $(ROOT_DIR)/ffsys

include $(FFBASE)/conf.mk
include andk.mk
SYS := android

LIB_DIR_AMD64 := $(ZVONILKA)/android/zvonilka/libs/x86_64
LIB_DIR_ARM64 := $(ZVONILKA)/android/zvonilka/libs/arm64-v8a
LIB_DIR_ARM := $(ZVONILKA)/android/zvonilka/libs/armeabi-v7a

clean-lib:
	$(RM) ../_android-arm64 ../_android-amd64 $(LIB_DIR_AMD64) $(LIB_DIR_ARM64)


CFLAGS := $(A_CFLAGS)
CFLAGS += -DFFBASE_MEM_ASSERT
CFLAGS += -MMD -MP
CFLAGS += -I$(FFBASE)
CFLAGS += -Wall -Wextra -Wno-unused-parameter -Wno-for-loop-analysis
CFLAGS += -g
CFLAGS += -fPIC
ifeq "$(DEBUG)" "1"
	CFLAGS += -DFF_DEBUG -O0 -Werror -Wno-deprecated-declarations
else
	CFLAGS += -O3 -fno-strict-aliasing -fvisibility=hidden
endif
ifdef PHI_VERSION_STR
	CFLAGS += -DPHI_VERSION_STR=\"$(PHI_VERSION_STR)\"
endif
CFLAGS += $(CFLAGS_USER)
CFLAGS_BASE := $(CFLAGS)
CFLAGS += -I$(ZVONILKA)/src -I$(PHIOLA)/src -I$(NETMILL)/src -I$(FFSYS)
LINKFLAGS := $(A_LINKFLAGS)

%.o: $(FFBASE)/ffbase/%.c
	$(C) $(CFLAGS) -DFFBASE_OPT_SIZE $< -o $@

-include $(wildcard *.d)

include $(ZVONILKA)/src/core/Makefile
include $(ZVONILKA)/src/format.mk
include $(ZVONILKA)/src/acodec.mk
include $(PHIOLA)/src/adev/Makefile
include $(PHIOLA)/src/afilter/Makefile

LIB_DIR := $(ZVONILKA)/android/zvonilka/libs/$(LIB_DIR_CPU)
ifeq "$(TARGETS)" ""
override TARGETS := libzvon.so $(MODS)
endif
libs:
	$(MKDIR) $(LIB_DIR)
	$(SUBMAKE) strip-debug
	cp -u $(LIBS3) *.so *.debug \
		$(LIB_DIR)
	bash ../android/build-rename-libs.sh $(LIB_DIR) $(MODS)


strip-debug: $(addsuffix .debug,$(TARGETS))
%.debug: %
	$(OBJCOPY) --only-keep-debug $< $@
	$(STRIP) $<
	$(OBJCOPY) --add-gnu-debuglink=$@ $<
	touch $@

PKG_ARCH := $(CPU)
PKG_PACKER := tar -c --owner=0 --group=0 --numeric-owner -v --zstd -f
PKG_EXT := tar.zst
ifeq "$(OS)" "windows"
	PKG_PACKER := zip -r -v
	PKG_EXT := zip
endif

PKG_DEBUG_NAME := zvonilka-$(APK_VER)-android-$(PKG_ARCH)-debug.$(PKG_EXT)
$(PKG_DEBUG_NAME): $(wildcard $(ZVONILKA)/android/zvonilka/libs/$(LIB_DIR_CPU)/*.debug)
	cd $(ZVONILKA) && $(PKG_PACKER) $@ android/zvonilka/libs/$(LIB_DIR_CPU)/*.debug
package-debug: $(PKG_DEBUG_NAME)

release: default
	$(SUBMAKE) package-debug
	$(SUBMAKE) sign
