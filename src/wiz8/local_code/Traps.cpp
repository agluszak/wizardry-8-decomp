#include "wiz8/local_code/Traps.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/Targeting.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/engine_code/Spells.h"
#include "wiz8/engine_code/Navigator.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/Levels.h"
#include "wiz8/engine_code/3dapi.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/startup_world.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/layouts/character.h"
#include "wiz8/vector.h"
#include "wiz8/utility.h"
#include "wiz8/string_database.h"
#include "wiz8/sr_api.h"
#include "random.h"
#include "FileMan.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

// GLOBAL: WIZ8 0x0069ca68
char g_record_line_69ca68[0x1000];
// GLOBAL: WIZ8 0x0069da6c
unsigned char g_flag_69da6c;
// GLOBAL: WIZ8 0x0069da68
int g_value_69da68;
// GLOBAL: WIZ8 0x0069da70
unsigned int g_record_line_length_69da70;
// GLOBAL: WIZ8 0x00650434
unsigned char g_table_650434[15][8] = {
    {0, 1, 0, 0, 0, 1, 0, 0}, {0, 1, 0, 1, 0, 0, 0, 0}, {0, 0, 1, 0, 0, 1, 0, 0},
    {0, 0, 0, 0, 1, 1, 0, 0}, {0, 0, 1, 1, 0, 0, 0, 1}, {1, 1, 0, 0, 1, 0, 0, 0},
    {0, 0, 1, 0, 1, 0, 1, 0}, {1, 0, 0, 1, 0, 1, 0, 0}, {0, 0, 1, 1, 1, 0, 0, 0},
    {1, 0, 0, 0, 0, 1, 0, 1}, {0, 1, 1, 0, 0, 0, 0, 1}, {0, 0, 1, 1, 0, 0, 1, 0},
    {0, 1, 0, 0, 1, 0, 1, 0}, {0, 1, 0, 0, 0, 1, 1, 0}, {1, 0, 0, 0, 0, 1, 1, 0},
};

// GLOBAL: WIZ8 0x006504ac
int g_trap_target_table_006504ac[W8_TRAP_TYPE_COUNT] = {1, 1, 2, 2, 3, 3, 3, 4,
                                                              5, 5, 5, 6, 6, 7, 7};

// GLOBAL: WIZ8 0x006503ac
const char g_default_location_code_006503ac[] = "tst";
// GLOBAL: WIZ8 0x006503b0
const char g_format_camera_angles_006503b0[] = "%f %f %f %f %f %d\n";
// GLOBAL: WIZ8 0x006503c4
const char g_format_camera_position_006503c4[] = "%f %f %f\n";
// GLOBAL: WIZ8 0x006503d0
char g_notes_file_name_006503d0[] = "data\\notes.txt";
// GLOBAL: WIZ8 0x006503e0
const char g_text_exiting_record_mode_006503e0[] = "Exiting record mode.";
// GLOBAL: WIZ8 0x006503f8
const char g_text_error_deleting_log_006503f8[] = "Error deleting log file.";
// GLOBAL: WIZ8 0x00650414
const char g_text_log_file_deleted_00650414[] = "Log file deleted.";
// GLOBAL: WIZ8 0x00650428
const char g_command_delete_log_00650428[] = "DELETE LOG";
// GLOBAL: WIZ8 0x00650384
const char g_text_record_mode_prompt_00650384[] = "Type in your text, then ENTER or ESC.";

/* Local Code\Traps.cpp. The three bodies at 0x5E35F0-0x5E3730 sit in the
   attribution gap before the asserted Traps.cpp body at 0x5E3800 (line 148);
   their placement here is provisional, not proven ownership. */

/* Append the record-mode line to notes.txt together with the camera
   position/orientation and the current level location code. */
