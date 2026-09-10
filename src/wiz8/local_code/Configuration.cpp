#include "wiz8/local_code/GameplayDatabase.h"
#include "surrender/srGERD.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/local_screens/MGSKeyboard.h"
#include "wiz8/chunk.h"
#include "wiz8/music_playlist.h"
#include "wiz8/render_state.h"
#include "wiz8/wiz8_windows.h"
#include "soundman.h"

#include <stdio.h>
#include <string.h>


// GLOBAL: WIZ8 0x006850c8
W8GameSettings g_settings_6850c8;
int g_music_sample_handle_60aae0 = -1;

// FUNCTION: WIZ8 0x00428e60
unsigned int GetTotalPhysicalMemory(void)
{
    MEMORYSTATUS status;
    memset(&status, 0, sizeof(status));
    status.dwLength = sizeof(status);
    GlobalMemoryStatus(&status);
    return status.dwTotalPhys;
}

// FUNCTION: WIZ8 0x00429800
int GetRendererFamily(void)
{
    char name[128];
    if (!g_gerd_659634) {
        return -1;
    }
    strncpy(name, g_gerd_659634->getName(), sizeof(name) - 1);
    name[sizeof(name) - 1] = '\0';
    _strupr(name);
    if (strstr(name, "OPENGL")) return 0;
    if (strstr(name, "GLIDE")) return 2;
    if (strstr(name, "DIRECT3D")) return 1;
    return strstr(name, "SOFTWARE") ? 3 : 4;
}

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
    }
    else if (g_settings_6850c8.muted_music_volume != 0xff) {
        g_settings_6850c8.music_volume = g_settings_6850c8.muted_music_volume;
        if (g_music_sample_handle_60aae0 != -1) {
            SoundSetVolume(g_music_sample_handle_60aae0, g_settings_6850c8.muted_music_volume);
        }
        g_settings_6850c8.muted_music_volume = 0xff;
    }
}

// FUNCTION: WIZ8 0x004291d0
void SetDisplayGamma(float value)
{
    srVector3T<float> gamma;
    gamma.Set(value, value, value);
    g_gerd_659634->setGamma(gamma);
}

// FUNCTION: WIZ8 0x0054b810
void LoadGameConfiguration(void)
{
    W8Chunk file;
    bool loaded = false;

    ResetMGSKeyboardBindings();
    if (file.OpenRead("Wiz8.CFG")) {
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
    if (!loaded) {
        Function54B560();
        SaveGameConfiguration();
    }
    SoundSetDefaultVolume(g_settings_6850c8.sound_effects_volume);
    SetMusicVolume(g_settings_6850c8.music_volume);
    float gamma = g_settings_6850c8.gamma;
    if (gamma < 0.5f || gamma > 2.0f) {
        gamma = 1.0f;
        g_settings_6850c8.gamma = gamma;
    }
    SetDisplayGamma(gamma);
}

// FUNCTION: WIZ8 0x0054b6d0
unsigned char SaveGameConfiguration(void)
{
    W8Chunk file;

    if (!file.OpenWrite("Wiz8.CFG")) {
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
