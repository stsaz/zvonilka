/** zvonilka: core: Client-Relay language
2024, Simon Zolin */

static inline void zlang_login_write(ffvec *buf, const char *name)
{
	ffvec_addfmt(buf, "/login?name=%s\r\n", name);
}

static inline int zlang_login_read(ffstr d, ffstr *name)
{
	int r = ffstr_matchfmt(&d, "/login?name=%S\r\n", name);
	if (r < 0) {
		errlog("invalid command");
		return -1;
	} else if (r > 0) {
		r--;
	}
	return r;
}

static inline void zlang_ok_write(ffstr *buf)
{
	ffstr_setz(buf, "200\r\n");
}

static inline void zlang_ready_write(ffvec *buf)
{
	ffvec_addfmt(buf, "/ready\r\n");
}

static inline int zlang_ready_read(ffstr d)
{
	int r = ffstr_matchfmt(&d, "/ready\r\n");
	if (r < 0) {
		errlog("invalid command");
		return -1;
	} else if (r > 0) {
		r--;
	}
	return r;
}

static inline void zlang_call_write(ffvec *buf, const char *target)
{
	ffvec_addfmt(buf, "/call?name=%s\r\n", target);
}

static inline int zlang_call_read(ffstr d, ffstr *name)
{
	int r = ffstr_matchfmt(&d, "/call?name=%S\r\n", name);
	if (r < 0) {
		errlog("invalid command");
		return -1;
	} else if (r > 0) {
		r--;
	}
	return r;
}
