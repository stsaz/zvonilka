/** zvonilka: core
2024, Simon Zolin */

static void play_ctl_close(void *f, phi_track *t)
{
	zvon_call *c = t->udata;
	c->play_trk_fin = 1;
	call_sig(t->udata, (t->error) ? ZVON_CALL_ERR : ZVON_CALL_FIN);
}

static int play_ctl_process(void *f, phi_track *t)
{
	return call_recv(t->udata, &t->data_out);
}

static const phi_filter play_ctl = {
	NULL, play_ctl_close, play_ctl_process,
	"play-ctl"
};

static int trk_play(zvon_call *c, phi_track **trk)
{
	const struct zvon_conn_conf *cc = &c->conn->conf;
	struct phi_track_conf conf = {
		.oaudio = {
			.format = {
				.format = PHI_PCM_FLOAT32,
				.rate = 48000,
				.channels = 1,
			},
			.buf_time = cc->buffer_length_msec,
		},
	};
	phi_track *t = track->create(&conf);
	t->data_type = "pcm"; // prevent crash in audio_out_open()

	track->filter(t, &play_ctl, 0);

#if defined FF_WIN
	track->filter(t, core->mod("core.win-sleep"), 0);
#elif defined FF_LINUX && !defined FF_ANDROID
	track->filter(t, core->mod("dbus.sleep"), 0);
#endif

	char *amod = NULL;
	if (!track->filter(t, core->mod("format.ogg"), 0)
		// || !track->filter(t, core->mod("afilter.gain"), 0)
		|| !track->filter(t, core->mod("afilter.auto-conv"), 0)
		|| !track->filter(t, core->mod(amod = ffsz_allocfmt("%s.play", cc->audio_module)), 0)
		) {
		track->close(t);
		return -1;
	}

	t->udata = c;
	track->start(t);
	*trk = t;
	ffmem_free(amod);
	return 0;
}
