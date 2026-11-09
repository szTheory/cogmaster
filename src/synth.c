/* Audio output: the exact rational sample cadence, the frame's PCM and the
   host's drain. Synthesis arrives with the sound chips; until then every
   sample is silence, so the cadence is proven on its own. */
#include <string.h>

#include "internal.h"

/* The 48 kHz rate for the loaded TV standard: per tick, inc over mod. */
static void synth_rate(const cogmaster *m, uint32_t *inc, uint32_t *mod)
{
    if (m->tv == COGMASTER_TV_PAL) {
        *inc = COGMASTER__AUDIO_INC_PAL;
        *mod = COGMASTER__AUDIO_MOD_PAL;
    } else {
        *inc = COGMASTER__AUDIO_INC_NTSC;
        *mod = COGMASTER__AUDIO_MOD_NTSC;
    }
}

/* The Game Gear is stereo in either video mode; the Master System is mono. */
static uint32_t synth_channels(const cogmaster *m)
{
    return m->machine == COGMASTER_MACHINE_GG ? 2u : 1u;
}

void cogmaster__synth_reset(cogmaster *m, int power)
{
    if (power) {
        memset(&m->synth, 0, sizeof m->synth);
    }
}

void cogmaster__synth_frame_end(cogmaster *m)
{
    struct cogmaster__synth *s = &m->synth;
    uint32_t inc, mod;
    synth_rate(m, &inc, &mod);

    /* Frames the host did not take before this one ends are lost; count
       them, never keep them (saturating, so the counter cannot wrap). */
    uint32_t left = s->pcm_frames - s->pcm_read;
    uint32_t room = UINT32_MAX - m->counters.audio_dropped;
    m->counters.audio_dropped += left < room ? left : room;

    uint64_t total = (uint64_t)s->phase + (uint64_t)cogmaster__frame_ticks(m) * inc;
    uint32_t n = (uint32_t)(total / mod);
    s->phase = (uint32_t)(total % mod);
    memset(s->pcm, 0, (size_t)n * synth_channels(m) * sizeof s->pcm[0]);
    s->pcm_frames = n;
    s->pcm_read = 0u;
}

cogmaster_status cogmaster_get_audio(cogmaster *m, cogmaster_audio *io)
{
    if (m == NULL || io == NULL) {
        return COGMASTER_STATUS_ERR_ARGUMENT;
    }
    cogmaster_status st = cogmaster__check_out_size(io, COGMASTER__FIRST_SIZE_AUDIO);
    if (st != COGMASTER_STATUS_OK) {
        return st;
    }
    if (!m->loaded) {
        return COGMASTER_STATUS_ERR_NO_CONTENT;
    }
    if (io->samples == NULL && io->capacity > 0u) {
        return COGMASTER_STATUS_ERR_ARGUMENT;
    }
    struct cogmaster__synth *s = &m->synth;
    uint32_t channels = synth_channels(m);
    uint32_t n = s->pcm_frames - s->pcm_read;
    if (n > io->capacity) {
        n = io->capacity;
    }
    if (n > 0u) {
        memcpy(io->samples, &s->pcm[(size_t)s->pcm_read * channels],
               (size_t)n * channels * sizeof s->pcm[0]);
        s->pcm_read += n;
    }
    io->written = n;
    io->channels = channels;
    io->dropped = m->counters.audio_dropped;
    return COGMASTER_STATUS_OK;
}

void cogmaster__synth_trace(cogmaster *m, uint64_t tick, uint32_t source, uint32_t level)
{
    (void)m;
    (void)tick;
    (void)source;
    (void)level;
}

cogmaster_status cogmaster_get_audio_trace(cogmaster *m, cogmaster_audio_trace *io)
{
    (void)m;
    (void)io;
    return COGMASTER_STATUS_ERR_NO_CONTENT;
}
