#include <cstring>
#include <math.h>

#include "wiz8/engine_code/Environment.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/game_timer.h"
#include "wiz8/engine_code/stTextureAnim.h"
#include "wiz8/screen_state.h"
#include "wiz8/wiz8_windows.h"
#include "wiz8/sr_api.h"
#include "wiz8/virtual_file.h"
#include "surrender/srFog.h"
#include "surrender/srNode.h"
#include "surrender/srScene.h"
#include "surrender/srTypeRegistry.h"

/*
 * Engine Code\Environment.cpp.
 *
 * The world's ambient settings: view distance, the fog and sky flags, the
 * light direction, and the render node the sky is hung from. The globals here
 * keep their addresses in their names where nothing establishes what they are
 * for; the reset that clears six of them together is what groups them.
 */

#define ENVIRONMENT_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\Environment.cpp"

extern "C" float g_view_distance_0060a390;
extern "C" unsigned char g_environment_flag_0060a394;
extern int g_environment_value_0060a3a8;
extern "C" float g_environment_value_0060a3a4;
extern unsigned char g_fog_enabled_0065b9ad;
extern unsigned char g_sky_enabled_0065b9ae;
extern "C" int g_light_direction_0065ad78;
extern "C" int g_light_direction_0065ad7c;
extern "C" int g_light_direction_0065ad80;
extern "C" W8Prop* g_environment_value_0065ad84;
extern "C" W8Prop* g_environment_value_0065a160;
extern "C" stTextureAnim* g_environment_value_0065a168;
extern "C" stTextureAnim* g_environment_value_0065a16c;
extern "C" stTextureAnim* g_environment_value_0065a170;
extern "C" srVector3T<float> g_environment_origin_65ad88;
unsigned long g_tick_65b9a8;
/* 0x00659AB4: the world being rendered. Its sky node is the one field these
   two bodies reach, and it is the same W8World the 3d code walks. */

/* The static and dynamic scene fogs owned by the environment. */
// GLOBAL: WIZ8 0x0065B9B0
srFog* g_environment_object_0065b9b0;
// GLOBAL: WIZ8 0x0065B9B4
srFog* g_environment_object_0065b9b4;
// SYNTHETIC: WIZ8 0x00482250
// `dynamic initializer for 'g_environment_lights_0065b998''
// SYNTHETIC: WIZ8 0x00482270
// `dynamic atexit destructor for 'g_environment_lights_0065b998''
W8GrowableVector<stLight*> g_environment_lights_0065b998(5);

extern void PublishLightDirection(const int* direction);                 /* 0x00427380 */
extern void Function502010(int elapsed);

/* Advance the authoritative game clock and place the two celestial props on
   opposite sides of the world's recovered sky origin. The three animated sky
   gradients consume the same 8-bit day phase, so the clock, prop placement,
   and gradient animation remain one update rather than parallel timers. */
