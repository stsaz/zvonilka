# zvonilka executor

# Inherit:
# C
# CFLAGS
# DOTEXE
# LINK
# LINK_DL
# LINK_RPATH_ORIGIN
# LINKFLAGS
# OS
# SO
# WINDRES
# ZVONILKA

# Modify:
# EXES

EXES += zvonilka$(DOTEXE)

%.o: $(ZVONILKA)/src/exe/%.c
	$(C) $(CFLAGS) $< -o $@

_:=
ifeq "$(OS)" "windows"
	EXE_COFF := exe.coff
endif
exe.coff: $(ZVONILKA)/res/exe.rc \
		$(ZVONILKA)/res/exe.manifest
	$(WINDRES) $< $@

zvonilka$(DOTEXE): main.o \
		$(EXE_COFF) \
		libzvon.$(SO)
	$(LINK) $+ $(LINKFLAGS) $(LINK_RPATH_ORIGIN) $(LINK_DL) -o $@
