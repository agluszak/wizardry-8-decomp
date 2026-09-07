#include "wiz8/local_code/GameplayDatabase.h"
#include "surrender/srGERD.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/render_state.h"
#include "wiz8/wiz8_windows.h"
#include "soundman.h"

#include <stdio.h>
#include <string.h>

extern "C" {


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

/* The 3D setup record is a 14-byte RIFF-like header followed by chunks with a
   four-byte tag, two reserved bytes, a little-endian length and payload.  The
   CNFG payload is the reviewed 0xa4-byte W8GameSettings block. */
// FUNCTION: WIZ8 0x0054b810
void LoadGameConfiguration(void)
{
    FILE* file = fopen("Wiz8.CFG", "rb");
    unsigned char header[14];
    bool loaded = false;

    if (file && fread(header, sizeof(header), 1, file) == 1
        && memcmp(header, "RIFF", 4) == 0) {
        for (;;) {
            char tag[4];
            unsigned short reserved;
            unsigned int size;
            if (fread(tag, sizeof(tag), 1, file) != 1
                || fread(&reserved, sizeof(reserved), 1, file) != 1
                || fread(&size, sizeof(size), 1, file) != 1) {
                break;
            }
            if (memcmp(tag, "CNFG", 4) == 0
                && size == sizeof(g_settings_6850c8)) {
                loaded = fread(&g_settings_6850c8, size, 1, file) == 1;
                break;
            }
            fseek(file, size, SEEK_CUR);
        }
    }
    if (file) {
        fclose(file);
    }
    if (!loaded) {
        Function54B560();
    }
    SoundSetDefaultVolume(g_settings_6850c8.field_02e);
    SetMusicVolume(g_settings_6850c8.field_02f);
    float gamma;
    memcpy(&gamma, &g_settings_6850c8.field_03c, sizeof(gamma));
    if (gamma < 0.5f || gamma > 2.0f) {
        gamma = 1.0f;
        memcpy(&g_settings_6850c8.field_03c, &gamma, sizeof(gamma));
    }
    SetDisplayGamma(gamma);
}

}
