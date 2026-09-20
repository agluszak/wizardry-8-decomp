#pragma once

#include "wiz8/layouts/encounter_tables.h"
#include "wiz8/vector.h"

#pragma pack(push, 1)
struct W8EncounterTableRuntime {
    ~W8EncounterTableRuntime();

    W8GrowableVector<unsigned short> species_ids;          /* 0x000 */
    W8GrowableVector<unsigned char> rarity_class;          /* 0x010: 3, 7, 20, or 70 */
    W8GrowableVector<unsigned char> time_condition;        /* 0x020 */
    W8GrowableVector<unsigned char> challenge_level;       /* 0x030: 1 through 50 */
    W8Vector<W8EncounterScriptName*> script_names; /* 0x040 */
    char name[256];                                        /* 0x050 */
    unsigned int unknown_150;                              /* 0x150 */
    unsigned char version_two_flags;                       /* 0x154 */
    unsigned char padding_155[3];                          /* 0x155 */
}; /* 0x158 */
#pragma pack(pop)

static_assert(sizeof(W8EncounterTableRuntime) == 0x158,
              "W8EncounterTableRuntime_size_must_be_0x158");

unsigned int InitializeEncounterTables(void);
void UnloadEncounterTables(void);

extern W8GrowableVector<W8EncounterTableRuntime*> g_encounter_tables;
extern W8GrowableVector<char*> g_encounter_names;
extern int g_encounter_tables_level;

struct W8MonsterGroup;
struct MonGen;

/* MonGen.cpp's generator registry accessors. */
int GetMonsterGeneratorCount(void);                         /* 0x0048BD80 */
MonGen* GetMonsterGenerator(int index);         /* 0x0048BD90 */
void AddMonsterGenerator(MonGen* generator);    /* 0x0048BE30 */
void RemoveMonsterGenerator(MonGen* generator); /* 0x0048BEB0 */
W8EncounterTableRuntime* GetEncounterTable(int index);      /* 0x0048AD00 */
int FindEncounterTableByName(const char* name);              /* 0x0048CCA0 */

void SaveEncounterState(int handle);
void SaveMonsterGenerators(int handle);
void DestroyMonsterGenerators(void);
void LoadMonsterGenerators(int handle); /* 0x0048C470 */
void RunMonsterGenerators(void);
void DespawnAllActiveMonsterGroups0048C9F0(void);
void ResetMonsterGeneratorTimers0048CBE0(void);
void SetMonsterGeneratorDurationScale(float scale); /* 0x0048CB80 */

/* The encounter-budget registry used by random groups. */
void UnregisterActiveEncounterGroup(W8MonsterGroup* group); /* 0x0048C670 */
void RegisterActiveEncounterGroup(W8MonsterGroup* group);   /* 0x0048C750 */

void UpdateRandomEncounterBudget(unsigned char reset_budget);

extern int g_random_encounter_budget;
extern int g_random_encounter_limit;
extern W8GrowableVector<W8MonsterGroup*> g_active_groups;
extern unsigned char g_generator_save_flag;
extern short g_generator_default_interval;
extern short g_generator_interval_min;
extern short g_generator_interval_max;
extern int g_saved_encounter_budget;
extern int g_encounter_culling_time_seconds;
extern const float g_generator_jitter_fraction;
extern float g_encounter_culling_scale_fast;
extern const float g_encounter_culling_rate;
extern const float g_encounter_culling_distance;
