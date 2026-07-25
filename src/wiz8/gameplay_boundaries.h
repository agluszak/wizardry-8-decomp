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
    unsigned char unknown_006d[0x20];
    int profession_levels[15];            /* 0x008d */
    unsigned char unknown_00c9[0x1799];
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

typedef struct W8MonsterGenerator {
    unsigned char unknown_00[0x24];
    char name[32];                        /* 0x24 */
} W8MonsterGenerator;

typedef struct W8MonsterGeneratorVector {
    int unknown_00;
    int count;                            /* 0x04 */
    int capacity;
    W8MonsterGenerator** data;            /* 0x0c */
} W8MonsterGeneratorVector;

typedef struct W8World {
    unsigned char unknown_000[0xc4];
    W8MonsterGeneratorVector* monster_generators; /* 0xc4 */
} W8World;

typedef struct W8Vector3 {
    float x;
    float y;
    float z;
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

#ifdef __cplusplus
}
#endif

#endif
