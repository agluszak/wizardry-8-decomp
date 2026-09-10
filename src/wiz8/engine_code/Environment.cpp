#include <cstring>
#include <math.h>

#include "wiz8/engine_code/Environment.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/game_timer.h"
#include "wiz8/engine_code/stTextureAnim.h"
#include "wiz8/float_constants.h"
#include "wiz8/render_state.h"
#include "wiz8/screen_state.h"
#include "wiz8/wiz8_windows.h"
#include "wiz8/sr_api.h"
#include "wiz8/virtual_file.h"
#include "surrender/srCore.h"
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

// GLOBAL: WIZ8 0x0060a3a8
int g_environment_value_0060a3a8 = 2;
extern "C" float g_environment_value_0060a3a4;

// GLOBAL: WIZ8 0x0065b9ad
unsigned char g_fog_enabled_0065b9ad;

// GLOBAL: WIZ8 0x0065b9ae
unsigned char g_sky_enabled_0065b9ae;
extern "C" int g_light_direction_0065ad78;
extern "C" int g_light_direction_0065ad7c;
extern "C" int g_light_direction_0065ad80;
extern "C" W8Prop* g_environment_value_0065ad84;
extern "C" W8Prop* g_environment_value_0065a160;
extern "C" stTextureAnim* g_environment_value_0065a168;
extern "C" stTextureAnim* g_environment_value_0065a16c;
extern "C" stTextureAnim* g_environment_value_0065a170;
extern "C" srVector3T<float> g_environment_origin_65ad88;
// GLOBAL: WIZ8 0x0065B9A8
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

// GLOBAL: WIZ8 0x0065b9b8
float g_environment_value_0065b9b8;
// GLOBAL: WIZ8 0x0060a3ac
int g_environment_value_0060a3ac = -1;
// GLOBAL: WIZ8 0x0060a395
unsigned char g_flag_0060a395 = 1;

/* The mapper starts its scroll rate at 0.002 texture units per second on x
   only and reseeds the shared frame clock, so the first scrolled frame uses
   the interval since this object's construction rather than since startup. */
// FUNCTION: WIZ8 0x00482010
W8MaterialMapper00482010::W8MaterialMapper00482010()
{
    value_04 = 0.002f;
    value_08 = 0.0f;
    g_frame_tick_65a154 = GetTickCount();
}

// FUNCTION: WIZ8 0x00482040
int W8MaterialMapper00482010::isActive(srVertexPipe&)
{
    return 1;
}

/* Add the frame-scaled rate to each axis and keep only the fractional part,
   then shift the first texture coordinate set of every vertex by it. */
// FUNCTION: WIZ8 0x00482050
void W8MaterialMapper00482010::process(srVertexPipe& pipe)
{
    float offset_x;
    float offset_y;

    if (!pipe.isChannelAvailable(
            static_cast<srVertexProcessor::e_channel>(5))) {
        return;
    }
    unsigned long count = pipe.getVertexCount();
    srCore.getStatisticsManager()->statistics_00
        .texture_coordinate_operations_34 += count;
    srVector2T<float>* coordinates = pipe.getST(0, 1);

    offset_x = value_04 * g_frame_elapsed_65a158 + offset_14;
    offset_14 = offset_x - static_cast<float>(floor(offset_x));
    offset_y = value_08 * g_frame_elapsed_65a158 + offset_18;
    offset_18 = offset_y - static_cast<float>(floor(offset_y));

    for (unsigned long index = 0; index < count; ++index) {
        coordinates[index].x += offset_14;
        coordinates[index].y += offset_18;
    }
}

/* Advance the authoritative game clock and place the two celestial props on
   opposite sides of the world's recovered sky origin. The three animated sky
   gradients consume the same 8-bit day phase, so the clock, prop placement,
   and gradient animation remain one update rather than parallel timers. */
