/** zvonilka: core
2024, Simon Zolin */

static void rec_grd_close(void *f, phi_track *t)
{
	zvon_call *c = t->udata;
	c->rec_trk_fin = 1;
	call_sig(t->udata, (t->error) ? ZVON_CALL_ERR : ZVON_CALL_FIN);
}

static int rec_grd_process(void *f, phi_track *t)
{
	return PHI_DONE;
}

static const phi_filter rec_guard = {
	NULL, rec_grd_close, rec_grd_process,
	"rec-guard"
};


static int rec_ctl_process(void *f, phi_track *t)
{
	zvon_call *c = t->udata;
	int r;
	if ((r = call_send(c, t->data_in)))
		return r;
	t->data_out = t->data_in;
	return (t->chain_flags & PHI_FFIRST) ? PHI_DONE : PHI_OK;
}

static const phi_filter rec_ctl = {
	NULL, NULL, rec_ctl_process,
	"rec-ctl"
};


static int trk_rec(zvon_call *c, phi_track **trk)
{
	const struct zvon_conn_conf *cc = &c->conn->conf;
	struct phi_track_conf conf = {
		.iaudio = {
			.format = {
				.format = PHI_PCM_FLOAT32,
				.rate = 48000,
				.channels = 1,
			},
			.buf_time = cc->buffer_length_msec,
		},
		.afilter.gain_db = cc->gain_db,
		.opus = {
			.bitrate = cc->bitrate_kbps,
			.mode = 1,
			.bandwidth = cc->bandwidth_khz,
		},
		.ogg.max_page_length_msec = cc->buffer_length_msec,
		.ofile.name = "tmp.opus", // ogg writer will add opus encoder
	};
	phi_track *t = track->create(&conf);

	char *amod = NULL;
	if (!track->filter(t, &rec_guard, 0)
		|| !track->filter(t, core->mod(amod = ffsz_allocfmt("%s.rec", cc->audio_module)), 0)
		|| !track->filter(t, core->mod("afilter.gain"), 0)
		|| !track->filter(t, core->mod("afilter.auto-conv"), 0)
		|| !track->filter(t, core->mod("format.ogg-write"), 0)
		|| !track->filter(t, &rec_ctl, 0)
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
