/** zvonilka: core
2024, Simon Zolin */

#include <zvonilka.h>
#include <track.h>

extern const phi_track_if *track;
static zvon_call* call_create(zvon_conn *conn);
static void call_run(zvon_call *c, uint flags);
#include <core/conn.h>

struct zvon_call {
	zvon_conn *conn;
	phi_track *rec_trk, *play_trk;
	ffvec buf_send;
	uint state;
	char peer_ip_str[FFIP6_STRLEN + 1];
	uint play_trk_fin :1;
	uint rec_trk_fin :1;
};

static zvon_call* call_create(zvon_conn *conn)
{
	zvon_call *c = ffmem_new(zvon_call);
	c->conn = conn;
	uint port;
	ffslice ip = ffsockaddr_ip_port(&conn->peer_addr, &port);
	ffip46_tostr(ip.ptr, c->peer_ip_str, sizeof(c->peer_ip_str) - 1);
	return c;
}

static int trk_play(zvon_call *c, phi_track **trk);
static int trk_rec(zvon_call *c, phi_track **trk);

static void call_run(zvon_call *c, uint flags)
{
	c->state |= flags;
	c->conn->conf.controller->open(c->conn->conf.opaque, c);
	trk_play(c, &c->play_trk);
	trk_rec(c, &c->rec_trk);
}

static void call_close(zvon_call *c)
{
	ffvec_free(&c->buf_send);
	ffmem_free(c);
}

static uint call_state(zvon_call *c)
{
	return c->state;
}

static int call_sig(zvon_call *c, uint sig)
{
	switch (sig) {
	case ZVON_CALL_STOP:
		break;

	case ZVON_CALL_ERR:
		c->state = ZVON_CLS_ERR;
		// fallthrough
	case ZVON_CALL_FIN:
		if ((c->state & 0x0f) != ZVON_CLS_ERR)
			c->state = ZVON_CLS_FIN;
		if (c->rec_trk_fin && c->play_trk_fin)
			c->conn->conf.controller->close(c->conn->conf.opaque, c);
		break;
	}
	return 1;
}

static void conn_send_ready(void *param)
{
	zvon_call *c = param;
	track->wake(c->rec_trk);
}

static int call_send(zvon_call *c, ffstr data)
{
	ffvec_addstr(&c->buf_send, &data);

	if ((c->state & 0x0f) == ZVON_CLS_ERR
		|| (c->state & 0x0f) == ZVON_CLS_FIN)
		return PHI_ERR;

	while (c->buf_send.len) {
		int r = conn_send(c->conn, *(ffstr*)&c->buf_send, conn_send_ready, c);
		switch (r) {
		case -PHI_ASYNC:
			return 0;
		case -PHI_ERR:
			return PHI_ERR;
		default:
			ffslice_rm((ffslice*)&c->buf_send, 0, r, 1);
		}
	}

	return 0;
}

static void conn_recv_ready(void *param)
{
	zvon_call *c = param;
	track->wake(c->play_trk);
}

static int call_recv(zvon_call *c, ffstr *out)
{
	if ((c->state & 0x0f) == ZVON_CLS_NONE) {
		c->state |= ZVON_CLS_ESTABLISHED;
		c->conn->conf.controller->process(c->conn->conf.opaque, c);
	}

	if ((c->state & 0x0f) == ZVON_CLS_ERR
		|| (c->state & 0x0f) == ZVON_CLS_FIN)
		return PHI_ERR;

	int r = conn_recv(c->conn, out, conn_recv_ready, c);
	if (r < 0)
		return -r;
	return PHI_DATA;
}

static void* call_get(zvon_call *c, uint flags)
{
	switch (flags) {
	case ZVON_CLG_PEER_IP:
		return c->peer_ip_str;
	}
	return NULL;
}

const struct zvon_call_if zvon_call_iface = {
	call_close,
	call_sig,
	call_state,
	call_get,
};

extern const phi_track_if *track;
#include <core/play.h>
#include <core/rec.h>
