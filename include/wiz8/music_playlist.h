#ifndef WIZ8_MUSIC_PLAYLIST_H
#define WIZ8_MUSIC_PLAYLIST_H
#include "wiz8/screen_state.h"

extern int g_music_sample_handle_60aae0;

extern "C" {

unsigned char InitializeMusicPlaylist(void);
unsigned char Function48FC10(
    const char* playlist, int immediate, int replace_current);

}

void Function48F9E0(void);
void StopMusicPlaylist(unsigned char fade);

#endif
