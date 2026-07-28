#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"

#include <stdlib.h>
#include <string.h>

#define TARGETING_CPP "C:\\Projects\\Wizardry 8\\Local Code\\Targeting.cpp"

/* BAD_INDEX is -1: the canonical assertions read "!= BAD_INDEX" where the
   bodies compare against -1. */
#define BAD_INDEX (-1)

/* Point a source at one party character. Everything is cleared first and the
   monster id invalidated, so a source built this way never reads as a monster;
   the character id is the only thing left set. */
// FUNCTION: WIZ8 0x0053BE00
void SetTargetSourceToCharacter(int party_slot, W8TargetSource* source)
{
    if (source == 0) {
        srAssertFail("pSource != NULL", TARGETING_CPP, 0xcc9, 0);
    }
    memset(source, 0, sizeof(W8TargetSource));
    source->iMonsterID = BAD_INDEX;
    source->iType = 1;
    source->iChar = party_slot;
}

/* The same for a monster, and the mirror image of it: the character id is the
   one invalidated and the monster id the one left set. The monster is passed
   as its info record rather than as an id, so the id is read out of it here -
   which is what makes the two builders take different kinds of argument for
   the same job. */
// FUNCTION: WIZ8 0x0053BE50
void SetTargetSourceToMonster(const W8MonsterInfo* monster_info, W8TargetSource* source)
{
    if (source == 0) {
        srAssertFail("pSource != NULL", TARGETING_CPP, 0xcd5, 0);
    }
    memset(source, 0, sizeof(W8TargetSource));
    source->iMonsterID = BAD_INDEX;
    source->iChar = BAD_INDEX;
    source->iType = 2;
    source->iMonsterID = monster_info->location_id;
}

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
extern W8TargetSource* Function53B7F0(int party_slot, int context);
extern unsigned char Function537160(W8TargetSource* target, char needed);
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
void ResetTargetSource(W8TargetSource* source)
{
    memset(source, 0, sizeof(W8TargetSource));
    source->iType = 0;
    source->iChar = BAD_INDEX;
    source->iMonsterID = BAD_INDEX;
}

/* The same for the shorter inline form, which has a third id to invalidate. */
// FUNCTION: WIZ8 0x00536170
void ResetCombatSlot(W8CombatSlot* slot)
{
    memset(slot, 0, sizeof(W8CombatSlot));
    slot->iType = 0;
    slot->iMonsterID = BAD_INDEX;
    slot->iChar = BAD_INDEX;
    slot->iGroupID = BAD_INDEX;
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
    W8TargetSource* target = Function53B7F0(party_slot, 6);
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
    target.iMonsterID = BAD_INDEX;
    target.iChar = BAD_INDEX;
    target.iGroupID = BAD_INDEX;
    target.iType = kind;
    AimAtTarget(actor, &target, context);
}

/* Aim at one character. */
// FUNCTION: WIZ8 0x00538670
void AimAtCharacter(int actor, int character_slot, int context)
{
    W8CombatSlot target;

    memset(&target, 0, sizeof(target));
    target.iMonsterID = BAD_INDEX;
    target.iGroupID = BAD_INDEX;
    target.iChar = character_slot;
    target.iType = W8_TARGET_KIND_CHARACTER;
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
    target.iMonsterID = BAD_INDEX;
    target.iChar = BAD_INDEX;
    target.iGroupID = BAD_INDEX;
    target.iType = W8_TARGET_KIND_PLACE;
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
    target.iMonsterID = BAD_INDEX;
    target.iGroupID = BAD_INDEX;
    target.iType = W8_TARGET_KIND_CHARACTER;
    target.iChar = character_slot;
    ApplyTarget(&target, context);
}

// FUNCTION: WIZ8 0x00538D60
void SetTargetToMonster(int monster_id, int context)
{
    W8CombatSlot target;

    memset(&target, 0, sizeof(target));
    target.iChar = BAD_INDEX;
    target.iGroupID = BAD_INDEX;
    target.iType = W8_TARGET_KIND_MONSTER;
    target.iMonsterID = monster_id;
    ApplyTarget(&target, context);
}

