#ifndef WIZ8_GAMEPLAY_BOUNDARIES_H
#define WIZ8_GAMEPLAY_BOUNDARIES_H

#include "wiz8/item_tables.h"
#ifdef __cplusplus
#include "surrender/srMath.h"
#endif

/* Shared recovered Wizardry interfaces used by matching translation units. */

#pragma pack(push, 1)

typedef struct W8Dice {
    short base;
    unsigned char count;
    unsigned char sides;
} W8Dice;

typedef struct W8SpellRuntimeRecord {
    unsigned char unknown_000[0x56];
    int spell_level;                    /* 0x056 */
    unsigned char unknown_05a[0xdd];
    int target_type;                    /* 0x137 */
    unsigned char unknown_13b[0x84];
} W8SpellRuntimeRecord;                 /* 0x1bf */

typedef struct W8FactionRuntimeRecord {
    signed char disposition_score;      /* 0x00 */
    unsigned char unknown_01[0x0d];
} W8FactionRuntimeRecord;               /* 0x0e */

/* One attribute record. The array is indexed by skill id biased by 0x22, so the
   seven attribute ids sit at the top of the skill numbering; only the leading
   value, which IsCharacterSkillAvailable tests against 100, is established. */
typedef struct W8CharacterAttribute {
    unsigned int value;                   /* 0x00 */
    unsigned char unknown_04[0x10];
} W8CharacterAttribute;                   /* 0x14 */

/* One skill record, indexed directly by skill id. PracticeCharacterSkill
   establishes the stride and the leading flag it sets when a skill first
   becomes available; IsCharacterSkillAvailable reads the same flag. */
typedef struct W8CharacterSkill {
    unsigned char flag_00;                /* 0x00 */
    unsigned char unknown_01[0x25];
} W8CharacterSkill;                       /* 0x26 */

typedef struct W8Character {
    unsigned char unknown_0000[4];
    unsigned char in_party;              /* 0x0004 */
    unsigned char unknown_0005[0x64];
    int current_profession;               /* 0x0069 */
    unsigned char unknown_006d[8];
    int faction;                          /* 0x0075: compared against the caller's faction */
    unsigned char unknown_0079[0x10];
    unsigned int level;                   /* 0x0089: averaged across occupied slots */
    int profession_levels[15];            /* 0x008d */
    unsigned char unknown_00c9[0x1c];
    W8CharacterAttribute attributes[7];   /* 0x00e5, indexed by skill_id - 0x22 */
    unsigned char unknown_0171[0x2c];
    W8CharacterSkill skills[0x29];        /* 0x019d, indexed by skill_id */
    unsigned char unknown_07b3[0x34e];
    /* 0x0b01 and 0x0b11 gate party-member selection: a slot is eligible when
       unknown_0b11 is non-zero and unknown_0b01 is below 0x12, and a second
       tier tests it against 0x0f. The thresholds look like a condition or
       status scale, but nothing here establishes the meaning, so they keep
       positional names. Both are unsigned: the canonical compares are JB/JBE,
       not JL/JE. */
    unsigned int unknown_0b01;            /* 0x0b01 */
    unsigned char unknown_0b05[0xc];
    unsigned int unknown_0b11;            /* 0x0b11 */
    unsigned char unknown_0b15[0x2d8];
    unsigned int skill_unlocks[0x29];     /* 0x0ded, indexed by skill_id */
    unsigned char unknown_0e91[0x9d1];
} W8Character;                           /* 0x1862 */

typedef struct W8FactDatabaseRecord {
    unsigned int identifier;
    char symbolic_name[256];             /* 0x004 */
    unsigned short description[106];     /* 0x104 */
} W8FactDatabaseRecord;                  /* 0x1d8 */

typedef struct W8ItemInstance {
    int item_id;
    unsigned char quantity;
    unsigned char charges;
    unsigned char identified;
    unsigned char unknown_07[4];
    unsigned char flag_0b;
} W8ItemInstance;                        /* 0x0c */

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
    unsigned char unknown_0d8[0x10];
    unsigned char field_0e8;
    unsigned char unknown_0e9[0x2f];
} W8MonsterSlot;                         /* 0x118 */

/* The game settings block that 0x0054B560 clears and fills with defaults. Every
   address that function writes falls inside the 0xa4 bytes it clears first, so
   this is one structure rather than a run of separate globals. Only offsets are
   established - nothing here names a setting - so the fields keep positional
   names. */
