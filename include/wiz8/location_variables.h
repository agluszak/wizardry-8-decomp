#pragma once

#include "wiz8/vector.h"

extern W8GrowableVector<char*> g_location_variable_names_006598f8;
extern W8GrowableVector<int> g_location_variable_levels_006598e0;
extern W8GrowableVector<int> g_location_variable_values_00659990;

int GetLocationVarIDByName(const char* name);
void SetTriggerVariableByName00444030(const char* name, int value);
