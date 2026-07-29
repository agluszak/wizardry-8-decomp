#include "wiz8/gameplay_boundaries.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/sr_api.h"

#include <stdlib.h>
#include <string.h>

/*
 * Local Code\MonsterAI.cpp.
 *
 * What a monster decides to do. Each monster's combat state owns a queue of
 * decided actions; the bodies here build entries for it, run the per-monster
 * decision over every live monster, and answer the questions the decision
 * itself asks about targets.
 */

#define MONSTER_AI_CPP "C:\\Projects\\Wizardry 8\\Local Code\\MonsterAI.cpp"

/* The AI table row value that marks a monster the flee and control rules treat
   differently. */
enum { W8_AI_KIND_ROW_SPECIAL = 6 };

/* The spell the AI casts when it wants a place rather than a target. */
enum { W8_AI_SPELL_PLACE = 0x77 };

/* The monster action kinds the AI validates. */
enum { W8_MONSTER_ACTION_ATTACK = 0, W8_MONSTER_ACTION_SPELL = 2, W8_MONSTER_ACTION_FLEE = 3 };

extern void UpdateMonsterAI(W8MonsterInfo* monster_info);                /* 0x00531540 */
extern unsigned char AimMonsterAtSpellTarget(W8MonsterInfo* monster_info, int spell_id);
/* 0x005326F0 */
extern unsigned char Function53C630(W8CombatSlot* slot, int arg_2);
extern unsigned char Function5474B0(int spell_id);
extern unsigned char Function5353E0(W8MonsterInfo* monster_info, int spell_id, W8CombatSlot* slot);
extern short Function4526C0(void* position, int arg_2, double radius);
extern void Function453F30(const W8Position* position);
extern float Function4C7CB0(W8MonsterInfo* monster_info);
struct W8SpellEffectEntry;
extern struct W8SpellEffectEntry* FindMonsterControlSpellEffect(void);   /* 0x00500F30 */
extern void ResetCombatSlot(W8CombatSlot* slot);                        /* 0x00536170 */
extern void GetPartyPosition(W8Position* position);                     /* 0x00421070 */
/* 0x0061EEFC: two bytes per AI kind; only the leading byte is read. */
extern const unsigned char g_ai_kind_table[][2];

/* Throw away the queue of actions a monster's AI had decided on. */
// FUNCTION: WIZ8 0x00532330
void DestroyMonsterActionQueue(W8MonsterInfo* monster_info)
{
    W8PList* queue = monster_info->pCombat->pending_actions;

    if (queue != 0 && PListDestroy(queue)) {
        monster_info->pCombat->pending_actions = 0;
    }
}

/* Run the decision over every monster that is in the fight and still alive. */
// FUNCTION: WIZ8 0x005314f0
void UpdateAllMonsterAI(void)
{
    unsigned int index;
    W8MonsterInfo* monster_info;

    for (index = 0; index < PListGetCount(g_active_monster_list_00683fad); ++index) {
        monster_info = MonsterGetScriptPartByLocationIndex(index);
        if (monster_info->fInCombat != 0 && monster_info->hp_current != 0) {
            UpdateMonsterAI(monster_info);
        }
    }
}

/* Add one decided action to a monster's queue. The third field only carries a
   value for the plain attack, and which of the two target fields the target
   goes in depends on what kind of target it is. Each entry gets a random tie
   break so two equal decisions do not always resolve the same way. */
// FUNCTION: WIZ8 0x00532360
void QueueMonsterAction(
    W8MonsterInfo* monster_info,
    int action_kind,
    int action_detail,
    int attack_index,
    int target_kind,
    int target_value)
{
    int* entry = (int*)malloc(0x30);

    if (entry == 0) {
        return;
    }
    memset(entry, 0, 0x30);
    entry[0] = action_kind;
    entry[1] = action_detail;
    if (action_kind == W8_MONSTER_ACTION_ATTACK) {
        entry[2] = attack_index;
    }
    ResetCombatSlot((W8CombatSlot*)(entry + 3));
    entry[3] = target_kind;
    if (target_kind == 1) {
        entry[4] = target_value;
    }
    else if (target_kind == 3) {
        entry[5] = target_value;
    }
    *(char*)(entry + 0xb) = (char)Random(100) + 1;
    PListAdd(monster_info->pCombat->pending_actions, entry);
}

