#include "wiz8/character.h"
#include "wiz8/combat_state.h"
#include "wiz8/game_status.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/Trigger.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/startup_runtime_state.h"
#include "wiz8/local_screens/Screens.h"
#include "wiz8/location_variables.h"
#include "wiz8/npc_interaction.h"
#include "wiz8/npc_state.h"
#include "wiz8/message_box.h"
#include "wiz8/regions.h"
#include "wiz8/sr_api.h"
#include "wiz8/xstatus.h"
#include "soundman.h"

#include <string.h>
#include "wiz8/factions.h"
#include "wiz8/fact_state.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/record_file_0055a480.h"
#include "wiz8/screen_state.h"
#include "wiz8/targeting.h"
#include "wiz8/utility.h"
#include "wiz8/3d_code/IList.h"
#include "random.h"

#include <stdio.h>
#include <wchar.h>
#include <windows.h>

extern const char* GetNpcDisplayName(W8NpcState* npc);
extern W8MonsterInfo* GetNpcMonsterInfo(W8NpcState* npc);
extern W8Monster* GetNpcMonster(W8NpcState* npc);
extern int GetEnvironmentValue0060A3A8(void);
extern void Function5E2F40(int sound_handle, unsigned char* state);
extern void Function509CD0(unsigned char value, int enabled, int location_id);
extern int Function50ADA0(char* name);
extern void Function50A570(int npc_index, int mode, int a, int b, int value);
extern void Function50AE40(int npc_index, int value);
extern void Function50B160(W8NpcState* npc);
extern void Function50B590(int npc_index, int a, int b, int c);
extern void Function525350(unsigned char* record, unsigned char step, int a, unsigned char b);
extern int Function5251F0(int script_op);
extern void Function528B50(int script_op, int a, int b);
extern void Function528FF0(int value, int a, int b);
extern void Function52A1F0(int a, W8MessageBoxLine* line);
extern void Function526810(int a, int b);
extern void Function570A20(void);
extern void Function570CF0(void);
extern void Function571660(wchar_t* text, int mode, int value);
extern void Function576030(int a, int b, int c, int d, int e);
extern void Function576060(int a, int b, int c, int d, int e, int f, int g, int h);
extern void Function5766B0(void);
extern void Function576B80(void);
extern void Function577520(void);
extern void Function5775D0(wchar_t* text, int mode);
extern void Function535CF0(int a, int b, int c, int value);
extern void Function529F90(void);
extern int Function52FEE0(int a, unsigned char b);
extern void Function569A50(int a, void* callback, int b, int c);
extern void Function56CA90(void);
extern void Function5A6580(void);
extern void Function5A6620(int a, int b, int c, void* callback, int d, int e);
extern void Function5E2EF0(unsigned char* state);
extern void Function445F70(W8MessageBoxLine* line);
extern void Function4D99E0(int index);
extern W8MessageBoxLine* Function5C3790(int index);
extern void Function48F800(srVector3T<float>* position, int a, int b);
extern void Function4DFAE0(int a);
extern void Function4DFB40(int a);
extern void Function4DFB80(int a);
extern void Function4E0430(void);
extern bool NpcKnowsFact(W8NpcState* npc, unsigned int fact);
extern unsigned char g_flag_6109f0;
extern int g_effect_005ee58c;
extern int g_effect_005ee654;
extern int g_effect_argument_005ed8e0;
extern int g_effect_argument_005ed8ec;
extern int g_npc_script_line_min_005ee6a0;
extern int g_npc_script_line_max_005ee6d0;
extern int g_value_68c358;
extern void Function52A080(W8Monster* monster);
extern void Function52A150(W8Monster* monster);
extern void Function52A190(W8Monster* monster);
extern void Function52A1B0(void);
extern void Function526E40(void);
extern void Function526E70(void);
extern unsigned char FindEntityByName(const char* name, srVector3T<float>* position, int* value,
                                      int unknown);
extern int SpawnMonsters(int monster_id, int count, unsigned char* position, int facing, int a,
                         int b, int c);
extern void MonsterForwardReferencePosition(W8Monster* monster, char alternate);
extern int IListGetAt(W8IList* list, int index);
extern char FindFactionByName(const char* name);

static int GrowMessageBoxLineCapacity(int required)
{
    int index;
    W8MessageBoxLine** old_lines;

    if (required <= g_message_box_line_capacity) {
        return 1;
    }
    old_lines = g_message_box_lines;
    g_message_box_lines = new W8MessageBoxLine*[required];
    if (g_message_box_lines == 0) {
        g_message_box_lines = old_lines;
        return 0;
    }
    g_message_box_line_capacity = required;
    for (index = 0; index < g_message_box_line_count; ++index) {
        g_message_box_lines[index] = old_lines[index];
    }
    delete[] old_lines;
    return 1;
}

// GLOBAL: WIZ8 0x0068506f
unsigned char g_flag_0068506f;
// GLOBAL: WIZ8 0x0061aea0
int g_ambient_sound_61aea0 = -1;
// GLOBAL: WIZ8 0x005ee634
int g_effect_005ee634;
// GLOBAL: WIZ8 0x0068c3c4
int g_value_68c3c4;
// GLOBAL: WIZ8 0x0068c3c8
int g_value_68c3c8;
// GLOBAL: WIZ8 0x0068c3ce
unsigned short g_value_68c3ce;
// GLOBAL: WIZ8 0x0068c3d0
unsigned char g_flag_68c3d0;
// GLOBAL: WIZ8 0x0068c3d8
int g_value_68c3d8;
// GLOBAL: WIZ8 0x0068c3dc
W8NpcState* g_npc_state_68c3dc;
// GLOBAL: WIZ8 0x0068c494
int g_value_68c494;
// GLOBAL: WIZ8 0x0068c498
int g_value_68c498;
// GLOBAL: WIZ8 0x0068c49c
int g_value_68c49c;
// GLOBAL: WIZ8 0x0068c4a1
unsigned char g_flag_68c4a1;
// GLOBAL: WIZ8 0x0068c4a8
int g_value_68c4a8;
// GLOBAL: WIZ8 0x0068c4b0
int g_sound_handle_68c4b0 = -1;
// GLOBAL: WIZ8 0x0068c4b4
unsigned int g_value_68c4b4;
// GLOBAL: WIZ8 0x0068c4b8
unsigned int g_value_68c4b8;
// GLOBAL: WIZ8 0x0068c4dc
unsigned char g_sound_state_68c4dc[0x10];
// GLOBAL: WIZ8 0x0068c4ec
int g_value_68c4ec;
// GLOBAL: WIZ8 0x0068c4f5
unsigned char g_flag_68c4f5;
// GLOBAL: WIZ8 0x0068c4f8
unsigned char g_flag_68c4f8;
// GLOBAL: WIZ8 0x0068c4f9
unsigned char g_flag_68c4f9;

// GLOBAL: WIZ8 0x0068c4f4
unsigned char g_flag_68c4f4;
// GLOBAL: WIZ8 0x0068c4fa
unsigned char g_flag_68c4fa;
// GLOBAL: WIZ8 0x0068c500
unsigned char g_flag_68c500;

// GLOBAL: WIZ8 0x0068c4d0
int g_value_68c4d0;
// GLOBAL: WIZ8 0x0068c4d4
int g_value_68c4d4;
// GLOBAL: WIZ8 0x0068c4d8
unsigned int** g_value_68c4d8;
// GLOBAL: WIZ8 0x0068c4f0
unsigned int g_value_68c4f0;
// GLOBAL: WIZ8 0x0068c4fb
unsigned char g_flag_68c4fb;
// GLOBAL: WIZ8 0x0068c501
unsigned char g_flag_68c501;

/* Message-box line storage. AddMessageBoxLine and the other owners in this
   translation unit share these globals; there is no MessageBox.cpp unit. */
// GLOBAL: WIZ8 0x0068c4c8
W8MessageBoxLine** g_message_box_lines;
// GLOBAL: WIZ8 0x0068c4c0
int g_message_box_line_count;
// GLOBAL: WIZ8 0x0068c4c4
int g_message_box_line_capacity;

/* Local Code\NPC Scripting.cpp. The NPC-scripting flag gates the scripted
   monster state; the four accessors below are its only owners. */

// FUNCTION: WIZ8 0x00529560
void SetFlag68C4F4(void)
{
    g_flag_68c4f4 = 1;
}
// FUNCTION: WIZ8 0x00529BC0
void SetFlag68C4F7(void)
{
    g_flag_68c4f7 = 1;
}
// FUNCTION: WIZ8 0x00529BD0
void ClearFlag68C4F7(void)
{
    g_flag_68c4f7 = 0;
}
// FUNCTION: WIZ8 0x0052A070
unsigned char GetFlag68C4FA(void)
{
    return g_flag_68c4fa;
}
// FUNCTION: WIZ8 0x0052A1A0
void SetFlag68C500(unsigned char value)
{
    g_flag_68c500 = value;
}

// FUNCTION: WIZ8 0x00525DD0
unsigned char IsNpcScriptSessionActive(void)
{
    return g_flag_68c4a0 != 0 || g_flag_68c4f6 != 0;
}

// FUNCTION: WIZ8 0x00525DF0
unsigned char ShouldDeferCharacterEventForNpcScript(unsigned char require_group_entry)
{
    if (g_flag_68c4f7 != 0) {
        return 0;
    }
    if (g_flag_68c4a0 == 0 && g_flag_68c4f6 == 0 && g_message_box_line_count == 0) {
        return 0;
    }
    if (g_npc_state_68c4ac == 0) {
        return 0;
    }
    if (GetNpcGroupEntry(g_npc_state_68c4ac) != 0 && require_group_entry == 0) {
        return 0;
    }
    return 1;
}

