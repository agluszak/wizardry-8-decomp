#include "wiz8/character.h"
#include "wiz8/combat_state.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/game_status.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/NPCScripting.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/Screens.h"
#include "wiz8/message_box.h"
#include "wiz8/npc_interaction.h"
#include "wiz8/npc_state.h"
#include "wiz8/character_event_queue.h"
#include "wiz8/xstatus.h"
#include "wiz8/bink_video.h"

#include "bink.h"
#include "soundman.h"

#include <windows.h>
#include <stdio.h>
#include <string.h>

extern int GetEnvironmentValue0060A3A8(void);
extern W8Monster* GetNpcMonster(W8NpcState* npc);
extern int g_effect_argument_005ed8c8;
extern int g_effect_argument_005ed914;

// GLOBAL: WIZ8 0x0068c3c4
int g_staged_value_68c3c4;
// GLOBAL: WIZ8 0x0068c3c8
int g_staged_value_68c3c8;
// GLOBAL: WIZ8 0x0068c3ce
short g_staged_short_68c3ce;
// GLOBAL: WIZ8 0x0068c3d0
unsigned char g_staged_flag_68c3d0;
// GLOBAL: WIZ8 0x0068c3d8
W8RecordFile0055A480* g_staged_value_68c3d8;
// GLOBAL: WIZ8 0x0068c3dc
W8NpcState* g_staged_npc_68c3dc;

// GLOBAL: WIZ8 0x0068c500
unsigned char g_flag_68c500;

// GLOBAL: WIZ8 0x0068506f
unsigned char g_flag_68506f;

// GLOBAL: WIZ8 0x0061aea0
int g_sedexus_sound_handle_61aea0 = -1;
// GLOBAL: WIZ8 0x0061c324
const char g_sedexus_moaning_sound_0061c324[] = "Data\\Sound\\Ambients\\Al_Sedexus Moaning.wav";

// GLOBAL: WIZ8 0x005EE634
int g_effect_005ee634 = 43;

/* Local Code\NPC Scripting.cpp. The NPC-scripting flag gates the scripted
   monster state; the four accessors below are its only owners. */

// FUNCTION: WIZ8 0x00524BD0
void FormatNpcVoiceSoundPath(W8NpcState* npc, char* output)
{
    const char* name = GetNpcDisplayName(npc);

    if (npc->is_grouped != 0 && g_flag_68c500 == 0) {
        sprintf(output, "RPC_%s", name);
    } else if (npc->record->flag_2ea != 0) {
        sprintf(output, "VOC_%s", name);
    } else {
        sprintf(output, "NPC_%s", name);
    }
}

// FUNCTION: WIZ8 0x00524CA0
void ReloadNpcScriptResources(W8NpcState* npc)
{
    const char* name = GetNpcDisplayName(npc);
    const char* prefix;
    char script_name[128];
    char resource_name[128];

    if (npc->is_grouped != 0 && g_flag_68c500 == 0) {
        prefix = "RPC_%s";
    } else if (npc->record->flag_2ea != 0) {
        prefix = "VOC_%s";
    } else {
        prefix = "NPC_%s";
    }
    sprintf(script_name, prefix, name);
    sprintf(resource_name, "Data\\NPC Scripts\\%s.nsf", script_name);
    npc->record_file = LoadRecordFile0055A480(resource_name);
    if (npc->record_file != 0) {
        W8Monster* monster = GetNpcMonster(npc);
        if (monster != 0) {
            sprintf(resource_name, "%s.msf", script_name);
            monster->SetScript004C7F10(resource_name, 1);
            unsigned int list_index = MonsterGetIndexByLocationID(
                0x315, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp",
                monster->propagated_value_1e4, 1);
            W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(list_index);
            if (monster_info != 0) {
                GetMonsterDataForInfo(monster_info);
            }
        }
    }
}

