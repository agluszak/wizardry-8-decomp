#pragma once

#include "wiz8/layouts/encounter_tables.h"

unsigned int InitializeEncounterTables(void);
void UnloadEncounterTables(void);

extern W8GrowableVector<W8EncounterTableRuntime*> g_encounter_tables;
extern W8GrowableVector<char*> g_encounter_names;
extern int g_encounter_tables_level;

struct W8MonsterGroup;

void SaveEncounterState(int handle);
void SaveMonsterGenerators(int handle);
void RunMonsterGenerators(void);
void DespawnAllActiveMonsterGroups0048C9F0(void);
void ResetMonsterGeneratorTimers0048CBE0(void);

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
