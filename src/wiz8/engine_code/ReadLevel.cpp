#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <windows.h>

#include "wiz8/engine_code/AnimObj.h"
#include "wiz8/engine_code/PathAI.h"
#include "wiz8/engine_code/ClipPlane.h"
#include "wiz8/engine_code/Environment.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/Level.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/engine_code/ReadLevel.h"
#include "wiz8/engine_code/Scene.h"
#include "wiz8/engine_code/materials.h"
#include "wiz8/engine_code/registry_classes.h"
#include "wiz8/engine_code/stModelInstance.h"
#include "wiz8/engine_code/stTextureAnim.h"
#include "wiz8/engine_code/Trigger.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/item_spawning.h"
#include "wiz8/sr_api.h"
#include "wiz8/virtual_file.h"
#include "surrender/srScene.h"
#include "wiz8/mesh_model.h"

#define READ_LEVEL_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\ReadLevel.cpp"

namespace {

struct W8LevelItemRecord004BC380 {
    int positional_00;
    srVector3T<float> position_04;
    int positional_10;
    int positional_14;
    int positional_18;
    char item_name_1c[20];
};

static_assert(sizeof(W8LevelItemRecord004BC380) == 0x30,
              "W8LevelItemRecord004BC380_size_must_be_0x30");

struct W8LevelLightRecord004BBAD0 {
    short version;
    unsigned char create;
    unsigned char visible;
    unsigned int flags;
    W8Position location;
    srVector3T<float> colour;
    float intensity;
    float range;
};

static_assert(sizeof(W8LevelLightRecord004BBAD0) == 0x28,
              "W8LevelLightRecord004BBAD0_size_must_be_0x28");

} // namespace

extern const float g_world_scale_005ebc40;
extern const float g_environment_distance_threshold_005ebcd0;
extern const float g_environment_near_scale_005ec0b0;
extern const float g_environment_far_scale_005ec3b8;
extern srVector3T<float> g_environment_offset_00659cd0;
extern float* RotateMatrixAroundAxis0042B910(
    float* matrix, double sine, double cosine, float* axis);
extern void WorldSetFarClip(W8World* world, float distance);
extern void WorldSetValue74(W8World* world, float value);
extern unsigned char ReadSingleLevelMesh00485B20(
    W8ReadLevelInfo* info, srModelInstance** instance,
    int positional_0, int positional_1, const char* name,
    unsigned char load_materials);
extern unsigned char ReadMultipleLevelMeshes00488240(
    W8ReadLevelInfo* info, srModelInstance** instances,
    unsigned long count, unsigned int flags);
extern void UpdateSky00482EA0(void);
extern void FinalizeWorldTriggers00448840(void);
extern void UpdateWorldProps0044E010(W8World* world);
extern void UpdateCameraView00450080(srCamera* camera, int mode);
extern void FinalizeWorldScenes0046F410(
    W8Scene005EBE48* static_scene, W8Node005EC208* dynamic_scene);
extern void* BuildWorldQuad004BE200(
    srModelInstance* instance, int positional,
    float minimum_x, float minimum_y, float minimum_z,
    float maximum_x, float maximum_y, float maximum_z,
    W8Scene005EBE48* scene, int flags);
extern void RefreshEnvironment00483560(void);
extern void FinalizeStaticScene0046F3A0(W8Scene005EBE48* scene);
extern unsigned char ReadWorldParticles004BD0D0(
    W8ReadLevelInfo* info, W8Node005EC208* scene,
    W8GrowableVector<stParticle*>* particles);
extern unsigned char ReadAutomapNodes00584DD0(int hFile);
extern void ReportLevelLoadError00401920(const char* message);
extern void SetChainValue15C(char* node, int value);

// FUNCTION: WIZ8 0x004BC060
void AssociateWorldLights004BC060(W8World* world)
{
    int light_index;

    for (light_index = 0;
         light_index < world->lights_to_update->GetCount();
         ++light_index) {
        stLight* light = *world->lights_to_update->GetAt(light_index);
        stLightDefinition* definition = light->m_definition_234;

        if (definition != 0 && definition->type_04 == 1 &&
            (static_cast<stLightDefinition005ECDBC*>(definition)->flags_08 &
             1) != 0) {
            int prop_count = PListGetCount(world->plsProps);
            int prop_index;

            for (prop_index = 0; prop_index < prop_count; ++prop_index) {
                W8Prop005EC1E0* prop = static_cast<W8Prop005EC1E0*>(
                    PListGetAt(world->plsProps, prop_index));

                if (prop->m_name_20 != 0 &&
                    _stricmp(prop->m_name_20, light->getName()) == 0) {
                    srModelInstance* instance =
                        prop->ToggleRepAnimationDefault();
                    light->m_prop_254 = prop;
                    GetModelAnimatedTexture004B9B50(instance)->flag_60 = 3;
                }
            }
        }
    }
}