// FUNCTION: WIZ8 0x00482a20
void Function482A20(int elapsed)
{
    unsigned int time = (unsigned int)(g_game_time_ms + elapsed);
    if (time > 86399999U) {
        ++g_game_time_days;
    }
    g_game_time_ms = (int)(time % 86400000U);

    if (g_screen_state_0068ec78.id == W8_SCREEN_MAIN_GAME) {
        Function502010(elapsed);
    }
    g_tick_65b9a8 = GetTickCount();

    const double arc = 3.141592653589793 * (double)(1.0f / 180.0f) * 80.0;
    bool day;
    double angle;
    if ((unsigned int)g_game_time_ms < 18000001U) {
        day = false;
        angle = (double)(g_game_time_ms + 7200000) * arc *
                3.9682539682539686e-08;
    }
    else if ((unsigned int)g_game_time_ms < 79200001U) {
        day = true;
        angle = (double)(g_game_time_ms - 18000000) * arc *
                1.633986928104575e-08;
    }
    else {
        day = false;
        angle = (double)(g_game_time_ms - 79200000) * arc *
                3.9682539682539686e-08;
    }

    srVector3T<float> direction;
    direction.method_00421680(0.0, g_environment_value_0060a3a4, 0.0);

    srMatrix3T<float> rotation;
    rotation.vectors[0].method_00421680(1.0, 0.0, 0.0);
    rotation.vectors[1].method_00421680(0.0, 1.0, 0.0);
    rotation.vectors[2].method_00421680(0.0, 0.0, 1.0);

    angle -= 3.141592653589793 * (double)(1.0f / 180.0f) * 40.0;
    if (angle != 0.0) {
        rotation.method_00438F90(sin(angle), cos(angle));
    }

    srVector3T<float> position;
    position.x = Function4218E0(rotation.vectors[0], direction) +
                 g_environment_origin_65ad88.x;
    position.y = Function4218E0(rotation.vectors[1], direction) +
                 g_environment_origin_65ad88.y;
    position.z = Function4218E0(rotation.vectors[2], direction) +
                 g_environment_origin_65ad88.z;

    W8Prop* moving = day ? g_environment_value_0065a160
                         : g_environment_value_0065ad84;
    W8Prop* opposite = day ? g_environment_value_0065ad84
                           : g_environment_value_0065a160;
    if (moving != 0) {
        moving->Rep()->SetLocation004B8850(&position);
    }
    if (opposite != 0) {
        opposite->Rep()->SetLocation004B8850(&g_environment_origin_65ad88);
    }

    unsigned int phase =
        (((unsigned int)g_game_time_ms / 1000U) << 8) / 86400U;
    stTextureAnim* animations[3] = {
        g_environment_value_0065a168,
        g_environment_value_0065a16c,
        g_environment_value_0065a170
    };
    for (int index = 0; index != 3; ++index) {
        if (animations[index] != 0) {
            animations[index]->frame_58 = (int)phase;
        }
    }
}

// FUNCTION: WIZ8 0x00482990
void Function482990(unsigned char enabled)
{
    if (enabled == 0) {
        g_environment_flag_0060a394 = 0;
        return;
    }

    g_environment_flag_0060a394 = 1;
    g_tick_65b9a8 = GetTickCount();
    if (g_environment_flag_0060a394 != 0) {
        unsigned long now = GetTickCount();
        unsigned long elapsed = now < g_tick_65b9a8
                                    ? now - g_tick_65b9a8 - 1
                                    : now - g_tick_65b9a8;
        if (elapsed != 0) {
            Function482A20((int)((double)elapsed * g_view_distance_0060a390));
        }
    }
}

/* The same component clamp is expanded at every red, green and blue write in
   all four table bodies below; it is source structure shared by those bodies,
   not an optimizer-control annotation. */
#define CLAMP_ENVIRONMENT_COMPONENT(component) \
    do {                                         \
        if (0.0f < (component)) {               \
            if (1.0f <= (component)) {          \
                (component) = 1.0f;             \
            }                                    \
        }                                        \
        else {                                   \
            (component) = 0.0f;                 \
        }                                        \
    } while (0)

// FUNCTION: WIZ8 0x00482F90
unsigned char ReadLightColourTable00482F90(int hFile)
{
    unsigned char components[256 * 3];
    int index;

    memset(components, 0xff, sizeof(components));
    if (hFile == 0 ||
        !ReadVirtualFile(hFile, components, sizeof(components), 0)) {
        return 0;
    }

    for (index = 0; index < 256; ++index) {
        g_environment_colours_65ad98[index].red =
            components[index * 3] * (1.0f / 255.0f);
        g_environment_colours_65ad98[index].green =
            components[index * 3 + 1] * (1.0f / 255.0f);
        g_environment_colours_65ad98[index].blue =
            components[index * 3 + 2] * (1.0f / 255.0f);
        CLAMP_ENVIRONMENT_COMPONENT(g_environment_colours_65ad98[index].red);
        CLAMP_ENVIRONMENT_COMPONENT(g_environment_colours_65ad98[index].green);
        CLAMP_ENVIRONMENT_COMPONENT(g_environment_colours_65ad98[index].blue);
    }
    return 1;
}

