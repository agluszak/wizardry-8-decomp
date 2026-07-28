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

extern void AimAtTarget(int actor, W8CombatSlot* target, int context);   /* 0x005387F0 */
extern void ApplyTarget(W8CombatSlot* target, int context);              /* 0x00538E00 */
extern unsigned char TargetIsReachable(W8CombatSlot* target);            /* 0x00536190 */
extern void GetPartyEyePosition(void* position);                         /* 0x00421070 */
extern void GetMonsterBounds(W8Monster* monster, void* lower, void* upper);
/* 0x004CA4F0 */
extern void ShowTargetMarker(void* eye, void* lower, void* upper);       /* 0x0046F820 */
extern void Function492500(void* scratch);
extern void Function5653F0(void);
extern unsigned char g_target_marker_00684073;

/* Aim at whatever the caller names, by kind. The other three fields are left
   at BAD_INDEX, so only the kind's own field is meaningful. */
// FUNCTION: WIZ8 0x00538620
void AimByKind(int actor, int kind, int context)
{
    W8CombatSlot target;

    memset(&target, 0, sizeof(target));
    target.monster_id = BAD_INDEX;
    target.character_slot = BAD_INDEX;
    target.group_id = BAD_INDEX;
    target.kind = kind;
    AimAtTarget(actor, &target, context);
}

/* Aim at one character. */
// FUNCTION: WIZ8 0x00538670
void AimAtCharacter(int actor, int character_slot, int context)
{
    W8CombatSlot target;

    memset(&target, 0, sizeof(target));
    target.monster_id = BAD_INDEX;
    target.group_id = BAD_INDEX;
    target.character_slot = character_slot;
    target.kind = W8_TARGET_KIND_CHARACTER;
    AimAtTarget(actor, &target, context);
}

/* Aim at the place the party is looking, drop the marker and let the display
   know. */
// FUNCTION: WIZ8 0x00538710
void AimAtPlace(int actor)
{
    W8CombatSlot target;
    unsigned char scratch[16];

    memset(&target, 0, sizeof(target));
    target.monster_id = BAD_INDEX;
    target.character_slot = BAD_INDEX;
    target.group_id = BAD_INDEX;
    target.kind = W8_TARGET_KIND_PLACE;
    Function492500(scratch);
    AimAtTarget(actor, &target, W8_TARGET_KIND_PLACE);
    g_target_marker_00684073 = 0;
    Function5653F0();
}

/* The three wrappers that set the party's own target rather than a
   combatant's, one per kind that names something. */
// FUNCTION: WIZ8 0x00538D10
void SetTargetToCharacter(int character_slot, int context)
{
    W8CombatSlot target;

    memset(&target, 0, sizeof(target));
    target.monster_id = BAD_INDEX;
    target.group_id = BAD_INDEX;
    target.kind = W8_TARGET_KIND_CHARACTER;
    target.character_slot = character_slot;
    ApplyTarget(&target, context);
}

// FUNCTION: WIZ8 0x00538D60
void SetTargetToMonster(int monster_id, int context)
{
    W8CombatSlot target;

    memset(&target, 0, sizeof(target));
    target.character_slot = BAD_INDEX;
    target.group_id = BAD_INDEX;
    target.kind = W8_TARGET_KIND_MONSTER;
    target.monster_id = monster_id;
    ApplyTarget(&target, context);
}

// FUNCTION: WIZ8 0x00538DB0
void SetTargetToGroup(int group_id, int context)
{
    W8CombatSlot target;

    memset(&target, 0, sizeof(target));
    target.monster_id = BAD_INDEX;
    target.character_slot = BAD_INDEX;
    target.kind = W8_TARGET_KIND_GROUP;
    target.group_id = group_id;
    ApplyTarget(&target, context);
}

/* Put the on-screen marker over one monster, from the party's eye to the
   monster's own bounds. */
// FUNCTION: WIZ8 0x00539870
void ShowMonsterTargetMarker(W8MonsterInfo* monster_info)
{
    unsigned char eye[8];
    unsigned char lower[12];
    unsigned char upper[12];

    if (monster_info == 0) {
        srAssertFail("pMonsterInfo", TARGETING_CPP, 2040, 0);
    }
    GetPartyEyePosition(eye);
    GetMonsterBounds(monster_info->monster, lower, upper);
    ShowTargetMarker(eye, lower, upper);
}

