#ifndef WIZ8_GAMEPLAY_BOUNDARIES_H
#define WIZ8_GAMEPLAY_BOUNDARIES_H

#include <stddef.h>

#include "wiz8/item_tables.h"
#ifdef __cplusplus
#include "surrender/srMath.h"
#include "wiz8/startup_runtime_state.h"
#endif

/* Shared recovered Wizardry interfaces used by matching translation units. */

#include "wiz8/3d_code/IList.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/vector.h"
#include "wiz8/combat_state.h"
#include "wiz8/magic.h"
#include "wiz8/screen_state.h"
#include "wiz8/game_state.h"
#include "wiz8/gameplay_databases.h"
#include "wiz8/monster_runtime.h"
#include "wiz8/layouts/encounter_tables.h"
#include "wiz8/regions.h"
#include "wiz8/ui_state.h"
#include "wiz8/utility.h"
#include "random.h"
#include "timer.h"

#pragma pack(push, 1)

/* A world position as the packed records carry it: three floats, C-compatible,
   distinct from srVector3T<float> which only exists for C++ consumers. */
typedef struct W8Position {
    float x;
    float y;
    float z;
} W8Position;

/* The 0x3c-byte anchor a character carries and the recall effect restores.
   Only the leading point is read field by field; the rest travels as one
   block, so nothing beyond it is named. */
typedef struct W8SavedLocation {
    W8Position point;                    /* 0x00 */
    unsigned char unknown_0c[0x30];
} W8SavedLocation;                       /* 0x3c */



/* The game's wide text format: fixed-size UINT16 arrays stored inline in
   records and manipulated through the CRT wide-string functions. The same
   typedef models these fields in config/types/wiz8/gameplay_databases.h;
   under VC6 wchar_t is unsigned short, so the two spellings are one type. */



/* The twenty-one factions, the domain the name lookup enumerates. */
enum { W8_FACTION_COUNT = 21 };

typedef struct W8FactionRuntimeRecord {
    signed char disposition_score;      /* 0x00 */
    unsigned char unknown_01[3];
    int value_04;                       /* 0x04 */
    unsigned char flag_08;              /* 0x08 */
    unsigned char unknown_09[5];
} W8FactionRuntimeRecord;               /* 0x0e */

/* One attribute record. The array is indexed by skill id biased by 0x22, so the
   seven attribute ids sit at the top of the skill numbering; only the leading
   value, which IsCharacterSkillAvailable tests against 100, is established. */
typedef struct W8CharacterAttribute {
    unsigned int value;                   /* 0x00 */
    /* 0x04: the value after equipment and effects. Resistance recalculation
       reads this one, not the base, and only above a threshold of 0x50. */
    unsigned int effective;
    unsigned char unknown_08[0xc];
} W8CharacterAttribute;                   /* 0x14 */

/* One skill record, indexed directly by skill id. PracticeCharacterSkill
   establishes the stride and the leading flag it sets when a skill first
   becomes available; IsCharacterSkillAvailable reads the same flag. */
typedef struct W8CharacterSkill {
    unsigned char flag_00;                /* 0x00 */
    unsigned char unknown_01;
    /* 0x02: a second figure the spell-learning ceiling divides by ten, the
       same way the resistances divide `level`. The two are distinct fields of
       one skill, not one field read two ways. */
    unsigned int value_02;
    /* 0x06: the skill's current level. Resistance recalculation divides it by
       ten for skills 28..33 and by five for skill 36, which is what places it. */
    unsigned int level;
    unsigned char unknown_0a[0x1c];
} W8CharacterSkill;                       /* 0x26 */

/* One resistance channel. Recalculation rebuilds `base` from scratch each time
   and then derives `total` from it, so the two are a computed pair rather than
   a stored value and a cache. */
typedef struct W8CharacterResistance {
    unsigned int base;                    /* 0x00 */
    unsigned int total;                   /* 0x04: clamped to 100 */
    unsigned char unknown_08[8];
} W8CharacterResistance;                  /* 0x10 */

/* The six realms a spell point pool is kept per are W8SpellRealm's, declared
   with the spell record in wiz8/gameplay_databases.h. */

/* The eighteen conditions a character can be under, indexed directly into
   W8Character::condition_turns. Only the ones a recovered body names are
   spelled out; the rest keep their numbers. */
enum {
    /* Twenty entries: the per-character copier walks all twenty, while the
       sweep that lifts everything stops at eighteen because the last two are
       not the kind a rest clears. */
    W8_CONDITION_COUNT = 20,
    W8_CONDITION_CLEARABLE_COUNT = 18,
    W8_CONDITION_FATIGUE_DOUBLED = 2,
    W8_CONDITION_LOAD_EASED = 5,
    /* Seven is the one condition that carries a second value alongside its
       duration, which both copiers special-case. */
    W8_CONDITION_WITH_ARGUMENT = 7,
    W8_CONDITION_SPELLCASTING_BLOCKED = 8,
    W8_CONDITION_HOSTILE = 0xd,
    W8_CONDITION_EXHAUSTED = 0x11,
    W8_CONDITION_EQUIPMENT_UNLOCKED = 18,
    /* The duration that means "until lifted". */
    W8_CONDITION_INDEFINITE = 9999
};

/* One enchantment slot. Both a character and a monster carry eight of them,
   and both clear a slot by zeroing all three dwords at once. */
typedef struct W8Enchantment {
    int value_00;
    int value_04;
    /* 0x08: the field the topmost-slot scan reads and the one the fatigue path
       consults on slot five. */
    int value_08;
} W8Enchantment;                          /* 0x0c */

enum {
    W8_RESISTANCE_COUNT = 6,
    /* The six skills whose level feeds the matching resistance, and the one
       whose presence adds a flat bonus to every one of them. */
    W8_FIRST_RESISTANCE_SKILL = 28,
    W8_RESISTANCE_BONUS_SKILL = 36,
    /* A race adjustment at or below this is a flat amount; above it, it selects
       a character attribute by index biased this far. */
    W8_RACE_ADJUSTMENT_ATTRIBUTE_BIAS = 1000
};

/* One hand's attack block. The extent is the stride between the two, and only
   the two leading fields are established. */
typedef struct W8HandAttack {
    unsigned char in_play;                /* 0x00 */
    int weapon_skill;                     /* 0x01, unaligned */
    unsigned char unknown_05[0xc];
    /* 0x11: what the hand's attack is worth, read only once the hand has a
       range to the target at all. */
    int attack_value;
    unsigned char unknown_15[0x46];
} W8HandAttack;                           /* 0x5b */

typedef struct W8ItemInstance {
    int item_id;
    unsigned char stack_count;           /* 0x04: quantity-kind 1 */
    unsigned char uses_or_charges;       /* 0x05: quantity-kinds 2 through 4 */
    unsigned char identified;
    unsigned char unknown_07[3];
    /* 0x0a: the binding has already been announced for this instance, which is
       what stops the log line repeating. */
    unsigned char bind_announced;
    /* 0x0b: the instance is bound to its wearer. Raised when a binds-on-equip
       item is worn and read by the predicates that refuse to take it off. */
    unsigned char bound;
} W8ItemInstance;                        /* 0x0c */

