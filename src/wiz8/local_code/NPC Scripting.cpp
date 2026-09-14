#include "wiz8/bink_video.h"

#include "bink.h"
#include "soundman.h"

#include "wiz8/character.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/combat_state.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/Environment.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/engine_code/World.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/NPCScripting.h"
#include "wiz8/dialog_code/DialogInterface.h"
#include "wiz8/local_code/NPCManager.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/local_screens/Screens.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/local_screens/ReviewCharacterScreen.h"
#include "wiz8/level_specific_code/MasterFunctionList.h"
#include "wiz8/message_box.h"
#include "wiz8/magic.h"
#include "wiz8/notices.h"
#include "wiz8/location_variables.h"
#include "wiz8/npc_interaction.h"
#include "wiz8/npc_script_file.h"
#include "wiz8/npc_state.h"
#include "wiz8/character_event_queue.h"
#include "wiz8/regions.h"
#include "wiz8/local_code/Targeting.h"
#include "wiz8/xstatus.h"
#include "wiz8/sr_api.h"

#include "FileMan.h"

#include <windows.h>
#include <stdio.h>
#include <string.h>

// GLOBAL: WIZ8 0x0068c3c4
int g_staged_value_68c3c4;
// GLOBAL: WIZ8 0x0068c3c8
int g_staged_value_68c3c8;
// GLOBAL: WIZ8 0x0068c3ce
short g_staged_short_68c3ce;
// GLOBAL: WIZ8 0x0068c3d0
unsigned char g_staged_flag_68c3d0;
// GLOBAL: WIZ8 0x0068c3d8
W8NpcScriptFile* g_staged_value_68c3d8;
// GLOBAL: WIZ8 0x0068c3dc
W8NpcState* g_staged_npc_68c3dc;

// GLOBAL: WIZ8 0x0068c500
unsigned char g_flag_68c500;
// GLOBAL: WIZ8 0x0068C501
unsigned char g_message_queue_idle_68c501;

// GLOBAL: WIZ8 0x0068C358
int g_pending_npc_travel_level;

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
    npc->script_file = LoadNpcScriptFile0055A480(resource_name);
    if (npc->script_file != 0) {
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
            ProcessMessageBoxQueue();
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
                g_npc_scripting.script_file = g_staged_value_68c3d8;
                g_npc_scripting.npc = g_staged_npc_68c3dc;
                g_npc_scripting.staging_restore.value_494 = g_staged_value_68c3c4;
                g_npc_scripting.flag_c5 = 0;
            }
            if (gXStatus.fNpcDialogueMode != 0 && g_status_685170.value_2435 == 0 &&
                g_flag_68506f == 0 && g_screen_state_00649f1c->script_busy == 0 &&
                (can_open_dialogue = CanOpenNpcDialogue(), can_open_dialogue != 0)) {
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
        g_staged_value_68c3d8 = g_npc_scripting.script_file;
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
    g_npc_scripting.script_file = npc->script_file;
}