// FUNCTION: WIZ8 0x00538DB0
void SetTargetToGroup(int group_id, int context)
{
    W8CombatSlot target;

    memset(&target, 0, sizeof(target));
    target.iMonsterID = BAD_INDEX;
    target.iChar = BAD_INDEX;
    target.iType = W8_TARGET_KIND_GROUP;
    target.iGroupID = group_id;
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
        if (target->iType == W8_TARGET_KIND_CHARACTER && target->iChar != BAD_INDEX) {
            matched = 1;
        }
        if (target->iType == W8_TARGET_KIND_MONSTER && target->iMonsterID != BAD_INDEX) {
            return TargetIsReachable(target);
        }
        if (matched) {
            return TargetIsReachable(target);
        }
        break;
    case 3:
    case 4:
        if (target->iType == W8_TARGET_KIND_PLACE) {
            return TargetIsReachable(target);
        }
        break;
    case 5:
        if (target->iType == W8_TARGET_KIND_GROUP && target->iGroupID != BAD_INDEX) {
            matched = 1;
        }
        if (target->iType == W8_TARGET_KIND_PARTY) {
            return TargetIsReachable(target);
        }
        if (matched) {
            return TargetIsReachable(target);
        }
        break;
    case 6:
        if (target->iType == W8_TARGET_KIND_ITEM && target->pPCItem != 0) {
            return TargetIsReachable(target);
        }
        break;
    case 7:
        if (target->iType == W8_TARGET_KIND_CHARACTER_INDIRECT &&
            target->iChar != BAD_INDEX) {
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
    W8TargetSource* target = Function53B7F0(party_slot, 6);
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

/* One monster considered as an auto-attack target, and the four things the
   ordering below reads out of it. The rest of the record is filled in as the
   candidate is built and is what makes the sort stable across the fields it
   does not compare. */
typedef struct W8MonsterTargetCandidate {
    int location_id;                     /* 0x00 */
    int state_04;                        /* 0x04: the monster's own 0x107 */
    unsigned char in_reach;              /* 0x08: reachable with a real attack */
    unsigned char pad_09[3];
    unsigned int range_band;             /* 0x0c: the first band that covers it */
    unsigned int hp_current;             /* 0x10 */
    unsigned char same_group;            /* 0x14: shares the caller's group */
    unsigned char pad_15[3];
    float distance;                      /* 0x18 */
} W8MonsterTargetCandidate;              /* 0x1c */

extern float MonsterDistanceToParty(W8MonsterInfo* monster_info);        /* 0x004C7CB0 */
extern unsigned char MonsterIsHostileTo(int party_slot, W8MonsterInfo* monster_info);
/* 0x00546F10 */
extern unsigned char CanReachTarget(
    int party_slot, int kind, W8MonsterInfo* monster_info, int context, int arg_5);
/* 0x005194E0 */
extern bool AnyoneStandsAhead(unsigned char position);
extern int GetBestMonsterAttackRange(const W8MonsterRecord* record, char close_quarters_only);
extern float CalcRangeDistance(int range_category);                      /* 0x0051A9A0 */
extern W8MonsterRecord* GetMonsterDataForInfo(W8MonsterInfo* monster_info);

/* The order the candidates are taken in: the monster in the lowest state
   first, then the one that can actually be reached, then the nearest. Only
   three of the record's seven fields are compared, so the rest are carried for
   the caller rather than for the sort. */
// FUNCTION: WIZ8 0x0053C920
int CompareMonsterTargetCandidates(const void* left, const void* right)
{
    const W8MonsterTargetCandidate* a = (const W8MonsterTargetCandidate*)left;
    const W8MonsterTargetCandidate* b = (const W8MonsterTargetCandidate*)right;

    if ((unsigned int)a->state_04 < (unsigned int)b->state_04) {
        return -1;
    }
    if ((unsigned int)a->state_04 > (unsigned int)b->state_04) {
        return 1;
    }
    if (a->in_reach != 0 && b->in_reach == 0) {
        return -1;
    }
    if (a->in_reach == 0 && b->in_reach != 0) {
        return 1;
    }
    if (a->distance < b->distance) {
        return -1;
    }
    if (a->distance > b->distance) {
        return 1;
    }
    return 0;
}

/* Which monster a party slot should turn on when it has to pick one for
   itself. Every live, in-combat, still-standing monster the slot is hostile to
   and can reach becomes a candidate; the candidates are then ordered and the
   first one taken.

   The whole array is built before any of it is compared, which is why the
   record carries fields the ordering never reads - they are what the caller
   would need if it took more than the first. A monster with no hit points at
   all is a data error rather than a candidate to skip. */
// FUNCTION: WIZ8 0x0053C720
int ChooseMonsterTarget(int party_slot, int group_id, int context)
{
    unsigned int monster_count = PListGetCount(g_active_monster_list_00683fad);
    W8MonsterTargetCandidate* candidates;
    W8MonsterTargetCandidate* next;
    size_t found = 0;
    unsigned int index;
    int chosen;

    if (monster_count == 0) {
        return BAD_INDEX;
    }
    candidates =
        (W8MonsterTargetCandidate*)malloc(monster_count * sizeof(W8MonsterTargetCandidate));
    if (candidates == 0) {
        return BAD_INDEX;
    }

    next = candidates;
    for (index = 0; index < monster_count; ++index) {
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(index);
        W8MonsterRecord* record;
        unsigned int band;

        if (monster_info->flag_14 == 0 || monster_info->fInCombat == 0 ||
            monster_info->hp_current == 0 ||
            MonsterIsHostileTo(party_slot, monster_info) != 1 ||
            !CanReachTarget(party_slot, 2, monster_info, context, 0)) {
            continue;
        }

        next->location_id = monster_info->location_id;
        next->state_04 = monster_info->value_107;
        next->in_reach = 0;

        record = GetMonsterDataForInfo(monster_info);
        if (GetBestMonsterAttackRange(record, 1) != -1) {
            unsigned int quadrant = GetMonsterQuadrant(monster_info) & 0xff;

            if ((unsigned char)quadrant == 2 ||
                (!AnyoneStandsAhead((unsigned char)quadrant) && AnyoneStandsAhead(4))) {
                next->in_reach = 1;
            }
        }

        /* The first range band whose reach covers where the monster is. */
        for (band = 0; band < 4; ++band) {
            if (MonsterDistanceToParty(monster_info) <= CalcRangeDistance((int)band)) {
                next->range_band = band;
                break;
            }
        }

        if (monster_info->hp_max == 0) {
            srAssertFail("pMonsterInfo->uiHPMax > 0", TARGETING_CPP, 0xeac, 0);
        }
        next->hp_current = monster_info->hp_current;
        next->same_group = (unsigned char)(monster_info->monster_group_id == group_id);
        next->distance = MonsterDistanceToParty(monster_info);

        ++found;
        ++next;
    }

    if (found == 0) {
        free(candidates);
        return BAD_INDEX;
    }
    qsort(
        candidates, found, sizeof(W8MonsterTargetCandidate), CompareMonsterTargetCandidates);
    chosen = candidates[0].location_id;
    free(candidates);
    return chosen;
}

/* The two conditions the indirect character kind reads: eighteen has to be
   running and nineteen not. The monster kind reads the same eighteenth entry of
   its own array, which is what pairs the two arrays entry for entry. */
enum { W8_CONDITION_REACHABLE_WHEN_DOWN = 18, W8_CONDITION_BEYOND_REACH = 19 };

/* Whether whatever a target names is still there to be acted on. Each kind
   checks its own field and then whatever that field points at, which is what
   makes the four assertions here - on iChar, iMonsterID, iGroupID and pPCItem -
   name four different fields of one block rather than one field four times.

   The two character kinds differ in what "still there" means: the direct one
   wants somebody alive and in a state under 0x12, and the indirect one wants
   the opposite, somebody with no hit points left whose two death fields say
   they can still be reached. Everything else - a place, the party as a whole -
   is always there and is not answered here at all. */
// FUNCTION: WIZ8 0x00536190
unsigned char IsTargetStillPresent(const W8CombatSlot* target)
{
    if (target == 0) {
        return 0;
    }

    switch (target->iType) {
    case W8_TARGET_KIND_CHARACTER:
        if (target->iChar == BAD_INDEX) {
            srAssertFail("pTarget->iChar != BAD_INDEX", TARGETING_CPP, 0x6c, 0);
        }
        if (g_party_slot_rows[target->iChar].flag_00 == 0 ||
            g_party_characters[target->iChar].hp_current == 0 ||
            g_party_characters[target->iChar].unknown_0b01 > 0x11) {
            return 0;
        }
        break;

    case W8_TARGET_KIND_CHARACTER_INDIRECT:
        if (target->iChar == BAD_INDEX) {
            srAssertFail("pTarget->iChar != BAD_INDEX", TARGETING_CPP, 0x75, 0);
        }
        if (g_party_slot_rows[target->iChar].flag_00 == 0 ||
            g_party_characters[target->iChar].hp_current != 0 ||
            g_party_characters[target->iChar].condition_turns[W8_CONDITION_REACHABLE_WHEN_DOWN] == 0 ||
            g_party_characters[target->iChar].condition_turns[W8_CONDITION_BEYOND_REACH] != 0) {
            return 0;
        }
        break;

    case W8_TARGET_KIND_MONSTER: {
        int index;
        W8MonsterInfo* monster_info;

        if (target->iMonsterID == BAD_INDEX) {
            srAssertFail("pTarget->iMonsterID != BAD_INDEX", TARGETING_CPP, 0x7e, 0);
        }
        index = MonsterGetIndexByLocationID(0x80, TARGETING_CPP, target->iMonsterID, 0);
        if (index == BAD_INDEX) {
            return 0;
        }
        monster_info = MonsterGetScriptPartByLocationIndex(index);
        if (monster_info == 0) {
            srAssertFail("pMonsterInfo != NULL", TARGETING_CPP, 0x88, 0);
        }
        if (monster_info->hp_current == 0 || monster_info->condition_turns[W8_CONDITION_REACHABLE_WHEN_DOWN] != 0 ||
            monster_info->flag_14 == 0) {
            return 0;
        }
        break;
    }

    case W8_TARGET_KIND_GROUP: {
        unsigned int group_list_index;
        W8MonsterGroup* group;

        if (target->iGroupID == BAD_INDEX) {
            srAssertFail("pTarget->iGroupID != BAD_INDEX", TARGETING_CPP, 0x99, 0);
        }
        group_list_index = GetMonsterGroupIndexByID(0x9b, TARGETING_CPP, target->iGroupID, 0);
        if (group_list_index == 0xffffffff) {
            return 0;
        }
        group = GetMonsterGroupByListIndex(group_list_index);
        if (group == 0) {
            srAssertFail("pMonsterGroup != NULL", TARGETING_CPP, 0xa3, 0);
        }
        if (group->member_count == 0 || group->flag_28 == 0) {
            return 0;
        }
        break;
    }

    case W8_TARGET_KIND_ITEM:
        if (target->pPCItem == 0) {
            srAssertFail("pTarget->pPCItem != NULL", TARGETING_CPP, 0xb4, 0);
        }
        if (target->pPCItem->item_id == BAD_INDEX) {
            return 0;
        }
        break;

    default:
        break;
    }
    return 1;
}

extern unsigned char IsSlotActionChosen(int party_slot, int context, int arg_3, int arg_4);
/* 0x004E79A0 */
extern unsigned char CanTargetMonsterWithAction(
    int party_slot, int location_id, int arg_3, int arg_4);              /* 0x00536AD0 */
extern void ClearMonsterTargetNotice(void);                              /* 0x00547510 */
extern void SetTargetCursor(int cursor);                                 /* 0x0055EE70 */
extern void ClearTargetCursor(void);                                     /* 0x0055EF90 */
extern int IsScreenIdle(void);
extern void* g_modal_owner_0068edd0;
extern int g_target_cursor_00683fdb;

/* The two cursors this body cares about: the one it puts up for a monster it
   can act on, and the one it takes down for a monster it cannot. */
enum { W8_CURSOR_VALID_TARGET = 6, W8_CURSOR_INVALID_TARGET = 7 };

/* Tint one monster to say whether the character could act on it, and move the
   cursor to match. Green means yes and red means no; asking for no highlight at
   all tints it to nothing and answers no without touching the cursor.

   With no character named the answer is yes outright, so a bare hover over a
   monster reads as valid - it is the action, not the monster, that makes a
   target invalid. The cursor is only moved while nothing modal is up, and the
   valid case additionally waits for the screen to be idle, which is what keeps
   a cursor from flickering under a menu. */
// FUNCTION: WIZ8 0x00539110
char HighlightMonsterAsTarget(int location_id, int party_slot, char highlight)
{
    int index = MonsterGetIndexByLocationID(0x68d, TARGETING_CPP, location_id, 0);
    W8MonsterInfo* monster_info;
    W8Monster* monster;
    char valid = 0;

    if (index == BAD_INDEX) {
        return 0;
    }
    monster_info = MonsterGetScriptPartByLocationIndex(index);
    monster = monster_info->monster;

    if (highlight == 0) {
        SetMonsterHighlightColour(monster, 0.0f, 0.0f, 0.0f, 0.0f);
        return 0;
    }

    if (party_slot == BAD_INDEX ||
        (IsSlotActionChosen(party_slot, 6, 1, 0) &&
         CanTargetMonsterWithAction(party_slot, location_id, 1, 0))) {
        valid = 1;
    }
    ClearMonsterTargetNotice();

    if (valid == 0) {
        SetMonsterHighlightColour(monster, 1.0f, 0.0f, 0.0f, 1.0f);
        if (g_modal_owner_0068edd0 == 0 && g_target_cursor_00683fdb == W8_CURSOR_VALID_TARGET) {
            ClearTargetCursor();
        }
        return 0;
    }

    SetMonsterHighlightColour(monster, 0.0f, 1.0f, 0.0f, 1.0f);
    if (g_modal_owner_0068edd0 == 0 && g_highlight_suppressed_00683fe7 == 0 && IsScreenIdle() &&
        g_target_cursor_00683fdb != W8_CURSOR_INVALID_TARGET) {
        SetTargetCursor(W8_CURSOR_VALID_TARGET);
    }
    return valid;
}