typedef struct W8Character {
    /* 0x0000: SaveCharacter stamps 1 here before writing the record, so the
       leading dword is a saved-record version rather than runtime state. */
    unsigned int record_version;
    unsigned char in_party;              /* 0x0004 */
    /* 0x0005: the character's name, wide, and the stem SaveCharacter formats
       "%ls.CHR" from. The extent below partitions the unknown run up to the
       profession at 0x0069; it is not proven, and only the fact that a wide
       string starts here is. */
    wchar_t name[0x10];
    unsigned char unknown_0025[0x44];
    /* 0x0069 and 0x006d: iProfession, named by the GameplayCode.cpp:399
       assertion that bounds it against PROF_COUNT, and the profession the
       character started in. The level band subtracts a base only while the two
       agree. */
    int current_profession;               /* 0x0069 */
    int original_profession;              /* 0x006d */
    int race;                             /* 0x0071: indexes the race resistance table */
    int faction;                          /* 0x0075: compared against the caller's faction */
    /* 0x0079: looked up from the table at 0x00616604 by faction, race and
       profession together. Note that the index multiplies 0x0075 by sixteen
       against an eleven-race domain, so either that field is not the faction
       the disposition code reads or the table is sparse; the disagreement is
       recorded rather than resolved. */
    int table_value_0079;
    unsigned char unknown_007d[0xc];
    unsigned int level;                   /* 0x0089: averaged across occupied slots */
    int profession_levels[15];            /* 0x008d */
    unsigned char unknown_00c9[0x14];
    /* 0x00dd: the eight-band ladder over the character's level in their
       current profession, and the base subtracted from it while they are still
       in the profession they started in. */
    int level_band;
    int level_band_base;
    W8CharacterAttribute attributes[7];   /* 0x00e5, indexed by skill_id - 0x22 */
    unsigned char unknown_0171[0x2c];
    W8CharacterSkill skills[0x29];        /* 0x019d, indexed by skill_id */
    unsigned char unknown_07b3[0x23a];
    /* 0x09ed..0x09f8: experience, the goal for the next level, and the goal
       the previous level had. A character is ready to advance once the first
       reaches the second. */
    unsigned int experience;
    unsigned int experience_goal;
    unsigned int experience_previous_goal;
    unsigned char unknown_09f9[4];
    /* 0x09fd: how many times this character has died. */
    int death_count_09fd;
    /* 0x0a01: one entry per condition, holding how long it has left to run;
       W8_CONDITION_INDEFINITE means until something lifts it. The exhausted
       condition the fatigue path calls 0x11 lands exactly on element
       seventeen, which is what fixes base and stride, and the monster carries
       the identical array at its own 0x57 with the same indices meaning the
       same things. Several entries were read individually before this array
       explained them: two doubles the fatigue an action costs, eight blocks
       spellcasting, eighteen unlocks bound equipment. */
    int condition_turns[W8_CONDITION_COUNT];   /* 0x0a01 */
    unsigned char unknown_0a51[0x14];
    W8Enchantment enchantments[8];             /* 0x0a65 */
    unsigned char unknown_0ac5[0x3c];
    /* 0x0b01 gates party-member selection alongside hp_current: a slot is
       eligible when it still has hit points and this is under 0x12, and a
       second tier tests it against 0x0f. It is unsigned - the canonical
       compares are JB/JBE, not JL/JE - but its meaning is not established. */
    unsigned int unknown_0b01;
    /* 0x0b05: the highest enchantment slot still in use, recomputed by
       scanning down from the last one whenever a slot is cleared. */
    int enchantment_top;
    /* 0x0b09: the argument the seventh condition carries. */
    int condition_argument;
    /* 0x0b0d..0x0b20: the two pools with a ceiling each, plus the adjustment
       damage is booked against before hit points are recalculated. A character
       whose hp_current is zero is treated as out of the fight everywhere. */
    int hp_max;                           /* 0x0b0d */
    unsigned int hp_current;              /* 0x0b11 */
    int hp_adjustment;                    /* 0x0b15 */
    int stamina_max;                      /* 0x0b19 */
    int stamina;                          /* 0x0b1d */
    unsigned char unknown_0b21[4];
    /* 0x0b25 and 0x0b45: iSPMax and iSPLeft, one per spell realm, named by the
       Health Stamina Mana.cpp:1067 assertion pPC->iSPLeft[uiRealm] and bounded
       at six realms by the total the party-wide restore accumulates. */
    int sp_max[W8_SPELL_REALM_COUNT];     /* 0x0b25 */
    unsigned char unknown_0b3d[8];
    int sp_left[W8_SPELL_REALM_COUNT];    /* 0x0b45 */
    unsigned char unknown_0b5d[0x6c];
    /* 0x0bc9: the load category, zero through four, which scales what an
       action costs in fatigue. FatigueCharacter's error text calls it that. */
    int load_category;
    /* 0x0bcd: one entry per spell. CanCharacterUseItem refuses a spell-source
       item whose spell already reads one here, so one is the learned state. */
    int spell_learned[136];               /* 0x0bcd */
    /* 0x0ded: indexed by skill_id. For the six realm skills it counts the
       spells known in that realm, which is what makes the skill available at
       all - LearnSpell bumps the entry and IsCharacterSkillAvailable reads it.
       LearnSpell also writes a recomputed figure into entry 36, which is a
       real skill id but not a count; that disagreement is recorded rather than
       resolved. */
    unsigned int skill_unlocks[0x29];
    unsigned char unknown_0e91[0x48];
    /* 0x0ed9: a percentage taken off incoming damage, the character's
       counterpart of the monster's own at 0x1e1. */
    int damage_reduction;
    W8CharacterResistance resistances[W8_RESISTANCE_COUNT]; /* 0x0edd */
    unsigned char unknown_0f3d[0x20];
    /* 0x0f5d: the twelve worn/held slots, indexed by the same slot numbering
       GetItemDefaultEquipSlot answers and GetItemEquipSlotMask sets bits for.
       Slots six and seven are the primary hands and eight and nine the
       alternate pair, which is what the two-handed and off-hand tests read. */
    W8ItemInstance equipment[12];         /* 0x0f5d */
    unsigned char unknown_0fed[0x3c];
    /* 0x1029: the eight per-character carried slots. GetOriginOfCharacterItem
       reports this array as origin zero and the equipment array as origin one. */
    W8ItemInstance backpack[8];           /* 0x1029 */
    unsigned char unknown_1089[0xc4];
    /* 0x114d: one block per hand. Only the leading flag - the hand is in play -
       and the weapon skill the attack setup stamps into it are established. */
    W8HandAttack hand_attacks[2];         /* 0x114d */
    unsigned char unknown_1203[0x49b];
    /* 0x169e: the fatigue band, zero through four, recomputed from the stamina
       fraction whenever it moves; a change re-runs the armour class pass. */
    int fatigue_band;
    unsigned char unknown_16a2[0xd5];
    signed char resistance_bonus_all;     /* 0x1777: added to every resistance */
    unsigned char unknown_1778[0x34];
    signed char resistance_bonus[W8_RESISTANCE_COUNT];      /* 0x17ac */
    unsigned char unknown_17b2[3];
    /* 0x17b5: the character is out of the formation, which is what the
       front-rank counts skip. */
    unsigned char out_of_formation;
    unsigned char unknown_17b6[0x21];
    /* 0x17d7: where the character was last anchored, restored by the recall
       effect in Magic Effects.cpp. Its extent is proven rather than assumed:
       the cross-level path copies exactly 0x3c bytes from here with one
       rep movsd, which lands precisely on the level id below. */
    W8SavedLocation saved_location;      /* 0x17d7 */
    /* 0x1813: which level that anchor belongs to. The recall compares it
       against g_current_level and takes a different path when they differ. */
    int saved_level;                     /* 0x1813 */
    unsigned char unknown_1817[0x44];
    /* 0x185b: the deep-fatigue effect is already on this character, which is
       what stops FatigueCharacter re-applying it every turn. */
    unsigned char deep_fatigue_applied;
    unsigned char unknown_185c[5];
    /* 0x1861: the anchor above has been set. Recall does nothing without it. */
    unsigned char has_saved_location;
} W8Character;                           /* 0x1862 */

typedef struct W8RPCSlot {
    unsigned char opaque[0x118];
} W8RPCSlot;



typedef struct W8LevelFolderRecord {
    char folder_name[50];
    char level_name[50];                 /* 0x32 */
    char location_code[4];               /* 0x64: three-letter code plus NUL */
    signed char unknown_68;
    signed char unknown_69;
    signed char unknown_6a;
} W8LevelFolderRecord;                   /* 0x6b */