// FUNCTION: WIZ8 0x005E3280
void AppendRecordModeNote005E3280(void)
{
    unsigned int file = FileOpen(g_notes_file_name_006503d0, 0x22, '\0');
    if (file != 0) {
        FileSeek(file, 0, '\x02');
        W8WorldCameraState state;
        GetWorldCameraState(GetWorld(), &state);
        char line[1024];
        sprintf(line, g_format_camera_position_006503c4, state.position.x, state.position.y,
                state.position.z);
        FileWrite(file, line, strlen(line), 0);
        sprintf(line, g_format_camera_angles_006503b0, state.pitch[0], state.pitch[1],
                state.pitch[2], state.pitch[3], state.pitch[4],
                // reinterpret-ok: the serialized angle record's sixth slot carries a byte flag
                *reinterpret_cast<unsigned char*>(&state.pitch[5]));
        FileWrite(file, line, strlen(line), 0);
        sprintf(line, g_format_camera_angles_006503b0, state.yaw[0], state.yaw[1], state.yaw[2],
                state.yaw[3], state.yaw[4],
                // reinterpret-ok: the serialized angle record's sixth slot carries a byte flag
                *reinterpret_cast<unsigned char*>(&state.yaw[5]));
        FileWrite(file, line, strlen(line), 0);
        char location_code[32];
        if (GetLevelLocationCode(g_status_685170.current_level, location_code) == '\0') {
            strcpy(location_code, g_default_location_code_006503ac);
        }
        unsigned int length = strlen(location_code);
        location_code[length] = '\n';
        FileWrite(file, location_code, length + 1, 0);
        length = strlen(g_record_line_69ca68);
        g_record_line_69ca68[length] = '\n';
        FileWrite(file, g_record_line_69ca68, length + 1, 0);
        FileClose(file);
    }
    ResetEditorStatusLine0058AA20(-1);
}

/* ENTER in record mode — "DELETE LOG" erases notes.txt, anything else is
   appended to the log and record mode keeps running. */
// FUNCTION: WIZ8 0x005E34B0
void SubmitRecordModeLine005E34B0(void)
{
    char text[1024];
    if (_stricmp(g_record_line_69ca68, g_command_delete_log_00650428) != 0) {
        AppendRecordModeNote005E3280();
        return;
    }
    ResetEditorStatusLine0058AA20(-1);
    if (FileDelete(g_notes_file_name_006503d0) == '\0') {
        strcpy(text, g_text_error_deleting_log_006503f8);
    } else {
        strcpy(text, g_text_log_file_deleted_00650414);
    }
    ShowNoticef(6, ConvertStringToWide(text));
    g_record_line_69ca68[g_record_line_length_69da70] = 0;
    g_record_line_length_69da70 = 0;
    g_flag_69da6c = '\0';
    strcpy(text, g_text_exiting_record_mode_006503e0);
    ShowNoticef(6, ConvertStringToWide(text));
}

/* The record-mode prompt, re-shown after every key. */
// FUNCTION: WIZ8 0x005E35A0
void ShowRecordModePrompt005E35A0(void)
{
    char text[100];
    ResetEditorStatusLine0058AA20(-1);
    strcpy(text, g_text_record_mode_prompt_00650384);
    ShowNoticef(6, ConvertStringToWide(text));
}

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
/* Record-mode key handler. Returns 1 on ENTER (dispatch the line), -1 on ESC;
   the prompt callback re-runs and the buffer echoes after keys. */
