#pragma once

#include "wiz8/layouts/encounter_tables.h"

unsigned int InitializeEncounterTables(void);

extern W8GrowableVector<W8EncounterTableRuntime*> g_encounter_tables;
extern W8GrowableVector<char*> g_encounter_names;
extern int g_encounter_tables_level;