/* One 0x118-byte slot in the block at 0x006836B8 that 0x0054B300 resets.
   Only the offsets that reset touches are established, so the fields keep
   positional names. */
typedef struct W8MonsterSlot {
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
    /* 0x0d8: the monsters this party slot currently has highlighted, held in a
       growable vector's storage. Clearing the highlight walks it and zeroes the
       count without releasing the buffer, which is what makes it a vector
       rather than a fixed array.

       Spelled as the layout rather than as W8GrowableVector<int> on purpose:
       that class has a virtual destructor, and naming it here would have VC6
       dynamically initialise the whole slot array and change two neighbouring
       bodies that only memset it. */
    struct {
        void* vptr;                       /* 0x0d8 */
        int count;                        /* 0x0dc */
        int capacity;                     /* 0x0e0 */
        int* data;                        /* 0x0e4 */
    } highlighted_monsters;
    unsigned char field_0e8;
    unsigned char unknown_0e9[0x2f];
} W8MonsterSlot;                         /* 0x118 */








/* Filled by 0x0042A370 from a level-table row; only its size is established
   here, by the 0x458-byte stack frame of its sole recovered caller. */
typedef struct W8LevelInfo {
    unsigned char unknown_000[0x458];
} W8LevelInfo;                           /* 0x458 */

/* What a generator hangs off itself. Both are released through slot zero of
   their own vtable with the deleting flag set, so both are polymorphic; nothing
   else about either is established. */
class W8MonsterGeneratorNode {
public:
    virtual ~W8MonsterGeneratorNode();
};

typedef struct W8MonsterGenerator {
    unsigned int flags;                   /* 0x00: bit 2 is cleared on teardown */
    unsigned char flag_04;                /* 0x04 */
    unsigned char unknown_05;
    unsigned short value_06;              /* 0x06 */
    unsigned short value_08;              /* 0x08 */
    unsigned char unknown_0a[2];
    /* 0x0c..0x14: three dwords, saved individually and handed to
       GenerateEncounter as a block. */
    int state_0c;
    int state_10;
    int state_14;
    W8MonsterGeneratorNode* node_18;      /* 0x18 */
    int value_1c;                         /* 0x1c */
    /* 0x20: m_pTimer, named by the MonGen.cpp:535 assertion, whose message also
       gives the owning class and method - "MonGen::Reset() out of memory
       allocating m_pTimer". */
    W8MonsterGeneratorNode* m_pTimer;
    char name[32];                        /* 0x24 */
    unsigned char flag_44;                /* 0x44: written to the save after the name */

#ifdef __cplusplus
    /* Named by the assertion message above. Rearms the generator's timer,
       creating it on first use, with a delay jittered around the configured
       interval. */
    void Reset();
    /* Arms or disarms the generator, loading its marker on the way in. */
    void SetActive(unsigned char active, W8MonsterGeneratorNode* node);
    /* The save pair. Both are __thiscall in the image. */
    void Save(int handle);
    unsigned char Load(int handle);
    /* Moves the generator, notifying the scene when it is armed. */
    void SetState(const W8Position* state);
    /* Loads the marker unconditionally, then applies the armed state. */
    void Reload(int unused, unsigned char active);
    ~W8MonsterGenerator();
#endif
} W8MonsterGenerator;

/* The stride is the record LoadMonsterGroup allocates, zeroes and reads whole,
   and which its own assertion spells sizeof(*pMonsterGroup). Only the fields
   that loader establishes are named; the rest stays opaque. */
/* The twelve formation bytes a group hands its members. Three dwords rather
   than a byte block because 0x0050FF40 sets them one dword at a time from a
   caller-supplied triple; what they mean is not established. */
typedef struct W8MonsterFormation {
    unsigned int value_00;
    unsigned int value_04;
    unsigned int value_08;
} W8MonsterFormation;

typedef struct W8MonsterGroup {
    int group_id;                         /* 0x00: GroupIndex ID lookup key */
    int member_count;                     /* 0x04: decremented when members leave */
    struct W8IList* monsters;             /* 0x08: fresh IList per live group */
    unsigned char unknown_0c[8];
    int active_member_count;              /* 0x14: recomputed from member conditions */
    int monster_id;                       /* 0x18 */
    /* 0x1c: the mean of the live members' positions, recomputed on demand. */
    W8Position centre;
    unsigned char flag_28;                /* 0x28: cleared after the record loads */
    /* 0x29: fInCombat, named by the MonsterGroup.cpp:492 assertion. Cleared
       after the record loads and again when the group leaves combat. */
    unsigned char flag_29;
    /* 0x2a: at one the group is live regardless of the global gate at
       0x00547510; anything else has to pass that gate as well. */
    unsigned char flag_2a;
    unsigned char unknown_2b;
    /* 0x2c: selects which of the record's two name sets a member is displayed
       under. GetMonsterName reads it and nothing recovered yet writes it. */
    unsigned char flag_2c;
    unsigned char unknown_2d[0x6e];
    /* 0x9b: which member of the group the party currently has picked out,
       by location id, and -1 when none - which is how the group loads. Cycling
       through the group's targetable members reads it to know where it is and
       writes back where it got to. */
    int highlighted_member;
    int value_9f;                         /* 0x9f: a member location id; RemoveMonster
                                             compares it against the departing
                                             member's before renotifying */
    /* 0xa3: the group this one follows. Walking it is how a member request is
       redirected to the group that actually leads the formation, and a zero
       ends the walk. */
    int leader_group_id;
    /* 0xa7: up to four allied group ids. The notification pass walks all four
       unconditionally and skips the zero entries, so the array is fixed-size
       rather than terminated. */
    int allied_group_ids[4];
    /* 0xb7: copied verbatim onto every member's live Monster when the formation
       is re-applied. Unaligned inside this packed record, which is why the copy
       comes out as twelve byte moves rather than three dword ones. */
    W8MonsterFormation formation;
    unsigned char flag_c3;                /* 0xc3: gates the trailing notification */
    /* 0xc4 is a saved-record version: at 2 and above the loader reads one more
       byte, and below 3 it clears flag_ca that older saves never wrote. */
    unsigned int version;                 /* 0xc4 */
    unsigned char unknown_c8[2];
    unsigned char flag_ca;                /* 0xca */
    int value_cb;                         /* 0xcb: cleared by the per-turn reset */
    /* 0xcf: when this group was last budgeted. UpdateRandomEncounterBudget
       advances it by the elapsed time and the culling pass measures against it. */
    int spawn_time;
    unsigned char flag_d3;                /* 0xd3: raised as flag_c3 is cleared */
    unsigned char unknown_d4[0x57];
} W8MonsterGroup;                         /* 0x12b */

/* 3D Code\PList.cpp. Distinct from W8GrowableVector: no vptr, elements at +0x00
   and count at +0x08, accessed through free functions. */

/* Member names and types at 0x08 and 0x48 come from the canonical assertion
   expressions "pWorld && pWorld->plsProps" (Engine Code\3d.cpp:344) and
   "pWorld->psrMeshes" (Engine Code\3dapi.cpp:446). The pls/psr prefixes are
   the original's own Hungarian coding for a PList and a SurRender object.
   The two offsets do not carry the same weight: plsProps is byte-proven by
   WorldUpdateProps (0x0046DED0), while psrMeshes is only placed by a reading
   of its asserting body and has no ported consumer yet. */
struct W8UpdateMeshSource;

