#include "wiz8/engine_code/Levels.h"
#include "wiz8/engine_code/Object0043A910.h"
#include "wiz8/xstatus.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/combat_state.h"
#include "wiz8/monster_runtime.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/sr_api.h"
#include "wiz8/utility.h"

#include <stdlib.h>

/*
 * Local Code\Sight.cpp.
 *
 * Who can see what. A monster carries one visibility record per other monster
 * it has an opinion about, and the two release paths below own that list. The
 * sweeps re-run the sight update over every live monster, once for each
 * direction the update takes.
 */

#define SIGHT_CPP "C:\\Projects\\Wizardry 8\\Local Code\\Sight.cpp"

/* One monster-to-monster visibility record. Only the subject at the front is
   established - it is what the release path matches on - and the record is
   plain malloc'd memory that free releases. */
typedef struct W8MonToMonVisibility {
    int about_location_id;               /* 0x00 */
} W8MonToMonVisibility;

/* The two lighting conditions the per-condition visibility table is indexed
   by. Their meaning is not established; only that each shifts the lookup one
   entry along. */
typedef struct W8SightConditions {
    unsigned char unknown_000[0x37a];
    unsigned char condition_37a;         /* 0x37a */
    unsigned char unknown_37b;
    unsigned char condition_37c;         /* 0x37c */
} W8SightConditions;

/* One row of per-visibility-kind flags. The two conditions above select
   between adjacent entries in the two pairs. */
typedef struct W8VisibilityRow {
    unsigned char unknown_00[4];
    unsigned char visible_04;            /* 0x04 */
    unsigned char visible_05;            /* 0x05 */
    unsigned char unknown_06;
    unsigned char visible_07;            /* 0x07 */
    unsigned char unknown_08[3];
    unsigned char visible_0b;            /* 0x0b */
    unsigned char unknown_0c[0x1c];
    unsigned char visible_28;            /* 0x28 */
} W8VisibilityRow;

extern void UpdateMonsterSight(W8MonsterInfo* monster_info, int direction, int arg_3);
/* 0x005049C0 */
extern void Function593330(void);
extern void Function53BF80(void);
extern void SetViewDistance(float value);
extern float GetViewDistance(void);
extern void Function452F50(int value);
extern void Function48CBE0(void);
extern float g_sight_default_005ec254;

/* Put the sight subsystem back to its starting state. */
// FUNCTION: WIZ8 0x005048e0
void ResetSight(void)
{
    SetViewDistance(12.0f);
    Function452F50(0);
    g_object_6598bc->ResetDurationScale();
    Function48CBE0();
}

/* Whether the view distance has been moved off its default. */
// FUNCTION: WIZ8 0x00504910
bool IsSightRangeOverridden(void)
{
    return GetViewDistance() != g_sight_default_005ec254;
}

/* Re-run both directions of the sight update for one monster. */
// FUNCTION: WIZ8 0x00505760
void RefreshMonsterSight(W8MonsterInfo* monster_info)
{
    UpdateMonsterSight(monster_info, 1, 0);
    UpdateMonsterSight(monster_info, 0, 0);
}

/* Re-run the outward direction over every live monster, and tell the combat
   display about it if a fight is on. The count is re-read every step because
   the update can remove a monster. */
// FUNCTION: WIZ8 0x00505780
void RefreshOutwardSightForAllMonsters(void)
{
    unsigned int index;

    for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
        UpdateMonsterSight(MonsterGetScriptPartByLocationIndex(index), 1, 0);
    }
    if (gXStatus.fCombatMode != 0) {
        Function593330();
        Function53BF80();
    }
}

/* The inward direction on its own, with nothing to tell the display. */
// FUNCTION: WIZ8 0x005057d0
void RefreshInwardSightForAllMonsters(void)
{
    unsigned int index;

    for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
        UpdateMonsterSight(MonsterGetScriptPartByLocationIndex(index), 0, 0);
    }
}

/* Both directions over every monster, in two separate passes rather than one
   pass doing both - the outward pass and its display notification have to
   finish before the inward pass starts. */
// FUNCTION: WIZ8 0x00505810
void RefreshAllSight(void)
{
    unsigned int index;

    for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
        UpdateMonsterSight(MonsterGetScriptPartByLocationIndex(index), 1, 0);
    }
    if (gXStatus.fCombatMode != 0) {
        Function593330();
        Function53BF80();
    }
    for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
        UpdateMonsterSight(MonsterGetScriptPartByLocationIndex(index), 0, 0);
    }
}

/* Drop every visibility record anyone held about one departing monster. */
// FUNCTION: WIZ8 0x00505ba0
void ReleaseMonToMonVisibilityInfoAbout(int location_id)
{
    unsigned int monster_index;
    int index;
    W8MonsterInfo* monster_info;
    W8MonToMonVisibility* visibility;

    for (monster_index = 0; monster_index < PLLength(gXStatus.plsMonsterList);
         ++monster_index) {
        monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
        if (monster_info->flag_14 == 0) {
            continue;
        }
        for (index = 0;
             index < (int)PLLength(monster_info->mon_to_mon_visibility);
             ++index) {
            visibility =
                (W8MonToMonVisibility*)PLGet(monster_info->mon_to_mon_visibility, index);
            if (visibility == 0) {
                return;
            }
            if (visibility->about_location_id == location_id) {
                visibility = (W8MonToMonVisibility*)PLRemoveAt(
                    monster_info->mon_to_mon_visibility, index);
                if (visibility == 0) {
                    srAssertFail(
                        "pVisibility != NULL", SIGHT_CPP, 968,
                        FormatString(
                            "ReleaseMonToMonVisibilityInfoAbout: ERROR - PLRemoveAt failed, index %d",
                            index));
                }
                free(visibility);
                break;
            }
        }
    }
}

