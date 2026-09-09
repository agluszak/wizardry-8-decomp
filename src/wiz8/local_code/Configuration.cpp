#include "wiz8/local_code/GameplayDatabase.h"
#include "surrender/srGERD.h"
#include "wiz8/local_code/Configuration.h"
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
    g_settings_6850c8.field_02f = volume;
    if (g_music_sample_handle_60aae0 != -1) {
        SoundSetVolume(g_music_sample_handle_60aae0, volume);
    }
}

// FUNCTION: WIZ8 0x004291d0
void SetDisplayGamma(float value)
{
    srVector3T<float> gamma;
    gamma.x = value;
    gamma.y = value;
    gamma.z = value;
    g_gerd_659634->setGamma(gamma);
}

/* Configuration.cpp reads CNFG and QLTY through the ordinary chunk reader.
   KEYM and the default-file save path remain unrecovered. */
// FUNCTION: WIZ8 0x0054b810
void LoadGameConfiguration(void)
{
    W8Chunk file;
    bool loaded = false;

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
    }
    SoundSetDefaultVolume(g_settings_6850c8.field_02e);
    SetMusicVolume(g_settings_6850c8.field_02f);
    float gamma = g_settings_6850c8.gamma;
    if (gamma < 0.5f || gamma > 2.0f) {
        gamma = 1.0f;
        g_settings_6850c8.gamma = gamma;
    }
    SetDisplayGamma(gamma);
}