typedef struct W8World {
    /* 0x000 and 0x004: two more lists beside the props, each with its own
       add/remove pair; nothing establishes what they hold. */
    W8PList* plsList00;
    W8PList* plsList04;
    W8PList* plsProps;                    /* 0x008: PList of props; byte-proven */
    unsigned char unknown_00c[4];
    W8PList* plsAmbientSounds;            /* 0x010: Engine Code\AmbientSound.cpp */
    unsigned char unknown_014[0x30];
    /* 0x044: the camera the viewport rebuilds its view plane against. It was
       modelled separately as a camera-owner class before the two globals were
       shown to be one, and it is the same object. */
    struct srCamera* camera;
    void** psrMeshes;                     /* 0x048: mesh pointer array; UNPROVEN placement */
    unsigned char unknown_04c[8];
    /* 0x054: the node the sky hangs from, which the environment accessors
       reach through this same world pointer. */
    void* sky_node;
    unsigned char unknown_058[0x18];
    W8UpdateMeshSource* update_mesh_source_70;
    /* 0x074 and 0x078: written together from one argument by 0x0046E350 and
       read back individually, so they are a pair rather than one field. Only
       the second is ever read, by the getter at 0x0046E3A0. */
    float value_74;
    float value_78;
    unsigned char unknown_07c[0x40];
    W8GrowableVector<class W8VectorElement005EC294*>* lights; /* 0xbc */
    unsigned char unknown_0c0[4];
    W8GrowableVector<W8MonsterGenerator*>* monster_generators; /* 0xc4 */
} W8World;

typedef struct W8NPCRecordRef {
    unsigned char unknown_00[0x58];
    int record_id;                        /* 0x058: matches W8NpcDatabaseRecord::record_id */
} W8NPCRecordRef;

typedef struct W8NPCItemList {
    int unknown_00;                       /* 0x00: handle passed to 0x0055A0A0 */
    unsigned char unknown_04[2];
    W8NPCRecordRef* npc_record;           /* 0x06 */
    unsigned char unknown_0a[0x10];
    unsigned char flag_1a;                /* 0x1a: gates the teardown in LoadFactState */
} W8NPCItemList;

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
    W8Position point;
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
    /* 0x10 through 0x1b means different things by kind, which is what the two
       readings of it are: a place-kind target keeps the world point there,
       while a character- or monster-kind target keeps a flag at 0x19 that
       decides whether it is described by name or by its faction's word for it.
       They are alternatives, not neighbours. */
    union {
        W8Position point;                 /* 0x10 */
        struct {
            unsigned char unknown_10[9];
            unsigned char name_known;     /* 0x19 */
            unsigned char unknown_1a[2];
        } described;
    };
    /* 0x1c: the item aimed at, for the one kind that aims at one. */
    W8ItemInstance* pPCItem;
} W8CombatSlot;                           /* 0x20 */

/* One party slot row. Only the three fields the reset touches are established,
   plus the leading flag UtilityFunctions reads as slot-occupied. */
typedef struct W8PartySlotRow {
    unsigned char flag_00;               /* 0x000: slot occupied */
    /* 0x001: the action this slot has chosen this round; -1 is none, and the
       round reset lifts the ninth action specifically. */
    int pending_action;
    /* 0x005: the attack mode chosen per hand, indexed by the combat state's
       current hand. The bound is a partition of the unknown run, not proven. */
    int attack_mode[4];                  /* 0x005 */
    unsigned char unknown_015[8];
    /* 0x01d, 0x04d, 0x081, 0x0a9 and 0x0d1: five target blocks of the same
       shape, one per targeting context. GetTargetBlockForContext picks between
       them by context number, which is what shows them as one array of
       alternatives rather than five unrelated fields; context two is answered
       from a shared global instead of from the row. The first two are cleared
       together when the character dies. */
    struct W8CombatSlot target_out_of_combat;   /* 0x01d, context 0 */
    unsigned char unknown_03d[0x10];
    struct W8CombatSlot target_in_combat;       /* 0x04d, context 1 */
    /* 0x06d and 0x071: what the slot is doing and the detail that qualifies
       it, the character counterpart of the monster's 0x2e1 and 0x2e5. */
    int action_kind;
    int action_detail;
    /* 0x075..0x0a0: the slot's pending spell target - what kind of target it
       is, at what strength, a cleared word, and the eight-dword target block
       the targeting code hands over. The strength is uiPowerLevel, which the
       Magic.cpp:4287 assertion bounds at one through seven; eight is the
       further "as high as the caster can afford" request that the affordable-
       power walk resolves before the cost is taken. */
    int spell_id;                        /* 0x075 */
    int spell_power_level;               /* 0x079 */
    int spell_power_extra;               /* 0x07d */
    struct W8CombatSlot spell_target;    /* 0x081, context 3 */
    /* 0x0a1..0x0cf: the item-use block, the same shape as the spell one above -
       what is being used, which item, where it is aimed and where it came
       from. */
    int item_use_kind;                   /* 0x0a1 */
    struct W8ItemInstance* item_in_use;  /* 0x0a5 */
    struct W8CombatSlot item_target;     /* 0x0a9, context 4 */
    /* 0x0c9: the item id recorded when the use was chosen. The use is checked
       again by looking the item up from its origin and slot and comparing this
       against what comes back, so it is what catches an item that moved. */
    int item_id_0c9;
    unsigned char item_origin;           /* 0x0cd */
    unsigned short item_slot;            /* 0x0ce */
    unsigned char flag_0d0;              /* 0x0d0: reset to 0xff */
    struct W8CombatSlot target_context_5;   /* 0x0d1 */
    unsigned char unknown_0f1[4];
    /* 0x0f5: set while the slot is out of action. The party-wide sweeps skip a
       slot that has it raised, and the targeting guard reads the same byte. */
    unsigned char flag_0f5;
    unsigned char unknown_0f6[4];
    /* 0x0fa: the animation this slot is driving, -1 when none. Death tells it
       to stop. */
    int animation_0fa;
    unsigned char unknown_0fe[6];
    /* 0x104: the slot's action is the first kind, cached beside it. */
    unsigned char action_is_kind_one;
    unsigned char flag_105;              /* 0x105 */
} W8PartySlotRow;                        /* 0x106 */


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


typedef unsigned char W8FactionDisposition;

/* Local Code\UtilityFunctions.cpp. These are the signed screen-space rectangle
   and point shapes consumed by 0x00517E20 and 0x00517E70. */
/* The same shape as W8ControlsRect; see the note there for why the two are
   kept apart. */
typedef struct W8ScreenRect {
    int left;
    int top;
    int right;
    int bottom;
} W8ScreenRect;

typedef struct W8ScreenPoint {
    int x;
    int y;
} W8ScreenPoint;

enum {
    W8_FACTION_HOSTILE = 0,
    W8_FACTION_NEUTRAL = 1,
    W8_FACTION_FRIENDLY = 2
};

/* Two faction ids the disposition calculation singles out before consulting the
   faction table at all. Their values come from that body's switch: zero falls
   through to the record's own hostility range, and one is always friendly. */
enum {
    W8_FACTION_UNALIGNED = 0,
    W8_FACTION_PARTY = 1
};

/* What a default disposition answers with. Distinct from W8FactionDisposition:
   the two scales disagree, and the calculation maps between them. */
enum {
    W8_DISPOSITION_NEUTRAL = 0,
    W8_DISPOSITION_HOSTILE = 1,
    W8_DISPOSITION_FRIENDLY = 2
};

struct W8MonsterInfo;

/* 0x0068C09C addresses a flat array of localized notice pointers. ShowRegionHelp
   indexes it by a region's help id, which is what fixes the shape as an array
   rather than a record; the constants below are the entries a ported body picks
   by name instead of out of data, and are the byte offsets those bodies use
   divided by the pointer stride. The element type is not established, so the
   array stays void*. */