// FUNCTION: WIZ8 0x004BBAD0
unsigned char ReadWorldLights004BBAD0(W8World* world, int hFile)
{
    short light_count;
    int index;
    unsigned char success;
    unsigned char path_success;

    success = ReadVirtualFile(hFile, &light_count, sizeof(light_count), 0);
    if (!success) {
        srAssertFail("fSuccess", READ_LEVEL_CPP, 486,
                     "Couldn't read number of lights");
    }

    for (index = 0; index < light_count; ++index) {
        W8LevelLightRecord004BBAD0 record;
        stLightDefinition005ECDBC* definition = 0;
        W8PathAI* path = 0;
        stLight* light = 0;
        char name[20];

        ReadVirtualFile(hFile, &record, sizeof(record), 0);
        if (record.version >= 2) {
            ReadVirtualFile(hFile, name, sizeof(name), 0);
            _strupr(name);

            if ((record.flags & 2) != 0) {
                definition = new stLightDefinition005ECDBC;
                record.create = 1;

                ReadVirtualFile(hFile, &definition->flags_08, 4, 0);
                ReadVirtualFile(hFile, &definition->value_0c, 4, 0);
                ReadVirtualFile(hFile, &definition->color_10.x, 4, 0);
                ReadVirtualFile(hFile, &definition->color_10.y, 4, 0);
                ReadVirtualFile(hFile, &definition->color_10.z, 4, 0);
                ReadVirtualFile(hFile, &definition->value_1c, 4, 0);
                ReadVirtualFile(hFile, &definition->value_20, 4, 0);
                ReadVirtualFile(hFile, &definition->value_24, 4, 0);
                ReadVirtualFile(hFile, &definition->intensity_28, 4, 0);
                ReadVirtualFile(hFile, &definition->value_2c, 4, 0);
                ReadVirtualFile(hFile, &definition->value_30, 4, 0);
                ReadVirtualFile(hFile, &definition->value_34, 4, 0);
                ReadVirtualFile(hFile, &definition->path_value_38, 4, 0);
                ReadVirtualFile(hFile, &definition->value_3c, 4, 0);
                ReadVirtualFile(hFile, &definition->value_40, 4, 0);

                if ((definition->flags_08 & 0x10) != 0) {
                    path_success = LoadPathAI004A92A0(&path, hFile);
                    if (!path_success) {
                        srAssertFail("fSuccess", READ_LEVEL_CPP, 532, 0);
                    }
                    path->flag_1c = 1;
                    path->flag_3a = 0;
                    path->value_2c = definition->path_value_38;
                }
            }
        }

        if (record.version >= 2 && record.create != 0) {
            if (definition == 0) {
                light = CreateWorldLight0046E140(world, name);
            }
            else {
                light = CreateWorldLight0046E030(world, name);
                light->m_definition_234 = definition;
                world->lights_to_update->Add(light);
                if (path != 0) {
                    light->m_owned_244 = path;
                }
                record.intensity = definition->intensity_28;
                if (definition->value_2c < definition->intensity_28) {
                    float swap = definition->intensity_28;
                    definition->intensity_28 = definition->value_2c;
                    definition->value_2c = swap;
                }
            }
            if (record.visible == 0) {
                light->setFlag(srNode::FLAG_POSITIONAL_0);
            }
            light->setGroupMask(2);
        }
        else if (record.version < 2 || record.visible != 0) {
            record.visible = 1;
            if (record.version < 2) {
                strcpy(name, "Static Point Light");
            }
            light = CreateLight0046DF90(world->dynamic_scene, name);
            if (light == 0) {
                srAssertFail("pstLight", READ_LEVEL_CPP, 593, 0);
            }
            light->setGroupMask(1);
        }
        else {
            delete definition;
        }

        if (light != 0) {
            if (_strnicmp(light->getName(), "Sun", 3) == 0) {
                light->m_color_6c.x = 0.0f;
                light->m_color_6c.y = 0.0f;
                light->m_color_6c.z = 0.0f;
                light->m_direction_60 = record.colour;
                light->setGroupMask(light->getGroupMask() | 4);
                AddEnvironmentLight00483F30(light);
            }
            else {
                light->m_color_6c = record.colour;
                light->m_direction_60.x = 0.0f;
                light->m_direction_60.y = 0.0f;
                light->m_direction_60.z = 0.0f;
            }

            light->m_position_78.x = 0.0f;
            light->m_position_78.y = 0.0f;
            light->m_position_78.z = 0.0f;
            ConfigureWorldLight0046E300(
                light, record.range * g_world_scale_005ebc40);
            light->m_positional_98 = record.intensity;
            light->setLocation(
                record.location.x * g_world_scale_005ebc40,
                record.location.y * g_world_scale_005ebc40,
                record.location.z * g_world_scale_005ebc40);
        }
    }

    if (light_count < 1) {
        return success;
    }
    return path_success;
}

