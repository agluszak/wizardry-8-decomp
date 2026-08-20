#ifndef WIZ8_LOCAL_CODE_MONSTER_MANAGER_H
#define WIZ8_LOCAL_CODE_MONSTER_MANAGER_H

#include "surrender/srMath.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/character.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/targeting.h"
#include "wiz8/vector.h"

/* One eight-byte row per animation cycle at 0x0060EA08. The parser at
   0x004C2010 compares exactly prefix_length characters and then uses the same
   offset to read an optional numeric subcycle suffix. */
struct W8CycleNameRow {
    const char* name;
    int prefix_length;
};

extern W8CycleNameRow g_cycle_names[];

/* The object constructed at 0x006836B8 begins with eight of these records.
   Its generated element constructor and destructor at 0x004E6A30 and
   0x004E6A10 exist because the record owns the ordinary vector at +0x0D8.
   The reset at 0x0054B300 clears the record wholesale despite that non-trivial
   member; that source behavior does not turn the vector into a second layout
   projection. */
#pragma pack(push, 1)
struct W8MonsterManagerEntry {
    W8MonsterManagerEntry();
    ~W8MonsterManagerEntry();

    unsigned char field_000;
    int field_001;
    unsigned char unknown_005[0x6c];
    int field_071;
    int field_075;
    int field_079;
    int field_07d;
    int field_081;
    int field_085;
    int field_089;
    int field_08d;
    int field_091;
    int field_095;
    unsigned char field_099;
    unsigned char field_09a;
    unsigned char field_09b;
    unsigned char field_09c;
    unsigned char field_09d;
    unsigned char field_09e;
    int field_09f;
    int field_0a3;
    int field_0a7;
    unsigned char field_0ab;
    int field_0ac;
    int field_0b0;
    int field_0b4;
    int field_0b8;
    unsigned char field_0bc;
    unsigned char field_0bd;
    int field_0be;
    int field_0c2;
    int field_0c6;
    int field_0ca;
    unsigned char field_0ce;
    unsigned char field_0cf;
    unsigned char field_0d0;
    unsigned char field_0d1;
    int field_0d2;
    unsigned short field_0d6;
    W8GrowableVector<int> highlighted_monsters; /* 0x0d8 */
    unsigned char field_0e8;
    unsigned char unknown_0e9[0x2f];
};                                       /* 0x118 */

/* 0x004E6900 passes 0x006836B8 as this constructor's receiver. The constructor
   builds the entry array and the vector at +0x9B7; 0x0054AFD0 then clears the
   full 0x1A0A-byte object from that same address. That clear proves the
   remaining extent, but not any internal boundaries in the trailing storage. */
struct W8MonsterManagerState {
    W8MonsterManagerState();
    ~W8MonsterManagerState();

    W8MonsterManagerEntry entries[8];     /* 0x000 .. 0x8c0 */
    unsigned char unknown_8c0[0xf7];
    W8GrowableVector<int> vector_9b7;      /* 0x9b7 */
    unsigned char unknown_9c7[0x1043];
};                                       /* 0x1a0a */
#pragma pack(pop)

static_assert(sizeof(W8GrowableVector<int>) == 0x10, "W8GrowableVector_int_size_must_be_0x10");
static_assert(sizeof(W8MonsterManagerEntry) == 0x118,
              "W8MonsterManagerEntry_size_must_be_0x118");
static_assert(sizeof(W8MonsterManagerState) == 0x1a0a,
              "W8MonsterManagerState_size_must_be_0x1a0a");

extern W8MonsterManagerState g_monster_manager_state;

/* The 0x153-byte combat allocation has two adjacent runs of 0x11-byte records.
   ClearEffectSlot consumes a record whenever its leading active byte is set. */
typedef struct W8MonsterCombatEntry {
    signed char active;
    unsigned char unknown_01[0x10];
} W8MonsterCombatEntry;                    /* 0x11 */

#pragma pack(push, 1)
typedef struct W8MonsterCombatState {
    /* 0x000: the phase of the round this monster next acts on, zero when it
       has finished acting. */
    unsigned int phase;
    unsigned char active;                   /* 0x004 */
    unsigned char unknown_005[4];
    /* 0x009: how many attacks it gets this round, which is what divides the
       remaining phases between them. */
    int attacks_per_round;
    unsigned char unknown_00d[4];
    /* 0x011: the attack index Monster.cpp launches when combat has already
       selected this monster. It is asserted below MAX_MONSTER_ATTACKS before
       indexing the database record. */
    unsigned int attack_index_11;
    unsigned char unknown_015;
    /* 0x016: the queue of actions the monster's AI has decided on, one 0x30
       byte record each. The AI owns the list and destroys it outright. */
    W8PList* pending_actions;
    unsigned char unknown_01a[0x24];
    W8MonsterCombatEntry entries_3e[9];     /* 0x03e .. 0x0d7 */
    W8MonsterCombatEntry entries_d7[6];     /* 0x0d7 .. 0x13d */
    unsigned char unknown_13d[0xf];
    int value_14c;                          /* 0x14c */
    /* 0x150: the monster's turn has been set up already, so the setup runs
       once per turn however often it is asked for. */
    unsigned char turn_started;
    unsigned char unknown_151[2];
} W8MonsterCombatState;                    /* 0x153 */
#pragma pack(pop)

