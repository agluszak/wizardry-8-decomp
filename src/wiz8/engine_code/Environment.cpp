#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"

/* The renderer node the sky hangs from. Only the two flag methods this file
   reaches are declared; the class itself is SurRender's. */
class srNode {
public:
    void setFlag(int flag);
    void clearFlag(int flag);
};

/*
 * Engine Code\Environment.cpp.
 *
 * The world's ambient settings: view distance, the fog and sky flags, the
 * light direction, and the render node the sky is hung from. The globals here
 * keep their addresses in their names where nothing establishes what they are
 * for; the reset that clears six of them together is what groups them.
 */

#define ENVIRONMENT_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\Environment.cpp"

extern float g_view_distance_0060a390;
extern unsigned char g_environment_flag_0060a394;
extern int g_environment_value_0060a3a8;
extern float g_environment_value_0060a3a4;
extern unsigned char g_fog_enabled_0065b9ad;
extern unsigned char g_sky_enabled_0065b9ae;
extern int g_light_direction_0065ad78;
extern int g_light_direction_0065ad7c;
extern int g_light_direction_0065ad80;
extern int g_environment_value_0065ad84;
extern int g_environment_value_0065a160;
extern int g_environment_value_0065a168;
extern int g_environment_value_0065a16c;
extern int g_environment_value_0065a170;
/* 0x00659AB4: the world being rendered. Its sky node is the one field these
   two bodies reach, and it is the same W8World the 3d code walks. */
extern W8World* g_render_world_00659ab4;

extern void SetSkyEnabled(int enabled);                                  /* 0x00483750 */
extern void PublishLightDirection(const int* direction);                 /* 0x00427380 */

/* The far plane the world is drawn to. */
// FUNCTION: WIZ8 0x00482750
void SetViewDistance(float distance)
{
    g_view_distance_0060a390 = distance;
}

// FUNCTION: WIZ8 0x00482760
float GetViewDistance(void)
{
    return g_view_distance_0060a390;
}

// FUNCTION: WIZ8 0x00482A10
unsigned char GetEnvironmentFlag0060A394(void)
{
    return g_environment_flag_0060a394;
}

// FUNCTION: WIZ8 0x004842F0
int GetEnvironmentValue0060A3A8(void)
{
    return g_environment_value_0060a3a8;
}

/* Fog, which is a plain flag with a matched pair of accessors. */
// FUNCTION: WIZ8 0x00482E80
void SetFogEnabled(unsigned char enabled)
{
    g_fog_enabled_0065b9ad = enabled;
}

// FUNCTION: WIZ8 0x00482E90
unsigned char IsFogEnabled(void)
{
    return g_fog_enabled_0065b9ad;
}

/* The sky, whose flag has to be cleared alongside the work of turning it off,
   so the read and the clear are not symmetric. */
// FUNCTION: WIZ8 0x00482F60
void DisableSky(void)
{
    SetSkyEnabled(0);
    g_sky_enabled_0065b9ae = 0;
}

// FUNCTION: WIZ8 0x00482F80
unsigned char IsSkyEnabled(void)
{
    return g_sky_enabled_0065b9ae;
}

/* Clear the whole ambient block. The six globals reset together are what makes
   them one group; the last is set to minus one rather than zero. */
// FUNCTION: WIZ8 0x004826B0
void ResetEnvironment(void)
{
    g_environment_value_0065a168 = 0;
    g_environment_value_0065a16c = 0;
    g_environment_value_0065a170 = 0;
    g_environment_value_0065a160 = 0;
    g_environment_value_0065ad84 = 0;
    g_environment_value_0060a3a4 = -1.0f;
}

/* The direction light comes from. Setting it also hands the new direction to
   the renderer, so the two are not a plain field pair. */
// FUNCTION: WIZ8 0x00483650
void SetLightDirection(const int* direction)
{
    g_light_direction_0065ad78 = direction[0];
    g_light_direction_0065ad7c = direction[1];
    g_light_direction_0065ad80 = direction[2];
    PublishLightDirection(direction);
}

// FUNCTION: WIZ8 0x00483680
void GetLightDirection(int* direction)
{
    direction[0] = g_light_direction_0065ad78;
    direction[1] = g_light_direction_0065ad7c;
    direction[2] = g_light_direction_0065ad80;
}

/* One value off the world object, guarded by an assertion that names it. */
// FUNCTION: WIZ8 0x00483AB0
float GetWorldValue24(const void* world)
{
    if (world == 0) {
        srAssertFail("pWorld", ENVIRONMENT_CPP, 648, 0);
    }
    return *(const float*)((const char*)world + 0x24);
}

/* Write one field of the sky node, if the sky has one. */
// FUNCTION: WIZ8 0x00483E30
void SetSkyNodeValue1D0(int value)
{
    unsigned char* sky = (unsigned char*)g_render_world_00659ab4->sky_node;

    if (sky != 0) {
        *(int*)(sky + 0x1d0) = value;
    }
}

/* Show or hide the sky node, which is the renderer's flag zero the other way
   round: showing it clears the flag. */
// FUNCTION: WIZ8 0x00483E50
void SetSkyNodeVisible(char visible)
{
    srNode* sky = (srNode*)g_render_world_00659ab4->sky_node;

    if (sky != 0) {
        if (visible) {
            sky->clearFlag(0);
        }
        else {
            sky->setFlag(0);
        }
    }
}
