#ifndef WIZ8_GAMEPLAY_BOUNDARIES_H
#define WIZ8_GAMEPLAY_BOUNDARIES_H

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

typedef struct W8Character {
    unsigned char unknown_0000[4];
    unsigned char in_party;              /* 0x0004 */
    unsigned char unknown_0005[0x64];
    int current_profession;               /* 0x0069 */
    unsigned char unknown_006d[8];
    int faction;                          /* 0x0075: compared against the caller's faction */
    unsigned char unknown_0079[0x14];
    int profession_levels[15];            /* 0x008d */
    unsigned char unknown_00c9[0xa38];
    /* 0x0b01 and 0x0b11 gate party-member selection: a slot is eligible when
       unknown_0b11 is non-zero and unknown_0b01 is below 0x12, and a second
       tier tests it against 0x0f. The thresholds look like a condition or
       status scale, but nothing here establishes the meaning, so they keep
       positional names. Both are unsigned: the canonical compares are JB/JBE,
       not JL/JE. */
    unsigned int unknown_0b01;            /* 0x0b01 */
    unsigned char unknown_0b05[0xc];
    unsigned int unknown_0b11;            /* 0x0b11 */
    unsigned char unknown_0b15[0xd4d];
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

typedef struct W8MonsterGeneratorVector {
    int unknown_00;
    int count;                            /* 0x04 */
    int capacity;
    W8MonsterGenerator** data;            /* 0x0c */
} W8MonsterGeneratorVector;

/* Member names and types at 0x08 and 0x48 come from the canonical assertion
   expressions "pWorld && pWorld->plsProps" (Engine Code\3d.cpp:344) and
   "pWorld->psrMeshes" (Engine Code\3dapi.cpp:446); the offsets come from the
   asserting bodies. The pls/psr prefixes are the original's own Hungarian
   coding for a PList and a SurRender object. */
typedef struct W8World {
    unsigned char unknown_000[8];
    void* plsProps;                       /* 0x008: PList of props */
    unsigned char unknown_00c[0x3c];
    void** psrMeshes;                     /* 0x048: allocated array of mesh pointers */
    unsigned char unknown_04c[0x78];
    W8MonsterGeneratorVector* monster_generators; /* 0xc4 */
} W8World;

typedef struct W8Vector3Double {
    double x;
    double y;
    double z;
} W8Vector3Double;

typedef struct W8Vector3 {
    float x;
    float y;
    float z;
#ifdef __cplusplus
    W8Vector3* VectorFromThreeFloats(double source_x, double source_y, double source_z);
    W8Vector3* Copy3DVector(const W8Vector3Double* source);
#endif
} W8Vector3;

typedef struct W8WorldItem {
    int runtime_id;                      /* 0x00 */
    void* unknown_04;
    unsigned char unknown_08;
    W8ItemInstance item;                 /* 0x09 */
    W8Vector3 position;                  /* 0x15 */
    unsigned char unknown_21[4];
    int unknown_25;
    unsigned int flags;                  /* 0x29 */
    unsigned char unknown_2d[8];
    int unknown_35;
    int sector_id;                       /* 0x39 */
    unsigned char unknown_3d[0x70];
} W8WorldItem;                           /* 0xad */

typedef struct W8NPCRecordRef {
    unsigned char unknown_00[0x58];
    int record_id;                        /* 0x058: matches W8NpcDatabaseRecord::record_id */
} W8NPCRecordRef;

typedef struct W8NPCItemList {
    unsigned char unknown_00[6];
    W8NPCRecordRef* npc_record;           /* 0x06 */
} W8NPCItemList;

typedef struct W8NPCItemListVector {
    int unknown_00;
    int count;                            /* 0x04 */
    int capacity;
    W8NPCItemList** data;                 /* 0x0c */
} W8NPCItemListVector;

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
extern unsigned char (*g_party_slot_rows)[0x106];
extern int g_profession_magic_level_offsets[15];
extern W8FactDatabaseRecord* g_fact_records;
extern unsigned char g_log_fact_checks;
extern unsigned char g_fact_values[1001];
extern int g_fact_record_count;
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
extern void* g_world_item_list;
extern W8NPCItemListVector* g_npc_item_lists;
/* 0x006840C7: lazily populated cache of runtime monster records, one slot per
   species ID. MAX_MONSTERS_IN_DATABASE comes from the canonical assertion text. */
extern W8MonsterRecord* g_monster_record_cache[1000];
extern void* g_monster_group_species_list;
extern void* g_monster_group_encounter_list;
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

unsigned int PListGetCount(void* list);
void* PListGetAt(void* list, unsigned int index);
int PListIndexOf(void* list, void* target);
W8MonsterGroup* GetMonsterGroupByID(unsigned int monster_id);

typedef struct W8MonsterInfo {
    int location_id;                      /* 0x00 */
} W8MonsterInfo;

extern void* g_monster_list;              /* gXStatus.plsMonsterList */
extern void* g_unborn_monster_list;       /* gXStatus.plsUnbornMonsterList */

W8MonsterInfo* MonsterGetScriptPartByLocationIndex(unsigned int monster_list_index);
unsigned int MonsterGetIndexByLocationID(
    int caller_line,
    const char* caller_file,
    int location_id,
    unsigned char assert_on_failure);

unsigned int GetRandomNumber(unsigned int upper_bound);
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
unsigned char GetFact(int fact_id);
void SetFact(int fact_id, unsigned char value, unsigned char suppress_side_effects);
int GetLoadedLevelID(void);
const char* LevelGetFolderNameByID(int level_id);
unsigned char LevelGetLocationCodeByID(int level_id, char* location_code);
W8MonsterRecord* GetMonsterDataByID(unsigned int monster_species);
void WorldUpdateProps(W8World* world);
unsigned char TargetSourceIsCharacter(const W8TargetSource* source, int allow_indirect);
unsigned char TargetSourceIsMonster(const W8TargetSource* source, int allow_indirect);
int GetRandomCharacter(int require_primary, int require_secondary, int excluded_slot,
                       signed char excluded_faction);
/* 0x0054A8A0, reviewed in config/analysis/functions/wiz8-formats.csv. */
unsigned char LoadMonsterDatabaseRecord(unsigned int monster_species, W8MonsterRecord* record);
int GetLocationIDFromCode(const char* location_code);
/* 0x0042A370, not yet recovered. */
unsigned char LevelBuildInfoByID(int level_id, W8LevelInfo* info);
W8World* GetWorld(void);
int GetItemInHand(void);
int GetLocationVarIDByName(const char* name);
W8MonsterGenerator* FindMonGenByName(const char* name);
W8WorldItem* CreateWorldItem(
    const W8ItemInstance* item,
    const W8Vector3* position,
    int unknown,
    unsigned char add_to_world);
W8WorldItem* SpawnItem(
    int item_id,
    const W8Vector3* position,
    int unknown,
    unsigned char add_to_world);
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
