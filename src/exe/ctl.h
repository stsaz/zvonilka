/** zvonilka: executor
2024, Simon Zolin */

static void call_open(void *opaque, zvon_call *c)
{
	if (x->clif->state(c) & ZVON_CLS_INCOMING)
		userlog("Incoming call from %s", x->clif->get(c, ZVON_CLG_PEER_IP));
}

static void call_close(void *opaque, zvon_call *c)
{
	if (!c)
		return;

	if ((x->clif->state(c) & 0x0f) == ZVON_CLS_ERR)
		warnlog("The call was interrupted");
	else
		infolog("The call is finished");
	x->clif->close(c);
}

static int call_process(void *opaque, zvon_call *c)
{
	switch (x->clif->state(c) & 0x0f) {
	case ZVON_CLS_ESTABLISHED:
		infolog("Speak");
		break;
	}
	return 0;
}

static const struct zvon_ctl exe_ctl = {
	call_open, call_close, call_process,
};
