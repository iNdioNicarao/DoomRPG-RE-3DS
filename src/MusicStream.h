/*
    DoomRPG-RE 3DS - MP3 streaming music player

    Streams MP3 tracks from the SD card, decoding on demand with libmad
    inside SDL_mixer's music callback. Uses a fixed-size ring buffer so
    full-length tracks play without loading decoded PCM into memory.
*/
#ifndef MUSIC_STREAM_H
#define MUSIC_STREAM_H

#if defined(__3DS__)

void MusicStream_play(int resourceID, int loop);
void MusicStream_stop(void);
void MusicStream_setVolume(int vol);
int  MusicStream_playing(void);

#endif // __3DS__
#endif // MUSIC_STREAM_H
