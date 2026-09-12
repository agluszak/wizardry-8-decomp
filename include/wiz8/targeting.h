#ifndef WIZ8_TARGETING_H
#define WIZ8_TARGETING_H

struct W8CombatSlot;
struct W8MonsterInfo;
struct W8MonsterGroup;
template <class T> class W8GrowableVector;
void ResetCombatSlot(W8CombatSlot* slot);

#include "wiz8/geometry.h"

struct W8ItemInstance;

#pragma pack(push, 1)
/* Local Code\Targeting.cpp. Field names and the BAD_INDEX sentinel come from
   the canonical assertions at lines 3299, 3307, 3320 and 3328; offsets come
   from the asserting bodies. iType 1 selects the character, 2 the monster, and
   3 either, which is why the type-3 path additionally requires a backfire or
   reflection flag. */
/* Where a spell or an attack comes from. Targeting.cpp's assertions name every
   field here - iType, iChar, iMonsterID, fBackfire and fReflection - and the
   three source kinds are a character, a monster and a point in the world; a
   backfired or reflected spell keeps the original's iChar or iMonsterID while
   reading as a point, which is what the two flags distinguish.

   This was modelled twice before, once from the assertions and once from
   SpellBackfires' stack frame, and they are one struct. */
/* The source-kind domain. Zero is the empty source, one a character, two a
   monster, and three a source that keeps the original's character or monster
   id while reading as a point - which is what a backfire or a reflection
   produces, and what the two flags distinguish. This is a different domain
   from W8TargetKind even though both name a leading field. */
enum W8TargetSourceKind {
    W8_TARGET_SOURCE_NONE = 0,
    W8_TARGET_SOURCE_CHARACTER = 1,
    W8_TARGET_SOURCE_MONSTER = 2,
    W8_TARGET_SOURCE_INDIRECT = 3
};

struct W8TargetSource {
    W8TargetSourceKind iType; /* 0x00 */
    int iChar;                /* 0x04, -1 when empty */
    int iMonsterID;           /* 0x08, -1 when empty */
    /* 0x0c: the world point, for a source that is a place rather than
       somebody. Note that this is not where the combat slot keeps its own
       point - that one has a group id at 0x0c and the point at 0x10 - so the
       two blocks are related but not the same shape. */
    srVector3T<float> point;
    unsigned char unknown_18[3]; /* 0x18..0x1a; [1] is the name-known flag
                                            SpellTargetString reads */
    unsigned char fReflection;   /* 0x1b */
    unsigned char fBackfire;     /* 0x1c */
    unsigned char unknown_1d[0x17];
}; /* 0x34 */

/* The target-kind domain a combat slot's leading field takes. The kinds that
   name something put it in their own field, which is what pairs each kind with
   the field the aiming wrappers fill in. Five and eight are switched on by
   consumers but have no agreed meaning yet, so they keep positional names the
   way W8_TARGETING_CONTEXT_FIVE does. This is a different domain from
   W8TargetSourceKind even though both name a leading field. */
enum W8TargetKind {
    W8_TARGET_KIND_NONE = 0,
    W8_TARGET_KIND_CHARACTER = 1,
    W8_TARGET_KIND_PARTY = 2,
    W8_TARGET_KIND_MONSTER = 3,
    W8_TARGET_KIND_GROUP = 4,
    W8_TARGET_KIND_FIVE = 5,
    W8_TARGET_KIND_PLACE = 6,
    W8_TARGET_KIND_ITEM = 7,
    W8_TARGET_KIND_EIGHT = 8,
    W8_TARGET_KIND_CHARACTER_INDIRECT = 9
};

/* The shorter form a combatant carries inline, with one more field reset to
   -1 and no room for the tail. */
struct W8CombatSlot {
    /* The four ids are named by the assertions that bound each of them -
       pTarget->iChar, pTarget->iMonsterID, pTarget->iGroupID and
       pTarget->pPCItem - and they are the same four the source block carries
       under the same names, one per target kind. */
    W8TargetKind iType; /* 0x00 */
    int iChar;          /* 0x04, -1 when empty */
    int iMonsterID;     /* 0x08, -1 when empty */
    int iGroupID;       /* 0x0c, -1 when empty */
    /* Place targets store the ordinary world vector here. Other target kinds
       reuse byte +0x19 as the "name known" flag; that overlapping byte use
       does not establish a second source type or union boundary. */
    srVector3T<float> point; /* 0x10 */
    /* 0x1c: the item aimed at, for the one kind that aims at one. */
    W8ItemInstance* pPCItem;
}; /* 0x20 */
/* The two-word block an action carries beside itself. A spell's holds the
   power level and a spare word; an item use's holds the use kind and the item.
   It is the party slot row's own pair in both cases rather than a copy, which
   is why every reader takes a pointer to it. */
union W8ActionDetailBlock {
    struct {
        int power_level;
        int unused;
    } spell;
    struct {
        int kind;
        W8ItemInstance* item;
    } item_use;
}; /* 0x08 */

/* The targeting contexts. Six of them name a block the slot carries; the
   seventh, "current", is not a context at all but the request to work out
   which of the others applies right now. Value five is unobserved and keeps
   its number rather than being given a meaning. */
enum W8TargetingContext {
    W8_TARGETING_CONTEXT_OUT_OF_COMBAT = 0,
    W8_TARGETING_CONTEXT_IN_COMBAT = 1,
    W8_TARGETING_CONTEXT_SHARED = 2,
    W8_TARGETING_CONTEXT_SPELL = 3,
    W8_TARGETING_CONTEXT_ITEM = 4,
    W8_TARGETING_CONTEXT_FIVE = 5,
    W8_TARGETING_CONTEXT_CURRENT = 6,
    W8_TARGETING_CONTEXT_DIALOGUE = 7
};
#pragma pack(pop)

