# zvonilka file formats

MODS += format.$(SO)

%.o: $(PHIOLA)/src/format/%.c
	$(C) $(CFLAGS_BASE) -DFFBASE_OPT_SIZE -I$(PHIOLA)/src -I$(AVPACK) -I$(FFSYS) $< -o $@

fmt.o: $(ZVONILKA)/src/fmt.c
	$(C) $(CFLAGS_BASE) -DFFBASE_OPT_SIZE -I$(PHIOLA)/src -I$(AVPACK) -I$(FFSYS) $< -o $@

format.$(SO): fmt.o \
		ogg.o \
		\
		sort.o \
		str-format.o
	$(LINK) -shared $+ $(LINKFLAGS) -o $@