// FUNCTION: WIZ8 0x005E3610
char HandleRecordModeInput005E3610(const InputAtom* input, void (*prompt)(void))
{
    if (g_monster_combat_timer_enabled_006f0531) {
        return '\0';
    }
    int key = toupper(input->usParam);
    if (input->usEvent == KEY_UP) {
        if ((key & 0xffff) == 8) {
            if (g_record_line_length_69da70 == 0) {
                return '\0';
            }
            g_record_line_69ca68[--g_record_line_length_69da70] = 0;
        } else if ((key & 0xffff) == 0xd) {
            g_record_line_69ca68[g_record_line_length_69da70] = 0;
            g_record_line_length_69da70 = 0;
            g_flag_69da6c = '\0';
            return '\x01';
        } else if ((key & 0xffff) == 0x1b) {
            g_record_line_69ca68[g_record_line_length_69da70] = 0;
            g_record_line_length_69da70 = 0;
            g_flag_69da6c = '\0';
            ResetEditorStatusLine0058AA20(-1);
            return -1;
        } else {
            // reinterpret-ok: the int key code is read as a wchar_t string {key, 0}
            char* text = ConvertWideStringToString(reinterpret_cast<wchar_t*>(&key));
            if (g_flag_006f0530 == '\0' && g_flag_006f0534 == '\0' && *text > '@' && *text < '[') {
                *text = *text + ' ';
            }
            g_record_line_69ca68[g_record_line_length_69da70] = *text;
            g_record_line_69ca68[g_record_line_length_69da70 + 1] = 0;
            ++g_record_line_length_69da70;
        }
        if (prompt != 0) {
            prompt();
        }
        ShowNoticef(0xf, ConvertStringToWide(g_record_line_69ca68));
    }
    return '\0';
}

// FUNCTION: WIZ8 0x005E3730
unsigned char GetTable650434Entry(int row, int column)
{
    return g_table_650434[row][column];
}

// FUNCTION: WIZ8 0x005E3780
void RunTrapTrigger005E3780(Trigger* trigger)
{
    trigger->CompleteItemInteraction004447F0();
    int trap = trigger->value_37c;
    if (Random(100) < 0x28) {
        ApplyItemEffectToRandomCharacter(g_learn_sound_0068c510, -1, 0, g_effect_argument_005ed8c8);
    }
    wchar_t* text = FormatWideString(g_format_s_space_s_00617584,
                                     gppStringList[g_value_0061e9ec[trap]], gppStringList[0x7b2]);
    ShowString(text);
    trigger->Run(-1);
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

// FUNCTION: WIZ8 0x005E3AB0
void CastTrapDeviceSpell005E3AB0(Trigger* trigger)
{
    int device_count = trigger->value_36c;
    if (device_count < 8) {
        if (device_count < 1) {
            device_count = 1;
        }
    } else {
        device_count = 7;
    }
    int trap = trigger->value_37c;
    wchar_t* effect_text;
    if (Random(2) == 0) {
        trigger->CompleteItemInteraction004447F0();
        effect_text = gppStringList[0x7b3];
    } else {
        effect_text = gppStringList[0x7b4];
    }
    wchar_t* text = FormatWideString(g_format_s_space_s_00617584,
                                     gppStringList[g_value_0061e9ec[trap]], effect_text);
    ShowString(text);
    int target_count = device_count - Random(device_count / 2);
    unsigned int power_level = 4;
    if (g_trap_target_table_006504ac[trap] < device_count) {
        device_count = device_count - g_trap_target_table_006504ac[trap];
        power_level = device_count + 4;
        if (power_level > 7) {
            target_count = target_count + Random(device_count - 3);
            power_level = 7;
        }
    }
    if (trigger->m_bRepType != '\x02') {
        srAssertFail("m_bRepType == TRIGGER_REP_PROP", "..\\Engine Code\\Include\\Trigger.hpp",
                     0x3ed, 0);
    }
    srVector3T<float> point;
    if (trigger->m_pProp == 0) {
        GetCameraForwardPoint00421150(1000.0, &point);
    } else {
        srVector3T<float> minimum;
        srVector3T<float> maximum;
        trigger->m_pProp->PlayRepAnimation(&minimum, &maximum);
        point.x = (minimum.x + maximum.x) * static_cast<float>(g_double_005ebe80);
        point.y = (minimum.y + maximum.y) * static_cast<float>(g_double_005ebe80);
        point.z = (minimum.z + maximum.z) * static_cast<float>(g_double_005ebe80);
    }
    srVector3T<float> camera;
    GetCameraPosition(&camera);
    g_octree_6598a4->TraceLineOfSight(&camera, &point, '\x01', -3, -3, '\x01', 0);
    CastTrapSpellAtPoint005E3800(point, g_table_6504e8[trap + 0xb], power_level, target_count);
}
