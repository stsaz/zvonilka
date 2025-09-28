/** phiola: audio frame reader
2025, Simon Zolin */

#include <avpack/reader.h>
#include <avpack/ogg-codec-read.h>

struct fmt_rd {
	phi_track *trk;
	avpk_reader rd;
	ffstr input;
	uint sample_rate, iframe;
};

static void fmtr_log(void *opaque, const char *fmt, va_list va)
{
	struct fmt_rd *f = opaque;
	phi_dbglogv(core, NULL, f->trk, fmt, va);
}

static void* fmtr_open(phi_track *t)
{
	struct fmt_rd *f = phi_track_allocT(t, struct fmt_rd);
	f->trk = t;
	struct avpk_reader_conf c = {
		.code_page = core->conf.code_page,
		.log = fmtr_log,
		.opaque = f,
	};
	if (avpk_open(&f->rd, &avpk_ogg, &c)) {
		errlog(t, "avpk_open");
		return PHI_OPEN_ERR;
	}
	return f;
}

static void fmtr_close(struct fmt_rd *f, phi_track *t)
{
	avpk_close(&f->rd);
	phi_track_free(t, f);
}

static const char* fmtr_hdr(struct fmt_rd *f, phi_track *t, struct avpk_info *hdr)
{
	if (hdr->channels > 8) {
		errlog(t, "Invalid channels number");
		return NULL;
	}

	t->audio.format.rate = hdr->sample_rate;
	t->audio.format.channels = hdr->channels;
	t->audio.total = hdr->duration;
	t->audio.bitrate = (hdr->audio_bitrate) ? hdr->audio_bitrate : hdr->real_bitrate;
	t->audio.start_delay = hdr->delay;
	t->audio.end_padding = hdr->padding;

	t->audio.format.format = hdr->sample_bits;
	if (hdr->sample_float)
		t->audio.format.format |= 0x0100;

	if (hdr->codec != AVPKC_OPUS) {
		errlog(t, "Decoding is not supported: %xu", hdr->codec);
		return NULL;
	}
	return "ac-opus.decode";
}

static int fmtr_process(struct fmt_rd *f, phi_track *t)
{
	union avpk_read_result res = {};

	if (t->chain_flags & PHI_FSTOP) {
		return PHI_LASTOUT;
	}
	if (t->chain_flags & PHI_FFWD) {
		f->input = t->data_in;
	}

	ffstr *in = &f->input;
	for (;;) {

		ffmem_zero_obj(&res);
		switch (avpk_read(&f->rd, in, &res)) {
		case AVPK_HEADER: {
			if (f->sample_rate) {
				if (!(t->audio.format.rate == res.hdr.sample_rate
					&& t->audio.format.channels == res.hdr.channels)) {
					errlog(t, "changing audio format on-the-fly is not supported");
					return PHI_ERR;
				}

				dbglog(t, "new logical stream");
				t->meta_changed = 1;
				t->audio.ogg_reset = 1;
				break;
			}

			const char *decoder = fmtr_hdr(f, t, &res.hdr);
			if (!decoder
				|| !core->track->filter(t, core->mod(decoder), 0))
				return PHI_ERR;

			f->sample_rate = res.hdr.sample_rate;
			break;
		}

		case AVPK_META:
			break;

		case AVPK_DATA:
			goto data;

		case AVPK_SEEK:
			t->input.seek = res.seek_offset;
			return PHI_MORE;

		case AVPK_MORE:
			if (in && (t->chain_flags & PHI_FFWD) && t->data_in.len == 0) {
				in = NULL;
				continue;
			}
			if (t->chain_flags & PHI_FFIRST)
				return PHI_LASTOUT;
			return PHI_MORE;

		case AVPK_FIN:
			return PHI_LASTOUT;

		case AVPK_WARNING:
			warnlog(t, "avpk_read() @0x%xU: %s"
				, res.error.offset, res.error.message);
			break;

		case AVPK_ERROR:
			errlog(t, "avpk_read() @0x%xU: %s"
				, res.error.offset, res.error.message);
			return PHI_ERR;
		}
	}

data:
	t->oaudio.ogg_granule_pos = ((oggread*)f->rd.ctx)->page_endpos;
	dbglog(t, "frame #%u  %d @%D  size:%L"
		, ++f->iframe, res.frame.duration, res.frame.pos, res.frame.len);
	t->audio.pos = res.frame.pos;
	t->data_out = *(ffstr*)&res.frame;
	return PHI_DATA;
}

const phi_filter ogg_read = {
	fmtr_open, (void*)fmtr_close, (void*)fmtr_process,
	"ogg-read"
};