#pragma pack(push, 1)
/* One initialization unit inside W8MonsterInfo. The creator clears all 0x67
   bytes in one constant-sized operation; later consumers independently name
   the damage reduction and per-attribute adjustments inside it. */
typedef struct W8MonsterRuntimeBlock1DB {
    unsigned char unknown_00[6];
    signed char damage_reduction;              /* +0x06, W8MonsterInfo +0x1e1 */
    unsigned char unknown_07[5];
    signed char attribute_adjustments[7];      /* +0x0c, W8MonsterInfo +0x1e7 */
    unsigned char unknown_13[0x54];
} W8MonsterRuntimeBlock1DB;                    /* 0x67 */

typedef struct W8MonsterInfo {
    int location_id;                      /* 0x00 */
    int monster_group_id;                 /* 0x04: group lookup input in 0x004e6020 */
    unsigned int monster_species;         /* 0x08 */
    W8Monster* monster;                   /* 0x0c: p3D, named by source assertions */
    /* 0x10: pCombat, named by the MonsterManager.cpp:672 assertion
       "pMonsterInfo->pCombat != NULL" over the malloc 0x004e4390 stores here.
       The allocation is 0x153 bytes, zeroed as 0x54 dwords plus a word and a
       byte, and 0x004e4500 frees it and nulls the field again. */
    W8MonsterCombatState* pCombat;
    unsigned char flag_14;                /* 0x14: live-entry gate in 0x004e5c00 */
    /* 0x15: fInCombat, named by the MonsterManager.cpp:666 and :712 assertions
       "!pMonsterInfo->fInCombat" and "pMonsterInfo->fInCombat", which bracket
       the pair that allocates and releases pCombat. */
    unsigned char fInCombat;
    unsigned char flag_16;                /* 0x16: copied from the group's +0x2a */
    /* 0x17: the spawn position, unaligned. 0x004e3930 copies the caller's three
       floats here and hands the same triple to 0x004be5c0, whose result it
       stores next, and to 0x0042e620 with the new entry's id. */
    srVector3T<float> position_17;
    float derived_23;                     /* 0x23: 0x004be5c0 over position_17 */
    int hp_max;                           /* 0x27: uiHPMax in the Targeting.cpp assertion */
    int hp_current;                       /* 0x2b: reduced by canonical damage consumers */
    int runtime_stat_max_2f;              /* 0x02f: initialized from MONSTERS.DBS dice */
    int runtime_stat_current_33;          /* 0x033: initialized to the same roll */
    unsigned char unknown_37[0x20];
    /* 0x057: the monster's copy of the character condition array, entry for
       entry - condition two doubles its action fatigue at 0x05f, eight blocks
       its spellcasting at 0x077, thirteen makes it hostile at 0x08b, fifteen
       at 0x093 and seventeen is exhaustion at 0x09b. */
    int condition_turns[W8_CONDITION_COUNT];   /* 0x057 */
    W8Enchantment enchantments[8];             /* 0x0a7 */
    int value_107;                        /* 0x107: set to 0x12 when an entry deactivates */
    /* 0x10b: the argument a condition carries when a monster's conditions are
       copied onto a character. */
    int condition_argument;
    unsigned char unknown_10f[0xcc];
    W8MonsterRuntimeBlock1DB runtime_block_1db; /* 0x1db */
    int runtime_value_242;                /* 0x242: derived from runtime_stat_current_33 */
    unsigned char unknown_246;
    unsigned char converted_attributes_247[5]; /* 0x247: values clamped to 1..125 */
    unsigned char unknown_24c;
    unsigned char flag_24d;                 /* 0x24d: cycle-2 eligibility gate */
    unsigned char motionless;               /* 0x24e: fMotionless in the demo diagnostic */
    float scale_24f;                       /* 0x24f: HP-dependent live Monster scale */
    unsigned char flag_253;                 /* 0x253: set by 0x004e5c00 after processing */
    unsigned char unknown_254;
    unsigned char flag_255;                 /* 0x255: reset by 0x004e5ea0 and 0x004e6020 */
    unsigned char unknown_256[0x34];
    /* 0x28a: at one the monster counts as a live threat for the group-level
       sight query, on top of being alive and not too far gone. */
    unsigned char threat_28a;
    unsigned char unknown_28b[2];
    unsigned char flag_28d;
    int value_28e;                          /* 0x28e: cleared by the per-turn reset */
    unsigned char unknown_292[0x19];
    unsigned char flag_2ab;
    unsigned char unknown_2ac[0x0a];
    /* 0x2b6: what this monster can see of other monsters, one heap record per
       other monster. The two release paths own it: one drops every record
       about a departing monster, the other empties and destroys the whole
       list. */
    W8PList* mon_to_mon_visibility;
    /* 0x2ba: passed by address to 0x00536170 when combat begins; extent runs to
       the next established field, so the array bound is a partition of the
       unknown run rather than a proven size. */
    W8CombatSlot combat_slot_2ba;
    int value_2da;                          /* 0x2da: nonzero gate in 0x004e5c00 */
    /* 0x2de: the monster is under the effect the magic code clears by name;
       clearing it posts a notice and drops the visual. */
    unsigned char effect_2de;
    unsigned char unknown_2df[2];
    /* 0x2e1: the action the monster is taking, -1 through 9. Its whole domain
       is enumerated by MonsterActionFatigueCost, whose error text names it. */
    int action_kind;
    /* 0x2e5: qualifies action kind zero; three costs markedly more. */
    int action_detail;
    unsigned char unknown_2e9[8];
    int runtime_value_2f1;                /* 0x2f1: released when an entry is destroyed */
    /* 0x2f5: the monster's own contribution to the spell-point budget its
       database record sets a base for; the power-level chooser adds the two
       and reports a DATA ERROR when the base is zero. */
    int sp_budget_bonus;
    unsigned char unknown_2f9[4];
    int control_state;                      /* 0x2fd: group-recomputed control state */
    unsigned char unknown_301[0x37];
    int runtime_values_338[3];              /* 0x338: creator clears as one unit */
    int value_344;                          /* 0x344: creator initializes to -1 */
    unsigned char unknown_348[4];
    /* 0x34c: a combat-entry state byte 0x004e4390 raises to 2 when it is still
       zero, stamping the global at 0x00686a48 into value_354 at the same time. */
    unsigned char state_34c;
    unsigned char unknown_34d[7];
    int value_354;                          /* 0x354 */
    unsigned char unknown_358[0xcd];
} W8MonsterInfo;                          /* 0x425 */
#pragma pack(pop)

