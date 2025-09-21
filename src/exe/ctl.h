/** zvonilka: executor
2024, Simon Zolin */

static void ctl_relay_connection(void *opaque, uint flags)
{
	if (flags & ZVON_REL_NEW_CLIENT)
		userlog("New client");
	if (flags & ZVON_REL_DISCONNECT)
		userlog("Client Disconnected");
}

static const struct zvon_relay_ctl exe_relay_ctl = {
	ctl_relay_connection,
};


static void ctl_connection(void *opaque, uint flags)
{
	if (flags & ZVON_CONN_CONNECTED) {
		userlog("Connected to server");
		if (x->callee)
			x->cnif->call(x->conn, x->callee);
		else
			x->cnif->call(x->conn, NULL);
	} else if (flags & ZVON_CONN_DISCONNECTED) {
		userlog("Disconnected from server");
	}
}

static void ctl_open(void *opaque, zvon_call *c)
{
	if (x->clif->state(c) & ZVON_CLS_INCOMING)
		userlog("Incoming call from %s", x->clif->get(c, ZVON_CLG_PEER_IP));
}

static void ctl_close(void *opaque, zvon_call *c)
{
	if (!c)
		return;

	if ((x->clif->state(c) & 0x0f) == ZVON_CLS_ERR)
		warnlog("The call was interrupted");
	else
		infolog("The call is finished");
	x->clif->close(c);
}

static int ctl_process(void *opaque, zvon_call *c)
{
	switch (x->clif->state(c) & 0x0f) {
	case ZVON_CLS_ESTABLISHED:
		infolog("Speak");
		break;
	}
	return 0;
}

static const struct zvon_ctl exe_ctl = {
	ctl_connection,
	ctl_open, ctl_close, ctl_process,
};