// FUNCTION: WIZ8 0x004830D0
unsigned char ReadEnvironmentColourTable004830D0(int hFile)
{
    unsigned char components[256 * 3];
    int index;

    memset(components, 0xff, sizeof(components));
    if (hFile == 0 ||
        !ReadVirtualFile(hFile, components, sizeof(components), 0)) {
        return 0;
    }

    for (index = 0; index < 256; ++index) {
        g_environment_colours_65a178[index].red =
            components[index * 3] * (1.0f / 255.0f);
        g_environment_colours_65a178[index].green =
            components[index * 3 + 1] * (1.0f / 255.0f);
        g_environment_colours_65a178[index].blue =
            components[index * 3 + 2] * (1.0f / 255.0f);
        CLAMP_ENVIRONMENT_COMPONENT(g_environment_colours_65a178[index].red);
        CLAMP_ENVIRONMENT_COMPONENT(g_environment_colours_65a178[index].green);
        CLAMP_ENVIRONMENT_COMPONENT(g_environment_colours_65a178[index].blue);
    }
    return 1;
}

// FUNCTION: WIZ8 0x00483210
void BuildEnvironmentColourRamp00483210(void)
{
    int index;
    float value;

    for (index = 0; index < 128; ++index) {
        value = index * (1.0f / 255.0f);
        g_environment_colours_65a178[index].red = value;
        g_environment_colours_65a178[index].green = value;
        g_environment_colours_65a178[index].blue = value;
        CLAMP_ENVIRONMENT_COMPONENT(g_environment_colours_65a178[index].red);
        CLAMP_ENVIRONMENT_COMPONENT(g_environment_colours_65a178[index].green);
        CLAMP_ENVIRONMENT_COMPONENT(g_environment_colours_65a178[index].blue);
    }
    for (; index < 256; ++index) {
        value = (255 - index) * (1.0f / 255.0f);
        g_environment_colours_65a178[index].red = value;
        g_environment_colours_65a178[index].green = value;
        g_environment_colours_65a178[index].blue = value;
        CLAMP_ENVIRONMENT_COMPONENT(g_environment_colours_65a178[index].red);
        CLAMP_ENVIRONMENT_COMPONENT(g_environment_colours_65a178[index].green);
        CLAMP_ENVIRONMENT_COMPONENT(g_environment_colours_65a178[index].blue);
    }
}

// FUNCTION: WIZ8 0x00483360
void BuildLightColourRamp00483360(void)
{
    int index;
    float value;

    for (index = 0; index < 128; ++index) {
        value = index * (1.0f / 255.0f);
        g_environment_colours_65ad98[index].red = value;
        g_environment_colours_65ad98[index].green = value;
        g_environment_colours_65ad98[index].blue = value;
        CLAMP_ENVIRONMENT_COMPONENT(g_environment_colours_65ad98[index].red);
        CLAMP_ENVIRONMENT_COMPONENT(g_environment_colours_65ad98[index].green);
        CLAMP_ENVIRONMENT_COMPONENT(g_environment_colours_65ad98[index].blue);
    }
    for (; index < 256; ++index) {
        value = (255 - index) * (1.0f / 255.0f);
        g_environment_colours_65ad98[index].red = value;
        g_environment_colours_65ad98[index].green = value;
        g_environment_colours_65ad98[index].blue = value;
        CLAMP_ENVIRONMENT_COMPONENT(g_environment_colours_65ad98[index].red);
        CLAMP_ENVIRONMENT_COMPONENT(g_environment_colours_65ad98[index].green);
        CLAMP_ENVIRONMENT_COMPONENT(g_environment_colours_65ad98[index].blue);
    }
}

// VTABLE: WIZ8 0x005EC94C
// class srClassSupport<srFog,srFog,0,4624>

// TEMPLATE: WIZ8 0x00484700
// srClassSupport<srFog,srFog,0,4624>::getClassID

// TEMPLATE: WIZ8 0x00484710
// srClassSupport<srFog,srFog,0,4624>::getClassName

