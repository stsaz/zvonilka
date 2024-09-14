/** zvonilka/Android
2024, Simon Zolin */

#include <netmill.h>
#include <util/ipaddr.h>
#include <ffsys/netconf.h>

static void call_open(void *opaque, zvon_call *c)
{
	int flags = 0;
	if (x->clif->state(c) & ZVON_CLS_INCOMING)
		flags = 1;

	JNIEnv *env;
	int r = jni_vm_attach(jvm, &env);
	if (r) {
		errlog("jni_vm_attach: %d", r);
		return;
	}
	jni_call_void(x->Zvonilka_Ctl_obj, x->Zvonilka_Ctl_open, flags);
	jni_vm_detach(jvm);
}

static void call_close(void *opaque, zvon_call *c)
{
	int flags = 1;

	if (c) {
		flags = 0;
		if ((x->clif->state(c) & 0x0f) == ZVON_CLS_ERR)
			flags = 2;

		x->clif->close(c);
	}

	JNIEnv *env;
	int r = jni_vm_attach(jvm, &env);
	if (r) {
		errlog("jni_vm_attach: %d", r);
		return;
	}
	jstring jmsg = jni_js_sz("");
	jni_call_void(x->Zvonilka_Ctl_obj, x->Zvonilka_Ctl_close, flags, jmsg);
	jni_vm_detach(jvm);
}

static int call_process(void *opaque, zvon_call *c)
{
	switch (x->clif->state(c) & 0x0f) {
	case ZVON_CLS_ESTABLISHED: {
		JNIEnv *env;
		int r = jni_vm_attach(jvm, &env);
		if (r) {
			errlog("jni_vm_attach: %d", r);
			return 1;
		}
		jni_call_void(x->Zvonilka_Ctl_obj, x->Zvonilka_Ctl_process, 1);
		jni_vm_detach(jvm);
		break;
	}
	}
	return 0;
}

static const struct zvon_ctl exe_ctl = {
	call_open, call_close, call_process,
};

static void jzvon_listen(struct core_data *d)
{
	x->conn = x->cnif->listen(&x->conf);
	ffmem_free(d);
}

static void jzvon_call(struct core_data *d)
{
	x->conn = x->cnif->connect(&x->conf);
	ffmem_free(d);
}

static void jzvon_disconnect(struct core_data *d)
{
	if (x->conn) {
		x->cnif->close(x->conn);
		x->conn = NULL;
	}
	ffmem_free(d);
}

static void settings_set(JNIEnv *env, jobject jo)
{
	struct zvon_conn_conf *cc = &x->conf;
	cc->controller = &exe_ctl;

	jclass c = jni_class_obj(jo);
	cc->port = jni_obj_int(jo, jni_field_int(c, "tcp_port"));

	cc->buffer_length_msec = jni_obj_int(jo, jni_field_int(c, "a_buffer"));
	cc->bitrate_kbps = jni_obj_int(jo, jni_field_int(c, "a_quality"));
	cc->gain_db = jni_obj_int(jo, jni_field_int(c, "a_gain"));
}

static void ctl_set(JNIEnv *env, jobject ctl)
{
	x->Zvonilka_Ctl_obj = jni_global_ref(ctl);
	jclass c = jni_class_obj(x->Zvonilka_Ctl_obj);
	x->Zvonilka_Ctl_open = jni_func(c, "open", "(" JNI_TINT ")" JNI_TVOID);
	x->Zvonilka_Ctl_close = jni_func(c, "close", "(" JNI_TINT JNI_TSTR ")" JNI_TVOID);
	x->Zvonilka_Ctl_process = jni_func(c, "process", "(" JNI_TINT ")" JNI_TVOID);
}

JNIEXPORT void JNICALL
Java_com_github_stsaz_zvonilka_Zvonilka_listen(JNIEnv *env, jobject thiz, jobject settings, jobject ctl)
{
	dbglog("%s: enter", __func__);
	settings_set(env, settings);
	ctl_set(env, ctl);

	struct core_data *d = ffmem_new(struct core_data);
	core_task(d, jzvon_listen);
	dbglog("%s: exit", __func__);
}

JNIEXPORT int JNICALL
Java_com_github_stsaz_zvonilka_Zvonilka_call(JNIEnv *env, jobject thiz, jobject settings, jobject ctl, jstring jtarget)
{
	dbglog("%s: enter", __func__);
	int rc = 1;
	const char *target = jni_sz_js(jtarget);
	uint port;
	if (1 != ffip_port_split(FFSTR_Z(target), x->conf.ip, &port))
		goto end;

	settings_set(env, settings);
	ctl_set(env, ctl);

	struct core_data *d = ffmem_new(struct core_data);
	core_task(d, jzvon_call);
	rc = 0;

end:
	jni_sz_free(target, jtarget);
	dbglog("%s: exit", __func__);
	return rc;
}

JNIEXPORT void JNICALL
Java_com_github_stsaz_zvonilka_Zvonilka_disconnect(JNIEnv *env, jobject thiz)
{
	dbglog("%s: enter", __func__);
	struct core_data *d = ffmem_new(struct core_data);
	core_task(d, jzvon_disconnect);
	dbglog("%s: exit", __func__);
}

/** Get IP addresses from 'struct nml_nif_info' and convert them to String[] */
JNIEXPORT jobjectArray JNICALL
Java_com_github_stsaz_zvonilka_Zvonilka_listIPAddresses(JNIEnv *env, jobject thiz)
{
	dbglog("%s: enter", __func__);
	ffvec ips = {};

	struct nml_nif_info ni = {};
	ffslice nifs = {};
	nml_nif_info(&ni, &nifs);

	struct ffnetconf_ifinfo **pnif;
	FFSLICE_WALK(&nifs, pnif) {
		struct ffnetconf_ifinfo *nif = *pnif;
		for (uint i = 0;  i < nif->ip_n;  i++) {
			char buf[100];
			int r = ffip46_tostr((void*)nif->ip[i], buf, sizeof(buf));
			buf[r] = '\0';
			*ffvec_pushT(&ips, char*) = ffsz_dup(buf);
		}
	}

	jobjectArray jsa = jni_jsa_sza(env, ips.ptr, ips.len);

	nml_nif_info_destroy(&ni);

	char **it;
	FFSLICE_WALK(&ips, it) {
		ffmem_free(*it);
	}
	ffvec_free(&ips);

	dbglog("%s: exit", __func__);
	return jsa;
}
