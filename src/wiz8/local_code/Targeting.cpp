#include "wiz8/float_constants.h"
#include "wiz8/xstatus.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/local_code/GameplayDatabase.h"
#include "wiz8/layouts/item_tables.h"
#include "wiz8/factions.h"
#include "wiz8/targeting.h"
#include "wiz8/character.h"
#include "wiz8/combat_state.h"
#include "wiz8/game_status.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/magic.h"
#include "wiz8/screen_state.h"
#include "wiz8/monster_runtime.h"
#include "wiz8/utility.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/sr_api.h"
#include "Types.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define TARGETING_CPP "C:\\Projects\\Wizardry 8\\Local Code\\Targeting.cpp"

W8CombatSlot* GetTargetBlockForContext(int party_slot, unsigned int context);
int ResolveTargetingContext(int party_slot, unsigned int context);
unsigned char GetCurrentTargetingContext(int party_slot);


/* Point a source at one party character. Everything is cleared first and the
   monster id invalidated, so a source built this way never reads as a monster;
   the character id is the only thing left set. */
// FUNCTION: WIZ8 0x0053be00
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
// FUNCTION: WIZ8 0x0053be50
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

// FUNCTION: WIZ8 0x0053bea0
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

// FUNCTION: WIZ8 0x0053bf10
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
char TargetMatchesNeeded(W8CombatSlot* target, int needed);
extern unsigned char Function519180(int party_slot, int arg_2, int context);
extern unsigned char ItemClassNormalizesTarget(const W8ItemDatabaseRecord* record, int context);
extern unsigned char g_targeting_flag_00685116;

/* Look a faction up by name, case-insensitively. -1 for a name that is not one
   of the twenty-one. */
// FUNCTION: WIZ8 0x005360b0
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
// FUNCTION: WIZ8 0x005360f0
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
// FUNCTION: WIZ8 0x005369f0
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

    if (!TargetMatchesNeeded(GetTargetBlockForContext(party_slot, context), needed)) {
        return false;
    }
    return Function519180(party_slot, 0, context) != 0;
}

/* The same check without the range half, and with one target kind that a
   global override always accepts. */
// FUNCTION: WIZ8 0x00537270
unsigned char IsSpellTargetOfNeededKind(int party_slot, int spell_id)
{
    W8CombatSlot* target = GetTargetBlockForContext(party_slot, W8_TARGETING_CONTEXT_CURRENT);
    int needed = GetTargetNeededForSpellFriendly(spell_id, 0, 6);

    if (needed == 2 && g_targeting_flag_00685116 != 0) {
        return 1;
    }
    return TargetMatchesNeeded(target, (char)needed);
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
        record->spell_id, ItemClassNormalizesTarget(record, 6), 0);
}

extern void AimAtTarget(int actor, W8CombatSlot* target, int context);   /* 0x005387F0 */
extern void ApplyTarget(W8CombatSlot* target, int context);              /* 0x00538E00 */
extern unsigned char TargetIsReachable(W8CombatSlot* target);            /* 0x00536190 */
extern void GetPartyEyePosition(void* position);                         /* 0x00421070 */
extern void GetMonsterBounds(W8Monster* monster, void* lower, void* upper);
/* 0x004CA4F0 */
extern void ShowTargetMarker(void* eye, void* lower, void* upper);       /* 0x0046F820 */
extern void Function492500(void* scratch);
extern void RequestRefreshPartyState(void);
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
    RequestRefreshPartyState();
}

/* The three wrappers that set the party's own target rather than a
   combatant's, one per kind that names something. */
// FUNCTION: WIZ8 0x00538d10
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

// FUNCTION: WIZ8 0x00538d60
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

// FUNCTION: WIZ8 0x00538db0
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
    W8Monster* monster, float r, float g, float b, float a);         /* 0x004C5AD0 */
extern unsigned int GetMonsterGroupIndexByID(
    int caller_line, const char* caller_file, int group_id, unsigned char assert_on_failure);
extern W8MonsterGroup* GetMonsterGroupByListIndex(unsigned int index);
extern unsigned char ItemClassNormalizesTarget(const W8ItemDatabaseRecord* record);

/* What the interface has to ask the player to pick for one action. Most
   actions answer a fixed kind; casting asks the spell and using an item asks
   the item's own spell, which is the same two-step the item path takes. */
