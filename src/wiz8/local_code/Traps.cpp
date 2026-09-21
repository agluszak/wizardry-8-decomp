#include "wiz8/local_code/Traps.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/Targeting.h"
#include "wiz8/engine_code/Spells.h"
#include "wiz8/engine_code/Navigator.h"
#include "wiz8/startup_world.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/layouts/character.h"
#include "wiz8/vector.h"
#include "wiz8/sr_api.h"
#include "random.h"

// GLOBAL: WIZ8 0x0069da6c
unsigned char g_flag_69da6c;
// GLOBAL: WIZ8 0x0069da68
int g_value_69da68;
// GLOBAL: WIZ8 0x00650434
unsigned char g_table_650434[15][8] = {
    {0, 1, 0, 0, 0, 1, 0, 0}, {0, 1, 0, 1, 0, 0, 0, 0}, {0, 0, 1, 0, 0, 1, 0, 0},
    {0, 0, 0, 0, 1, 1, 0, 0}, {0, 0, 1, 1, 0, 0, 0, 1}, {1, 1, 0, 0, 1, 0, 0, 0},
    {0, 0, 1, 0, 1, 0, 1, 0}, {1, 0, 0, 1, 0, 1, 0, 0}, {0, 0, 1, 1, 1, 0, 0, 0},
    {1, 0, 0, 0, 0, 1, 0, 1}, {0, 1, 1, 0, 0, 0, 0, 1}, {0, 0, 1, 1, 0, 0, 1, 0},
    {0, 1, 0, 0, 1, 0, 1, 0}, {0, 1, 0, 0, 0, 1, 1, 0}, {1, 0, 0, 0, 0, 1, 1, 0},
};

/* Local Code\Traps.cpp. The three bodies at 0x5E35F0-0x5E3730 sit in the
   attribution gap before the asserted Traps.cpp body at 0x5E3800 (line 148);
   their placement here is provisional, not proven ownership. */

// FUNCTION: WIZ8 0x005E35F0
void ClearValue69DA68(void)
{
    g_value_69da68 = 0;
}
// FUNCTION: WIZ8 0x005E3600
unsigned char GetFlag69DA6C(void)
{
    return g_flag_69da6c;
}
// FUNCTION: WIZ8 0x005E3730
unsigned char GetTable650434Entry(int row, int column)
{
    return g_table_650434[row][column];
}

// FUNCTION: WIZ8 0x005E3800
void CastTrapSpellAtPoint005E3800(srVector3T<float> point, int spell_id, unsigned int power_level,
                                  int target_count)
{
    W8GrowableVector<int> targets;
    if (target_count < 1) {
        srAssertFail("iNumTargets > 0", "C:\\Projects\\Wizardry 8\\Local Code\\Traps.cpp", 0x94, 0);
    }

    W8TargetSource source;
    ResetTargetSource(&source);
    source.iType = W8_TARGET_SOURCE_INDIRECT;
    source.point = point;

    W8CombatSlot target;
    if (GetSpellTargetType(spell_id, '\0') == 8) {
        target.iType = W8_TARGET_KIND_PLACE;
        target.point = g_startup_world_659c0c->GetPosition();
        CastSpellFromSource(spell_id, &source, &target, power_level, 0, 0, 0, 0, 0, 0, 0);
    } else {
        ResetCombatSlot(&target);
        target.iType = W8_TARGET_KIND_PARTY;
        target.point = g_startup_world_659c0c->GetPosition();

        int eligible = 0;
        for (int slot = 0; slot < 8; ++slot) {
            if (g_status_685170.buffers.party_rows[slot].occupied &&
                g_status_685170.buffers.characters[slot].hp_current != 0 &&
                g_status_685170.buffers.characters[slot].highest_condition < W8_CONDITION_DEAD) {
                targets.Add(slot);
                ++eligible;
            }
        }
        if (eligible <= target_count) {
            target_count = eligible;
        }
        while (target_count < targets.GetCount()) {
            int index = Random(targets.GetCount());
            if (g_status_685170.selected_character != *targets.GetAt(index)) {
                targets.RemoveAt(index);
            }
        }
        CastSpellFromSource(spell_id, &source, &target, power_level, 0, 0, 0, 0, 0, &targets, 0);
    }
}