// FUNCTION: WIZ8 0x00524DA0
void UpdateNpcDialogueVoiceAndCursor(void)
{
    DWORD tick_count;
    W8Monster* monster;

    if (g_npc_scripting.flag_c6 != 0) {
        tick_count = GetTickCount();
        if (g_npc_scripting.value_84 < tick_count - g_npc_scripting.value_88) {
            FinishNpcVoicePlayback(1);
        }
        goto update_cursor;
    }
    if (g_npc_scripting.flag_70 != 0) {
        if (g_npc_scripting.flag_71 == 0) {
            tick_count = GetTickCount();
            if (g_npc_scripting.value_84 < tick_count - g_npc_scripting.value_88) {
                FinishNpcVoicePlayback(1);
            }
            if (g_npc_scripting.flag_71 == 0) {
                goto update_cursor;
            }
        }
        if (g_npc_scripting.npc->is_grouped == 0) {
            Function5E2F40(g_npc_scripting.voice_handle, &g_npc_scripting.gap_track);
            monster = GetNpcMonster(g_npc_scripting.npc);
            if (monster != 0) {
                monster->unknown_214 = g_npc_scripting.gap_track.mouth_open;
            }
        }
    }

update_cursor:
    if (gXStatus.fNpcDialogueMode != 0) {
        if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
            SetTargetCursor(1);
            return;
        }
        if (g_npc_scripting.flag_70 == 0) {
            if (g_npc_scripting.message_lines.GetCount() < 1) {
                if (gXStatus.iCurrentCursor != 8) {
                    SetTargetCursor(-1);
                    return;
                }
            }
        } else if (g_flag_68506f == 0) {
            SetTargetCursor(9);
        }
    }
}

// FUNCTION: WIZ8 0x00524EB0
void ProcessNpcScriptingFrame(void)
{
    W8Character* character;
    unsigned char can_open_dialogue;
    char dialogue_ready;
    int environment;
    int party_slot;
    int selected_party_member;
    SOUNDPARMS local_sound_parms;

    if ((g_npc_scripting.flag_c9 != 0 || g_npc_scripting.flag_c8 != 0) &&
        ((environment = GetEnvironmentValue0060A3A8(), environment == 0) ||
         (environment = GetEnvironmentValue0060A3A8(), environment == 2))) {
        g_npc_scripting.flag_c7 = 0;
        if (g_npc_scripting.flag_c8 == 0) {
            memset(&local_sound_parms, 0xff, sizeof(SOUNDPARMS));
            g_sedexus_sound_handle_61aea0 = (int)SoundPlayStreamedFile(
                (STR)g_sedexus_moaning_sound_0061c324, &local_sound_parms);
        } else {
            ClearMainGameTargetState();
            selected_party_member = g_status_685170.selected_party_member_2434;
            for (party_slot = 0; party_slot < 8; ++party_slot) {
                W8PartySlotRow* row = &g_status_685170.buffers.party_rows[party_slot];
                character = &g_status_685170.buffers.characters[party_slot];
                if (row->occupied != 0 &&
                    ((character->hp_current > 0 || character->highest_condition < 0x12) &&
                     party_slot != selected_party_member)) {
                    RemoveCharacterCondition(party_slot, 0x11, 0);
                    selected_party_member = g_status_685170.selected_party_member_2434;
                }
            }
            character = &g_status_685170.buffers.characters[selected_party_member];
            if (character->gender == W8_GENDER_MALE) {
                QueueCharacterEvent(character, g_effect_005ee634, 0, g_effect_argument_005ed8c8,
                                    g_effect_argument_005ed914);
            }
            g_npc_scripting.flag_ca = 0;
            if (g_sedexus_sound_handle_61aea0 != -1) {
                SoundStop((unsigned int)g_sedexus_sound_handle_61aea0);
                g_sedexus_sound_handle_61aea0 = -1;
            }
        }
        g_npc_scripting.flag_c9 = 0;
        g_npc_scripting.flag_c8 = 0;
    }
    if (g_npc_scripting.flag_c7 == 0 && g_npc_scripting.flag_70 == 0 &&
        g_npc_scripting.flag_c6 == 0) {
        if (g_screen_state_00649f1c->script_busy < 1) {
            ProcessNpcScriptingIdlePass();
            if (g_npc_scripting.flag_70 != 0) {
                return;
            }
            if (g_npc_scripting.flag_c6 != 0) {
                return;
            }
        }
        if (g_npc_scripting.message_lines.GetCount() == 0) {
            if (g_npc_scripting.flag_c5 != 0) {
                g_npc_scripting.staging_restore.staged_short_49e = g_staged_short_68c3ce;
                g_npc_scripting.flag_70 = g_staged_flag_68c3d0;
                g_npc_scripting.staging_restore.value_498 = g_staged_value_68c3c8;
                g_npc_scripting.record_file = g_staged_value_68c3d8;
                g_npc_scripting.npc = g_staged_npc_68c3dc;
                g_npc_scripting.staging_restore.value_494 = g_staged_value_68c3c4;
                g_npc_scripting.flag_c5 = 0;
            }
            if (gXStatus.fNpcDialogueMode != 0 && g_status_685170.value_2435 == 0 &&
                g_flag_68506f == 0 && g_screen_state_00649f1c->script_busy == 0 &&
                (can_open_dialogue = Function577850(), can_open_dialogue != 0)) {
                Function56E800(0);
            }
            if (g_screen_state_00649f1c->script_busy == 0 &&
                g_screen_state_00649f1c->dialogue_panel_hidden != 0 &&
                (dialogue_ready = gXStatus.character_event_queue->IsMainQueueEmpty(),
                 dialogue_ready != 0)) {
                SetNpcDialoguePanelVisible(1);
            }
        }
    }
}