// FUNCTION: WIZ8 0x00525E50
bool IsMessageBoxLineQueueEmpty(void)
{
    return g_message_box_line_count == 0;
}

// FUNCTION: WIZ8 0x00528a80
void AddMessageBoxLine(int type, W8WideChar* text, void* extra)
{
    W8MessageBoxLine* line = new W8MessageBoxLine;
    int new_capacity;
    int index;

    memset(line, 0, sizeof(W8MessageBoxLine));
    line->unknown_00 = -1;
    line->type = type;
    line->text = text;
    line->extra = extra;
    line->npc = g_npc_state_68c4ac;

    new_capacity = g_message_box_line_count + 1;
    if (new_capacity > g_message_box_line_capacity) {
        W8MessageBoxLine** old_lines = g_message_box_lines;

        g_message_box_lines = new W8MessageBoxLine*[new_capacity];
        if (g_message_box_lines == 0) {
            g_message_box_lines = old_lines;
            return;
        }
        g_message_box_line_capacity = new_capacity;
        for (index = 0; index < g_message_box_line_count; ++index) {
            g_message_box_lines[index] = old_lines[index];
        }
        delete[] old_lines;
    }

    g_message_box_lines[g_message_box_line_count] = line;
    ++g_message_box_line_count;
}

// FUNCTION: WIZ8 0x00524da0
void UpdateNpcDialogueVoiceIdle(void)
{
    W8Monster* monster;
    unsigned int now;

    if (g_flag_68c4f6 == 0) {
        if (g_flag_68c4a0 != 0) {
            if (g_flag_68c4a1 == 0) {
                now = GetTickCount();
                if (g_value_68c4b4 < now - g_value_68c4b8) {
                    FinishNpcVoicePlayback(1);
                }
                if (g_flag_68c4a1 == 0) {
                    goto update_cursor;
                }
            }
            if (g_npc_state_68c4ac->is_grouped == 0) {
                Function5E2F40(g_sound_handle_68c4b0, g_sound_state_68c4dc);
                monster = GetNpcMonster(g_npc_state_68c4ac);
                if (monster != 0) {
                    monster->unknown_214 = g_value_68c4ec;
                }
            }
        }
    } else {
        now = GetTickCount();
        if (g_value_68c4b4 < now - g_value_68c4b8) {
            FinishNpcVoicePlayback(1);
        }
    }
update_cursor:
    if (gXStatus.fNpcDialogueMode != 0) {
        if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
            SetTargetCursor(1);
            return;
        }
        if (g_flag_68c4a0 == 0 && g_message_box_line_count < 1) {
            if (gXStatus.iCurrentCursor != 8) {
                SetTargetCursor(-1);
                return;
            }
        } else if (g_flag_0068506f == 0) {
            SetTargetCursor(9);
        }
    }
}

// FUNCTION: WIZ8 0x00524eb0
void ProcessNpcScriptingIdlePass(void)
{
    W8Character* character;
    int party_slot;
    unsigned int party_row_stride;
    SOUNDPARMS sound_parms;
    int selected_slot;

    if ((g_flag_68c4f9 != 0 || g_flag_68c4f8 != 0) &&
        (GetEnvironmentValue0060A3A8() == 0 || GetEnvironmentValue0060A3A8() == 2)) {
        g_flag_68c4f7 = 0;
        if (g_flag_68c4f8 == 0) {
            memset(&sound_parms, 0xff, sizeof(sound_parms));
            g_ambient_sound_61aea0 = (int)SoundPlayStreamedFile(
                "Data\\Sound\\Ambients\\Al_Sedexus Moaning.wav", &sound_parms);
        } else {
            ClearMainGameTargetState();
            party_slot = 0;
            party_row_stride = 0;
            selected_slot = g_status_685170.selected_character;
            do {
                if (g_status_685170.buffers.party_rows[party_slot].occupied != 0 &&
                    (g_status_685170.buffers.characters[party_slot].hp_current != 0 ||
                     g_status_685170.buffers.characters[party_slot].highest_condition < 0x12) &&
                    party_slot != selected_slot) {
                    RemoveCharacterCondition(party_slot, 0x11, 0);
                    selected_slot = g_status_685170.selected_character;
                }
                party_row_stride += 0x106;
                ++party_slot;
            } while (party_row_stride < 0x830);
            character = &g_status_685170.buffers.characters[selected_slot];
            if (character->gender == W8_GENDER_MALE) {
                QueueCharacterEvent(character, g_effect_005ee634, 0, g_effect_argument_005ed8c8,
                                    g_effect_argument_005ed914);
            }
            g_flag_68c4fa = 0;
            if (g_ambient_sound_61aea0 != -1) {
                SoundStop(g_ambient_sound_61aea0);
                g_ambient_sound_61aea0 = -1;
            }
        }
        g_flag_68c4f9 = 0;
        g_flag_68c4f8 = 0;
    }
    if (g_flag_68c4f7 == 0 && g_flag_68c4a0 == 0 && g_flag_68c4f6 == 0) {
        if (g_screen_state_00649f1c->script_busy < 1) {
            ProcessMessageBoxQueue();
            if (g_flag_68c4a0 != 0) {
                return;
            }
            if (g_flag_68c4f6 != 0) {
                return;
            }
        }
        if (g_message_box_line_count == 0) {
            if (g_flag_68c4f5 != 0) {
                *reinterpret_cast<unsigned short*>(reinterpret_cast<char*>(&g_value_68c49c) + 2) =
                    g_value_68c3ce;
                g_flag_68c4a0 = g_flag_68c3d0;
                g_value_68c498 = g_value_68c3c8;
                g_value_68c4a8 = g_value_68c3d8;
                g_npc_state_68c4ac = g_npc_state_68c3dc;
                g_value_68c494 = g_value_68c3c4;
                g_flag_68c4f5 = 0;
            }
            if (gXStatus.fNpcDialogueMode != 0 && g_status_685170.value_2435 == 0 &&
                g_flag_0068506f == 0 && g_screen_state_00649f1c->script_busy == 0 &&
                Function577850() != 0) {
                Function56E800(0);
            }
            if (g_screen_state_00649f1c->script_busy == 0 &&
                g_screen_state_00649f1c->dialogue_panel_hidden != 0 &&
                StartupRuntimeDeferredQueueEmpty(gXStatus.pStartupRuntime) != 0) {
                SetNpcDialoguePanelVisible(1);
            }
        }
    }
}

// FUNCTION: WIZ8 0x00524bd0
void FormatNpcVoiceSoundPath(W8NpcState* npc, char* output)

{
    const char* pcVar1;

    if ((npc->is_grouped != '\0') && (g_flag_68c500 == '\0')) {
        pcVar1 = GetNpcDisplayName(npc);
        sprintf(output, "RPC_%s", pcVar1);
        return;
    }
    if (npc->record->flag_2ea != '\0') {
        pcVar1 = GetNpcDisplayName(npc);
        sprintf(output, "VOC_%s", pcVar1);
        return;
    }
    pcVar1 = GetNpcDisplayName(npc);
    sprintf(output, "NPC_%s", pcVar1);
    return;
}

// FUNCTION: WIZ8 0x00524ca0
void ReloadNpcScriptResources(W8NpcState* npc)

{
    const char* pcVar1;
    W8RecordFile0055A480* pWVar2;
    W8Monster* this_;
    unsigned int monster_list_index;
    W8MonsterInfo* monster_info;
    const char* _Format;
    char local_100[128];
    char local_80[128];

    if ((npc->is_grouped == '\0') || (g_flag_68c500 != '\0')) {
        if (npc->record->flag_2ea == '\0') {
            pcVar1 = GetNpcDisplayName(npc);
            _Format = "NPC_%s";
        } else {
            pcVar1 = GetNpcDisplayName(npc);
            _Format = "VOC_%s";
        }
    } else {
        pcVar1 = GetNpcDisplayName(npc);
        _Format = "RPC_%s";
    }
    sprintf(local_100, _Format, pcVar1);
    sprintf(local_80, "Data\\NPC Scripts\\%s.nsf", local_100);
    pWVar2 = LoadRecordFile0055A480(local_80);
    npc->record_file = pWVar2;
    if ((pWVar2 != 0) && (this_ = GetNpcMonster(npc), this_ != 0)) {
        sprintf(local_80, "%s.msf", local_100);
        this_->SetScript004C7F10(local_80, '\x01');
        monster_list_index = MonsterGetIndexByLocationID(
            0x315, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp",
            this_->propagated_value_1e4, '\x01');
        monster_info = MonsterGetScriptPartByLocationIndex(monster_list_index);
        if (monster_info != 0) {
            GetMonsterDataForInfo(monster_info);
        }
    }
    return;
}

// FUNCTION: WIZ8 0x00525110
void BeginNpcScriptDialogue(W8NpcState* npc, unsigned char preserve_state)
{
    if (preserve_state != 0) {
        g_flag_68c4f5 = 1;
        *reinterpret_cast<unsigned short*>(reinterpret_cast<char*>(&g_value_68c49c) + 2) =
            g_value_68c3ce;
        g_flag_68c3d0 = g_flag_68c4a0;
        g_value_68c3c8 = g_value_68c498;
        g_value_68c3d8 = g_value_68c4a8;
        g_npc_state_68c3dc = g_npc_state_68c4ac;
        g_value_68c3c4 = g_value_68c494;
    }
    if (npc->has_monster == 0) {
        Function509CD0(npc->name_style, 0, -1);
    }
    *reinterpret_cast<unsigned short*>(reinterpret_cast<char*>(&g_value_68c49c) + 2) = 0;
    g_flag_68c4a0 = 0;
    g_value_68c498 = -1;
    g_npc_state_68c4ac = npc;
    g_value_68c4a8 = reinterpret_cast<int>(npc->record_file);
}

