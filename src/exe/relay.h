/** zvonilka: executor
2024, Simon Zolin */

static int listen_help()
{
	help_info_write("\
Relay calls between peers:\n\
    `zvonilka relay` [OPTIONS]\n\
\n\
Options:\n\
  `-port` NUMBER          TCP port (default: 21073)\n\
");
	x->exit_code = 0;
	return 1;
}

static void listen_action()
{
	x->rlif = x->core->mod("core.relay");
	struct zvon_relay_conf conf = {
		.port = x->port,

		.controller = &exe_relay_ctl,
		.opaque = x,
	};
	userlog("Listening...");
	x->rel = x->rlif->listen(&conf);
}

static int listen_prepare(void *param)
{
	x->action = listen_action;
	return 0;
}

#define O(m)  (void*)FF_OFF(struct exe, m)
static const struct ffarg cmd_listen[] = {
	{ "-help",			0,		listen_help },
	{ "-port",			'u',	O(port) },
	{ "",				0,		listen_prepare },
};
#undef O
