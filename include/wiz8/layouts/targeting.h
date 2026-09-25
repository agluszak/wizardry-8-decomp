#ifndef WIZ8_LAYOUTS_TARGETING_H
#define WIZ8_LAYOUTS_TARGETING_H

#include "surrender/srMath.h"

struct W8ItemInstance;
struct W8MonsterInfo;
struct W8MonsterGroup;

#pragma pack(push, 1)
/* Local Code\Targeting.cpp. Assertions name iType, iChar, iMonsterID,
   fBackfire and fReflection and establish the field offsets; SpellBackfires'
   stack frame independently agrees with the same source-record layout. */
/* The source-kind domain. Zero is the empty source, one a character, two a
   monster, and three a source that keeps the original's character or monster
   id while reading as a point - which is what a backfire or a reflection
   produces, and what the two flags distinguish. This is a different domain
   from W8TargetKind even though both name a leading field. */
enum W8TargetSourceKind {
    W8_TARGET_SOURCE_NONE = 0,
    W8_TARGET_SOURCE_CHARACTER = 1,
    W8_TARGET_SOURCE_MONSTER = 2,
    W8_TARGET_SOURCE_INDIRECT = 3,
    W8_TARGET_SOURCE_COUNT = 4 /* SOURCE_TYPE_COUNT in PrepareSpellTarget's assert */
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
    /* 0x18: system-driven cast (retaliation/AI/trap): skips the backfire
       check and counts as already reported. */
    unsigned char auto_cast_18;
    /* 0x19: the name-known flag SpellTargetString reads. */
    unsigned char name_known_19;
    /* 0x1a: the cast's aim was already resolved; MonsterCanAimSpell retargeting
       is skipped. */
    bool aim_resolved_1a;
    bool fReflection; /* 0x1b */
    bool fBackfire;   /* 0x1c */
    /* 0x1d: the cast's source was resolved to a point, not a creature;
       missile/spell paths then skip the monster's spell vertex. */
    unsigned char point_source_1d;
    /* 0x1e: the attack's target ended up different from the one the slot row
       aimed at - a fumble reroll or a guardian interception replaced it. */
    unsigned char target_diverted;
    /* 0x1f: the precomputed cast difficulty item/tracked sources carry; the
       spell engine reads it back when name_known_19 skips recomputation. */
    unsigned char spell_difficulty_1f;
    unsigned char unknown_20;
    /* 0x21: set on sources the item-spell path builds. */
    unsigned char item_cast_21;
    unsigned char unknown_22[0x12];
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
    W8_TARGET_KIND_CHARACTER_INDIRECT = 9,
    W8_TARGET_KIND_COUNT = 10 /* TARGET_TYPE_COUNT in PrepareSpellTarget's assert */
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

/* The combat actions a party slot row's action_03d and the level block's
   selection_kind carry; ChooseCombatAction picks one and ChooseAction applies
   it. The names are the submenu help captions (string ids 0x52-0x5f): a menu-0
   entry's caption is the action its row selects. 10 and 11 are the two party
   move kinds, which ApplyPartyCombatAction routes away from the action
   record. */
enum W8ActionKind {
    W8_ACTION_ATTACK = 0,
    W8_ACTION_BERSERK = 1,
    W8_ACTION_BREATHE = 2,
    W8_ACTION_TURN_UNDEAD = 3,
    W8_ACTION_DEFEND = 4,
    W8_ACTION_PROTECT = 5,
    W8_ACTION_PRAY = 6,
    W8_ACTION_CAST_SPELL = 7,
    W8_ACTION_USE_ITEM = 8,
    W8_ACTION_EQUIP = 9,
    W8_ACTION_WALK = 10,
    W8_ACTION_RUN = 11
};

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

#endif