// FUNCTION: WIZ8 0x00536a20
char GetTargetNeededForAction(
    int action, int spell_id, const W8ActionDetailBlock* detail_block)
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
        if (detail_block->item_use.item != 0 && detail_block->item_use.item->item_id != -1) {
            record = &g_item_records[detail_block->item_use.item->item_id];
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
// FUNCTION: WIZ8 0x005372b0
unsigned char IsItemTargetOfNeededKind(int party_slot, const W8ItemInstance* item)
{
    W8CombatSlot* target = GetTargetBlockForContext(party_slot, W8_TARGETING_CONTEXT_CURRENT);
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
    return TargetMatchesNeeded(target, (char)needed);
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
// FUNCTION: WIZ8 0x00538c00
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
    for (member = 0; member < ILLength(group->monsters); ++member) {
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

    for (index = 0; index < PLLength(g_active_monster_list_00683fad); ++index) {
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
// FUNCTION: WIZ8 0x0053c920
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
// FUNCTION: WIZ8 0x0053c720
int ChooseMonsterTarget(int party_slot, int group_id, int context)
{
    unsigned int monster_count = PLLength(g_active_monster_list_00683fad);
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
            if (monster_info->monster->GetDistanceToPlayer004C7CB0() <=
                CalcRangeDistance((int)band)) {
                next->range_band = band;
                break;
            }
        }

        if (monster_info->hp_max == 0) {
            srAssertFail("pMonsterInfo->uiHPMax > 0", TARGETING_CPP, 0xeac, 0);
        }
        next->hp_current = monster_info->hp_current;
        next->same_group = (unsigned char)(monster_info->monster_group_id == group_id);
        next->distance = monster_info->monster->GetDistanceToPlayer004C7CB0();

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
        if (g_party_slot_rows[target->iChar].occupied == 0 ||
            g_party_characters[target->iChar].hp_current == 0 ||
            g_party_characters[target->iChar].unknown_0b01 > 0x11) {
            return 0;
        }
        break;

    case W8_TARGET_KIND_CHARACTER_INDIRECT:
        if (target->iChar == BAD_INDEX) {
            srAssertFail("pTarget->iChar != BAD_INDEX", TARGETING_CPP, 0x75, 0);
        }
        if (g_party_slot_rows[target->iChar].occupied == 0 ||
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
        if (g_modal_owner_0068edd0 == 0 && g_cursor_state_00683fdb == W8_CURSOR_VALID_TARGET) {
            ClearTargetCursor();
        }
        return 0;
    }

    SetMonsterHighlightColour(monster, 0.0f, 1.0f, 0.0f, 1.0f);
    if (g_modal_owner_0068edd0 == 0 && g_highlight_suppressed_00683fe7 == 0 && IsScreenIdle() &&
        g_cursor_state_00683fdb != W8_CURSOR_INVALID_TARGET) {
        SetTargetCursor(W8_CURSOR_VALID_TARGET);
    }
    return valid;
}

/* Take down whatever one party slot was highlighting. A slot with a highlight
   list of its own empties that and nothing else; only a slot with none falls
   through to the target it was given, and then a monster and a group are
   handled separately because each has its own way of naming its monsters.

   Clearing a monster drops this slot's bit out of the monster's own highlight
   mask, so a monster several slots are highlighting stays lit for the rest;
   the original carries that body inline at both places rather than calling
   it. */
// FUNCTION: WIZ8 0x0053ac30
void ClearTargetHighlights(int party_slot, const W8CombatSlot* target)
{
    W8MonsterManagerEntry* slot = &g_monster_manager_state.entries[party_slot];
    unsigned int index;

    if (slot->highlighted_monsters.count > 0) {
        for (index = 0; index < (unsigned int)slot->highlighted_monsters.count; ++index) {
            SetMonsterHighlight(party_slot, slot->highlighted_monsters.data[index], 0, 0);
        }
        slot->highlighted_monsters.count = 0;
        return;
    }

    if (target->iType == W8_TARGET_KIND_MONSTER && target->iMonsterID != BAD_INDEX) {
        SetMonsterHighlight(party_slot, target->iMonsterID, 0, 0);
    }

    if (target->iType == W8_TARGET_KIND_GROUP && target->iGroupID != BAD_INDEX) {
        unsigned int group_list_index =
            GetMonsterGroupIndexByID(0x5da, TARGETING_CPP, target->iGroupID, 0);

        if (group_list_index != 0xffffffff) {
            W8MonsterGroup* group = GetMonsterGroupByListIndex(group_list_index);

            for (index = 0; index < ILLength(group->monsters); ++index) {
                SetMonsterHighlight(party_slot, IListGetAt(group->monsters, index), 0, 0);
            }
        }
    }
}

/* 0x005EBB34: the float that stands for "no distance given". GameData.cpp
   reads the same constant as the level vector's absent value. */
/* The side selector that means any side at all. */
enum { W8_SIDE_ANY = 3 };

/* Gather every monster within one distance of a point, appending their
   location ids to the caller's vector. The distance is measured to the
   monster's surface rather than its centre, which is what the radius
   subtraction is; a distance of the "no distance given" constant gathers
   nothing at all rather than everything.

   The highlighting caller and the line-of-sight caller share the body: with
   highlighting on, a monster out of range has its tint cleared and one in
   range is taken without looking, and with it off nothing is tinted and a
   monster in range still has to be visible from the given eye point. */
// FUNCTION: WIZ8 0x00539e70
void CollectMonstersWithinRadius(
    const srVector3T<float>* centre, const srVector3T<float>* eye, W8GrowableVector<int>* found, float radius,
    char side, char highlighting)
{
    unsigned int index;

    if (radius == g_float_005ebb34) {
        return;
    }

    for (index = 0; index < PLLength(g_active_monster_list_00683fad); ++index) {
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(index);
        W8Monster* monster = monster_info->monster;
        W8MonsterRecord* record;
        srVector3T<float> position;
        float dx;
        float dy;
        float dz;

        if (monster_info->flag_14 == 0 || monster_info->hp_current == 0 ||
            monster_info->condition_turns[W8_CONDITION_REACHABLE_WHEN_DOWN] != 0) {
            continue;
        }
        record = GetMonsterDataForInfo(monster_info);
        if (record->untargetable_24a != 0) {
            continue;
        }
        if (monster_info->flag_16 != side && side != W8_SIDE_ANY) {
            continue;
        }

        position = monster->GetPosition();
        dx = centre->x - position.x;
        dy = centre->y - position.y;
        dz = centre->z - position.z;

        if (radius < (float)sqrt(dx * dx + dy * dy + dz * dz) - monster->radius_084) {
            if (highlighting != 0) {
                SetMonsterHighlightColour(monster, 0.0f, 0.0f, 0.0f, 0.0f);
            }
            continue;
        }
        if (highlighting != 0 ||
            monster_info->monster->HasLineOfSightFromPoint004C4C40(*eye)) {
            found->Add(monster_info->location_id);
        }
    }
}
/* 0x0068408B: the target block context two answers with, shared by the whole
   party rather than kept per slot - which is what makes it the odd one out
   among the six. */
extern W8CombatSlot g_shared_target_0068408b;


/* The two dialogue selections that have a targeting context of their own, and
   they are the same two action kinds - casting and using an item. */
enum { W8_SELECTION_SPELL = 7, W8_SELECTION_ITEM = 8 };

/* Which targeting context is in force. A dialogue that is up and has settled on
   casting or on using an item owns the choice; failing that, the active slot
   with either overlay up gets the shared context, and otherwise it is simply
   whether a fight is on.

   Everything below carries this body inline rather than calling it, which is
   why the same fifteen-odd instructions open three of them. */
// FUNCTION: WIZ8 0x0053bc10
unsigned char GetCurrentTargetingContext(int party_slot)
{
    if (g_screen_state_0068ec78.id == W8_SCREEN_MAIN_GAME && g_level_block != 0 &&
        g_level_block->selection_kind != -1) {
        if (g_level_block->selection_kind == W8_SELECTION_SPELL &&
            g_level_block->selection_settled != 0) {
            return W8_TARGETING_CONTEXT_SPELL;
        }
        if (g_level_block->selection_kind == W8_SELECTION_ITEM &&
            g_level_block->selection_settled != 0) {
            return W8_TARGETING_CONTEXT_ITEM;
        }
        return W8_TARGETING_CONTEXT_DIALOGUE;
    }
    if (party_slot == g_status_685170.selected_character &&
        (g_flag_00683f95 != 0 || g_flag_00683f96 != 0)) {
        return W8_TARGETING_CONTEXT_SHARED;
    }
    return (unsigned char)(g_in_combat_00683f94 != 0);
}

/* Resolve "current" to a real context and check that what comes back is one.
   The switch answers each context with itself, so it exists only to catch a
   sixth value the caller invented rather than to map anything. */
// FUNCTION: WIZ8 0x0053b920
int ResolveTargetingContext(int party_slot, unsigned int context)
{
    if (context == W8_TARGETING_CONTEXT_CURRENT) {
        context = GetCurrentTargetingContext(party_slot);
    }
    switch (context) {
    case W8_TARGETING_CONTEXT_OUT_OF_COMBAT:
        return W8_TARGETING_CONTEXT_OUT_OF_COMBAT;
    case W8_TARGETING_CONTEXT_IN_COMBAT:
        return W8_TARGETING_CONTEXT_IN_COMBAT;
    case W8_TARGETING_CONTEXT_SHARED:
        return W8_TARGETING_CONTEXT_SHARED;
    case W8_TARGETING_CONTEXT_SPELL:
        return W8_TARGETING_CONTEXT_SPELL;
    case W8_TARGETING_CONTEXT_ITEM:
        return W8_TARGETING_CONTEXT_ITEM;
    case W8_TARGETING_CONTEXT_FIVE:
        return W8_TARGETING_CONTEXT_FIVE;
    case W8_TARGETING_CONTEXT_DIALOGUE:
        return W8_TARGETING_CONTEXT_DIALOGUE;
    default:
        srAssertFail("FALSE", TARGETING_CPP, 0xc5b, 0);
    }
    return W8_TARGETING_CONTEXT_IN_COMBAT;
}

/* The target block one context uses. Five of the six live on the slot's own
   row and the sixth is shared by the party, which is what makes this the one
   place that knows the row holds five blocks of the same shape rather than one
   block and four runs of numbers. */
// FUNCTION: WIZ8 0x0053b7f0
W8CombatSlot* GetTargetBlockForContext(int party_slot, unsigned int context)
{
    W8PartySlotRow* row = &g_party_slot_rows[party_slot];

    if (context == W8_TARGETING_CONTEXT_CURRENT) {
        context = GetCurrentTargetingContext(party_slot);
    }
    switch (context) {
    case W8_TARGETING_CONTEXT_OUT_OF_COMBAT:
        return &row->target_out_of_combat;
    case W8_TARGETING_CONTEXT_IN_COMBAT:
        return &row->target_in_combat;
    case W8_TARGETING_CONTEXT_SHARED:
        return &g_shared_target_0068408b;
    case W8_TARGETING_CONTEXT_SPELL:
        return &row->spell_target;
    case W8_TARGETING_CONTEXT_ITEM:
        return &row->item_target;
    case W8_TARGETING_CONTEXT_FIVE:
        return &row->target_context_5;
    default:
        srAssertFail("FALSE", TARGETING_CPP, 0xc37, 0);
    }
    return 0;
}

extern W8MonsterRecord* GetMonsterGroupRecord(W8MonsterGroup* group);    /* 0x00510180 */
/* 0x004E77B0: hands back what the slot has chosen - the action, the detail
   qualifying it, and a pointer to the action's own two-word block, which is
   the party slot row's own pair rather than a copy. */
extern void GetSlotChosenAction(
    int party_slot, unsigned int context, int* action, int* detail, void* unused,
    const W8ActionDetailBlock** detail_block);
extern int GetTargetNeededForSpellHostile(int spell_id);                 /* 0x005011C0 */
extern unsigned char GetItemSpell(const W8ItemInstance* item);           /* 0x00520880 */
extern unsigned char IsTargetSourceInRangeOfGroup(
    const W8TargetSource* source, W8MonsterGroup* group, int context);   /* 0x00537780 */
extern unsigned char CanReachTarget(
    int party_slot, int kind, W8MonsterInfo* monster_info, int context, int arg_5);

/* Replace a monster's current combat target with one monster id. The target
   block is cleared as a whole before its four discriminating fields are
   established, matching the other target builders in this unit. */
// FUNCTION: WIZ8 0x0053A2C0
void Function53A2C0(W8MonsterInfo* monster_info, int location_id)
{
    W8CombatSlot* target = &monster_info->combat_slot_2ba;

    memset(target, 0, sizeof(*target));
    target->iType = 0;
    target->iMonsterID = BAD_INDEX;
    target->iChar = BAD_INDEX;
    target->iGroupID = BAD_INDEX;
    target->iMonsterID = location_id;
    target->iType = W8_TARGET_KIND_MONSTER;
}

/* Probe whether a monster's current combat target is one of the kinds its
   chosen hostile spell accepts. The caller only needs the validator's side
   effects, so this wrapper discards its answer. */
// FUNCTION: WIZ8 0x0053A300
void Function53A300(W8MonsterInfo* monster_info, int spell_id)
{
    TargetMatchesNeeded(
        &monster_info->combat_slot_2ba, GetTargetNeededForSpellHostile(spell_id));
}

/* Map the current screen state to the targeting context used by this path.
   The address-qualified name preserves the still-unidentified original name;
   the screen ids and returned context numbers are direct switch evidence. */
// FUNCTION: WIZ8 0x0053A3D0
unsigned int Function53A3D0(int alternate)
{
    switch (g_highlight_suppressed_00683fe7) {
    case 1:
    case 6:
    case 7:
        return (alternate != 0) + 3;
    case 2:
        return alternate != 0;
    case 3:
        return 2;
    case 4:
        return 12;
    case 5:
        return 11 - (alternate != 0);
    default:
        return 0xffffffff;
    }
}

/* Whether the pending spell in one party row needs an explicit target. */
// FUNCTION: WIZ8 0x0053A700
unsigned char Function53A700(int party_slot)
{
    switch (GetSpellTargetType(g_party_slot_rows[party_slot].spell_id, 0)) {
    case 0:
    case 2:
    case 7:
    case 10:
        return 0;
    case 3:
        if (g_camp_open_00683f9b != 0) {
            return 0;
        }
        return g_targeting_flag_00685116 == 0;
    default:
        return 1;
    }
}

/* Return the spell-like id carried by a chosen action: the fixed attack id,
   a spell's detail word, or the spell attached to an item use. */
// FUNCTION: WIZ8 0x0053A8D0
unsigned int Function53A8D0(int party_slot, unsigned int context)
{
    int action;
    int detail;
    const W8ActionDetailBlock* detail_block;

    GetSlotChosenAction(
        party_slot, context, &action, &detail, 0, &detail_block);
    if (action == 2) {
        return 0x77;
    }
    if (action == 7) {
        return detail;
    }
    if (action == 8) {
        return GetItemSpell(detail_block->item_use.item);
    }
    return 0;
}

extern void Function4ADD30(int enabled);
extern void Function56AA30(void);
extern void Function56AAB0(void);
extern void Function53B660(const srVector3T<float>* position, srVector3T<float>* target, int enabled);
// GLOBAL: WIZ8 0x0068406F
srVector3T<float> g_target_position_0068406f;
// GLOBAL: WIZ8 0x0068407F
srVector3T<float> g_target_position_0068407f;

/* Select the cursor and renderer-side targeting mode for one targeting state,
   then clear the cached world point so the following refresh recomputes it. */
// FUNCTION: WIZ8 0x0053A320
void Function53A320(int state)
{
    int cursor;

    g_highlight_suppressed_00683fe7 = state;
    switch (state) {
    case 1:
    case 6:
    case 7:
        cursor = 3;
        break;
    case 2:
        cursor = 0;
        break;
    case 3:
        cursor = 2;
        break;
    case 4:
        cursor = 12;
        break;
    case 5:
        cursor = 11;
        break;
    default:
        cursor = -1;
        break;
    }
    if (cursor != g_cursor_state_00683fdb) {
        SetTargetCursor(cursor);
    }
    g_target_position_0068407f.x = 0.0f;
    g_target_position_0068407f.y = 0.0f;
    g_target_position_0068407f.z = 0.0f;
    RequestRefreshPartyState();
    if (state == 4) {
        Function4ADD30(1);
        Function56AA30();
    }
    else {
        Function4ADD30(0);
        if (g_screen_state_0068ec78.id == W8_SCREEN_MAIN_GAME) {
            Function56AAB0();
        }
    }
}

/* Remove one party slot's highlight bit from every live monster that carries
   it, notifying the render-side highlight owner for each changed monster. */
// FUNCTION: WIZ8 0x0053AEB0
void Function53AEB0(unsigned int party_slot)
{
    unsigned int index;

    for (index = 0; index < PLLength(g_active_monster_list_00683fad); ++index) {
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(index);
        W8Monster* monster = monster_info->monster;

        if (monster_info->flag_14 != 0 && monster != 0) {
            unsigned char flags = MonsterGetRuntimeFlag5BC(monster);
            unsigned char bit = static_cast<unsigned char>(1 << (party_slot & 31));

            if ((flags & bit) != 0) {
                MonsterSetRuntimeFlag5BC(monster, static_cast<unsigned char>(flags & ~bit));
                NotifyMonsterHighlight(party_slot, monster_info->location_id, 0);
            }
        }
    }
}

/* Clear the target marker and request the party-display refresh that consumes
   the change. */
// FUNCTION: WIZ8 0x0053B160
void Function53B160(void)
{
    g_target_marker_00684073 = 0;
    RequestRefreshPartyState();
}

/* Recompute the target point and hand it to the marker only when it differs
   from the cached three-float position. */
// FUNCTION: WIZ8 0x0053B170
void Function53B170(void)
{
    srVector3T<float> position;

    Function492500(&position);
    if (position.x != g_target_position_0068407f.x ||
        position.y != g_target_position_0068407f.y ||
        position.z != g_target_position_0068407f.z) {
        g_target_position_0068407f = position;
        Function53B660(&position, &g_target_position_0068406f, 1);
    }
}

/* A party slot can participate only while occupied, alive, and below the
   terminal character-state threshold. */
// FUNCTION: WIZ8 0x0053C270
unsigned char Function53C270(int party_slot)
{
    return g_party_slot_rows[party_slot].occupied != 0 &&
           g_party_characters[party_slot].hp_current != 0 &&
           g_party_characters[party_slot].unknown_0b01 < 0x12;
}

/* Validate a targeting context a second time, after resolving "current". The
   inner resolution has an assertion of its own, so a context that gets this far
   has already been checked once; this one guards the caller's own use of the
   answer, and the two report different lines. */
// FUNCTION: WIZ8 0x0053ba20
int GetValidatedTargetingContext(int party_slot, unsigned int context)
{
    if (context == W8_TARGETING_CONTEXT_CURRENT) {
        context = ResolveTargetingContext(party_slot, W8_TARGETING_CONTEXT_CURRENT);
    }
    switch (context) {
    case W8_TARGETING_CONTEXT_OUT_OF_COMBAT:
        return W8_TARGETING_CONTEXT_OUT_OF_COMBAT;
    case W8_TARGETING_CONTEXT_IN_COMBAT:
        return W8_TARGETING_CONTEXT_IN_COMBAT;
    case W8_TARGETING_CONTEXT_SHARED:
        return W8_TARGETING_CONTEXT_SHARED;
    case W8_TARGETING_CONTEXT_SPELL:
        return W8_TARGETING_CONTEXT_SPELL;
    case W8_TARGETING_CONTEXT_ITEM:
        return W8_TARGETING_CONTEXT_ITEM;
    case W8_TARGETING_CONTEXT_FIVE:
        return W8_TARGETING_CONTEXT_FIVE;
    case W8_TARGETING_CONTEXT_DIALOGUE:
        return W8_TARGETING_CONTEXT_DIALOGUE;
    default:
        srAssertFail("FALSE", TARGETING_CPP, 0xc87, 0);
    }
    return W8_TARGETING_CONTEXT_IN_COMBAT;
}

/* Whether a party slot could aim its chosen action at one whole monster group.
   An untargetable group is refused before anything else is worked out. What
   the action needs then decides how the question is asked: a group-wide need
   is a range test against the group, and a single-monster need is satisfied by
   any one member the slot can reach.

   The source block is built here rather than passed in, so this always asks on
   the character's own behalf. */
// FUNCTION: WIZ8 0x00536d60
unsigned char CanTargetMonsterGroup(int party_slot, W8MonsterGroup* group)
{
    W8TargetSource source;
    int action;
    int detail;
    const W8ActionDetailBlock* detail_block;
    char needed;
    unsigned int index;
    int reachable;

    if (GetMonsterGroupRecord(group)->untargetable_24a != 0) {
        return 0;
    }

    if (ResolveTargetingContext(party_slot, W8_TARGETING_CONTEXT_CURRENT) == 0) {
        needed = 0;
    }
    else {
        GetSlotChosenAction(
            party_slot, W8_TARGETING_CONTEXT_CURRENT, &action, &detail, 0, &detail_block);
        needed = GetTargetNeededForAction(action, detail, detail_block);
    }

    SetTargetSourceToCharacter(party_slot, &source);

    if (needed == 5) {
        return IsTargetSourceInRangeOfGroup(&source, group, 6) != 0;
    }
    if (needed != 2 && needed != 1) {
        return 0;
    }

    reachable = 0;
    for (index = 0; index < ILLength(group->monsters); ++index) {
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(
            MonsterGetIndexByLocationID(0x37a, TARGETING_CPP, IListGetAt(group->monsters, index), 1));

        if (CanReachTarget(party_slot, 2, monster_info, 6, 0)) {
            ++reachable;
        }
    }
    return reachable != 0;
}
/* 0x00649F1C: the camp screen, which pins targeting to the one monster it is
   showing. */
extern unsigned char* g_camp_screen_00649f1c;
extern unsigned char IsSlotInRangeOfGroup(
    int party_slot, int group_id, int context, int arg_4);               /* 0x00519920 */

/* Whether a party slot's chosen action can be aimed at one monster. The
   monster has to be live, standing and reachable; while the camp screen is up
   only the monster it is showing can be aimed at at all.

   What the action needs decides the rest: a group need is answered against the
   monster's group, a single-monster need only when the caller allows one, and
   an action needing nothing is refused outright in combat. The database's
   untargetable flag is checked last rather than first, so an action needing
   nothing still reaches a monster carrying it. */
// FUNCTION: WIZ8 0x00536ad0
unsigned char CanTargetMonster(
    int party_slot, int location_id, int allow_single_target, int reason)
{
    W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(
        MonsterGetIndexByLocationID(0x1ce, TARGETING_CPP, location_id, 1));
    W8MonsterRecord* record;
    int action;
    int detail;
    const W8ActionDetailBlock* detail_block;
    char needed;

    if (monster_info->flag_14 == 0) {
        return 0;
    }
    if (monster_info->hp_current == 0) {
        return 0;
    }
    if (monster_info->condition_turns[W8_CONDITION_REACHABLE_WHEN_DOWN] != 0) {
        return 0;
    }
    if (g_camp_open_00683f9b != 0 &&
        *(const int*)(g_camp_screen_00649f1c + 0xf8) != location_id) {
        return 0;
    }

    if (ResolveTargetingContext(party_slot, W8_TARGETING_CONTEXT_CURRENT) == 0) {
        needed = 0;
        if (g_in_combat_00683f94 != 0) {
            return 0;
        }
    }
    else {
        GetSlotChosenAction(
            party_slot, W8_TARGETING_CONTEXT_CURRENT, &action, &detail, 0, &detail_block);
        needed = GetTargetNeededForAction(action, detail, detail_block);

        if (needed != 2 && needed != 5) {
            if (needed == 1) {
                if (allow_single_target == 0) {
                    return 0;
                }
            }
            else {
                if (needed != 0) {
                    return 0;
                }
                if (g_in_combat_00683f94 != 0) {
                    return 0;
                }
            }
        }
    }

    record = GetMonsterDataForInfo(monster_info);
    if (record->untargetable_24a != 0 && needed != 0) {
        return 0;
    }
    if (needed == 5) {
        return IsSlotInRangeOfGroup(party_slot, monster_info->monster_group_id, 6, reason) != 0;
    }
    return CanReachTarget(party_slot, 2, monster_info, 6, reason) != 0;
}

extern unsigned char CanTargetPartySlot(int party_slot, const W8CombatSlot* target);
/* 0x00545C20 */
extern unsigned char SpellHasAnyValidTarget(
    int party_slot, int spell_id, unsigned char normalize);              /* 0x0053D010 */

/* Whether a party slot's chosen action has anything at all to aim at. Each
   action asks its own question: an attack looks for one targetable monster, a
   fifth-kind action looks at the other party members first and then at the
   monsters, and casting or using an item hands the question to the spell.

   An action this does not know about answers yes, so the check narrows rather
   than gates - only the actions with a way of being impossible can fail it.
   The two dialogue-driven actions answer yes as well until the dialogue has
   settled, since until then there is no spell or item to ask about. */
// FUNCTION: WIZ8 0x0053cdf0
unsigned char SlotHasAnyValidTarget(int party_slot)
{
    int action;
    int detail;
    const W8ActionDetailBlock* detail_block;
    unsigned char unused[4];
    W8CombatSlot target;
    unsigned int index;
    unsigned int other_slot;

    GetSlotChosenAction(
        party_slot, W8_TARGETING_CONTEXT_CURRENT, &action, &detail, unused, &detail_block);

    switch (action) {
    case 0:
    case 1:
        for (index = 0; index < PLLength(g_active_monster_list_00683fad); ++index) {
            if (CanTargetMonster(
                    party_slot, MonsterGetScriptPartByLocationIndex(index)->location_id, 0, 0)) {
                return 1;
            }
        }
        return 0;

    case 5:
        for (other_slot = 0; other_slot < 8; ++other_slot) {
            if (other_slot == (unsigned int)party_slot) {
                continue;
            }
            memset(&target, 0, sizeof(target));
            target.iMonsterID = BAD_INDEX;
            target.iGroupID = BAD_INDEX;
            target.iType = W8_TARGET_KIND_CHARACTER;
            target.iChar = other_slot;
            if (CanTargetPartySlot(party_slot, &target)) {
                return 1;
            }
        }
        for (index = 0; index < PLLength(g_active_monster_list_00683fad); ++index) {
            memset(&target, 0, sizeof(target));
            target.iChar = BAD_INDEX;
            target.iGroupID = BAD_INDEX;
            target.iType = W8_TARGET_KIND_MONSTER;
            target.iMonsterID = MonsterGetScriptPartByLocationIndex(index)->location_id;
            if (CanTargetPartySlot(party_slot, &target)) {
                return 1;
            }
        }
        return 0;

    case 7:
        if (g_level_block->selection_settled != 0) {
            return SpellHasAnyValidTarget(party_slot, detail, 0);
        }
        break;

    case 8:
        if (g_level_block->selection_settled != 0) {
            const W8ItemInstance* item = detail_block->item_use.item;

            if (item == 0) {
                return 0;
            }
            return SpellHasAnyValidTarget(
                party_slot, g_item_records[item->item_id].spell_id,
                ItemClassNormalizesTarget(&g_item_records[item->item_id]));
        }
        break;
    }
    return 1;
}

/* 0x00683FB1: every monster group in the level. */

/* The spell target kinds this has an opinion about. Everything else is
   answered yes outright, so the question only ever narrows. */
enum {
    W8_SPELL_TARGET_ONE_MONSTER = 2,
    W8_SPELL_TARGET_MONSTER_GROUP = 5,
    W8_SPELL_TARGET_DEAD_CHARACTER = 7
};

/* Whether a spell has anything to be cast at. What it needs decides the sweep:
   one monster looks for a targetable one, a group looks for a targetable
   group, and the raise-the-dead kind looks for a party member who is down but
   still reachable - occupied, out of hit points, carrying the eighteenth
   condition and not the nineteenth, which is the same pair IsTargetStillPresent
   reads for its indirect character kind.

   Out of combat the one-monster kind answers yes without looking, since
   anything in the level can be walked up to. */
// FUNCTION: WIZ8 0x0053d010
unsigned char SpellHasAnyValidTarget(int party_slot, int spell_id, unsigned char normalize)
{
    unsigned int index;

    switch (GetTargetNeededForSpellFriendly(spell_id, normalize, 6)) {
    case W8_SPELL_TARGET_ONE_MONSTER:
        if (g_targeting_flag_00685116 != 0) {
            return 1;
        }
        for (index = 0; index < PLLength(g_active_monster_list_00683fad); ++index) {
            if (CanTargetMonster(
                    party_slot, MonsterGetScriptPartByLocationIndex(index)->location_id, 0, 0)) {
                return 1;
            }
        }
        return 0;

    case W8_SPELL_TARGET_MONSTER_GROUP:
        for (index = 0; index < PLLength(g_monster_group_list_00683fb1); ++index) {
            W8MonsterGroup* group = GetMonsterGroupByListIndex(index);

            if (group->flag_28 != 0 && CanTargetMonsterGroup(party_slot, group)) {
                return 1;
            }
        }
        return 0;

    case W8_SPELL_TARGET_DEAD_CHARACTER:
        for (index = 0; index < 8; ++index) {
            if (g_party_slot_rows[index].occupied != 0 &&
                g_party_characters[index].hp_current == 0 &&
                g_party_characters[index].condition_turns[W8_CONDITION_REACHABLE_WHEN_DOWN] != 0 &&
                g_party_characters[index].condition_turns[W8_CONDITION_BEYOND_REACH] == 0) {
                return 1;
            }
        }
        return 0;

    default:
        return 1;
    }
}

extern void GetPartyPosition(srVector3T<float>* position);                      /* 0x00421070 */
extern float AngleFromPartyTo(const srVector3T<float>* from, const srVector3T<float>* to);
/* 0x004BE420 */
extern float NormalizeAngle(float radians);
extern int CompareSignedAscending(const void* left, const void* right);  /* 0x00517A30 */
extern W8Monster* GetMonsterByLocationID(int location_id);
extern void AimAtTarget(int actor, const W8CombatSlot* target, int context);
/* 0x005387F0 */
extern void StartBreathCycle(int party_slot, int arg_2);                 /* 0x0052FE80 */
extern void NoteTargetChosen(const W8TargetSource* source, const W8CombatSlot* target);
/* 0x004ECC80 */

/* One candidate in the angle sort: the screen angle to the monster and the
   monster itself. The angle leads so that the ordinary signed comparison sorts
   on it. */
typedef struct W8GroupMemberByAngle {
    int angle;                           /* 0x00 */
    int location_id;                     /* 0x04 */
} W8GroupMemberByAngle;                  /* 0x08 */

/* Step to the next member of a group, going round the party rather than
   through the list: the candidates are sorted by the angle from the party to
   each of them, and the one after whichever is currently picked is taken,
   wrapping at the end. With nothing picked yet the leftmost is taken.

   The angle is what makes this feel like cycling across the screen rather than
   jumping about, and it is why the sort buffer holds a pair per candidate
   rather than just the ids. */
// FUNCTION: WIZ8 0x005383e0
int SelectNextGroupMemberByAngle(const W8GrowableVector<int>* candidates, int current)
{
    unsigned int count = (unsigned int)candidates->GetCount();
    W8GroupMemberByAngle* sorted;
    srVector3T<float> party;
    unsigned int index;
    int result;

    if (count == 0) {
        return BAD_INDEX;
    }
    GetPartyPosition(&party);

    sorted = (W8GroupMemberByAngle*)malloc(count * sizeof(W8GroupMemberByAngle));
    if (sorted == 0) {
        srAssertFail("pSortBuffer != NULL", TARGETING_CPP, 0x499, 0);
    }

    for (index = 0; index < count; ++index) {
        int location_id = *((W8GrowableVector<int>*)candidates)->GetAt((int)index);
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(
            MonsterGetIndexByLocationID(0x4a1, TARGETING_CPP, location_id, 1));
        srVector3T<float> position = monster_info->monster->GetPosition();

        sorted[index].location_id = location_id;
        sorted[index].angle = (int)NormalizeAngle(AngleFromPartyTo(&party, &position));
    }
    qsort(sorted, count, sizeof(W8GroupMemberByAngle), CompareSignedAscending);

    result = sorted[0].location_id;
    if (current != BAD_INDEX) {
        for (index = 0; index < count; ++index) {
            if (sorted[index].location_id == current) {
                result = sorted[index + 1 < count ? index + 1 : 0].location_id;
                break;
            }
        }
    }
    free(sorted);
    return result;
}

/* Which member of a group the party should pick out next. Every member the
   slot could aim at becomes a candidate - including the single-target case,
   which is what the third argument allows - and the group's own record of
   where it got to decides which of them comes next. */
// FUNCTION: WIZ8 0x00538280
int PickNextTargetableGroupMember(int party_slot, W8MonsterGroup* group)
{
    W8GrowableVector<int> targetable;
    unsigned int index;

    for (index = 0; index < ILLength(group->monsters); ++index) {
        int location_id = IListGetAt(group->monsters, index);

        MonsterGetScriptPartByLocationIndex(
            MonsterGetIndexByLocationID(0x46e, TARGETING_CPP, location_id, 1));
        if (CanTargetMonster(party_slot, location_id, 1, 0)) {
            targetable.Add(location_id);
        }
    }
    return SelectNextGroupMemberByAngle(&targetable, group->highlighted_member);
}

/* Point one party slot at a monster group, or at one monster inside it. An
   action that wants the whole group is aimed at the group and nothing else
   happens; anything else steps to the next targetable member and moves the
   highlight to it.

   The step is taken three times over: once to find where to go, once - before
   the group's record is updated - to find where the highlight currently is so
   it can be put out, and once after to find it again so it can be lit. That
   the same call answers differently each time is exactly what the group's own
   record of where it got to is for. */
// FUNCTION: WIZ8 0x00537b00
void AimAtMonsterGroupMember(int party_slot, W8MonsterGroup* group)
{
    W8CombatSlot target;
    W8TargetSource source;
    int action;
    int detail;
    const W8ActionDetailBlock* detail_block;
    int picked;
    int previous;

    if (ResolveTargetingContext(party_slot, W8_TARGETING_CONTEXT_CURRENT) != 0) {
        GetSlotChosenAction(
            party_slot, W8_TARGETING_CONTEXT_CURRENT, &action, &detail, 0, &detail_block);
        if (GetTargetNeededForAction(action, detail, detail_block) == 5) {
            memset(&target, 0, sizeof(target));
            target.iChar = BAD_INDEX;
            target.iMonsterID = BAD_INDEX;
            target.iType = W8_TARGET_KIND_GROUP;
            target.iGroupID = group->group_id;
            AimAtTarget(party_slot, &target, 6);
            StartBreathCycle(party_slot, 0);
            SetTargetSourceToCharacter(party_slot, &source);
            NoteTargetChosen(
                &source, GetTargetBlockForContext(party_slot, W8_TARGETING_CONTEXT_CURRENT));
            return;
        }
    }

    picked = PickNextTargetableGroupMember(party_slot, group);
    if (picked == BAD_INDEX) {
        return;
    }

    memset(&target, 0, sizeof(target));
    target.iChar = BAD_INDEX;
    target.iGroupID = BAD_INDEX;
    target.iType = W8_TARGET_KIND_MONSTER;
    target.iMonsterID = picked;
    AimAtTarget(party_slot, &target, 6);

    previous = PickNextTargetableGroupMember(party_slot, group);
    if (previous != BAD_INDEX) {
        SetMonsterHighlightColour(GetMonsterByLocationID(previous), 0.0f, 0.0f, 0.0f, 0.0f);
    }

    group->highlighted_member = picked;

    picked = PickNextTargetableGroupMember(party_slot, group);
    if (picked != BAD_INDEX) {
        SetMonsterHighlightColour(GetMonsterByLocationID(picked), 1.0f, 1.0f, 1.0f, 1.0f);
    }
    g_level_block->pick_changed_154 = 1;

    StartBreathCycle(party_slot, 0);
    SetTargetSourceToCharacter(party_slot, &source);
    NoteTargetChosen(
        &source, GetTargetBlockForContext(party_slot, W8_TARGETING_CONTEXT_CURRENT));
}

/* 0x006840B7: the group the party currently has picked out, by id, and -1 when
   none. It is where the sweep below starts from and wraps back to. */
extern int g_picked_group_006840b7;

/* Which monster group the party should pick out next. The sweep starts at the
   one after whichever is currently picked and goes round until it comes back
   to where it began, so the same group is only offered again when nothing else
   will do. A picked group that has since gone away resets the starting point
   rather than ending the sweep. */
// FUNCTION: WIZ8 0x00537ed0
int PickNextTargetableGroup(int party_slot)
{
    unsigned int count = PLLength(g_monster_group_list_00683fb1);
    unsigned int start;
    unsigned int index;

    if (count == 0) {
        return BAD_INDEX;
    }

    if (g_picked_group_006840b7 == BAD_INDEX) {
        start = 0;
    }
    else {
        start = GetMonsterGroupIndexByID(0x414, TARGETING_CPP, g_picked_group_006840b7, 0);
        if (start == 0xffffffff) {
            start = 0;
            g_picked_group_006840b7 = BAD_INDEX;
        }
        else {
            ++start;
            if (start == PLLength(g_monster_group_list_00683fb1)) {
                start = 0;
            }
        }
    }

    index = start;
    do {
        W8MonsterGroup* group = GetMonsterGroupByListIndex(index);

        if (CanTargetMonsterGroup(party_slot, group)) {
            return group->group_id;
        }
        ++index;
        if (index == PLLength(g_monster_group_list_00683fb1)) {
            index = 0;
        }
    } while (index != start);

    return BAD_INDEX;
}