enum W8NoticeId {
    W8_NOTICE_MONSTER_SLAIN = 0x74c / 4,
    W8_NOTICE_CHARACTER_SAVE_FAILED = 0x780 / 4,
    W8_NOTICE_CHARACTER_LOAD_FAILED = 0x784 / 4,
    W8_NOTICE_COMBAT_CANNOT_END = 0x878 / 4,
    W8_NOTICE_COMBAT_CANNOT_END_ENGAGED = 0x87c / 4,
    W8_NOTICE_COMBAT_CANNOT_END_PENDING = 0x880 / 4,
    W8_NOTICE_COMBAT_ENDED = 0x884 / 4,
    W8_NOTICE_COMBAT_STANCE_RELAXED = 0x888 / 4,
    W8_NOTICE_COMBAT_STANCE_READY = 0x88c / 4
};

/* One record per character class, 0x1e5 bytes, indexed by the class index a
   combat actor carries at its +0x1d8. Only the flag the combat toggle reads is
   established. */
typedef struct W8CharacterClassRecord {
    unsigned char unknown_000[0x154];
    unsigned char flag_154;               /* 0x154 */
    unsigned char unknown_155[0x90];
} W8CharacterClassRecord;                 /* 0x1e5 */

/* What the engaged-actor iterator at 0x004A2760 hands back. Only the class
   index is placed; the object is much larger and otherwise unrecovered. */
typedef struct W8CombatActor {
    unsigned char unknown_000[0x1d8];
    int class_record_index;               /* 0x1d8 */
} W8CombatActor;

/* The block the pointer at 0x006836A8 addresses: the engine's combat state.
   Only what a ported body reaches is named, and only where the use establishes
   a meaning - the monster slot at +0x7b8 is named because every consumer either
   compares it against a W8MonsterInfo it already holds or clears it when that
   entry leaves the world. The rest keep positional names. */
/* One combat participant's row, 0xd4 bytes per character. The eight of them
   begin at the combat state's own address, so W8CombatState's leading fields
   are the first row's; only the fields the fatigue, death and engagement paths
   touch are established. */
typedef struct W8CombatCharacterRow {
    unsigned char unknown_00[0x18];
    int value_18;                        /* 0x18: cleared when the character dies */
    unsigned char unknown_1c[0x30];
    unsigned char flag_4c;               /* 0x4c: raised when the character dies */
    unsigned char unknown_4d[0x37];
    int current_hand;                    /* 0x84: indexes the slot row's attack modes */
    int current_equip_slot;              /* 0x88: indexes the character's equipment */
    unsigned char unknown_8c[0x48];
} W8CombatCharacterRow;                  /* 0xd4 */

typedef struct W8CombatState {
    unsigned char flag_000;               /* 0x000: blocks ending combat while set */
    unsigned char flag_001;               /* 0x001 */
    unsigned char unknown_002[2];
    int value_004;                        /* 0x004: blocks ending combat while non-zero */
    /* 0x008: the round counter every combatant stamps itself against. */
    int round_counter;
    unsigned char unknown_00c[0x7a4];
    int selected_slot;                    /* 0x7b0: cleared with selected_monster */
    /* 0x7b4: the party slot whose turn it is; -1 when nobody's. It is cleared
       alongside selected_slot when that character dies. */
    int selected_character;
    struct W8MonsterInfo* selected_monster; /* 0x7b8 */
    unsigned char unknown_7bc[0x104];
    W8CombatActor* engaged_actor;         /* 0x8c0 */
    unsigned char unknown_8c4[0x24];
    /* 0x8e8: the slots whose characters died this round, and how many. */
    int pending_deaths[8];
    int pending_death_count;              /* 0x908 */
    /* 0x90c: what kind of move is pending, one or two; zero is none. */
    int pending_move_kind;
    /* 0x910: how the party is moving - zero out of combat, one and two the two
       combat modes, which is what the speed and phase rules branch on. */
    int movement_mode;
    unsigned char unknown_914[4];
    /* 0x918: which phase of the turn is running, zero through three. */
    int turn_phase;
    unsigned char unknown_91c[4];
    /* 0x920: the formation combat started with, thirty-three dwords copied
       whole out of 0x00687511. */
    int saved_formation[0x21];
    unsigned char unknown_9a4[0xac];
    unsigned char flag_a50;               /* 0xa50 */
    unsigned char flag_a51;               /* 0xa51 */
    unsigned char unknown_a52[2];
    unsigned char flag_a54;               /* 0xa54 */
    unsigned char unknown_a55[0xd];
    unsigned char flag_a62;               /* 0xa62: the party's combat-ready bit */
} W8CombatState;

typedef struct W8LevelRuntimeBlock {
    unsigned char unknown_000[0xf4];
    /* 0x0f4: what still needs redrawing. Everything that changes anything the
       screen shows ORs its own bit in here rather than drawing. */
    unsigned int redraw_flags;
    unsigned char unknown_0f8[0xc];
    /* 0x104: which region the pointer is over. The portrait hit test walks the
       party slots against two runs of region numbers. */
    unsigned int hover_region;
    unsigned char unknown_108[0x4c];
    /* 0x154: raised whenever the party moves its pick from one monster to
       another, so whatever draws the pick knows to look again. */
    unsigned char pick_changed_154;
    /* 0x155: the last of the combat regions is only taken down when this is
       clear. */
    unsigned char flag_155;
    unsigned char unknown_156[0x16];
    /* 0x16c: the character whose highlight overrides everyone else's, -1 when
       none does. */
    int highlight_override;
    unsigned char unknown_170[0x38];
    /* 0x1a8: one entry per message line the main game screen is showing. */
    int text_lines[12];
    /* 0x1d8 and 0x1e8: two four-entry tables the text box clears a slot of at
       a time, each to -1. */
    int text_slots_1d8[4];
    int text_slots_1e8[4];
    /* 0x1f8: a dialogue is open, and 0x1fc is whose - the flag has to be up
       before the pointer is worth reading. */
    unsigned char dialogue_open;
    unsigned char unknown_1f9[3];
    unsigned char* dialogue_owner;         /* 0x1fc */
    unsigned char unknown_200[0x64];
    /* 0x264: the item the pointer is over, -1 for none. */
    int highlighted_item;
    /* 0x268: the item the interface has selected, -1 for none. */
    int selected_item;
    unsigned char unknown_26c[0x10];
    /* 0x27c and 0x280: the level a queued transition is bound for and the
       entry point within it, the second always -1 when the recall effect
       stages the move. */
    int pending_level;                   /* 0x27c */
    int pending_entry_id;                /* 0x280 */
    unsigned char unknown_284[0x3c];
    /* 0x2c0 and 0x2c8: two redraw requests the party-state change raises. Only
       the first is conditional on a fight being on. */
    unsigned char refresh_combat_panel;
    unsigned char unknown_2c1[7];
    unsigned char refresh_party_panel;
    unsigned char unknown_2c9;
    short combat_end_notification;         /* 0x2ca: -1 suppresses the callback */
    /* 0x2cc and 0x2d4: the text box's scroll extent, whose difference is how
       far it can still be scrolled. */
    int scroll_top;
    unsigned char unknown_2d0[4];
    int scroll_bottom;
    unsigned char unknown_2d8[4];
    /* 0x2dc and 0x2e0: the two movement budgets, both filled to a hundred when
       the party is put back under its own control. */
    int move_budget_2dc;
    int move_budget_2e0;
    unsigned char unknown_2e4[4];
    int value_2e8;                         /* 0x2e8 */
    unsigned char unknown_2ec[4];
    /* 0x2f0 and 0x2f8: what the interface currently has picked, and whether
       the pick is settled. */
    int selection_kind;
    unsigned char unknown_2f4[4];
    unsigned char selection_settled;
    unsigned char unknown_2f9[3];
    /* 0x2fc..0x30b: the hover tooltip - when the pointer settled, that it is
       showing, and what it is over. Moving to anything else restarts the
       clock, which is what groups the four. */
    unsigned int tooltip_since;
    unsigned char tooltip_pending;
    unsigned char unknown_301[3];
    int tooltip_subject;
    int tooltip_kind;
} W8LevelRuntimeBlock;

#pragma pack(pop)

