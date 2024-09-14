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

	const nml_tcp_listener_if *tlif;
	nml_tcp_listener *tl;
	ffsock ls;
	struct zzkevent kev;

	struct zvon_conn_conf conf;
};

static void conn_close(zvon_conn *c)
{
	ffvec_free(&c->buf);
	ffsock_close(c->cs);
	ffsock_close(c->ls);
	c->tlif->free(c->tl);
	ffmem_free(c);
}

static void conn_err(zvon_conn *c, uint code)
{
	ffsock_close(c->cs);
	c->cs = FFSOCK_NULL;
	c->buf.len = 0;
	c->conf.controller->close(c->conf.opaque, NULL);
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

	call_run(call_create(c), 0);
}

extern const struct nml_tcp_listener_if nml_tcp_listener_interface;

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
	c->ls = FFSOCK_NULL;
	c->tlif = &nml_tcp_listener_interface;
	if (!conf->audio_module)
		conf->audio_module = AUMOD_DEF;
	if (!conf->buffer_length_msec)
		conf->buffer_length_msec = 200;
	if (!conf->bitrate_kbps)
		conf->bitrate_kbps = 32;
	if (!conf->port)
		conf->port = 21073;
	ffvec_alloc(&c->buf, 64*1024, 1);
	c->conf = *conf;
	return c;
}

static int conn_setup(zvon_conn *c)
{
	ffsock_setopt(c->cs, IPPROTO_TCP, TCP_NODELAY, 1);
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

static void conn_on_accept(void *opaque, ffsock sk, ffsockaddr *addr)
{
	zvon_conn *c = opaque;
	c->cs = sk;
	c->peer_addr = *addr;

	conn_setup(c);

	call_run(call_create(c), ZVON_CLS_INCOMING);
}

static void nml_log(void *log_obj, uint level, const char *ctx, const char *id, const char *format, ...)
{
	static const uint levels[] = {
		/*NML_LOG_SYSFATAL*/PHI_LOG_ERR | PHI_LOG_SYS,
		/*NML_LOG_SYSERR*/	PHI_LOG_ERR | PHI_LOG_SYS,
		/*NML_LOG_ERR*/		PHI_LOG_ERR,
		/*NML_LOG_SYSWARN*/	PHI_LOG_WARN | PHI_LOG_SYS,
		/*NML_LOG_WARN*/	PHI_LOG_WARN,
		/*NML_LOG_INFO*/	PHI_LOG_INFO,
		/*NML_LOG_VERBOSE*/	PHI_LOG_VERBOSE,
		/*NML_LOG_DEBUG*/	PHI_LOG_DEBUG,
		/*NML_LOG_EXTRA*/	PHI_LOG_EXTRA,
	};
	level = levels[level];

	va_list va;
	va_start(va, format);
	core->conf.logv(core->conf.log_obj, level, NULL, NULL, format, va);
	va_end(va);
}

static struct zzkevent* nmlcore_kev_new(void *boss)
{
	return (struct zzkevent*)core->kev_alloc(0);
}

static void nmlcore_kev_free(void *boss, struct zzkevent *kev)
{
	core->kev_free(0, (phi_kevent*)kev);
}

static int nmlcore_kq_attach(void *boss, ffsock sk, struct zzkevent *kev, void *obj)
{
	kev->obj = obj;
	return core->kq_attach(0, (phi_kevent*)kev, (fffd)sk, 0);
}

static void nmlcore_timer(void *boss, nml_timer *tmr, int interval_msec, fftimerqueue_func func, void *param)
{
	core->timer(0, (phi_timer*)tmr, interval_msec, func, param);
}

static void nmlcore_task(void *boss, nml_task *t, uint flags)
{
	if (flags == 0)
		core->task(0, (phi_task*)t, NULL, NULL);
	else
		core->task(0, (phi_task*)t, t->handler, t->param);
}

static fftime nmlcore_date(void *boss, ffstr *dts)
{
	fftime t;
	fftime_now(&t);
	return t;
}

static const struct nml_core nmlcore = {
	.kev_new = nmlcore_kev_new,
	.kev_free = nmlcore_kev_free,
	.kq_attach = nmlcore_kq_attach,
	.timer = nmlcore_timer,
	.task = nmlcore_task,
	.date = nmlcore_date,
};

static zvon_conn* conn_listen(struct zvon_conn_conf *conf)
{
	zvon_conn *c = conn_new(conf);

	struct nml_tcp_listener_conf tlconf;
	c->tlif->conf(NULL, &tlconf);
	c->tl = c->tlif->create();

	tlconf.log_level = core->conf.log_level;
	tlconf.log = nml_log;
	tlconf.log_obj = core->conf.log_obj;

	tlconf.core = nmlcore;
	tlconf.on_accept = conn_on_accept;
	tlconf.opaque = c;
	tlconf.addr.port = conf->port;
	tlconf.reuse_port = 1;

	if (c->tlif->conf(c->tl, &tlconf)) {
		goto err;
	}
	c->tlif->run(c->tl);
	return c;

err:
	conn_close(c);
	return NULL;
}

static int conn_send(zvon_conn *c, ffstr data, phi_task_func func, void *param)
{
	int r = ffsock_send_async(c->cs, data.ptr, data.len, &c->kev.wtask);
	if (r < 0) {
		if (fferr_last() == FFSOCK_EINPROGRESS) {
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
	int r = ffsock_recv_async(c->cs, c->buf.ptr, c->buf.cap, &c->kev.rtask);
	if (r < 0) {
		if (fferr_last() == FFSOCK_EINPROGRESS) {
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

static int conn_sig(uint sig)
{
	switch (sig) {
	case ZVON_CONN_STOP:
		break;
	}
	return 1;
}

const struct zvon_conn_if zvon_conn_iface = {
	conn_sig,
	conn_connect,
	conn_listen,
	conn_close,
};