typedef struct W8GameSettings {
    unsigned char field_000;             /* 0x000 */
    unsigned char field_001;             /* 0x001 */
    unsigned char unknown_002[0x4];
    int field_006;                       /* 0x006 */
    unsigned char field_00a;             /* 0x00a */
    unsigned char field_00b;             /* 0x00b */
    unsigned char field_00c;             /* 0x00c */
    int field_00d;                       /* 0x00d */
    int field_011;                       /* 0x011 */
    int field_015;                       /* 0x015 */
    int field_019;                       /* 0x019 */
    int field_01d;                       /* 0x01d */
    int field_021;                       /* 0x021 */
    int field_025;                       /* 0x025 */
    unsigned char field_029;             /* 0x029 */
    unsigned char field_02a;             /* 0x02a */
    unsigned char field_02b;             /* 0x02b */
    unsigned char field_02c;             /* 0x02c */
    unsigned char unknown_02d[0x1];
    unsigned char field_02e;             /* 0x02e */
    unsigned char field_02f;             /* 0x02f */
    unsigned char field_030;             /* 0x030 */
    unsigned char field_031;             /* 0x031 */
    unsigned char field_032;             /* 0x032 */
    unsigned char field_033;             /* 0x033 */
    unsigned char field_034;             /* 0x034 */
    unsigned char field_035;             /* 0x035 */
    unsigned char field_036;             /* 0x036 */
    int field_037;                       /* 0x037 */
    unsigned char field_03b;             /* 0x03b */
    int field_03c;                       /* 0x03c */
    unsigned char field_040;             /* 0x040 */
    unsigned char field_041;             /* 0x041 */
    unsigned char field_042;             /* 0x042 */
    unsigned char field_043;             /* 0x043 */
    unsigned char unknown_044[0x1];
    unsigned char field_045;             /* 0x045 */
    unsigned char unknown_046[0x1];
    unsigned char field_047;             /* 0x047 */
    unsigned char field_048;             /* 0x048 */
    unsigned char field_049;             /* 0x049 */
    unsigned char field_04a;             /* 0x04a */
    unsigned char field_04b;             /* 0x04b */
    unsigned char field_04c;             /* 0x04c */
    unsigned char field_04d;             /* 0x04d */
    unsigned char field_04e;             /* 0x04e */
    unsigned char field_04f;             /* 0x04f */
    unsigned char field_050;             /* 0x050 */
    unsigned char unknown_051[0x53];
} W8GameSettings;                        /* 0x0a4 */

/* One party slot row. Only the three fields the reset touches are established,
   plus the leading flag UtilityFunctions reads as slot-occupied. */
typedef struct W8PartySlotRow {
    unsigned char flag_00;               /* 0x000: slot occupied */
    unsigned char unknown_001[0x74];
    int unknown_075;                     /* 0x075 */
    unsigned char unknown_079[0x57];
    unsigned char flag_0d0;              /* 0x0d0: reset to 0xff */
    unsigned char unknown_0d1[0x35];
} W8PartySlotRow;                        /* 0x106 */

/* The two heap buffers a status block owns. GetSaveGameLevel builds one of
   these on the stack, reads through it and tears it down again; only the two
   pointers this pair manages are established, not the block's full extent. */
typedef struct W8StatusBuffers {
    unsigned char unknown_00[4];
    void* buffer_04;                     /* 0x04: 0xc310 bytes */
    void* buffer_08;                     /* 0x08: 0x830 bytes */
} W8StatusBuffers;

/* The global status block. It opens with the same two heap buffers
   AllocateStatusBuffers manages on a caller-supplied one, and 0x0054AF30 clears
   the whole block - pointers included - before allocating fresh ones. */
typedef struct W8GlobalStatus {
    W8StatusBuffers buffers;             /* 0x0000 */
    unsigned char unknown_000c[0x49b6];
} W8GlobalStatus;                        /* 0x49c2 */

/* One Data\Databases\NPC.DBS record. Only the stride and the sub-list pointer
   the loader fills and the destructor tears down are established. */
typedef struct W8NpcDatabaseRecord {
    unsigned short count_00;             /* 0x000: sub-list loads only when this exceeds 1 */
    unsigned char unknown_002[0x9b];
    unsigned char flag_9d;               /* 0x09d: and only when this is clear */
    unsigned char unknown_09e[0x22c];
    void* sub_list;                      /* 0x2ca */
    unsigned char unknown_2ce[0x3b];
} W8NpcDatabaseRecord;                   /* 0x309 */

/* One Data\Databases\LEVELS.DBS record. Only the disk and runtime stride is
   established; the leading field is a display name. */
