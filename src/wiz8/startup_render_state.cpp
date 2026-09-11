#include "wiz8/engine_code/Environment.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/engine_code/stTextureAnim.h"
#include "wiz8/render_state.h"
#include "wiz8/sr_api.h"

#include <stdlib.h>
#include <string.h>

// GLOBAL: WIZ8 0x0065a154
unsigned int g_frame_tick_65a154;
// GLOBAL: WIZ8 0x0065a158
float g_frame_elapsed_65a158;

/* The renderer's shared millisecond delta. The constructor immediately before
   this body seeds the same clock; every consumer reads the single scaled
   elapsed value rather than maintaining a parallel frame timer. */
// FUNCTION: WIZ8 0x00482140
void UpdateRenderElapsedTime00482140(void)
{
    unsigned int now = GetTickCount();
    unsigned int elapsed = now - g_frame_tick_65a154;
    g_frame_tick_65a154 = now;
    g_frame_elapsed_65a158 = static_cast<float>(elapsed) * 0.001f;
}

// GLOBAL: WIZ8 0x0065A178
EnvironmentColour g_environment_colours_65a178[256];
// GLOBAL: WIZ8 0x0065AD98
EnvironmentColour g_environment_colours_65ad98[256];
// GLOBAL: WIZ8 0x0065A168
stTextureAnim* g_environment_value_0065a168;
// GLOBAL: WIZ8 0x0065A16C
stTextureAnim* g_environment_value_0065a16c;
// GLOBAL: WIZ8 0x0060A394
unsigned char g_environment_flag_0060a394;
// GLOBAL: WIZ8 0x0065A160
W8Prop* g_environment_value_0065a160;
// GLOBAL: WIZ8 0x0065AD84
W8Prop* g_environment_value_0065ad84;
// GLOBAL: WIZ8 0x0065AD88
srVector3T<float> g_environment_origin_65ad88;
// GLOBAL: WIZ8 0x0065B99C
int g_environment_state_65b99c;
// GLOBAL: WIZ8 0x0060A390
float g_view_distance_0060a390;
// GLOBAL: WIZ8 0x0060A3A4
float g_environment_value_0060a3a4;
// GLOBAL: WIZ8 0x0065AD78
EnvironmentColour g_light_direction_0065ad78;
// GLOBAL: WIZ8 0x0065A170
stTextureAnim* g_environment_value_0065a170;

static float normalized_colour(unsigned int component)
{
    return (float)component * (1.0f / 255.0f);
}

/* Builds the two 512-entry greyscale ramps consumed by the environment
   renderer.  Each ramp rises from black through 127/255, then falls from
   127/255 to zero, matching the two loops at the original address. */
// FUNCTION: WIZ8 0x00482280
unsigned char InitializeEnvironmentColours(void)
{
    unsigned int index;
    float value;

    for (index = 0; index != 128; ++index) {
        value = normalized_colour(index);
        g_environment_colours_65a178[index] = value;
        g_environment_colours_65ad98[index] = value;
    }
    for (index = 128; index != 256; ++index) {
        value = normalized_colour(255 - index);
        g_environment_colours_65a178[index] = value;
        g_environment_colours_65ad98[index] = value;
    }
    g_environment_value_0065a168 = 0;
    g_environment_value_0065a16c = 0;
    g_environment_flag_0060a394 = 0;
    g_environment_value_0065a160 = 0;
    g_environment_value_0065ad84 = 0;
    g_environment_state_65b99c = 0;
    g_view_distance_0060a390 = 12.0f;
    g_environment_value_0065a170 = 0;
    return 1;
}
