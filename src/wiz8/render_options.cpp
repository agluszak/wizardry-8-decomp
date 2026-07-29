#include "wiz8/gameplay_boundaries.h"
#include "wiz8/render_state.h"
#include "surrender/srGERD.h"

/*
 * The render-option table. 0x0047B630 is a switch over 0x11 options that pushes
 * each one into the SurRender device - texture filtering among them - and then
 * records its new state. Its second argument is a boolean: two of the cases
 * store it as `argument != 0` outright, and the rest read as off against on.
 * The three helpers here sit on top of it: turn one option off, turn every
 * option on, and read an option's recorded state back.
 *
 * The stored bytes live two bytes into the block at 0x0065A118, which is what
 * the +2 in the accessor is. Nothing here establishes what that leading pair
 * holds, so it is not modelled.
 */

extern "C" {

float g_render_brightness_60a210;
float g_render_fog_distance_60e610;
unsigned char g_render_flag_60a20c;
unsigned char g_render_flag_603c6c;
int g_resident_texture_policy_659714;
unsigned char g_swap_interval_enabled_659718;
float g_surface_scale_659680;

static void SetSurfaceScale(float scale)
{
    float* values = (float*)((unsigned char*)g_surface_node_659664 + 0x170);
    float factor = 1.0f / (float)*(int*)((unsigned char*)g_surface_node_659664 + 0x144);
    float previous = *(float*)((unsigned char*)g_surface_node_659664 + 0x190);
    int index;

    for (index = 0; index != 8; ++index) {
        values[index] += (scale - previous) * factor;
    }
    *(float*)((unsigned char*)g_surface_node_659664 + 0x190) = scale;
    g_surface_scale_659680 = scale;
}

static void SetResidentTexturePolicy(int policy)
{
    if (policy != g_resident_texture_policy_659714) {
        g_gerd_659634->invalidateResidentTextures();
        g_gerd_659634->invalidateTextureCache();
        g_resident_texture_policy_659714 = policy;
    }
}

static void SetTextureCacheSize(unsigned long bytes)
{
    if (bytes > 0x7fffff && bytes != g_gerd_659634->getTextureCacheSize()) {
        g_gerd_659634->invalidateResidentTextures();
        g_gerd_659634->invalidateTextureCache();
        g_gerd_659634->setTextureCacheSize(bytes);
    }
}

static void SetSwapInterval(unsigned char enabled)
{
    g_swap_interval_enabled_659718 = enabled;
    g_gerd_659634->setSwapInterval(enabled ? 1 : 0);
}

// FUNCTION: WIZ8 0x0047B630
void SetRenderOption(int option, int enabled)
{
    switch (option) {
    case 2: g_gerd_659634->setTextureDefaultMagFilter((srTextureIFace::e_filter)(enabled ? 3 : 0)); break;
    case 3: g_gerd_659634->setTextureDefaultMinFilter((srTextureIFace::e_filter)(enabled ? 3 : 0)); break;
    case 4: g_gerd_659634->setTextureDefaultMipmap((srTextureIFace::e_mipmap)(enabled ? 2 : 0)); break;
    case 5:
        if (((*((unsigned char*)g_gerd_659634 + 0x20) & 1) != 0) != (enabled != 0)) {
            g_gerd_659634->toggle((srGERD::e_enable)0);
        }
        break;
    case 6: g_render_brightness_60a210 = enabled ? 1.0f : 0.8f; break;
    case 7:
        if (!enabled && g_render_fog_distance_60e610 < 0.7f) g_render_fog_distance_60e610 = 0.7f;
        if (enabled && g_render_fog_distance_60e610 > 0.3f) g_render_fog_distance_60e610 = 0.3f;
        break;
    case 8:
        if (!enabled && g_render_fog_distance_60e610 < 0.9f) g_render_fog_distance_60e610 = 0.9f;
        if (enabled && g_render_fog_distance_60e610 > 0.1f) g_render_fog_distance_60e610 = 0.1f;
        break;
    case 9: g_render_flag_60a20c = enabled != 0; break;
    case 10: g_render_flag_603c6c = enabled != 0; break;
    case 11: SetResidentTexturePolicy(enabled ? 0 : 1); break;
    case 12: SetTextureCacheSize(enabled ? 0x2000000 : 0x1000000); break;
    case 13: SetSwapInterval(enabled != 0); break;
    case 16: SetSurfaceScale(enabled ? 0.5f : 0.0f); break;
    }
    if (option < 0x11) {
        g_render_options_65a118[2 + option] = enabled != 0;
    }
}

// FUNCTION: WIZ8 0x0047B5B0
void DisableRenderOption(int option)
{
    if (option < 0x11) {
        SetRenderOption(option, 0);
    }
}

/* The original carries a dead entry test: it compares the counter against the
   bound before the first iteration and, when that fails, jumps to the increment
   rather than past the loop. Starting at zero it can never fire, and VC6 folds
   it away here whichever way the loop is written - for, while and do-while all
   give the same 22 bytes. The five-byte difference is that fold, not a
   difference in what the loop does. */
// FUNCTION: WIZ8 0x0047B5F0
void EnableAllRenderOptions(void)
{
    int option;

    option = 0;
    while (option < 0x11) {
        SetRenderOption(option, 1);
        option++;
    }
}

/* Out-of-range reads report zero rather than indexing past the block. */
// FUNCTION: WIZ8 0x0047B610
unsigned char GetRenderOptionState(int option)
{
    if (option >= 0x11) {
        return 0;
    }
    return g_render_options_65a118[2 + option];
}

}