typedef struct W8LevelDatabaseRecord {
    unsigned char unknown_000[0xd8];
} W8LevelDatabaseRecord;                 /* 0xd8 */

/* One runtime DATABASES\MONSTERS.DBS record. The size is the tracked disk and
   runtime record size; the fields are typed by their consumers elsewhere. */
typedef struct W8MonsterRecord {
    unsigned char unknown_000[0xd1];
    unsigned char attribute_values_d1[5]; /* 0x0d1: converted by 0x004e5d00 */
    unsigned char unknown_0d6[0xab];
    unsigned int combat_value_181;         /* 0x181: combat-strength/display value */
    unsigned char unknown_185[4];
    char cycle_name_189[0x31];             /* 0x189: GrCycle lookup key */
    float float_1ba;                      /* 0x1ba: scaled by 0x005ed4f0 */
    unsigned char unknown_1be[0x95];
    int value_253;                        /* 0x253: selected by 0x004e5b50 */
    int value_257;                        /* 0x257: alternate selected value */
    unsigned char unknown_25b[0xc];
    /* 0x267, already carried by config/types/wiz8/gameplay_databases.h as the
       canonical applied type; carved out here because LoadMonsterGroup skips
       every live-group step for a record that has it set. */
    unsigned char deleted;                /* 0x267 */
    unsigned char unknown_268[3];
    unsigned int combat_value_override_26b; /* 0x26b: nonzero override */
    unsigned char unknown_26f[0x28];
} W8MonsterRecord;                       /* 0x297 */

/* Filled by 0x0042A370 from a level-table row; only its size is established
   here, by the 0x458-byte stack frame of its sole recovered caller. */
typedef struct W8LevelInfo {
    unsigned char unknown_000[0x458];
} W8LevelInfo;                           /* 0x458 */

typedef struct W8MonsterGenerator {
    unsigned char unknown_00[0x24];
    char name[32];                        /* 0x24 */
} W8MonsterGenerator;

/* The stride is the record LoadMonsterGroup allocates, zeroes and reads whole,
   and which its own assertion spells sizeof(*pMonsterGroup). Only the fields
   that loader establishes are named; the rest stays opaque. */
typedef struct W8MonsterGroup {
    unsigned char unknown_00[4];
    int unknown_04;                       /* 0x04: cleared after the record loads */
    struct W8IList* monsters;             /* 0x08: fresh IList per live group */
    unsigned char unknown_0c[8];
    int unknown_14;                       /* 0x14: cleared after the record loads */
    int monster_id;                       /* 0x18 */
    unsigned char unknown_1c[0xc];
    unsigned char flag_28;                /* 0x28: cleared after the record loads */
    unsigned char flag_29;                /* 0x29: cleared after the record loads */
    unsigned char unknown_2a[0x79];
    int unknown_a3;                       /* 0xa3: gates the trailing notification */
    unsigned char unknown_a7[0x1c];
    unsigned char flag_c3;                /* 0xc3: gates the trailing notification */
    /* 0xc4 is a saved-record version: at 2 and above the loader reads one more
       byte, and below 3 it clears flag_ca that older saves never wrote. */
    unsigned int version;                 /* 0xc4 */
    unsigned char unknown_c8[2];
    unsigned char flag_ca;                /* 0xca */
    unsigned char unknown_cb[0x60];
} W8MonsterGroup;                         /* 0x12b */

/* 3D Code\PList.cpp. Distinct from W8PtrVector: no vptr, elements at +0x00 and
   count at +0x08, accessed through free functions. */
typedef struct W8PList {
    void** data;                          /* 0x00 */
    int capacity;                         /* 0x04: PListInit allocates 10 */
    int count;                            /* 0x08 */
} W8PList;

/* Member names and types at 0x08 and 0x48 come from the canonical assertion
   expressions "pWorld && pWorld->plsProps" (Engine Code\3d.cpp:344) and
   "pWorld->psrMeshes" (Engine Code\3dapi.cpp:446); the offsets come from the
   asserting bodies. The pls/psr prefixes are the original's own Hungarian
   coding for a PList and a SurRender object. */