// FUNCTION: WIZ8 0x00525110
void BeginNpcScriptDialogue(W8NpcState* npc, unsigned char preserve_state)
{
    if (preserve_state != 0) {
        g_npc_scripting.flag_c5 = 1;
        g_staged_short_68c3ce = g_npc_scripting.staging_restore.staged_short_49e;
        g_staged_flag_68c3d0 = g_npc_scripting.flag_70;
        g_staged_value_68c3c8 = g_npc_scripting.staging_restore.value_498;
        g_staged_value_68c3d8 = g_npc_scripting.record_file;
        g_staged_npc_68c3dc = g_npc_scripting.npc;
        g_staged_value_68c3c4 = g_npc_scripting.staging_restore.value_494;
    }
    if (npc->has_monster == 0) {
        Function509CD0(npc->name_style, 0, -1);
    }
    g_npc_scripting.staging_restore.staged_short_49e = 0;
    g_npc_scripting.flag_70 = 0;
    g_npc_scripting.staging_restore.value_498 = -1;
    g_npc_scripting.npc = npc;
    g_npc_scripting.record_file = npc->record_file;
}

// FUNCTION: WIZ8 0x00525C50
void FinishNpcVoicePlayback(unsigned char resume_script)
{
    if (g_npc_scripting.flag_c6 != 0) {
        g_npc_scripting.flag_c6 = 0;
        Function576030(0, 0, 0, -1, -1);
        g_npc_scripting.staging_restore.value_498 = g_npc_scripting.staging_restore.value_494;
        return;
    }
    if (g_npc_scripting.flag_70 != 0) {
        if (g_npc_scripting.flag_71 != 0) {
            g_npc_scripting.flag_71 = 0;
            if (g_npc_scripting.voice_handle != -1) {
                g_npc_scripting.flag_cb = 1;
                SoundStop(static_cast<unsigned int>(g_npc_scripting.voice_handle));
                g_npc_scripting.flag_cb = 0;
            }
            Function5E2EF0(&g_npc_scripting.gap_track);
        }
        W8Monster* monster = GetNpcMonster(g_npc_scripting.npc);
        if (monster != 0) {
            monster->StopTalking004C7470();
        }
        W8MonsterManagerEntry* entry = GetNpcGroupEntry(g_npc_scripting.npc);
        if (entry != 0) {
            if (entry->field_071 == 0) {
                SetPartyPortraitEventState(g_npc_scripting.npc->group_index, 0, -1, 0, 1);
            } else {
                gXStatus.character_event_queue->ProcessOwnedEntry(entry->field_071);
            }
        }
        g_npc_scripting.flag_70 = 0;
        g_npc_scripting.voice_handle = -1;
        g_npc_scripting.staging_restore.value_498 = g_npc_scripting.staging_restore.value_494;
        if (g_screen_state_00649f1c->script_busy == 0 && resume_script != 0) {
            if (g_npc_scripting.npc != 0 && g_npc_scripting.npc->record_file != 0 &&
                g_npc_scripting.staging_restore.value_494 <
                    g_npc_scripting.npc->record_file->record_count) {
                W8FileRecord0055A140* records = g_npc_scripting.npc->record_file->records;
                // clang-format off
                int record_address = reinterpret_cast<int>(records); // reinterpret-ok: retail callback API takes the record address as an integer
                // clang-format on
                Function576030(0, 0,
                               record_address + g_npc_scripting.staging_restore.value_494 * 0xc,
                               g_npc_scripting.staging_restore.value_494, -1);
                return;
            }
            Function576030(0, 0, 0, -1, -1);
        }
    }
}

