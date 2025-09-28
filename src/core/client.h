/** zvonilka: core
2024, Simon Zolin */

#include <ffbase/ring.h>

typedef struct zvon_relay_client zvon_relay_client;
struct zvon_relay_client {
	struct zvon_relay *r;

	ffsock			cs;
	ffsockaddr		peer_addr;
	struct zzkevent	kev;
	char			name[128];
	ffring*			buf;
	ffring_head		ring_head, ring_head2;
	ffstr			wdata;
	zvon_relay_client* peer;
	uint 			r_full;
};

static void rel_sig(zvon_relay *r, int sig);
static void rel_close_cl(zvon_relay *r, zvon_relay_client *c);
static zvon_relay_client* rel_find_cl(zvon_relay *r, ffstr name);
static void rel_cl_send(void *param);
static void cl_respond(void *param);
static void rel_cl_recv(void *param);

static zvon_relay_client* rel_cl_new()
{
	zvon_relay_client *c = ffmem_new(zvon_relay_client);
	c->buf = ffring_alloc(64*1024, FFRING_1_READER | FFRING_1_WRITER);
	return c;
}

static void rel_cl_close(zvon_relay_client *c)
{
	ffsock_close(c->cs);
	ffring_free(c->buf);
	ffmem_free(c);
}

static void rel_cl_err(zvon_relay_client *c)
{
	rel_close_cl(c->r, c);
	rel_cl_close(c);
}

static inline void ffring_read_finish_n(ffring *b, ffring_head rh, ffsize n)
{
	rh.nu = rh.old + n;
	ffring_read_finish(b, rh);
}

static inline void ffring_write_finish_n(ffring *b, ffring_head wh, ffsize n)
{
	b->whead = wh.old + n;
	wh.nu = wh.old + n;
	ffring_write_finish(b, wh, NULL);
}

static void cl_login(zvon_relay_client *c, ffstr d)
{
	ffstr name;
	int r = zlang_login_read(d, &name);
	if (r < 0) {
		rel_cl_err(c);
		return;
	}
	ffring_read_finish_n(c->buf, c->ring_head2, r);
	ffsz_copystr(c->name, sizeof(c->name), &name);
	rel_sig(c->r, ZVON_REL_NEW_CLIENT);

	zlang_ok_write(&c->wdata);
	cl_respond(c);
}

static void cl_ready(zvon_relay_client *c, ffstr d)
{
	int r = zlang_ready_read(d);
	if (r < 0) {
		rel_cl_err(c);
		return;
	}
	ffring_read_finish_n(c->buf, c->ring_head2, r);

	// waiting until called // TODO
}

static void cl_call(zvon_relay_client *c, ffstr d)
{
	ffstr name;
	int r = zlang_call_read(d, &name);
	if (r < 0) {
		rel_cl_err(c);
		return;
	}
	ffring_read_finish_n(c->buf, c->ring_head2, r);
	c->peer = rel_find_cl(c->r, name);
	if (!c->peer) {
		errlog("no such peer");
		rel_cl_err(c);
		return;
	}
	c->peer->peer = c;
	rel_sig(c->r, ZVON_REL_NEW_CALL);

	zlang_ok_write(&c->peer->wdata);
	cl_respond(c->peer);

	zlang_ok_write(&c->wdata);
	cl_respond(c);
}

static void cl_respond(void *param)
{
	zvon_relay_client *c = param;
	while (c->wdata.len) {
		int r = ffsock_send_async(c->cs, c->wdata.ptr, c->wdata.len, &c->kev.wtask);
		if (r < 0) {
			if (fferr_last() == FFSOCK_EINPROGRESS) {
				extralog("sending to client...");
				c->kev.obj = c;
				c->kev.whandler = cl_respond;
				return;
			}
			syserrlog("ffsock_send_async");
			rel_cl_err(c);
			return;
		}
		extralog("send: %u", r);
		ffstr_shift(&c->wdata, r);
	}

	rel_cl_recv(c);
}

static void rel_cl_recv(void *param)
{
	zvon_relay_client *c = param;
	ffstr d;
	c->ring_head = ffring_write_begin(c->buf, c->buf->cap, &d, NULL);
	if (!d.len) {
		extralog("r_full");
		c->r_full = 1; // TODO
		return;
	}
	c->r_full = 0;

	int r = ffsock_recv_async(c->cs, d.ptr, d.len, &c->kev.rtask);
	if (r < 0) {
		if (fferr_last() == FFSOCK_EINPROGRESS) {
			ffring_write_finish_n(c->buf, c->ring_head, 0);

			extralog("receiving from client...");
			c->kev.obj = c;
			c->kev.rhandler = rel_cl_recv;
			return;
		}
		syserrlog("ffsock_recv_async");
		rel_cl_err(c);
		return;
	}
	extralog("recv: %u", r);
	if (r == 0) {
		rel_cl_err(c);
		return;
	}
	ffring_write_finish_n(c->buf, c->ring_head, r);

	if (!c->name[0]) {
		c->ring_head2 = ffring_read_begin(c->buf, c->buf->cap, &d, NULL);
		dbglog("command: '%S'", &d);
		cl_login(c, d);
		return;
	}

	if (!c->peer) {
		c->ring_head2 = ffring_read_begin(c->buf, c->buf->cap, &d, NULL);
		dbglog("command: '%S'", &d);
		if (zlang_ready_read(d) >= 0)
			cl_ready(c, d);
		else
			cl_call(c, d);
		return;
	}

	if (c->peer == (void*)-1) {
		dbglog("peer disconnected");
		rel_cl_err(c);
		return;
	}

	rel_cl_send(c->peer);
}

static void rel_cl_send(void *param)
{
	zvon_relay_client *c = param;
	c->ring_head2 = ffring_read_begin(c->peer->buf, c->buf->cap, &c->wdata, NULL);
	int r = ffsock_send_async(c->cs, c->wdata.ptr, c->wdata.len, &c->kev.wtask);
	if (r < 0) {
		if (fferr_last() == FFSOCK_EINPROGRESS) {
			extralog("sending to client...");
			c->kev.obj = c;
			c->kev.whandler = rel_cl_send;
			return;
		}
		syserrlog("ffsock_send_async");
		rel_cl_err(c);
		return;
	}
	extralog("send: %u", r);
	ffstr_shift(&c->wdata, r);
	if (!c->wdata.len) {
		ffring_read_finish(c->peer->buf, c->ring_head2);
		rel_cl_recv(c->peer);
	}
}
