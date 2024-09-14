/** phiola: format module */

#include <track.h>
#include <util/util.h>

const struct phi_core *core;
#define errlog(t, ...)  phi_errlog(core, NULL, t, __VA_ARGS__)
#define warnlog(t, ...)  phi_warnlog(core, NULL, t, __VA_ARGS__)
#define dbglog(t, ...)  phi_dbglog(core, NULL, t, __VA_ARGS__)

#include <format/meta.h>

extern const phi_filter
	phi_ogg_write,
	phi_ogg_read,
	phi_opusmeta_read;

static const void* fmt_mod_iface(const char *name)
{
	static const struct map_sz_vptr mods[] = {
		{ "meta",		&phi_metaif },
		{ "ogg",		&phi_ogg_read },
		{ "ogg-write",	&phi_ogg_write },
		{ "opusmeta",	&phi_opusmeta_read },
	};
	return map_sz_vptr_findz2(mods, FF_COUNT(mods), name);
}

static const phi_mod phi_mod_fmt = {
	.ver = PHI_VERSION, .ver_core = PHI_VERSION_CORE,
	.iface = fmt_mod_iface,
};

FF_EXPORT const phi_mod* phi_mod_init(const struct phi_core *_core)
{
	core = _core;
	return &phi_mod_fmt;
}
