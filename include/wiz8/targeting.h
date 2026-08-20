#ifndef WIZ8_TARGETING_H
#define WIZ8_TARGETING_H

#include "wiz8/geometry.h"

struct W8ItemInstance;

#pragma pack(push, 1)
/* Local Code\Targeting.cpp. Field names and the BAD_INDEX sentinel come from
   the canonical assertions at lines 3299, 3307, 3320 and 3328; offsets come
   from the asserting bodies. iType 1 selects the character, 2 the monster, and
   3 either, which is why the type-3 path additionally requires a backfire or
   reflection flag. */
/* The block a spell or attack records its target in. Only the leading three
   fields are established; the two resets that build an empty one agree on the
   extent and on which of them start at -1 rather than zero. */
/* Where a spell or an attack comes from. Targeting.cpp's assertions name every
   field here - iType, iChar, iMonsterID, fBackfire and fReflection - and the
   three source kinds are a character, a monster and a point in the world; a
   backfired or reflected spell keeps the original's iChar or iMonsterID while
   reading as a point, which is what the two flags distinguish.

   This was modelled twice before, once from the assertions and once from
   SpellBackfires' stack frame, and they are one struct. */
typedef struct W8TargetSource {
    int iType;                            /* 0x00 */
    int iChar;                            /* 0x04, -1 when empty */
    int iMonsterID;                       /* 0x08, -1 when empty */
    /* 0x0c: the world point, for a source that is a place rather than
       somebody. Note that this is not where the combat slot keeps its own
       point - that one has a group id at 0x0c and the point at 0x10 - so the
       two blocks are related but not the same shape. */
    srVector3T<float> point;
    unsigned char unknown_18[3];
    unsigned char fReflection;            /* 0x1b */
    unsigned char fBackfire;              /* 0x1c */
    unsigned char unknown_1d[0x17];
} W8TargetSource;                         /* 0x34 */

/* The shorter form a combatant carries inline, with one more field reset to
   -1 and no room for the tail. */
typedef struct W8CombatSlot {
    /* The four ids are named by the assertions that bound each of them -
       pTarget->iChar, pTarget->iMonsterID, pTarget->iGroupID and
       pTarget->pPCItem - and they are the same four the source block carries
       under the same names, one per target kind. */
    int iType;                            /* 0x00 */
    int iChar;                            /* 0x04, -1 when empty */
    int iMonsterID;                       /* 0x08, -1 when empty */
    int iGroupID;                         /* 0x0c, -1 when empty */
    /* Place targets store the ordinary world vector here. Other target kinds
       reuse byte +0x19 as the "name known" flag; that overlapping byte use
       does not establish a second source type or union boundary. */
    srVector3T<float> point;               /* 0x10 */
    /* 0x1c: the item aimed at, for the one kind that aims at one. */
    W8ItemInstance* pPCItem;
} W8CombatSlot;                           /* 0x20 */
/* The two-word block an action carries beside itself. A spell's holds the
   power level and a spare word; an item use's holds the use kind and the item.
   It is the party slot row's own pair in both cases rather than a copy, which
   is why every reader takes a pointer to it. */
typedef union W8ActionDetailBlock {
    struct {
        int power_level;
        int unused;
    } spell;
    struct {
        int kind;
        W8ItemInstance* item;
    } item_use;
} W8ActionDetailBlock;                    /* 0x08 */

/* The targeting contexts. Six of them name a block the slot carries; the
   seventh, "current", is not a context at all but the request to work out
   which of the others applies right now. */
enum {
    W8_TARGETING_CONTEXT_OUT_OF_COMBAT = 0,
    W8_TARGETING_CONTEXT_IN_COMBAT = 1,
    W8_TARGETING_CONTEXT_SHARED = 2,
    W8_TARGETING_CONTEXT_SPELL = 3,
    W8_TARGETING_CONTEXT_ITEM = 4,
    W8_TARGETING_CONTEXT_FIVE = 5,
    W8_TARGETING_CONTEXT_CURRENT = 6,
    W8_TARGETING_CONTEXT_DIALOGUE = 7
};

/* The target kinds a combat slot's leading field takes. The four that name
   something put it in their own field, which is what pairs each kind with the
   field the aiming wrappers fill in. */
enum {
    W8_TARGET_KIND_CHARACTER = 1,
    W8_TARGET_KIND_PARTY = 2,
    W8_TARGET_KIND_MONSTER = 3,
    W8_TARGET_KIND_GROUP = 4,
    W8_TARGET_KIND_PLACE = 6,
    W8_TARGET_KIND_ITEM = 7,
    W8_TARGET_KIND_CHARACTER_INDIRECT = 9
};
#pragma pack(pop)

unsigned char TargetSourceIsCharacter(const W8TargetSource* source, int allow_indirect);
unsigned char TargetSourceIsMonster(const W8TargetSource* source, int allow_indirect);

extern "C" {
extern int g_target_state_6840b3;
extern int g_picked_group_006840b7;
void ResetTargetingState(void);
}

#endif
