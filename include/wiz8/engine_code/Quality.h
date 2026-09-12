#pragma once

/* Engine Code\Quality.cpp. The startup constructor at 0x0047B500 asserts this
   unit (Quality.cpp:159) where it allocates the shared render-options record. */

void InitializeRenderQuality(void);
void DestroyRenderQuality0047B570(void);
void SetRenderOption(int option, int enabled);
void EnableRenderOption(int option);
void DisableRenderOption(int option);
void DisableAllRenderOptions0047B5D0(void);
void EnableAllRenderOptions(void);
unsigned char GetRenderOptionState(int option);
unsigned char LoadRenderOptions0047B890(int handle);
bool SaveRenderOptions0047B920(int handle);

extern unsigned char* g_render_options_65a118;
extern float g_render_brightness_60a210;
extern float g_render_fog_distance_60e610;
extern unsigned char g_render_flag_60a20c;
extern unsigned char g_render_flag_603c6c;
