#include "wiz8/gameplay_boundaries.h"
#include "wiz8/engine_code/Fog.h"
#include "wiz8/engine_code/Scene.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/registry_classes.h"
#include "wiz8/wiz8_windows.h"
#include "wiz8/sr_api.h"
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
extern int g_dword_6874f7;
extern unsigned long g_tick_65b9a8;
/* 0x00659AB4: the world being rendered. Its sky node is the one field these
   two bodies reach, and it is the same W8World the 3d code walks. */

/* The static and dynamic scene fogs owned by the environment. */
extern W8Fog005EC94C* g_environment_object_0065b9b0;
extern W8Fog005EC94C* g_environment_object_0065b9b4;
extern int g_environment_count_0065b99c;

extern void PublishLightDirection(const int* direction);                 /* 0x00427380 */

// SYNTHETIC: WIZ8 0x00484840
// W8Fog005EC94C::`scalar deleting destructor'

// FUNCTION: WIZ8 0x00484700
unsigned long W8Fog005EC94C::getClassID() const
{
    return 0x1210;
}

// FUNCTION: WIZ8 0x00484710
const char* W8Fog005EC94C::getClassName() const
{
    return "srFog";
}

// FUNCTION: WIZ8 0x00484720
srRegistry::ClassNode* W8Fog005EC94C::getClassNode() const
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x1210);

    if (node == 0) {
        srRegistry* parent_registry = srCore.getRegistry();
        srRegistry::ClassNode* parent = parent_registry->getClassNode(0x1200);

        if (parent == 0) {
            srRegistry* node_registry = srCore.getRegistry();
            srRegistry::ClassNode* node_parent =
                node_registry->getClassNode(0x1000);

            if (node_parent == 0) {
                node_parent = node_registry->registerClass(
                    srNode::sGetClassName(),
                    srClass::sGetClassNode(),
                    0x1000,
                    1);
            }
            parent = parent_registry->registerClass(
                srIlluminator::sGetClassName(), node_parent, 0x1200, 0);
        }
        node = registry->registerClass("srFog", parent, 0x1210, 0);
    }
    return node;
}

// FUNCTION: WIZ8 0x004847C0
srNode* W8Fog005EC94C::clone()
{
    srFog* copy = static_cast<srFog*>(vInstance());
    *copy = *this;
    return copy;
}

// FUNCTION: WIZ8 0x00483750
void SetSkyEnabled(unsigned char enabled)
{
    if (enabled != 0) {
        if (g_world == 0 || g_world->dynamic_scene == 0 ||
            g_environment_object_0065b9b0 != 0) {
            return;
        }

        g_environment_object_0065b9b0 =
            new W8Fog005EC94C(g_world->static_scene);
        g_environment_object_0065b9b4 =
            new W8Fog005EC94C(g_world->dynamic_scene);
        g_environment_object_0065b9b0->m_positional_28 = 1.0f;
        g_environment_object_0065b9b4->m_positional_28 = 1.0f;

        if (g_environment_object_0065b9b0 != 0 && g_world != 0) {
            g_environment_object_0065b9b0->m_positional_double_20 =
                WorldGetFarClip(g_world) * g_world->m_positional_018;
            g_environment_object_0065b9b0->m_positional_double_18 =
                WorldGetFarClip(g_world) * g_world->m_positional_014;
            g_environment_object_0065b9b4->m_positional_double_18 =
                WorldGetFarClip(g_world) * g_world->m_positional_014;
            g_environment_object_0065b9b4->m_positional_double_20 =
                WorldGetFarClip(g_world) * g_world->m_positional_018;
        }

        PublishLightDirection(&g_light_direction_0065ad78);
        if (g_world == 0 || g_world->camera == 0) {
            return;
        }
        g_world->camera->setEnvironmentRange(
            (float)WorldGetFarClip(g_world) * g_world->m_positional_014,
            (float)WorldGetFarClip(g_world) * g_world->m_positional_018);
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
    g_environment_count_0065b99c = 0;
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

/* Write one field of the sky node, if the sky has one. */
// FUNCTION: WIZ8 0x00483e30
void SetSkyNodeValue1D0(int value)
{
    unsigned char* sky = (unsigned char*)g_world->sky_node;

    if (sky != 0) {
        *(int*)(sky + 0x1d0) = value;
    }
}

/* Show or hide the sky node, which is the renderer's flag zero the other way
   round: showing it clears the flag. */
// FUNCTION: WIZ8 0x00483e50
void SetSkyNodeVisible(char visible)
{
    srNode* sky = (srNode*)g_world->sky_node;

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
    g_dword_6874f7 = value;
    g_tick_65b9a8 = GetTickCount();
}
}
