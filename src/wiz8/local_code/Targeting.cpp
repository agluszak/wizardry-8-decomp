#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"

#include <string.h>

#define TARGETING_CPP "C:\\Projects\\Wizardry 8\\Local Code\\Targeting.cpp"

/* BAD_INDEX is -1: the canonical assertions read "!= BAD_INDEX" where the
   bodies compare against -1. */
#define BAD_INDEX (-1)

// FUNCTION: WIZ8 0x0053BEA0
unsigned char TargetSourceIsCharacter(const W8TargetSource* source, int allow_indirect)
{
    if (source->iType == 1) {
        if (source->iChar == BAD_INDEX) {
            srAssertFail("pSource->iChar != BAD_INDEX", TARGETING_CPP, 0xce3, 0);
        }
        return 1;
    }
    if (allow_indirect == 1 && source->iType == 3 && source->iChar != BAD_INDEX) {
        if (!source->fBackfire && !source->fReflection) {
            srAssertFail("pSource->fBackfire || pSource->fReflection", TARGETING_CPP, 0xceb, 0);
        }
        return 1;
    }
    return 0;
}

// FUNCTION: WIZ8 0x0053BF10
unsigned char TargetSourceIsMonster(const W8TargetSource* source, int allow_indirect)
{
    if (source->iType == 2) {
        if (source->iMonsterID == BAD_INDEX) {
            srAssertFail("pSource->iMonsterID != BAD_INDEX", TARGETING_CPP, 0xcf8, 0);
        }
        return 1;
    }
    if (allow_indirect == 1 && source->iType == 3 && source->iMonsterID != BAD_INDEX) {
        if (!source->fBackfire && !source->fReflection) {
            srAssertFail("pSource->fBackfire || pSource->fReflection", TARGETING_CPP, 0xd00, 0);
        }
        return 1;
    }
    return 0;
}

/* The faction names, thirty bytes apart, in the same order as the faction ids.
   Twenty-one of them, which is the whole faction domain. */
extern const char g_faction_names[][0x1e];               /* 0x0061CE74 */
extern W8FactionRuntimeRecord g_faction_runtime[];       /* 0x0068D6EA */

extern char GetTargetNeededForSpellFriendly(int spell_id, unsigned char normalize, int context);
extern W8TargetBlock* Function53B7F0(int party_slot, int context);
extern unsigned char Function537160(W8TargetBlock* target, char needed);
extern unsigned char Function519180(int party_slot, int arg_2, int context);
extern unsigned char Function5207C0(const W8ItemDatabaseRecord* record, int context);
extern unsigned char g_targeting_flag_00685116;

/* Look a faction up by name, case-insensitively. -1 for a name that is not one
   of the twenty-one. */
// FUNCTION: WIZ8 0x005360B0
char FindFactionByName(const char* name)
{
    char faction;

    for (faction = 0; faction < W8_FACTION_COUNT; ++faction) {
        if (_stricmp(g_faction_names[faction], name) == 0) {
            return faction;
        }
    }
    return -1;
}

/* One faction's runtime value. */
// FUNCTION: WIZ8 0x005360F0
int GetFactionValue(char faction)
{
    return g_faction_runtime[faction].value_04;
}

/* Raise or lower one faction's flag. */
// FUNCTION: WIZ8 0x00536110
void SetFactionFlag(char faction, unsigned char flag)
{
    g_faction_runtime[faction].flag_08 = flag;
}

// FUNCTION: WIZ8 0x00536130
unsigned char GetFactionFlag(char faction)
{
    return g_faction_runtime[faction].flag_08;
}

/* Build an empty target block: everything zeroed, then the two ids set to
   BAD_INDEX. The kind is written twice, once by the clear and once on its
   own. */
// FUNCTION: WIZ8 0x00536150
void ResetTargetBlock(W8TargetBlock* target)
{
    memset(target, 0, sizeof(W8TargetBlock));
    target->kind = 0;
    target->character_slot = BAD_INDEX;
    target->monster_id = BAD_INDEX;
}

/* The same for the shorter inline form, which has a third id to invalidate. */
// FUNCTION: WIZ8 0x00536170
void ResetCombatSlot(W8CombatSlot* slot)
{
    memset(slot, 0, sizeof(W8CombatSlot));
    slot->kind = 0;
    slot->monster_id = BAD_INDEX;
    slot->character_slot = BAD_INDEX;
    slot->group_id = BAD_INDEX;
}

/* Clear whatever a monster was aiming at, and report that it now has no
   target. */
// FUNCTION: WIZ8 0x005369F0
bool ClearMonsterCombatSlot(W8MonsterInfo* monster_info)
{
    ResetCombatSlot(&monster_info->combat_slot_2ba);
    return false;
}

/* Whether one party slot's target is still good in a given context: it has to
   be of the kind the spell needs and within range. */
// FUNCTION: WIZ8 0x00537220
bool IsSpellTargetStillValidIn(int party_slot, int spell_id, int context)
{
    char needed = GetTargetNeededForSpellFriendly(spell_id, 0, context);

    if (!Function537160(Function53B7F0(party_slot, context), needed)) {
        return false;
    }
    return Function519180(party_slot, 0, context) != 0;
}

/* The same check without the range half, and with one target kind that a
   global override always accepts. */
// FUNCTION: WIZ8 0x00537270
unsigned char IsSpellTargetOfNeededKind(int party_slot, int spell_id)
{
    W8TargetBlock* target = Function53B7F0(party_slot, 6);
    int needed = GetTargetNeededForSpellFriendly(spell_id, 0, 6);

    if (needed == 2 && g_targeting_flag_00685116 != 0) {
        return 1;
    }
    return Function537160(target, (char)needed);
}

/* What an item's spell needs picked before it can be cast. An item with no
   spell needs nothing. */
// FUNCTION: WIZ8 0x00537330
char GetTargetNeededForItem(const W8ItemInstance* item)
{
    const W8ItemDatabaseRecord* record;

    if (item == 0 || item->item_id == -1) {
        return 0;
    }
    record = &g_item_records[item->item_id];
    if (record->spell_id == 0) {
        return 0;
    }
    return GetTargetNeededForSpellFriendly(
        record->spell_id, Function5207C0(record, 6), 0);
}
