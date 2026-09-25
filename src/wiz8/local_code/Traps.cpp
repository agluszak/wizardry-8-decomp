#include "wiz8/local_code/Traps.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/Navigator.h"
#include "wiz8/engine_code/Spells.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/Levels.h"
#include "wiz8/engine_code/3dapi.h"
#include "wiz8/startup_world.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/Targeting.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/layouts/character.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/vector.h"
#include "wiz8/utility.h"
#include "wiz8/float_constants.h"
#include "wiz8/string_database.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/sr_api.h"
#include "FileMan.h"
#include "random.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define TRAPS_CPP "C:\\Projects\\Wizardry 8\\Local Code\\Traps.cpp"

// GLOBAL: WIZ8 0x0069ca68
char g_record_mode_line_0069ca68[0x1000];
// GLOBAL: WIZ8 0x0069da6c
bool g_flag_69da6c;
// GLOBAL: WIZ8 0x0069da68
int g_value_69da68;
// GLOBAL: WIZ8 0x0069da70
int g_record_mode_length_0069da70;

// GLOBAL: WIZ8 0x00650384
char s_record_mode_prompt_00650384[] = "Type in your text, then ENTER or ESC.";
// GLOBAL: WIZ8 0x006503ac
char s_record_mode_default_location_006503ac[] = "tst";
// GLOBAL: WIZ8 0x006503b0
char s_record_mode_orientation_format_006503b0[] = "%f %f %f %f %f %d\n";
// GLOBAL: WIZ8 0x006503c4
char s_record_mode_position_format_006503c4[] = "%f %f %f\n";
// GLOBAL: WIZ8 0x006503d0
char s_data_notes_txt_006503d0[] = "data\\notes.txt";
// GLOBAL: WIZ8 0x006503e0
char s_exiting_record_mode_006503e0[] = "Exiting record mode.";
// GLOBAL: WIZ8 0x006503f8
char s_error_deleting_log_file_006503f8[] = "Error deleting log file.";
// GLOBAL: WIZ8 0x00650414
char s_log_file_deleted_00650414[] = "Log file deleted.";
// GLOBAL: WIZ8 0x00650428
char s_delete_log_00650428[] = "DELETE LOG";
/* 0x006504E8: per-device spell/notice table; TriggerTrapDevice005E3AB0 reads
   the effect spell id at index device + 0xb. */
// GLOBAL: WIZ8 0x006504E8
int g_table_6504e8[] = {10,  25, 35, 40, 50, 60, 70, 80,  90,  100, 110, 121, 122,
                        123, 24, 47, 36, 37, 60, 70, 124, 125, 126, 86,  127, 91};
// GLOBAL: WIZ8 0x00650434
unsigned char g_table_650434[15][8] = {
    {0, 1, 0, 0, 0, 1, 0, 0}, {0, 1, 0, 1, 0, 0, 0, 0}, {0, 0, 1, 0, 0, 1, 0, 0},
    {0, 0, 0, 0, 1, 1, 0, 0}, {0, 0, 1, 1, 0, 0, 0, 1}, {1, 1, 0, 0, 1, 0, 0, 0},
    {0, 0, 1, 0, 1, 0, 1, 0}, {1, 0, 0, 1, 0, 1, 0, 0}, {0, 0, 1, 1, 1, 0, 0, 0},
    {1, 0, 0, 0, 0, 1, 0, 1}, {0, 1, 1, 0, 0, 0, 0, 1}, {0, 0, 1, 1, 0, 0, 1, 0},
    {0, 1, 0, 0, 1, 0, 1, 0}, {0, 1, 0, 0, 0, 1, 1, 0}, {1, 0, 0, 0, 0, 1, 1, 0},
};

/* Record mode appends the current camera position, orientation, level
   location code and the typed line to data\notes.txt. */
// FUNCTION: WIZ8 0x005E3280
void WriteRecordModeEntry005E3280(void)
{
    W8WorldCameraState state;
    char location_code[32];
    char line[1024];
    HWFILE file;
    unsigned int length;

    file = FileOpen(s_data_notes_txt_006503d0, FILE_ACCESS_WRITE | FILE_CREATE_ALWAYS, FALSE);
    if (file != 0) {
        FileSeek(file, 0, FILE_SEEK_FROM_END);
        GetWorldCameraState(GetWorld(), &state);
        sprintf(line, s_record_mode_position_format_006503c4, state.position.x, state.position.y,
                state.position.z);
        FileWrite(file, line, strlen(line), 0);
        sprintf(line, s_record_mode_orientation_format_006503b0, state.pitch[0], state.pitch[1],
                state.pitch[2], state.pitch[3], state.pitch[4],
                // reinterpret-ok: raw low byte of the angle record's trailing slot
                *reinterpret_cast<unsigned int*>(&state.pitch[5]) & 0xff);
        FileWrite(file, line, strlen(line), 0);
        sprintf(line, s_record_mode_orientation_format_006503b0, state.yaw[0], state.yaw[1],
                state.yaw[2], state.yaw[3], state.yaw[4],
                // reinterpret-ok: raw low byte of the angle record's trailing slot
                *reinterpret_cast<unsigned int*>(&state.yaw[5]) & 0xff);
        FileWrite(file, line, strlen(line), 0);
        if (GetLevelLocationCode(g_status_685170.current_level, location_code) == 0) {
            strcpy(location_code, s_record_mode_default_location_006503ac);
        }
        length = strlen(location_code);
        location_code[length] = '\n';
        FileWrite(file, location_code, length + 1, 0);
        length = strlen(g_record_mode_line_0069ca68);
        g_record_mode_line_0069ca68[length] = '\n';
        FileWrite(file, g_record_mode_line_0069ca68, length + 1, 0);
        FileClose(file);
    }
    ResetEditorStatusLine0058AA20(-1);
}

