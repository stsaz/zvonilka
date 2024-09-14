/** zvonilka: executor
2024, Simon Zolin */

static int listen_help()
{
	help_info_write("\
Listen for incoming calls:\n\
    `zvonilka listen` [OPTIONS]\n\
\n\
Options:\n\
  `-port` NUMBER          TCP port (default: 21073)\n\
\n\
  `-audio` STRING         Audio library name (e.g. alsa)\n\
  `-buffer` MSEC          Audio buffer length (default: 200)\n\
  `-bandwidth` KHZ        Opus bandwidth (4, 6, 8, 12 or 20)\n\
  `-quality` KBPS         Opus encoding bitrate (default: 32)\n\
");
	x->exit_code = 0;
	return 1;
}

static void listen_action()
{
	struct zvon_conn_conf conf = conn_init();
	x->conn = x->cnif->listen(&conf);
	userlog("Listening...");
}

static int listen_prepare(void *param)
{
	x->action = listen_action;
	return 0;
}

#define O(m)  (void*)FF_OFF(struct exe, m)
static const struct ffarg cmd_listen[] = {
	{ "-audio",			'=s',	O(audio_module) },
	{ "-bandwidth",		'u',	O(bandwidth_khz) },
	{ "-buffer",		'u',	O(buffer_length_msec) },
	{ "-help",			0,		listen_help },
	{ "-port",			'u',	O(port) },
	{ "-quality",		'u',	O(bitrate_kbps) },
	{ "",				0,		listen_prepare },
};
#undef O