/* Whether a monster can aim the spell it wants to cast. The two area target
   types aim at the world; anything else either needs no aim at all or has to
   pass the slot check. */
// FUNCTION: WIZ8 0x00534290
unsigned char CanMonsterAimSpell(W8MonsterInfo* monster_info, int spell_id)
{
    int target_type = GetSpellTargetType(spell_id, 0);

    if (target_type > 4 && target_type < 7) {
        return AimMonsterAtSpellTarget(monster_info, spell_id);
    }
    if (g_spell_records[spell_id].needs_aim_13f != 0) {
        return Function53C630(&monster_info->combat_slot_2ba, 0);
    }
    return 1;
}

/* Aim a monster that wants to get away. A monster of the singled-out AI kind
   aims at where the party is standing instead of at anybody in it. */
// FUNCTION: WIZ8 0x00534cb0
unsigned char AimFleeingMonster(W8MonsterInfo* monster_info, const W8MonsterRecord* record)
{
    W8Position party;

    if (g_ai_kind_table[record->ai_kind][0] == W8_AI_KIND_ROW_SPECIAL) {
        GetPartyPosition(&party);
        ResetCombatSlot(&monster_info->combat_slot_2ba);
        monster_info->combat_slot_2ba.iType = 6;
        monster_info->combat_slot_2ba.point = party;
        return 1;
    }
    return AimMonsterAtSpellTarget(monster_info, W8_AI_SPELL_PLACE) != 0;
}

/* Whether the action a monster has settled on can actually be carried out. An
   attack needs a character to swing at; a spell needs to be castable and needs
   a target of a kind it accepts; fleeing is refused outright to the AI kind
   that has nowhere to flee to. */
// FUNCTION: WIZ8 0x00535150
unsigned char IsMonsterActionUsable(W8MonsterInfo* monster_info)
{
    int spell_id;

    switch (monster_info->action_kind) {
    case W8_MONSTER_ACTION_ATTACK:
        return monster_info->combat_slot_2ba.iType == 1;
    case W8_MONSTER_ACTION_SPELL:
        spell_id = monster_info->action_detail;
        if (!Function5474B0(spell_id)) {
            return 0;
        }
        switch (monster_info->combat_slot_2ba.iType) {
        case 1:
        case 2:
            return 1;
        case 5:
        case 6:
            break;
        default:
            return 0;
        }
        break;
    case W8_MONSTER_ACTION_FLEE:
        if (g_ai_kind_table[GetMonsterDataForInfo(monster_info)->ai_kind][0] ==
            W8_AI_KIND_ROW_SPECIAL) {
            return 0;
        }
        spell_id = W8_AI_SPELL_PLACE;
        break;
    default:
        return 0;
    }
    return Function5353E0(monster_info, spell_id, &monster_info->combat_slot_2ba) != 0;
}

/* How near the nearest member of a group has come. */
// FUNCTION: WIZ8 0x005324b0
float GetGroupNearestDistance(W8MonsterGroup* group, float furthest)
{
    unsigned int index;
    int location_id;
    W8MonsterInfo* monster_info;
    float distance;

    if (group == 0) {
        srAssertFail("pMonsterGroup", MONSTER_AI_CPP, 1840, 0);
    }

    for (index = 0; index < PListGetCount((W8PList*)group->monsters); ++index) {
        location_id = IListGetAt(group->monsters, index);
        monster_info = MonsterGetScriptPartByLocationIndex(
            MonsterGetIndexByLocationID(1845, MONSTER_AI_CPP, location_id, 1));
        distance = Function4C7CB0(monster_info);
        if (distance < furthest) {
            furthest = distance;
        }
    }
    return furthest;
}

/* Whether the point the monster-control effect is anchored to is still within
   reach. With no effect running, or nothing anchored, there is nothing to be
   in range of; failing the test falls back on where the party is standing. */
// FUNCTION: WIZ8 0x00534d50
short IsMonsterControlPointInRange(void)
{
    W8SpellEffectEntry* effect = FindMonsterControlSpellEffect();
    void** anchor;
    short in_range;
    W8Position party;

    if (effect == 0) {
        return 0;
    }
    anchor = *(void***)((char*)effect + 0x10c);
    if (*anchor == 0) {
        return 0;
    }
    in_range = Function4526C0((char*)*anchor + 0x18, 0, 2500.0);
    if (in_range == 0) {
        GetPartyPosition(&party);
        Function453F30(&party);
    }
    return in_range;
}