typedef struct W8World {
    unsigned char unknown_000[8];
    W8PList* plsProps;                    /* 0x008: PList of props */
    unsigned char unknown_00c[0x3c];
    void** psrMeshes;                     /* 0x048: allocated array of mesh pointers */
    unsigned char unknown_04c[0x78];
    W8PtrVector* monster_generators;      /* 0xc4: elements are W8MonsterGenerator* */
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

typedef struct W8MessageBoxLine {
    int unknown_00;                       /* initialized to -1 */
    int unknown_04;
    int unknown_08;
    int type;                             /* 0x0c: message category, e.g. combat log channel */
    unsigned short* text;                 /* 0x10 */
    int unknown_14;
    int unknown_18;
    void* extra;                          /* 0x1c: caller-owned payload, e.g. format-arg storage */
    int sequence;                         /* 0x20: copied from a running global counter */
} W8MessageBoxLine;                       /* 0x24 */

/* Local Code\Targeting.cpp. Field names and the BAD_INDEX sentinel come from
   the canonical assertions at lines 3299, 3307, 3320 and 3328; offsets come
   from the asserting bodies. iType 1 selects the character, 2 the monster, and
   3 either, which is why the type-3 path additionally requires a backfire or
   reflection flag. */
typedef struct W8TargetSource {
    int iType;                            /* 0x00 */
    int iChar;                            /* 0x04 */
    int iMonsterID;                       /* 0x08 */
    unsigned char unknown_0c[0x0f];
    unsigned char fReflection;            /* 0x1b */
    unsigned char fBackfire;              /* 0x1c */
} W8TargetSource;

typedef unsigned char W8FactionDisposition;

/* Local Code\UtilityFunctions.cpp. These are the signed screen-space rectangle
   and point shapes consumed by 0x00517E20 and 0x00517E70. */
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

/* Local Code\RegionManager.cpp. The unit's indexing expressions prove the
   strides; this first cluster only establishes the leading state fields. */
typedef struct W8RegionSet {
    unsigned int enabled;
    unsigned int first_region;
    unsigned int last_region;
} W8RegionSet;                           /* 0x0c */

typedef struct W8Region {
    unsigned int flags;
    short x1;
    short y1;
    short x2;
    short y2;
    unsigned char unknown_0c[0x10];
} W8Region;                              /* 0x1c */

enum {
    W8_FACTION_HOSTILE = 0,
    W8_FACTION_NEUTRAL = 1,
    W8_FACTION_FRIENDLY = 2
};

#pragma pack(pop)

#ifdef __cplusplus
extern "C" {
#endif

extern W8SpellRuntimeRecord* g_spell_records;
extern W8FactionRuntimeRecord g_factions[21];
extern W8Character* g_party_characters;
extern unsigned int g_region_set_count;  /* guiRegsetCount */
extern W8RegionSet g_region_sets[];
extern unsigned int g_region_count;      /* guiRegionCount */
extern W8Region g_regions[];
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
/* gXStatus.uiMonstersInDatabase, named by the assertion at GameplayDatabase.cpp
   line 320. It sits immediately below the NPC count, so the database counts are
   fields of one gXStatus structure rather than separate globals; they are kept
   separate here until that structure is recovered. */
extern unsigned int g_monster_record_count;
/* Two gXStatus members named by the MonsterManager.cpp assertions at lines 92
   and 93. Recovering the whole structure is tracked separately; until then its
   members stay separate externs. */
extern W8PList* g_monster_list;               /* gXStatus.plsMonsterList */
extern W8PList* g_monster_group_list;         /* gXStatus.plsMonsterGroupList */
/* Reset together by ResetGameplayStatusBlock; their meaning is not established,
   so they keep address-positional names. The object at 0x00683FD7 is torn down
   by 0x0054B0B0 through operator delete after the same method's sibling. */
extern unsigned char g_status_block_685078[56];
extern void* g_object_683fd7;
extern unsigned char g_flag_6850b5;
extern unsigned char g_flag_683fc5;
/* Written by Targeting.cpp and reset to -1 here; meaning not established. */
extern int g_target_state_6840b3;
extern int g_target_state_6840b7;
extern W8NpcDatabaseRecord* g_npc_records;
extern unsigned int g_npc_record_count;
extern W8ItemTableRecord** g_item_tables;
extern unsigned int g_item_table_count;
extern char** g_item_table_category_names;
extern unsigned int g_item_table_category_count;
extern W8LevelDatabaseRecord* g_level_records;
extern int g_level_record_count;
extern int g_loaded_level_id;
extern W8World* g_world;
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
extern int g_location_variable_count;
extern char** g_location_variable_names;
extern int g_location_variable_level_count;
extern int* g_location_variable_levels;
extern int g_next_world_item_id;
extern W8PList* g_world_item_list;
/* Elements are W8NPCItemList*. */
extern W8PtrVector* g_npc_item_lists;
/* 0x006840C7: lazily populated cache of runtime monster records, one slot per
   species ID. MAX_MONSTERS_IN_DATABASE comes from the canonical assertion text. */
extern W8MonsterRecord* g_monster_record_cache[1000];
extern W8PList* g_monster_group_species_list;
extern W8PList* g_monster_group_encounter_list;
/* Provisional name: a fixed-address, non-per-character item pool distinct
   from the equipped/carried slots GetOriginOfCharacterItem also searches. */
extern unsigned char g_shared_item_pool[];
extern unsigned int g_shared_item_pool_count;

extern W8MessageBoxLine** g_message_box_lines;
extern int g_message_box_line_count;
extern int g_message_box_line_capacity;
/* Provisional name: a running counter stamped onto each new line's `sequence`
   field; distinct from g_message_box_line_count. */
extern int g_message_sequence;


/* 3D Code\IList.cpp: the integer sibling of W8PList, same shape with int
   elements, which is why a failed lookup returns -1 rather than null. */
typedef struct W8IList {
    int* data;                            /* 0x00 */
    int capacity;                         /* 0x04: IListInit allocates 10 */
    int count;                            /* 0x08 */
} W8IList;

W8IList* IListCreate(void);
unsigned char IListInit(W8IList* pls);
int IListAdd(W8IList* pls, int value);
unsigned char IListDestroy(W8IList* pls);
int IListIndexOf(W8IList* pls, int value);
unsigned char IListFreeData(W8IList* pls);
void IListClear(W8IList* pls);
int IListRemove(W8IList* pls, int value);
int IListGetAt(W8IList* pls, int index);

W8PList* PListCreate(void);
unsigned char PListInit(W8PList* ppl);
unsigned char PListDestroy(W8PList* ppl);
unsigned char PListFreeData(W8PList* ppl);
int PListAdd(W8PList* ppl, void* pEntry);
int PListInsert(W8PList* ppl, int position, void* pEntry);
void PListClear(W8PList* ppl);
void* PListRemove(W8PList* ppl, void* pEntry);
void* PListRemoveAt(W8PList* ppl, int position);
unsigned int PListGetCount(W8PList* ppl);
void* PListGetAt(W8PList* ppl, int index);
int PListIndexOf(W8PList* ppl, void* pEntry);
W8MonsterGroup* GetMonsterGroupByID(unsigned int monster_id);

typedef struct W8Monster W8Monster;

#ifdef __cplusplus
enum { W8_MONSTER_CYCLE_COUNT = 27 };

struct W8MonsterCycle {
    unsigned char unknown_00[4];
    unsigned char ubNumSubs;                /* 0x04 */
    unsigned char unknown_05[4];
    unsigned char unknown_09;               /* 0x09: cleared for cycle 22 by 0x004e6130 */
    unsigned char unknown_0a[2];
    signed char* unknown_0c;                /* 0x0c: cycle 18 pointee byte +0xa7 read by 0x004e60b0 */
};                                          /* 0x10 */

struct W8MonsterMember18 {
    unsigned char unknown_00[0x0c];
    unsigned int flags_0c;                  /* 0x0c: Monster +0x24 */
    unsigned char unknown_10[0x4c];
    int value_5c;                           /* 0x5c: Monster +0x74 */
    unsigned char unknown_60[0x28];
    unsigned char unknown_88;               /* 0x88: Monster +0xa0 */
    unsigned char unknown_89[3];
    signed char m_bCurrentCycle;             /* 0x8c: Monster +0xa4 */
    unsigned char unknown_8d[7];

    srVector3T<float> Function4534C0();
};                                          /* 0x94: through the cycle array at Monster +0xac */

struct W8Monster {
    unsigned char unknown_000[0x18];
    W8MonsterMember18 member_18;            /* 0x018: positional member proven by 0x004e4600 */
    W8MonsterCycle m_cycles[W8_MONSTER_CYCLE_COUNT]; /* 0x0ac .. 0x25c */

    unsigned char GetNumSubsPerCycle(signed char bCycle);
    unsigned char Function4CA4C0();
    int Function4C6A50();
    void Function4C6990(int value);
};

typedef char W8MonsterCycle_size_must_be_0x10[
    sizeof(W8MonsterCycle) == 0x10 ? 1 : -1];
typedef char W8MonsterMember18_size_must_be_0x94[
    sizeof(W8MonsterMember18) == 0x94 ? 1 : -1];
#endif

#pragma pack(push, 1)
typedef struct W8MonsterInfo {
    int location_id;                      /* 0x00 */
    int monster_group_id;                 /* 0x04: group lookup input in 0x004e6020 */
    unsigned int monster_species;         /* 0x08 */
    W8Monster* monster;                   /* 0x0c: live engine object, if any */
    unsigned char unknown_10[4];
    unsigned char flag_14;                /* 0x14: live-entry gate in 0x004e5c00 */
    unsigned char unknown_15[0x12];
    int hp_max;                           /* 0x27: uiHPMax in the Targeting.cpp assertion */
    int hp_current;                       /* 0x2b: reduced by canonical damage consumers */
    unsigned char unknown_2f[0x70];
    int value_9f;                         /* 0x09f: zero gate in 0x004e5c00 */
    unsigned char unknown_a3[0x144];
    signed char attribute_adjustments_1e7[7]; /* 0x1e7: indexed through a five-way map */
    unsigned char unknown_1ee[0x59];
    unsigned char converted_attributes_247[5]; /* 0x247: values clamped to 1..125 */
    unsigned char unknown_24c[2];
    unsigned char flag_24e;                 /* 0x24e: state transition gate in 0x004e60b0 */
    unsigned char unknown_24f[4];
    unsigned char flag_253;                 /* 0x253: set by 0x004e5c00 after processing */
    unsigned char unknown_254;
    unsigned char flag_255;                 /* 0x255: reset by 0x004e5ea0 and 0x004e6020 */
    unsigned char unknown_256[0x84];
    int value_2da;                          /* 0x2da: nonzero gate in 0x004e5c00 */
    unsigned char unknown_2de[0x1f];
    int value_2fd;                          /* 0x2fd: transition state in 0x004e6020 */
    unsigned char unknown_301[0x124];
} W8MonsterInfo;                          /* 0x425 */
#pragma pack(pop)

typedef char W8MonsterInfo_size_must_be_0x425[
    sizeof(W8MonsterInfo) == 0x425 ? 1 : -1];

extern W8PList* g_monster_list;            /* gXStatus.plsMonsterList */
extern W8PList* g_unborn_monster_list;     /* gXStatus.plsUnbornMonsterList */
extern float g_monster_record_float_scale; /* 0x005ed4f0, provisional name */
extern int g_monster_info_iterator_index;  /* 0x00683698, retained cursor */

W8MonsterInfo* MonsterGetScriptPartByLocationIndex(unsigned int monster_list_index);
void Function4E4600(W8MonsterInfo* monster_info);
void Function4E4690(W8MonsterInfo* monster_info, int value);
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
int Function4E5B50(unsigned int monster_species);
void Function4E5C00(unsigned char value);
void ConvertMonsterAttributes(W8MonsterInfo* monster_info);
W8MonsterInfo* FindMonsterInfoBySpecies(unsigned int monster_species);
void Function4E5EA0(void);
void Function4E6020(W8MonsterInfo* monster_info, int value);
void Function4E60B0(W8MonsterInfo* monster_info, unsigned char value);
void MoveMonsterToLiveList(W8MonsterInfo* monster_info);
unsigned int GetMonsterCombatValue(const W8MonsterRecord* record);
unsigned char Function4E68C0(void);

unsigned int Random(unsigned int upper_bound);
int RollDice(const W8Dice* dice);
int IntegerPower(int base, unsigned int exponent);
void ClampInteger(int* value, int minimum, int maximum);
int GetSpellTargetType(int spell_id, unsigned char normalize_single_target);
#ifdef __cplusplus
bool CanSpellBackfire(int spell_id);
#else
unsigned char CanSpellBackfire(int spell_id);
#endif
int MinimumCasterLevelForSpellLevel(int spell_level);
int GetMinimumCasterLevelForSpell(int spell_id);
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
void UnionScreenRects(const W8ScreenRect* first, const W8ScreenRect* second,
                      W8ScreenRect* result);
unsigned char ScreenPointInRect(const W8ScreenRect* rect, const W8ScreenPoint* point);
void StripMonsterNameSuffix(unsigned short* name);
unsigned int CharacterPointerToPartySlot(W8Character* character);
unsigned char IsPartyCharacterPointer(const W8Character* character);
void AdjustByteByPercent(unsigned char* value, unsigned int percent);
void AdjustIntegerByPercent(unsigned int* value, unsigned int percent);
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
void AddLinesToMessageBox(int type, unsigned short* text, void* extra);

#ifdef __cplusplus
}
#endif

#endif
