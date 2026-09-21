#include "wiz8/local_code/Traps.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/Navigator.h"
#include "wiz8/engine_code/Spells.h"
#include "wiz8/startup_world.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/Targeting.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/layouts/character.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/vector.h"
#include "wiz8/utility.h"
#include "wiz8/float_constants.h"
#include "wiz8/string_database.h"
#include "wiz8/local_code/Strings.h"
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

/* Per-type device floor for the sprung-trap discharge: the type's entry is
   subtracted from the trigger's device count before extra targets and power
   are rolled. */
// GLOBAL: WIZ8 0x006504AC
int g_trap_difficulty_6504ac[W8_TRAP_TYPE_COUNT] = {1, 1, 2, 2, 3, 3, 3, 4, 5, 5, 5, 6, 6, 7, 7};

void DischargeTrapSpell005E3800(float x, float y, float z, int spell_id, unsigned int power_level,
                                int num_targets); /* 0x005E3800 */

// FUNCTION: WIZ8 0x005E3780
void CompleteTrapDisarm005E3780(Trigger* trigger)
{
    int type;
    wchar_t* text;

    trigger->CompleteItemInteraction004447F0();
    type = trigger->value_37c;
    if (Random(100) < 40) {
        ApplyItemEffectToRandomCharacter(g_learn_sound_0068c510, -1, 0, g_effect_argument_005ed8c8);
    }
    text = FormatWideString(g_format_s_space_s_00617584, gppStringList[g_value_0061e9ec[type]],
                            gppStringList[0x7b2]);
    ShowString(text);
    trigger->Run(-1);
}

// FUNCTION: WIZ8 0x005E3800
void DischargeTrapSpell005E3800(float x, float y, float z, int spell_id, unsigned int power_level,
                                int num_targets)
{
    int index;
    int eligible;
    W8GrowableVector<int> targets;
    W8CombatSlot target;
    W8TargetSource source;

    if (num_targets < 1) {
        srAssertFail("iNumTargets > 0", "C:\\Projects\\Wizardry 8\\Local Code\\Traps.cpp", 0x94, 0);
    }
    ResetTargetSource(&source);
    source.iType = W8_TARGET_SOURCE_INDIRECT;
    source.point.x = x;
    source.point.y = y;
    source.point.z = z;
    if (GetSpellTargetType(spell_id, 0) == W8_TARGET_TYPE_POINT) {
        target.iType = W8_TARGET_KIND_PLACE;
        target.point = g_startup_world_659c0c->GetPosition();
        CastSpellFromSource(spell_id, &source, &target, power_level, 0, 0, 0, 0, 0, 0, 0);
    } else {
        ResetCombatSlot(&target);
        target.iType = W8_TARGET_KIND_PARTY;
        target.point = g_startup_world_659c0c->GetPosition();
        eligible = 0;
        for (index = 0; index < W8_PARTY_SLOT_COUNT; ++index) {
            if (g_status_685170.buffers.party_rows[index].occupied &&
                g_status_685170.buffers.characters[index].hp_current != 0 &&
                g_status_685170.buffers.characters[index].highest_condition < W8_CONDITION_DEAD) {
                targets.Add(index);
                ++eligible;
            }
        }
        if (num_targets >= eligible) {
            num_targets = eligible;
        }
        while (targets.count > num_targets) {
            index = Random(targets.count);
            if (*targets.GetAt(index) != g_status_685170.selected_character) {
                targets.RemoveAt(index);
            }
        }
        CastSpellFromSource(spell_id, &source, &target, power_level, 0, 0, 0, 0, 0, &targets, 0);
    }
}

// FUNCTION: WIZ8 0x005E3AB0
void ResolveSprungTrap005E3AB0(Trigger* trigger)
{
    int devices;
    int type;
    int count;
    int power;
    const wchar_t* result;
    wchar_t* text;
    srVector3T<float> point;
    srVector3T<float> camera;
    srVector3T<float> minimum;
    srVector3T<float> maximum;

    devices = trigger->value_36c;
    if (devices > 7) {
        devices = 7;
    } else if (devices < 1) {
        devices = 1;
    }
    type = trigger->value_37c;
    if (Random(2) == 0) {
        trigger->CompleteItemInteraction004447F0();
        result = gppStringList[0x7b3];
    } else {
        result = gppStringList[0x7b4];
    }
    text = FormatWideString(g_format_s_space_s_00617584, gppStringList[g_value_0061e9ec[type]],
                            result);
    ShowString(text);
    count = devices - static_cast<int>(Random(devices / 2));
    power = 4;
    if (devices > g_trap_difficulty_6504ac[type]) {
        devices -= g_trap_difficulty_6504ac[type];
        power = devices + 4;
        if (power > 7) {
            count += static_cast<int>(Random(devices - 3));
            power = 7;
        }
    }
    if (trigger->m_bRepType != 2) {
        srAssertFail("m_bRepType == TRIGGER_REP_PROP", "..\\Engine Code\\Include\\Trigger.hpp",
                     0x3ed, 0);
    }
    if (trigger->m_pProp == 0) {
        GetCameraForwardPoint00421150(1000.0f, &point);
    } else {
        trigger->m_pProp->PlayRepAnimation(&minimum, &maximum);
        point.x = (minimum.x + maximum.x) * g_double_005ebe80;
        point.y = (minimum.y + maximum.y) * g_double_005ebe80;
        point.z = (minimum.z + maximum.z) * g_double_005ebe80;
    }
    GetCameraPosition(&camera);
    g_octree_6598a4->TraceLineOfSight(&camera, &point, 1, -3, -3, 1, 0);
    DischargeTrapSpell005E3800(point.x, point.y, point.z, g_table_6504e8[type + 11], power, count);
}
