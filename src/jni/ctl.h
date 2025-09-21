/** zvonilka/Android
2024, Simon Zolin */

#include <netmill.h>
#include <util/ipaddr.h>

static void ctl_connection(void *opaque, uint flags)
{
	if (flags & ZVON_CONN_CONNECTED) {

		JNIEnv *env;
		int r = jni_vm_attach(jvm, &env);
		if (r) {
			errlog("jni_vm_attach: %d", r);
			return;
		}
		jni_call_void(x->Zvonilka_Ctl_obj, x->Zvonilka_Ctl_process, 2);
		jni_vm_detach(jvm);

		if (x->callee)
			x->cnif->call(x->conn, x->callee);
		else
			x->cnif->call(x->conn, NULL);
		ffmem_free(x->callee);
		x->callee = NULL;
	}
}

static void ctl_open(void *opaque, zvon_call *c)
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

static void ctl_close(void *opaque, zvon_call *c)
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

static int ctl_process(void *opaque, zvon_call *c)
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
	ctl_connection, ctl_open, ctl_close, ctl_process,
};

static void ctl_set(JNIEnv *env, jobject ctl)
{
	x->Zvonilka_Ctl_obj = jni_global_ref(ctl);
	jclass c = jni_class_obj(x->Zvonilka_Ctl_obj);
	x->Zvonilka_Ctl_open = jni_func(c, "open", "(" JNI_TINT ")" JNI_TVOID);
	x->Zvonilka_Ctl_close = jni_func(c, "close", "(" JNI_TINT JNI_TSTR ")" JNI_TVOID);
	x->Zvonilka_Ctl_process = jni_func(c, "process", "(" JNI_TINT ")" JNI_TVOID);
}
