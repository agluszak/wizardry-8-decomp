#include "wiz8/engine_code/Quality.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/sr_api.h"
#include "wiz8/virtual_file.h"
#include "surrender/srGERD.h"
#include "FileMan.h"

#include <stdlib.h>
#include <string.h>

/* Engine Code\Quality.cpp allocates one 0x34-byte process-wide record.  Its
   leading fields are still unnamed, but the allocation, clear and reviewed
   defaults are complete observations from the startup constructor. */
// FUNCTION: WIZ8 0x0047b500
void InitializeRenderQuality(void)
{
    g_render_options = static_cast<W8RenderQuality*>(malloc(sizeof(*g_render_options)));
    if (!g_render_options) {
        srAssertFail("gpQuality", "C:\\Projects\\Wizardry 8\\Engine Code\\Quality.cpp", 159, 0);
    }
    memset(g_render_options, 0, sizeof(*g_render_options));
    g_render_options->unknown_20 = 0xffffffff;
    g_render_options->unknown_24 = 0xffffffff;
    g_render_options->unknown_14 = 1;
    g_render_options->unknown_2c = 3;
}

// GLOBAL: WIZ8 0x0060A210
float g_render_brightness = 1.0f;
// GLOBAL: WIZ8 0x0060E610
float g_render_fog_distance = 0.5f;
// GLOBAL: WIZ8 0x0060A20C
bool g_render_missile_lights = true;
// GLOBAL: WIZ8 0x00603C6C
bool g_render_mesh_sky = true;

// FUNCTION: WIZ8 0x0047b570
void DestroyRenderQuality(void)
{
    if (g_render_options != 0) {
        free(g_render_options);
    }
    g_render_options = 0;
}

// FUNCTION: WIZ8 0x0047b590
void EnableRenderOption(W8RenderOption option)
{
    if (option < W8_RENDER_OPTION_COUNT) {
        SetRenderOption(option, 1);
    }
}

// FUNCTION: WIZ8 0x0047b630
void SetRenderOption(W8RenderOption option, int enabled)
{
    /* The option record also contains unnamed internal slots. Keep their
       numeric dispatch while the public option ids retain their enum type. */
    switch (static_cast<int>(option)) {
    case 6:
        g_render_brightness = enabled ? 1.0f : 0.8f;
        break;
    case 7:
        if (!enabled && g_render_fog_distance < 0.7f)
            g_render_fog_distance = 0.7f;
        if (enabled && g_render_fog_distance > 0.3f)
            g_render_fog_distance = 0.3f;
        break;
    case 8:
        if (!enabled && g_render_fog_distance < 0.9f)
            g_render_fog_distance = 0.9f;
        if (enabled && g_render_fog_distance > 0.1f)
            g_render_fog_distance = 0.1f;
        break;
    case W8_RENDER_OPTION_MISSILE_LIGHTS:
        g_render_missile_lights = enabled != 0;
        break;
    case W8_RENDER_OPTION_DITHER:
        if ((g_gerd->isEnabled(srGERD::ENABLE_POSITIONAL_0) != 0) != (enabled != 0)) {
            g_gerd->toggle(srGERD::ENABLE_POSITIONAL_0);
        }
        break;
    case 2:
        g_gerd->setTextureDefaultMagFilter(enabled ? srTextureIFace::FILTER_BEST
                                                   : srTextureIFace::FILTER_NONE);
        break;
    case 3:
        g_gerd->setTextureDefaultMinFilter(enabled ? srTextureIFace::FILTER_BEST
                                                   : srTextureIFace::FILTER_NONE);
        break;
    case W8_RENDER_OPTION_MIP_MAPPING:
        g_gerd->setTextureDefaultMipmap(enabled ? srTextureIFace::MIPMAP_BEST
                                                : srTextureIFace::MIPMAP_NONE);
        break;
    case W8_RENDER_OPTION_MESH_SKY:
        g_render_mesh_sky = enabled != 0;
        break;
    case W8_RENDER_OPTION_HIGH_TEXTURE_DETAIL:
        SetResidentTexturePolicy(enabled ? 0 : 1);
        break;
    case W8_RENDER_OPTION_HIGH_TEXTURE_CACHE:
        SetTextureCacheSize(enabled ? 0x2000000 : 0x1000000);
        break;
    case W8_RENDER_OPTION_VIDEO_SYNC:
        SetSwapInterval(enabled != 0);
        break;
    case W8_RENDER_OPTION_CORRECT_BLURRED_TEXT:
        SetSurfaceScale(enabled ? 0.5f : 0.0f);
        break;
    }
    if (option < W8_RENDER_OPTION_COUNT) {
        g_render_options->option_states[option] = enabled != 0;
    }
}

// FUNCTION: WIZ8 0x0047b5b0
void DisableRenderOption(W8RenderOption option)
{
    if (option < W8_RENDER_OPTION_COUNT) {
        SetRenderOption(option, 0);
    }
}

// FUNCTION: WIZ8 0x0047b5d0
void DisableAllRenderOptions(void)
{
    int option = 0;

    do {
        SetRenderOption(static_cast<W8RenderOption>(option), 0);
        ++option;
    } while (option < W8_RENDER_OPTION_COUNT);
}

/* The original carries a dead entry test: it compares the counter against the
   bound before the first iteration and, when that fails, jumps to the increment
   rather than past the loop. Starting at zero it can never fire, and VC6 folds
   it away here whichever way the loop is written - for, while and do-while all
   give the same 22 bytes. The five-byte difference is that fold, not a
   difference in what the loop does. */
// FUNCTION: WIZ8 0x0047b5f0
void EnableAllRenderOptions(void)
{
    for (int option = 0; option < W8_RENDER_OPTION_COUNT; ++option) {
        SetRenderOption(static_cast<W8RenderOption>(option), 1);
    }
}

/* Out-of-range reads report zero rather than indexing past the block. */
// FUNCTION: WIZ8 0x0047b610
unsigned char GetRenderOptionState(W8RenderOption option)
{
    if (option >= W8_RENDER_OPTION_COUNT) {
        return 0;
    }
    return g_render_options->option_states[option];
}

// FUNCTION: WIZ8 0x0047b890
unsigned char LoadRenderOptions(int handle)
{
    int version;
    unsigned int transferred;
    unsigned char options[0x14];
    int option;

    if (FileRead(handle, &version, 4, &transferred) == 0 || version != 1) {
        return 0;
    }
    if (FileRead(handle, options, W8_RENDER_OPTION_COUNT, &transferred) == 0) {
        return 0;
    }
    option = 0;
    do {
        SetRenderOption(static_cast<W8RenderOption>(option), options[option] != 0);
        ++option;
    } while (option < W8_RENDER_OPTION_COUNT);
    return 1;
}

// FUNCTION: WIZ8 0x0047b920
bool SaveRenderOptions(int handle)
{
    unsigned int transferred;
    int version = 1;

    if (FileWrite(handle, &version, 4, &transferred) == 0) {
        return false;
    }
    return FileWrite(handle, g_render_options->option_states, W8_RENDER_OPTION_COUNT,
                     &transferred) != 0;
}