/* Empty and destroy one monster's whole visibility list, then drop what
   everybody else held about it. */
// FUNCTION: WIZ8 0x00505c80
void ReleaseMonToMonVisibilityList(W8MonsterInfo* monster_info)
{
    W8MonToMonVisibility* visibility;

    while ((int)PLLength(monster_info->mon_to_mon_visibility) > 0) {
        visibility =
            (W8MonToMonVisibility*)PLRemoveAt(monster_info->mon_to_mon_visibility, 0);
        if (visibility == 0) {
            srAssertFail(
                "pVisibility != NULL", SIGHT_CPP, 990,
                FormatString("ReleaseMonToMonVisibilityList: ERROR - PLRemoveAt failed"));
        }
        free(visibility);
    }
    if (PLDestroy(monster_info->mon_to_mon_visibility)) {
        monster_info->mon_to_mon_visibility = 0;
        ReleaseMonToMonVisibilityInfoAbout(monster_info->location_id);
    }
}

/* Whether a monster group still has anybody in it worth seeing: alive, not
   dying, not too far gone, and marked as a threat. */
// FUNCTION: WIZ8 0x00505ea0
bool MonsterGroupHasVisibleThreat(W8MonsterGroup* group)
{
    unsigned int index;
    W8MonsterInfo* monster_info;

    for (index = 0; index < ILLength(group->monsters); ++index) {
        monster_info = MonsterInfoFromID(1120, SIGHT_CPP, IListGetAt(group->monsters, index), 1);
        if (monster_info->flag_14 != 0 && !monster_info->monster->IsDying() &&
            monster_info->hp_current != 0 && (unsigned int)monster_info->value_107 < 0xc &&
            monster_info->threat_28a == 1) {
            return true;
        }
    }
    return false;
}

/* Which of the two adjacent visibility entries the first lighting condition
   selects. */
// FUNCTION: WIZ8 0x00505e60
bool GetSightCondition37A(const W8SightConditions* conditions)
{
    return conditions->condition_37a != 0;
}

/* The second condition, answered as the entry index it picks rather than as a
   flag - two or three. */
// FUNCTION: WIZ8 0x00505e80
char GetSightCondition37CIndex(const W8SightConditions* conditions)
{
    return (conditions->condition_37c != 0) + 2;
}

/* Read one flag out of a visibility row. Two of the seven kinds are pairs that
   the lighting conditions choose between; the rest are fixed, and anything
   past the named kinds falls back to the last flag. */
// FUNCTION: WIZ8 0x00505dd0
bool IsVisibleUnderConditions(
    const W8SightConditions* conditions, const W8VisibilityRow* row, int kind)
{
    if (row == 0) {
        return false;
    }
    switch (kind) {
    case 0:
        return row->visible_04 != 0;
    case 1:
        return row->visible_0b != 0;
    case 2:
        return row->visible_05 != 0;
    case 3:
        return row->visible_07 != 0;
    case 4:
        return (&row->visible_05)[conditions->condition_37a != 0] != 0;
    case 5:
        return (&row->visible_07)[conditions->condition_37c != 0] != 0;
    default:
        return row->visible_28 != 0;
    }
}

extern void AgeMonsterSight(W8MonsterInfo* monster_info, unsigned int minutes, int arg_3);
/* 0x00503990 */

/* Bring every monster's sight up to date with the clock, in whole two-minute
   steps - anything short of one step is left for next time. */
// FUNCTION: WIZ8 0x00504930
unsigned int AgeAllMonsterSight(void)
{
    unsigned int elapsed;
    unsigned int steps;
    unsigned int index;
    W8MonsterInfo* monster_info;

    elapsed = g_status_685170.world_clock * 1000 -
              g_status_685170.level_progress[g_status_685170.current_level].sight_clock * 1000;
    steps = elapsed / 120000;
    if (steps == 0) {
        return 0;
    }
    for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
        monster_info = MonsterGetScriptPartByLocationIndex(index);
        if ((unsigned int)monster_info->value_107 < 0x12) {
            AgeMonsterSight(monster_info, steps, 1);
        }
    }
    return PLLength(gXStatus.plsMonsterList);
}

/* What one monster has recorded about another, if anything. Both monsters have
   to be in the world - the two assertions name that field fActive - and a null
   entry in the list is itself an error rather than an end marker. */
// FUNCTION: WIZ8 0x00505d20
W8MonToMonVisibility* FindMonToMonVisibility(
    W8MonsterInfo* source, int unused, W8MonsterInfo* target)
{
    int index;
    W8MonToMonVisibility* visibility;

    if (source->flag_14 == 0) {
        srAssertFail("pSourceMonsterInfo->fActive", SIGHT_CPP, 1020, 0);
    }
    if (target->flag_14 == 0) {
        srAssertFail("pTargetMonsterInfo->fActive", SIGHT_CPP, 1021, 0);
    }

    for (index = 0; index < (int)PLLength(source->mon_to_mon_visibility); ++index) {
        visibility =
            (W8MonToMonVisibility*)PLGet(source->mon_to_mon_visibility, index);
        if (visibility == 0) {
            srAssertFail("FALSE", SIGHT_CPP, 1030, 0);
        }
        else if (visibility->about_location_id == target->location_id) {
            return visibility;
        }
    }
    return 0;
}