/* Whether a recorded target is of the kind a caller needs. Each needed kind
   admits one or two target kinds, and every admitted target additionally has
   to be reachable - so the answer is never just the kind test. */
// FUNCTION: WIZ8 0x00537160
char TargetMatchesNeeded(W8CombatSlot* target, int needed)
{
    char matched = 0;

    if (target == 0) {
        return 0;
    }
    switch (needed) {
    case 1:
    case 2:
    case 8:
        if (target->kind == W8_TARGET_KIND_CHARACTER && target->character_slot != BAD_INDEX) {
            matched = 1;
        }
        if (target->kind == W8_TARGET_KIND_MONSTER && target->monster_id != BAD_INDEX) {
            return TargetIsReachable(target);
        }
        if (matched) {
            return TargetIsReachable(target);
        }
        break;
    case 3:
    case 4:
        if (target->kind == W8_TARGET_KIND_PLACE) {
            return TargetIsReachable(target);
        }
        break;
    case 5:
        if (target->kind == W8_TARGET_KIND_GROUP && target->group_id != BAD_INDEX) {
            matched = 1;
        }
        if (target->kind == W8_TARGET_KIND_PARTY) {
            return TargetIsReachable(target);
        }
        if (matched) {
            return TargetIsReachable(target);
        }
        break;
    case 6:
        if (target->kind == W8_TARGET_KIND_ITEM && target->item != 0) {
            return TargetIsReachable(target);
        }
        break;
    case 7:
        if (target->kind == W8_TARGET_KIND_CHARACTER_INDIRECT &&
            target->character_slot != BAD_INDEX) {
            return TargetIsReachable(target);
        }
        break;
    }
    return 0;
}

extern unsigned char MonsterGetRuntimeFlag5BC(W8Monster* monster);
extern void MonsterSetRuntimeFlag5BC(W8Monster* monster, unsigned char flags);
extern void NotifyMonsterHighlight(int party_slot, int location_id, int on);
/* 0x004C5EB0 */
extern void SetMonsterHighlightColour(
    W8Monster* monster, float r, float g, float b, float a);            /* 0x004C5AD0 */
extern unsigned int GetMonsterGroupIndexByID(
    int caller_line, const char* caller_file, int group_id, unsigned char assert_on_failure);
extern W8MonsterGroup* GetMonsterGroupByListIndex(unsigned int index);
extern unsigned char ItemClassNormalizesTarget(const W8ItemDatabaseRecord* record);
extern W8LevelRuntimeBlock* g_level_block;
extern W8PList* g_active_monster_list_00683fad;
extern int g_highlight_suppressed_00683fe7;

/* What the interface has to ask the player to pick for one action. Most
   actions answer a fixed kind; casting asks the spell and using an item asks
   the item's own spell, which is the same two-step the item path takes. */
// FUNCTION: WIZ8 0x00536A20
char GetTargetNeededForAction(int action, int spell_id, const W8ItemInstance** item)
{
    const W8ItemDatabaseRecord* record;

    switch (action) {
    case 0:
    case 1:
        return 2;
    case 2:
        return 4;
    case 5:
        return 1;
    case 7:
        return GetTargetNeededForSpellFriendly(spell_id, 0, 6);
    case 8:
        if (item[1] != 0 && (*(const int**)&item[1])[0] != -1) {
            record = &g_item_records[(*(const int**)&item[1])[0]];
            if (record->spell_id != 0) {
                return GetTargetNeededForSpellFriendly(
                    record->spell_id, ItemClassNormalizesTarget(record), 0);
            }
        }
        break;
    }
    return 0;
}

/* Whether a slot's recorded target suits the item it would be used with. An
   item with no spell needs nothing picked, and one target kind a global
   override always accepts. */
// FUNCTION: WIZ8 0x005372B0
unsigned char IsItemTargetOfNeededKind(int party_slot, const W8ItemInstance* item)
{
    W8TargetBlock* target = Function53B7F0(party_slot, 6);
    const W8ItemDatabaseRecord* record;
    int needed = 0;

    if (item != 0 && item->item_id != -1) {
        record = &g_item_records[item->item_id];
        if (record->spell_id != 0) {
            needed = GetTargetNeededForSpellFriendly(
                record->spell_id, ItemClassNormalizesTarget(record), 0);
            if (needed == 2 && g_targeting_flag_00685116 != 0) {
                return 1;
            }
        }
    }
    return Function537160(target, (char)needed);
}

/* Tint one monster for whoever is highlighting it. Three tints are named -
   nothing, and two that differ only in which channel is lit - and everything
   else leaves the colour as it was. */