/* The ENTER-key apply callback: an ordinary line is appended to the log, while
   "DELETE LOG" removes the file and leaves record mode. */
// FUNCTION: WIZ8 0x005E34B0
void ApplyRecordModeLine005E34B0(void)
{
    char message[1024];

    if (_stricmp(g_record_mode_line_0069ca68, s_delete_log_00650428) != 0) {
        WriteRecordModeEntry005E3280();
        return;
    }
    if (FileDelete(s_data_notes_txt_006503d0) == 0) {
        ResetEditorStatusLine0058AA20(-1);
        strcpy(message, s_error_deleting_log_file_006503f8);
    } else {
        ResetEditorStatusLine0058AA20(-1);
        strcpy(message, s_log_file_deleted_00650414);
    }
    ShowNoticef(6, ConvertStringToWide(message));
    g_record_mode_line_0069ca68[g_record_mode_length_0069da70] = 0;
    g_record_mode_length_0069da70 = 0;
    g_flag_69da6c = false;
    strcpy(message, s_exiting_record_mode_006503e0);
    ShowNoticef(6, ConvertStringToWide(message));
}

/* The per-key prompt callback: clears the status line and shows the record
   mode prompt while each character is being composed. */
// FUNCTION: WIZ8 0x005E35A0
void PromptRecordModeEntry005E35A0(void)
{
    char message[96];

    ResetEditorStatusLine0058AA20(-1);
    strcpy(message, s_record_mode_prompt_00650384);
    ShowNoticef(6, ConvertStringToWide(message));
}

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
/* Record-mode key handler: collects a printable line into
   g_record_mode_line_0069ca68. Returns 1 on ENTER (the caller then runs the
   apply callback), -1 on ESC, 0 otherwise. */
// FUNCTION: WIZ8 0x005E3610
char HandleRecordModeKey005E3610(const InputAtom* input, void (*prompt)(void))
{
    wchar_t character;
    char* text;

    if (g_monster_combat_timer_enabled_006f0531 != 0) {
        return 0;
    }
    character = static_cast<wchar_t>(toupper(input->usParam));
    if (input->usEvent != 2) {
        return 0;
    }
    if (character == 8) {
        if (g_record_mode_length_0069da70 == 0) {
            return 0;
        }
        --g_record_mode_length_0069da70;
        g_record_mode_line_0069ca68[g_record_mode_length_0069da70] = 0;
    } else if (character == 0xd) {
        g_record_mode_line_0069ca68[g_record_mode_length_0069da70] = 0;
        g_record_mode_length_0069da70 = 0;
        g_flag_69da6c = false;
        return 1;
    } else if (character == 0x1b) {
        g_record_mode_line_0069ca68[g_record_mode_length_0069da70] = 0;
        g_record_mode_length_0069da70 = 0;
        g_flag_69da6c = false;
        ResetEditorStatusLine0058AA20(-1);
        return -1;
    } else {
        text = ConvertWideStringToString(&character);
        if (g_shift_held_006f0530 == 0 && g_modifier_held_006f0534 == 0 && *text >= 'A' &&
            *text <= 'Z') {
            *text += 0x20;
        }
        g_record_mode_line_0069ca68[g_record_mode_length_0069da70] = *text;
        g_record_mode_line_0069ca68[g_record_mode_length_0069da70 + 1] = 0;
        ++g_record_mode_length_0069da70;
    }
    if (prompt != 0) {
        prompt();
    }
    ShowNoticef(0xf, ConvertStringToWide(g_record_mode_line_0069ca68));
    return 0;
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

/* Roll the trigger's trap type (device_id) on first interaction: rejection-
   sample the fifteen-row trap table until a type whose per-type difficulty
   lands within four of the trigger's grade (difficulty, floored at one). */
// FUNCTION: WIZ8 0x005E3740
void SelectTrapType005E3740(Trigger* trigger)
{
    W8LockState* lock_state;
    int budget;
    int type;

    lock_state = &trigger->lock_state;
    if (lock_state == 0) {
        return;
    }
    budget = lock_state->difficulty;
    if (budget < 1) {
        budget = 1;
    }
    do {
        type = Random(0xf);
        lock_state->device_id = type;
    } while (g_trap_difficulty_6504ac[type] > budget ||
             g_trap_difficulty_6504ac[type] + 4 < budget);
}

void DischargeTrapSpell005E3800(float x, float y, float z, int spell_id, unsigned int power_level,
                                int num_targets); /* 0x005E3800 */

// FUNCTION: WIZ8 0x005E3780
void CompleteTrapDisarm005E3780(Trigger* trigger)
{
    int type;
    wchar_t* text;

    trigger->CompleteItemInteraction004447F0();
    type = trigger->lock_state.device_id;
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
            if (g_status_685170.buffers.XChar[index].fOccupied &&
                g_status_685170.buffers.Char[index].hp_current != 0 &&
                g_status_685170.buffers.Char[index].highest_condition < W8_CONDITION_DEAD) {
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

    devices = trigger->lock_state.difficulty;
    if (devices > 7) {
        devices = 7;
    } else if (devices < 1) {
        devices = 1;
    }
    type = trigger->lock_state.device_id;
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
