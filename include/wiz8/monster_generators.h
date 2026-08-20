#pragma once

#include "wiz8/layouts/encounter_tables.h"

extern "C" {

extern W8EncounterTableRuntime** g_encounter_tables;
extern char** g_encounter_names;
extern unsigned int g_encounter_name_count;
extern int g_encounter_tables_level;
extern unsigned int g_encounter_table_count;

}