// TEMPLATE: WIZ8 0x00484720
// srClassSupport<srFog,srFog,0,4624>::getClassNode

// TEMPLATE: WIZ8 0x004847C0
// srClassSupport<srFog,srFog,0,4624>::clone

// SYNTHETIC: WIZ8 0x00484840
// srClassSupport<srFog,srFog,0,4624>::`scalar deleting destructor'

// FUNCTION: WIZ8 0x00483750
void SetSkyEnabled(unsigned char enabled)
{
    if (enabled != 0) {
        if (g_world == 0 || g_world->dynamic_scene == 0 ||
            g_environment_object_0065b9b0 != 0) {
            return;
        }

        g_environment_object_0065b9b0 =
            new srClassSupport<srFog, srFog, false, 0x1210>(
                g_world->static_scene);
        g_environment_object_0065b9b4 =
            new srClassSupport<srFog, srFog, false, 0x1210>(
                g_world->dynamic_scene);
        g_environment_object_0065b9b0->m_positional_28 = 1.0f;
        g_environment_object_0065b9b4->m_positional_28 = 1.0f;

        if (g_environment_object_0065b9b0 != 0 && g_world != 0) {
            g_environment_object_0065b9b0->m_positional_double_20 =
                WorldGetFarClip(g_world) * g_world->environment_range_end_018;
            g_environment_object_0065b9b0->m_positional_double_18 =
                WorldGetFarClip(g_world) * g_world->environment_range_start_014;
            g_environment_object_0065b9b4->m_positional_double_18 =
                WorldGetFarClip(g_world) * g_world->environment_range_start_014;
            g_environment_object_0065b9b4->m_positional_double_20 =
                WorldGetFarClip(g_world) * g_world->environment_range_end_018;
        }

        PublishLightDirection(&g_light_direction_0065ad78);
        if (g_world == 0 || g_world->camera == 0) {
            return;
        }
        g_world->camera->setEnvironmentRange(
            (float)WorldGetFarClip(g_world) * g_world->environment_range_start_014,
            (float)WorldGetFarClip(g_world) * g_world->environment_range_end_018);
        return;
    }

    if (g_environment_object_0065b9b0 != 0) {
        g_environment_object_0065b9b0->release();
    }
    if (g_environment_object_0065b9b4 != 0) {
        g_environment_object_0065b9b4->release();
    }
    g_environment_object_0065b9b0 = 0;
    g_environment_object_0065b9b4 = 0;

    int direction[3] = {0, 0, 0};
    PublishLightDirection(direction);
    if (g_world == 0 || g_world->camera == 0) {
        return;
    }
    g_world->camera->setEnvironmentRange(
        0.0f, (float)WorldGetFarClip(g_world));
}

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

// FUNCTION: WIZ8 0x00482a10
unsigned char GetEnvironmentFlag0060A394(void)
{
    return g_environment_flag_0060a394;
}

// FUNCTION: WIZ8 0x004842f0
int GetEnvironmentValue0060A3A8(void)
{
    return g_environment_value_0060a3a8;
}

/* Fog, which is a plain flag with a matched pair of accessors. */
// FUNCTION: WIZ8 0x00482e80
void SetFogEnabled(unsigned char enabled)
{
    g_fog_enabled_0065b9ad = enabled;
}

// FUNCTION: WIZ8 0x00482e90
unsigned char IsFogEnabled(void)
{
    return g_fog_enabled_0065b9ad;
}

/* The sky, whose flag has to be cleared alongside the work of turning it off,
   so the read and the clear are not symmetric. */
// FUNCTION: WIZ8 0x00482f60
void DisableSky(void)
{
    SetSkyEnabled(0);
    g_sky_enabled_0065b9ae = 0;
}

// FUNCTION: WIZ8 0x00482f80
unsigned char IsSkyEnabled(void)
{
    return g_sky_enabled_0065b9ae;
}

/* Clear the whole ambient block. The six globals reset together are what makes
   them one group; the last is set to minus one rather than zero. */
