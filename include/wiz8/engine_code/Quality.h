#pragma once

/* Engine Code\Quality.cpp. The startup constructor at 0x0047B500 asserts this
   unit (Quality.cpp:159) where it allocates the shared render-options record. */

/* Engine option ids. The UI mapping plus recovered consumers establish
   all nine user-visible entries: mip mapping calls the SR mipmap setter,
   missile lights gate spell/missile lights, mesh sky gates the separate sky
   world, high texture cache selects 16/32 MiB, video sync sets swap interval,
   additional animations selects the richer monster-cycle values, and blurred
   text applies the half-pixel 2D correction. */
enum W8RenderOption {
    W8_RENDER_OPTION_MIP_MAPPING = 4,
    W8_RENDER_OPTION_DITHER = 5,
    W8_RENDER_OPTION_MISSILE_LIGHTS = 9,
    W8_RENDER_OPTION_MESH_SKY = 10,
    W8_RENDER_OPTION_HIGH_TEXTURE_DETAIL = 11,
    W8_RENDER_OPTION_HIGH_TEXTURE_CACHE = 12,
    W8_RENDER_OPTION_VIDEO_SYNC = 13,
    W8_RENDER_OPTION_ADDITIONAL_ANIMATIONS = 14,
    /* PlayFootstep (0x0047a440) and the audio panel mute button agree on 15. */
    W8_RENDER_OPTION_FOOTSTEP_SOUND = 15,
    W8_RENDER_OPTION_CORRECT_BLURRED_TEXT = 16,
    W8_RENDER_OPTION_COUNT = 17
};

/* InitializeRenderQuality's 0x34-byte allocation; SetRenderOption and the
   save path establish the seventeen byte states starting at +0x02.
   The other fields' roles remain unresolved. */
struct W8RenderQuality {
    unsigned char unknown_00[2];
    unsigned char option_states[W8_RENDER_OPTION_COUNT];
    unsigned char unknown_13;
    unsigned char unknown_14;
    unsigned char unknown_15[11];
    unsigned int unknown_20;
    unsigned int unknown_24;
    unsigned char unknown_28[4];
    unsigned int unknown_2c;
    unsigned char unknown_30[4];
};

static_assert(sizeof(W8RenderQuality) == 0x34, "W8RenderQuality_size");
extern W8RenderQuality* g_render_options;

void InitializeRenderQuality(void);
void DestroyRenderQuality(void);
void SetRenderOption(W8RenderOption option, int enabled);
void EnableRenderOption(W8RenderOption option);
void DisableRenderOption(W8RenderOption option);
void DisableAllRenderOptions(void);
void EnableAllRenderOptions(void);
unsigned char GetRenderOptionState(W8RenderOption option);
unsigned char LoadRenderOptions(int handle);
bool SaveRenderOptions(int handle);

extern float g_render_brightness;
extern float g_render_fog_distance;
extern bool g_render_missile_lights;
extern bool g_render_mesh_sky;
