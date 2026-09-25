#pragma once

/* Engine Code\Quality.cpp. The startup constructor at 0x0047B500 asserts this
   unit (Quality.cpp:159) where it allocates the shared render-options record. */

/* Public graphics-option ids. The UI mapping plus recovered consumers establish
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
    W8_RENDER_OPTION_CORRECT_BLURRED_TEXT = 16,
    W8_RENDER_OPTION_COUNT = 17
};

void InitializeRenderQuality(void);
void DestroyRenderQuality(void);
void SetRenderOption(int option, int enabled);
void EnableRenderOption(int option);
void DisableRenderOption(int option);
void DisableAllRenderOptions(void);
void EnableAllRenderOptions(void);
unsigned char GetRenderOptionState(int option);
unsigned char LoadRenderOptions(int handle);
bool SaveRenderOptions(int handle);

extern float g_render_brightness_60a210;
extern float g_render_fog_distance_60e610;
extern bool g_render_flag_60a20c;
extern bool g_render_flag_603c6c;
