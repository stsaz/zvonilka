/** zvonilka: core
2024, Simon Zolin */

#include <ffsys/socket.h>
#include <netmill.h>

extern phi_core *core;
#define syserrlog(...) \
	core->conf.log(core->conf.log_obj, PHI_LOG_ERR | PHI_LOG_SYS, "core", NULL, __VA_ARGS__)
#define extralog(...) \
do { \
	if (ff_unlikely(core->conf.log_level >= PHI_LOG_EXTRA)) \
		core->conf.log(core->conf.log_obj, PHI_LOG_EXTRA, "core", NULL, __VA_ARGS__); \
} while (0)

struct zvon_conn {
	ffvec buf;

	ffsock cs;
	ffsockaddr peer_addr;
	struct zzkevent kev;

	struct zvon_conn_conf conf;
	char *target_name;

	char *error_msg;
};

static void conn_connect2(void *param);
static void conn_login_req(void *param);
static void conn_login_resp(void *param);
static void conn_call_req(void *param);
static void conn_call_resp(void *param);
static int conn_send(zvon_conn *c, ffstr data, phi_task_func func, void *param);
static int conn_recv(zvon_conn *c, ffstr *out, phi_task_func func, void *param);
static void conn_ready_resp(void *param);

static void conn_close(zvon_conn *c)
{
	ffvec_free(&c->buf);
	ffsock_close(c->cs);
	ffmem_free(c->target_name);
	ffmem_free(c->error_msg);
	ffmem_free(c);
}

static void conn_err(zvon_conn *c, uint code)
{
	ffmem_free(c->error_msg);
	c->error_msg = ffsz_dup(fferr_strptr(fferr_last()));

	ffsock_close(c->cs);
	c->cs = FFSOCK_NULL;
	c->buf.len = 0;
	c->conf.controller->connection(c->conf.opaque, c, ZVON_CONN_DISCONNECTED);
	// c->conf.controller->close(c->conf.opaque, NULL);
}

#if defined FF_WIN
	#define AUMOD_DEF  "wasapi"
#elif defined FF_ANDROID
	#define AUMOD_DEF  "aaudio"
#else
	#define AUMOD_DEF  "pulse"
#endif

static zvon_conn* conn_new(struct zvon_conn_conf *conf)
{
	if (ffsock_init(FFSOCK_INIT_SIGPIPE | FFSOCK_INIT_WSA | FFSOCK_INIT_WSAFUNCS)) {
		syserrlog("ffsock_init");
		return NULL;
	}

	zvon_conn *c = ffmem_new(zvon_conn);
	c->cs = FFSOCK_NULL;
	if (!conf->port)
		conf->port = 21073;

	if (!conf->audio_module)
		conf->audio_module = AUMOD_DEF;
	if (!conf->buffer_length_msec)
		conf->buffer_length_msec = 200;
	if (!conf->bitrate_kbps)
		conf->bitrate_kbps = 32;

	ffvec_alloc(&c->buf, 64*1024, 1);
	c->conf = *conf;
	return c;
}

static int conn_sig(uint sig)
{
	switch (sig) {
	case ZVON_CONN_STOP:
		// TODO
		break;
	}
	return 1;
}

static int conn_setup(zvon_conn *c)
{
	if (ffsock_setopt(c->cs, IPPROTO_TCP, TCP_NODELAY, 1))
		syserrlog("ffsock_setopt");
	if (core->kq_attach(0, (phi_kevent*)&c->kev, (fffd)c->cs, 0)) {
		return 1;
	}
	return 0;
}

static zvon_conn* conn_connect(struct zvon_conn_conf *conf)
{
	zvon_conn *c = conn_new(conf);

	const ffip4 *ip4 = ffip6_tov4((ffip6*)conf->ip);
	if (ip4)
		ffsockaddr_set_ipv4(&c->peer_addr, ip4, conf->port);
	else
		ffsockaddr_set_ipv6(&c->peer_addr, conf->ip, conf->port);

	if (FFSOCK_NULL == (c->cs = ffsock_create_tcp(c->peer_addr.ip4.sin_family, FFSOCK_NONBLOCK))) {
		syserrlog("ffsock_create_tcp");
		goto err;
	}

	if (conn_setup(c)) {
		goto err;
	}

	conn_connect2(c);
	return c;

err:
	conn_close(c);
	return NULL;
}

