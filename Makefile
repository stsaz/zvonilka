# phiola Makefile

ROOT_DIR := ..
ZVONILKA := $(ROOT_DIR)/zvonilka
PHIOLA := $(ROOT_DIR)/phiola
NETMILL := $(ROOT_DIR)/netmill
FFSYS := $(ROOT_DIR)/ffsys
FFBASE := $(ROOT_DIR)/ffbase
APP_DIR := zvonilka-0

include $(FFBASE)/conf.mk
SYS := $(OS)

# COMPILER

CFLAGS += -DFFBASE_MEM_ASSERT \
	-MMD -MP \
	-I$(FFBASE) \
	-Wall -Wextra -Wno-unused-parameter -Wno-multichar \
	-fPIC -fvisibility=hidden \
	-g
ifeq "$(DEBUG)" "1"
	CFLAGS += -DFF_DEBUG -O0 -Werror -Wno-deprecated-declarations
else
	CFLAGS += -O3 -fno-strict-aliasing
endif
ifeq "$(ASAN)" "1"
	CFLAGS += -fsanitize=address
	LINKFLAGS += -fsanitize=address
endif
CFLAGS += $(CFLAGS_USER)
CFLAGS_BASE := $(CFLAGS)
CFLAGS += -I$(ZVONILKA)/src -I$(PHIOLA)/src -I$(NETMILL)/src -I$(FFSYS)
CXXFLAGS := -std=c++11 $(CFLAGS) -fno-exceptions -fno-rtti
CFLAGS := -std=c99 $(CFLAGS)
LINKFLAGS += $(LINKFLAGS_USER)
ifeq "$(OS)" "windows"
	LINKFLAGS += -lws2_32
endif
LINK_DL :=
ifeq "$(OS)" "linux"
	LINK_DL := -ldl
endif

# MODULES

default: build
ifneq "$(DEBUG)" "1"
	$(SUBMAKE) strip-debug
endif
	$(SUBMAKE) app

-include $(wildcard *.d)

%.o: $(PHIOLA)/src/%.c
	$(C) $(CFLAGS) $< -o $@
%.o: $(FFBASE)/ffbase/%.c
	$(C) $(CFLAGS) -DFFBASE_OPT_SIZE $< -o $@

EXES :=
MODS :=

include $(ZVONILKA)/src/core/Makefile
include $(ZVONILKA)/src/exe/Makefile
include $(ZVONILKA)/src/format.mk
include $(ZVONILKA)/src/acodec.mk

include $(PHIOLA)/src/adev/Makefile
include $(PHIOLA)/src/afilter/Makefile

# MISC

ifeq "$(OS)" "linux"
MODS += dbus.$(SO)
sys-sleep-dbus.o: $(PHIOLA)/src/sys-sleep-dbus.c
	$(C) $(CFLAGS) `pkg-config --cflags dbus-1` $< -o $@
dbus.$(SO): sys-sleep-dbus.o
	$(LINK) -shared $+ $(LINKFLAGS) -ldbus-1 -o $@
endif

ifeq "$(TARGETS)" ""
override TARGETS := libzvon.$(SO) $(EXES) $(MODS)
endif
build: $(TARGETS)

strip-debug: $(addsuffix .debug,$(TARGETS))
%.debug: %
	$(OBJCOPY) --only-keep-debug $< $@
	$(STRIP) $<
	$(OBJCOPY) --add-gnu-debuglink=$@ $<
	touch $@

app:
	$(MKDIR) $(APP_DIR) $(APP_DIR)/mod
	$(CP) zvonilka$(DOTEXE) libzvon.$(SO) \
		$(APP_DIR)/
	chmod 644 $(APP_DIR)/libzvon.$(SO)
	$(CP) $(ZVONILKA)/LICENSE \
		$(ZVONILKA)/README.md \
		$(APP_DIR)/

	$(CP) $(MODS) $(APP_DIR)/mod/
ifneq "$(LIBS3)" ""
	$(CP) $(LIBS3) $(APP_DIR)/mod/
endif
	chmod 644 $(APP_DIR)/mod/*.$(SO)

ifeq "$(OS)" "windows"
	mv $(APP_DIR)/README.md $(APP_DIR)/README.txt
	unix2dos $(APP_DIR)/README.txt
endif

PKG_VER := test
PKG_ARCH := $(CPU)
PKG_PACKER := tar -c --owner=0 --group=0 --numeric-owner -v --zstd -f
PKG_EXT := tar.zst
ifeq "$(OS)" "windows"
	PKG_PACKER := zip -r -v
	PKG_EXT := zip
endif
PKG_NAME := zvonilka-$(PKG_VER)-$(OS)-$(PKG_ARCH).$(PKG_EXT)
package: $(PKG_NAME)
$(PKG_NAME): $(APP_DIR)
	$(PKG_PACKER) $@ $<

PKG_DEBUG_NAME := zvonilka-$(PKG_VER)-$(OS)-$(PKG_ARCH)-debug.$(PKG_EXT)
$(PKG_DEBUG_NAME):
	$(PKG_PACKER) $@ *.debug
package-debug: $(PKG_DEBUG_NAME)

release: default
	$(SUBMAKE) package
	$(SUBMAKE) package-debug