static_assert(sizeof(W8MonsterInfo) == 0x425, "W8MonsterInfo_size_must_be_0x425");
static_assert(sizeof(W8MonsterRuntimeBlock1DB) == 0x67, "W8MonsterRuntimeBlock1DB_size_must_be_0x67");


W8MonsterInfo* MonsterGetScriptPartByLocationIndex(unsigned int monster_list_index);
void ActivateMonster(W8MonsterInfo* monster_info, int mode);
void Function4E4600(W8MonsterInfo* monster_info);
void MonsterStartsDying(W8MonsterInfo* monster_info, int display_message);
W8MonsterRecord* GetMonsterDataForInfo(W8MonsterInfo* monster_info);
unsigned int MonsterGetIndexByLocationID(
    int caller_line,
    const char* caller_file,
    int location_id,
    unsigned char assert_on_failure);
W8MonsterInfo* MonsterInfoFromID(
    int caller_line,
    const char* caller_file,
    int location_id,
    unsigned char assert_on_failure);
W8MonsterRecord* GetMonsterDataByLocationID(int location_id);
W8Monster* GetMonsterByLocationID(int location_id);
float GetMonsterRecordScaledFloat1BA(W8MonsterInfo* monster_info);
void UpdateMonsterDamageAppearance(W8MonsterInfo* monster_info);
W8MonsterInfo* GetNextMonsterInfo(unsigned char reset_iterator);
int GetMonsterQuadrant(W8MonsterInfo* monster_info);
int GetQuadrantForPosition(srVector3T<float> position);
int Function4E5B50(unsigned int monster_species);
void ProcessMonstersAtCombatEnd(unsigned char forced_cleanup);
void ConvertMonsterAttributes(W8MonsterInfo* monster_info);
W8MonsterInfo* FindMonsterInfoBySpecies(unsigned int monster_species);
void ResetLivingMonstersAfterCombat(void);
void DestroyUngroupedMonsters(void);
void SetMonsterControlState(W8MonsterInfo* monster_info, int control_state);
void MonsterInfoSetMotionless(W8MonsterInfo* monster_info, unsigned char motionless);
void MoveMonsterToLiveList(W8MonsterInfo* monster_info);
W8MonsterInfo* FindNearestMonsterInfo(
    const srVector3T<float>* position,
    double maximum_distance);
void InitializeMonsterRuntimeStats(void);
float CalculateMonsterScale(W8MonsterInfo* monster_info);
void TryStartMonsterCycle2(
    W8MonsterInfo* monster_info,
    W8Monster* monster,
    int query_state);
unsigned int GetMonsterCombatValue(const W8MonsterRecord* record);
unsigned char AnyMonsterDying(void);

#endif