// FUNCTION: WIZ8 0x00525DD0
unsigned char IsNpcScriptSessionActive(void)
{
    return g_npc_scripting.flag_70 != 0 || g_npc_scripting.flag_c6 != 0;
}

// FUNCTION: WIZ8 0x00525DF0
unsigned char ShouldDeferCharacterEventForNpcScript(unsigned char require_group_entry)
{
    if (g_npc_scripting.flag_c7 != 0) {
        return 0;
    }
    if (g_npc_scripting.flag_70 == 0 && g_npc_scripting.flag_c6 == 0 &&
        g_npc_scripting.message_lines.GetCount() == 0) {
        return 0;
    }
    if (g_npc_scripting.npc == 0) {
        return 0;
    }
    if (GetNpcGroupEntry(g_npc_scripting.npc) != 0 && require_group_entry == 0) {
        return 0;
    }
    return 1;
}

// FUNCTION: WIZ8 0x00525E50
bool IsMessageBoxLineQueueEmpty(void)
{
    return g_npc_scripting.message_lines.GetCount() == 0;
}

// FUNCTION: WIZ8 0x00525E60
int ComputePortraitMessageDuration(wchar_t* text)
{
    return static_cast<int>(wcslen(text) * 0x3c + 2000);
}

// FUNCTION: WIZ8 0x00528a80
void AddMessageBoxLine(int type, W8WideChar* text, void* extra)
{
    W8MessageBoxLine* line = new W8MessageBoxLine;

    memset(line, 0, sizeof(W8MessageBoxLine));
    line->unknown_00 = -1;
    line->type = type;
    line->text = text;
    line->extra = extra;
    line->npc = g_npc_scripting.npc;

    if (g_npc_scripting.message_lines.Add(line) < 0) {
        delete line;
    }
}
// FUNCTION: WIZ8 0x00529560
void SetFlag68C4F4(void)
{
    g_npc_scripting.flag_c4 = 1;
}
// FUNCTION: WIZ8 0x00529BC0
void SetFlag68C4F7(void)
{
    g_npc_scripting.flag_c7 = 1;
}
// FUNCTION: WIZ8 0x00529BD0
void ClearFlag68C4F7(void)
{
    g_npc_scripting.flag_c7 = 0;
}
// FUNCTION: WIZ8 0x0052A070
unsigned char GetFlag68C4FA(void)
{
    return g_npc_scripting.flag_ca;
}
// FUNCTION: WIZ8 0x0052A1A0
void SetFlag68C500(unsigned char value)
{
    g_flag_68c500 = value;
}