// FUNCTION: WIZ8 0x004BC9D0
unsigned char ReadWorldEnvironment004BC9D0(
    W8ReadLevelInfo* pInfo, W8World* pWorld)
{
    EnvironmentColour environment_colour;
    EnvironmentColour white;
    W8Position position;
    srVector3T<float> axis;
    srMatrix3T<float> rotation;
    float intensity;
    float view_distance;
    float angle;
    float distance_scale;
    unsigned char camera_mode;
    unsigned char has_light_colours;
    unsigned char has_environment_colours;
    unsigned char fog_enabled;
    unsigned char success;

    success =
        ReadVirtualFile(pInfo->hFile, &fog_enabled, sizeof(fog_enabled), 0) &&
        ReadVirtualFile(pInfo->hFile, &environment_colour.red,
                        sizeof(environment_colour.red), 0) &&
        ReadVirtualFile(pInfo->hFile, &environment_colour.green,
                        sizeof(environment_colour.green), 0) &&
        ReadVirtualFile(pInfo->hFile, &environment_colour.blue,
                        sizeof(environment_colour.blue), 0) &&
        ReadVirtualFile(pInfo->hFile, &intensity, sizeof(intensity), 0) &&
        ReadVirtualFile(pInfo->hFile, &view_distance,
                        sizeof(view_distance), 0) &&
        ReadVirtualFile(pInfo->hFile, &camera_mode, sizeof(camera_mode), 0);

    if (camera_mode == 1) {
        success = success && ReadVirtualFile(
            pInfo->hFile, &position, sizeof(position), 0);
        position.x *= g_world_scale_005ebc40;
        position.y *= g_world_scale_005ebc40;
        position.z *= g_world_scale_005ebc40;
        SetWorldScenePosition004511D0(pWorld, &position);
    }
    else if (camera_mode == 2) {
        success = success &&
            ReadVirtualFile(pInfo->hFile, &position, sizeof(position), 0) &&
            ReadVirtualFile(pInfo->hFile, &angle, sizeof(angle), 0) &&
            ReadVirtualFile(pInfo->hFile, &axis.x, sizeof(axis.x), 0) &&
            ReadVirtualFile(pInfo->hFile, &axis.y, sizeof(axis.y), 0) &&
            ReadVirtualFile(pInfo->hFile, &axis.z, sizeof(axis.z), 0);
        position.x *= g_world_scale_005ebc40;
        position.y *= g_world_scale_005ebc40;
        position.z *= g_world_scale_005ebc40;
        SetWorldScenePosition004511D0(GetWorld(), &position);

        rotation.vectors[0].method_00421680(1.0f, 0.0f, 0.0f);
        rotation.vectors[1].method_00421680(0.0f, 1.0f, 0.0f);
        rotation.vectors[2].method_00421680(0.0f, 0.0f, 1.0f);
        if (angle != 0.0f) {
            RotateMatrixAroundAxis0042B910(
                &rotation.vectors[0].x, sin(angle), cos(angle), &axis.x);
        }
        Function421030(&rotation);
    }

    success = success && ReadVirtualFile(
        pInfo->hFile, &has_light_colours, sizeof(has_light_colours), 0);
    if (has_light_colours != 0) {
        ReadLightColourTable00482F90(pInfo->hFile);
    }
    else {
        BuildLightColourRamp00483360();
    }

    success = success && ReadVirtualFile(
        pInfo->hFile, &has_environment_colours,
        sizeof(has_environment_colours), 0);
    if (has_environment_colours != 0) {
        ReadEnvironmentColourTable004830D0(pInfo->hFile);
    }
    else {
        BuildEnvironmentColourRamp00483210();
    }

    g_environment_offset_00659cd0.x = 0.0f;
    g_environment_offset_00659cd0.y = 0.0f;
    g_environment_offset_00659cd0.z = 0.0f;
    pWorld->view_distance_020 = view_distance * g_world_scale_005ebc40;
    white.red = 1.0f;
    white.green = 1.0f;
    white.blue = 1.0f;
    SetWorldEnvironment00483BA0(pWorld, intensity, &white);
    WorldSetFarClip(pWorld, pWorld->view_distance_020);
    distance_scale = view_distance < g_environment_distance_threshold_005ebcd0
        ? g_environment_near_scale_005ec0b0
        : g_environment_far_scale_005ec3b8;
    WorldSetValue74(pWorld, distance_scale * pWorld->view_distance_020);
    pWorld->environment_range_start_014 = environment_colour.red;
    pWorld->environment_range_end_018 = environment_colour.green;
    pWorld->m_positional_01c = environment_colour.blue;

    if (fog_enabled == 0) {
        SetFogEnabled(0);
    }
    else {
        SetFogEnabled(1);
        UpdateEnvironmentLight004834B0();
    }
    return success;
}

