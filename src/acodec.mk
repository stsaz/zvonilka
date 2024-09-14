# zvonilka audio codecs

AVPACK := $(ROOT_DIR)/avpack
ALIB3 := $(PHIOLA)/alib3
ALIB3_BIN := $(ALIB3)/_$(SYS)-$(CPU)

MODS += opus.$(SO)
LIBS3 += $(ALIB3_BIN)/libopus-phi.$(SO)
opus.o: $(PHIOLA)/src/acodec/opus.c
	$(C) $(CFLAGS) -I$(ALIB3) -I$(AVPACK) $< -o $@
opus.$(SO): opus.o
	$(LINK) -shared $+ $(LINKFLAGS) $(LINK_RPATH_ORIGIN) -L$(ALIB3_BIN) -lopus-phi -o $@
