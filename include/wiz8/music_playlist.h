#ifndef WIZ8_MUSIC_PLAYLIST_H
#define WIZ8_MUSIC_PLAYLIST_H

extern "C" {

unsigned char InitializeMusicPlaylist(void);
unsigned char Function48FC10(
    const char* playlist, int immediate, int replace_current);

}

void Function48F9E0(void);
void Function591780(void);

#endif
