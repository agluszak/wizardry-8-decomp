#ifndef WIZ8_GAMEPLAY_BOUNDARIES_H
#define WIZ8_GAMEPLAY_BOUNDARIES_H

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
    unsigned char unknown_0079[0x14];
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

/* One Data\Databases\ItemTables.DBS record. The record opens with a
   NUL-terminated table name, which FindItemTableByName compares against; the
   name's field width is not established, so only its start is typed. */
typedef struct W8ItemTableRecord {
    char name[1];                        /* 0x000: NUL-terminated, width unknown */
    unsigned char unknown_001[0x1f0];
} W8ItemTableRecord;                     /* 0x1f1 */

/* One Data\Databases\Items.DBS record, and its LEVELS.DBS sibling. As with
   W8MonsterRecord only the disk and runtime stride is established; the leading
   field is a display name. */
typedef struct W8ItemDatabaseRecord {
    unsigned char unknown_000[0x10d];
} W8ItemDatabaseRecord;                  /* 0x10d */

typedef struct W8LevelDatabaseRecord {
    unsigned char unknown_000[0xd8];
} W8LevelDatabaseRecord;                 /* 0xd8 */

/* One runtime DATABASES\MONSTERS.DBS record. The size is the tracked disk and
   runtime record size; the fields are typed by their consumers elsewhere. */
typedef struct W8MonsterRecord {
    unsigned char unknown_000[0x297];
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

typedef struct W8MonsterGroup {
    unsigned char unknown_00[0x18];
    int monster_id;                       /* 0x18 */
} W8MonsterGroup;

/* Hand-rolled growable pointer array, instantiated once per element type. The
   executable contains 75 distinct instantiations, each with its own one-entry
   vtable whose single virtual is the destructor; see
   docs/libraries/wiz8-foundation-types.md. The leading word is a VPTR, not
   padding: these are polymorphic C++ objects. Every decoded constructor
   allocates the object with operator new(0x10) and a backing store of
   operator new(capacity * 4) with capacity 5. */
typedef struct W8PtrVector {
    void* vptr;                           /* 0x00: one virtual, the destructor */
    int count;                            /* 0x04 */
    int capacity;                         /* 0x08 */
    void** data;                          /* 0x0c */
} W8PtrVector;                            /* 0x10 */

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

typedef struct W8MonsterInfo {
    int location_id;                      /* 0x00 */
    unsigned char unknown_04[4];
    unsigned int monster_species;         /* 0x08 */
    W8Monster* monster;                   /* 0x0c: live engine object, if any */
} W8MonsterInfo;

extern W8PList* g_monster_list;            /* gXStatus.plsMonsterList */
extern W8PList* g_unborn_monster_list;     /* gXStatus.plsUnbornMonsterList */

W8MonsterInfo* MonsterGetScriptPartByLocationIndex(unsigned int monster_list_index);
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
void StripMonsterNameSuffix(unsigned short* name);
unsigned int CharacterPointerToPartySlot(W8Character* character);
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
unsigned char InitializeNpcDatabase(void);
void DestroyNpcDatabase(void);
int FindItemTableByName(const char* name);
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
int GetLoadedLevelID(void);
const char* LevelGetFolderNameByID(int level_id);
unsigned char LevelGetLocationCodeByID(int level_id, char* location_code);
W8MonsterRecord* GetMonsterDataByID(unsigned int monster_species);
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