/* Answer which of the real contexts applies right now. Kept after the enum so
   its result type is the domain rather than a plain int. */
W8TargetingContext GetCurrentTargetingContext(int party_slot);

unsigned char TargetSourceIsCharacter(const W8TargetSource* source, int allow_indirect);
unsigned char TargetSourceIsMonster(const W8TargetSource* source, int allow_indirect);

extern int g_target_state_6840b3;
extern int g_picked_group_006840b7;
/* ABS 0x0068408B: the shared combat slot the context-2 outputs name. */
extern W8CombatSlot g_shared_target_0068408b;
/* ABS 0x006840AB: one action-detail block following the shared slot; the
   eight bytes fit exactly before g_target_state_6840b3. Context 2 hands out
   both addresses and stores the selected use-item value in the block. */
extern W8ActionDetailBlock g_shared_action_detail_006840ab;
void ResetTargetingState(void);

unsigned char GetFactionFlag(char faction);
void AimByKind(int actor, W8TargetKind kind, W8TargetingContext context);
void SetMonsterCombatTarget(W8MonsterInfo* monster_info, int location_id);
unsigned char MonsterTargetMatchesSpell(W8MonsterInfo* monster_info, int spell_id);
W8CombatSlot* GetTargetBlockForContext(int party_slot, W8TargetingContext context);
void ClearTargetMarker(void);
void RefreshTargetMarker(void);
void RefreshAllPartyTargets0053BF80(void);
unsigned char Function536570(int party_slot, W8TargetingContext context, int arg);
/* 0x005387F0 */
void AimAtTarget(int actor, W8CombatSlot* target, W8TargetingContext context);
void RefreshCombatTargetHighlights(int party_slot, W8CombatSlot* target);
void HighlightSpellTargetsAtCachedPosition(void);
int Function53C990(int party_slot, int group_id, int arg);
void SetFactionFlag(char faction, unsigned char flag);
unsigned char ShowMonsterTargetMarker(W8MonsterInfo* monster_info);
bool IsSpellTargetStillValidIn(int party_slot, int spell_id, W8TargetingContext context);
void ClearTargetHighlights(int party_slot, const W8CombatSlot* target);
void ClearPartySlotMonsterHighlights(unsigned int party_slot);
void SetTargetToCharacter(int character_slot, W8TargetingContext context);

void SetTargetingMode(int state);
void UpdateAllMonsterHighlights(int party_slot, int location_id);
/* Cursor-table index for SetTargetCursor, including slots 10..12. Not a
   W8TargetingContext. Retail mangles the return as `H`. */
int GetTargetingCursorForState(int alternate);
bool ActionNeedsExplicitTarget(int party_slot);
unsigned int GetActionSpellLikeId(int party_slot, W8TargetingContext context);
void ResetTargetSource(W8TargetSource* source);
void SetTargetSourceToMonster(const W8MonsterInfo* monster_info, W8TargetSource* source);
W8TargetingContext ResolveTargetingContext(int party_slot, W8TargetingContext context);
char TargetMatchesNeeded(W8CombatSlot* target, int needed);
unsigned char SpellHasAnyValidTarget(int party_slot, int spell_id, unsigned char normalize);
void SetTargetToMonster(int monster_id, W8TargetingContext context);
void SetTargetToGroup(int group_id, W8TargetingContext context);

bool ClearMonsterCombatSlot(W8MonsterInfo* monster_info);

unsigned char Function53C630(W8CombatSlot* slot, int arg_2);
void AimCombatSlotAtParty(W8CombatSlot* combat_slot, int hostile);
void ApplyTarget(W8CombatSlot* target, W8TargetingContext context);
bool IsTargetStillPresent(const W8CombatSlot* target);
bool IsTargetSourceInRangeOfGroup(const W8TargetSource* source, W8MonsterGroup* group,
                                  W8TargetingContext context);
void NoteTargetChosen(const W8TargetSource* source, const W8CombatSlot* target);

unsigned char CanTargetMonster(int party_slot, int location_id, int allow_single_target,
                               int reason); /* 0x00536AD0 */
void ClearTargetingMode0053B050(int party_slot);
unsigned char Function536F60(int party_slot, int value,
                             W8TargetingContext context = W8_TARGETING_CONTEXT_OUT_OF_COMBAT);
void Function5398D0(void);
unsigned char Function53A1D0(void);
void Function53B1D0(void);
void RefreshSpellTargetHighlightsAtRange(void);
void PopulateTargetMarkerForCurrentAction(const srVector3T<float>* position,
                                          W8GrowableVector<int>* marker_vector, int enabled);
unsigned char TargetIsInPlay(int party_slot, int arg_2, int arg_3); /* 0x00536F60 */

void Function53AE00(void); /* 0x0053AE00 */
void Function53CD60(void); /* 0x0053CD60 */
/* Combat action-selection helpers used across the combat units. */
bool CanPartySlotParticipate(int party_slot);      /* 0x0053C270 */
W8TargetingContext Function53BC90(int party_slot); /* 0x0053BC90 */
W8TargetingContext GetValidatedTargetingContext(int party_slot,
                                                W8TargetingContext context); /* 0x0053BBD0 */
void SetTargetSourceToCharacter(int party_slot, W8TargetSource* source);     /* 0x0053A9D0 */

#endif
