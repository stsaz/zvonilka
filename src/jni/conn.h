/** zvonilka/Android
2024, Simon Zolin */

static void jzvon_listen(struct core_data *d)
{
	x->conn = x->cnif->connect(&x->conf);
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

static void conf_set(struct zvon_conn_conf *cc, JNIEnv *env, jobject jo)
{
	cc->controller = &exe_ctl;

	jclass c = jni_class_obj(jo);
	cc->port = jni_obj_int(jo, jni_field_int(c, "relay_port"));

	cc->buffer_length_msec = jni_obj_int(jo, jni_field_int(c, "a_buffer"));
	cc->bitrate_kbps = jni_obj_int(jo, jni_field_int(c, "a_quality"));
	cc->gain_db = jni_obj_int(jo, jni_field_int(c, "a_gain"));
	cc->noise_gate_db = 0;
}

JNIEXPORT void JNICALL
Java_com_github_stsaz_zvonilka_Zvonilka_listen(JNIEnv *env, jobject thiz, jobject settings, jobject ctl, jstring jrelay, jstring jname)
{
	dbglog("%s: enter", __func__);
	const char *name = jni_sz_js(jname)
		, *relay_ip = jni_sz_js(jrelay);

	ffsz_copyz(x->conf.name, sizeof(x->conf.name), name);

	uint port;
	if (1 != ffip_port_split(FFSTR_Z(relay_ip), x->conf.ip, &port))
		goto end;

	conf_set(&x->conf, env, settings);
	ctl_set(env, ctl);

	struct core_data *d = ffmem_new(struct core_data);
	core_task(d, jzvon_listen);

end:
	jni_sz_free(name, jname);
	jni_sz_free(relay_ip, jrelay);
	dbglog("%s: exit", __func__);
}

JNIEXPORT int JNICALL
Java_com_github_stsaz_zvonilka_Zvonilka_call(JNIEnv *env, jobject thiz, jobject settings, jobject ctl, jstring jrelay, jstring jname, jstring jtarget)
{
	dbglog("%s: enter", __func__);
	int rc = 1;
	const char *name = jni_sz_js(jname)
		, *target = jni_sz_js(jtarget)
		, *relay_ip = jni_sz_js(jrelay);

	ffsz_copyz(x->conf.name, sizeof(x->conf.name), name);

	ffmem_free(x->callee);
	x->callee = ffsz_dup(target);

	uint port;
	if (1 != ffip_port_split(FFSTR_Z(relay_ip), x->conf.ip, &port))
		goto end;

	conf_set(&x->conf, env, settings);
	ctl_set(env, ctl);

	struct core_data *d = ffmem_new(struct core_data);
	core_task(d, jzvon_call);
	rc = 0;

end:
	jni_sz_free(name, jname);
	jni_sz_free(target, jtarget);
	jni_sz_free(relay_ip, jrelay);
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
