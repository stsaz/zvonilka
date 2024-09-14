# zvonilka Core

# Inherit:
# C
# CFLAGS
# LINK
# LINK_DL
# LINK_PTHREAD
# LINKFLAGS
# OS
# PHIOLA
# ZVONILKA
# SO
# SYS

NETMILL := $(ROOT_DIR)/netmill

LINKFLAGS_CORE := $(LINKFLAGS)
CORE_O := \
	core.o \
	call.o \
	tcp-listener.o \
	track.o
ifeq "$(OS)" "windows"
	CORE_O += sys-sleep-win.o
endif
ifeq "$(SYS)" "android"
	CORE_O += jni.o \
		nif.o
	LINKFLAGS_CORE += -llog -landroid
endif
CORE_O += \
	sort.o \
	str-format.o

libzvon.$(SO): $(CORE_O)
	$(LINK) -shared $+ $(LINKFLAGS_CORE) $(LINK_PTHREAD) $(LINK_DL) -o $@

CFLAGS_CORE := $(CFLAGS) -DFFBASE_OPT_SIZE
ifdef ZVON_VER_STR
	CFLAGS_CORE += -DZVON_VER_STR=\"$(ZVON_VER_STR)\"
endif

%.o: $(ZVONILKA)/src/core/%.c
	$(C) $(CFLAGS_CORE) -I$(NETMILL)/src $< -o $@

%.o: $(ZVONILKA)/src/jni/%.c
	$(C) $(CFLAGS_CORE) -I$(NETMILL)/src $< -o $@

%.o: $(NETMILL)/src/core/%.c
	$(C) $(CFLAGS_BASE) -DFFBASE_OPT_SIZE -I$(NETMILL)/src -I$(FFSYS) $< -o $@

%.o: $(NETMILL)/src/%.c
	$(C) $(CFLAGS_BASE) -DFFBASE_OPT_SIZE -I$(NETMILL)/src -I$(FFSYS) $< -o $@

%.o: $(PHIOLA)/src/core/%.c
	$(C) $(CFLAGS_BASE) -DFFBASE_OPT_SIZE -I$(PHIOLA)/src -I$(FFSYS) $< -o $@