// FUNCTION: WIZ8 0x00482a20
void Function482A20(int elapsed)
{
    unsigned int time = (unsigned int)(g_status_685170.game_time_ms + elapsed);
    if (time > 86399999U) {
        ++g_status_685170.game_time_days;
    }
    g_status_685170.game_time_ms = (int)(time % 86400000U);

    if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME) {
        Function502010(elapsed);
    }
    g_tick_65b9a8 = GetTickCount();

    const double arc = 3.141592653589793 * (double)(1.0f / 180.0f) * 80.0;
    bool day;
    double angle;
    if ((unsigned int)g_status_685170.game_time_ms < 18000001U) {
        day = false;
        angle = (double)(g_status_685170.game_time_ms + 7200000) * arc *
                3.9682539682539686e-08;
    }
    else if ((unsigned int)g_status_685170.game_time_ms < 79200001U) {
        day = true;
        angle = (double)(g_status_685170.game_time_ms - 18000000) * arc *
                1.633986928104575e-08;
    }
    else {
        day = false;
        angle = (double)(g_status_685170.game_time_ms - 79200000) * arc *
                3.9682539682539686e-08;
    }

    srVector3T<float> direction;
    direction.Set(0.0, g_environment_value_0060a3a4, 0.0);

    srMatrix3T<float> rotation;
    rotation.vectors[0].Set(1.0, 0.0, 0.0);
    rotation.vectors[1].Set(0.0, 1.0, 0.0);
    rotation.vectors[2].Set(0.0, 0.0, 1.0);

    angle -= 3.141592653589793 * (double)(1.0f / 180.0f) * 40.0;
    if (angle != 0.0) {
        rotation.RotateAboutY(sin(angle), cos(angle));
    }

    srVector3T<float> position;
    position.x = DotProduct(rotation.vectors[0], direction) +
                 g_environment_origin_65ad88.x;
    position.y = DotProduct(rotation.vectors[1], direction) +
                 g_environment_origin_65ad88.y;
    position.z = DotProduct(rotation.vectors[2], direction) +
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
        (((unsigned int)g_status_685170.game_time_ms / 1000U) << 8) / 86400U;
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

/* The environment's per-frame update. A bypass value short-circuits to the
   alternate body. Otherwise, in day/night mode the sky consumes the day phase
   to pick the light direction, and the world colour table is refreshed from
   the same phase whenever that flag is set; any other mode just advances the
   clock. Every path that advances the clock converts the elapsed ticks with
   the current view distance. */