#ifdef __cplusplus
extern "C" {
#endif

/* 0x0068C09C holds a pointer to the loaded string table, one wide string per
   entry, which Strings.cpp fills from the localisation file. Bodies name
   entries by their byte offset into it, so the index is spelled as one. It was
   reached under five names before this - notices, a dialog message table, a
   plain dword and a message-string array - and one of those had it a level of
   indirection out, treating the pointer itself as the table.

   The two are named by the Strings.cpp assertions gppStringList and
   giStringListLen rather than described. */
extern wchar_t** gppStringList;
extern int giStringListLen;
/* 0x00517A90. One shared buffer, so the answer is only good until the next
   call - which is why every caller consumes it immediately. */
extern W8CharacterClassRecord* g_character_class_records; /* 0x0065BDE0 */
/* The gameplay globals reached from more than a couple of units. Each was
   declared separately in every file that used it, which is how two of them
   ended up with two names and two more with two types; one declaration each is
   what keeps that from happening again. */
extern unsigned char g_camp_open_00683f9b;
/* 0x00687599: one byte, and the image proves it. All 16 relocation sites that
   name the address touch it a byte at a time - fifteen `mov r8, byte ptr [..]`
   reads across seven functions and the single write `mov byte ptr [..], 1` in
   Function54B250 - and the globals census puts its extent at one byte, between
   a dword at 0x00687595 and another at 0x0068759A. It had been declared `int`
   in quarantine_common.h, `bool` in GameplayDatabase.cpp and `unsigned char` in
   LoadSaveGame.cpp, which emitted three different symbols for one address and
   only linked because /FORCE leaves them unresolved.
   It admits a save the autosave interval gate would otherwise refuse and
   selects the current slot over the fixed AutoSave name, so the recovered name
   stays positional until the meaning behind that pairing is established. */
extern unsigned char g_save_flag_00687599;
/* 0x00683FAD: every monster currently in the level. */
extern W8PList* g_active_monster_list_00683fad;
/* 0x0068EC78: which screen is up. The whole 0x98-byte block lives there; the
   leading id is what everything outside the screen code reads, which is why
   this is a union rather than two globals. */
/* 0x005EBB34: the float that stands for "no distance given", which the level
   vector reads as its absent value too. */
extern float g_float_005ebb34;
extern W8CombatCharacterRow* g_combat_character_rows;

/* Which screen is up, as g_screen_state_0068ec78.id holds it. Only the two the
   recovered bodies test are named. */
enum {
    W8_SCREEN_CAMP = 6,
    W8_SCREEN_MAIN_GAME = 7
};


extern W8FactionRuntimeRecord g_factions[21];
extern W8RaceResistanceProfile g_race_resistance_profiles[];
/* 0x00685178: one 0x106-byte row per party slot; only the leading byte is
   established, and GetRandomCharacter treats it as a slot-occupied flag. */
extern W8PartySlotRow* g_party_slot_rows;
/* Flat table indexed by skill_id * 15 + profession. */
extern int g_profession_skill_availability[0x29][15];
extern int g_profession_bonus_skills[15];
/* Four skill ids per profession. */
extern int g_profession_skills[15][4];
extern int g_profession_magic_level_offsets[15];
extern W8FactDatabaseRecord* g_fact_records;
extern unsigned char g_log_fact_checks;
extern unsigned char g_fact_values[1001];
/* Suppresses the journal entry and its sound in the fact-change recorder at
   0x005588F0, which consults this before displaying anything. InitializeFactState
   brackets its whole run of SetFact calls with it. */
extern unsigned char g_fact_notifications_suppressed;
/* The Wizardry 7 party import, read from Saves\\Import by the parser at
   0x00558D00 and named for that path. It stores up to six 0x248-byte character
   records, then unpacks one option byte and a 96-bit flag mask into the
   following. Set when an import loaded successfully. */
extern unsigned char g_import_party_loaded;
extern int g_import_character_count;
/* Four mutually exclusive high bits of the imported option byte, mapped to 0-3,
   or -1 when no import applies. InitializeFactState turns 1 and 2 into two
   different facts and everything else into a third. */
extern int g_import_ending_choice;
/* One byte per bit of the imported 96-bit mask. Only elements 5 and 11 are
   consumed so far, by InitializeFactState. */
extern unsigned char g_import_flags[0x60];
extern int g_fact_record_count;
extern W8ItemDatabaseRecord* g_item_records;
extern int g_item_record_count;
/* 0x00616E84: one entry per item category, giving the interface presentation
   of whatever spell the item carries. */
extern const int g_item_spell_presentation[];
/* 0x00648C5C: one entry per equipment slot; -1 marks a slot that has no
   interface position, which is what the unequip rule tests for. */
extern const int g_equip_slot_icons[];
/* 0x0068517C: gStatus.fGameStarted, named by the PC Item.cpp assertion at line
   3795 that guards the party-pool sort. Item placement consults it because the
   character-creation screens fill different slots than play does. */
extern unsigned char g_game_started;
/* 0x00685189: the party's purse. AddPartyGold plays Data\Sound\Misc\ChaChing.wav
   and posts the pickup message when it is told to announce; SpendPartyGold is
   the matching debit and floors at zero rather than wrapping. */
extern unsigned int g_party_gold;
/* gXStatus.uiMonstersInDatabase, named by the assertion at GameplayDatabase.cpp
   line 320. It sits immediately below the NPC count, so the database counts are
   fields of one gXStatus structure rather than separate globals; they are kept
   separate here until that structure is recovered. */
/* Two gXStatus members named by the MonsterManager.cpp assertions at lines 92
   and 93. Recovering the whole structure is tracked separately; until then its
   members stay separate externs. */
/* Reset together by ResetGameplayStatusBlock; their meaning is not established,
   so they keep address-positional names. The object at 0x00683FD7 is torn down
   by 0x0054B0B0 through operator delete after the same method's sibling. */
extern unsigned char g_status_block_685078[56];
#ifdef __cplusplus
extern W8StartupRuntimeState* g_startup_runtime_state;
#else
extern void* g_startup_runtime_state;
#endif
extern void* g_object_685067;
extern unsigned char g_party_moving_006850b5;
extern unsigned char g_surprise_possible_00683fc5;
/* Written by Targeting.cpp and reset to -1 here; meaning not established. */
extern int g_target_state_6840b3;
extern int g_picked_group_006840b7;
extern W8NpcDatabaseRecord* g_npc_records;
extern unsigned int g_npc_record_count;
extern W8ItemTableRecord** g_item_tables;
extern unsigned int g_item_table_count;
extern char** g_item_table_category_names;
extern unsigned int g_item_table_category_count;
extern W8EncounterTableRuntime** g_encounter_tables;
extern char** g_encounter_names;
extern unsigned int g_encounter_name_count;
/* 0x0060A6BC: the level whose encounter tables are loaded, -1 when none are. */
extern int g_encounter_tables_level;
extern unsigned int g_encounter_table_count;
extern W8LevelDatabaseRecord* g_level_records;
extern int g_level_record_count;
extern int g_loaded_level_id;
extern W8World* g_world;
/* 0x00659AB8: the second world. It was spelled `void*` where it is defined and
   `W8World*` where the viewport reaches through it, which is two claims about
   one object; the viewport's is the one a body proves, since it reads the
   camera member off it. One declaration here settles it. */
extern W8World* g_world_659ab8;
/* Defined by DisplayList.cpp, which owns the list this gates; the recovered
   clear at 0x0040C220 reads it from outside that unit, so the declaration sits
   here rather than being spelled a second time where it is used. */
extern unsigned char g_display_flag_650e90;
extern unsigned char g_item_in_hand_valid;
extern W8ItemInstance g_item_in_hand;
/* The party's carried pool, bounded by the count that follows it in memory;
   0x0054B100 walks it by address rather than by index. */
extern W8ItemInstance g_party_item_pool[];
extern int g_party_item_count;
/* Starting item ids, terminated by the address after the last, with 0xffffffff
   marking an empty slot. */
extern unsigned int g_starting_item_ids[];
extern unsigned int g_starting_item_ids_end[];
extern W8LevelFolderRecord g_level_folders[47];
/* 0x00686A70: the level the party is on. The save loader compares it against
   the level id in an LVLS chunk, and it indexes the per-level rows below -
   bounded at 0x2f, the same forty-seven the folder table has. */
/* 0x00686B7D: one row per level. Gold picked up there and the clock its sight
   state was last brought up to are the two fields established; the gold credit
   and the sight ageing reach the row four bytes apart, which is what makes it
   one row rather than two arrays. */
typedef struct W8LevelProgressRow {
    int gold_collected;                   /* 0x00 */
    int sight_clock;                      /* 0x04 */
    unsigned char unknown_08[0x19];
} W8LevelProgressRow;                     /* 0x21 */
extern W8LevelProgressRow g_level_progress[47];
extern int g_location_variable_count;
extern char** g_location_variable_names;
extern int g_location_variable_level_count;
extern int* g_location_variable_levels;
extern int g_next_world_item_id;
extern W8PList* g_world_item_list;
extern W8GrowableVector<W8NPCItemList*>* g_npc_item_lists;
/* 0x006840C7: lazily populated cache of runtime monster records, one slot per
   species ID. MAX_MONSTERS_IN_DATABASE comes from the canonical assertion text. */
/* Provisional name: a fixed-address, non-per-character item pool distinct
   from the equipped/carried slots GetOriginOfCharacterItem also searches. */
extern unsigned char g_shared_item_pool[];
extern unsigned int g_shared_item_pool_count;


/* 3D Code\IList.cpp: the integer sibling of W8PList, same shape with int
   elements, which is why a failed lookup returns -1 rather than null. */


unsigned int GetMonsterGroupIndexByID(
    int caller_line,
    const char* caller_file,
    int group_id,
    unsigned char assert_on_failure);
W8MonsterGroup* GetMonsterGroupByListIndex(unsigned int group_list_index);
void RecountActiveMonsterGroupMembers(W8MonsterGroup* monster_group);

typedef struct W8Monster W8Monster;
struct W8AnimObj;

#ifdef __cplusplus
enum { W8_MONSTER_CYCLE_COUNT = 27 };

/* Partial layout of the engine object referenced by cycle 18. The Monster
   wrappers at 0x004C5780..0x004C5AA0 establish the timestamp, animation state,
   pending cycle, and scale fields below. */
struct W8MonsterCycleRuntime {
    unsigned char unknown_000[0x66];
    unsigned short value_066;               /* 0x066: cleared when a cycle starts */
    unsigned int animation_timestamp;       /* 0x068 */
    unsigned char unknown_06c;
    unsigned char animating;                /* 0x06d */
    unsigned char unknown_06e[3];
    signed char behaviour;                   /* 0x071: asserted BEHAVIOUR_FIRST..LAST */
    unsigned char unknown_072[0x35];
    signed char pending_cycle;               /* 0x0a7 */
    unsigned char unknown_0a8[0x514];
    unsigned char flag_5bc;                 /* 0x5bc */
    unsigned char unknown_5bd[0x33];
    float scale;                             /* 0x5f0 */
    float minimum_scale;                     /* 0x5f4 */
    float maximum_scale;                     /* 0x5f8 */
};

struct W8MonsterCycle {
    union {
        unsigned int flags_00;              /* 0x00: cycle 19 bit 7 set by 0x004e67a0 */
        struct {
            unsigned char flag_00;
            unsigned char flag_01;
            unsigned char state_02;         /* 0x02: cycle 17 state saved across activation */
            unsigned char flag_03;
        } bytes;
    };
    union {
        unsigned int num_subs_04;           /* 0x04: IsCycleSupported tests the complete field */
        struct {
            unsigned char ubNumSubs;        /* 0x04: GetNumSubsPerCycle returns the low byte */
            unsigned char unknown_05[3];
        } count;
    };
    union {
        unsigned int value_08;              /* 0x08: copied by 0x004c5870 */
        struct {
            unsigned char unknown_08;
            unsigned char unknown_09;       /* 0x09: cleared for cycle 22 by 0x004e6130 */
            unsigned char unknown_0a[2];
        } bytes_08;
    };
    union {
        W8MonsterCycleRuntime* runtime;      /* 0x0c: cycle 18's shared engine state */
        W8AnimObj** animation_objects;       /* 0x0c: per-subcycle AnimObj table */
    };
};                                          /* 0x10 */

struct W8MonsterPolymorphicSubobject18 {
    /* 0x00, which is Monster +0x18: the additional vftable at 0x005ED218,
       five slots. The root-relative placement is proven; whether this
       polymorphic subobject is a base or an embedded member is not. */
    void* vptr;
    unsigned char unknown_04[8];
    unsigned int flags_0c;                  /* 0x0c: Monster +0x24 */
    unsigned char unknown_10[0x4c];
    int value_5c;                           /* 0x5c: Monster +0x74 */
    unsigned char unknown_60[0x20];
    signed char animation_index_80;          /* 0x80: Monster +0x98 */
    unsigned char unknown_81[3];
    /* 0x84, which is Monster +0x9c: the monster's own extent. Every range test
       subtracts it from the centre-to-centre distance, so it is a radius
       rather than a diameter or a bounding box. */
    float radius_84;
    unsigned char state_a0;                 /* 0x88: Monster +0xa0 */
    unsigned char unknown_89[3];
    signed char m_bCurrentCycle;             /* 0x8c: Monster +0xa4 */
    signed char current_subcycle_8d;         /* 0x8d: Monster +0xa5 */
    unsigned char unknown_8e[6];

    srVector3T<float> GetPosition();
};                                          /* 0x94: through the cycle array at Monster +0xac */

/* Partial source model of the Monster class (0x628 bytes, vtable 0x005ed200,
   constructor 0x004bea20). Only members proven by recovered consumers are
   modelled here; unresolved layout remains original-binary analysis. */
struct W8Monster {
    /* 0x000: the root vtable at 0x005ED200, six slots. Held as a field
       rather than declared through a virtual member, because giving this class
       virtuals would have VC6 synthesize its own vptr. The additional vptr at
       +0x18 cannot be expressed through inheritance or composition until its
       lifecycle role is recovered from the constructor family. */
    void* vptr;
    unsigned char unknown_004[0x0c];
    void* linked_objects_010;                /* 0x010: collection traversed by 0x004c5870 */
    unsigned char unknown_014[4];
    W8MonsterPolymorphicSubobject18 polymorphic_subobject_18; /* 0x018: proven placement */
    W8MonsterCycle m_cycles[W8_MONSTER_CYCLE_COUNT]; /* 0x0ac .. 0x25c */
    /* Two further runs of 27 adjacent 0x10-byte subobjects, the same shape as
       the array above. Nothing proves they are the same type, so they stay
       opaque rather than borrowing its name. */
    unsigned char subobject_array_25c[0x24];    /* 0x25c */
    W8MonsterFormation formation;               /* 0x280: from the owning group */
    unsigned char subobject_array_28c[0x180];   /* 0x28c */
    unsigned char subobject_array_40c[0x1b0];   /* 0x40c */
    unsigned char tail_fields_5bc[0x6c];        /* 0x5bc: fields seen through 0x624 */

    unsigned char GetNumSubsPerCycle(signed char bCycle);
    void SubmitCycleAnimValue004BF970(signed char cycle);
    unsigned char IsDying();
    unsigned char Function4C2CF0(signed char cycle);
    void Function4C50F0();
    int Function4C6A50();
    void Function4C6990(int value);
};

/* The constructor at 0x004BEA20 initialises through 0x624 and its sole caller
   allocates this much, so the extent is proven even though most of it is not.
   Asserting it here is what stops a field edit from silently shortening the
   object. */
static_assert(sizeof(W8Monster) == 0x628, "W8Monster_size_must_be_0x628");

static_assert(sizeof(W8MonsterCycle) == 0x10, "W8MonsterCycle_size_must_be_0x10");
static_assert(sizeof(W8MonsterPolymorphicSubobject18) == 0x94, "W8MonsterPolymorphicSubobject18_size_must_be_0x94");
#endif

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
    unsigned char unknown_00d[9];
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
    W8Monster* monster;                   /* 0x0c: live engine object, if any */
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
    unsigned char unknown_28b[3];
    int value_28e;                          /* 0x28e: cleared by the per-turn reset */
    unsigned char unknown_292[0x24];
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

void SetDice(W8Dice* dice, unsigned char count, unsigned char sides, short base);
int RollDice(const W8Dice* dice);
int IntegerPower(int base, unsigned int exponent);
void ClampInteger(int* value, int minimum, int maximum);
void ClampUnsignedInteger(unsigned int* value, unsigned int minimum,
                          unsigned int maximum);
int CompareUnsignedDescending(const unsigned int* first, const unsigned int* second);
int CompareSignedAscending(const int* first, const int* second);
int CompareSignedDescending(const int* first, const int* second);
char* FormatString(const char* format, ...);
wchar_t* FormatWideString(const wchar_t* format, ...);
wchar_t* ConvertStringToWide(const char* string);
char* ConvertWideStringToString(const wchar_t* string);
wchar_t* FormatUnsignedIntegerWithCommas(wchar_t* output, unsigned int value);
char* TitleCaseString(char* string);
float NormalizeAngle(float angle);
float ShortestAngleDistance(float first, float second);
int GetNextCharacter(int require_primary, int require_secondary, int previous_slot);
void FormatDebugMessage(int channel, const char* format, ...);
int GetSpellTargetType(int spell_id, unsigned char normalize_single_target);
#ifdef __cplusplus
bool CanSpellBackfire(int spell_id);
#else
unsigned char CanSpellBackfire(int spell_id);
#endif
int MinimumCasterLevelForSpellLevel(int spell_level);
int GetMinimumCasterLevelForSpell(int spell_id);
/* 0x00521EF0, the address CFAgent seeds. Answers whether the item found a
   home, which one of its two callers tests and the other ignores. */
bool AddItemToParty(W8ItemInstance* item, int arg_2, int arg_3);
W8FactionDisposition GetFactionDisposition(signed char faction);
void RegionSetEnable(unsigned int region_set_index);
void RegionSetDisable(unsigned int region_set_index);
void ClearRegionSetModeBits(unsigned int region_set_index);
void SetRegionSetMode4(unsigned int region_set_index);
void ClearRegionModeBits(unsigned int region_index);
void SetRegionMode4(unsigned int region_index);
void SetRegionBounds(unsigned int region_index, unsigned short x1, unsigned short y1,
                     unsigned short x2, unsigned short y2);
#ifdef __cplusplus
bool RegionContainsPoint(unsigned int region_index, unsigned short x, unsigned short y);
bool RegionHasFlags(unsigned int region_index, unsigned int flags);
#else
unsigned char RegionContainsPoint(unsigned int region_index, unsigned short x,
                                  unsigned short y);
unsigned char RegionHasFlags(unsigned int region_index, unsigned int flags);
#endif
unsigned int CreateRegionSet(void);
void ResetRegionSet(unsigned int region_set_index);
unsigned int AddRegionToSet(unsigned int region_set_index);
void SetRegionCallback(unsigned int region_index, W8RegionCallback callback,
                       unsigned short callback_id);
void SetRegionOwner(unsigned int region_index, void* owner);
void SetRegionHelp(unsigned int region_index, unsigned char enabled, int help_text_id);
unsigned char ClearActiveRegionIfMatches(unsigned int region_index);
int RPCPtrToPCSlot(const W8RPCSlot* rpc);
void FreeStringTable(void);
#ifdef __cplusplus
bool IsStringTableLoaded(void);
#else
unsigned char IsStringTableLoaded(void);
#endif
void UnionScreenRects(const W8ScreenRect* first, const W8ScreenRect* second,
                      W8ScreenRect* result);
unsigned char ScreenPointInRect(const W8ScreenRect* rect, const W8ScreenPoint* point);
void StripMonsterNameSuffix(W8WideChar* name);
unsigned int CharacterPointerToPartySlot(W8Character* character);
unsigned char IsPartyCharacterPointer(const W8Character* character);
int GetProfessionCasterLevel(W8Character* character, int profession_id);
unsigned char IsCharacterSkillAvailable(W8Character* character, unsigned int skill_id,
                                        const unsigned char* expert_realm_flags);
unsigned char GetFact(int fact_id);
void SetFact(int fact_id, unsigned char value, unsigned char suppress_side_effects);
void SaveFactState(int save_handle);
void InitializeFactState(void);
void LoadFactState(int save_handle);
void SetFactNotificationsSuppressed(unsigned char suppressed);
unsigned char InitializeFactDatabase(void);
unsigned char InitializeItemDatabase(void);
unsigned char InitializeLevelDatabase(void);
unsigned char InitializeItemTables(void);
void DestroyItemTables(void);
void Function54B100(void);
void Function54B560(void);
void Function54B300(unsigned int slot);
unsigned char InitializeNpcDatabase(void);
void DestroyNpcDatabase(void);
void DestroyFactDatabase(void);
void DestroyItemDatabase(void);
void DestroyLevelDatabase(void);
void FreeIfNotNull(void* block);
unsigned char AllocateStatusBuffers(W8StatusBuffers* status);
void FreeStatusBuffers(W8StatusBuffers* status);
void ResetPartySlotRow(int slot);
void ResetGameplayStatusBlock(void);
void ResetTargetingState(void);
void DestroyGameplayObjects(void);
bool CheckCdPresent(void);
int GetLoadedLevelID(void);
const char* LevelGetFolderNameByID(int level_id);
unsigned char LevelGetLocationCodeByID(int level_id, char* location_code);
W8MonsterRecord* MonsterDBFromSpecies(unsigned int monster_species);
void WorldUpdateProps(W8World* world);
unsigned char TargetSourceIsCharacter(const W8TargetSource* source, int allow_indirect);
unsigned char TargetSourceIsMonster(const W8TargetSource* source, int allow_indirect);
int GetRandomCharacter(int require_primary, int require_secondary, int excluded_slot,
                       signed char excluded_faction);
/* 0x0054A8A0, reviewed in evidence/reviewed/wiz8/functions.csv. */
unsigned char LoadMonsterDatabaseRecord(unsigned int monster_species, W8MonsterRecord* record);
int GetLocationIDFromCode(const char* location_code);
/* 0x0042A370, not yet recovered. */
unsigned char LevelBuildInfoByID(int level_id, W8LevelInfo* info);
W8World* GetWorld(void);
/* The second world the 3D API keeps, and the renderer's catch-up request.
   Both are defined in Engine Code\3dapi.cpp. */
W8World* GetWorld659AB8(void);                                           /* 0x004512A0 */
void MarkRendererReady(void);                                            /* 0x00451010 */
int GetItemInHand(void);
int GetLocationVarIDByName(const char* name);
W8MonsterGenerator* FindMonGenByName(const char* name);
W8NPCItemList* GetNPCItemListByID(int npc_record_id);
W8MonsterGroup* FindFirstMonsterByID(int monster_id);
W8MonsterGroup* FindNextExistingMonsterByID(int monster_id, W8MonsterGroup* previous);
void GetOriginOfCharacterItem(
    int character_index,
    void* item,
    unsigned char* origin,
    unsigned short* slot);
void AddLinesToMessageBox(int type, W8WideChar* text, void* extra);

#ifdef __cplusplus
}
#endif

#endif