// FUNCTION: WIZ8 0x00525c50
void FinishNpcVoicePlayback(unsigned char resume_script)

{
    int iVar1;
    W8Monster* this_;
    W8MonsterManagerEntry* pWVar2;

    if (g_flag_68c4f6 != '\0') {
        g_flag_68c4f6 = '\0';
        Function576030(0, 0, 0, 0xffffffff, 0xffffffff);
        g_value_68c498 = g_value_68c494;
        return;
    }
    if (g_flag_68c4a0 != '\0') {
        if (g_flag_68c4a1 != '\0') {
            g_flag_68c4a1 = '\0';
            if ((unsigned int)g_sound_handle_68c4b0 != 0xffffffffu) {
                g_flag_68c4fb = 1;
                SoundStop(g_sound_handle_68c4b0);
                g_flag_68c4fb = 0;
            }
            Function5E2EF0(g_sound_state_68c4dc);
        }
        this_ = GetNpcMonster(g_npc_state_68c4ac);
        if (this_ != 0) {
            this_->StopTalking004C7470();
        }
        pWVar2 = GetNpcGroupEntry(g_npc_state_68c4ac);
        if (pWVar2 != 0) {
            if (pWVar2->field_071 == 0) {
                SetPartyPortraitEventState(g_npc_state_68c4ac->group_index, 0,
                                           static_cast<unsigned int>(-1), 0, 1);
            } else {
                (gXStatus.pStartupRuntime)->ProcessOwnedEntry(pWVar2->field_071);
            }
        }
        g_flag_68c4a0 = '\0';
        g_sound_handle_68c4b0 = 0xffffffff;
        g_value_68c498 = g_value_68c494;
        if ((g_screen_state_00649f1c->script_busy == 0) && (resume_script != 0)) {
            if (g_npc_state_68c4ac != 0 && g_npc_state_68c4ac->record_file != 0 &&
                g_value_68c494 < g_npc_state_68c4ac->record_file->record_count) {
                Function576030(0, 0,
                               reinterpret_cast<int>(g_npc_state_68c4ac->record_file->records) +
                                   g_value_68c494 * 0xc,
                               g_value_68c494, -1);
                return;
            }
            Function576030(0, 0, 0, 0xffffffff, 0xffffffff);
        }
    }
    return;
}

// FUNCTION: WIZ8 0x00525e60
int ComputePortraitMessageDuration(wchar_t* text)

{
    size_t sVar1;

    sVar1 = wcslen(text);
    return sVar1 * 0x3c + 2000;
}

// FUNCTION: WIZ8 0x00525fa0
void RunNpcScriptLine(int script_line, unsigned char param)

