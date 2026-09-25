#ifndef WIZ8_MUSIC_PLAYLIST_H
#define WIZ8_MUSIC_PLAYLIST_H

#include "wiz8/local_screens/Screens.h"

extern int g_music_sample_handle;
extern bool g_music_playlist_active;
extern int g_music_playlist_weight_total;
extern int g_music_playlist_track_count;
extern int g_music_state_60aae8;
extern int g_music_state_60aaec;
extern int g_music_state_60aaf0;

unsigned char InitializeMusicPlaylist(void);
unsigned char StartMusicResource(const char* resource, int immediate, int replace_current);

void ServiceMusicPlaylist(void);
void StopMusicPlaylist(unsigned char fade);

bool IsCurrentMusicPlaylist(const char* playlist);

#endif