// FUNCTION: WIZ8 0x004BCE20
unsigned char ReadWorldClipPlanes004BCE20(
    W8ReadLevelInfo* pInfo, W8World* pWorld)
{
    W8GrowableVector<W8ClipPlane005ED180*> clip_planes(5);
    srVector4T<float> plane;
    srVector4T<float> serialized_position;
    srVector3T<double> position;
    W8ClipPlane005ED180* clip_plane;
    char name[64];
    int count;
    int index;
    unsigned char version;
    unsigned char success;

    if (pInfo == 0) {
        srAssertFail("pInfo", READ_LEVEL_CPP, 0x59b, 0);
    }
    if (pInfo->hFile == 0) {
        srAssertFail("pInfo->hFile", READ_LEVEL_CPP, 0x59c, 0);
    }
    if (pWorld == 0) {
        srAssertFail("pWorld", READ_LEVEL_CPP, 0x59d, 0);
    }

    success = ReadVirtualFile(pInfo->hFile, &count, sizeof(count), 0);
    if (!success) {
        srAssertFail("fSuccess", READ_LEVEL_CPP, 0x5a2,
                     "Error reading num clipping planes");
        return 0;
    }
    if (count == 0) {
        return 1;
    }

    ReadVirtualFile(pInfo->hFile, &version, sizeof(version), 0);
    plane.x = 0.0f;
    plane.y = 1.0f;
    plane.z = 0.0f;
    plane.w = 0.0f;
    for (index = 0; index < count; ++index) {
        clip_plane = new W8ClipPlane005ED180(pWorld->dynamic_scene);
        if (clip_plane == 0) {
            srAssertFail("psrClipPlane", READ_LEVEL_CPP, 0x5ae,
                         "out of memory creating clip plane");
        }

        ReadVirtualFile(pInfo->hFile, name, sizeof(name), 0);
        ReadVirtualFile(pInfo->hFile, &serialized_position,
                        sizeof(serialized_position), 0);
        _strupr(name);
        clip_plane->setName(name);
        clip_plane->setClipPlane(plane);

        position.x = serialized_position.x * g_world_scale_005ebc40;
        position.y = serialized_position.y * g_world_scale_005ebc40;
        position.z = serialized_position.z * g_world_scale_005ebc40;
        clip_plane->setLocation(position);
        clip_plane->setFlag(srNode::FLAG_POSITIONAL_2);
        clip_plane->setClipType(srClipPlane::CLIP_POSITIONAL_0);
        clip_plane->setFlag(srNode::FLAG_POSITIONAL_0);
    }
    return 1;
}

struct W8PropBounds004BC5E0 {
    srVector3T<float> minimum;
    srVector3T<float> maximum;
};

static_assert(sizeof(W8PropBounds004BC5E0) == 0x18,
              "W8PropBounds004BC5E0_size_must_be_0x18");

// FUNCTION: WIZ8 0x004BC5E0
unsigned char ReadWorldProps004BC5E0(
    W8ReadLevelInfo* pInfo, W8World* pWorld,
    unsigned char mark_model_instances)
{
    /* CollectModelInstances appends. The canonical body deliberately keeps
       this one vector across the complete prop loop. */
    W8GrowableVector<stModelInstance005EC7D0*> model_instances(5);
    W8Prop005EC1E0* prop;
    W8PropBounds004BC5E0 bounds;
    int count;
    int index;
    int collidable_index;
    int model_index;
    unsigned char success;

    collidable_index = 0;
    if (pInfo == 0 || pInfo->hFile == 0 || pWorld == 0) {
        return 0;
    }
    success = ReadVirtualFile(pInfo->hFile, &count, sizeof(count), 0);
    if (!success || count >= 100000) {
        return 0;
    }
    if (count == 0) {
        return 1;
    }

    for (index = 0; index < count; ++index) {
        prop = 0;
        if (!success || !CreateAndLoadProp0044BF50(pInfo, &prop)) {
            success = 0;
        }
        else {
            success = 1;
            if (g_octree_6598a4 != 0) {
                if (!g_octree_6598a4->MarkVisited0042E400(index)) {
                    prop->flags_1c |= 0x40;
                }
                g_octree_6598a4->AddLoadedProp(prop);
            }
            PListAdd(pWorld->plsProps, prop);
            if ((prop->flags_1c & 1) != 0) {
                prop->GetBounds0044DD60(&bounds.minimum, &bounds.maximum);
                pWorld->collidable_props->Add(prop);
                if (pWorld->octree != 0) {
                    pWorld->octree->AddCollidablePropBounds0042EAB0(
                        collidable_index, &bounds.minimum);
                    ++collidable_index;
                }
            }
        }

        if (mark_model_instances && prop != 0) {
            prop->CollectModelInstances0044E570(&model_instances);
            for (model_index = 0;
                 model_index < model_instances.GetCount();
                 ++model_index) {
                stModelInstance005EC7D0* instance =
                    *model_instances.GetAt(model_index);
                if (instance != 0) {
                    instance->state_178 |= 0x10;
                }
            }
        }
    }
    if (g_octree_6598a4 != 0) {
        g_octree_6598a4->MarkVisited0042E400(-1);
    }
    return success;
}