{
    unsigned char* pbVar1;
    unsigned char uVar2;
    char cVar3;
    int* piVar4;
    W8Monster* this_;
    int* puVar5;
    wchar_t* pwVar6;
    UINT32 UVar7;
    int* piVar8;
    int iVar9;
    int iVar10;
    int* puVar12;
    int uVar13;
    int local_784;
    int local_77c;
    int local_778;
    char local_770[52];
    char local_73c[52];
    wchar_t local_708[100];
    wchar_t local_640[100];
    wchar_t local_578[100];
    wchar_t local_4b0[100];
    wchar_t local_3e8[100];
    wchar_t local_320[100];
    wchar_t local_258[100];
    wchar_t local_190[100];
    wchar_t local_c8[100];

    g_value_68c494 = script_line;
    if (script_line != g_value_68c498) {
        g_value_68c49c = g_value_68c49c & 0xffffff00;
    }
    if (script_line < (int)(unsigned int)*(unsigned short*)(*(int*)g_npc_state_68c4ac + 4)) {
        pbVar1 = (unsigned char*)(*(int*)(*(int*)g_npc_state_68c4ac + 10) + script_line * 0xc);
        if ((unsigned char)g_value_68c49c == *pbVar1) {
            g_value_68c49c = g_value_68c49c & 0xffffff00;
        }
        local_784 = -1;
        if (((g_value_68c49c & 0xff) == *pbVar1 - 1) &&
            (local_778 = 0, *(short*)(pbVar1 + 9) != 0)) {
            local_77c = 0;
            do {
                iVar10 = local_77c + *(int*)(pbVar1 + 5);
                switch (*(unsigned char*)(local_77c + *(int*)(pbVar1 + 5))) {
                case 2:
                    if (*(int*)(iVar10 + 9) == 4) {
                        if (*(int*)(iVar10 + 5) != 2) {
                            Function528B50(iVar10, 0, 0);
                            local_784 = -1;
                            goto LAB_0052669f;
                        }
                    } else if (*(int*)(iVar10 + 5) != 2) {
                        local_784 = *(int*)(iVar10 + 1);
                        goto LAB_0052669f;
                    }
                    UVar7 =
                        Random((int)(char)((*(char*)(iVar10 + 9) - *(char*)(iVar10 + 1)) + '\x01'));
                    local_784 = UVar7 + *(int*)(iVar10 + 1);
                    goto LAB_0052669f;
                case 3:
                    local_784 = Function5251F0(iVar10);
                    if (local_784 != -1)
                        goto LAB_0052669f;
                    break;
                case 5:
                    g_screen_state_00649f1c->script_busy = 0xff;
                    Function528B50(iVar10, 0, 0);
                    goto LAB_0052669f;
                case 7:
                    SetFact(*(int*)(iVar10 + 1), *(unsigned char*)(iVar10 + 5), '\0');
                    break;
                case 8:
                    AddMessageBoxLine(1, 0, 0);
                    iVar10 = 0;
                    if (0 < g_message_box_line_count) {
                        do {
                            piVar4 = (int*)Function5C3790(iVar10);
                            if (*(int*)(*piVar4 + 0xc) == 0x1f) {
                                delete reinterpret_cast<W8MessageBoxLine*>(piVar4);
                                Function4D99E0(iVar10);
                                break;
                            }
                            iVar10 = iVar10 + 1;
                        } while (iVar10 < g_message_box_line_count);
                    }
                    break;
                case 9:
                case 10:
                case 0x10:
                case 0x11:
                    Function528B50(iVar10, 0, 0);
                    break;
                case 0xc:
                    this_ = GetNpcMonster(g_npc_state_68c4ac);
                    if (((this_ != 0) && (*(int*)(iVar10 + 9) != 4)) &&
                        (uVar2 =
                             this_->SetScriptLabel004CA260(*(char**)(*(int*)(iVar10 + 0xe) + 4)),
                         uVar2 == '\0')) {
                        swprintf(local_c8, L"%s", *(wchar_t**)(*(int*)(iVar10 + 0xe) + 4));
                    }
                    break;
                case 0xd:
                    if ((g_value_68c494 < g_npc_script_line_min_005ee6a0) ||
                        (g_npc_script_line_max_005ee6d0 < g_value_68c494)) {
                        Function577520();
                        sprintf(local_770, "%s",
                                reinterpret_cast<char*>(*(int*)(*(int*)(iVar10 + 0xe) + 4) + 4));
                        iVar10 = Function50ADA0(local_770);
                        if (iVar10 != 0) {
                            if (*(char*)(iVar10 + 0x26) == '\0') {
                                ClearMainGameTargetState();
                            } else {
                                AddMessageBoxLine(7, (unsigned short*)(int)*(char*)(iVar10 + 0x2b),
                                                  0);
                            }
                        }
                    }
                    break;
                case 0xe:
                    puVar5 = reinterpret_cast<int*>(new W8MessageBoxLine);
                    puVar12 = puVar5;
                    for (iVar10 = 9; iVar10 != 0; iVar10 = iVar10 + -1) {
                        *puVar12 = 0;
                        puVar12 = puVar12 + 1;
                    }
                    *puVar5 = 0xffffffff;
                    puVar5[3] = 5;
                    puVar5[4] = 0;
                    puVar5[7] = 0;
                    puVar5[8] = reinterpret_cast<int>(g_npc_state_68c4ac);
                    Function445F70(reinterpret_cast<W8MessageBoxLine*>(puVar5));
                    goto LAB_0052669f;
                case 0x13:
                    g_screen_state_00649f1c->script_busy = 0xff;
                    Function528B50(iVar10, 0, 0);
                    goto LAB_0052669f;
                case 0x14:
                    if ((g_value_68c494 < g_npc_script_line_min_005ee6a0) ||
                        (g_npc_script_line_max_005ee6d0 < g_value_68c494)) {
                        Function577520();
                        AddMessageBoxLine(
                            7, (unsigned short*)(int)*(char*)((int)g_npc_state_68c4ac + 0x2b), 0);
                    }
                    break;
                case 0x15:
                    sprintf(local_73c, "%s",
                            reinterpret_cast<char*>(*(int*)(*(int*)(iVar10 + 0xe) + 4) + 4));
                    iVar9 = Function50ADA0(local_73c);
                    if (iVar9 != 0) {
                        Function50A570(iVar9, 4, 0, 0, *(int*)(iVar10 + 1));
                    }
                    break;
                case 0x16:
                    cVar3 = FindFactionByName(*(char**)(*(int*)(iVar10 + 0xe) + 4));
                    if (cVar3 != -1) {
                        Function535CF0(3, 1, cVar3, *(int*)(iVar10 + 1));
                    }
                    break;
                case 0x18:
                    uVar13 = *(int*)(iVar10 + 1);
                    puVar5 = reinterpret_cast<int*>(new W8MessageBoxLine);
                    puVar12 = puVar5;
                    for (iVar10 = 9; iVar10 != 0; iVar10 = iVar10 + -1) {
                        *puVar12 = 0;
                        puVar12 = puVar12 + 1;
                    }
                    *puVar5 = 0xffffffff;
                    puVar5[3] = 0x27;
                    puVar5[4] = uVar13;
                    puVar5[7] = 0;
                    puVar5[8] = reinterpret_cast<int>(g_npc_state_68c4ac);
                    Function445F70(reinterpret_cast<W8MessageBoxLine*>(puVar5));
                    break;
                case 0x19:
                    if (g_settings_6850c8.simplified_npc_interaction != '\0') {
                        swprintf(local_708, L"%s", *(wchar_t**)(*(int*)(iVar10 + 0xe) + 4));
                        if ((gXStatus.fNpcDialogueMode == '\0') ||
                            (g_screen_state_00649f1c->dialogue_panel_hidden == 0)) {
                            uVar13 = 1;
                            pwVar6 = local_708;
                        LAB_005265a7:
                            Function571660(pwVar6, uVar13, 1);
                        } else {
                            Function5775D0(local_708, 1);
                        }
                    }
                    break;
                case 0x1a:
                    if (g_settings_6850c8.simplified_npc_interaction != '\0') {
                        swprintf(local_320, L"%s", *(wchar_t**)(*(int*)(iVar10 + 0xe) + 4));
                        if ((gXStatus.fNpcDialogueMode == '\0') ||
                            (g_screen_state_00649f1c->dialogue_panel_hidden == 0)) {
                            uVar13 = 0;
                            pwVar6 = local_320;
                            goto LAB_005265a7;
                        }
                        Function5775D0(local_320, 0);
                    }
                    break;
                case 0x1b:
                    if (g_settings_6850c8.simplified_npc_interaction != '\0') {
                        swprintf(local_4b0, L"%s", *(wchar_t**)(*(int*)(iVar10 + 0xe) + 4));
                        if ((gXStatus.fNpcDialogueMode == '\0') ||
                            (g_screen_state_00649f1c->dialogue_panel_hidden == 0)) {
                            uVar13 = 2;
                            pwVar6 = local_4b0;
                            goto LAB_005265a7;
                        }
                        Function5775D0(local_4b0, 2);
                    }
                    break;
                case 0x1c:
                    if (g_settings_6850c8.simplified_npc_interaction != '\0') {
                        swprintf(local_190, L"%s", *(wchar_t**)(*(int*)(iVar10 + 0xe) + 4));
                        if ((gXStatus.fNpcDialogueMode == '\0') ||
                            (g_screen_state_00649f1c->dialogue_panel_hidden == 0)) {
                            uVar13 = 3;
                            pwVar6 = local_190;
                            goto LAB_005265a7;
                        }
                        Function5775D0(local_190, 3);
                    }
                    break;
                case 0x1f:
                    if (g_settings_6850c8.simplified_npc_interaction != '\0') {
                        swprintf(local_640, L"%s", *(wchar_t**)(*(int*)(iVar10 + 0xe) + 4));
                        if ((gXStatus.fNpcDialogueMode == '\0') ||
                            (g_screen_state_00649f1c->dialogue_panel_hidden == 0)) {
                            uVar13 = 1;
                            pwVar6 = local_640;
                            goto LAB_005265a7;
                        }
                        Function5775D0(local_640, 1);
                    }
                    break;
                case 0x20:
                    if (g_settings_6850c8.simplified_npc_interaction != '\0') {
                        swprintf(local_578, L"%s", *(wchar_t**)(*(int*)(iVar10 + 0xe) + 4));
                        if ((gXStatus.fNpcDialogueMode == '\0') ||
                            (g_screen_state_00649f1c->dialogue_panel_hidden == 0)) {
                            uVar13 = 0;
                            pwVar6 = local_578;
                            goto LAB_005265a7;
                        }
                        Function5775D0(local_578, 0);
                    }
                    break;
                case 0x21:
                    if (g_settings_6850c8.simplified_npc_interaction != '\0') {
                        swprintf(local_3e8, L"%s", *(wchar_t**)(*(int*)(iVar10 + 0xe) + 4));
                        if ((gXStatus.fNpcDialogueMode == '\0') ||
                            (g_screen_state_00649f1c->dialogue_panel_hidden == 0)) {
                            uVar13 = 2;
                            pwVar6 = local_3e8;
                            goto LAB_005265a7;
                        }
                        Function5775D0(local_3e8, 2);
                    }
                    break;
                case 0x22:
                    if (g_settings_6850c8.simplified_npc_interaction != '\0') {
                        swprintf(local_258, L"%s", *(wchar_t**)(*(int*)(iVar10 + 0xe) + 4));
                        if ((gXStatus.fNpcDialogueMode == '\0') ||
                            (g_screen_state_00649f1c->dialogue_panel_hidden == 0)) {
                            uVar13 = 3;
                            pwVar6 = local_258;
                            goto LAB_005265a7;
                        }
                        Function5775D0(local_258, 3);
                    }
                }
                local_778 = local_778 + 1;
                local_77c = local_77c + 0x12;
            } while (local_778 < (int)(unsigned int)*(unsigned short*)(pbVar1 + 9));
        }
    LAB_0052669f:
        if (g_flag_68c4f4 != '\0') {
            g_flag_68c4f4 = '\0';
            return;
        }
        if (-1 < local_784) {
            piVar8 = reinterpret_cast<int*>(new W8MessageBoxLine);
            piVar4 = piVar8;
            for (iVar10 = 9; iVar10 != 0; iVar10 = iVar10 + -1) {
                *piVar4 = 0;
                piVar4 = piVar4 + 1;
            }
            *piVar8 = local_784;
            *(unsigned char*)(piVar8 + 1) = 0;
            *(unsigned char*)(piVar8 + 6) = 0;
            piVar8[8] = reinterpret_cast<int>(g_npc_state_68c4ac);
            Function445F70(reinterpret_cast<W8MessageBoxLine*>(piVar8));
        }
        Function525350(pbVar1, g_value_68c49c & 0xff, 0, param);
        g_value_68c49c =
            (g_value_68c49c & 0xffffff00) | (unsigned char)((g_value_68c49c & 0xff) + 1);
        if (g_value_68c494 == 0) {
            *(unsigned char*)((int)g_npc_state_68c4ac + 0x1d) = 0;
        }
        iVar10 = g_value_68c494;
        if ((unsigned char)g_value_68c49c < *pbVar1) {
            piVar8 = reinterpret_cast<int*>(new W8MessageBoxLine);
            piVar4 = piVar8;
            for (iVar9 = 9; iVar9 != 0; iVar9 = iVar9 + -1) {
                *piVar4 = 0;
                piVar4 = piVar4 + 1;
            }
            *piVar8 = iVar10;
            *(unsigned char*)(piVar8 + 1) = 0;
            *(unsigned char*)(piVar8 + 6) = 1;
            piVar8[8] = reinterpret_cast<int>(g_npc_state_68c4ac);
            Function52A1F0(0, reinterpret_cast<W8MessageBoxLine*>(piVar8));
        }
    }
    return;
}

// WARNING: Globals starting with '_' overlap smaller symbols at the same address
// FUNCTION: WIZ8 0x00526e90
void ProcessMessageBoxQueue(void)

