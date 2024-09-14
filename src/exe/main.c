/** zvonilka: executor
2024, Simon Zolin */

#include <zvonilka.h>
#include <track.h>
#include <util/crash.h>
#include <util/log.h>
#include <ffsys/environ.h>
#include <ffsys/std.h>
#include <ffsys/globals.h>
#include <ffsys/dylib.h>
#include <ffbase/args.h>

struct exe {
	const phi_core*		core;
	const zvon_conn_if*	cnif;
	const zvon_call_if*	clif;
	const phi_track_if*	tkif;

	struct zzlog	log;
	fftime			time_last;
	char			log_date[32];

	u_char	debug;
	char*	cmd_line;
	uint	exit_code;
	ffstr root_dir;
	struct ffargs	cmd;
	const char*		audio_module;
	uint			buffer_length_msec;
	uint			bitrate_kbps;
	uint			bandwidth_khz;
	u_char			call_ip[16];
	uint			port;

	phi_task task_stop_all;
	void (*action)();
	zvon_conn *conn;
	zvon_call *call;

	char*	dump_file_dir;
	struct	crash_info ci;
};

static struct exe *x;

#include <exe/log.h>
#include <exe/ctl.h>
#include <exe/ver.h>
#include <exe/cmd.h>

static int conf_root_dir(const char *argv0)
{
	char fn[4096];
	const char *p;
	if (!(p = ffps_filename(fn, sizeof(fn), argv0)))
		return -1;
	ffstr path;
	if (0 > ffpath_splitpath(p, ffsz_len(p), &path, NULL))
		return -1;
	if (!ffstr_dup(&x->root_dir, path.ptr, path.len + 1))
		return -1;
	return 0;
}

static char* env_expand(const char *s)
{
	return ffenv_expand(NULL, NULL, 0, s);
}

static char* mod_loading(ffstr name)
{
	return ffsz_allocfmt("%Smod%c%S.%s"
		, &x->root_dir, FFPATH_SLASH, &name, FFDL_EXT);
}

static int core_load()
{
	struct phi_core_conf conf = {
		.log_level = (x->debug) ? PHI_LOG_EXTRA : PHI_LOG_INFO,
		.log = exe_log,
		.logv = exe_logv,
		.log_obj = &x->log,

		.env_expand = env_expand,
		.mod_loading = mod_loading,

		.workers = 0,
	};
	if (!(x->core = phi_core_create(&conf)))
		return -1;
	return 0;
}

static void stop_all(void *param)
{
	x->clif->sig(x->call, ZVON_CALL_STOP);
	if (x->cnif->sig(ZVON_CONN_STOP))
		x->core->sig(PHI_CORE_STOP);
}

static void on_sig(struct ffsig_info *i)
{
	switch (i->sig) {
	case FFSIG_INT:
		x->core->task(0, &x->task_stop_all, stop_all, x);

#ifdef FF_WIN
		if (i->flags == CTRL_CLOSE_EVENT) {
			// This is a separate signal handler thread - just wait until main() exits.
			ffthread_sleep(-1);
		}
#endif
		break;

	default:
		crash_handler(&x->ci, i);
	}
}

static void signals_subscribe(char **argv, int argc, const char *cmd_line)
{
	struct crash_info ci = {
		.app_name = "zvonilka",
		.full_name = ffsz_allocfmt("zvonilka v%s (" OS_STR "-" CPU_STR ")", x->core->version_str),
		.dump_file_dir = "/tmp",

		.argv = argv,
		.argc = argc,
		.cmd_line = cmd_line,

		.back_trace = 1,
		.print_std_err = 1,
		.strip_paths = 1,
	};

#ifdef FF_WIN
	if (!(x->dump_file_dir = ffenv_expand(NULL, NULL, 0, "%TMP%")))
		x->dump_file_dir = ffsz_dupstr(&x->root_dir);
	ci.dump_file_dir = x->dump_file_dir;
#endif

	x->ci = ci;

	static const uint sigs[] = {
		FFSIG_INT,
#ifdef PHI_DEBUG
		FFSIG_SEGV, FFSIG_ILL, FFSIG_FPE, FFSIG_ABORT,
#endif
	};
	ffsig_subscribe(on_sig, sigs, FF_COUNT(sigs));
}

static int jobs_start()
{
	x->action();
	return 0;
}

static void cleanup()
{
	phi_core_destroy();
#ifdef PHI_DEBUG
	ffmem_free(x->dump_file_dir);
	ffmem_free((char*)x->ci.full_name);
	ffmem_free(x->cmd_line);
	ffmem_free(x);
#endif
}

int main(int argc, char **argv, char **env)
{
	x = ffmem_new(struct exe);
	x->exit_code = ~0U;
	logs_init(&x->log);

	if (conf_root_dir(argv[0]))
		goto end;

#ifdef FF_WIN
	x->cmd_line = ffsz_alloc_wtou(GetCommandLineW());
#endif
	if (cmd_process(argv, argc, x->cmd_line)) goto end;

	if (core_load()) goto end;
	version_print();
	signals_subscribe(argv, argc, x->cmd_line);
	if (jobs_start()) goto end;
	phi_core_run();

end:
	{
	uint ec = x->exit_code;
	if (ec == ~0U)
		ec = 1;
	dbglog("exit code: %d", ec);
	cleanup();
	return ec;
	}
}
