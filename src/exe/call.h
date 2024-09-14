/** zvonilka: executor
2024, Simon Zolin */

#include <util/ipaddr.h>

static int call_help()
{
	help_info_write("\
Start a call:\n\
    `zvonilka call` TARGET [OPTIONS]\n\
\n\
TARGET  Target machine IP address (e.g. 192.168.1.1)\n\
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

static void call_action()
{
	struct zvon_conn_conf conf = conn_init();
	ffmem_copy(conf.ip, x->call_ip, 16);
	x->conn = x->cnif->connect(&conf);
	userlog("Calling...");
}

static int call_input(void *param, ffstr s)
{
	uint port;
	if (1 != ffip_port_split(s, x->call_ip, &port))
		return _ffargs_err(&x->cmd, 1, "Invalid IP address");
	return 0;
}

static int call_prepare(void *param)
{
	x->action = call_action;
	return 0;
}

#define O(m)  (void*)FF_OFF(struct exe, m)
static const struct ffarg cmd_call[] = {
	{ "-audio",			'=s',	O(audio_module) },
	{ "-bandwidth",		'u',	O(bandwidth_khz) },
	{ "-buffer",		'u',	O(buffer_length_msec) },
	{ "-help",			0,		call_help },
	{ "-port",			'u',	O(port) },
	{ "-quality",		'u',	O(bitrate_kbps) },
	{ "\0\1",			'S',	call_input },
	{ "",				0,		call_prepare },
};
#undef O
