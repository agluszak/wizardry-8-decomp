#include "wiz8/combat_state.h"
#include "wiz8/engine_code/stScript.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/music_playlist.h"
#include "wiz8/wiz8_windows.h"
#include "random.h"
#include "soundman.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

stScript* g_music_playlist_65ba74;
unsigned int g_music_playlist_tick_65ba78;
unsigned char g_music_playlist_active_65ba7e;
int g_music_playlist_weight_total_65ba80;
int g_music_playlist_track_count_65ba84;
extern "C" int g_music_sample_handle_60aae0;
unsigned char g_music_fade_60aae4;
unsigned char g_music_force_next_60aae5;
int g_music_state_60aae8;
int g_music_state_60aaec;
int g_music_state_60aaf0;
extern unsigned char g_flag_689b32;
extern void Function591780(void);

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

/* Count playable rows, load the pause directives, and retain a nonzero weight
   total only when every playable row carries a parenthesized weight. */
// FUNCTION: WIZ8 0x0048FF50
int AnalyzeMusicPlaylist0048FF50(stScript* playlist, int* total_weight)
{
    int playable_count = 0;
    unsigned char found_unweighted = 0;

    *total_weight = 0;
    for (int index = 0; index < playlist->lines.GetCount(); ++index) {
        const char* line = (*playlist->lines.GetAt(index))->text;

        if (line[0] == '#') {
            if (_strnicmp(line + 1, "PAUSEMIN=", 9) == 0) {
                g_music_state_60aae8 = atoi(line + 10);
            }
            else if (_strnicmp(line + 1, "PAUSEMAX=", 9) == 0) {
                g_music_state_60aaec = atoi(line + 10);
            }
            else if (_strnicmp(line + 1, "PAUSECHANCE=", 12) == 0) {
                g_music_state_60aaf0 = atoi(line + 13);
            }
            continue;
        }

        ++playable_count;
        if (found_unweighted == 0) {
            const char* weight = strchr(line, '(');
            if (weight != 0) {
                *total_weight += atoi(weight + 1);
            }
            else {
                found_unweighted = 1;
                *total_weight = 0;
            }
        }
    }
    return playable_count;
}

// FUNCTION: WIZ8 0x0048F9E0
void Function48F9E0(void)
{
    int failures = 0;

    if (g_music_playlist_active_65ba7e == 0) {
        return;
    }
    if (g_music_sample_handle_60aae0 != -1 &&
        SoundIsPlaying(g_music_sample_handle_60aae0) != 0) {
        return;
    }
    if (GetTickCount() <= g_music_playlist_tick_65ba78) {
        return;
    }

    if (g_music_force_next_60aae5 == 0 && g_in_combat_00683f94 == 0 &&
        Random(100) <= static_cast<unsigned int>(g_music_state_60aaf0)) {
        g_music_playlist_tick_65ba78 =
            GetTickCount() + g_music_state_60aae8 * 1000 +
            Random(g_music_state_60aaec * 1000 -
                   g_music_state_60aae8 * 1000);
        return;
    }

    for (;;) {
        int selected = -1;

        if (g_music_playlist_weight_total_65ba80 == 0) {
            selected = Random(g_music_playlist_track_count_65ba84);
        }
        else {
            unsigned int target = Random(g_music_playlist_weight_total_65ba80);
            unsigned int accumulated = 0;
            for (int index = 0;
                 index < g_music_playlist_65ba74->lines.GetCount(); ++index) {
                const char* line =
                    (*g_music_playlist_65ba74->lines.GetAt(index))->text;
                if (line[0] == '#') {
                    continue;
                }
                selected = index;
                accumulated += atoi(strchr(line, '(') + 1);
                if (target < accumulated) {
                    break;
                }
            }
        }

        if (selected < 0) {
            g_music_force_next_60aae5 = 0;
            return;
        }

        char track[260];
        strcpy(track,
               (*g_music_playlist_65ba74->lines.GetAt(selected))->text);
        char* weight = strchr(track, '(');
        if (weight != 0) {
            *weight = 0;
        }

        if (Function48FC10(track, g_music_fade_60aae4, 1) == 0) {
            ++failures;
        }
        if (failures > 4 || g_music_sample_handle_60aae0 != -1) {
            g_music_force_next_60aae5 = 0;
            return;
        }
    }
}

// FUNCTION: WIZ8 0x0048FC10
extern "C" unsigned char Function48FC10(
    const char* playlist, int fade, int replace_current)
{
    char path[260];

    if (playlist == 0) {
        return 0;
    }
    if (g_flag_689b32 != 0) {
        Function591780();
    }

    sprintf(path, "Data\\Music\\%s", playlist);
    _strupr(path);
    g_music_fade_60aae4 = static_cast<unsigned char>(fade);

    if (strstr(path, ".MPL") == 0) {
        if (g_music_playlist_active_65ba7e == 0) {
            g_music_playlist_65ba74->setName("");
        }

        int handle = static_cast<int>(SoundPlayStreamedFile(path, 0));
        if (handle == -1) {
            return 0;
        }
        SoundSetMusic(handle);

        if (g_music_fade_60aae4 != 0) {
            if (g_music_sample_handle_60aae0 != -1) {
                SoundSetFadeVolume(
                    g_music_sample_handle_60aae0, 0, 2000, 1);
            }
            SoundSetVolume(handle, 0);
            SoundSetFadeVolume(
                handle, g_settings_6850c8.field_02f, 5000, 0);
        }
        else {
            if (g_music_sample_handle_60aae0 != -1) {
                SoundStop(g_music_sample_handle_60aae0);
            }
            SoundSetVolume(handle, g_settings_6850c8.field_02f);
        }
        g_music_sample_handle_60aae0 = handle;
        return 1;
    }

    if (_stricmp(playlist, g_music_playlist_65ba74->getName()) == 0) {
        return 1;
    }

    g_music_playlist_tick_65ba78 = GetTickCount() - 1;
    g_music_state_60aae8 = 0;
    g_music_state_60aaec = 0;
    g_music_state_60aaf0 = 0;
    g_music_playlist_65ba74->Clear004CF690();
    g_music_playlist_65ba74->Load004CF3B0(path);

    if (g_music_playlist_65ba74->lines.GetCount() == 0) {
        return 0;
    }
    if (replace_current != 0) {
        if (g_music_sample_handle_60aae0 != -1) {
            if (g_music_fade_60aae4 == 0) {
                SoundStop(g_music_sample_handle_60aae0);
            }
            else {
                SoundSetFadeVolume(
                    g_music_sample_handle_60aae0, 0, 2000, 1);
            }
        }
        g_music_sample_handle_60aae0 = -1;
    }

    g_music_playlist_track_count_65ba84 = AnalyzeMusicPlaylist0048FF50(
        g_music_playlist_65ba74,
        &g_music_playlist_weight_total_65ba80);
    if (g_music_playlist_track_count_65ba84 != 0) {
        g_music_playlist_65ba74->setName(playlist);
        g_music_playlist_active_65ba7e = 1;
        g_music_force_next_60aae5 = 1;
    }
    return 1;
}
