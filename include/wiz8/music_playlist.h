#ifndef WIZ8_MUSIC_PLAYLIST_H
#define WIZ8_MUSIC_PLAYLIST_H

#ifdef __cplusplus
extern "C" {
#endif

unsigned char InitializeMusicPlaylist(void);
unsigned char Function48FC10(
    const char* playlist, int immediate, int replace_current);

#ifdef __cplusplus
}

void Function48F9E0(void);
#endif

#endif
