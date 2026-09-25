#pragma once

#include "wiz8/vector.h"

extern W8GrowableVector<char*> g_location_variable_names;
extern W8GrowableVector<int> g_location_variable_levels;
extern W8GrowableVector<int> g_location_variable_values;

int GetLocationVarIDByName(const char* name);
void SetTriggerVariableByName(const char* name, int value);
void CreateLocationVar(const char* name, int value); /* 0x00443DC0 */
int GetLocationVarValueByName(const char* name);     /* 0x004440D0 */
void SaveLocationVariables(int handle);
bool LoadLocationVariables(int handle);
