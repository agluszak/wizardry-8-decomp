#pragma once

#include "wiz8/vector.h"

extern W8GrowableVector<char*> g_location_variable_names_006598f8;
extern W8GrowableVector<int> g_location_variable_levels_006598e0;
extern W8GrowableVector<int> g_location_variable_values_00659990;

extern "C" {

extern int g_loaded_level_id;

int GetLocationVarIDByName(const char* name);

}