// FUNCTION: WIZ8 0x00525C50
void FinishNpcVoicePlayback(unsigned char resume_script)
{
    if (g_npc_scripting.flag_c6 != 0) {
        g_npc_scripting.flag_c6 = 0;
        SetNpcQuoteBubbleVisible(0, 0, 0, -1, -1);
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
            if (entry->active_character_event == 0) {
                SetPartyPortraitEventState(g_npc_scripting.npc->group_index, 0, -1, 0, 1);
            } else {
                gXStatus.character_event_queue->CompleteActiveEvent(entry->active_character_event);
            }
        }
        g_npc_scripting.flag_70 = 0;
        g_npc_scripting.voice_handle = -1;
        g_npc_scripting.staging_restore.value_498 = g_npc_scripting.staging_restore.value_494;
        if (g_screen_state_00649f1c->script_busy == 0 && resume_script != 0) {
            if (g_npc_scripting.npc != 0 && g_npc_scripting.npc->script_file != 0 &&
                g_npc_scripting.staging_restore.value_494 <
                    g_npc_scripting.npc->script_file->quote_count) {
                W8NpcScriptQuote* quotes = g_npc_scripting.npc->script_file->quotes;
                SetNpcQuoteBubbleVisible(0, 0, &quotes[g_npc_scripting.staging_restore.value_494],
                                         g_npc_scripting.staging_restore.value_494, -1);
                return;
            }
            SetNpcQuoteBubbleVisible(0, 0, 0, -1, -1);
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

// FUNCTION: WIZ8 0x00526E90
void ProcessMessageBoxQueue(void)
{
    W8MessageBoxLine* line;
    W8NpcState* npc;
    int index;

    if (g_npc_scripting.message_lines.GetCount() < 1) {
        if (g_message_queue_idle_68c501 == 0) {
            FlushPendingNoticeLines005766B0();
        }
        g_message_queue_idle_68c501 = 1;
        return;
    }

    line = *g_npc_scripting.message_lines.GetAt(0);
    npc = line->npc;
    if (npc != 0 && npc != g_npc_scripting.npc && line->type == 0) {
        BeginNpcScriptDialogue(npc, 1);
    }

    if (g_current_screen_state.id != W8_SCREEN_MAIN_GAME) {
        index = 0;
        while (line == 0 && line->type != 0) {
            ++index;
            if (index == g_npc_scripting.message_lines.GetCount()) {
                return;
            }
            line = *g_npc_scripting.message_lines.GetAt(index);
        }
    }

    if (line->type == 0) {
        if (g_message_queue_idle_68c501 != 0) {
            g_message_queue_idle_68c501 = 0;
            if (g_npc_scripting.npc->record->flag_2ea == 0) {
                for (index = 0; index < g_npc_scripting.message_lines.GetCount(); ++index) {
                    W8MessageBoxLine* queued = *g_npc_scripting.message_lines.GetAt(index);
                    if (queued->type == 0 && static_cast<char>(queued->unknown_18) == 0) {
                        for (int pending = 0;
                             pending < g_npc_scripting.pending_script_values.GetCount();
                             ++pending) {
                            if (**g_npc_scripting.pending_script_values.GetAt(pending) ==
                                queued->unknown_00) {
                                if (g_npc_scripting.npc->record->unknown_056 == 0 &&
                                    queued->unknown_00 != 0x76) {
                                    W8NpcScriptQuote* quote = &g_npc_scripting.npc->script_file
                                                                   ->quotes[queued->unknown_00];
                                    int entry;
                                    for (entry = 0; entry < quote->entry_count; ++entry) {
                                        if (quote->entries[entry].kind_00 == 0x1d) {
                                            break;
                                        }
                                    }
                                    if (entry == quote->entry_count) {
                                        RunNpcScriptLine(0x1f, 0);
                                        return;
                                    }
                                }
                                break;
                            }
                        }
                    }
                }
            }
            for (index = 0; index < g_npc_scripting.pending_script_values.GetCount(); ++index) {
                delete *g_npc_scripting.pending_script_values.GetAt(index);
            }
            g_npc_scripting.pending_script_values.Clear();
        }

        line = *g_npc_scripting.message_lines.GetAt(0);
        if (static_cast<char>(line->unknown_04) != 0) {
            int* script_line = new int;
            *script_line = line->unknown_00;
            g_npc_scripting.pending_script_values.Add(script_line);
        }

        W8NpcScriptFile* script = g_npc_scripting.npc->script_file;
        if (script != 0 && static_cast<char>(line->unknown_18) == 0 &&
            line->unknown_00 < script->quote_count) {
            W8NpcScriptQuote* quote = &script->quotes[line->unknown_00];
            for (index = 0; index < quote->entry_count; ++index) {
                unsigned char kind = quote->entries[index].kind_00;
                if ((kind == 0x12 || kind == 0x1e) &&
                    (kind == 0x1e || NpcKnowsFact(g_npc_scripting.npc, line->unknown_00) == 0)) {
                    RunNpcScriptLine(0x12, 0);
                    g_screen_state_00649f1c->script_busy = 0xff;
                    W8MessageBoxLine* continuation = new W8MessageBoxLine;
                    memset(continuation, 0, sizeof(W8MessageBoxLine));
                    continuation->unknown_00 = -1;
                    continuation->unknown_08 = reinterpret_cast<int>( // reinterpret-ok: tagged data
                        &quote->entries[index]);
                    continuation->type = 2;
                    continuation->unknown_14 = line->unknown_00;
                    continuation->npc = g_npc_scripting.npc;
                    g_npc_scripting.message_lines.InsertAt(0, continuation);
                    g_npc_scripting.message_lines.Remove(line);
                    delete line;
                    return;
                }
            }
        }
        if (script != 0) {
            RunNpcScriptLine(line->unknown_00, 0);
        }
        g_npc_scripting.message_lines.Remove(line);
        delete line;
        return;
    }

    switch (line->type) {
    case 1:
        CloseNpcDialogueIfActive();
        g_flag_6109f0 = 1;
        break;
    case 2:
        Function526810(line->unknown_08, line->unknown_14);
        break;
    case 3:
        Function570A20();
        Function570CF0();
        break;
    case 4:
        Function528FF0(line->text, 0, -1);
        break;
    case 5:
        CloseNpcDialogueIfActive();
        if (g_screen_state_00649f1c->dialogue_npc != 0) {
            Function50AE40(g_screen_state_00649f1c->dialogue_npc, 1);
        }
        break;
    case 6: {
        int npc_kind = reinterpret_cast<int>(line->text); // reinterpret-ok: tagged NPC kind
        npc = GetNpcStateByKind(npc_kind);
        if (npc != 0) {
            Function50B160(npc);
        }
        Function56E800(0);
        W8MessageBoxLine* continuation = new W8MessageBoxLine;
        memset(continuation, 0, sizeof(W8MessageBoxLine));
        continuation->npc = g_npc_scripting.npc;
        g_npc_scripting.message_lines.Add(continuation);
        break;
    }
    case 7: {
        int group = reinterpret_cast<int>(line->text); // reinterpret-ok: tagged group index
        ClearMainGameTargetState();
        Function50B590(group, 0, 0, 0);
        if (g_screen_state_00649f1c->value_fc == 3) {
            for (int party_slot = 0; party_slot < 8; ++party_slot) {
                if (g_status_685170.buffers.party_rows[party_slot].occupied != 0) {
                    RegionSetDisable(party_slot + 7);
                    DisableRegionSetInput(party_slot + 7);
                }
            }
        }
        break;
    }
    case 8:
        SetNpcQuoteBubbleVisible(1, gppStringList[0x74a], 0, -1, 0x47);
        g_npc_scripting.flag_71 = 0;
        g_npc_scripting.value_84 = 2000;
        g_npc_scripting.last_tick = GetTickCount();
        g_npc_scripting.value_88 = GetTickCount();
        g_npc_scripting.flag_70 = 1;
        SoundPlay((STR) "Data\\Sound\\Misc\\Journal Entry.wav",
                  0); // c-style-cast-ok: released SGP textual API uses UINT8 pointer spelling
        break;
    case 9: {
        int string_index = reinterpret_cast<int>(line->text); // reinterpret-ok: tagged string index
        g_npc_scripting.flag_c6 = 1;
        SetNpcQuoteBubbleVisible(1, gppStringList[string_index], 0, -1, -1);
        g_npc_scripting.value_84 = ComputePortraitMessageDuration(gppStringList[string_index]);
        g_npc_scripting.value_88 = GetTickCount();
        break;
    }
    case 10:
        Function4DFAE0(0);
        break;
    case 0xb:
        Function4DFB40(0);
        break;
    case 0xc:
        Function4DFB80(0);
        break;
    case 0xd:
        if (line->text == 0) {
            ClearMainGameTargetState();
        } else {
            Function577520();
        }
        break;
    case 0xe: {
        Trigger* trigger = FindTriggerByName("Path2Trigger");
        if (trigger != 0) {
            trigger->Run(-1);
        }
        break;
    }
    case 0xf: {
        Function56E800(0);
        ResetLevelDataVectors0041F0D0();
        W8MonsterGroup* group = FindFirstMonsterByID(0xc2);
        if (group != 0) {
            unsigned int monster_index = MonsterGetIndexByLocationID(
                0x916, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp", group->value_9f,
                1);
            W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
            monster_info->monster->SetScript004C7F10("MoveSavant.msf", 1);
        }
        break;
    }
    case 0x10: {
        Function56E800(0);
        ResetLevelDataVectors0041F0D0();
        W8MonsterGroup* group = FindFirstMonsterByID(0x18c);
        if (group != 0) {
            unsigned int monster_index = MonsterGetIndexByLocationID(
                0xb5e, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp", group->value_9f,
                1);
            W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
            monster_info->monster->SetScript004C7F10("MoveBela.msf", 1);
        }
        srVector3T<float> position;
        if (FindEntityByName("NP_DSExit", &position, 0, 0)) {
            Function48F800(&position, 0, 1);
        }
        break;
    }
    case 0x12: {
        Function56E800(0);
        Function577520();
        W8MonsterGroup* group = FindFirstMonsterByID(0x13e);
        if (group != 0) {
            unsigned int monster_index = MonsterGetIndexByLocationID(
                0xb1b, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp", group->value_9f,
                1);
            W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
            monster_info->monster->SetScript004C7F10("MoveGolem.msf", 1);
        }
        break;
    }
    case 0x13:
        Function4E0430();
        break;
    case 0x14: {
        W8SkillNoticePayload* skill_changes = static_cast<W8SkillNoticePayload*>(line->extra);
        if (g_settings_6850c8.skill_increase_messages == 0) {
            for (index = 0; index < skill_changes->count; ++index) {
                int party_slot = skill_changes->party_slots[index];
                int skill = skill_changes->skills[index];
                W8Character* character = &g_status_685170.buffers.characters[party_slot];
                unsigned int value = character->skills[skill].value_02;
                if (skill == g_profession_bonus_skills[character->current_profession]) {
                    value = value * 125 / 100;
                }
                PostCharacterNotice(party_slot, gppStringList[0x1d9],
                                    gppStringList[g_character_skill_name_ids_61e454[skill]], value);
            }
            delete skill_changes;
        } else {
            g_npc_scripting.flag_c6 = 1;
            SetNpcQuoteBubbleVisible(1, line->text, 0, -1, -1, 2, line->extra, -1);
            g_npc_scripting.value_84 = ComputePortraitMessageDuration(line->text);
            g_npc_scripting.value_88 = GetTickCount();
        }
        delete[] line->text;
        break;
    }
    case 0x15: {
        Function56E800(0);
        Function577520();
        W8MonsterGroup* group = FindFirstMonsterByID(0x112);
        if (group != 0) {
            unsigned int monster_index = MonsterGetIndexByLocationID(
                0xa3a, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp", group->value_9f,
                1);
            W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
            monster_info->monster->SetCycleCallback004CA340(0x12, NpcScriptCallback0052A080);
            StartMonsterCycle(monster_info, 0x12, 1);
        }
        break;
    }
    case 0x16: {
        Function56E800(0);
        W8MonsterGroup* group = FindFirstMonsterByID(0xdc);
        if (group != 0) {
            unsigned int monster_index = MonsterGetIndexByLocationID(
                0x968, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp", group->value_9f,
                1);
            W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
            StartMonsterCycle(monster_info, 0x12, 1);
            monster_info->monster->SetCycleCallback004CA340(0x12, NpcScriptCallback0052A150);
        }
        ClearMainGameTargetState();
        break;
    }
    case 0x17:
        g_npc_scripting.flag_c6 = 1;
        SetNpcQuoteBubbleVisible(1, line->text, 0, -1, -1, 1, line->extra, -1);
        g_npc_scripting.value_84 = ComputePortraitMessageDuration(line->text);
        g_npc_scripting.value_88 = GetTickCount();
        delete[] line->text;
        break;
    case 0x18:
        g_npc_scripting.flag_c6 = 1;
        SetNpcQuoteBubbleVisible(1, line->text, 0, -1, -1);
        g_npc_scripting.value_84 = ComputePortraitMessageDuration(line->text);
        g_npc_scripting.value_88 = GetTickCount();
        delete[] line->text;
        break;
    case 0x19: {
        int party_slot = *static_cast<int*>(line->extra);
        g_status_685170.buffers.party_rows[party_slot].flag_103 = 1;
        if (g_settings_6850c8.skill_increase_messages == 0) {
            SoundPlay((STR) "Data\\Sound\\Misc\\GainLevel.wav",
                      0); // c-style-cast-ok: released SGP textual API uses UINT8 pointer spelling
            delete[] line->text;
        } else {
            g_npc_scripting.flag_c6 = 1;
            SetNpcQuoteBubbleVisible(1, line->text, 0, -1, -1, 3, line->extra, -1);
            g_npc_scripting.value_84 = ComputePortraitMessageDuration(line->text);
            g_npc_scripting.value_88 = GetTickCount();
            delete[] line->text;
        }
        break;
    }
    case 0x1a: {
        Trigger* trigger = FindTriggerByName("pillargate05");
        if (trigger != 0) {
            trigger->Run(-1);
        }
        npc = GetNpcStateByKind(0x3f);
        W8MonsterInfo* monster_info = GetNpcMonsterInfo(npc);
        if (monster_info != 0) {
            monster_info->monster->BeginFadeOutAndRemove004C5040(0);
        }
        break;
    }
    case 0x1b: {
        Trigger* trigger = FindTriggerByName("pillargate04");
        if (trigger != 0) {
            trigger->Run(-1);
        }
        npc = GetNpcStateByKind(0x3e);
        W8MonsterInfo* monster_info = GetNpcMonsterInfo(npc);
        if (monster_info != 0) {
            monster_info->monster->BeginFadeOutAndRemove004C5040(0);
        }
        break;
    }
    case 0x1c: {
        Trigger* trigger = FindTriggerByName("pillargate01");
        if (trigger != 0) {
            trigger->Run(-1);
        }
        npc = GetNpcStateByKind(0x3d);
        W8MonsterInfo* monster_info = GetNpcMonsterInfo(npc);
        if (monster_info != 0) {
            monster_info->monster->BeginFadeOutAndRemove004C5040(0);
        }
        break;
    }
    case 0x1d:
        if (gXStatus.fCombatMode == 0 || gXStatus.fPartyMovementMode != 0) {
            if (line->text == 0) {
                ClearLevelDataFlag6();
            } else {
                ResetLevelDataVectors0041F0D0();
            }
        }
        break;
    case 0x1e: {
        int party_slot = reinterpret_cast<int>(line->text); // reinterpret-ok: tagged party slot
        g_status_685170.skip_next_condition_reaction = 1;
        SetCharacterCondition(party_slot, 0x13, 9999, 0, 0, 0);
        g_status_685170.flag_2487 = 1;
        g_status_685170.value_248b = g_status_685170.world_clock;
        g_status_685170.value_248f = line->text;
        break;
    }
    case 0x1f:
        SetNpcDialoguePanelVisible(1);
        break;
    case 0x20: {
        Function56E800(0);
        W8MonsterGroup* group = FindFirstMonsterByID(0x1ab);
        if (group != 0) {
            unsigned int monster_index = MonsterGetIndexByLocationID(
                0x983, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp", group->value_9f,
                1);
            W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
            monster_info->monster->BeginFadeOutAndRemove004C5040(0);
        }
        group = FindFirstMonsterByID(0x15d);
        if (group != 0) {
            SetMonsterGroupHostility(group, 1, 0);
        }
        break;
    }
    case 0x21: {
        Function56E800(0);
        W8Monster* monster = GetNpcMonster(g_npc_scripting.npc);
        if (monster != 0) {
            monster->BeginFadeOutAndRemove004C5040(0);
        }
        break;
    }
    case 0x22: {
        Function56E800(0);
        npc = GetNpcStateByKind(0x62);
        W8MonsterInfo* monster_info = npc == 0 ? 0 : GetNpcMonsterInfo(npc);
        if (monster_info != 0) {
            if (monster_info->fActive != 0) {
                MonsterStartsDying(monster_info, 1);
            } else {
                unsigned int monster_index = MonsterGetIndexByLocationID(
                    0xbbf, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp",
                    monster_info->location_id, 1);
                RemoveMonster(monster_index, 1);
            }
        }
        break;
    }
    case 0x23: {
        Function56E800(0);
        npc = GetNpcStateByKind(100);
        W8MonsterInfo* monster_info = npc == 0 ? 0 : GetNpcMonsterInfo(npc);
        if (monster_info != 0) {
            if (monster_info->fActive == 0) {
                unsigned int monster_index = MonsterGetIndexByLocationID(
                    0xc06, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp",
                    monster_info->location_id, 1);
                RemoveMonster(monster_index, 1);
            } else if (monster_info->monster != 0) {
                monster_info->monster->BeginFadeOutAndRemove004C5040(0);
            }
        }
        break;
    }
    case 0x24: {
        Function56E800(0);
        Function577520();
        W8MonsterGroup* group = FindFirstMonsterByID(0x162);
        if (group != 0) {
            unsigned int monster_index = MonsterGetIndexByLocationID(
                0x78d, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp", group->value_9f,
                1);
            W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
            monster_info->monster->SetScript004C7F10("MoveGari.msf", 1);
        }
        break;
    }
    case 0x25: {
        Trigger* door = FindTriggerByName("RatDoor02");
        if (door == 0) {
            srAssertFail("pDoor", "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp", 0x7b6,
                         0);
        }
        door->CompleteItemInteraction004447F0();
        Function56E800(0);
        W8MonsterGroup* group = FindFirstMonsterByID(0xcf);
        if (group != 0) {
            unsigned int monster_index = MonsterGetIndexByLocationID(
                0x7c1, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp", group->value_9f,
                1);
            W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
            monster_info->monster->SetScript004C7F10("Milano.msf", 1);
        }
        break;
    }
    case 0x26: {
        Function56E800(0);
        npc = GetNpcStateByKind(0x4d);
        W8MonsterInfo* monster_info = npc == 0 ? 0 : GetNpcMonsterInfo(npc);
        if (monster_info != 0) {
            if (monster_info->fActive == 0) {
                unsigned int monster_index = MonsterGetIndexByLocationID(
                    0xba8, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp",
                    monster_info->location_id, 1);
                RemoveMonster(monster_index, 1);
            } else {
                monster_info->monster->BeginFadeOutAndRemove004C5040(0);
            }
        }
        break;
    }
    case 0x27: {
        unsigned int event_type = reinterpret_cast<unsigned int>( // reinterpret-ok: tagged id
            line->text);
        int party_slot =
            PickRandomPartySpeaker(event_type, g_status_685170.selected_party_member_2434);
        if (party_slot != -1) {
            g_status_685170.selected_party_member_2434 = static_cast<unsigned char>(party_slot);
            QueueCharacterEvent(&g_status_685170.buffers.characters[party_slot], event_type,
                                g_event_flag_005ed8e0, g_effect_argument_005ed8c8,
                                g_effect_argument_005ed914);
            SetNpcDialoguePanelVisible(0);
            if (g_screen_state_00649f1c->flag_250 != 0) {
                g_screen_state_00649f1c->flag_23c = 1;
            }
        }
        break;
    }
    case 0x28: {
        int party_slot = reinterpret_cast<int>(line->text); // reinterpret-ok: tagged party slot
        QueueCharacterEvent(&g_status_685170.buffers.characters[party_slot], g_effect_005ee58c,
                            g_event_flag_005ed8e0, g_effect_argument_005ed8c8,
                            g_effect_argument_005ed914);
        break;
    }
    case 0x29: {
        Function56E800(0);
        Function577520();
        W8MonsterGroup* group = FindFirstMonsterByID(0x83);
        if (group != 0) {
            unsigned int monster_index = MonsterGetIndexByLocationID(
                0x7a8, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp", group->value_9f,
                1);
            W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
            monster_info->monster->SetScript004C7F10("MoveRubble.msf", 1);
        }
        break;
    }
    case 0x2a: {
        Function56E800(0);
        SetTriggerVariableByName00444030("LezboDemonAppeared", 0);
        npc = GetNpcStateByKind(0x40);
        W8MonsterInfo* monster_info = npc == 0 ? 0 : GetNpcMonsterInfo(npc);
        if (monster_info != 0) {
            if (monster_info->fActive != 0) {
                monster_info->monster->BeginFadeOutAndRemove004C5040(0);
            } else {
                unsigned int monster_index = MonsterGetIndexByLocationID(
                    0xbd8, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp",
                    monster_info->location_id, 1);
                RemoveMonster(monster_index, 1);
            }
        }
        break;
    }
    case 0x2b: {
        Trigger* trigger = FindTriggerByName("triggerFix");
        if (trigger != 0) {
            trigger->Run(-1);
        }
        break;
    }
    case 0x2c: {
        Function56E800(0);
        npc = GetNpcStateByKind(0x42);
        W8MonsterInfo* monster_info = npc == 0 ? 0 : GetNpcMonsterInfo(npc);
        if (monster_info != 0) {
            if (monster_info->fActive != 0) {
                monster_info->monster->BeginFadeOutAndRemove004C5040(0);
            } else {
                unsigned int monster_index = MonsterGetIndexByLocationID(
                    0xbef, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp",
                    monster_info->location_id, 1);
                RemoveMonster(monster_index, 1);
            }
        }
        break;
    }
    case 0x2d: {
        npc = GetNpcStateByKind(0xc);
        W8MonsterInfo* monster_info = GetNpcMonsterInfo(npc);
        srVector3T<float> position;
        if (monster_info != 0 && FindEntityByName("NP_Balbrakhome", &position, 0, 0)) {
            monster_info->monster->SetPosition(&position);
        }
        break;
    }
    case 0x2e: {
        unsigned int eligible = 0;
        int party_slot;
        for (party_slot = 2; party_slot < 8; ++party_slot) {
            if (g_status_685170.buffers.party_rows[party_slot].occupied != 0 &&
                g_status_685170.buffers.characters[party_slot].highest_condition < 0xf) {
                ++eligible;
            }
        }
        if (eligible > 1) {
            for (party_slot = 0; party_slot < 8; ++party_slot) {
                W8Character* character = &g_status_685170.buffers.characters[party_slot];
                if (g_status_685170.buffers.party_rows[party_slot].occupied != 0 &&
                    character->race == 10 && character->highest_condition < 0xf) {
                    QueueCharacterEvent(character, g_effect_005ee654, g_event_flag_005ed8e0,
                                        g_effect_argument_005ed8c8, g_effect_argument_005ed914);
                    break;
                }
            }
        }
        break;
    }
    case 0x2f: {
        W8MonsterGroup* group = FindFirstMonsterByID(0x1b6);
        if (group != 0) {
            unsigned int monster_index = MonsterGetIndexByLocationID(
                0xb2e, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp", group->value_9f,
                1);
            W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
            StartMonsterCycle(monster_info, 0x19, 1);
            monster_info->monster->SetCycleCallback004CA340(0x19, NpcScriptCallback0052A190);
        }
        break;
    }
    case 0x30: {
        Function56E800(0);
        W8MonsterGroup* group = FindFirstMonsterByID(0x1b4);
        if (group != 0) {
            unsigned int monster_index = MonsterGetIndexByLocationID(
                0xa57, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp", group->value_9f,
                1);
            W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
            monster_info->monster->SetScript004C7F10("belapath1.msf", 1);
        }
        Trigger* trigger = FindTriggerByName("CC_TRIGGERPLANE3");
        if (trigger != 0) {
            trigger->flags_0a0 &= ~0x10;
        }
        trigger = FindTriggerByName("CC_TRIGGERPLANE2");
        if (trigger != 0) {
            trigger->flags_0a0 |= 0x10;
            trigger->Run(-1);
        }
        break;
    }
    case 0x31: {
        Trigger* trigger = FindTriggerByName("CC_TRIGGERPLANE3");
        if (trigger != 0) {
            trigger->flags_0a0 |= 0x10;
            trigger->Run(-1);
        }
        npc = GetNpcStateByKind(0x8d);
        if (npc != 0) {
            Function56C5E0(npc, 0, 7, 0, 0);
        }
        break;
    }
    case 0x32: {
        srVector3T<float> position;
        if (FindEntityByName("NP_DS1", &position, 0, 0)) {
            W8MonsterGroup* group = SpawnMonsters(0x234, 1, &position, 0, 1, 0, 0);
            int location_id = IListGetAt(group->monsters, 0);
            if (location_id != 0) {
                unsigned int monster_index = MonsterGetIndexByLocationID(
                    0xa9e, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp", location_id,
                    1);
                W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
                MonsterForwardReferencePosition(monster_info->monster, 0);
            }
            npc = GetNpcStateByKind(0x84);
            if (npc != 0) {
                Function56C5E0(npc, 0, -1, 0, 0);
            }
        }
        break;
    }
    case 0x33:
        if (NpcLeadHasNameStyle(0x18)) {
            npc = GetNpcStateByKind(0x18);
            if (npc != 0) {
                Function50B590(npc->group_index, 0, 1, 0);
            }
            srVector3T<float> position;
            if (FindEntityByName("NP_VI1", &position, 0, 0)) {
                W8MonsterGroup* group = SpawnMonsters(0x1b9, 1, &position, 2, 1, 0, 0);
                int location_id = IListGetAt(group->monsters, 0);
                if (location_id != 0) {
                    unsigned int monster_index = MonsterGetIndexByLocationID(
                        0xafc, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp",
                        location_id, 1);
                    W8MonsterInfo* monster_info =
                        MonsterGetScriptPartByLocationIndex(monster_index);
                    if (FindEntityByName("NP_DS1", &position, 0, 0)) {
                        monster_info->monster->AimAtPosition(&position);
                    }
                }
            }
        }
        break;
    case 0x34: {
        npc = GetNpcStateByKind(0x84);
        W8MonsterInfo* monster_info = npc == 0 ? 0 : GetNpcMonsterInfo(npc);
        if (monster_info != 0) {
            StartMonsterCycle(monster_info, 0x1a, 1);
        }
        srVector3T<float> position;
        W8MonsterGroup* group;
        if (FindEntityByName("NP_PHOONZANGLEE", &position, 0, 0) &&
            (group = FindFirstMonsterByID(0x197)) != 0) {
            unsigned int monster_index = MonsterGetIndexByLocationID(
                0xacf, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp", group->value_9f,
                1);
            monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
            monster_info->monster->SetPosition(&position);
            MonsterForwardReferencePosition(monster_info->monster, 0);
            StartMonsterCycle(monster_info, 0x12, 1);
        }
        npc = GetNpcStateByKind(0x8d);
        if (npc != 0) {
            Function56C5E0(npc, 0, 0x12, 0, 0);
        }
        break;
    }
    case 0x35:
        ClearMainGameTargetState();
        BeginEndgameSequence005A6580();
        break;
    case 0x36:
        if (g_combat_state != 0) {
            int party_slot = reinterpret_cast<int>(line->text); // reinterpret-ok: tagged party slot
            if (g_combat_state->iActionChar == party_slot) {
                g_combat_state->eCombatActionStatus = 0;
                g_combat_state->iActionChar = -1;
            }
            ClearMainGameTargetState();
            Function50B590(party_slot, 0, 0, 1);
            SetTargetToCharacter(party_slot, W8_TARGETING_CONTEXT_OUT_OF_COMBAT);
            g_combat_state->npc_combat_script_pending[party_slot] = false;
        }
        break;
    case 0x37:
        Function56CA90();
        break;
    case 0x38:
        ShowMainGameNoticeLine(gppStringList[0x7eb], OnNpcTravelConfirmationClosed, 1, 1);
        g_pending_npc_travel_level =
            reinterpret_cast<int>(line->text); // reinterpret-ok: queued level id
        break;
    case 0x39: {
        Function56E800(0);
        W8MonsterGroup* group = FindFirstMonsterByID(0x1aa);
        if (group != 0) {
            unsigned int monster_index = MonsterGetIndexByLocationID(
                0x9a0, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp", group->value_9f,
                1);
            W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
            monster_info->monster->BeginFadeOutAndRemove004C5040(0);
        }
        break;
    }
    case 0x3a: {
        int party_slot = reinterpret_cast<int>(line->text); // reinterpret-ok: tagged party slot
        QueueCharacterEvent(&g_status_685170.buffers.characters[party_slot], 0x18,
                            g_event_flag_005ed8ec | g_event_flag_005ed8e0,
                            g_effect_argument_005ed8c8, g_effect_argument_005ed914);
        break;
    }
    case 0x3c:
        npc = GetNpcStateByKind(0x87);
        if (npc != 0) {
            Function56C5E0(npc, 0, -1, 0, 0);
        }
        break;
    case 0x3d:
        Function5A6620(0, 0, 500, NpcScriptCallback00526E40, 1, 1);
        break;
    case 0x3e: {
        Function56E800(0);
        npc = GetNpcStateByKind(0x2e);
        W8MonsterInfo* monster_info = npc == 0 ? 0 : GetNpcMonsterInfo(npc);
        if (monster_info != 0) {
            unsigned int monster_index = MonsterGetIndexByLocationID(
                0x8c2, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp",
                monster_info->location_id, 1);
            RemoveMonster(monster_index, 1);
        }
        break;
    }
    case 0x3f: {
        Function56E800(0);
        npc = GetNpcStateByKind(0x2d);
        W8MonsterInfo* monster_info = npc == 0 ? 0 : GetNpcMonsterInfo(npc);
        if (monster_info != 0) {
            unsigned int monster_index = MonsterGetIndexByLocationID(
                0x8dd, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp",
                monster_info->location_id, 1);
            RemoveMonster(monster_index, 1);
        }
        break;
    }
    case 0x40: {
        Function56E800(0);
        npc = GetNpcStateByKind(0x2f);
        W8MonsterInfo* monster_info = npc == 0 ? 0 : GetNpcMonsterInfo(npc);
        if (monster_info != 0) {
            unsigned int monster_index = MonsterGetIndexByLocationID(
                0x8f7, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp",
                monster_info->location_id, 1);
            RemoveMonster(monster_index, 1);
        }
        break;
    }
    case 0x41:
        Function529F90();
        break;
    case 0x42:
        Function5A6620(0, 0, 500, NpcScriptCallback00526E70, 1, 1);
        break;
    }

    g_npc_scripting.message_lines.Remove(line);
    delete line;
}

// FUNCTION: WIZ8 0x0052a1b0
void OnNpcTravelConfirmationClosed(W8DialogBase* dialog)
{
    if (GetDialogResult(dialog)) {
        QueueNpcTravelRefusals(g_pending_npc_travel_level);
    }
}

// FUNCTION: WIZ8 0x00528a80
void AddMessageBoxLine(int type, wchar_t* text, void* extra)
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
/* QA audit over every `Data\NPC Scripts\*.nsf`: load each script file, print
   every quote line that is not a placeholder sentinel through ShowNotice, log
   each notice that wraps past seven lines to data\longquotes.txt, and write
   per-script plus grand totals to data\quotereport.txt. Raising
   g_status_685170.quote_audit_2431 makes ShowNotice maintain
   g_notice_line_count_0069b7bc and g_status_685170.long_quote_2432. The
   report handle is used unchecked after the appending fopen, which is the
   original's own error handling. */
// FUNCTION: WIZ8 0x00529660
void AuditNpcScriptQuotes00529660(void)
{
    FILE* file;
    FILE* log_file;
    W8NpcScriptFile* script;
    W8NpcScriptQuote* quote;
    GETFILESTRUCT find;
    char date[128];
    char line[200];
    char path[512];
    char pattern[512];
    wchar_t display[2048];
    char* text;
    int file_lines;
    int file_quotes;
    int total_lines;
    int total_quotes;
    int total_scripts;
    int record_index;
    int sub_index;
    BOOLEAN found;

    total_lines = 0;
    total_quotes = 0;
    total_scripts = 0;
    file = fopen("data\\longquotes.txt", "w");
    if (file != 0) {
        fclose(file);
    }
    file = fopen("data\\quotereport.txt", "w");
    if (file != 0) {
        fclose(file);
    }
    GetDateFormatA(LOCALE_SYSTEM_DEFAULT, 0, 0, "dddd',' MMMM dd',' yyyy", date, 0x80);
    file = fopen("data\\quotereport.txt", "a+t");
    g_status_685170.quote_audit_2431 = 1;
    sprintf(pattern, "Data\\NPC Scripts\\*.nsf");
    fprintf(file, "Script Report File: %s\n", date);
    fprintf(file, "-------------------------------------------------\n");
    found = GetFileFirst(pattern, &find);
    while (found != 0) {
        sprintf(path, "Data\\NPC Scripts\\%s", find.zFileName);
        script = LoadNpcScriptFile0055A480(path);
        if (script != 0) {
            file_lines = 0;
            file_quotes = 0;
            for (record_index = 0; record_index < script->quote_count; ++record_index) {
                quote = &script->quotes[record_index];
                for (sub_index = 0; sub_index < quote->subquote_count; ++sub_index) {
                    text = quote->subquotes[sub_index];
                    if (strlen(text) != 0 && _stricmp(text, "EMPTY") != 0 &&
                        _stricmp(text, "BLANK") != 0 && _stricmp(text, "UNKNOWN") != 0 &&
                        _stricmp(text, "CLASSIFIED") != 0) {
                        swprintf(display, L" \"%S\"", text);
                        ShowNotice(0, display, 0, GetTextBoxScrollRange(), 0);
                        if (g_status_685170.long_quote_2432 != 0) {
                            sprintf(line, "Long Quote: #%d, subquote: #%d, script file: %s \n",
                                    record_index, sub_index, find.zFileName);
                            log_file = fopen("data\\longquotes.txt", "a+t");
                            if (log_file != 0) {
                                fprintf(log_file, "%s\n", line);
                                fclose(log_file);
                            }
                        }
                        file_lines += g_notice_line_count_0069b7bc;
                        total_lines += g_notice_line_count_0069b7bc;
                        ++total_quotes;
                        ++file_quotes;
                    }
                }
            }
            fprintf(file, "Script: %s\n", find.zFileName);
            fprintf(file, "  #quotes: %d\n  #lines: %d\n", file_quotes, file_lines);
            ReleaseNpcScriptFile0055A0A0(script);
            ++total_scripts;
        }
        found = GetFileNext(&find);
    }
    ShowNotice(0, L"Quote test complete. See log file for results", -1, GetTextBoxScrollRange(), 0);
    fprintf(file, "Total lines: %d\nTotal Quotes: %d\nTotal Scripts: %d", total_lines, total_quotes,
            total_scripts);
    fclose(file);
    g_status_685170.quote_audit_2431 = 0;
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
/* Fact 0x1bf: raise the scripted-scene gate, drop the level's transient data
   vectors, force the single-target mode, and reopen the party-member region
   sets (7 + slot, member region 0x5a + slot) for every occupied slot. */
// FUNCTION: WIZ8 0x00529BE0
void BeginNpcScriptedScene(void)
{
    int party_slot;

    g_npc_scripting.flag_c7 = 1;
    ResetLevelDataVectors0041F0D0();
    g_flag_68506f = 1;
    SetTargetingMode(1);
    for (party_slot = 0; party_slot < 8; ++party_slot) {
        if (g_status_685170.buffers.party_rows[party_slot].occupied != 0) {
            RegionSetEnable(party_slot + 7);
            EnableRegionSetInput(party_slot + 7);
            EnableRegionInput(party_slot + 0x5a);
        }
    }
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
