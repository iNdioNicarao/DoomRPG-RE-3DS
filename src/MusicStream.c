/*
    DoomRPG-RE 3DS - MP3 streaming music player (see MusicStream.h)

    Loads compressed MP3 into memory (1.5 - 4.1 MB) on track start,
    then decodes frames incrementally with libmad directly from RAM.
    This eliminates all file I/O from the SDL audio callback thread,
    prevents FS session deadlocks, and supports seamless looping.
*/
#ifdef __3DS__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <mad.h>
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include "MusicStream.h"

#define RING_SIZE   (64 * 1024)   /* power of two */

static unsigned char* ms_mp3_buf = NULL;
static long           ms_mp3_size = 0;
static struct mad_stream ms_mad;
static struct mad_frame  ms_frame;
static struct mad_synth  ms_synth;
static int  ms_loop = 0;
static int  ms_active = 0;
static volatile int ms_hooked = 0;
static int  ms_volume = 128;      /* 0..128 (matches MIX_MAX_VOLUME) */

/* ring buffer of interleaved S16LE stereo samples */
static unsigned char ms_ring[RING_SIZE];
static volatile int  ms_ringRead;   /* consumer pos */
static int           ms_ringWrite;  /* producer pos */
static int           ms_eof;        /* track finished */

static void ms_log(const char* fmt, ...)
{
    FILE* log = fopen("sdmc:/3ds/doomrpg/music_debug.log", "a");
    if (log) {
        va_list args;
        va_start(args, fmt);
        vfprintf(log, fmt, args);
        va_end(args);
        fclose(log);
    }
}

static void ms_resetDecoder(void)
{
    mad_stream_init(&ms_mad);
    mad_frame_init(&ms_frame);
    mad_synth_init(&ms_synth);
    ms_eof = 0;
    if (ms_mp3_buf && ms_mp3_size > 0) {
        mad_stream_buffer(&ms_mad, ms_mp3_buf, ms_mp3_size);
    }
}

static int ms_decodeMore(void)
{
    int made = 0;
    while (!ms_eof && ms_mp3_buf) {
        if (mad_frame_decode(&ms_frame, &ms_mad) == -1) {
            if (MAD_RECOVERABLE(ms_mad.error)) {
                continue;
            }
            if (ms_mad.error == MAD_ERROR_BUFLEN) {
                if (ms_loop && ms_mp3_buf) {
                    ms_resetDecoder();
                    continue;
                }
                ms_eof = 1;
                break;
            }
            /* unrecoverable stream error */
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
                /* mad_fixed_t: 28-bit fractional -> 16-bit signed integer */
                s += (1L << (MAD_F_FRACBITS - 16));
                if (s > MAD_F_MAX) s = MAD_F_MAX;
                else if (s < -MAD_F_MAX) s = -MAD_F_MAX;
                frame[c] = (short)(s >> (MAD_F_FRACBITS + 1 - 16));
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
        if (made > RING_SIZE / 2) break; /* filled half ring */
    }
    return made;
}

static void ms_callback(void* udata, unsigned char* stream, int len)
{
    (void)udata;
    if (!ms_active || !ms_mp3_buf || !ms_hooked) {
        memset(stream, 0, len);
        return;
    }

    int avail = ms_ringWrite - ms_ringRead;
    if (avail < len * 2) {
        ms_decodeMore();
    }
    avail = ms_ringWrite - ms_ringRead;

    int i = 0;
    while (i < len && ms_ringRead < ms_ringWrite) {
        stream[i++] = ms_ring[ms_ringRead & (RING_SIZE - 1)];
        ++ms_ringRead;
    }
    if (i < len) memset(stream + i, 0, len - i); /* underrun silence */

    /* Apply volume */
    if (ms_volume < 128) {
        short* samples = (short*)stream;
        int numSamples = len / 2;
        for (int s = 0; s < numSamples; s++) {
            samples[s] = (short)((samples[s] * ms_volume) / 128);
        }
    }
}

static void ms_teardown(void)
{
    mad_synth_finish(&ms_synth);
    mad_frame_finish(&ms_frame);
    mad_stream_finish(&ms_mad);
    if (ms_mp3_buf) {
        free(ms_mp3_buf);
        ms_mp3_buf = NULL;
        ms_mp3_size = 0;
    }
}

void MusicStream_setVolume(int vol)
{
    if (vol < 0) vol = 0;
    if (vol > 128) vol = 128;
    ms_volume = vol;
}

void MusicStream_play(int resourceID, int loop)
{
    char path[128];
    ms_log("MusicStream_play(res=%d, loop=%d)\n", resourceID, loop);

    MusicStream_stop();
    ms_teardown();

    snprintf(path, sizeof(path), "sdmc:/3ds/doomrpg/%d.mp3", resourceID);
    SDL_RWops* f = SDL_RWFromFile(path, "rb");
    if (!f) {
        snprintf(path, sizeof(path), "sdmc:/3ds/DoomRPG/%d.mp3", resourceID);
        f = SDL_RWFromFile(path, "rb");
    }
    if (!f) {
        snprintf(path, sizeof(path), "sdmc:/DoomRPG/%d.mp3", resourceID);
        f = SDL_RWFromFile(path, "rb");
    }
    if (!f) {
        ms_log("Failed to open MP3 for res %d\n", resourceID);
        return;
    }

    Sint64 size = SDL_RWseek(f, 0, RW_SEEK_END);
    SDL_RWseek(f, 0, RW_SEEK_SET);
    if (size <= 0 || size > 16 * 1024 * 1024) {
        ms_log("Invalid MP3 file size: %lld\n", (long long)size);
        SDL_RWclose(f);
        return;
    }

    ms_mp3_buf = (unsigned char*)malloc((size_t)size);
    if (!ms_mp3_buf) {
        ms_log("Failed to allocate %lld bytes for MP3\n", (long long)size);
        SDL_RWclose(f);
        return;
    }

    size_t read_bytes = SDL_RWread(f, ms_mp3_buf, 1, (size_t)size);
    SDL_RWclose(f);

    if (read_bytes != (size_t)size) {
        ms_log("Read incomplete: expected %lld, got %zu\n", (long long)size, read_bytes);
        free(ms_mp3_buf);
        ms_mp3_buf = NULL;
        return;
    }
    ms_mp3_size = (long)size;

    ms_loop = loop;
    ms_ringRead = ms_ringWrite = 0;
    ms_resetDecoder();
    ms_active = 1;

    /* Prime the ring buffer with decoded PCM BEFORE attaching audio callback */
    int primed = ms_decodeMore();
    ms_log("Primed ring buffer with %d bytes (write=%d)\n", primed, ms_ringWrite);

    Mix_HookMusic(ms_callback, NULL);
    ms_hooked = 1;
    ms_log("Mix_HookMusic attached successfully\n");
}

void MusicStream_stop(void)
{
    if (ms_hooked) {
        Mix_HookMusic(NULL, NULL);
        ms_hooked = 0;
    }
    ms_active = 0;
}

int MusicStream_playing(void) { return ms_active; }

#endif /* __3DS__ */
