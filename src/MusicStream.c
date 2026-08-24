/*
    DoomRPG-RE 3DS - MP3 streaming music player (see MusicStream.h)

    Decodes MP3 from SD with libmad inside SDL_mixer's music callback
    (Mix_HookMusic), writing into a fixed-size ring buffer that the mixer
    drains. Streaming keeps decoded PCM out of memory: only the ring buffer
    and libmad's frame state are resident.
*/
#ifdef __3DS__

#include <stdio.h>
#include <string.h>
#include <mad.h>
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include "MusicStream.h"

#define RING_SIZE   (64 * 1024)   /* power of two */
#define READ_CHUNK  2048

static SDL_RWops* ms_file = NULL;
static struct mad_stream ms_mad;
static struct mad_frame  ms_frame;
static struct mad_synth  ms_synth;
static int  ms_synthPos;          /* sample index within current synth'd frame */
static int  ms_loop = 0;
static int  ms_active = 0;
static volatile int ms_hooked = 0;

/* ring buffer of interleaved S16LE stereo samples */
static unsigned char ms_ring[RING_SIZE];
static volatile int  ms_ringRead;   /* consumer pos */
static int           ms_ringWrite;  /* producer pos */
static int           ms_eof;        /* file exhausted (no more frames) */

static void ms_resetDecoder(void)
{
    mad_stream_init(&ms_mad);
    mad_frame_init(&ms_frame);
    mad_synth_init(&ms_synth);
    ms_synthPos = 0;
    ms_eof = 0;
    if (ms_file) SDL_RWseek(ms_file, 0, RW_SEEK_SET);
}

static int ms_decodeMore(void)
{
    /* decode frames until the ring has some room or the file is exhausted */
    int made = 0;
    while (!ms_eof) {
        if (ms_mad.buffer == NULL || ms_mad.error == MAD_ERROR_BUFLEN) {
            /* (re)fill the input buffer */
            static unsigned char inbuf[READ_CHUNK * 4];
            int keep = (int)(ms_mad.bufend - ms_mad.next_frame);
            if (keep > 0) memmove(inbuf, ms_mad.next_frame, keep);
            int n = SDL_RWread(ms_file, inbuf + keep, 1, sizeof(inbuf) - keep);
            if (n <= 0) {
                if (ms_loop) {
                    ms_resetDecoder();
                    continue;
                }
                ms_eof = 1;
                break;
            }
            mad_stream_buffer(&ms_mad, inbuf, keep + n);
        }

        if (mad_frame_decode(&ms_frame, &ms_mad) == -1) {
            if (MAD_RECOVERABLE(ms_mad.error)) continue;
            if (ms_mad.error == MAD_ERROR_BUFLEN) continue;
            ms_eof = 1;
            break;
        }

        mad_synth_frame(&ms_synth, &ms_frame);
        const struct mad_pcm* pcm = &ms_synth.pcm;
        int ns = pcm->length;                 /* samples per channel */
        int ch = pcm->channels;

        /* synth to interleaved S16LE */
        for (int i = 0; i < ns; ++i) {
            short frame[2];
            for (int c = 0; c < 2; ++c) {
                mad_fixed_t s = pcm->samples[c < ch ? c : ch - 1][i];
                /* mad_fixed_t: 28-bit fractional -> 16-bit */
                s += (1L << (MAD_F_FRACBITS - 16));
                if (s > MAD_F_MAX) s = MAD_F_MAX;
                else if (s < -MAD_F_MAX) s = -MAD_F_MAX;
                frame[c] = (short)(s >> (MAD_F_FRACBITS + 2 - 16));
            }
            int used = 0;
            while (used < (int)sizeof(frame)) {
                int wpos = ms_ringWrite & (RING_SIZE - 1);
                ms_ring[wpos] = ((unsigned char*)frame)[used];
                ++ms_ringWrite;
                ++used;
                ++made;
                /* if the ring fills completely, drop oldest sample */
                if (ms_ringWrite - ms_ringRead >= RING_SIZE) {
                    ms_ringRead = ms_ringWrite - (RING_SIZE / 2);
                }
            }
        }
        if (made > RING_SIZE / 2) break; /* enough for now */
    }
    return made;
}

static void ms_callback(void* udata, unsigned char* stream, int len)
{
    (void)udata;
    if (!ms_active || !ms_file || !ms_hooked) {
        memset(stream, 0, len);
        return;
    }

    int avail = ms_ringWrite - ms_ringRead;
    if (avail < len / 2) ms_decodeMore();
    avail = ms_ringWrite - ms_ringRead;

    int i = 0;
    while (i < len && ms_ringRead < ms_ringWrite) {
        stream[i++] = ms_ring[ms_ringRead & (RING_SIZE - 1)];
        ++ms_ringRead;
    }
    if (i < len) memset(stream + i, 0, len - i); /* underrun silence */
}

static void ms_teardown(void)
{
    if (ms_file) {
        mad_synth_finish(&ms_synth);
        mad_frame_finish(&ms_frame);
        mad_stream_finish(&ms_mad);
        SDL_RWclose(ms_file);
        ms_file = NULL;
    }
}

void MusicStream_play(int resourceID, int loop)
{
    char path[128];
    snprintf(path, sizeof(path), "sdmc:/3ds/doomrpg/%d.mp3", resourceID);

    MusicStream_stop();
    ms_teardown();  /* clean up any previous stream (safe: hook already detached) */

    SDL_RWops* f = SDL_RWFromFile(path, "rb");
    if (!f) {
        /* try the loose folder variant used by the PC datafiles */
        snprintf(path, sizeof(path), "sdmc:/3ds/doomrpg/%d.mp3", resourceID);
        f = SDL_RWFromFile(path, "rb");
    }
    if (!f) return;

    ms_file = f;
    ms_loop = loop;
    ms_ringRead = ms_ringWrite = 0;
    ms_resetDecoder();
    ms_active = 1;
    Mix_HookMusic(ms_callback, NULL);
    ms_hooked = 1;
    ms_decodeMore(); /* prime the ring */
}

void MusicStream_stop(void)
{
    /* Detaching the hook is synchronized with the mixer thread by SDL_mixer,
       so after this returns the callback is no longer running. The decoder
       state is torn down lazily on the next start to keep this path simple. */
    if (ms_hooked) {
        Mix_HookMusic(NULL, NULL);
        ms_hooked = 0;
    }
    ms_active = 0;
}

int MusicStream_playing(void) { return ms_active; }

#endif /* __3DS__ */
