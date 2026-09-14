#include "wiz8/local_code/Noise.h"

#include "wiz8/3d_code/PList.h"
#include "wiz8/engine_code/GrCycle.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/startup_world.h"
#include "wiz8/xstatus.h"

#include "random.h"

/* Retail Local Code\Noise.cpp. */

// FUNCTION: WIZ8 0x004F0E80
void AlertMonsterGroupsToNoise004F0E80(const srVector3T<float>* position, int radius, int flag)
{
    if (g_status_685170.value_2390 != 0) {
        return;
    }
    srVector3T<float> noise_position = *position;
    for (unsigned int group_index = 0; group_index < PLLength(gXStatus.plsMonsterGroupList);
         ++group_index) {
        W8MonsterGroup* group = GetMonsterGroupByListIndex(group_index);
        W8MonsterInfo* info = MonsterInfoFromID(
            0x2e, "C:\\Projects\\Wizardry 8\\Local Code\\Noise.cpp", group->value_9f, 1);
        if (info->monster->deaf_28f != 0) {
            continue;
        }
        if (flag == 1) {
            if (gXStatus.fCombatMode != 0 && group->fInCombat != 0) {
                continue;
            }
        } else if (flag == 0 && info->ubDisposition != 1) {
            continue;
        }
        W8MonsterRecord* record = GetMonsterDataForInfo(info);
        srVector3T<float> monster_position = info->monster->GetPosition();
        int remaining = radius - (int)(monster_position - noise_position).Length();
        if (remaining <= 0) {
            continue;
        }
        if ((float)Random(100) >=
            (float)remaining * g_float_005ec128 + (float)record->attribute_values_d1[4]) {
            continue;
        }
        if (group->leader_group_id != 0) {
            unsigned int leader_index = GetMonsterGroupIndexByID(
                0x54, "C:\\Projects\\Wizardry 8\\Local Code\\Noise.cpp", group->leader_group_id, 1);
            W8MonsterGroup* leader = GetMonsterGroupByListIndex(leader_index);
            info = MonsterInfoFromID(0x55, "C:\\Projects\\Wizardry 8\\Local Code\\Noise.cpp",
                                     leader->value_9f, 1);
            if (info->monster->deaf_28f != 0) {
                continue;
            }
        }
        if (flag == 1 && gXStatus.fCombatMode != 0) {
            float range = (float)radius;
            int hops;
            if (g_octree_6598a4->TestNoiseLineOfSight00434220(&monster_position, &noise_position,
                                                              &range, &hops) == 0) {
                continue;
            }
            if ((float)((int)GetMonsterRecordScaledFloat1BA(info) * 1000) < range) {
                continue;
            }
        }
        if (remaining <= info->sp_budget_bonus) {
            continue;
        }
        if (flag == 1) {
            NotifyMonsterGroupActivity(group);
        }
        info->heard_noise_radius_43 = radius;
        info->sp_budget_bonus = remaining;
        info->heard_noise_position_37 = noise_position;
    }
}

// FUNCTION: WIZ8 0x004F1100
void AlertWorldNoise004F1100(void)
{
    srVector3T<float> position = g_startup_world_659c0c->GetPosition();
    AlertMonsterGroupsToNoise004F0E80(&position, 50000, 1);
}

// FUNCTION: WIZ8 0x004F1150
void AlertCombatNoise004F1150(char large_radius)
{
    if (g_status_685170.value_2390 != 0) {
        return;
    }
    srVector3T<float> position = g_startup_world_659c0c->GetPosition();
    AlertMonsterGroupsToNoise004F0E80(&position, large_radius != 0 ? 0x927c : 25000, 0);
}
