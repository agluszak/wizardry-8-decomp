#include "wiz8/engine_code/stScript.h"
#include "wiz8/wiz8_windows.h"

stScript* g_music_playlist_65ba74;
unsigned int g_music_playlist_tick_65ba78;
int g_music_state_60aae8;
int g_music_state_60aaec;
int g_music_state_60aaf0;
extern "C" unsigned char g_flag_650e50;

/* Builds the named playlist object before the screen loop begins. */
// FUNCTION: WIZ8 0x0048f940
extern "C" unsigned char InitializeMusicPlaylist(void)
{
    g_music_playlist_65ba74 = new stScript();
    if (g_music_playlist_65ba74) {
        g_music_playlist_65ba74->setName("Music Playlist");
    }
    g_music_playlist_tick_65ba78 = GetTickCount();
    g_music_state_60aae8 = 0;
    g_music_state_60aaec = 0;
    g_music_state_60aaf0 = 0;
    return g_music_playlist_65ba74 != 0;
}

/* The visible menu does not require a music stream, and the retail routine is
   a no-op when the Miles subsystem did not open.  Preserve that startup path
   without inventing the playlist parser; the enabled-audio path remains a
   separate recovery. */
extern "C" unsigned char Function48FC10(const char*, int, int)
{
    if (!g_flag_650e50) {
        return 1;
    }
    return 0;
}
