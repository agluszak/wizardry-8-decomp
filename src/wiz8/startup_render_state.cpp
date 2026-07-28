#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"

#include <stdlib.h>
#include <string.h>

extern "C" {

extern unsigned char* g_render_options_65a118;

/* Engine Code\Quality.cpp allocates one 0x34-byte process-wide record.  Its
   leading fields are still unnamed, but the allocation, clear and reviewed
   defaults are complete observations from the startup constructor. */
// FUNCTION: WIZ8 0x0047B500
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

struct EnvironmentColour {
    float red;
    float green;
    float blue;
};

EnvironmentColour g_environment_colours_65a178[512];
EnvironmentColour g_environment_colours_65ad98[512];
int g_environment_value_0065a168;
int g_environment_value_0065a16c;
unsigned char g_environment_flag_0060a394;
int g_environment_value_0065a160;
int g_environment_value_0065ad84;
int g_environment_state_65b99c;
float g_view_distance_0060a390;
int g_environment_value_0065a170;

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
