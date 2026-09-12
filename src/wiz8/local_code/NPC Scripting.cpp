#include "wiz8/character.h"
#include "wiz8/combat_state.h"
#include "wiz8/game_status.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/Screens.h"
#include "wiz8/npc_interaction.h"
#include "wiz8/npc_state.h"
#include "wiz8/message_box.h"
#include "wiz8/xstatus.h"
#include "wiz8/engine_code/Monster.h"
#include "soundman.h"

#include <string.h>
#include <windows.h>

extern void Function525C50(unsigned char value);
extern void Function526E90(void);
extern unsigned char Function52E470(void);
extern void Function577880(int value);
extern int GetEnvironmentValue0060A3A8(void);
extern W8Monster* GetNpcMonster(W8NpcState* npc);
extern unsigned char Function577850(void);
extern void Function5E2F40(int sound_handle, unsigned char* state);

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
unsigned char Function525DD0(void)
{
    return g_flag_68c4a0 != 0 || g_flag_68c4f6 != 0;
}

// FUNCTION: WIZ8 0x00525DF0
unsigned char Function525DF0(unsigned char require_group_entry)
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
                    Function525C50(1);
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
            Function525C50(1);
        }
    }
update_cursor:
    if (gXStatus.fNpcDialogueMode != 0) {
        if (reinterpret_cast<unsigned char*>(g_screen_state_00649f1c)[0x228] != 0) {
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
        if (reinterpret_cast<unsigned char*>(g_screen_state_00649f1c)[0x1fa] < 1) {
            Function526E90();
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
                g_flag_0068506f == 0 &&
                reinterpret_cast<unsigned char*>(g_screen_state_00649f1c)[0x1fa] == 0 &&
                Function577850() != 0) {
                Function56E800(0);
            }
            if (reinterpret_cast<unsigned char*>(g_screen_state_00649f1c)[0x1fa] == 0 &&
                reinterpret_cast<unsigned char*>(g_screen_state_00649f1c)[0x262] != 0 &&
                Function52E470() != 0) {
                Function577880(1);
            }
        }
    }
}
