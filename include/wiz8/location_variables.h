#pragma once

extern "C" {

extern int g_loaded_level_id;
extern int g_location_variable_count;
extern char** g_location_variable_names;
extern int g_location_variable_level_count;
extern int* g_location_variable_levels;

int GetLocationVarIDByName(const char* name);

}