static void conn_connect2(void *param)
{
	zvon_conn *c = param;

	if (ffsock_connect_async(c->cs, &c->peer_addr, &c->kev.wtask)) {
		if (fferr_last() == FFSOCK_EINPROGRESS) {
			c->kev.obj = c;
			c->kev.whandler = (void*)conn_connect2;
			return;
		}
		syserrlog("ffsock_connect_async");
		conn_err(c, 2);
		return;
	}

	conn_login_req(c);
}

static int conn_send(zvon_conn *c, ffstr data, phi_task_func func, void *param)
{
	int r = ffsock_send_async(c->cs, data.ptr, data.len, &c->kev.wtask);
	if (r < 0) {
		if (fferr_last() == FFSOCK_EINPROGRESS) {
			extralog("sending to server...");
			c->kev.obj = param;
			c->kev.whandler = func;
			return -PHI_ASYNC;
		}
		syserrlog("ffsock_send_async");
		conn_err(c, 3);
		return -PHI_ERR;
	}
	extralog("send: %u", r);
	return r;
}

static int conn_recv(zvon_conn *c, ffstr *out, phi_task_func func, void *param)
{
	int r = ffsock_recv_async(c->cs, c->buf.ptr, c->buf.cap, &c->kev.rtask); // TODO
	if (r < 0) {
		if (fferr_last() == FFSOCK_EINPROGRESS) {
			extralog("receiving from server...");
			c->kev.obj = param;
			c->kev.rhandler = func;
			return -PHI_ASYNC;
		}
		syserrlog("ffsock_recv_async");
		conn_err(c, 3);
		return -PHI_ERR;
	}
	extralog("recv: %u", r);
	if (r == 0)
		return -PHI_DONE;
	ffstr_set(out, c->buf.ptr, r);
	return r;
}

static void conn_login_req(void *param)
{
	zvon_conn *c = param;

	if (!c->buf.len)
		zlang_login_write(&c->buf, c->conf.name);

	int r = conn_send(c, *(ffstr*)&c->buf, conn_login_req, c);
	if (r <= 0)
		return;
	conn_login_resp(c);
}

static void conn_login_resp(void *param)
{
	zvon_conn *c = param;
	ffstr out;
	int r = conn_recv(c, &out, conn_login_resp, c);
	if (r <= 0) {
		assert(r != -PHI_DONE);
		return;
	}
	c->conf.controller->connection(c->conf.opaque, c, ZVON_CONN_CONNECTED);
}

static void conn_ready_req(void *param)
{
	zvon_conn *c = param;

	if (!c->buf.len)
		zlang_ready_write(&c->buf);

	int r = conn_send(c, *(ffstr*)&c->buf, conn_ready_req, c);
	if (r <= 0)
		return;
	conn_ready_resp(c);
}

static void conn_ready_resp(void *param)
{
	zvon_conn *c = param;
	ffstr out;
	int r = conn_recv(c, &out, conn_ready_resp, c);
	if (r <= 0) {
		assert(r != -PHI_DONE);
		return;
	}
	call_run(call_create(c), 0);
}

static void conn_call(zvon_conn *c, const char *target)
{
	c->buf.len = 0;

	if (!target) {
		conn_ready_req(c);
		return;
	}

	c->target_name = ffsz_dup(target);
	conn_call_req(c);
}

static void conn_call_req(void *param)
{
	zvon_conn *c = param;

	if (!c->buf.len)
		zlang_call_write(&c->buf, c->target_name);

	int r = conn_send(c, *(ffstr*)&c->buf, conn_call_req, c);
	if (r <= 0)
		return;
	conn_call_resp(c);
}

static void conn_call_resp(void *param)
{
	zvon_conn *c = param;
	ffstr out;
	int r = conn_recv(c, &out, conn_call_resp, c);
	if (r <= 0) {
		assert(r != -PHI_DONE);
		return;
	}
	call_run(call_create(c), 0);
}

static void* conn_get(zvon_conn *c, uint flags)
{
	switch (flags) {
	case ZVON_CNG_ERROR:
		return (c->error_msg) ? c->error_msg : "";
	}
	return NULL;
}

const struct zvon_conn_if zvon_conn_iface = {
	conn_sig,
	conn_connect,
	conn_close,
	conn_call,
	conn_get,
};
