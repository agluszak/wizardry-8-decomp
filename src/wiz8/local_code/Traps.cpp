#include "wiz8/local_code/Traps.h"

#include <stdio.h>
#include <string.h>

#include "FileMan.h"
#include "random.h"
#include "surrender/srDebug.h"
#include "wiz8/engine_code/3dapi.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/Levels.h"
#include "wiz8/engine_code/Navigator.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/engine_code/Spells.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/engine_code/World.h"
#include "wiz8/float_constants.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_code/Targeting.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/startup_world.h"
#include "wiz8/string_database.h"
#include "wiz8/utility.h"
#include "wiz8/vector.h"

#define TRAPS_CPP "C:\\Projects\\Wizardry 8\\Local Code\\Traps.cpp"

// GLOBAL: WIZ8 0x0069ca68
char g_record_mode_line_0069ca68[0x1000];
// GLOBAL: WIZ8 0x0069da6c
unsigned char g_flag_69da6c;
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
    g_flag_69da6c = 0;
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
        g_flag_69da6c = 0;
        return 1;
    } else if (character == 0x1b) {
        g_record_mode_line_0069ca68[g_record_mode_length_0069da70] = 0;
        g_record_mode_length_0069da70 = 0;
        g_flag_69da6c = 0;
        ResetEditorStatusLine0058AA20(-1);
        return -1;
    } else {
        text = ConvertWideStringToString(&character);
        if (g_flag_006f0530 == 0 && g_flag_006f0534 == 0 && *text >= 'A' && *text <= 'Z') {
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

/* 0x006504AC: the pin-count ceiling indexed by the rolled tumbler id.
   Random(0xf) returns 0..14, so the table ends before g_table_6504e8. */
// GLOBAL: WIZ8 0x006504AC
int g_tumbler_count_table_006504ac[15] = {1, 1, 2, 2, 3, 3, 3, 4, 5, 5, 5, 6, 6, 7, 7};

// FUNCTION: WIZ8 0x005E3740
void RandomizeTriggerTumblerCount005E3740(Trigger* trigger)
{
    int* lock_state;
    int count;
    int limit;
    int roll;

    lock_state = &trigger->value_368;
    if (lock_state == 0) {
        return;
    }
    count = lock_state[1];
    if (count < 1) {
        count = 1;
    }
    do {
        roll = static_cast<int>(Random(0xf));
        lock_state[5] = roll;
        limit = g_tumbler_count_table_006504ac[roll];
    } while (count < limit || limit + 4 < count);
}

/* A finished lock/trap interaction completes the pending item action, reports
   the device result, and hands the trigger back to Run. */
// FUNCTION: WIZ8 0x005E3780
void CompleteTrapInteraction005E3780(Trigger* trigger)
{
    trigger->CompleteItemInteraction004447F0();
    if (Random(100) < 0x28) {
        ApplyItemEffectToRandomCharacter(g_learn_sound_0068c510, -1, 0, g_effect_argument_005ed8c8);
    }
    ShowString(FormatWideString(g_format_s_space_s_00617584,
                                gppStringList[g_value_0061e9ec[trigger->value_37c]],
                                gppStringList[0x7b2]));
    trigger->Run(-1);
}

/* Fires the trap's effect spell from `point` against up to `num_targets`
   living, non-incapacitated party members (a point-targeted spell casts at the
   camera position instead). */
// FUNCTION: WIZ8 0x005E3800
void CastTrapSpell005E3800(srVector3T<float> point, int spell_id, int power, int num_targets)
{
    W8GrowableVector<int> targets;
    W8TargetSource source;
    W8CombatSlot target;
    srVector3T<float> position;
    int slot;
    int collected;
    int index;

    if (num_targets < 1) {
        srAssertFail("iNumTargets > 0", TRAPS_CPP, 0x94, 0);
    }
    ResetTargetSource(&source);
    source.iType = W8_TARGET_SOURCE_INDIRECT;
    source.point.x = point.x;
    source.point.y = point.y;
    source.point.z = point.z;
    if (GetSpellTargetType(spell_id, 0) == W8_TARGET_TYPE_POINT) {
        target.iType = W8_TARGET_KIND_PLACE;
        position = g_startup_world_659c0c->GetPosition();
        target.point.x = position.x;
        target.point.y = position.y;
        target.point.z = position.z;
        CastSpellFromSource(spell_id, &source, &target, power, 0, 0, 0, 0, 0, 0, 0);
    } else {
        ResetCombatSlot(&target);
        target.iType = W8_TARGET_KIND_PARTY;
        position = g_startup_world_659c0c->GetPosition();
        target.point.x = position.x;
        target.point.y = position.y;
        target.point.z = position.z;
        collected = 0;
        for (slot = 0; slot < 8; ++slot) {
            if (g_status_685170.buffers.party_rows[slot].occupied != 0 &&
                g_status_685170.buffers.characters[slot].hp_current != 0 &&
                g_status_685170.buffers.characters[slot].highest_condition < 0x12) {
                targets.Add(slot);
                ++collected;
            }
        }
        if (num_targets >= collected) {
            num_targets = collected;
        }
        while (targets.count > num_targets) {
            index = static_cast<int>(Random(targets.count));
            if (g_status_685170.selected_character != *targets.GetAt(index)) {
                targets.RemoveAt(index);
            }
        }
        CastSpellFromSource(spell_id, &source, &target, power, 0, 0, 0, 0, 0, &targets, 0);
    }
}

/* A sprung trap/lock device: picks the success or failure notice, then casts
   the device's spell from the prop (or the camera) toward the party. */
// FUNCTION: WIZ8 0x005E3AB0
void TriggerTrapDevice005E3AB0(Trigger* trigger)
{
    srVector3T<float> origin;
    srVector3T<float> minimum;
    srVector3T<float> maximum;
    srVector3T<float> camera;
    wchar_t* result;
    int difficulty;
    int device;
    int power;
    int num_targets;

    difficulty = trigger->value_36c;
    if (difficulty > 7) {
        difficulty = 7;
    } else if (difficulty < 1) {
        difficulty = 1;
    }
    device = trigger->value_37c;
    if (Random(2) == 0) {
        trigger->CompleteItemInteraction004447F0();
        result = gppStringList[0x7b3];
    } else {
        result = gppStringList[0x7b4];
    }
    ShowString(FormatWideString(g_format_s_space_s_00617584,
                                gppStringList[g_value_0061e9ec[device]], result));
    num_targets = difficulty - static_cast<int>(Random(difficulty / 2));
    power = 4;
    if (g_tumbler_count_table_006504ac[device] < difficulty) {
        difficulty = difficulty - g_tumbler_count_table_006504ac[device];
        power = difficulty + 4;
        if (power > 7) {
            num_targets += static_cast<int>(Random(power - 7));
            power = 7;
        }
    }
    if (trigger->m_bRepType != 2) {
        srAssertFail("m_bRepType == TRIGGER_REP_PROP", "..\\Engine Code\\Include\\Trigger.hpp",
                     0x3ed, 0);
    }
    if (trigger->m_pProp == 0) {
        GetCameraForwardPoint00421150(1000.0, &origin);
    } else {
        trigger->m_pProp->PlayRepAnimation(&minimum, &maximum);
        origin.x = (minimum.x + maximum.x) * g_double_005ebe80;
        origin.y = (minimum.y + maximum.y) * g_double_005ebe80;
        origin.z = (minimum.z + maximum.z) * g_double_005ebe80;
    }
    GetCameraPosition(&camera);
    g_octree_6598a4->TraceLineOfSight(&camera, &origin, 1, -3, -3, 1, 0);
    CastTrapSpell005E3800(origin, g_table_6504e8[device + 0xb], power, num_targets);
}
