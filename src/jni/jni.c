/** zvonilka/Android
2024, Simon Zolin */

#include <zvonilka.h>
#include <track.h>
#include <util/jni-helper.h>
#include <util/util.h>
#include <android/log.h>

#define PJC_ZVONILKA  "com/github/stsaz/zvonilka/Zvonilka"

struct zvonilka_jni {
	phi_core *core;
	const zvon_call_if *clif;
	const zvon_conn_if *cnif;

	ffstr dir_libs;
	ffbyte debug;

	zvon_conn *conn;
	struct zvon_conn_conf conf;

	jclass Zvonilka_class;
	jmethodID Zvonilka_lib_load;

	jobject Zvonilka_Ctl_obj;
	jmethodID Zvonilka_Ctl_open;
	jmethodID Zvonilka_Ctl_close;
	jmethodID Zvonilka_Ctl_process;
};
static struct zvonilka_jni *x;
static JavaVM *jvm;

struct core_data {
	phi_task task;
	uint cmd;
	int64 param_int;
	phi_queue_id q;
};

static void core_task(struct core_data *d, void (*func)(struct core_data*))
{
	x->core->task(0, &d->task, (void(*)(void*))func, d);
}

#include <jni/log.h>
#include <jni/ctl.h>

static void conf()
{
#ifdef FF_DEBUG
	x->debug = 1;
#endif
}

/** Load modules via Java before dlopen().
Some modules also have the dependencies - load them too. */
static char* mod_loading(ffstr name)
{
	int e = -1;
	char* znames[2] = {};

	static const struct map_sz_vptr mod_deps[] = {
		{ "opus",	"libopus-phi" },
		{ "soxr",	"libsoxr-phi" },
		{}
	};
	const char *dep = map_sz_vptr_findstr(mod_deps, FF_COUNT(mod_deps), name);
	znames[0] = ffsz_allocfmt("%S/lib%S.so", &x->dir_libs, &name);
	if (dep) {
		znames[1] = ffsz_allocfmt("%S/%s.so", &x->dir_libs, dep);
	}

	JNIEnv *env;
	int r, attached;
	if ((r = jni_vm_attach_once(jvm, &env, &attached, JNI_VERSION_1_6))) {
		errlog("jni_vm_attach: %d", r);
		goto end;
	}

	char **it;
	FF_FOREACH(znames, it) {
		if (!*it) break;

		if (!jni_scall_bool(x->Zvonilka_class, x->Zvonilka_lib_load, jni_js_sz(*it)))
			goto end;
		dbglog("loaded library %s", *it);
	}

	e = 0;

end:
	if (attached)
		jni_vm_detach(jvm);

	ffmem_free(znames[1]);
	if (e) {
		ffmem_free(znames[0]);
		return NULL;
	}
	return znames[0];
}

static int core()
{
	struct phi_core_conf conf = {
		.log_level = (x->debug) ? PHI_LOG_EXTRA : PHI_LOG_VERBOSE,
		.log = exe_log,
		.logv = exe_logv,

		.mod_loading = mod_loading,

		.run_detach = 1,
	};
	if (!(x->core = phi_core_create(&conf)))
		return -1;
	x->cnif = x->core->mod("core.conn");
	x->clif = x->core->mod("core.call");
	return 0;
}

JNIEXPORT void JNICALL
Java_com_github_stsaz_zvonilka_Zvonilka_init(JNIEnv *env, jobject thiz, jstring jlibdir, jobject jasset_mgr)
{
	if (x) return;

	x = ffmem_new(struct zvonilka_jni);
	conf();

	const char *libdir = jni_sz_js(jlibdir);
	x->dir_libs.ptr = ffsz_dup(libdir);
	jni_sz_free(libdir, jlibdir);
	x->dir_libs.len = ffsz_len(x->dir_libs.ptr);

	x->Zvonilka_class = jni_global_ref(jni_class(PJC_ZVONILKA));
	x->Zvonilka_lib_load = jni_sfunc(x->Zvonilka_class, "lib_load", "(" JNI_TSTR ")" JNI_TBOOL);

	if (core()) return;
	phi_core_run();
	dbglog("%s: exit", __func__);
}

JNIEXPORT void JNICALL
Java_com_github_stsaz_zvonilka_Zvonilka_destroy(JNIEnv *env, jobject thiz)
{
	if (!x) return;

	dbglog("%s: enter", __func__);
	phi_core_destroy();
	jni_global_unref(x->Zvonilka_Ctl_obj);

	ffstr_free(&x->dir_libs);
	ffmem_free(x);  x = NULL;
}

JNIEXPORT jstring JNICALL
Java_com_github_stsaz_zvonilka_Zvonilka_version(JNIEnv *env, jobject thiz)
{
	return jni_js_sz(x->core->version_str);
}

JNIEXPORT jint JNI_OnLoad(JavaVM *_jvm, void *reserved)
{
	jvm = _jvm;
	return JNI_VERSION_1_6;
}