// FUNCTION: WIZ8 0x004BC380
unsigned char ReadWorldItems004BC380(
    W8ReadLevelInfo* pInfo, W8World* pWorld)
{
    unsigned char has_trigger;
    int positional_value;
    int index;
    int count;
    W8LevelItemRecord004BC380 record;
    unsigned char positional_byte;
    unsigned char success;
    Trigger* trigger;
    int item_id;
    W8WorldItem* world_item;
    W8Item* item;

    if (pInfo == 0 || pInfo->hFile == 0 || pWorld == 0) {
        return 0;
    }
    success = ReadVirtualFile(pInfo->hFile, &count, sizeof(count), 0);
    if (!success || count >= 100000) {
        return 0;
    }
    if (count == 0) {
        return 1;
    }

    for (index = 0; index < count; ++index) {
        item = 0;
        trigger = 0;
        success = ReadVirtualFile(
            pInfo->hFile, record.item_name_1c,
            sizeof(record.item_name_1c), 0);
        if (success) {
            ReadVirtualFile(pInfo->hFile, &record.position_04,
                            sizeof(record.position_04), 0);
            record.position_04.x *= g_world_scale_005ebc40;
            record.position_04.y *= g_world_scale_005ebc40;
            record.position_04.z *= g_world_scale_005ebc40;
            ReadVirtualFile(
                pInfo->hFile, &record.positional_00, sizeof(int), 0);
            ReadVirtualFile(
                pInfo->hFile, &record.positional_10, sizeof(int), 0);
            ReadVirtualFile(
                pInfo->hFile, &record.positional_14, sizeof(int), 0);
            ReadVirtualFile(
                pInfo->hFile, &record.positional_18, sizeof(int), 0);
            ReadVirtualFile(
                pInfo->hFile, &has_trigger, sizeof(has_trigger), 0);
            if (has_trigger != 0) {
                trigger = Trigger::CreateAndLoadLevelTrigger(
                    pInfo->hFile, pInfo->world);
            }
            ReadVirtualFile(
                pInfo->hFile, &positional_byte, sizeof(positional_byte), 0);
            ReadVirtualFile(
                pInfo->hFile, &positional_byte, sizeof(positional_byte), 0);
            ReadVirtualFile(
                pInfo->hFile, &positional_byte, sizeof(positional_byte), 0);
            ReadVirtualFile(
                pInfo->hFile, &positional_value, sizeof(positional_value), 0);
            ReadVirtualFile(
                pInfo->hFile, &positional_value, sizeof(positional_value), 0);
            ReadVirtualFile(
                pInfo->hFile, &positional_value, sizeof(positional_value), 0);
            ReadVirtualFile(
                pInfo->hFile, &positional_value, sizeof(positional_value), 0);

            if (record.item_name_1c[0] >= '0' &&
                record.item_name_1c[0] <= '9') {
                item_id = atoi(record.item_name_1c);
            }
            else {
                item_id = FindItemRecordByName(record.item_name_1c);
            }
            if (item_id >= 0) {
                world_item = SpawnItem(item_id, &record.position_04, 3, 1);
                if (world_item != 0) {
                    success = 1;
                    ActivateItem(world_item);
                    item = world_item->owner;
                }
            }
            if (trigger != 0 && item != 0) {
                trigger->m_bRepType = 1;
                trigger->value_114 = item;
                item->trigger_018 = trigger;
            }
        }
    }
    return success;
}

// FUNCTION: WIZ8 0x004BC140
unsigned char ReadMonsterPaths004BC140(
    W8ReadLevelInfo* pInfo, W8World* pWorld)
{
    int count;
    int index;
    unsigned char success;
    unsigned char has_options;
    unsigned char update_representation;
    unsigned char active;
    char monster_name[20];
    char options[12];
    char* separator;
    W8Position origin;
    W8Position camera_position;
    W8MonsterGroup* group;
    int monster_id;
    int location_id;
    unsigned int monster_index;
    W8MonsterInfo* monster_info;
    W8Monster* monster;
    W8PathAI* path;

    /* Canonical 0x004BC140 initializes this once, not once per record. A
       colon-free record after an option-bearing one therefore reuses options. */
    has_options = 0;
    if (pInfo == 0 || pInfo->hFile == 0 || pWorld == 0) {
        return 0;
    }
    success = ReadVirtualFile(pInfo->hFile, &count, sizeof(count), 0);
    if (!success || count >= 100000) {
        return 0;
    }
    if (count == 0) {
        return 1;
    }

    origin.x = 0.0f;
    origin.y = 0.0f;
    origin.z = 0.0f;
    for (index = 0; index < count; ++index) {
        update_representation = 1;
        active = 1;
        success = success && ReadVirtualFile(
            pInfo->hFile, monster_name, sizeof(monster_name), 0);
        separator = strchr(monster_name, ':');
        if (separator != 0) {
            has_options = 1;
            strncpy(options, separator + 1, sizeof(options));
            *separator = '\0';
        }

        monster_id = atoi(monster_name);
        group = CreateGroup(monster_id, 1, &origin, 1, 0, 1);
        if (group == 0) {
            continue;
        }
        location_id = IListGetAt(group->monsters, 0);
        monster_index = MonsterGetIndexByLocationID(
            0x315, READ_LEVEL_CPP, location_id, 1);
        monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
        ActivateMonster(monster_info, 0);
        monster = monster_info->monster;

        {
            srVector3T<double> camera_location =
                pWorld->camera->getLocation();
            camera_position.x = static_cast<float>(camera_location.x);
            camera_position.y = static_cast<float>(camera_location.y);
            camera_position.z = static_cast<float>(camera_location.z);
        }
        monster->Function4A7BE0(&camera_position.x);

        if (LoadPathAI004A92A0(&path, pInfo->hFile)) {
            monster->SetPathAI(path);
        }
        /* The image also leaves the option-controlled path calls outside the
           successful-load branch. Preserve that behavior rather than adding
           a speculative null guard. */
        if (has_options) {
            if (options[0] == '0' || options[0] == '\0') {
                update_representation = 0;
            }
            if (options[1] == '0' || options[1] == '\0') {
                active = 0;
            }
            if (options[2] == '1') {
                PathAISetFlag3A004A9B90(path, 1);
            }
            if (options[3] == '1') {
                PathAISetFlag38004AA9D0(path, 1);
            }
            if (!active) {
                group->flag_28 = 0;
                monster->m_pRep->active = 0;
            }
            if (!update_representation) {
                continue;
            }
        }
        monster->UpdateRepresentation(pWorld);
    }
    return success;
}