// FUNCTION: WIZ8 0x00482770
void UpdateEnvironment482770(void)
{
    if (g_environment_value_0065b9b8 != g_float_005ebb34) {
        UpdateEnvironmentLighting00484300();
        return;
    }
    if (g_environment_flag_0060a394 == 0) {
        return;
    }
    if (g_environment_value_0060a3a8 == 2) {
        if (g_sky_enabled_0065b9ae != 0) {
            unsigned long now = GetTickCount();
            unsigned long elapsed = now < g_tick_65b9a8
                                        ? now - g_tick_65b9a8 - 1
                                        : now - g_tick_65b9a8;
            if (elapsed != 0) {
                Function482A20(
                    (int)((double)elapsed * g_view_distance_0060a390));
            }
            unsigned int phase =
                (((unsigned int)g_status_685170.game_time_ms / 1000U) << 8) /
                86400U;
            if (phase != (unsigned int)g_environment_value_0060a3ac) {
                const int* direction = reinterpret_cast<const int*>(
                    &g_environment_colours_65ad98[phase]);
                g_light_direction_0065ad78 = direction[0];
                g_light_direction_0065ad7c = direction[1];
                g_light_direction_0065ad80 = direction[2];
                PublishLightDirection(direction);
                g_environment_value_0060a3ac = (int)phase;
            }
        }
        if (g_flag_0060a395 != 0) {
            if (g_environment_flag_0060a394 != 0) {
                unsigned long now = GetTickCount();
                unsigned long elapsed = now < g_tick_65b9a8
                                            ? now - g_tick_65b9a8 - 1
                                            : now - g_tick_65b9a8;
                if (elapsed != 0) {
                    Function482A20(
                        (int)((double)elapsed * g_view_distance_0060a390));
                }
            }
            unsigned int phase =
                (((unsigned int)g_status_685170.game_time_ms / 1000U) << 8) /
                86400U;
            if (phase != (unsigned int)g_environment_value_0060a3b0) {
                EnvironmentColour colour = g_environment_colours_65a178[phase];
                if (g_world == 0) {
                    srAssertFail("pWorld", ENVIRONMENT_CPP, 634, 0);
                    srAssertFail("pWorld", ENVIRONMENT_CPP, 648, 0);
                }
                SetWorldEnvironment00483BA0(
                    g_world, g_world->environment_intensity_024, &colour);
                g_environment_value_0060a3b0 = (int)phase;
                return;
            }
        }
    }
    else {
        unsigned long now = GetTickCount();
        unsigned long elapsed = now < g_tick_65b9a8 ? now - g_tick_65b9a8 - 1
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
        !FileRead(hFile, components, sizeof(components), 0)) {
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
        !FileRead(hFile, components, sizeof(components), 0)) {
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
        g_environment_colours_65a178[index] = value;
        CLAMP_ENVIRONMENT_COMPONENT(g_environment_colours_65a178[index].red);
        CLAMP_ENVIRONMENT_COMPONENT(g_environment_colours_65a178[index].green);
        CLAMP_ENVIRONMENT_COMPONENT(g_environment_colours_65a178[index].blue);
    }
    for (; index < 256; ++index) {
        value = (255 - index) * (1.0f / 255.0f);
        g_environment_colours_65a178[index] = value;
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
            SR_NEW(srFog)(
                g_world->static_scene);
        g_environment_object_0065b9b4 =
            SR_NEW(srFog)(
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
// FUNCTION: WIZ8 0x00482EA0
void EnableSky(void)
{
    SetSkyEnabled(1);
    g_sky_enabled_0065b9ae = 1;
    if (g_environment_flag_0060a394 != 0) {
        unsigned long now = GetTickCount();
        unsigned long elapsed = now < g_tick_65b9a8
                                    ? now - g_tick_65b9a8 - 1
                                    : now - g_tick_65b9a8;
        if (elapsed != 0) {
            Function482A20((int)((double)elapsed * g_view_distance_0060a390));
        }
    }
    unsigned int phase =
        (((unsigned int)g_status_685170.game_time_ms / 1000U) << 8) / 86400U;
    if (phase != (unsigned int)g_environment_value_0060a3a8) {
        g_light_direction_0065ad78 =
            reinterpret_cast<const int*>(&g_environment_colours_65ad98[phase])[0];
        g_light_direction_0065ad7c =
            reinterpret_cast<const int*>(&g_environment_colours_65ad98[phase])[1];
        g_light_direction_0065ad80 =
            reinterpret_cast<const int*>(&g_environment_colours_65ad98[phase])[2];
        PublishLightDirection(
            reinterpret_cast<const int*>(&g_environment_colours_65ad98[phase]));
        g_environment_value_0060a3a8 = (int)phase;
    }
}

// GLOBAL: WIZ8 0x0060a3b0
int g_environment_value_0060a3b0 = -1;

/* Advance the clock-driven sky and refresh the world's environment colour
   from the day-phase table when the phase turns over. */
// FUNCTION: WIZ8 0x00483560
void RefreshEnvironment00483560(void)
{
    if (g_environment_flag_0060a394 != 0) {
        unsigned long now = GetTickCount();
        unsigned long elapsed = now < g_tick_65b9a8
                                    ? now - g_tick_65b9a8 - 1
                                    : now - g_tick_65b9a8;
        if (elapsed != 0) {
            Function482A20((int)((double)elapsed * g_view_distance_0060a390));
        }
    }
    unsigned int phase =
        (((unsigned int)g_status_685170.game_time_ms / 1000U) << 8) / 86400U;
    if (phase != (unsigned int)g_environment_value_0060a3b0) {
        EnvironmentColour colour = g_environment_colours_65a178[phase];
        if (g_world == 0) {
            srAssertFail("pWorld", ENVIRONMENT_CPP, 634, 0);
            srAssertFail("pWorld", ENVIRONMENT_CPP, 648, 0);
        }
        SetWorldEnvironment00483BA0(
            g_world, g_world->environment_intensity_024, &colour);
        g_environment_value_0060a3b0 = (int)phase;
    }
}

/* Set the world's environment intensity, clamped to the unit range, keeping
   its current colour unless the world has no static scene to take it from. */
// FUNCTION: WIZ8 0x00483AE0
void SetWorldEnvironmentValue00483AE0(W8World* world, float value)
{
    EnvironmentColour colour;

    if (world == 0) {
        srAssertFail("pWorld", ENVIRONMENT_CPP, 0x298, 0);
    }
    if (g_double_005ebc30 <= value || g_zero_005ebb40 < value) {
        if (g_double_005ebc30 <= value) {
            value = (float)g_double_005ebc30;
        }
    }
    else {
        value = (float)g_zero_005ebb40;
    }
    if (world->static_scene == 0) {
        colour.red = 0.0f;
        colour.green = 0.0f;
        SetWorldEnvironment00483BA0(world, value, &colour);
        return;
    }
    SetWorldEnvironment00483BA0(world, value, &world->environment_colour_02c);
}

// FUNCTION: WIZ8 0x00482F60
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

/* Refresh the two fog objects' ranges from the world's far clip. Runs after
   the camera's clip range moves, so the fog tracks the same plane. */
// FUNCTION: WIZ8 0x004836A0
void Function4836A0(void)
{
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
   what makes the three writes a colour triple rather than three  */
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

// FUNCTION: WIZ8 0x00482720
void Function482720(int value)
{
    g_status_685170.game_time_ms = value;
    g_tick_65b9a8 = GetTickCount();
}

// TEMPLATE: WIZ8 0x004848d0
// srMatrix3T<float>::RotateAboutY(double,double)

// FUNCTION: WIZ8 0x00482740
void Function482740(int value)
{
    g_status_685170.game_time_days = value;
}

// GLOBAL: WIZ8 0x0060a398
const char* g_sky_gradient_names_0060a398[3] = {
    "SkyGrad0000.ifl", "Skytop0000.ifl", "Horizon0000.ifl"};

/* Level-entry environment setup: locate the level's Sun and Moon props, derive
   the environment origin and range from the distance between them, register
   the three sky gradient textures, advance the day clock and publish the
   current day phase's colour and light direction. */
// FUNCTION: WIZ8 0x00482410
void Function482410(void)
{
    if (g_world_659ab8 != 0) {
        g_environment_value_0065a160 = FindPropByName(g_world_659ab8, "Sun");
        g_environment_value_0065ad84 = FindPropByName(g_world_659ab8, "Moon");
    }
    if (g_environment_value_0060a3a4 < g_float_005ebb34) {
        if (g_environment_value_0065a160 == 0
            || g_environment_value_0065ad84 == 0) {
            g_environment_value_0065a160 = 0;
            g_environment_value_0065ad84 = 0;
        }
        else {
            srVector3T<float> sun;
            srVector3T<float> moon;
            stTextureAnim** sky_gradient_slots[3] = {
                &g_environment_value_0065a168, &g_environment_value_0065a16c,
                &g_environment_value_0065a170};

            static_cast<W8AnimRepBase005EC1D8*>(
                g_environment_value_0065a160->m_pRep)
                ->GetLocation004B8890(&sun);
            static_cast<W8AnimRepBase005EC1D8*>(
                g_environment_value_0065ad84->m_pRep)
                ->GetLocation004B8890(&moon);
            srVector3T<float> delta = sun - moon;
            srVector3T<float> midpoint = (sun + moon) * 0.5;
            g_environment_value_0060a3a4 = delta.Length() * 0.5f;
            g_environment_origin_65ad88 = midpoint;
            for (int index = 0; index < 3; ++index) {
                const char* name = g_sky_gradient_names_0060a398[index];
                srRegistry* registry = srCore.getRegistry();
                srRegistry::ClassNode* node = registry->getClassNode(0x10000);

                if (node == 0) {
                    node = registry->registerClass(
                        "stTextureAnim", stTextureAnim::sGetClassNode(), 0x10000,
                        0);
                }
                stTextureAnim* animation = static_cast<stTextureAnim*>(
                    registry->find(
                        node, name, static_cast<const srRuntimeClass*>(0)));

                *sky_gradient_slots[index] = animation;
                if (animation != 0) {
                    animation->flag_60 = 3;
                }
            }
        }
    }
    if (g_environment_flag_0060a394 != 0) {
        unsigned int now = GetTickCount();
        unsigned int elapsed;

        if (now < g_tick_65b9a8) {
            elapsed = now - g_tick_65b9a8 - 1;
        }
        else {
            elapsed = now - g_tick_65b9a8;
        }
        if (elapsed != 0) {
            Function482A20(static_cast<int>(elapsed));
        }
    }
    {
        unsigned int phase =
            ((unsigned int)g_status_685170.game_time_ms / 1000U << 8) / 0x15180;
        EnvironmentColour colour = g_environment_colours_65a178[phase];

        if (g_world == 0) {
            srAssertFail("pWorld", ENVIRONMENT_CPP, 0x27a, 0);
            srAssertFail("pWorld", ENVIRONMENT_CPP, 0x288, 0);
        }
        Function483BA0(g_world, g_world->environment_intensity_024, &colour);
        {
            const int* direction = reinterpret_cast<const int*>( /* reinterpret-ok: published as raw words */
                &g_environment_colours_65ad98[phase]);

            g_light_direction_0065ad78 = direction[0];
            g_light_direction_0065ad7c = direction[1];
            g_light_direction_0065ad80 = direction[2];
            PublishLightDirection(direction);
        }
    }
}
