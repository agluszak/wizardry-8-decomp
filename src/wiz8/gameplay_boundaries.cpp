#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"

#include <wchar.h>
#include <stdio.h>

extern unsigned char EvaluateFact(int fact_id);
/* Provisional semantic name for the journal/notification path at 0x005588f0. */
extern void RecordFactChangeForJournal(int fact_id);
extern void HandleFactChange(int fact_id, unsigned char value);

static __inline int MinimumCasterLevel(int spell_level)
{
    switch (spell_level) {
    case 2:
        return 3;
    case 3:
        return 5;
    case 4:
        return 8;
    case 5:
        return 11;
    case 6:
        return 14;
    case 7:
        return 18;
    default:
        return 1;
    }
}

// FUNCTION: WIZ8 0x004AC9D0
int GetSpellTargetType(int spell_id, unsigned char normalize_single_target)
{
    int target_type = g_spell_records[spell_id].target_type;

    if (target_type == 1 && normalize_single_target) {
        target_type = 0;
    }
    return target_type;
}

// FUNCTION: WIZ8 0x004ACB40
int MinimumCasterLevelForSpellLevel(int spell_level)
{
    return MinimumCasterLevel(spell_level);
}

// FUNCTION: WIZ8 0x004ACBA0
int GetMinimumCasterLevelForSpell(int spell_id)
{
    return MinimumCasterLevel(g_spell_records[spell_id].spell_level);
}



// FUNCTION: WIZ8 0x00506280
unsigned char GetFact(int fact_id)
{
    unsigned char value;
    wchar_t display_value[10];

    if (fact_id > 1000) {
        return 0;
    }

    value = EvaluateFact(fact_id);
    if (g_log_fact_checks) {
        if (value) {
            wcscpy(display_value, L"TRUE");
        } else {
            wcscpy(display_value, L"FALSE");
        }
        WriteGameLog(
            5,
            L"Checking fact %S which is %s",
            g_fact_records[fact_id].symbolic_name,
            display_value);
    }
    return value;
}

// FUNCTION: WIZ8 0x005061A0
void SetFact(int fact_id, unsigned char value, unsigned char suppress_side_effects)
{
    unsigned char previous_value;
    wchar_t display_value[10];

    if (fact_id > 1000) {
        return;
    }

    previous_value = g_fact_values[fact_id];
    g_fact_values[fact_id] = (unsigned char)value;

    if (fact_id < g_fact_record_count) {
        if (value) {
            sprintf((char*)display_value, "TRUE");
        } else {
            sprintf((char*)display_value, "FALSE");
        }
    }

    if (!suppress_side_effects) {
        if (g_fact_values[fact_id] != previous_value) {
            RecordFactChangeForJournal(fact_id);
        }
        HandleFactChange(fact_id, value);

        if (g_log_fact_checks) {
            if (value) {
                wcscpy(display_value, L"TRUE");
            } else {
                wcscpy(display_value, L"FALSE");
            }
            WriteGameLog(
                5,
                L"%S set to %s",
                g_fact_records[fact_id].symbolic_name,
                display_value);
        }
    }
}
