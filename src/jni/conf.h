/** zvonilka/Android
2024, Simon Zolin */

static const char setting_names[][20] = {
	"a_buffer",
	"a_gain",
	"a_quality",
	"name",
	"relay_ip",
	"relay_port",
	"target",
};

/** Read config data into Java array.
(KEY VALUE LF)... -> {value-offset value-length value-as-number}...
*/
static jintArray conf_read(JNIEnv *env, ffstr data, const char settings[][20], uint n_settings, int int_default)
{
	const char *data_start = data.ptr;
	ffvec fields = {};
	ffvec_zalloc(&fields, n_settings * 3, 4);
	fields.len = n_settings * 3;
	int *dst = fields.ptr;

	while (data.len) {
		ffstr ln, k, v;
		ffstr_splitby(&data, '\n', &ln, &data);
		ffstr_splitby(&ln, ' ', &k, &v);

		int r = ffcharr_findsorted(settings, n_settings, sizeof(settings[0]), k.ptr, k.len);
		if (r < 0)
			continue;

		dst[r*3 + 0] = v.ptr - data_start;
		dst[r*3 + 1] = v.len;

		int n = int_default;
		ffstr_to_int32(&v, &n);
		dst[r*3 + 2] = n;
	}

	jintArray jia = jni_jia_vec(env, *(ffslice*)&fields);
	ffvec_free(&fields);
	return jia;
}

JNIEXPORT jboolean JNICALL
Java_com_github_stsaz_zvonilka_Conf_confRead(JNIEnv *env, jobject thiz, jstring jfilepath)
{
	int rc = 0;
	dbglog("%s: enter", __func__);
	const char *fn = jni_sz_js(jfilepath);
	ffvec d = {};
	if (fffile_readwhole(fn, &d, 1*1024*1024))
		goto end;
	jintArray jia = conf_read(env, *(ffstr*)&d, setting_names, FF_COUNT(setting_names), 0);

	jclass jc = jni_class_obj(thiz);
	jni_obj_jba_set(env, thiz, jni_field_jba(jc, "data"), *(ffstr*)&d);
	jni_obj_jo_set(thiz, jni_field(jc, "fields", JNI_TARR JNI_TINT), jia);
	rc = 1;

end:
	jni_sz_free(fn, jfilepath);
	ffvec_free(&d);
	dbglog("%s: exit", __func__);
	return rc;
}

JNIEXPORT jboolean JNICALL
Java_com_github_stsaz_zvonilka_Conf_confWrite(JNIEnv *env, jobject thiz, jstring jfilepath, jbyteArray jdata)
{
	dbglog("%s: enter", __func__);
	const char *fn = jni_sz_js(jfilepath);
	char *fn_tmp = ffsz_allocfmt("%s.tmp", fn);
	ffstr data = jni_str_jba(env, jdata);
	int rc = 0;
	if (0 != fffile_writewhole(fn_tmp, data.ptr, data.len, 0)) {
		syserrlog("fffile_writewhole: %s", fn_tmp);
		goto end;
	}

	if (0 != fffile_rename(fn_tmp, fn)) {
		syserrlog("fffile_rename: %s", fn);
		goto end;
	}
	rc = 1;

end:
	jni_bytes_free(data.ptr, jdata);
	jni_sz_free(fn, jfilepath);
	ffmem_free(fn_tmp);
	dbglog("%s: exit", __func__);
	return rc;
}