// FUNCTION: WIZ8 0x00539480
void TintHighlightedMonster(W8Monster* monster, int tint)
{
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 0.0f;

    if (tint == 1) {
        g = 1.0f;
        a = 1.0f;
    }
    else if (tint == 2) {
        r = 1.0f;
        a = 1.0f;
    }
    SetMonsterHighlightColour(monster, r, g, b, a);
}

/* Raise or lower one character's bit in a monster's highlight mask, and tell
   whatever draws it. */
// FUNCTION: WIZ8 0x00539630
void SetMonsterHighlight(int party_slot, int location_id, int unused, char on)
{
    int index = MonsterGetIndexByLocationID(1879, TARGETING_CPP, location_id, 0);
    W8MonsterInfo* monster_info;
    W8Monster* monster;
    unsigned char bit;

    if (index == -1) {
        return;
    }
    monster_info = MonsterGetScriptPartByLocationIndex(index);
    monster = monster_info->monster;
    if (monster == 0) {
        srAssertFail("pMonster", TARGETING_CPP, 1888, 0);
    }

    bit = (unsigned char)(1 << (location_id & 0x1f));
    if (on) {
        MonsterSetRuntimeFlag5BC(monster, MonsterGetRuntimeFlag5BC(monster) | bit);
        NotifyMonsterHighlight(location_id, location_id, 1);
        return;
    }
    MonsterSetRuntimeFlag5BC(monster, MonsterGetRuntimeFlag5BC(monster) & ~bit);
    NotifyMonsterHighlight(location_id, location_id, 0);
}

/* The same over a whole group, one member at a time. The count is re-read each
   step because highlighting can remove a member. */
// FUNCTION: WIZ8 0x00538C00
void SetGroupHighlight(int party_slot, int group_id, char on)
{
    unsigned int group_index =
        GetMonsterGroupIndexByID(1498, TARGETING_CPP, group_id, 0);
    W8MonsterGroup* group;
    unsigned int member;
    int location_id;
    int index;
    W8MonsterInfo* monster_info;
    W8Monster* monster;
    unsigned char bit;

    if (group_index == 0xffffffff) {
        return;
    }
    group = GetMonsterGroupByListIndex(group_index);
    for (member = 0; member < PListGetCount((W8PList*)group->monsters); ++member) {
        location_id = IListGetAt(group->monsters, member);
        index = MonsterGetIndexByLocationID(1879, TARGETING_CPP, location_id, 0);
        if (index == -1) {
            continue;
        }
        monster_info = MonsterGetScriptPartByLocationIndex(index);
        monster = monster_info->monster;
        if (monster == 0) {
            srAssertFail("pMonster", TARGETING_CPP, 1888, 0);
        }
        bit = (unsigned char)(1 << (party_slot & 0x1f));
        if (on == 0) {
            MonsterSetRuntimeFlag5BC(monster, MonsterGetRuntimeFlag5BC(monster) & ~bit);
        }
        else {
            MonsterSetRuntimeFlag5BC(monster, MonsterGetRuntimeFlag5BC(monster) | bit);
        }
        NotifyMonsterHighlight(party_slot, location_id, on != 0);
    }
}

/* Re-tint every monster for one character. A character whose highlight is
   overridden tints for whoever overrode it instead, and then only the monsters
   that character was already highlighting. */
// FUNCTION: WIZ8 0x00539570
void UpdateAllMonsterHighlights(int party_slot, int location_id)
{
    bool overridden = false;
    int owner = party_slot;
    unsigned int index;
    W8MonsterInfo* monster_info;
    int tint;

    if (g_level_block->highlight_override != -1 && g_highlight_suppressed_00683fe7 == 0) {
        overridden = true;
        owner = g_level_block->highlight_override;
    }

    for (index = 0; index < PListGetCount(g_active_monster_list_00683fad); ++index) {
        monster_info = MonsterGetScriptPartByLocationIndex(index);
        if (monster_info->flag_14 == 0) {
            continue;
        }
        if (location_id == monster_info->location_id) {
            tint = 1;
        }
        else if (overridden &&
                 ((1 << (owner & 0x1f)) &
                  MonsterGetRuntimeFlag5BC(monster_info->monster)) != 0) {
            tint = 1;
        }
        else {
            tint = 0;
        }
        SetMonsterHighlight(monster_info->location_id, owner, 0, (char)tint);
    }
}