// FUNCTION: WIZ8 0x004BC850
unsigned char ReadWorldCameras004BC850(
    W8ReadLevelInfo* pInfo, W8World* pWorld)
{
    int count;
    int index;
    int positional_0;
    int positional_1;
    unsigned char has_scale;
    float scale;
    unsigned char success;
    W8WorldCameraEntry* entry;

    if (pInfo == 0 || (pInfo->hFile == 0 | pWorld == 0)) {
        return 0;
    }
    success = ReadVirtualFile(pInfo->hFile, &count, sizeof(count), 0);
    if (!success || count >= 100000) {
        return 0;
    }
    if (count == 0) {
        return 1;
    }

    for (index = 0; index < count; ++index) {
        entry = static_cast<W8WorldCameraEntry*>(
            malloc(sizeof(W8WorldCameraEntry)));
        if (entry == 0) {
            return 0;
        }
        memset(entry, 0, sizeof(W8WorldCameraEntry));
        ReadVirtualFile(pInfo->hFile, &positional_0,
                        sizeof(positional_0), 0);
        ReadVirtualFile(pInfo->hFile, &positional_1,
                        sizeof(positional_1), 0);
        ReadVirtualFile(pInfo->hFile, &has_scale, sizeof(has_scale), 0);
        ReadVirtualFile(pInfo->hFile, entry->positional_00,
                        sizeof(entry->positional_00), 0);
        if (has_scale > 0) {
            ReadVirtualFile(pInfo->hFile, &scale, sizeof(scale), 0);
        }
        else {
            scale = 15.0f;
        }

        entry->path = 0;
        success = success && LoadPathAI004A92A0(&entry->path, pInfo->hFile);
        PathAIEnableTimedMode004A9BA0(entry->path);
        PListAdd(pWorld->plsCameras, entry);
        entry->path->value_10 = index;
        PathAISetScale004AA9C0(entry->path, scale);
    }
    return 1;
}

// FUNCTION: WIZ8 0x004BDC90
unsigned char ReadNamedPositions004BDC90(
    W8ReadLevelInfo* pInfo,
    W8GrowableVector<W8NamedPosition*>* named_positions)
{
    int hFile;
    int count;
    int index;
    unsigned char version;
    W8NamedPosition* pNamedPos;

    hFile = pInfo->hFile;
    ReadVirtualFile(hFile, &count, sizeof(count), 0);
    for (index = 0; index < count; ++index) {
        pNamedPos = new W8NamedPosition;
        if (pNamedPos == 0) {
            srAssertFail("pNamedPos", READ_LEVEL_CPP, 0x6d3,
                         "out of memory creating NamedPos");
        }

        ReadVirtualFile(hFile, &version, sizeof(version), 0);
        if (version != 1) {
            srAssertFail("bVersion == 1", READ_LEVEL_CPP, 0x6d6,
                         "Unknown Named Position version");
        }

        ReadVirtualFile(hFile, pNamedPos->name,
                        sizeof(pNamedPos->name), 0);
        ReadVirtualFile(hFile, &pNamedPos->position.x,
                        sizeof(pNamedPos->position.x), 0);
        ReadVirtualFile(hFile, &pNamedPos->position.y,
                        sizeof(pNamedPos->position.y), 0);
        ReadVirtualFile(hFile, &pNamedPos->position.z,
                        sizeof(pNamedPos->position.z), 0);
        pNamedPos->position.x *= 500.0;
        pNamedPos->position.y *= 500.0;
        pNamedPos->position.z *= 500.0;
        ReadVirtualFile(hFile, &pNamedPos->value_08c,
                        sizeof(pNamedPos->value_08c), 0);
        ReadVirtualFile(hFile, &pNamedPos->value_090,
                        sizeof(pNamedPos->value_090), 0);
        ReadVirtualFile(hFile, &pNamedPos->value_094,
                        sizeof(pNamedPos->value_094), 0);
        ReadVirtualFile(hFile, &pNamedPos->value_098,
                        sizeof(pNamedPos->value_098), 0);
        named_positions->Add(pNamedPos);
    }
    return 1;
}

