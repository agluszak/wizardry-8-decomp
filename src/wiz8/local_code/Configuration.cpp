#include "wiz8/local_code/GameplayDatabase.h"
#include "wiz8/sr_api.h"
#include "surrender/srGERD.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/local_screens/MGSKeyboard.h"
#include "wiz8/chunk.h"
#include "wiz8/engine_code/AmbientSound.h"
#include "wiz8/music_playlist.h"
#include "wiz8/render_state.h"
#include "wiz8/utility.h"
#include "wiz8/wiz8_windows.h"
#include "soundman.h"

#include <stdio.h>
#include <string.h>

// GLOBAL: WIZ8 0x006850c8
W8GameSettings g_settings_6850c8;
// GLOBAL: WIZ8 0x0061e184
char g_config_file_name[8] = "Wiz8";
// GLOBAL: WIZ8 0x0061e18c
char g_config_file_extension[] = "CFG";
int g_music_sample_handle_60aae0 = -1;

// FUNCTION: WIZ8 0x0048fe50
void SetMusicVolume(unsigned char volume)
{
    g_settings_6850c8.music_volume = volume;
    if (g_music_sample_handle_60aae0 != -1) {
        SoundSetVolume(g_music_sample_handle_60aae0, volume);
    }
}

// FUNCTION: WIZ8 0x0048fe80
bool IsMusicMuted(void)
{
    return g_settings_6850c8.muted_music_volume != 0xff;
}

// FUNCTION: WIZ8 0x0048fe90
void SetMusicMuted(unsigned char muted)
{
    if (muted != 0) {
        if (g_settings_6850c8.muted_music_volume == 0xff) {
            g_settings_6850c8.muted_music_volume = g_settings_6850c8.music_volume;
            g_settings_6850c8.music_volume = 0;
            if (g_music_sample_handle_60aae0 != -1) {
                SoundSetVolume(g_music_sample_handle_60aae0, 0);
            }
        }
    } else if (g_settings_6850c8.muted_music_volume != 0xff) {
        g_settings_6850c8.music_volume = g_settings_6850c8.muted_music_volume;
        if (g_music_sample_handle_60aae0 != -1) {
            SoundSetVolume(g_music_sample_handle_60aae0, g_settings_6850c8.muted_music_volume);
        }
        g_settings_6850c8.muted_music_volume = 0xff;
    }
}

// FUNCTION: WIZ8 0x0054b810
void LoadGameConfiguration(void)
{
    W8Chunk file;
    char path[60];
    bool loaded = false;
    bool reset = true;

    sprintf(path, "%s.%s", g_config_file_name, g_config_file_extension);
    ResetMGSKeyboardBindings();
    if (file.OpenRead(path)) {
        int count = file.ChunkCount();
        for (int index = 0; index < count; ++index) {
            file.OpenChunk(0, 0);
            unsigned int id = file.CurrentChunkId();
            if (id == 0x47464e43) {
                if (file.CurrentChunkExtent() == sizeof(g_settings_6850c8)) {
                    file.Read(&g_settings_6850c8, sizeof(g_settings_6850c8), 0);
                    loaded = true;
                }
            } else if (id == 0x4d59454b) {
                g_mgs_keyboard->Load(file.m_hFile, 0);
            } else if (id == 0x59544c51) {
                LoadRenderOptions0047B890(file.m_hFile);
            }
            file.SkipCurrentChunk();
            file.ReleaseCurrentChunk();
        }
        file.Close();
    }
    if (loaded) {
        if (static_cast<unsigned int>(g_settings_6850c8.combat_delay_ms) <= 5000u &&
            g_settings_6850c8.text_display_delay_ms <= 5000u) {
            reset = false;
        } else {
            srAssertFail(
                "FALSE", "C:\\Projects\\Wizardry 8\\Local Code\\Configuration.cpp", 0xd9,
                FormatString("LoadConfig: ERROR - Config file appears to be corrupted.  Delete %s",
                             path));
        }
    }
    if (reset) {
        ResetGameplaySettings();
        SaveGameConfiguration();
    }
    SetAmbientSoundVolume0047AD00(g_settings_6850c8.sound_effects_volume);
    SetMusicVolume(g_settings_6850c8.music_volume);
    if (g_settings_6850c8.gamma < 0.1f || g_settings_6850c8.gamma > 2.0f) {
        g_settings_6850c8.gamma = 1.0f;
    }
    SetDisplayGamma(g_settings_6850c8.gamma);
}

// FUNCTION: WIZ8 0x0054b6d0
unsigned char SaveGameConfiguration(void)
{
    W8Chunk file;
    char path[60];

    sprintf(path, "%s.%s", g_config_file_name, g_config_file_extension);
    if (!file.OpenWrite(path)) {
        return 0;
    }
    file.OpenChunk(0x47464e43, 0);
    file.Write(&g_settings_6850c8, sizeof(g_settings_6850c8), 0);
    file.ReleaseCurrentChunk();
    file.OpenChunk(0x59544c51, 0);
    SaveRenderOptions0047B920(file.m_hFile);
    file.ReleaseCurrentChunk();
    file.OpenChunk(0x4d59454b, 0);
    g_mgs_keyboard->Save(file.m_hFile);
    file.ReleaseCurrentChunk();
    file.Close();
    return 1;
}
