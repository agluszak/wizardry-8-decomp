#include "wiz8/location_variables.h"

#include <string.h>

// FUNCTION: WIZ8 0x00444170
int GetLocationVarIDByName(const char* name)
{
    int variable_count = g_location_variable_count;
    int variable_id;
    char** variable_name;
    int* variable_level;

    for (variable_id = 0; variable_id < variable_count; ++variable_id) {
        variable_name = g_location_variable_names;
        if (variable_id < g_location_variable_count) {
            variable_name += variable_id;
        }
        if (_stricmp(*variable_name, name) == 0) {
            if (variable_id < g_location_variable_level_count) {
                variable_level = &g_location_variable_levels[variable_id];
            } else {
                variable_level = g_location_variable_levels;
            }
            if (*variable_level == g_loaded_level_id) {
                return variable_id;
            }
        }
    }
    return -1;
}
#include "wiz8/location_variables.h"
