/** zvonilka: core
2024, Simon Zolin */

#include <zvonilka.h>
#include <netmill.h>

extern phi_core *core;
#define syserrlog(...) \
	core->conf.log(core->conf.log_obj, PHI_LOG_ERR | PHI_LOG_SYS, "core", NULL, __VA_ARGS__)
#define errlog(...) \
	core->conf.log(core->conf.log_obj, PHI_LOG_ERR, "core", NULL, __VA_ARGS__)
#define dbglog(...) \
do { \
	if (ff_unlikely(core->conf.log_level >= PHI_LOG_DEBUG)) \
		core->conf.log(core->conf.log_obj, PHI_LOG_DEBUG, "core", NULL, __VA_ARGS__); \
} while (0)
#define extralog(...) \
do { \
	if (ff_unlikely(core->conf.log_level >= PHI_LOG_EXTRA)) \
		core->conf.log(core->conf.log_obj, PHI_LOG_EXTRA, "core", NULL, __VA_ARGS__); \
} while (0)

#include <core/lang.h>
#include <core/client.h>

struct zvon_relay {
	const nml_tcp_listener_if *tlif;
	nml_tcp_listener *tl;
	ffsock ls;
	struct zzkevent kev;

	ffvec clients; // zvon_relay_client[]
	struct zvon_relay_conf conf;
};

static void rel_sig(zvon_relay *r, int sig)
{
	r->conf.controller->connection(r->conf.opaque, sig);
}

static zvon_relay_client* rel_find_cl(zvon_relay *r, ffstr name)
{
	zvon_relay_client **pc;
	FFSLICE_WALK(&r->clients, pc) {
		if (ffstr_ieqz(&name, (*pc)->name))
			return *pc;
	}
	return NULL;
}

static void rel_close_cl(zvon_relay *r, zvon_relay_client *c)
{
	r->conf.controller->connection(r->conf.opaque, ZVON_REL_DISCONNECT);

	ssize_t i = -1;
	zvon_relay_client **pc;
	FFSLICE_WALK(&r->clients, pc) {
		if (*pc == c) {
			i = pc - (zvon_relay_client**)r->clients.ptr;
		} else if ((*pc)->peer == c) {
			(*pc)->peer = (void*)-1;
		}
	}

	ffslice_rmswapT((ffslice*)&r->clients, i, 1, void*);
}

static void rel_close(zvon_relay *r)
{
	ffsock_close(r->ls);
	r->tlif->free(r->tl);
	ffmem_free(r);
}

static void rel_on_accept(void *opaque, ffsock sk, ffsockaddr *addr)
{
	zvon_relay *r = opaque;
	zvon_relay_client *c = rel_cl_new();
	c->r = r;
	c->cs = sk;
	c->peer_addr = *addr;
	*ffvec_pushT(&r->clients, zvon_relay_client*) = c;

	ffsock_setopt(c->cs, IPPROTO_TCP, TCP_NODELAY, 1);
	if (core->kq_attach(0, (phi_kevent*)&c->kev, (fffd)c->cs, 0)) {
		return;
	}

	rel_cl_recv(c);
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

extern const struct nml_tcp_listener_if nml_tcp_listener_interface;

static zvon_relay* rel_listen(struct zvon_relay_conf *conf)
{
	if (ffsock_init(FFSOCK_INIT_SIGPIPE | FFSOCK_INIT_WSA | FFSOCK_INIT_WSAFUNCS)) {
		syserrlog("ffsock_init");
		return NULL;
	}

	zvon_relay *r = ffmem_new(zvon_relay);
	r->ls = FFSOCK_NULL;
	r->tlif = &nml_tcp_listener_interface;
	if (!conf->port)
		conf->port = 21073;
	r->conf = *conf;

	struct nml_tcp_listener_conf tlconf;
	r->tlif->conf(NULL, &tlconf);
	r->tl = r->tlif->create();

	tlconf.log_level = core->conf.log_level;
	tlconf.log = nml_log;
	tlconf.log_obj = core->conf.log_obj;

	tlconf.core = nmlcore;
	tlconf.on_accept = rel_on_accept;
	tlconf.opaque = r;
	tlconf.addr.port = conf->port;
	tlconf.reuse_port = 1;

	if (r->tlif->conf(r->tl, &tlconf)) {
		goto err;
	}
	r->tlif->run(r->tl);
	return r;

err:
	rel_close(r);
	return NULL;
}

const struct zvon_relay_if zvon_relay_iface = {
	rel_listen, rel_close
};