#define CHECK_PVL_OFFSET(message)                                             \
    if (world->octree != 0 &&                                                \
        (!ReadVirtualFile(handle, &section_end, sizeof(section_end), 0) ||    \
         section_end != -1)) {                                               \
        sprintf(error_message, "%s\nTry deleting .PVL file and reloading.", \
                message);                                                    \
        ReportLevelLoadError00401920(error_message);                         \
    }

// FUNCTION: WIZ8 0x004BAFF0
unsigned char ReadLevel(
    W8World* world, int handle, unsigned char use_octree,
    const char* bitmap_folder)
{
    W8ReadLevelInfo info;
    srModelInstance* level_mesh;
    srVector3T<float> minimum;
    srVector3T<float> maximum;
    srVector3T<float> environment_offset;
    char error_message[512];
    int section_end;
    int section_count;
    int camera_mode;
    int index;
    unsigned int prop_count;
    unsigned char success;

    level_mesh = 0;
    if (world->update_mesh_source != 0) {
        world->update_mesh_source->release();
    }
    if (world->psrMeshes != 0) {
        while (world->psrMeshes[0] != 0) {
            world->psrMeshes[0]->release();
            world->psrMeshes[0] = 0;
        }
        if (world->octree == 0) {
            free(world->psrMeshes);
            world->psrMeshes = 0;
        }
    }

    if (world == 0 || world->static_scene == 0 || handle == 0) {
        return 0;
    }

    info.world = world;
    info.hFile = handle;
    info.bitmap_folder = bitmap_folder;
    GetTickCount();

    if (world->psrMeshes == 0 || world->octree == 0) {
        if (!ReadSingleLevelMesh00485B20(
                &info, &level_mesh, 0, 0, 0, 1)) {
            return 0;
        }
        if (level_mesh == 0) {
            srAssertFail("psrMesh", READ_LEVEL_CPP, 0xfa, 0);
        }
        level_mesh->setName("ReadLevel");
        world->update_mesh_source = level_mesh;
        level_mesh->setParent(world->level, 1);
        SetChainValue15C((char*)level_mesh, 1);
    }
    else {
        if (!ReadMultipleLevelMeshes00488240(
                &info, world->psrMeshes, world->octree->GetMeshCount(), 0)) {
            ReportLevelLoadError00401920(
                "ReadLevel: Error reading multi-meshes.");
        }
        for (unsigned int mesh_index = 0;
             mesh_index < world->octree->m_positional_1b4;
             ++mesh_index) {
            if (world->psrMeshes[mesh_index] != 0) {
                world->psrMeshes[mesh_index]->setParent(world->level, 1);
                SetChainValue15C((char*)world->psrMeshes[mesh_index], 1);
            }
        }
    }

    success = ReadWorldLights004BBAD0(world, handle);
    CHECK_PVL_OFFSET("Wrong offset in .pvl file after lights.");
    success = success && ReadMonsterPaths004BC140(&info, world);
    CHECK_PVL_OFFSET("Wrong offset in .pvl file after monsters.");
    success = success && ReadWorldItems004BC380(&info, world);
    CHECK_PVL_OFFSET("Wrong offset in .pvl file after items.");

    if (!success || info.hFile == 0 ||
        !ReadVirtualFile(info.hFile, &section_count,
                         sizeof(section_count), 0) ||
        section_count >= 100000) {
        success = 0;
    }
    CHECK_PVL_OFFSET("Wrong offset in .pvl file after missiles.");
    success = success && ReadWorldProps004BC5E0(&info, world, 0);
    CHECK_PVL_OFFSET("Wrong offset in .pvl file after props.");
    success = success && ReadWorldProps004BC5E0(&info, world, 1);
    CHECK_PVL_OFFSET("Wrong offset in .pvl file after bitmaps.");
    success = success && ReadWorldCameras004BC850(&info, world);
    CHECK_PVL_OFFSET("Wrong offset in .pvl file after cameras.");
    AssociateWorldLights004BC060(world);
    if (!success) {
        return 0;
    }

    ReadVirtualFile(info.hFile, &section_count, sizeof(section_count), 0);
    if (section_count == 0) {
        WorldSetFarClip(world, 42500.0f);
        WorldSetValue74(world, 37500.0f);
        if (!IsSkyEnabled()) {
            DisableSky();
        }
        else {
            UpdateSky00482EA0();
        }
    }
    else {
        success = ReadWorldEnvironment004BC9D0(&info, world);
    }
    CHECK_PVL_OFFSET("Wrong offset in .pvl file after fog options.");

    if (success && info.hFile != 0) {
        ReadVirtualFile(info.hFile, &section_count, sizeof(section_count), 0);
        if (section_count != 0) {
            for (index = 0; index < section_count; ++index) {
                Trigger::CreateAndLoadLevelTrigger(info.hFile, world);
            }
            if (world->m_owned_04c != 0 &&
                *(int*)world->m_owned_04c != 0) {
                FinalizeWorldTriggers00448840();
            }
        }
    }
    CHECK_PVL_OFFSET("Wrong offset in .pvl file after triggers.");

    UpdateWorldProps0044E010(world);
    ReadVirtualFile(info.hFile, &camera_mode, sizeof(camera_mode), 0);
    UpdateCameraView00450080(world->camera, camera_mode == 0 ? -1 : 1);
    if (world->octree == 0 && world != g_world) {
        FinalizeWorldScenes0046F410(world->static_scene, world->dynamic_scene);
    }

    success = ReadWorldClipPlanes004BCE20(&info, world);
    if (!success) {
        srAssertFail("fSuccess", READ_LEVEL_CPP, 0x15c, 0);
    }
    CHECK_PVL_OFFSET("Wrong offset in .pvl file after Clipping Planes.");

    if (use_octree != 0) {
        srMeshModel* model = static_cast<srMeshModel*>(level_mesh->model());
        model->getBoundingBox(minimum, maximum);
        world->m_owned_06c = BuildWorldQuad004BE200(
            level_mesh, 0,
            minimum.x, minimum.y, minimum.z,
            maximum.x, maximum.y, maximum.z,
            world->static_scene, 0);
    }
    RefreshEnvironment00483560();
    FinalizeStaticScene0046F3A0(world->static_scene);

    if (!success ||
        !ReadVirtualFile(handle, &environment_offset.x,
                         sizeof(environment_offset.x), 0) ||
        !ReadVirtualFile(handle, &environment_offset.y,
                         sizeof(environment_offset.y), 0) ||
        !ReadVirtualFile(handle, &environment_offset.z,
                         sizeof(environment_offset.z), 0)) {
        success = 0;
    }
    else {
        success = ReadWorldParticles004BD0D0(
            &info, world->dynamic_scene, world->particles);
    }
    CHECK_PVL_OFFSET("Wrong offset in .pvl file after particles.");
    success = success &&
        ReadNamedPositions004BDC90(&info, world->named_positions);
    CHECK_PVL_OFFSET(
        "Wrong offset in .pvl file after named position points.");
    success = success && ReadAutomapNodes00584DD0(handle);
    CHECK_PVL_OFFSET("Wrong offset in .pvl file after automap nodes.");

    g_environment_offset_00659cd0 = environment_offset;
    prop_count = PListGetCount(world->plsProps);
    for (index = 0; index < (int)prop_count; ++index) {
        W8Prop005EC1E0* prop = static_cast<W8Prop005EC1E0*>(
            PListGetAt(world->plsProps, index));
        W8AnimObj* animation = prop->m_owned_14->animation;

        if ((prop->flags_1c & 0x40) != 0) {
            unsigned int animation_count = AnimObjListCount004A1620(
                animation, 2);
            for (unsigned int animation_index = 0;
                 animation_index < animation_count;
                 ++animation_index) {
                srModelInstance* instance = prop->ToggleRepAnimation(
                    animation_index);
                stMeshModel* mesh = static_cast<stMeshModel*>(
                    instance->model());

                for (; mesh != 0; mesh = mesh->next) {
                    if (!AnimationIsRunning(animation)) {
                        mesh->InitializeVertexWeights004721E0(1);
                        SetChainValue15C((char*)instance, 5);
                    }
                    else if (AnimationIsRunning(animation) == 1) {
                        SetChainValue15C((char*)instance, 4);
                    }

                    srMaterialIFace* material_iface = mesh->getMaterial(
                        0, (srMeshModel::e_side)0);
                    if (material_iface != 0) {
                        srMaterial* material = static_cast<srMaterial*>(
                            material_iface);
                        srMaterial* copy = static_cast<srMaterial*>(material->clone());
                        copy->setName("Unsunlit Prop Material");
                        copy->autoRelease();
                        copy->parms_18.ambient.x = 0.0f;
                        copy->parms_18.ambient.y = 0.0f;
                        copy->parms_18.ambient.z = 0.0f;
                        copy->parms_18.ambient.w = 0.0f;
                        copy->dirty_74 = 1;
                        copy->parms_18.emissive.x +=
                            g_environment_offset_00659cd0.x;
                        copy->parms_18.emissive.y +=
                            g_environment_offset_00659cd0.y;
                        copy->parms_18.emissive.z +=
                            g_environment_offset_00659cd0.z;
                        copy->dirty_74 = 1;
                        mesh->setMaterial(
                            copy, 0, (srMeshModel::e_side)0);
                    }
                }
            }
        }
    }
    if (g_octree_6598a4 != 0) {
        g_octree_6598a4->m_fAccumulating = 0;
    }
    return success;
}

#undef CHECK_PVL_OFFSET
