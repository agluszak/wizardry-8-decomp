#include "wiz8/engine_code/Environment.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/engine_code/stLight.hpp"
#include "wiz8/engine_code/stTextureAnim.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/sr_api.h"

#include <stdlib.h>
#include <string.h>

// GLOBAL: WIZ8 0x0065a154
unsigned int g_frame_tick;
// GLOBAL: WIZ8 0x0065a158
float g_frame_elapsed;

/* The renderer's shared millisecond delta. The constructor immediately before
   this body seeds the same clock; every consumer reads the single scaled
   elapsed value rather than maintaining a parallel frame timer. */
// FUNCTION: WIZ8 0x00482140
void UpdateRenderElapsedTime(void)
{
    unsigned int now = GetTickCount();
    unsigned int elapsed = now - g_frame_tick;
    g_frame_tick = now;
    g_frame_elapsed = elapsed * 0.001f;
}

// GLOBAL: WIZ8 0x0065A178
EnvironmentColour g_environment_colours_65a178[256];
// GLOBAL: WIZ8 0x0065AD98
EnvironmentColour g_environment_colours_65ad98[256];
// GLOBAL: WIZ8 0x0065A168
stTextureAnim* g_sky_gradient_animations[3];
// GLOBAL: WIZ8 0x0060A394
bool g_environment_time_enabled = true;
// GLOBAL: WIZ8 0x0065A160
W8Prop* g_sun_prop;
// GLOBAL: WIZ8 0x0065AD84
W8Prop* g_moon_prop;
// GLOBAL: WIZ8 0x0065AD88
srVector3T<float> g_celestial_origin;
// GLOBAL: WIZ8 0x0060A390
float g_view_distance = 12.0f;
// GLOBAL: WIZ8 0x0060A3A4
float g_celestial_orbit_radius = -1.0f;
// GLOBAL: WIZ8 0x0065AD78
EnvironmentColour g_light_direction;

static float normalized_colour(unsigned int component)
{
    return component * (1.0f / 255.0f);
}

/* Builds the two 512-entry greyscale ramps consumed by the environment
   renderer.  Each ramp rises from black through 127/255, then falls from
   127/255 to zero. Retail builds one array at a time in four pointer-walk
   loops and saturates every element through SaturateColor; this body
   fills both arrays in two loops, so its shape is still an open mismatch. */
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
    g_sky_gradient_animations[0] = 0;
    g_sky_gradient_animations[1] = 0;
    g_environment_time_enabled = 0;
    g_sun_prop = 0;
    g_moon_prop = 0;
    g_environment_lights.Clear();
    g_view_distance = 12.0f;
    g_sky_gradient_animations[2] = 0;
    return 1;
}
