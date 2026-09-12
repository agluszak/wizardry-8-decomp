#ifndef WIZ8_MUSIC_PLAYLIST_H
#define WIZ8_MUSIC_PLAYLIST_H

struct W8MonsterInfo;
#include "wiz8/screen_state.h"

extern int g_music_sample_handle_60aae0;
extern unsigned char g_music_playlist_active_65ba7e;
extern int g_music_playlist_weight_total_65ba80;
extern int g_music_playlist_track_count_65ba84;
extern int g_music_state_60aae8;
extern int g_music_state_60aaec;
extern int g_music_state_60aaf0;

unsigned char InitializeMusicPlaylist(void);
unsigned char StartMusicResource0048FC10(const char* resource, int immediate, int replace_current);

void ServiceMusicPlaylist0048F9E0(void);
void StopMusicPlaylist(unsigned char fade);

char IsCurrentMusicPlaylist(const char* playlist);

void Function48F650(W8MonsterInfo* monster_info, unsigned char value_1, unsigned char value_2);

#endif