// FUNCTION: WIZ8 0x004826b0
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

/* The ambient light the world contributes, or nothing at all when the world's
   own gate at 0x3c is clear. Both assertions belong to this body: line 616
   names the world and line 617 names the out-parameter pLightValue, which is
   what makes the three writes a colour triple rather than three fields. */
// FUNCTION: WIZ8 0x004839e0
void GetWorldLightValue(const void* world, int* light_value)
{
    if (world == 0) {
        srAssertFail("pWorld", ENVIRONMENT_CPP, 616, 0);
    }
    if (light_value == 0) {
        srAssertFail("pLightValue", ENVIRONMENT_CPP, 617, 0);
    }
    if (*(const int*)((const char*)world + 0x3c) != 0) {
        light_value[0] = *(const int*)((const char*)world + 0x2c);
        light_value[1] = *(const int*)((const char*)world + 0x30);
        light_value[2] = *(const int*)((const char*)world + 0x34);
    } else {
        light_value[0] = 0;
        light_value[1] = 0;
        light_value[2] = 0;
    }
}

/* Drops the two renderer objects the environment holds and clears the count
   that goes with them. Both releases run through one loaded import address,
   which is what makes the two globals the same class rather than two. */
// FUNCTION: WIZ8 0x004826e0
void ReleaseEnvironmentObjects(void)
{
    if (g_environment_object_0065b9b0 != 0) {
        g_environment_object_0065b9b0->release();
    }
    if (g_environment_object_0065b9b4 != 0) {
        g_environment_object_0065b9b4->release();
    }
    g_environment_object_0065b9b0 = 0;
    g_environment_object_0065b9b4 = 0;
    g_environment_lights_0065b998.Clear();
}

// FUNCTION: WIZ8 0x00483F30
void AddEnvironmentLight00483F30(stLight* light)
{
    if (light != 0) {
        g_environment_lights_0065b998.Add(light);
    }
}

/* One value off the world object, guarded by an assertion that names it. */
// FUNCTION: WIZ8 0x00483ab0
float GetWorldValue24(const void* world)
{
    if (world == 0) {
        srAssertFail("pWorld", ENVIRONMENT_CPP, 648, 0);
    }
    return *(const float*)((const char*)world + 0x24);
}

/* Retain the world's current intensity while replacing its environment
   colour. Retail takes the colour triple by value and forwards its address to
   SetWorldEnvironment. */
// FUNCTION: WIZ8 0x00483a60
void SetWorldEnvironmentColour00483A60(
    W8World* world, EnvironmentColour colour)
{
    if (world == 0) {
        srAssertFail("pWorld", ENVIRONMENT_CPP, 634, 0);
        srAssertFail("pWorld", ENVIRONMENT_CPP, 648, 0);
    }
    SetWorldEnvironment00483BA0(
        world, world->environment_intensity_024, &colour);
}

/* Write one field of the sky node, if the sky has one. */
// FUNCTION: WIZ8 0x00483e30
void SetSkyNodeValue1D0(int value)
{
    unsigned char* sky = (unsigned char*)g_world->camera_light;

    if (sky != 0) {
        *(int*)(sky + 0x1d0) = value;
    }
}

/* Show or hide the sky node, which is the renderer's flag zero the other way
   round: showing it clears the flag. */
// FUNCTION: WIZ8 0x00483e50
void SetSkyNodeVisible(char visible)
{
    srNode* sky = (srNode*)g_world->camera_light;

    if (sky != 0) {
        if (visible) {
            sky->clearFlag(srNode::FLAG_POSITIONAL_0);
        }
        else {
            sky->setFlag(srNode::FLAG_POSITIONAL_0);
        }
    }
}

extern "C" {
// FUNCTION: WIZ8 0x00482720
void Function482720(int value)
{
    g_game_time_ms = value;
    g_tick_65b9a8 = GetTickCount();
}
}

// TEMPLATE: WIZ8 0x004848d0
// srMatrix3T<float>::method_00438F90(double,double)
