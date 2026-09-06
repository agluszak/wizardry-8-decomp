#include "wiz8/location_variables.h"

#include <string.h>

// FUNCTION: WIZ8 0x00444170
int GetLocationVarIDByName(const char* name)
{
    int variable_count = g_location_variable_names_006598f8.GetCount();
    int variable_id;
    char** variable_name;
    int* variable_level;

    for (variable_id = 0; variable_id < variable_count; ++variable_id) {
        variable_name = g_location_variable_names_006598f8.GetAt(variable_id);
        if (_stricmp(*variable_name, name) == 0) {
            variable_level = g_location_variable_levels_006598e0.GetAt(variable_id);
            if (*variable_level == g_loaded_level_id) {
                return variable_id;
            }
        }
    }
    return -1;
}
#include "wiz8/location_variables.h"