{
    int iVar1;
    char cVar2;
    unsigned char uVar3;
    size_t sVar4;
    W8NpcState* pWVar5;
    W8MonsterInfo* pWVar6;
    W8MonsterGroup* pWVar7;
    W8Monster* this_;
    Trigger* pTVar8;
    unsigned int uVar9;
    W8MessageBoxLine** ppLine;
    unsigned int** ppDeferred;
    unsigned int* puVar12;
    W8MessageBoxLine* puVar13;
    int iVar14;
    W8MessageBoxLine* puVar15;
    char* pcVar16;
    int iVar17;
    char* pcVar18;
    int iVar19;
    bool bVar20;
    unsigned char* puVar21;
    W8MessageBoxLine* local_20;
    int local_1c;
    srVector3T<float> local_18;
    srVector3T<float> local_c;

    if (g_message_box_line_count < 1) {
        if (g_flag_68c501 == '\0') {
            Function5766B0();
        }
        g_flag_68c501 = '\x01';
        return;
    }
    local_20 = *g_message_box_lines;
    pWVar5 = local_20->npc;
    if (((pWVar5 != 0) && (pWVar5 != g_npc_state_68c4ac)) && (local_20->type == 0)) {
        g_value_68c3ce =
            *reinterpret_cast<unsigned short*>(reinterpret_cast<char*>(&g_value_68c49c) + 2);
        g_flag_68c3d0 = g_flag_68c4a0;
        g_value_68c3c8 = g_value_68c498;
        g_flag_68c4f5 = 1;
        g_value_68c3d8 = g_value_68c4a8;
        g_npc_state_68c3dc = g_npc_state_68c4ac;
        g_value_68c3c4 = g_value_68c494;
        if (pWVar5->has_monster == 0) {
            Function509CD0(pWVar5->name_style, 0, 0xffffffff);
        }
        *reinterpret_cast<unsigned short*>(reinterpret_cast<char*>(&g_value_68c49c) + 2) = 0;
        g_flag_68c4a0 = '\0';
        g_value_68c498 = 0xffffffff;
        g_value_68c4a8 = reinterpret_cast<int>(pWVar5->record_file);
        g_npc_state_68c4ac = pWVar5;
    }
    if (g_current_screen_state.id != 7) {
        iVar14 = 0;
        ppLine = g_message_box_lines;
        while (local_20 == 0 && iVar14 < g_message_box_line_count) {
            iVar14 = iVar14 + 1;
            ppLine = ppLine + 1;
            if (iVar14 == g_message_box_line_count) {
                return;
            }
            local_20 = *ppLine;
            if (g_message_box_line_count <= iVar14) {
                local_20 = *g_message_box_lines;
            }
        }
    }
    if (local_20->type == 0) {
        if (g_flag_68c501 != '\0') {
            g_flag_68c501 = '\0';
            if ((*(char*)(*(int*)((int)g_npc_state_68c4ac + 6) + 0x2ea) == '\0') &&
                (local_1c = 0, 0 < g_message_box_line_count)) {
                local_20 = *g_message_box_lines;
                do {
                    if (g_message_box_line_count <= local_1c) {
                        puVar15 = *g_message_box_lines;
                    } else {
                        puVar15 = g_message_box_lines[local_1c];
                    }
                    if (((puVar15->type == 0) &&
                         (reinterpret_cast<char&>(puVar15->unknown_18) == '\0')) &&
                        (iVar14 = 0, 0 < g_value_68c4d0)) {
                        iVar17 = puVar15->unknown_00;
                        bVar20 = ((((unsigned int)(0) ^ (unsigned int)(g_value_68c4d0)) &
                                   ((unsigned int)(0) ^
                                    ((unsigned int)(0) - (unsigned int)(g_value_68c4d0)))) >>
                                  31);
                        iVar19 = -g_value_68c4d0;
                        ppDeferred = g_value_68c4d8;
                        do {
                            puVar12 = *ppDeferred;
                            if (bVar20 == iVar19 < 0) {
                                ppDeferred = g_value_68c4d8;
                                puVar12 = *ppDeferred;
                            }
                            if (*puVar12 == static_cast<unsigned int>(iVar17)) {
                                if ((*(char*)(*(int*)((int)g_npc_state_68c4ac + 6) + 0x56) ==
                                     '\0') &&
                                    (iVar17 != 0x76)) {
                                    iVar14 = *(int*)(*(int*)g_npc_state_68c4ac + 10) + iVar17 * 0xc;
                                    uVar9 = (unsigned int)*(unsigned short*)(iVar14 + 9);
                                    iVar17 = 0;
                                    if (uVar9 == 0)
                                        goto LAB_00528390;
                                    pcVar16 = *(char**)(iVar14 + 5);
                                    while (*pcVar16 != '\x1d') {
                                        iVar17 = iVar17 + 1;
                                        pcVar16 = pcVar16 + 0x12;
                                        if ((int)uVar9 <= iVar17) {
                                        LAB_00528390:
                                            RunNpcScriptLine(0x1f, 0);
                                            return;
                                        }
                                    }
                                }
                                break;
                            }
                            iVar14 = iVar14 + 1;
                            ppDeferred = ppDeferred + 1;
                            bVar20 =
                                ((((unsigned int)(iVar14) ^ (unsigned int)(g_value_68c4d0)) &
                                  ((unsigned int)(iVar14) ^
                                   ((unsigned int)(iVar14) - (unsigned int)(g_value_68c4d0)))) >>
                                 31);
                            iVar19 = iVar14 - g_value_68c4d0;
                        } while (iVar14 < g_value_68c4d0);
                    }
                    local_1c = local_1c + 1;
                } while (local_1c < g_message_box_line_count);
            }
            iVar14 = 0;
            if (0 < g_value_68c4d0) {
                do {
                    ppDeferred = g_value_68c4d8 + iVar14;
                    if (g_value_68c4d0 <= iVar14) {
                        ppDeferred = g_value_68c4d8;
                    }
                    delete *ppDeferred;
                    iVar14 = iVar14 + 1;
                } while (iVar14 < g_value_68c4d0);
            }
            g_value_68c4d0 = 0;
        }
        puVar15 = *g_message_box_lines;
        ppDeferred = g_value_68c4d8;
        if (reinterpret_cast<char&>(puVar15->unknown_04) != '\0') {
            uVar9 = puVar15->unknown_00;
            puVar12 = new unsigned int;
            *puVar12 = uVar9;
            ppDeferred = g_value_68c4d8;
            iVar14 = g_value_68c4d0 + 1;
            if (g_value_68c4d4 < iVar14) {
                g_value_68c4d8 = new unsigned int*[iVar14];
                if (g_value_68c4d8 == 0)
                    goto LAB_00528492;
                iVar17 = 0;
                g_value_68c4d4 = iVar14;
                if (0 < g_value_68c4d0) {
                    do {
                        g_value_68c4d8[iVar17] = ppDeferred[iVar17];
                        iVar17 = iVar17 + 1;
                    } while (iVar17 < g_value_68c4d0);
                }
                delete[] ppDeferred;
            }
            g_value_68c4d8[g_value_68c4d0] = puVar12;
            g_value_68c4d0 = g_value_68c4d0 + 1;
            ppDeferred = g_value_68c4d8;
        }
    LAB_00528492:
        g_value_68c4d8 = ppDeferred;
        iVar14 = reinterpret_cast<int>(g_npc_state_68c4ac->record_file);
        if (iVar14 != 0) {
            if ((reinterpret_cast<char&>(puVar15->unknown_18) == '\0') &&
                (puVar15->unknown_00 < (int)(unsigned int)*(unsigned short*)(iVar14 + 4))) {
                iVar14 = *(int*)(iVar14 + 10) + puVar15->unknown_00 * 0xc;
                iVar17 = 0;
                if (*(short*)(iVar14 + 9) != 0) {
                    iVar19 = 0;
                    do {
                        iVar1 = *(int*)(iVar14 + 5);
                        cVar2 = *(char*)(iVar19 + iVar1);
                        if (((cVar2 == '\x12') || (cVar2 == '\x1e')) &&
                            ((cVar2 == '\x1e' ||
                              (uVar3 = NpcKnowsFact(g_npc_state_68c4ac, puVar15->unknown_00),
                               uVar3 == '\0')))) {
                            RunNpcScriptLine(0x12, 0);
                            g_screen_state_00649f1c->script_busy = 0xff;
                            uVar9 = puVar15->unknown_00;
                            puVar13 = new W8MessageBoxLine;
                            memset(puVar13, 0, sizeof(W8MessageBoxLine));
                            puVar13->type = 2;
                            puVar13->unknown_00 = -1;
                            puVar13->unknown_08 = iVar19 + iVar1;
                            puVar13->unknown_14 = uVar9;
                            puVar13->npc = g_npc_state_68c4ac;
                            ppLine = g_message_box_lines;
                            if ((g_message_box_line_capacity < g_message_box_line_count + 1) &&
                                (iVar14 = g_message_box_line_capacity + 5,
                                 g_message_box_line_capacity < iVar14)) {
                                g_message_box_lines = new W8MessageBoxLine*[iVar14];
                                if (g_message_box_lines == 0)
                                    goto LAB_0052864c;
                                iVar17 = 0;
                                g_message_box_line_capacity = iVar14;
                                if (0 < g_message_box_line_count) {
                                    do {
                                        g_message_box_lines[iVar17] = ppLine[iVar17];
                                        iVar17 = iVar17 + 1;
                                    } while (iVar17 < g_message_box_line_count);
                                }
                                delete[] ppLine;
                            }
                            iVar14 = g_message_box_line_count;
                            if (g_message_box_line_count < 1) {
                                iVar14 = 0;
                            } else {
                                do {
                                    g_message_box_lines[iVar14] = g_message_box_lines[iVar14 + -1];
                                    iVar14 = iVar14 + -1;
                                } while (0 < iVar14);
                            }
                            g_message_box_lines[iVar14] = puVar13;
                            g_message_box_line_count = g_message_box_line_count + 1;
                        LAB_0052864c:
                            iVar14 = 0;
                            ppLine = g_message_box_lines;
                            if (0 < g_message_box_line_count) {
                                while (*ppLine != puVar15) {
                                    iVar14 = iVar14 + 1;
                                    ppLine = ppLine + 1;
                                    if (g_message_box_line_count <= iVar14) {
                                        delete puVar15;
                                        return;
                                    }
                                }
                                if ((-1 < iVar14) && (iVar14 < g_message_box_line_count)) {
                                    if (iVar14 < g_message_box_line_count + -1) {
                                        do {
                                            g_message_box_lines[iVar14] =
                                                g_message_box_lines[iVar14 + 1];
                                            iVar14 = iVar14 + 1;
                                        } while (iVar14 < g_message_box_line_count + -1);
                                    }
                                    g_message_box_line_count = g_message_box_line_count + -1;
                                }
                            }
                            delete puVar15;
                            return;
                        }
                        iVar17 = iVar17 + 1;
                        iVar19 = iVar19 + 0x12;
                    } while (iVar17 < (int)(unsigned int)*(unsigned short*)(iVar14 + 9));
                }
            }
            RunNpcScriptLine(puVar15->unknown_00, 0);
        }
        iVar14 = 0;
        ppLine = g_message_box_lines;
        if (0 < g_message_box_line_count) {
            while (*ppLine != puVar15) {
                iVar14 = iVar14 + 1;
                ppLine = ppLine + 1;
                if (g_message_box_line_count <= iVar14) {
                    delete puVar15;
                    return;
                }
            }
            if ((-1 < iVar14) && (iVar14 < g_message_box_line_count)) {
                if (iVar14 < g_message_box_line_count + -1) {
                    do {
                        g_message_box_lines[iVar14] = g_message_box_lines[iVar14 + 1];
                        iVar14 = iVar14 + 1;
                    } while (iVar14 < g_message_box_line_count + -1);
                }
                g_message_box_line_count = g_message_box_line_count + -1;
            }
        }
        delete puVar15;
        return;
    }
    switch (local_20->type) {
    case 1:
        Function576B80();
        g_flag_6109f0 = '\x01';
        break;
    case 2:
        Function526810(*(int*)((int)local_20 + 8), *(int*)((int)local_20 + 0x14));
        break;
    case 3:
        Function570A20();
        Function570CF0();
        break;
    case 4:
        Function528FF0(*(int*)((int)local_20 + 0x10), 0, 0xffffffff);
        break;
    case 5:
        Function576B80();
        if (g_screen_state_00649f1c->value_1d4 != 0) {
            Function50AE40(g_screen_state_00649f1c->value_1d4, 1);
        }
        break;
    case 6:
        pWVar5 = GetNpcStateByKind(reinterpret_cast<int>(local_20->text));
        if (pWVar5 != 0) {
            Function50B160(pWVar5);
        }
        Function56E800(0);
        puVar13 = new W8MessageBoxLine;
        memset(puVar13, 0, sizeof(W8MessageBoxLine));
        puVar13->unknown_00 = 0;
        puVar13->unknown_04 = 0;
        puVar13->unknown_18 = 0;
        puVar13->npc = g_npc_state_68c4ac;
        if ((g_message_box_line_count + 1 <= g_message_box_line_capacity) ||
            (GrowMessageBoxLineCapacity(g_message_box_line_count + 1) != 0)) {
            g_message_box_lines[g_message_box_line_count] = puVar13;
            g_message_box_line_count = g_message_box_line_count + 1;
        }
        break;
    case 7:
        ClearMainGameTargetState();
        Function50B590(reinterpret_cast<int>(local_20->text), 0, 0, 0);
        if (g_screen_state_00649f1c->value_fc == 3) {
            iVar14 = 0;
            uVar9 = 7;
            do {
                if (g_status_685170.buffers.party_rows[iVar14 / 0x106].occupied != 0) {
                    RegionSetDisable(uVar9);
                    DisableRegionSetInput(uVar9);
                }
                iVar14 = iVar14 + 0x106;
                iVar17 = uVar9 - 6;
                uVar9 = uVar9 + 1;
            } while (iVar17 < 8);
        }
        break;
    case 8:
        Function576030(1, *(int*)(gppStringList + 0x1d28), 0, 0xffffffff, 0x47);
        g_flag_68c4a1 = 0;
        g_value_68c4b4 = 2000;
        g_value_68c4f0 = GetTickCount();
        g_value_68c4b8 = GetTickCount();
        g_flag_68c4a0 = '\x01';
        SoundPlay("Data\\Sound\\Misc\\Journal Entry.wav", 0);
        break;
    case 9:
        g_flag_68c4f6 = '\x01';
        Function576030(1, *(int*)(gppStringList + *(int*)((int)local_20 + 0x10) * 4), 0, 0xffffffff,
                       0xffffffff);
        sVar4 = wcslen(*(wchar_t**)(gppStringList + *(int*)((int)local_20 + 0x10) * 4));
        g_value_68c4b4 = sVar4 * 0x3c + 2000;
        g_value_68c4b8 = GetTickCount();
        break;
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
        if (*(int*)((int)local_20 + 0x10) == 0) {
            ClearMainGameTargetState();
        } else {
            Function577520();
        }
        break;
    case 0xe:
        pcVar16 = "Path2Trigger";
        goto LAB_0052807e;
    case 0xf:
        Function56E800(0);
        ResetLevelDataVectors0041F0D0();
        pWVar7 = FindFirstMonsterByID(0xc2);
        if (pWVar7 != 0) {
            uVar9 = MonsterGetIndexByLocationID(
                0x916, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp", pWVar7->value_9f,
                '\x01');
            pWVar6 = MonsterGetScriptPartByLocationIndex(uVar9);
            (pWVar6->monster)->SetScript004C7F10("MoveSavant.msf", '\x01');
        }
        break;
    case 0x10:
        Function56E800(0);
        ResetLevelDataVectors0041F0D0();
        pWVar7 = FindFirstMonsterByID(0x18c);
        if (pWVar7 != 0) {
            uVar9 = MonsterGetIndexByLocationID(
                0xb5e, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp", pWVar7->value_9f,
                '\x01');
            pWVar6 = MonsterGetScriptPartByLocationIndex(uVar9);
            (pWVar6->monster)->SetScript004C7F10("MoveBela.msf", '\x01');
        }
        cVar2 = FindEntityByName("NP_DSExit", &local_c, 0, 0);
        if (cVar2 != '\0') {
            Function48F800(&local_c, 0, 1);
        }
        break;
    case 0x12:
        Function56E800(0);
        Function577520();
        pWVar7 = FindFirstMonsterByID(0x13e);
        if (pWVar7 != 0) {
            uVar9 = MonsterGetIndexByLocationID(
                0xb1b, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp", pWVar7->value_9f,
                '\x01');
            pWVar6 = MonsterGetScriptPartByLocationIndex(uVar9);
            (pWVar6->monster)->SetScript004C7F10("MoveGolem.msf", '\x01');
        }
        break;
    case 0x13:
        Function4E0430();
        break;
    case 0x14:
        if (g_settings_6850c8.skill_increase_messages == '\0') {
            pcVar16 = *(char**)((int)local_20 + 0x1c);
            if ('\0' < *pcVar16) {
                pcVar18 = pcVar16 + 1;
                do {
                    iVar17 = (int)pcVar18[8];
                    iVar14 = *pcVar18 * 0x1862;
                    uVar9 = *(unsigned int*)(iVar17 * 0x26 + iVar14 + 0x19f +
                                             (int)g_status_685170.buffers.characters);
                    if (iVar17 ==
                        g_profession_bonus_skills[*(
                            int*)(iVar14 + 0x69 + (int)g_status_685170.buffers.characters)]) {
                        uVar9 = (uVar9 * 0x7d) / 100;
                    }
                    PostCharacterNotice(
                        (int)*pcVar18,
                        reinterpret_cast<const wchar_t*>(*(int*)(gppStringList + 0x764)),
                        reinterpret_cast<const wchar_t*>(
                            *(int*)(gppStringList +
                                    (unsigned int)g_character_skill_name_ids_61e454[iVar17] * 4)),
                        uVar9);
                    pcVar18 = pcVar18 + 1;
                } while ((int)(pcVar18 + (-1 - (int)pcVar16)) < (int)*pcVar16);
            }
            delete pcVar16;
        } else {
            g_flag_68c4f6 = '\x01';
            Function576060(1, *(int*)((int)local_20 + 0x10), 0, 0xffffffff, 0xffffffff, 2,
                           *(int*)((int)local_20 + 0x1c), 0xffffffff);
            sVar4 = wcslen(*(wchar_t**)((int)local_20 + 0x10));
            g_value_68c4b4 = sVar4 * 0x3c + 2000;
            g_value_68c4b8 = GetTickCount();
        }
        delete local_20->text;
        break;
    case 0x15:
        Function56E800(0);
        Function577520();
        pWVar7 = FindFirstMonsterByID(0x112);
        if (pWVar7 != 0) {
            uVar9 = MonsterGetIndexByLocationID(
                0xa3a, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp", pWVar7->value_9f,
                '\x01');
            pWVar6 = MonsterGetScriptPartByLocationIndex(uVar9);
            (pWVar6->monster)->SetCycleCallback004CA340(0x12, Function52A080);
            StartMonsterCycle(pWVar6, 0x12, 1);
        }
        break;
    case 0x16:
        Function56E800(0);
        pWVar7 = FindFirstMonsterByID(0xdc);
        if (pWVar7 != 0) {
            uVar9 = MonsterGetIndexByLocationID(
                0x968, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp", pWVar7->value_9f,
                '\x01');
            pWVar6 = MonsterGetScriptPartByLocationIndex(uVar9);
            StartMonsterCycle(pWVar6, 0x12, 1);
            (pWVar6->monster)->SetCycleCallback004CA340(0x12, Function52A150);
        }
        ClearMainGameTargetState();
        break;
    case 0x17:
        g_flag_68c4f6 = '\x01';
        Function576060(1, *(int*)((int)local_20 + 0x10), 0, 0xffffffff, 0xffffffff, 1,
                       *(int*)((int)local_20 + 0x1c), 0xffffffff);
        sVar4 = wcslen(*(wchar_t**)((int)local_20 + 0x10));
        g_value_68c4b4 = sVar4 * 0x3c + 2000;
        g_value_68c4b8 = GetTickCount();
        delete local_20->text;
        break;
    case 0x18:
        g_flag_68c4f6 = '\x01';
        Function576030(1, *(int*)((int)local_20 + 0x10), 0, 0xffffffff, 0xffffffff);
        sVar4 = wcslen(*(wchar_t**)((int)local_20 + 0x10));
        g_value_68c4b4 = sVar4 * 0x3c + 2000;
        g_value_68c4b8 = GetTickCount();
        delete local_20->text;
        break;
    case 0x19:
        reinterpret_cast<unsigned char*>(
            &g_status_685170.buffers
                 .party_rows[**reinterpret_cast<int**>(local_20->extra)])[0x103] = 1;
        if (g_settings_6850c8.skill_increase_messages == '\0') {
            SoundPlay("Data\\Sound\\Misc\\GainLevel.wav", 0);
            delete local_20->text;
        } else {
            g_flag_68c4f6 = '\x01';
            Function576060(1, reinterpret_cast<int>(local_20->text), 0, 0xffffffff, 0xffffffff, 3,
                           reinterpret_cast<int>(local_20->extra), 0xffffffff);
            sVar4 = wcslen(local_20->text);
            g_value_68c4b4 = sVar4 * 0x3c + 2000;
            g_value_68c4b8 = GetTickCount();
            delete local_20->text;
        }
        break;
    case 0x1a:
        pTVar8 = (Trigger*)FindTriggerByName("pillargate05");
        if (pTVar8 != 0) {
            pTVar8->Run(-1);
        }
        iVar14 = 0x3f;
        goto LAB_00527ac2;
    case 0x1b:
        pTVar8 = (Trigger*)FindTriggerByName("pillargate04");
        if (pTVar8 != 0) {
            pTVar8->Run(-1);
        }
        iVar14 = 0x3e;
        goto LAB_00527ac2;
    case 0x1c:
        pTVar8 = (Trigger*)FindTriggerByName("pillargate01");
        if (pTVar8 != 0) {
            pTVar8->Run(-1);
        }
        iVar14 = 0x3d;
    LAB_00527ac2:
        pWVar5 = GetNpcStateByKind(iVar14);
        pWVar6 = GetNpcMonsterInfo(pWVar5);
        if (pWVar6 != 0) {
        LAB_00527ad8:
            this_ = pWVar6->monster;
        LAB_0052822b:
            this_->BeginFadeOutAndRemove004C5040('\0');
        }
        break;
    case 0x1d:
        if ((gXStatus.fCombatMode == '\0') || (gXStatus.fPartyMovementMode != '\0')) {
            if (*(int*)((int)local_20 + 0x10) == 0) {
                ClearLevelDataFlag6();
            } else {
                ResetLevelDataVectors0041F0D0();
            }
        }
        break;
    case 0x1e: {
        unsigned char* status_bytes = reinterpret_cast<unsigned char*>(&g_status_685170);
        status_bytes[0x2488] = 1;
        SetCharacterCondition(reinterpret_cast<int>(local_20->text), 0x13, 9999, 0, '\0', '\0');
        status_bytes[0x2487] = 1;
        status_bytes[0x248b] = status_bytes[0x18d8];
        status_bytes[0x248c] = status_bytes[0x18d9];
        status_bytes[0x248d] = status_bytes[0x18da];
        status_bytes[0x248e] = status_bytes[0x18db];
        *reinterpret_cast<int*>(&status_bytes[0x2955]) = reinterpret_cast<int>(local_20->text);
        break;
    }
    case 0x1f:
        SetNpcDialoguePanelVisible(1);
        break;
    case 0x20:
        Function56E800(0);
        pWVar7 = FindFirstMonsterByID(0x1ab);
        if (pWVar7 != 0) {
            uVar9 = MonsterGetIndexByLocationID(
                0x983, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp", pWVar7->value_9f,
                '\x01');
            pWVar6 = MonsterGetScriptPartByLocationIndex(uVar9);
            (pWVar6->monster)->BeginFadeOutAndRemove004C5040('\0');
        }
        pWVar7 = FindFirstMonsterByID(0x15d);
        if (pWVar7 != 0) {
            Function547570(pWVar7, 1, 0);
        }
        break;
    case 0x21:
        Function56E800(0);
        this_ = GetNpcMonster(g_npc_state_68c4ac);
        goto joined_r0x00528225;
    case 0x22:
        Function56E800(0);
        pWVar5 = GetNpcStateByKind(0x62);
        if ((pWVar5 == 0) || (pWVar6 = GetNpcMonsterInfo(pWVar5), pWVar6 == 0))
            break;
        if (pWVar6->flag_14 != '\0') {
            MonsterStartsDying(pWVar6, 1);
            break;
        }
        iVar14 = pWVar6->location_id;
        iVar17 = 0xbbf;
        goto LAB_005281d1;
    case 0x23:
        Function56E800(0);
        pWVar5 = GetNpcStateByKind(100);
        if ((pWVar5 == 0) || (pWVar6 = GetNpcMonsterInfo(pWVar5), pWVar6 == 0))
            break;
        if (pWVar6->flag_14 == '\0') {
            iVar14 = pWVar6->location_id;
            iVar17 = 0xc06;
            goto LAB_005281d1;
        }
        this_ = pWVar6->monster;
    joined_r0x00528225:
        if (this_ == 0)
            break;
        goto LAB_0052822b;
    case 0x24:
        Function56E800(0);
        Function577520();
        pWVar7 = FindFirstMonsterByID(0x162);
        if (pWVar7 != 0) {
            uVar9 = MonsterGetIndexByLocationID(
                0x78d, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp", pWVar7->value_9f,
                '\x01');
            pWVar6 = MonsterGetScriptPartByLocationIndex(uVar9);
            (pWVar6->monster)->SetScript004C7F10("MoveGari.msf", '\x01');
        }
        break;
    case 0x25:
        pTVar8 = (Trigger*)FindTriggerByName("RatDoor02");
        if (pTVar8 == 0) {
            srAssertFail("pDoor", "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp", 0x7b6,
                         0);
        }
        pTVar8->CompleteItemInteraction004447F0();
        Function56E800(0);
        pWVar7 = FindFirstMonsterByID(0xcf);
        if (pWVar7 != 0) {
            uVar9 = MonsterGetIndexByLocationID(
                0x7c1, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp", pWVar7->value_9f,
                '\x01');
            pWVar6 = MonsterGetScriptPartByLocationIndex(uVar9);
            (pWVar6->monster)->SetScript004C7F10("Milano.msf", '\x01');
        }
        break;
    case 0x26:
        Function56E800(0);
        pWVar5 = GetNpcStateByKind(0x4d);
        if ((pWVar5 == 0) || (pWVar6 = GetNpcMonsterInfo(pWVar5), pWVar6 == 0))
            break;
        if (pWVar6->flag_14 == '\0') {
            iVar14 = pWVar6->location_id;
            iVar17 = 0xba8;
            goto LAB_005281d1;
        }
        goto LAB_00527ad8;
    case 0x27:
        iVar14 = Function52FEE0(reinterpret_cast<int>(local_20->text),
                                g_status_685170.selected_party_member_2434);
        if (iVar14 != -1) {
            g_status_685170.selected_party_member_2434 = (unsigned char)iVar14;
            QueueCharacterEvent(&g_status_685170.buffers.characters[iVar14],
                                reinterpret_cast<int>(local_20->text), g_effect_argument_005ed8e0,
                                g_effect_argument_005ed8c8, g_effect_argument_005ed914);
            SetNpcDialoguePanelVisible(0);
            if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
                g_screen_state_00649f1c->unknown_23c[0] = 1;
            }
        }
        break;
    case 0x28:
        iVar14 = g_effect_005ee58c;
        uVar9 = g_effect_argument_005ed8e0;
        goto LAB_00527909;
    case 0x29:
        Function56E800(0);
        Function577520();
        pWVar7 = FindFirstMonsterByID(0x83);
        if (pWVar7 != 0) {
            uVar9 = MonsterGetIndexByLocationID(
                0x7a8, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp", pWVar7->value_9f,
                '\x01');
            pWVar6 = MonsterGetScriptPartByLocationIndex(uVar9);
            (pWVar6->monster)->SetScript004C7F10("MoveRubble.msf", '\x01');
        }
        break;
    case 0x2a:
        Function56E800(0);
        SetTriggerVariableByName00444030("LezboDemonAppeared", 0);
        pWVar5 = GetNpcStateByKind(0x40);
        if ((pWVar5 == 0) || (pWVar6 = GetNpcMonsterInfo(pWVar5), pWVar6 == 0))
            break;
        if (pWVar6->flag_14 != '\0')
            goto LAB_00527ad8;
        iVar14 = pWVar6->location_id;
        iVar17 = 0xbd8;
        goto LAB_005281d1;
    case 0x2b:
        pcVar16 = "triggerFix";
    LAB_0052807e:
        pTVar8 = (Trigger*)FindTriggerByName(pcVar16);
        if (pTVar8 != 0) {
            pTVar8->Run(-1);
        }
        break;
    case 0x2c:
        Function56E800(0);
        pWVar5 = GetNpcStateByKind(0x42);
        if ((pWVar5 == 0) || (pWVar6 = GetNpcMonsterInfo(pWVar5), pWVar6 == 0))
            break;
        if (pWVar6->flag_14 != '\0')
            goto LAB_00527ad8;
        iVar14 = pWVar6->location_id;
        iVar17 = 0xbef;
        goto LAB_005281d1;
    case 0x2d:
        pWVar5 = GetNpcStateByKind(0xc);
        pWVar6 = GetNpcMonsterInfo(pWVar5);
        if ((pWVar6 != 0) &&
            (cVar2 = FindEntityByName("NP_Balbrakhome", &local_18, 0, 0), cVar2 != '\0')) {
            pWVar6->monster->AimAtPosition(&local_18);
        }
        break;
    case 0x2e:
        uVar9 = 0;
        iVar14 = 2;
        do {
            if ((g_status_685170.buffers.party_rows[iVar14].occupied != 0) &&
                (reinterpret_cast<unsigned int*>(
                     &g_status_685170.buffers.characters[iVar14])[0xef1] < 0xf)) {
                uVar9 = uVar9 + 1;
            }
            iVar14 = iVar14 + 1;
        } while (iVar14 < 8);
        if (1 < uVar9) {
            uVar9 = 0;
            do {
                if ((g_status_685170.buffers.party_rows[uVar9].occupied != 0) &&
                    (reinterpret_cast<unsigned int*>(
                         &g_status_685170.buffers.characters[uVar9])[0x2c0] == 10) &&
                    (reinterpret_cast<unsigned int*>(
                         &g_status_685170.buffers.characters[uVar9])[0x2c4] < 0xf)) {
                    QueueCharacterEvent(&g_status_685170.buffers.characters[uVar9],
                                        g_effect_005ee654, g_effect_argument_005ed8e0,
                                        g_effect_argument_005ed8c8, g_effect_argument_005ed914);
                    break;
                }
                uVar9 = uVar9 + 1;
            } while (uVar9 < 8);
        }
        break;
    case 0x2f:
        pWVar7 = FindFirstMonsterByID(0x1b6);
        if (pWVar7 != 0) {
            uVar9 = MonsterGetIndexByLocationID(
                0xb2e, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp", pWVar7->value_9f,
                '\x01');
            pWVar6 = MonsterGetScriptPartByLocationIndex(uVar9);
            StartMonsterCycle(pWVar6, 0x19, 1);
            (pWVar6->monster)->SetCycleCallback004CA340(0x19, Function52A190);
        }
        break;
    case 0x30:
        Function56E800(0);
        pWVar7 = FindFirstMonsterByID(0x1b4);
        if (pWVar7 != 0) {
            uVar9 = MonsterGetIndexByLocationID(
                0xa57, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp", pWVar7->value_9f,
                '\x01');
            pWVar6 = MonsterGetScriptPartByLocationIndex(uVar9);
            (pWVar6->monster)->SetScript004C7F10("belapath1.msf", '\x01');
        }
        pTVar8 = FindTriggerByName("CC_TRIGGERPLANE3");
        if (pTVar8 != 0) {
            pTVar8->flags_0a0 = pTVar8->flags_0a0 & 0xffffffef;
        }
        pTVar8 = (Trigger*)FindTriggerByName("CC_TRIGGERPLANE2");
        if (pTVar8 != 0) {
            pTVar8->flags_0a0 = pTVar8->flags_0a0 | 0x10;
            pTVar8->Run(-1);
        }
        break;
    case 0x31:
        pTVar8 = (Trigger*)FindTriggerByName("CC_TRIGGERPLANE3");
        if (pTVar8 != 0) {
            pTVar8->flags_0a0 = pTVar8->flags_0a0 | 0x10;
            pTVar8->Run(-1);
        }
        pWVar5 = GetNpcStateByKind(0x8d);
        if (pWVar5 != 0) {
            Function56C5E0(pWVar5, 0, 7, 0, 0);
        }
        break;
    case 0x32:
        cVar2 = FindEntityByName("NP_DS1", &local_18, 0, 0);
        if (cVar2 != '\0') {
            iVar14 =
                SpawnMonsters(0x234, 1, reinterpret_cast<unsigned char*>(&local_18), 0, 1, 0, 0);
            iVar14 = IListGetAt(*(W8IList**)(iVar14 + 8), 0);
            if (iVar14 != 0) {
                uVar9 = MonsterGetIndexByLocationID(
                    0xa9e, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp", iVar14,
                    '\x01');
                pWVar6 = MonsterGetScriptPartByLocationIndex(uVar9);
                MonsterForwardReferencePosition(pWVar6->monster, '\0');
            }
            pWVar5 = GetNpcStateByKind(0x84);
            if (pWVar5 != 0) {
                Function56C5E0(pWVar5, 0, 0xffffffff, 0, 0);
            }
        }
        break;
    case 0x33:
        uVar3 = NpcLeadHasNameStyle(0x18);
        if (uVar3 != '\0') {
            pWVar5 = GetNpcStateByKind(0x18);
            if (pWVar5 != 0) {
                Function50B590((int)pWVar5->group_index, 0, 1, 0);
            }
            cVar2 = FindEntityByName("NP_VI1", &local_18, 0, 0);
            if (cVar2 != '\0') {
                iVar14 = SpawnMonsters(0x1b9, 1, reinterpret_cast<unsigned char*>(&local_18), 2, 1,
                                       0, 0);
                iVar14 = IListGetAt(*(W8IList**)(iVar14 + 8), 0);
                if (iVar14 != 0) {
                    uVar9 = MonsterGetIndexByLocationID(
                        0xafc, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp", iVar14,
                        '\x01');
                    pWVar6 = MonsterGetScriptPartByLocationIndex(uVar9);
                    cVar2 = FindEntityByName("NP_DS1", &local_c, 0, 0);
                    if (cVar2 != '\0') {
                        pWVar6->monster->AimAtPosition(&local_c);
                    }
                }
            }
        }
        break;
    case 0x34:
        pWVar5 = GetNpcStateByKind(0x84);
        if ((pWVar5 != 0) && (pWVar6 = GetNpcMonsterInfo(pWVar5), pWVar6 != 0)) {
            StartMonsterCycle(pWVar6, 0x1a, 1);
        }
        cVar2 = FindEntityByName("NP_PHOONZANGLEE", &local_18, 0, 0);
        if ((cVar2 != '\0') && (pWVar7 = FindFirstMonsterByID(0x197), pWVar7 != 0)) {
            uVar9 = MonsterGetIndexByLocationID(
                0xacf, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp", pWVar7->value_9f,
                '\x01');
            pWVar6 = MonsterGetScriptPartByLocationIndex(uVar9);
            pWVar6->monster->AimAtPosition(&local_18);
            MonsterForwardReferencePosition(pWVar6->monster, '\0');
            StartMonsterCycle(pWVar6, 0x12, 1);
        }
        pWVar5 = GetNpcStateByKind(0x8d);
        if (pWVar5 != 0) {
            Function56C5E0(pWVar5, 0, 0x12, 0, 0);
        }
        break;
    case 0x35:
        ClearMainGameTargetState();
        Function5A6580();
        break;
    case 0x36:
        if (g_screen_state_00649f1c != 0) {
            iVar14 = *(int*)((int)local_20 + 0x10);
            if (*(int*)(g_screen_state_00649f1c + 0x7b4) == iVar14) {
                *(int*)(g_screen_state_00649f1c + 0x7b0) = 0;
                *(int*)(g_screen_state_00649f1c + 0x7b4) = 0xffffffff;
            }
            ClearMainGameTargetState();
            Function50B590(iVar14, 0, 0, 1);
            SetTargetToCharacter(iVar14, W8_TARGETING_CONTEXT_OUT_OF_COMBAT);
            *(unsigned char*)(g_screen_state_00649f1c + 0xa58 + iVar14) = 0;
        }
        break;
    case 0x37:
        Function56CA90();
        break;
    case 0x38:
        Function569A50(*(int*)(gppStringList + 0x1fac), reinterpret_cast<void*>(Function52A1B0), 1,
                       1);
        g_value_68c358 = *(int*)((int)local_20 + 0x10);
        break;
    case 0x39:
        Function56E800(0);
        pWVar7 = FindFirstMonsterByID(0x1aa);
        if (pWVar7 == 0)
            break;
        uVar9 = MonsterGetIndexByLocationID(
            0x9a0, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp", pWVar7->value_9f,
            '\x01');
        pWVar6 = MonsterGetScriptPartByLocationIndex(uVar9);
        this_ = pWVar6->monster;
        goto LAB_0052822b;
    case 0x3a:
        iVar14 = 0x18;
        uVar9 = g_effect_argument_005ed8ec | g_effect_argument_005ed8e0;
    LAB_00527909:
        QueueCharacterEvent((W8Character*)((int)g_status_685170.buffers.characters +
                                           *(int*)((int)local_20 + 0x10) * 0x1862),
                            iVar14, uVar9, g_effect_argument_005ed8c8, g_effect_argument_005ed914);
        break;
    case 0x3c:
        pWVar5 = GetNpcStateByKind(0x87);
        if (pWVar5 != 0) {
            Function56C5E0(pWVar5, 0, 0xffffffff, 0, 0);
        }
        break;
    case 0x3d:
        puVar21 = reinterpret_cast<unsigned char*>(Function526E40);
        goto LAB_00527f96;
    case 0x3e:
        Function56E800(0);
        pWVar5 = GetNpcStateByKind(0x2e);
        if ((pWVar5 == 0) || (pWVar6 = GetNpcMonsterInfo(pWVar5), pWVar6 == 0))
            break;
        iVar14 = pWVar6->location_id;
        iVar17 = 0x8c2;
        goto LAB_005281d1;
    case 0x3f:
        Function56E800(0);
        pWVar5 = GetNpcStateByKind(0x2d);
        if ((pWVar5 == 0) || (pWVar6 = GetNpcMonsterInfo(pWVar5), pWVar6 == 0))
            break;
        iVar14 = pWVar6->location_id;
        iVar17 = 0x8dd;
        goto LAB_005281d1;
    case 0x40:
        Function56E800(0);
        pWVar5 = GetNpcStateByKind(0x2f);
        if ((pWVar5 == 0) || (pWVar6 = GetNpcMonsterInfo(pWVar5), pWVar6 == 0))
            break;
        iVar14 = pWVar6->location_id;
        iVar17 = 0x8f7;
    LAB_005281d1:
        uVar3 = '\x01';
        uVar9 = MonsterGetIndexByLocationID(
            iVar17, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp", iVar14, '\x01');
        RemoveMonster(uVar9, uVar3);
        break;
    case 0x41:
        Function529F90();
        break;
    case 0x42:
        puVar21 = reinterpret_cast<unsigned char*>(Function526E70);
    LAB_00527f96:
        Function5A6620(0, 0, 500, puVar21, 1, 1);
    }
    iVar14 = 0;
    ppLine = g_message_box_lines;
    if (0 < g_message_box_line_count) {
        while (*ppLine != local_20) {
            iVar14 = iVar14 + 1;
            ppLine = ppLine + 1;
            if (g_message_box_line_count <= iVar14) {
                delete local_20;
                return;
            }
        }
        if ((-1 < iVar14) && (iVar14 < g_message_box_line_count)) {
            if (iVar14 < g_message_box_line_count + -1) {
                do {
                    g_message_box_lines[iVar14] = g_message_box_lines[iVar14 + 1];
                    iVar14 = iVar14 + 1;
                } while (iVar14 < g_message_box_line_count + -1);
            }
            g_message_box_line_count = g_message_box_line_count + -1;
        }
    }
    delete local_20;
    return;
}
