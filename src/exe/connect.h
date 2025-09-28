/** zvonilka: executor
2024, Simon Zolin */

#include <util/ipaddr.h>

static int connect_help()
{
	help_info_write("\
Connect to a relay server:\n\
    `zvonilka connect` RELAY LOGIN [OPTIONS]\n\
\n\
RELAY   Relay IP address (e.g. 192.168.1.1)\n\
LOGIN   Your name\n\
\n\
Options:\n\
  `-port` NUMBER          TCP port (default: 21073)\n\
\n\
  `-audio` STRING         Audio library name (e.g. alsa)\n\
  `-mic` NUM              Microphone device index\n\
  `-play` NUM             Playback device index\n\
  `-buffer` MSEC          Audio buffer length (default: 200)\n\
  `-gain` dB              Microphone signal gain\n\
  `-noise_gate` dB        Noise gate threshold (default: 30)\n\
\n\
  `-bandwidth` KHZ        Opus bandwidth (4, 6, 8, 12 or 20)\n\
  `-quality` KBPS         Opus encoding bitrate (default: 32)\n\
\n\
  `-call` TARGET          Start calling\n\
");
	x->exit_code = 0;
	return 1;
}

static void connect_action()
{
	struct zvon_conn_conf conf = conn_init();
	ffmem_copy(conf.ip, x->relay_ip, 16);
	ffmem_copy(conf.name, x->login, sizeof(conf.name));
	userlog("Connecting...");
	x->conn = x->cnif->connect(&conf);
}

static int connect_input(void *param, ffstr s)
{
	if (x->arg_i == 0) {
		uint port;
		if (1 != ffip_port_split(s, x->relay_ip, &port))
			return _ffargs_err(&x->cmd, 1, "Invalid IP address");

	} else if (x->arg_i == 1) {
		x->login = ffsz_dupstr(&s);
	}
	x->arg_i++;
	return 0;
}

static int connect_prepare(void *param)
{
	if (x->arg_i != 2)
		return _ffargs_err(&x->cmd, 1, "Expecting 2 arguments");
	x->action = connect_action;
	return 0;
}

#define O(m)  (void*)FF_OFF(struct exe, m)
static const struct ffarg cmd_connect[] = {
	{ "-audio",			'=s',	O(audio_module) },
	{ "-bandwidth",		'u',	O(bandwidth_khz) },
	{ "-buffer",		'u',	O(buffer_length_msec) },
	{ "-call",			'=s',	O(callee) },
	{ "-gain",			'u',	O(mic_gain_db) },
	{ "-help",			0,		connect_help },
	{ "-mic",			'u',	O(mic_dev_index) },
	{ "-noise_gate",	'u',	O(noise_gate_db) },
	{ "-play",			'u',	O(play_dev_index) },
	{ "-port",			'u',	O(port) },
	{ "-quality",		'u',	O(bitrate_kbps) },
	{ "\0\1",			'+S',	connect_input },
	{ "",				0,		connect_prepare },
};
#undef O
