#include "wiz8/level_specific_code/Trynnie1.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/engine_code/World.h"
#include "wiz8/fact_state.h"
#include "wiz8/location_variables.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/layouts/item_instance.h"
#include "wiz8/layouts/world.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_code/MonsterGenerator.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/monster_generators.h"
#include "wiz8/sr_api.h"
#include "wiz8/utility.h"
#include "wiz8/xstatus.h"
#include "wiz8/vector.h"
#include "soundman.h"

/* Level Specific Code\Trynnie1.cpp (level 0x19).

   Attribution evidence: 0x004DA670 and 0x004DA850 cite this file's path
   string in their assertions; 0x004DA670 is the case-0x19 setup that reads
   the 'Trynnies'/'Dead Trynnies' encounter tables into the Trynnie1Killed
   location variable. The level-0x1a neighbors belong to Trynnie2.cpp
   (0x004D9E60 cites it). */

#define TRYNNIE1_CPP "C:\\Projects\\Wizardry 8\\Level Specific Code\\Trynnie1.cpp"

/* Item ids 0x1b3 and 0x1c3 route the use-item action down the origin/equip
   path instead of activating, the same as a non-usable equip class. */
// FUNCTION: WIZ8 0x004DA0F0
bool IsSpecialItemId004DA0F0(W8ItemInstance* item)
{
    return item->iItemNo == 0x1b3 || item->iItemNo == 0x1c3;
}

/* Level 0x19 setup: once fact 0x229 reports the Trynnies dead and the
   Trynnie1Killed location variable does not exist yet, swap the encounter
   tables and spawn the dead shaman (0xcd) at VOC_SHAMAN_DYING. */
// FUNCTION: WIZ8 0x004DA670
void EnsureTrynnie1KilledVar004DA670(void)
{
    srVector3T<float> vPos;
    srVector3T<float> vPos2;
    W8MonsterGroup* group;

    if (GetFact(0x229) == 1) {
        if (GetLocationVarIDByName("Trynnie1Killed") == -1) {
            KillTrynnieGroups004DA850();
            if (FindEntityByName("VOC_SHAMAN_DYING", &vPos, 0, 0) == 0) {
                srAssertFail("WorldGetNamedPosition(\"VOC_SHAMAN_DYING\", vPos)", TRYNNIE1_CPP,
                             0x12, 0);
            }
            vPos2 = vPos;
            group = CreateGroup(0xcd, 1, &vPos2, 1, 0, 1);
            if (group == 0) {
                srAssertFail("CreateGroup(205, 1, vPos2, TRUE, FALSE, TRUE)", TRYNNIE1_CPP, 0x18,
                             0);
            }
            CreateLocationVar("Trynnie1Killed", 1);
        }
    }
}

/* "Fount_randomFX": the magic fountain rolls d100 - afflict a random party
   member with condition 9 (25%), restore the party's spell points (25%),
   heal everyone (25%) or restore everyone's stamina (25%). */
// FUNCTION: WIZ8 0x004DA740
bool Trynnie1FountRandomFX004DA740(Trigger* pTrigger)
{
    unsigned int roll;
    int slot;
    int i;

    roll = Random(100);
    if (roll < 25) {
        slot = GetRandomCharacter(0, 0, -1, -1);
        if (slot != -1) {
            SetCharacterCondition(slot, 9, Random(5) + 10, 0, 0, 1);
            SoundPlay("Data\\Sound\\misc\\fountain_magic.wav", 0);
            return 1;
        }
    } else if (roll < 50) {
        RestorePartySpellPoints(-1);
        SoundPlay("Data\\Sound\\misc\\fountain_magic.wav", 0);
        return 1;
    } else if (roll < 75) {
        for (i = 0; i < W8_PARTY_SLOT_COUNT; ++i) {
            if (g_status_685170.buffers.XChar[i].fOccupied != 0) {
                HealCharacter(i, 100, 0);
            }
        }
        SoundPlay("Data\\Sound\\misc\\fountain_magic.wav", 0);
        return 1;
    } else {
        for (i = 0; i < W8_PARTY_SLOT_COUNT; ++i) {
            if (g_status_685170.buffers.XChar[i].fOccupied != 0) {
                RestoreCharacterStamina(i, 100, 0);
            }
        }
    }
    SoundPlay("Data\\Sound\\misc\\fountain_magic.wav", 0);
    return 1;
}

/* Replace every live faction-0xd (Trynnie) group with a dead-Trynnie (0x1be)
   group, then point each 'Trynnies' generator at the 'Dead Trynnies' table. */
// FUNCTION: WIZ8 0x004DA850
void KillTrynnieGroups004DA850(void)
{
    W8MonsterGroup* group;
    W8MonsterRecord* record;
    MonGen* generator;
    int iOldTable;
    int iNewTable;
    int index;

    for (index = 0; index < static_cast<int>(PLLength(gXStatus.plsMonsterGroupList)); ++index) {
        group = GetMonsterGroupByListIndex(index);
        if (group != 0 && group->members_active != 0 &&
            (record = MonsterGroupGetRecord(group), record->faction_id_25f == 0xd) &&
            ReplaceMonsterGroupSpecies00511A40(group, 0x1be) != 0) {
            RemoveAllGroupMembers(group);
        }
    }
    iOldTable = FindEncounterTableByName("Trynnies");
    iNewTable = FindEncounterTableByName("Dead Trynnies");
    if (iOldTable == -1) {
        srAssertFail("iOldTable!=(-1)", TRYNNIE1_CPP, 0x68, "Can't find monster table 'Trynnies'");
    }
    if (iNewTable == -1) {
        srAssertFail("iNewTable!=(-1)", TRYNNIE1_CPP, 0x69,
                     "Can't find monster table 'Dead Trynnies'");
    }
    for (index = 0; index < g_world->monster_generators->GetCount(); ++index) {
        generator = *g_world->monster_generators->GetAt(index);
        if (generator->encounter_table_index == iOldTable) {
            generator->SetEncounterTable(iNewTable);
        }
    }
}
