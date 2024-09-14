/** zvonilka: executor
2024, Simon Zolin */

#include <ffsys/std.h>
#include <ffsys/path.h>
#include <ffsys/dirscan.h>

static void help_info_write(const char *sz)
{
	ffstr s = FFSTR_INITZ(sz), l, k;
	ffvec v = {};

	const char *clr = FFSTD_CLR_B(FFSTD_PURPLE);
	while (s.len) {
		ffstr_splitby(&s, '`', &l, &s);
		ffstr_splitby(&s, '`', &k, &s);
		if (x->log.use_color) {
			ffvec_addfmt(&v, "%S%s%S%s"
				, &l, clr, &k, FFSTD_CLR_RESET);
		} else {
			ffvec_addfmt(&v, "%S%S"
				, &l, &k);
		}
	}

	ffstdout_write(v.ptr, v.len);
	ffvec_free(&v);
}

static int root_help()
{
	help_info_write("\
Usage:\n\
    zvonilka [GLOBAL-OPTIONS] COMMAND [OPTIONS]\n\
\n\
Global options:\n\
  `-Debug`        Print debug log messages\n\
\n\
Commands:\n\
  `call`     Call\n\
  `listen`   Listen for incoming calls\n\
\n\
'zvonilka COMMAND -help' will print information on a particular command.\n\
");
	x->exit_code = 0;
	return 1;
}

static int usage()
{
	help_info_write("\
Usage:\n\
	zvonilka [GLOBAL-OPTIONS] COMMAND [OPTIONS]\n\
Run `zvonilka -help` for complete help info.\n\
");
	return 1;
}

static struct zvon_conn_conf conn_init()
{
	x->cnif = x->core->mod("core.conn");
	x->clif = x->core->mod("core.call");
	struct zvon_conn_conf conf = {
		.port = x->port,

		.audio_module = x->audio_module,
		.buffer_length_msec = x->buffer_length_msec,
		.bitrate_kbps = x->bitrate_kbps,
		.bandwidth_khz = x->bandwidth_khz,

		.controller = &exe_ctl,
		.opaque = NULL,
	};
	return conf;
}

#include <exe/call.h>
#include <exe/listen.h>

#define O(m)  (void*)FF_OFF(struct exe, m)
static const struct ffarg cmd_root[] = {
	{ "-Debug",		'1',		O(debug) },

	{ "-help",		0,			root_help },

	{ "call",		'>',		cmd_call },
	{ "listen",		'>',		cmd_listen },
	{ "",			0,			usage },
};
#undef O

static int cmd_process(char **argv, uint argc, const char *cmd_line)
{
	uint f = FFARGS_O_PARTIAL | FFARGS_O_DUPLICATES | FFARGS_O_SKIP_FIRST;
	int r;

#ifdef FF_WIN
	r = ffargs_process_line(&x->cmd, cmd_root, x, f, cmd_line);
#else
	r = ffargs_process_argv(&x->cmd, cmd_root, x, f, argv, argc);
#endif

	if (r < 0)
		errlog("%s", x->cmd.error);

	return r;
}
