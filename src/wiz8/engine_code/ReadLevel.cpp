#include <cstdlib>
#include <cstring>

#include "wiz8/engine_code/PathAI.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/engine_code/ReadLevel.h"
#include "wiz8/engine_code/stModelInstance.h"
#include "wiz8/engine_code/Trigger.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/item_spawning.h"
#include "wiz8/sr_api.h"
#include "wiz8/virtual_file.h"
#include "surrender/srScene.h"

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

} // namespace

extern const float g_world_scale_005ebc40;

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
