#include "wiz8/engine_code/Environment.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/engine_code/stTextureAnim.h"
#include "wiz8/render_state.h"
#include "wiz8/sr_api.h"

#include <stdlib.h>
#include <string.h>

extern "C" {


/* Engine Code\Quality.cpp allocates one 0x34-byte process-wide record.  Its
   leading fields are still unnamed, but the allocation, clear and reviewed
   defaults are complete observations from the startup constructor. */
// FUNCTION: WIZ8 0x0047b500
void InitializeRenderQuality(void)
{
    unsigned int* quality;

    g_render_options_65a118 = (unsigned char*)malloc(0x34);
    if (!g_render_options_65a118) {
        srAssertFail("gpQuality",
                     "C:\\Projects\\Wizardry 8\\Engine Code\\Quality.cpp",
                     159, 0);
        return;
    }
    memset(g_render_options_65a118, 0, 0x34);
    quality = (unsigned int*)g_render_options_65a118;
    quality[8] = 0xffffffff;
    quality[9] = 0xffffffff;
    g_render_options_65a118[20] = 1;
    quality[11] = 3;
}

unsigned int g_frame_tick_65a154;
float g_frame_elapsed_65a158;

/* The renderer's shared millisecond delta. The constructor immediately before
   this body seeds the same clock; every consumer reads the single scaled
   elapsed value rather than maintaining a parallel frame timer. */
// FUNCTION: WIZ8 0x00482140
void Function482140(void)
{
    unsigned int now = GetTickCount();
    unsigned int elapsed = now - g_frame_tick_65a154;
    g_frame_tick_65a154 = now;
    g_frame_elapsed_65a158 = static_cast<float>(elapsed) * 0.001f;
}

EnvironmentColour g_environment_colours_65a178[256];
EnvironmentColour g_environment_colours_65ad98[256];
stTextureAnim* g_environment_value_0065a168;
stTextureAnim* g_environment_value_0065a16c;
unsigned char g_environment_flag_0060a394;
W8Prop* g_environment_value_0065a160;
W8Prop* g_environment_value_0065ad84;
srVector3T<float> g_environment_origin_65ad88;
int g_environment_state_65b99c;
float g_view_distance_0060a390;
float g_environment_value_0060a3a4;
int g_light_direction_0065ad78;
int g_light_direction_0065ad7c;
int g_light_direction_0065ad80;
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
        g_environment_colours_65a178[index].red = value;
        g_environment_colours_65a178[index].green = value;
        g_environment_colours_65a178[index].blue = value;
        g_environment_colours_65ad98[index].red = value;
        g_environment_colours_65ad98[index].green = value;
        g_environment_colours_65ad98[index].blue = value;
    }
    for (index = 128; index != 256; ++index) {
        value = normalized_colour(255 - index);
        g_environment_colours_65a178[index].red = value;
        g_environment_colours_65a178[index].green = value;
        g_environment_colours_65a178[index].blue = value;
        g_environment_colours_65ad98[index].red = value;
        g_environment_colours_65ad98[index].green = value;
        g_environment_colours_65ad98[index].blue = value;
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

}
#include "wiz8/render_state.h"
