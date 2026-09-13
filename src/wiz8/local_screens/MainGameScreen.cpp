#include "line.h"
#include "soundman.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/Sight.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/local_code/FormationAndFacing.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/local_code/party_encumbrance.h"
#include "wiz8/npc_interaction.h"
#include "wiz8/video_object_catalog.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/engine_code/Levels.h"
#include "wiz8/local_screens/MGSSpellCasting.h"
#include "wiz8/local_screens/AutomapScreen.h"
#include "wiz8/targeting.h"
#include "wiz8/combat_state.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/npc_state.h"
#include "wiz8/fact_state.h"
#include "wiz8/local_screens/MGSKeyboard.h"
#include "wiz8/render_state.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/local_screens/RCSCommon.h"
#include "wiz8/local_code/LoadSaveGame.h"
#include "wiz8/dialog_code/DialogInterface.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/cursor.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/Navigator.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/music_playlist.h"
#include "wiz8/sr_api.h"
#include "wiz8/utility.h"
#include "wiz8/notices.h"
#include "wiz8/regions.h"
#include "wiz8/screen_state.h"
#include "wiz8/fonts.h"
#include "wiz8/local_code/GameplayDatabase.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/engine_code/Environment.h"
#include "wiz8/local_screens/MGSUseItemSelect.h"
#include "wiz8/character_event_queue.h"
#include "wiz8/xstatus.h"
#include "wiz8/wiz8_windows.h"
#include "wiz8/world_cursor.h"

#include "font.h"
#include "FileMan.h"
#include "input.h"
#include "timer.h"
#include "Types.h"
#include "mousesystem.h"
#include "surrender/srTypeRegistry.h"
#include "wiz8/engine_code/Trigger.h"
#include "wiz8/engine_code/3dapi.h"
#include "wiz8/local_code/ItemManager.h"
#include "wiz8/local_screens/MGSPartyMovement.h"
#include "wiz8/engine_code/Cursor3d.h"
#include "wiz8/local_code/NPCScripting.h"
#include "wiz8/local_screens/Screens.h"
#include "wiz8/local_screens/mipe.h"
#include "wiz8/local_screens/MGSPortraits.h"
#include "wiz8/local_screens/ReviewCharacterScreen.h"
#include "wiz8/local_screens/IntroScreen.h"
#include "wiz8/local_screens/JournalScreen.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/character.h"
#include "wiz8/character_skills.h"
#include "wiz8/magic.h"
#include "wiz8/float_constants.h"
#include "wiz8/local_code/Traps.h"
#include "wiz8/local_code/ButtonSound.h"
#include "vobject_blitters.h"
#include "random.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wiz8/game_status.h"

/*
 * Local Screens\MainGameScreen.cpp.
 *
 * The screen the game is played on. Its frame coordinates input, dialogs,
 * world updates and drawing. Other units request UI updates through the
 * level runtime block's redraw word.
 */

/* Every redraw request checks the screen state first, so a request made from
   another screen is simply dropped. */

/* The region set the two enable/disable wrappers below own. */
enum { W8_REGION_SET_MAIN = 4 };
unsigned char g_flag_006840bd;
// GLOBAL: WIZ8 0x0068edcc
W8LevelRuntimeBlock* g_level_block;
// GLOBAL: WIZ8 0x0068edd0
W8DialogBase* g_modal_owner_0068edd0;
// GLOBAL: WIZ8 0x0068edd4
W8DialogBase* g_pending_main_game_dialog_0068edd4;

// GLOBAL: WIZ8 0x006840bc
unsigned char g_flag_006840bc;

// GLOBAL: WIZ8 0x006840be
unsigned short g_value_006840be;

// GLOBAL: WIZ8 0x00685070
unsigned char g_flag_00685070;

// GLOBAL: WIZ8 0x00685071
unsigned char g_flag_00685071;

// GLOBAL: WIZ8 0x00685072
int g_value_00685072;

// GLOBAL: WIZ8 0x00685076
unsigned char g_flag_00685076;

// GLOBAL: WIZ8 0x00685077
signed char g_value_00685077;

// GLOBAL: WIZ8 0x0068edbc
unsigned char g_flag_0068edbc;

// GLOBAL: WIZ8 0x0068edc8
unsigned char g_flag_0068edc8;

// GLOBAL: WIZ8 0x0068edc9
unsigned char g_flag_0068edc9;

// GLOBAL: WIZ8 0x0068edd8
unsigned char g_flag_0068edd8;

// GLOBAL: WIZ8 0x0068eddc
int g_main_game_mode_0068eddc;

// GLOBAL: WIZ8 0x0064827c
W8MainGameResourceSlot g_main_game_resource_slots[17] = {
    {0, 1, 0, 0, 0, 0, 2},  {0, 5, 0, 0, 0, 0, 3},  {0, 1, 0, 0, 0, 0, 4},  {0, 1, 0, 0, 0, 0, 5},
    {0, 1, 0, 0, 0, 0, 6},  {0, 4, 0, 0, 0, 0, 7},  {0, 4, 0, 0, 0, 0, 0},  {0, 0, 0, 0, 0, 0, 8},
    {0, 1, 0, 0, 0, 0, 9},  {0, 1, 0, 0, 0, 0, 10}, {0, 5, 0, 0, 0, 0, 11}, {0, 1, 0, 0, 0, 0, 12},
    {0, 1, 0, 0, 0, 0, 13}, {0, 4, 0, 0, 0, 0, 15}, {0, 1, 0, 0, 0, 0, 14}, {0, 1, 0, 0, 0, 0, 16},
    {0, 1, 0, 0, 0, 0, 0},
};

// GLOBAL: WIZ8 0x0065bd2c
unsigned char g_build_level_links_0065bd2c;

// GLOBAL: WIZ8 0x0068ede8
int g_next_link_level_0068ede8;

// GLOBAL: WIZ8 0x0068edd9
unsigned char g_flag_0068edd9;

// GLOBAL: WIZ8 0x0068ee90
W8MainScreenState g_screen_state_storage_0068ee90;
// GLOBAL: WIZ8 0x00649f1c
W8MainScreenState* g_screen_state_00649f1c = &g_screen_state_storage_0068ee90;
/* 0x0068EE80: the dialogue keyword tables. Element zero is the English file
   list and element one the translated one; each file list holds one line list
   per line and each line list one word per field. */
// GLOBAL: WIZ8 0x0068EE80
W8GrowableVector<W8GrowableVector<W8GrowableVector<wchar_t*>*>*> g_keyword_lists;
/* 0x0068F0F8: both keyword files are loaded and the tables are usable. */
// GLOBAL: WIZ8 0x0068F0F8
unsigned char g_keyword_lists_loaded_68f0f8;
/* 0x0068F0F9: the keyword subsystem's active flag, written absolutely by the
   screen reset and by the keyword panel helpers. */
// GLOBAL: WIZ8 0x0068F0F9
unsigned char g_flag_68f0f9;
// GLOBAL: WIZ8 0x0068f0fc
unsigned char g_debug_monster_cycle_0068f0fc;

// GLOBAL: WIZ8 0x0068f100
W8IList* g_debug_monster_ids_0068f100;

// GLOBAL: WIZ8 0x0068f2c8
unsigned int g_main_game_text_panel_region_set_0068f2c8;
// GLOBAL: WIZ8 0x0068f2cc
unsigned int g_main_game_text_key_region_set_0068f2cc;
// GLOBAL: WIZ8 0x0068f2d0
unsigned int g_main_game_action_panel_region_set_0068f2d0;

// GLOBAL: WIZ8 0x0061e9ec
unsigned short g_value_0061e9ec[] = {
    0x542, 0x543, 0x544, 0x545, 0x546, 0x547, 0x548, 0x549, 0x54a, 0x54b, 0x54c,
    0x54d, 0x54e, 0x54f, 0x550, 0,     0x551, 0x552, 0x553, 0x554, 0x555, 0,
    0x556, 0x557, 0x558, 0x559, 0x55a, 0x55b, 0x55c, 0x563, 0x56a, 0x55c,
};

// GLOBAL: WIZ8 0x006504e8
int g_table_6504e8[] = {10, 25, 35, 40, 50, 60, 70, 80, 90, 100, 110, 121, 122, 123, 24, 47};

// GLOBAL: WIZ8 0x0064bbac
const char* g_trap_sounds_0064bbac[8] = {
    "Data\\Sound\\Misc\\Trap 03.wav", "Data\\Sound\\Misc\\Trap 07.wav",
    "Data\\Sound\\Misc\\Trap 01.wav", "Data\\Sound\\Misc\\Trap 05.wav",
    "Data\\Sound\\Misc\\Trap 02.wav", "Data\\Sound\\Misc\\Trap 08.wav",
    "Data\\Sound\\Misc\\Trap 06.wav", "Data\\Sound\\Misc\\Trap 04.wav",
};

// GLOBAL: WIZ8 0x0064bcac
const char g_trap_inspection_sound_0064bcac[] = "Data\\Sound\\Misc\\Trap Inspection.wav";

// GLOBAL: WIZ8 0x0064bcd0
const char g_trap_sprung_sound_0064bcd0[] = "Data\\Sound\\Misc\\Trap Sprung.wav";

// GLOBAL: WIZ8 0x0064bab0
const wchar_t g_format_d_percent_0064bab0[] = L"%d%%";

// GLOBAL: WIZ8 0x006068e4
const wchar_t g_format_s_006068e4[] = L"%s";

// GLOBAL: WIZ8 0x005ec258
const float g_float_005ec258 = 0.019999999552965164f;
// GLOBAL: WIZ8 0x005eebbc
const float g_float_005eebbc = 120.0f;

// GLOBAL: WIZ8 0x00659c11
unsigned char g_navigator_position_changed_659c11;

// GLOBAL: WIZ8 0x006840BB
unsigned char g_flag_006840bb;

void Function4314C0(int save);
void RequestLevelTransition005615F0(int level, int entry, unsigned char flag);
void Function568C40(void);
void Function569CC0(void);
void Function5A6970(void);
unsigned char Function5A6790(void);
void Function5A68C0(void);
void Function50B3B0(int value);
void Function5542E0(void);
void Function59A3A0(void);
void Function575C50(void);
void Function577560(void);
void Function59B1A0(void);
void Function59B4C0(void);
void Function59B390(void);
void Function502650(void);
void Function562A80(void);
void Function4916C0(void);
unsigned char Function5684E0(void);
void Function561330(unsigned char value);
void Function57E0E0(int event, const POINT* point);
void Function59B2D0(void);
void Function5171C0(void);
void Function4E8EA0(void);
void StartCombat(int surprise);
void Function530110(void);
void Function530150(int value);
void Function56E510(void);
void Function586740(void);
void Function593360(void);
void Function56C6D0(int, int, int, int, int);
unsigned char Function57E3C0(void);

// FUNCTION: WIZ8 0x00587960
void Function587960(void)
{
    Function586740();
}

// FUNCTION: WIZ8 0x00587cf0
W8MainGameTextKeyHandler::W8MainGameTextKeyHandler(Controls* panel, int left, int top, int right,
                                                   int bottom, int line_count,
                                                   unsigned short* field_ac,
                                                   unsigned int* region_set)
    : W8Widget(panel, 0xffffffff, left, top, right - 0x13, bottom),
      m_range_038(panel->origin_x - 0x12 + right, panel->origin_y + top, right + panel->origin_x,
                  panel->origin_y + bottom, region_set)
{
    m_line_count_0a4 = line_count;
    m_visible_lines_0a8 = (bottom - top) / 0xe;
    m_field_0ac = field_ac;
    m_field_0b0 = 0;
    m_field_0b8 = 0;
    m_range_listener_0bc = 0;
    m_field_0b4 = -1;
    m_range_038.m_listener = this;
    m_range_038.SetRange(0, m_line_count_0a4 - m_visible_lines_0a8);
    m_range_038.Invalidate(0);
    m_range_038.SetEnabled(1);
    m_range_038.SetRangeEnabled(1);
}

// SYNTHETIC: WIZ8 0x00587e30
// W8MainGameTextKeyHandler::`scalar deleting destructor'

// FUNCTION: WIZ8 0x00587e50
W8MainGameTextKeyHandler::~W8MainGameTextKeyHandler() {}

// FUNCTION: WIZ8 0x00587ea0
void W8MainGameTextKeyHandler::Redraw(int full_redraw)
{
    int left;
    int top;
    int right;
    int bottom;
    int line;
    int last;
    unsigned short* colour;

    if (!m_active) {
        return;
    }
    if (!m_dirty && full_redraw == 0) {
        return;
    }
    left = m_pPanel->origin_x + m_left;
    top = m_pPanel->origin_y + m_top;
    right = m_pPanel->origin_x + m_right;
    bottom = m_pPanel->origin_y + m_bottom;
    InvalidateRegion(left, top, right, bottom, 0);
    BlitCatalogSurfaceRectTo16BPP(-14, left, top, right, bottom, 0x1b6, 0, 0);
    SetFont(g_font_683660);
    last = m_field_0b8 + m_visible_lines_0a8;
    left += 2;
    top += 1;
    for (line = m_field_0b8; line < last; ++line) {
        if (line == m_field_0b0) {
            colour = g_font_state_palettes_68ee1c[3];
        } else if (line == m_field_0b4) {
            colour = g_font_state_palettes_68ee1c[4];
        } else {
            colour = g_colour_68ee08;
        }
        SetFontObjectPalette16BPP(g_font_683660, colour);
        mprintf(left, top, const_cast<wchar_t*>(g_format_s_006068e4),
                gppStringList[m_field_0ac[line]]);
        top += 0xe;
    }
    SetFontObjectPalette16BPP(g_font_683660, g_colour_68ee08);
    m_range_038.Invalidate(0);
    m_dirty = 0;
}

// FUNCTION: WIZ8 0x00587ff0
void W8MainGameTextKeyHandler::OnMouseLeave(int event)
{
    m_field_0b4 = -1;
    Invalidate((unsigned char)event);
}

// FUNCTION: WIZ8 0x00588010
void W8MainGameTextKeyHandler::OnMouseMove(int)
{
    POINT point;
    int line;

    SGPMouseGetPos(&point);
    line = (point.y - m_pPanel->origin_y - m_top) / 0xe + m_field_0b8;
    if (line != m_field_0b4) {
        m_field_0b4 = line;
        Invalidate(0);
    }
}

// FUNCTION: WIZ8 0x00588070
void W8MainGameTextKeyHandler::AdjustValue(int steps)
{
    if (steps > 0) {
        for (; steps > 0; --steps) {
            m_range_038.Decrement();
        }
    } else {
        for (; steps < 0; ++steps) {
            m_range_038.Increment();
        }
    }
}

// FUNCTION: WIZ8 0x005880b0
void W8MainGameTextKeyHandler::OnLeftButtonUp(int)
{
    POINT point;

    SGPMouseGetPos(&point);
    SetSelectedLine((point.y - m_pPanel->origin_y - m_top) / 0xe + m_field_0b8);
}

// FUNCTION: WIZ8 0x00588100
void W8MainGameTextKeyHandler::SetSelectedLine(int line)
{
    if (line == m_field_0b0) {
        return;
    }
    m_field_0b0 = line;
    Invalidate(0);
    if (m_field_0b0 < m_range_038.m_value ||
        m_field_0b0 >= m_range_038.m_value + m_visible_lines_0a8) {
        int first = m_field_0b0;
        if (m_field_0b0 >= m_range_038.m_value) {
            first = m_field_0b0 - m_visible_lines_0a8 + 1;
        }
        m_field_0b8 = first;
        m_range_038.SetValue(first);
    }
    if (m_range_listener_0bc != 0) {
        m_range_listener_0bc->OnRangeChanged(&m_range_038);
    }
}

// FUNCTION: WIZ8 0x00588170
char W8MainGameTextKeyHandler::HandleKey(unsigned short key)
{
    int line;

    switch (key) {
    case 0x21:
        line = m_field_0b0 - 5;
        if (line < 0) {
            line = 0;
        }
        SetSelectedLine(line);
        return 1;
    case 0x22:
        line = m_field_0b0 + 5;
        if (m_line_count_0a4 - 1 < line) {
            line = m_line_count_0a4 - 1;
        }
        SetSelectedLine(line);
        return 1;
    case 0x23:
        SetSelectedLine(m_line_count_0a4 - 1);
        return 1;
    case 0x24:
        SetSelectedLine(0);
        return 1;
    case 0x26:
        SetSelectedLine(m_field_0b0 - 1 < 0 ? 0 : m_field_0b0 - 1);
        return 1;
    case 0x28:
        line = m_field_0b0 + 1;
        if (m_line_count_0a4 - 1 < line) {
            line = m_line_count_0a4 - 1;
        }
        SetSelectedLine(line);
        return 1;
    default:
        return 0;
    }
}

// FUNCTION: WIZ8 0x00588240
void W8MainGameTextKeyHandler::OnRangeChanged(W8RangeControl* control)
{
    m_field_0b8 = control->m_value;
    Invalidate(0);
}

// FUNCTION: WIZ8 0x00588260
W8MainGameTextEntry::W8MainGameTextEntry(Controls* panel, int index)
{
    int sprites = index * 4;
    int left;
    int top;

    m_input_blocked_bc = 0;
    m_imageFrame = 0;
    m_image_b8 = -1;
    m_alternatePressedSprite = -1;
    m_normalSprite = sprites;
    m_pressedSprite = sprites + 2;
    m_imageObject = 0x1b2;
    m_alternateNormalSprite = sprites + 1;
    m_disabledSprite = sprites + 3;
    MeasureText004F4800();
    left = (index % 4) * 0x1e + 0xd;
    top = (index / 4) * 0x32 + 8;
    m_left = left;
    m_right = m_measured_w + left;
    m_bottom = m_measured_h + top;
    m_top = top;
    SetPanel(panel);
    AddLayoutFlags(g_W8TextControlMask005ED588 | g_W8TextControlMask005ED578);
}

// SYNTHETIC: WIZ8 0x00588350
// W8MainGameTextEntry::`scalar deleting destructor'

// FUNCTION: WIZ8 0x00588370
W8MainGameTextEntry::~W8MainGameTextEntry() {}

// FUNCTION: WIZ8 0x005883c0
void W8MainGameTextEntry::Redraw(int full_redraw)
{
    bool dirty = m_dirty;
    int image;
    int left;
    int top;

    W8TextControl::Redraw(full_redraw);
    if (!m_active) {
        return;
    }
    if (!dirty && full_redraw == 0) {
        return;
    }
    image = m_image_b8;
    left = m_pPanel->origin_x + m_left;
    top = m_pPanel->origin_y + m_top;
    if (image == 0) {
        DrawCatalogImage(-14, 0x1b3, 0, 2, left, top, 2, 0);
    } else if (image == 1) {
        DrawCatalogImage(-14, 0x1b3, 0, 0, left, top, 2, 0);
    } else if (image == 2) {
        DrawCatalogImage(-14, 0x1b3, 0, 1, left, top, 2, 0);
    }
}

// FUNCTION: WIZ8 0x00588440
void W8MainGameTextEntry::OnLeftButtonDown(int event)
{
    if (m_input_blocked_bc) {
        PushButtonSoundScheme005587C0(0, 1);
        return;
    }
    W8TextControl::OnLeftButtonDown(event);
}

// FUNCTION: WIZ8 0x00588470
void W8MainGameTextEntry::OnLeftButtonUp(int event)
{
    if (m_input_blocked_bc) {
        PushButtonSoundScheme005587C0(0, 1);
        return;
    }
    W8TextControl::OnLeftButtonUp(event);
}

// FUNCTION: WIZ8 0x005884a0
void W8MainGameTextEntry::OnMouseEnter(int event)
{
    if (m_input_blocked_bc) {
        PushButtonSoundScheme005587C0(0, 1);
        return;
    }
    W8TextControl::OnMouseEnter(event);
}

// FUNCTION: WIZ8 0x005884d0
W8MainGameTextPanel::W8MainGameTextPanel()
    : Controls(0xd3, 0x166, 0, 0, 0x1b1, 0, 1), m_selection_078(0), m_screen_07c(0),
      m_values_080(0), m_flag_084(0), m_timer_118(0.04f, 0)
{
    int index;

    m_flag_141 = 0;
    AcquireRegionSet(&g_main_game_text_panel_region_set_0068f2c8);
    for (index = 0; index < 8; ++index) {
        m_entries_054[index] = new W8MainGameTextEntry(this, index);
        m_entries_054[index]->m_listener = this;
        m_entries_054[index]->EnableRegionHelp(0x7d1);
    }
    m_key_handler_074 =
        new W8MainGameTextKeyHandler(this, 0x8e, 5, 0x10f, 0x59, 0xf, g_value_0061e9ec,
                                     &g_main_game_text_key_region_set_0068f2cc);
    m_key_handler_074->m_range_listener_0bc = this;
    m_text_bounds_0b8.left = origin_x + 0xc;
    m_text_bounds_0b8.top = origin_y + 0x29;
    m_text_bounds_0b8.right = origin_x + 0x84;
    m_text_bounds_0b8.bottom = origin_y + 0x33;
    m_text_buffer_0c8.SetLayoutBounds(&m_text_bounds_0b8, 1, 1);
    EnableRegionSet(1);
    m_key_handler_074->m_range_038.EnableRegionSet(1);
    SetEnabled(1);
    Invalidate(0);
    for (index = 0; index < 8; ++index) {
        if ((static_cast<unsigned char>(m_entries_054[index]->m_stateFlags) &
             g_W8TextControlMask005ED570) == 0) {
            m_entries_054[index]->SetEnabled(GetTable650434Entry(m_selection_078, index) != 0);
            m_entries_054[index]->Invalidate(0);
        }
    }
    for (index = 0; index < 8; ++index) {
        if (m_values_080 == 0 ||
            (m_values_080[index] == 0 && GetTable650434Entry(m_selection_078, index) == 0)) {
            m_entries_054[index]->m_image_b8 = -1;
        } else {
            m_entries_054[index]->m_image_b8 = m_values_080[index];
        }
        m_entries_054[index]->Invalidate(0);
    }
}

// SYNTHETIC: WIZ8 0x00588770
// W8MainGameTextPanel::`scalar deleting destructor'

// FUNCTION: WIZ8 0x00588790
W8MainGameTextPanel::~W8MainGameTextPanel()
{
    EnableRegionSet(0);
    DestroyAllControls();
}

// FUNCTION: WIZ8 0x00588830
void W8MainGameTextPanel::Redraw()
{
    SGPRect previous;
    SGPRect clip;
    int progress;

    if (m_fDirty) {
        m_flag_141 = 1;
    }
    Controls::Redraw();
    m_key_handler_074->m_range_038.Redraw();
    if (!m_fEnabled) {
        return;
    }
    if (m_flag_084) {
        progress = (int)(m_field_08c * g_float_005eebbc);
        if (progress > m_field_090) {
            m_field_090 = progress;
            InvalidateRegion(m_text_bounds_0b8.left, m_text_bounds_0b8.top, m_text_bounds_0b8.right,
                             m_text_bounds_0b8.bottom, 0);
            GetClippingRect(&previous);
            clip = previous;
            clip.iBottom = m_text_bounds_0b8.left + m_field_090;
            SetClippingRect(&clip);
            DrawCatalogImage(-14, 0x1b5, 0, 0, m_text_bounds_0b8.left, m_text_bounds_0b8.top, 2, 0);
            SetClippingRect(&previous);
            m_text_buffer_0c8.RenderToTarget(0, 1, -14);
        }
    }
    if (m_fEnabled && m_target_changed_140 && m_flag_141) {
        int frame = m_field_13c % 12;
        if (frame >= 7) {
            frame = 12 - frame;
        }
        DrawCatalogImageAndInvalidate(-14, 0x1b4, 0, frame, origin_x + 1, origin_y + 2, 2, 0);
        m_flag_141 = 0;
    }
}

// FUNCTION: WIZ8 0x005889a0
void W8MainGameTextPanel::OnPrimary(W8TextControl* control)
{
    int index;

    for (index = 0; index < 8; ++index) {
        if (control == m_entries_054[index]) {
            break;
        }
    }
    m_entries_054[index]->m_input_blocked_bc = 1;
    if (m_screen_07c != 0) {
        m_screen_07c->SelectTextEntry(index);
    }
}

// FUNCTION: WIZ8 0x005889e0
void W8MainGameTextPanel::OnRangeChanged(W8RangeControl* control)
{
    int column;
    int* values;
    int selection = m_key_handler_074->m_field_0b0;

    (void)control;
    m_selection_078 = selection;
    for (column = 0; column < 8; ++column) {
        if ((static_cast<unsigned char>(m_entries_054[column]->m_stateFlags) &
             g_W8TextControlMask005ED570) == 0) {
            m_entries_054[column]->SetEnabled(GetTable650434Entry(selection, column) != 0);
            m_entries_054[column]->Invalidate(0);
        }
    }
    values = m_values_080;
    for (column = 0; column < 8; ++column) {
        if (values == 0 || (values[column] == 0 && GetTable650434Entry(selection, column) == 0)) {
            m_entries_054[column]->m_image_b8 = -1;
        } else {
            m_entries_054[column]->m_image_b8 = values[column];
        }
        m_entries_054[column]->Invalidate(0);
    }
}

// FUNCTION: WIZ8 0x00588a90
W8MainGameStatusPanel005EEBC0::W8MainGameStatusPanel005EEBC0()
    : Controls(0x17, 0x166, 0, 0, 0x1b1, 0, 0)
{
    W8ControlsRect bounds;

    bounds.left = origin_x + 0x25;
    bounds.top = origin_y + 7;
    bounds.right = origin_x + 0xb4;
    bounds.bottom = origin_y + 0x11;
    m_text_04c = new W8TextBuffer(&bounds, 0, g_font_683660, g_W8TextBufferLayoutMask005ED54C, 4);
    bounds.top += 0xe;
    bounds.bottom += 0xe;
    m_text_050 = new W8TextBuffer(&bounds, gppStringList[0x1ea4 / 4], g_font_683660,
                                  g_W8TextBufferLayoutMask005ED548, 4);
    bounds.top += 0xe;
    bounds.bottom += 0xe;
    m_text_058 = new W8TextBuffer(&bounds, gppStringList[0x1ea8 / 4], g_font_683660,
                                  g_W8TextBufferLayoutMask005ED548, 4);
    bounds.top += 0xe;
    bounds.bottom += 0xe;
    m_text_060 = new W8TextBuffer(&bounds, gppStringList[0x1ec0 / 4], g_font_683660,
                                  g_W8TextBufferLayoutMask005ED548, 4);
    bounds.left = origin_x + 0x98;
    bounds.top = origin_y + 0x15;
    bounds.right = origin_x + 0xb5;
    bounds.bottom = origin_y + 0x1f;
    m_text_054 = new W8TextBuffer(&bounds, 0, g_font_683660, g_W8TextBufferLayoutMask005ED54C, 4);
    bounds.top += 0xe;
    bounds.bottom += 0xe;
    m_text_05c = new W8TextBuffer(&bounds, 0, g_font_683660, g_W8TextBufferLayoutMask005ED54C, 4);
    bounds.top += 0xe;
    bounds.bottom += 0xe;
    m_text_064 = new W8TextBuffer(&bounds, 0, g_font_683660, g_W8TextBufferLayoutMask005ED54C, 4);
    m_target_068 = 0;
    SetEnabled(1);
    Invalidate(0);
}

// SYNTHETIC: WIZ8 0x00588d90
// W8MainGameStatusPanel005EEBC0::`scalar deleting destructor'

// FUNCTION: WIZ8 0x00588db0
W8MainGameStatusPanel005EEBC0::~W8MainGameStatusPanel005EEBC0()
{
    delete m_text_04c;
    delete m_text_050;
    delete m_text_054;
    delete m_text_058;
    delete m_text_05c;
    delete m_text_060;
    delete m_text_064;
}

// FUNCTION: WIZ8 0x00588e60
void W8MainGameStatusPanel005EEBC0::RefreshStatusTexts()
{
    W8Character* character =
        &g_status_685170.buffers.characters[g_status_685170.selected_character];
    int level = GetPartySlotSkill10Level(g_status_685170.selected_character);
    unsigned int book;
    unsigned int realm;
    unsigned int figure;

    m_text_04c->SetText(character->name, g_font_683660);
    if (level >= 0) {
        level += m_target_068 * 6;
    }
    if (level < 0) {
        m_text_054->SetFontStateIndex(0);
        m_text_054->SetText(g_dash_0064789c, g_font_683660);
    } else {
        m_text_054->SetFontStateIndex(m_target_068 < 1 ? -1 : 3);
        m_text_054->SetText(FormatWideString(g_format_d_percent_0064bab0, level), g_font_683660);
    }
    if (!IsPartySlotEligible00524A10(g_status_685170.selected_character) ||
        character->spell_learned[0x27] != 1) {
        m_text_05c->SetFontStateIndex(0);
        m_text_05c->SetText(g_dash_0064789c, g_font_683660);
    } else {
        book = GetBestSpellbookSkillForSpell(character, 0x27, 1, 0, 7, 0);
        realm = character->skills[0x1c + g_spell_records[0x27].realm].level;
        book = character->skills[book].level;
        m_text_05c->SetFontStateIndex(-1);
        m_text_05c->SetText(FormatWideString(g_format_d_percent_0064bab0, (book + realm * 4) / 5),
                            g_font_683660);
    }
    if (IsPartySlotEligible00524A10(g_status_685170.selected_character) &&
        character->spell_learned[0x12] == 1) {
        figure = GetBestSpellbookSkillForSpell(character, 0x12, 1, 0, 7, 0);
        figure = (character->skills[figure].level +
                  character->skills[0x1c + g_spell_records[0x12].realm].level * 4) /
                 5;
        if ((int)figure >= 0) {
            m_text_064->SetFontStateIndex(-1);
            m_text_064->SetText(FormatWideString(g_format_d_percent_0064bab0, figure),
                                g_font_683660);
            m_text_050->SetGeometryDirty();
            m_text_058->SetGeometryDirty();
            m_text_060->SetGeometryDirty();
            Invalidate(0);
            return;
        }
    }
    m_text_064->SetFontStateIndex(0);
    m_text_064->SetText(g_dash_0064789c, g_font_683660);
    m_text_050->SetGeometryDirty();
    m_text_058->SetGeometryDirty();
    m_text_060->SetGeometryDirty();
    Invalidate(0);
}

// FUNCTION: WIZ8 0x00589090
int GetPartySlotSkill10Level(int slot)
{
    W8Character* character;

    if (!IsPartySlotEligible00524A10(slot)) {
        return -1;
    }
    character = &g_status_685170.buffers.characters[slot];
    if (character->skills[10].flag_00 == 0 && character->skills[10].level == 0) {
        return -1;
    }
    return (int)character->skills[10].level;
}

// FUNCTION: WIZ8 0x005890e0
void W8MainGameStatusPanel005EEBC0::Redraw()
{
    if (m_fEnabled && m_fDirty) {
        Controls::Redraw();
        m_text_04c->RenderToTarget(0, 0, -14);
        m_text_050->RenderToTarget(0, 0, -14);
        m_text_054->RenderToTarget(0, 0, -14);
        m_text_058->RenderToTarget(0, 0, -14);
        m_text_05c->RenderToTarget(0, 0, -14);
        m_text_060->RenderToTarget(0, 0, -14);
        m_text_064->RenderToTarget(0, 0, -14);
    }
}

// FUNCTION: WIZ8 0x00589160
W8MainGameScreen::W8MainGameScreen(Trigger* owner)
    : m_owner_008(owner), m_state_018(0), m_target_14c(0), m_field_150(0)
{
    m_field_034 = owner->value_37c;
    m_field_038 = owner->value_36c;
    m_text_panel_00c = new W8MainGameTextPanel();
    m_text_panel_00c->m_screen_07c = this;
    m_status_panel_010 = new W8MainGameStatusPanel005EEBC0();
    m_action_panel_014 = new Controls(0x1e7, 0x166, 0, 0, 0x1b1, 0, 2);
    m_action_panel_014->AcquireRegionSet(&g_main_game_action_panel_region_set_0068f2d0);
    m_action_controls_020[1] =
        new W8TextControl(m_action_panel_014, 0xffffffff, 6, 6, 0, 0, 0x1b0, 0, 8, 10, 9, 10, 0xb);
    m_action_controls_020[1]->m_listener = this;
    m_action_controls_020[2] =
        new W8TextControl(m_action_panel_014, 0xffffffff, 0x24, 6, 0, 0, 0x1b0, 0, 0, 2, 1, 2, 3);
    m_action_controls_020[2]->m_listener = this;
    m_action_controls_020[3] =
        new W8TextControl(m_action_panel_014, 0xffffffff, 6, 0x24, 0, 0, 0x1b0, 0, 4, 6, 5, 6, 7);
    m_action_controls_020[3]->m_listener = this;
    m_action_controls_020[4] = new W8TextControl(m_action_panel_014, 0xffffffff, 0x24, 0x24, 0, 0,
                                                 0x1b0, 0, 0xc, 0xe, 0xd, 0xe, 0xf);
    m_action_controls_020[4]->m_listener = this;
    m_action_controls_020[0] =
        new W8TextControl(m_action_panel_014, 0xffffffff, 0x44, 6, 0, 0, 0x8e, 0, 4, 6, 5, 6, 7);
    m_action_controls_020[0]->m_listener = this;
    m_action_controls_020[1]->EnableRegionHelp(0x7ce);
    m_action_controls_020[2]->EnableRegionHelp(0x7cf);
    m_action_controls_020[3]->EnableRegionHelp(0x7cc);
    m_action_controls_020[4]->EnableRegionHelp(0x7d0);
    m_action_panel_014->EnableRegionSet(1);
    m_action_panel_014->SetEnabled(1);
    m_action_panel_014->Invalidate(0);
    for (int i = 0; i < 8; ++i) {
        m_unknown_03c[i] = 0;
        m_slot_flag_044[i] = 0;
    }
    RefreshActionPanel();
}

// SYNTHETIC: WIZ8 0x0058a840
// W8MainGameScreen::`scalar deleting destructor'

// FUNCTION: WIZ8 0x005894b0
W8MainGameScreen::~W8MainGameScreen()
{
    delete m_text_panel_00c;
    delete m_status_panel_010;
    m_action_panel_014->EnableRegionSet(0);
    m_action_panel_014->DestroyAllControls();
    delete m_action_panel_014;
}

// FUNCTION: WIZ8 0x00589550
void W8MainGameScreen::SelectTextEntry(int index)
{
    int slot = g_status_685170.selected_character;
    int skill;
    int chance;
    int roll;
    float hold;
    float duration;
    int i;

    m_selected_character_01c = slot;
    skill = GetPartySlotSkill10Level(slot);
    hold = g_navigator_linked_radius_scale_005ebc98 - (float)skill * g_float_005ec258;
    chance = m_field_038;
    if (chance < 0) {
        chance = 0;
    }
    skill = GetPartySlotSkill10Level(m_selected_character_01c);
    if (skill > -1) {
        skill += ((m_target_14c + 1) / 2) * 6;
    }
    skill -= g_table_6504e8[chance];
    chance = (skill * 3) / 2 + (11 - g_settings_6850c8.difficulty) * 10;
    if (chance < 0) {
        chance = 0;
    } else if (chance > 0x63) {
        chance = 0x63;
    }
    if (GetTable650434Entry(m_field_034, index) == 0) {
        m_state_018 = 4;
        duration = (float)(Random(0x18) + 0x32) * g_movement_speed_step_005ed490;
    } else {
        roll = (int)Random(0x64);
        if (roll < (chance * chance) / 100) {
            m_unknown_03c[index] = 1;
            m_state_018 = 2;
            duration = 1.0f;
        } else {
            m_state_018 = 4;
            duration = (float)(Random(0x31) + 0x32) * g_movement_speed_step_005ed490;
        }
    }
    m_action_controls_020[1]->SetEnabled(0);
    m_action_controls_020[2]->SetEnabled(0);
    m_action_controls_020[3]->SetEnabled(0);
    m_action_controls_020[4]->SetEnabled(0);
    m_action_panel_014->Invalidate(0);
    m_text_panel_00c->m_flag_084 = 1;
    m_text_panel_00c->m_text_buffer_0c8.SetText(gppStringList[0x1ed4 / 4], g_font_683660);
    m_text_panel_00c->m_field_088 = duration;
    m_text_panel_00c->m_field_08c = 0.0f;
    m_text_panel_00c->m_field_090 = 0;
    m_text_panel_00c->m_timer_094.SetDuration(hold);
    m_text_panel_00c->m_timer_094.Restart();
    for (i = 0; i < 8; ++i) {
        if ((static_cast<unsigned char>(m_text_panel_00c->m_entries_054[i]->m_stateFlags) &
             g_W8TextControlMask005ED570) == 0) {
            m_text_panel_00c->m_entries_054[i]->m_input_blocked_bc = 1;
        }
    }
    m_field_150 = (int)SoundPlay((STR)g_trap_sounds_0064bbac[index], 0);
}

// FUNCTION: WIZ8 0x005897f0
void W8MainGameScreen::OnPrimary(W8TextControl* control)
{
    int slot;
    int skill;
    int chance;
    int roll;
    float hold;
    float duration;
    int i;

    if (control == m_action_controls_020[0]) {
        m_state_018 = 0xa;
        return;
    }
    if (control != m_action_controls_020[4]) {
        if (control == m_action_controls_020[2]) {
            m_state_018 = 4;
            return;
        }
        if (control == m_action_controls_020[1]) {
            m_state_018 = 5;
            return;
        }
        if (control == m_action_controls_020[3]) {
            m_state_018 = 6;
        }
        return;
    }
    slot = g_status_685170.selected_character;
    m_selected_character_01c = slot;
    skill = GetPartySlotSkill10Level(slot);
    hold = g_float_005ebca0 - (float)skill * g_camera_snap_epsilon_005ebc2c;
    chance = m_field_038;
    if (chance < 0) {
        chance = 0;
    }
    skill = GetPartySlotSkill10Level(m_selected_character_01c);
    if (skill > -1) {
        skill += m_target_14c * 6;
    }
    skill -= g_table_6504e8[chance];
    chance = (skill * 3) / 2 + (11 - g_settings_6850c8.difficulty) * 10;
    if (chance < 0) {
        chance = 0;
    } else if (chance > 0x63) {
        chance = 0x63;
    }
    roll = (int)Random(0x64);
    if (roll < chance) {
        m_state_018 = 1;
        duration = 1.0f;
    } else {
        m_state_018 = 4;
        duration = (float)(Random(0x18) + 0x4b) * g_movement_speed_step_005ed490;
    }
    m_action_controls_020[1]->SetEnabled(0);
    m_action_controls_020[2]->SetEnabled(0);
    m_action_controls_020[3]->SetEnabled(0);
    m_action_controls_020[4]->SetEnabled(0);
    m_action_panel_014->Invalidate(0);
    m_text_panel_00c->m_flag_084 = 1;
    m_text_panel_00c->m_text_buffer_0c8.SetText(gppStringList[0x1ed8 / 4], g_font_683660);
    m_text_panel_00c->m_field_088 = duration;
    m_text_panel_00c->m_field_08c = 0.0f;
    m_text_panel_00c->m_field_090 = 0;
    m_text_panel_00c->m_timer_094.SetDuration(hold);
    m_text_panel_00c->m_timer_094.Restart();
    for (i = 0; i < 8; ++i) {
        if ((static_cast<unsigned char>(m_text_panel_00c->m_entries_054[i]->m_stateFlags) &
             g_W8TextControlMask005ED570) == 0) {
            m_text_panel_00c->m_entries_054[i]->m_input_blocked_bc = 1;
        }
    }
    m_field_150 = (int)SoundPlayStreamedFile((STR)g_trap_inspection_sound_0064bcac, 0);
}

// FUNCTION: WIZ8 0x00589a80
void W8MainGameScreen::Update()
{
    W8MainGameTextPanel* panel;
    int column;
    int elapsed;
    float progress;

    if (m_state_018 == 9 && m_timer_154.GetProgress() >= g_float_005ebb38 &&
        PartyPortraitEventsIdle() != 0) {
        m_owner_008->Run(-1);
        m_state_018 = 10;
    }
    if (m_state_018 == 10) {
        gXStatus.fTrapInteractMode = 0;
        if (g_main_game_screen != 0) {
            delete g_main_game_screen;
        }
        g_main_game_screen = 0;
        ClearLevelDataFlag6();
        Function58F6B0(0);
        ApplyMainGameModeFlag(g_value_68f2c4, 1);
        RequestRedraw(0x200);
        RequestRedraw(0x100);
        RequestRedraw(0x1000);
        return;
    }

    panel = m_text_panel_00c;
    if (panel->m_target_changed_140 != 0) {
        elapsed = (int)panel->m_timer_118.GetProgress();
        if (elapsed != 0) {
            panel->m_field_13c += elapsed;
            panel->m_flag_141 = 1;
        }
    }

    panel = m_text_panel_00c;
    if (panel->m_flag_084 != 0) {
        progress = panel->m_timer_094.GetProgress();
        if (progress < panel->m_field_088) {
            panel->m_field_08c = progress;
            return;
        }
        panel->m_flag_084 = 0;
        panel->Invalidate(0);
        for (column = 0; column < 8; ++column) {
            if ((static_cast<unsigned char>(panel->m_entries_054[column]->m_stateFlags) &
                 g_W8TextControlMask005ED570) == 0) {
                panel->m_entries_054[column]->m_input_blocked_bc = 0;
            }
        }
    }

    switch (m_state_018) {
    case 1:
        m_state_018 = 0;
        ApplyInspectSuccess();
        return;
    case 2:
        m_state_018 = 0;
        m_field_150 = 0;
        PracticeCharacterSkill(&g_status_685170.buffers.characters[m_selected_character_01c], 10, 1,
                               0);
        RefreshActionPanel();
        for (column = 0; column < 8; ++column) {
            if (GetTable650434Entry(m_field_034, column) != 0 && m_unknown_03c[column] == 0) {
                return;
            }
        }
        m_state_018 = 3;
        return;
    case 3:
        panel = m_text_panel_00c;
        panel->EnableRegionSet(0);
        panel->m_key_handler_074->m_range_038.EnableRegionSet(0);
        m_action_panel_014->EnableRegionSet(0);
        Function5E3780(m_owner_008);
        gXStatus.fTrapInteractMode = 0;
        if (g_main_game_screen != 0) {
            delete g_main_game_screen;
        }
        g_main_game_screen = 0;
        ClearLevelDataFlag6();
        Function58F6B0(0);
        ApplyMainGameModeFlag(g_value_68f2c4, 1);
        RequestRedraw(0x200);
        RequestRedraw(0x100);
        RequestRedraw(0x1000);
        return;
    case 4:
        if (m_field_150 != 0) {
            SoundStop(m_field_150);
            m_field_150 = 0;
        }
        SoundPlay((STR)g_trap_sprung_sound_0064bcd0, 0);
        EnablePanelRegionSets(0);
        Function5E3AB0(m_owner_008);
        m_state_018 = 9;
        m_timer_154.SetDuration(2.0f);
        m_timer_154.Restart();
        return;
    case 5:
        m_state_018 = 0;
        CastTrapSpell();
        return;
    case 6:
        m_state_018 = 0;
        UseTrapItem();
        return;
    case 7:
    case 8:
        if (m_timer_154.GetProgress() >= g_float_005ebb38) {
            m_state_018 = (m_state_018 != 7) + 3;
        }
        break;
    }
}

// FUNCTION: WIZ8 0x00589d90
void W8MainGameScreen::RefreshActionPanel()
{
    int slot = g_status_685170.selected_character;
    int skill = GetPartySlotSkill10Level(slot);
    W8Character* character = &g_status_685170.buffers.characters[slot];
    int can_cast;
    int column;
    int* values;
    int i;

    m_action_controls_020[4]->SetEnabled(skill >= 0);
    for (i = 0; i < 8; ++i) {
        if ((static_cast<unsigned char>(m_text_panel_00c->m_entries_054[i]->m_stateFlags) &
             g_W8TextControlMask005ED570) == 0) {
            m_text_panel_00c->m_entries_054[i]->m_input_blocked_bc = skill < 0;
        }
    }
    if ((!IsPartySlotEligible00524A10(slot) || character->spell_learned[0x27] != 1) &&
        (!IsPartySlotEligible00524A10(slot) || character->spell_learned[0x12] != 1)) {
        can_cast = 0;
    } else {
        if (IsPartySlotEligible00524A10(slot) && character->spell_learned[0x27] == 1) {
            GetBestSpellbookSkillForSpell(character, 0x27, 1, 0, 7, 0);
        } else {
            GetBestSpellbookSkillForSpell(character, 0x12, 1, 0, 7, 0);
        }
        can_cast = CanCharacterCastSpell(character, 0x27) != 0 ||
                   CanCharacterCastSpell(character, 0x12) != 0;
    }
    m_action_controls_020[1]->SetEnabled(can_cast != 0);
    if (m_slot_flag_044[slot] == 0) {
        m_text_panel_00c->m_values_080 = 0;
        for (column = 0; column < 8; ++column) {
            m_text_panel_00c->m_entries_054[column]->m_image_b8 = -1;
            m_text_panel_00c->m_entries_054[column]->Invalidate(0);
        }
    } else {
        values = m_slot_values_04c[slot];
        m_text_panel_00c->m_values_080 = values;
        for (column = 0; column < 8; ++column) {
            if (values == 0 ||
                (values[column] == 0 &&
                 GetTable650434Entry(m_text_panel_00c->m_selection_078, column) == 0)) {
                m_text_panel_00c->m_entries_054[column]->m_image_b8 = -1;
            } else {
                m_text_panel_00c->m_entries_054[column]->m_image_b8 = values[column];
            }
            m_text_panel_00c->m_entries_054[column]->Invalidate(0);
        }
    }
    m_action_controls_020[2]->SetEnabled(1);
    m_action_controls_020[3]->SetEnabled(1);
    m_status_panel_010->RefreshStatusTexts();
    m_action_panel_014->Invalidate(0);
}

// FUNCTION: WIZ8 0x0058a030
void W8MainGameScreen::EnablePanelRegionSets(bool enable)
{
    W8MainGameTextPanel* panel = m_text_panel_00c;

    panel->EnableRegionSet(enable);
    panel->m_key_handler_074->m_range_038.EnableRegionSet(enable);
    m_action_panel_014->EnableRegionSet(enable);
}

// FUNCTION: WIZ8 0x0058a060
void W8MainGameScreen::ApplyInspectSuccess()
{
    int chance;
    int skill;
    int column;
    int* values;
    int roll;

    m_field_150 = 0;
    PracticeCharacterSkill(&g_status_685170.buffers.characters[m_selected_character_01c], 10, 1, 0);
    RefreshActionPanel();
    chance = m_field_038;
    if (chance < 0) {
        chance = 0;
    }
    skill = GetPartySlotSkill10Level(m_selected_character_01c);
    if (skill > -1) {
        skill += m_target_14c * 6;
    }
    skill -= g_table_6504e8[chance];
    chance = (skill * 3) / 2 + (11 - g_settings_6850c8.difficulty) * 10;
    if (chance < 0) {
        chance = 0;
    } else if (chance > 0x63) {
        chance = 0x63;
    }
    for (column = 0; column < 8; ++column) {
        roll = (int)Random(0x64);
        if (roll < chance) {
            m_slot_values_04c[m_selected_character_01c][column] =
                GetTable650434Entry(m_field_034, column);
        } else {
            m_slot_values_04c[m_selected_character_01c][column] = 2;
        }
    }
    m_slot_flag_044[m_selected_character_01c] = 1;
    values = m_slot_values_04c[m_selected_character_01c];
    m_text_panel_00c->m_values_080 = values;
    for (column = 0; column < 8; ++column) {
        if (values == 0 || (values[column] == 0 &&
                            GetTable650434Entry(m_text_panel_00c->m_selection_078, column) == 0)) {
            m_text_panel_00c->m_entries_054[column]->m_image_b8 = -1;
        } else {
            m_text_panel_00c->m_entries_054[column]->m_image_b8 = values[column];
        }
        m_text_panel_00c->m_entries_054[column]->Invalidate(0);
    }
}

// FUNCTION: WIZ8 0x0058a200
void W8MainGameScreen::CastTrapSpell()
{
    int slot = g_status_685170.selected_character;
    W8Character* character = &g_status_685170.buffers.characters[slot];
    unsigned int book;
    unsigned int figure;
    int spell;
    W8MainGameScreen* screen;
    W8MainGameTextPanel* panel;
    unsigned char ready = 0;

    if (IsPartySlotEligible00524A10(slot) && character->spell_learned[0x27] == 1) {
        book = GetBestSpellbookSkillForSpell(character, 0x27, 1, 0, 7, 0);
        figure = (character->skills[book].level +
                  character->skills[0x1c + g_spell_records[0x27].realm].level * 4) /
                 5;
        if ((int)figure >= 0) {
            ready = 1;
        }
    }
    if (ready == 0) {
        character = &g_status_685170.buffers.characters[slot];
        if (!IsPartySlotEligible00524A10(slot) || character->spell_learned[0x12] != 1) {
            return;
        }
        book = GetBestSpellbookSkillForSpell(character, 0x12, 1, 0, 7, 0);
        figure = (character->skills[book].level +
                  character->skills[0x1c + g_spell_records[0x12].realm].level * 4) /
                 5;
        if ((int)figure < 0) {
            return;
        }
    }
    if (CanCharacterCastSpell(character, 0x27) == 0 &&
        CanCharacterCastSpell(&g_status_685170.buffers.characters[slot], 0x12) == 0) {
        return;
    }
    spell =
        CanCharacterCastSpell(&g_status_685170.buffers.characters[slot], 0x12) != 0 ? 0x12 : 0x27;
    m_action_controls_020[1]->SetAlternateTextEnabled(0);
    screen = g_main_game_screen;
    gXStatus.fTrapInteractMode = 0;
    panel = screen->m_text_panel_00c;
    panel->EnableRegionSet(0);
    panel->m_key_handler_074->m_range_038.EnableRegionSet(0);
    screen->m_action_panel_014->EnableRegionSet(0);
    gXStatus.fTrapInteract = 1;
    Function58F6B0(0);
    ApplyMainGameModeFlag(g_value_68f2c4, 1);
    RequestRedraw(0x200);
    RequestRedraw(0x100);
    RequestRedraw(0x1000);
    Function5A0110(spell, -1, -1);
}

// FUNCTION: WIZ8 0x0058a3e0
void W8MainGameScreen::UseTrapItem()
{
    W8MainGameScreen* screen;
    W8MainGameTextPanel* panel;

    m_action_controls_020[3]->SetAlternateTextEnabled(0);
    screen = g_main_game_screen;
    gXStatus.fTrapInteractMode = 0;
    panel = screen->m_text_panel_00c;
    panel->EnableRegionSet(0);
    panel->m_key_handler_074->m_range_038.EnableRegionSet(0);
    screen->m_action_panel_014->EnableRegionSet(0);
    gXStatus.fTrapInteract = 1;
    Function58F6B0(0);
    ApplyMainGameModeFlag(g_value_68f2c4, 1);
    RequestRedraw(0x200);
    RequestRedraw(0x100);
    RequestRedraw(0x1000);
    Function59C930(g_status_685170.selected_character);
}

// FUNCTION: WIZ8 0x0058A750
void UpdateMainGameScreen(void)
{
    g_main_game_screen->Update();
}

// FUNCTION: WIZ8 0x00577850
unsigned char Function577850(void)
{
    return gXStatus.fNpcDialogueMode != 0 && g_screen_state_00649f1c->flag_252 != 0;
}

// FUNCTION: WIZ8 0x005929d0
void HandleManualCameraHotkeys(void)
{
    if (g_modal_owner_0068edd0 == 0 && gXStatus.fNpcDialogueMode == 0) {
        if (g_mgs_keyboard->IsCommandPressed(0x25a)) {
            BeginManualCameraControl();
        }
        ApplyWorldRenderHotkeys();
    }
}

// FUNCTION: WIZ8 0x00592a10
void ApplyWorldRenderHotkeys(void)
{
    if (g_mgs_keyboard->IsCommandPressed(0xcc) || g_mgs_keyboard->IsCommandPressed(0xcd)) {
        g_level_block->world_render_flags |= 0x100;
    }
    if (g_mgs_keyboard->IsCommandPressed(0xce) || g_mgs_keyboard->IsCommandPressed(0xcf)) {
        g_level_block->world_render_flags |= 0x200;
    }
    if (g_mgs_keyboard->IsCommandPressed(0xc8)) {
        g_level_block->world_render_flags |= 4;
    }
    if (g_mgs_keyboard->IsCommandPressed(0xc9)) {
        g_level_block->world_render_flags |= 0x84;
    }
    if (g_mgs_keyboard->IsCommandPressed(0xca)) {
        g_level_block->world_render_flags |= 8;
    }
    if (g_mgs_keyboard->IsCommandPressed(0xcb)) {
        g_level_block->world_render_flags |= 0x88;
    }
    if (g_mgs_keyboard->IsCommandPressed(0xd4)) {
        g_level_block->world_render_flags |= 0x400;
    }
    if (g_mgs_keyboard->IsCommandPressed(0xd5)) {
        g_level_block->world_render_flags |= 0x800;
    }
    if (g_mgs_keyboard->IsCommandPressed(0xd0)) {
        g_level_block->world_render_flags |= 1;
    }
    if (g_mgs_keyboard->IsCommandPressed(0xd2)) {
        g_level_block->world_render_flags |= 2;
    }
    if (g_mgs_keyboard->IsCommandPressed(0xd1)) {
        g_level_block->world_render_flags |= 0x81;
    }
    if (g_mgs_keyboard->IsCommandPressed(0xd3)) {
        g_level_block->world_render_flags |= 0x82;
    }
}

// FUNCTION: WIZ8 0x00593330
void Function593330(void)
{
    if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0 &&
        g_level_block->flag_314 != 0) {
        Function593360();
    }
}

// FUNCTION: WIZ8 0x00577540
void ClearMainGameTargetState(void)
{
    g_status_685170.value_2435 = 0;
    ClearLevelDataFlag6();
    SetTargetCursor(W8_CURSOR_NONE);
}

// FUNCTION: WIZ8 0x00577220
void Function577220(void)
{
    Function56C6D0(g_screen_state_00649f1c->value_1d4, 0, -1, 0, 1);
    g_screen_state_00649f1c->value_238 = g_screen_state_00649f1c->value_104;
    g_screen_state_00649f1c->flag_234 = 1;
}

// FUNCTION: WIZ8 0x00577260
void Function577260(void)
{
    Function56C6D0(g_screen_state_00649f1c->value_1d4, 0, -1, 0, 1);
    g_screen_state_00649f1c->value_238 = g_screen_state_00649f1c->value_104;
}

struct W8StartupGridRow {
    int x1;
    int y1;
    int x2;
    int y2;
    int unknown_10;
    int unknown_14;
    int unknown_18;
};

W8StartupGridRow g_startup_grid_647da0[8];

// FUNCTION: WIZ8 0x0055f7b0
unsigned char MainGameScreenInitialize(void)
{
    unsigned int index;
    for (index = 0; index != 8; ++index) {
        W8StartupGridRow& row = g_startup_grid_647da0[index];
        row.x1 = (index & 1) << 9;
        row.y1 = (index >> 1) * 0x55 + 0x12;
        row.x2 = row.x1 + 0x7f;
        row.y2 = row.y1 + 0x55;
    }
    return 1;
}

/* Reset the complete Main Game state block and the UI/selection state that is
   coupled to it. The clear's 0xcc dwords independently prove the 0x330 extent
   used by the allocating enter handler. */
// FUNCTION: WIZ8 0x0055f800
void ResetMainGameScreenState(void)
{
    int unset;

    if (g_level_block) {
        memset(g_level_block, 0, sizeof(W8LevelRuntimeBlock));
        TurnPartyToImmediate(g_status_685170.party_facing, 0);
        g_level_block->flag_24d = 0;
        gXStatus.fSpellCastMode = 0;
        gXStatus.fNpcDialogueMode = 0;
        gXStatus.fItemSelectMode = 0;
        gXStatus.fLockInteractMode = 0;
        gXStatus.fTrapInteractMode = 0;
        gXStatus.fReviewCharacterMode = 0;
        gXStatus.fSurprisePossible = 0;
        g_flag_006840bc = 0;
        g_flag_006840bd = 0;
        unset = -1;
        g_value_006840be = static_cast<unsigned short>(unset);
        g_held_item_source_006840c0 = unset;
        g_held_item_origin_006840c4 = static_cast<unsigned char>(unset);
        g_held_item_slot_006840c5 = static_cast<unsigned short>(unset);
        g_gameplay_timer_685067->Restart();
        g_flag_00685070 = 1;
        g_flag_00685071 = 0;
        g_value_00685072 = 0;
        g_flag_00685076 = 0xff;
        g_value_00685077 = -1;
        ResetTargetingState();
    }
}

/* Copy the next '/'-terminated field of a keyword line into the caller's
   buffer, trimming the leading and trailing spaces and stopping at a newline
   or at the end of the line. A field reached at a slash position, or a line
   that ends before any field, answers null; otherwise the returned cursor
   sits past the terminating slash so the next call continues the line. */
// FUNCTION: WIZ8 0x0056be40
wchar_t* ParseKeywordToken(wchar_t* line, wchar_t* field)
{
    wchar_t* cursor = line;
    wchar_t* out = field;
    int length = 0;

    *out = 0;
    while (*cursor == L' ' && *cursor != 0) {
        ++cursor;
    }
    if (*cursor == L'/') {
        return 0;
    }
    while (*cursor != 0 && *cursor != L'\n' && *cursor != L'\r') {
        *out = *cursor;
        ++cursor;
        ++length;
        ++out;
        if (*cursor == L'/') {
            break;
        }
    }
    if (length == 0) {
        return 0;
    }
    while (field[length - 1] == L' ') {
        --length;
        if (length < 1) {
            return 0;
        }
    }
    field[length] = 0;
    if (*cursor == L'/') {
        ++cursor;
    }
    return cursor;
}

/* Load one keyword file into a file list: a fresh line list per line and a
   malloc'd wide copy of every '/'-separated field. The first line is read only
   to prime the end-of-file test, and parsing starts eleven wide characters
   into every line - retail's own offset, whose prefix meaning is not
   resolved. A file that cannot be opened answers zero; otherwise every line
   adds a list, an empty one included, and the loader answers one. */
// FUNCTION: WIZ8 0x0056bed0
unsigned char LoadKeywordFile(const char* path, W8GrowableVector<W8GrowableVector<wchar_t*>*>* file)
{
    wchar_t line[1000];
    wchar_t field[1000];
    W8GrowableVector<wchar_t*>* entry;
    wchar_t* cursor;
    wchar_t* word;
    FILE* stream;
    size_t length;

    stream = fopen(path, "rb");
    if (stream == 0) {
        return 0;
    }
    memset(line, 0, sizeof(line));
    fgetws(line, 1000, stream);
    while (!feof(stream)) {
        memset(line, 0, sizeof(line));
        fgetws(line, 1000, stream);
        entry = new W8GrowableVector<wchar_t*>;
        cursor = line + 11;
        while ((cursor = ParseKeywordToken(cursor, field)) != 0) {
            length = wcslen(field);
            word = static_cast<wchar_t*>(malloc(length * 2 + 2));
            wcscpy(word, field);
            entry->Add(word);
        }
        file->Add(entry);
    }
    fclose(stream);
    return 1;
}

/* Release every keyword file list: its words through free, each line list and
   each file list through its deleting destructor. The loaded flag is lowered
   either way and the outer count is cleared. */
// FUNCTION: WIZ8 0x0056c130
void ClearKeywordLists(void)
{
    W8GrowableVector<W8GrowableVector<wchar_t*>*>* file;
    W8GrowableVector<wchar_t*>* entry;
    int file_index;
    int entry_index;
    int word_index;

    for (file_index = 0; file_index < g_keyword_lists.count; ++file_index) {
        file = *g_keyword_lists.GetAt(file_index);
        for (entry_index = 0; entry_index < file->count; ++entry_index) {
            entry = *file->GetAt(entry_index);
            for (word_index = 0; word_index < entry->count; ++word_index) {
                free(*entry->GetAt(word_index));
            }
            entry->count = 0;
            delete entry;
        }
        delete file;
    }
    g_keyword_lists.count = 0;
    g_keyword_lists_loaded_68f0f8 = 0;
}

/* Replace the keyword tables: release the current pair, then load the English
   file into element zero and the translated file into element one. A failed
   first load leaves an empty table; a failed second load releases the first
   list again. Only a complete pair raises the loaded flag. */
// FUNCTION: WIZ8 0x0056c200
void ReloadKeywordLists(void)
{
    W8GrowableVector<W8GrowableVector<wchar_t*>*>* english;
    W8GrowableVector<W8GrowableVector<wchar_t*>*>* translated;

    ClearKeywordLists();
    english = new W8GrowableVector<W8GrowableVector<wchar_t*>*>;
    if (!LoadKeywordFile("Data\\Strings\\English_Keywords.txt", english)) {
        return;
    }
    g_keyword_lists.Add(english);
    translated = new W8GrowableVector<W8GrowableVector<wchar_t*>*>;
    if (!LoadKeywordFile("Data\\Strings\\translated_Keywords.txt", translated)) {
        ClearKeywordLists();
        return;
    }
    g_keyword_lists.Add(translated);
    g_keyword_lists_loaded_68f0f8 = 1;
}

/* Reset the screen state block: zero its 0x268 bytes, write its reset values,
   clear the keyword status byte, and reload the keyword lists. */
// FUNCTION: WIZ8 0x0056c520
void ResetMainScreenStateBlock(void)
{
    int unset = -1;

    memset(g_screen_state_00649f1c, 0, sizeof(W8MainScreenState));
    g_screen_state_00649f1c->flag_1d8 = unset;
    g_screen_state_00649f1c->flag_1ec = 0;
    g_screen_state_00649f1c->flag_234 = 0;
    g_screen_state_00649f1c->value_258 = unset;
    g_screen_state_00649f1c->flag_260 = 1;
    g_status_685170.selected_party_member_2434 = 0xff;
    g_flag_68f0f9 = 0;
    ReloadKeywordLists();
}

/* Enter the live game screen. The 0x330 allocation is the complete extent of
   the per-screen block; the previously modeled fields only reached its last
   observed access at 0x327. */
// FUNCTION: WIZ8 0x0055f8c0
unsigned char MainGameScreenEnter(void)
{
    int display_mode;
    int difficulty = g_status_685170.difficulty;

    if (!g_level_block) {
        g_level_block = static_cast<W8LevelRuntimeBlock*>(malloc(sizeof(W8LevelRuntimeBlock)));
        if (!g_level_block) {
            return 0;
        }
        ResetMainGameScreenState();
        Function568E10();
    }
    gXStatus.unknown_026[1] = 1;
    MSYS_Init();
    ResetRegions();
    Function598AB0();
    Function5AE9D0();
    Function59B940();
    Function59BDB0();
    InitializeMainGameLevelBlock();
    ScrollTextBoxToCursor();
    SetClippingRegionAndImageWidth(0x500, 0, 0, 0x280, 0x1e0);
    SetFontDestBuffer(-14, 0, 0, 0x280, 0x1e0, 0);
    g_flag_65970d = 1;
    g_monster_shadow_updates_enabled_0065970c = 1;
    ClearPrimarySurface();
    if (IsFogEnabled()) {
        EnableSky();
    } else {
        DisableSky();
    }
    if (TakePendingSaveFlag()) {
        ShowNotice(0xc, gppStringList[0x1e08 / 4], -1, -1, 0);
    }
    if (g_settings_6850c8.difficulty != difficulty) {
        g_settings_6850c8.difficulty = difficulty;
        switch (difficulty) {
        case 0:
            display_mode = 0x7f8;
            break;
        case 1:
            display_mode = 0x7f9;
            break;
        case 2:
            display_mode = 0x7fa;
            break;
        }
        WriteGameLog(0xc, gppStringList[0x1e30 / 4], gppStringList[display_mode]);
    }
    ResetTransientRenderScenes();
    MoveTimer(4);
    if (!g_flag_006840bc && !gXStatus.fCombatMode) {
        SetEnvironmentTimeEnabled00482990(1);
    }
    {
        W8GameTimer* timer = g_gameplay_timer_685067;
        if ((timer->m_flags & 8) != 0 || (g_shared_timer_paused && (timer->m_flags & 1) == 0) ||
            g_shared_timer_flag_d1) {
            timer->m_flags &= ~8;
            timer->m_start = timer->Method00439A60() - timer->m_start;
            timer->SetDuration(-1.0f);
        }
    }
    Function55D3C0();
    if (g_status_685170.item_in_hand_235b.item_id != -1) {
        SetItemCursor(0);
    } else {
        ClearHeldItemDisplay();
    }
    SetTargetingMode(0);
    SetPrimarySurfaceTextureHint2Enabled(1);
    if (gXStatus.fLockInteract) {
        Function587510(0);
    }
    if (gXStatus.fTrapInteract) {
        Function58A470(0);
    }
    if (g_flag_00685071) {
        if (IsPartySlotEligible00524A10(g_value_00685077)) {
            Function565740(g_value_00685077);
            Function59C930(g_value_00685077);
            SelectCurrentUseItemLine0059E0E0();
        } else {
            g_flag_00685071 = 0;
            g_value_00685072 = 0;
            g_flag_00685076 = 0xff;
            g_value_00685077 = -1;
        }
    }
    if (!gXStatus.fCombatMode) {
        Function42B770(1, 1);
    }
    return 1;
}

/* The active screen's frame, including modal input and pending transitions. */
// FUNCTION: WIZ8 0x0055fb30
void MainGameScreenFrame(void)
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-compare"
    /* Retail compiled this comparison with VC6's mixed-sign operands; the
   signedness is part of the recovered body and changing it would change
   the compare and branch. Suppress only this diagnostic here. */
    if (g_flag_689b32) {
        RequestExitScreen();
    }
    if (g_build_level_links_0065bd2c) {
        char path[512];
        W8LevelInfo info;
        strcpy(path, static_cast<const char*>(g_world->octree->m_owned_0c0));
        char* extension = strrchr(path, '.');
        if (extension) {
            *extension = '\0';
        }
        strcat(path, ".rlk");
        if (!FileExists(path)) {
            Function4314C0(1);
        }
        for (; g_next_link_level_0068ede8 < 47; ++g_next_link_level_0068ede8) {
            if (LevelBuildInfoByID(g_next_link_level_0068ede8, &info)) {
                if (g_next_link_level_0068ede8 < 47) {
                    int level = g_next_link_level_0068ede8++;
                    RequestLevelTransition005615F0(level, -1, 0);
                    goto update_screen;
                }
                break;
            }
        }
        g_build_level_links_0065bd2c = 0;
    }
update_screen:
    Function568C40();
    Function569CC0();
    if (IsMessageBoxActive() || g_modal_owner_0068edd0) {
        if (g_flag_0068edd8) {
            SetFlag603C60();
            g_flag_0068edd8 = 0;
            gfTrackMousePos = 0;
        }
        UpdateHeldItemCursor();
        if (!g_modal_owner_0068edd0) {
            ProcessMessageBoxInput();
        }
    }
    if (!g_level_block->flag_328) {
        if (g_level_block->flag_327) {
            Function5A6970();
            return;
        }
    } else if (Function5A6790()) {
        return;
    }
    if (!AnyCharacterActive() || g_party_moving_006850b5) {
        Function5A68C0();
    }
    if (g_value_006840be != 0xffff) {
        Function50B3B0(static_cast<short>(g_value_006840be));
        g_value_006840be = 0xffff;
    }
    if (gXStatus.unknown_026[0]) {
        gXStatus.unknown_026[0] = 0;
        Function577220();
    }
    if (!IsScreenBusy()) {
        Function5542E0();
    }
    if (!g_level_block->transition_active && !gXStatus.fCombatMode && gXStatus.fEncumbranceDirty) {
        RedistributePartyEncumbrance();
    }
    Function59A3A0();
    if (g_modal_owner_0068edd0) {
        if (!g_flag_006840bc) {
            g_flag_006840bc = 1;
            if (gXStatus.fPartyMovementUi) {
                DisableRegionSet1C();
            }
            if (!gXStatus.fCombatMode) {
                if (g_flag_006840bd) {
                    MoveTimer(1);
                    EnableRegionInput(0x137);
                    ActivateDialogRegion(0x137);
                }
                SetEnvironmentTimeEnabled00482990(0);
                MonsterForward453160();
                ResetLevelDataVectors0041F0D0();
            }
            if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block) {
                g_level_block->redraw_flags |= 0x8000;
            }
        }
        if (!ProcessDialogInput(g_modal_owner_0068edd0)) {
            ClearActiveRegionIfMatches(0x138);
            delete g_modal_owner_0068edd0;
            g_modal_owner_0068edd0 = 0;
            if (g_pending_main_game_dialog_0068edd4) {
                g_modal_owner_0068edd0 = g_pending_main_game_dialog_0068edd4;
                g_pending_main_game_dialog_0068edd4->m_dirty_flags |= 1;
                g_pending_main_game_dialog_0068edd4 = 0;
            }
            if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block) {
                g_level_block->redraw_flags |= 0x8000;
            }
            ResumeMainGameWorld();
        }
    }
    Function575C50();
    NoOp();
    Function577560();
    gXStatus.character_event_queue->ProcessDeferredCharacterEvents();
    UpdateCharacterEventState();
    Function59B1A0();
    if (gXStatus.fCombatMode) {
        Function59B4C0();
    }
    g_status_685170.value_2390 = 0;
    if (g_level_block->flag_314 || g_level_block->combat_slot != -1) {
        Function59B390();
    }
    Function502650();
    if (gXStatus.fCombatMode) {
        for (int slot = 0; slot < 8; ++slot) {
            if (!g_status_685170.buffers.party_rows[slot].occupied ||
                g_status_685170.buffers.characters[slot].highest_condition > 0x11 ||
                (g_level_block->flag_314 && g_level_block->combat_slot == slot)) {
                DisableRegionInput(slot + 10);
            } else {
                EnableRegionInput(slot + 10);
            }
        }
    }
    if (IsScreenTransitionPending()) {
        g_level_block->transition_pending = 1;
        Function562A80();
        return;
    }
    Function515B00();
    UpdateSharedGameDataObject0041F1F0();
    g_byte_00659a64 = 0;
    WorldUpdateProps(GetWorld());
    if (GetWorld659AB8()) {
        WorldUpdateProps(GetWorld659AB8());
    }
    POINT point;
    POINT current;
    unsigned int value;
    SGPMouseGetPos(&point);
    if (!IsWorldCursorVisible()) {
        if (!g_modal_owner_0068edd0) {
            if ((!Function525DF0(1) || !gXStatus.fNpcDialogueMode) && !g_status_685170.value_2435) {
                g_level_block->hover_region = UpdateRegionMousePosition(point.x, point.y);
            } else {
                g_level_block->hover_region = FindRegionAtPoint(
                    static_cast<unsigned short>(point.x), static_cast<unsigned short>(point.y));
            }
        } else {
            g_level_block->hover_region = FindRegionAtPoint(static_cast<unsigned short>(point.x),
                                                            static_cast<unsigned short>(point.y));
            for (int portrait = 0; portrait < 8; ++portrait) {
                if (g_status_685170.buffers.party_rows[portrait].occupied &&
                    (g_level_block->hover_region == portrait * 6 + 0x24U ||
                     g_level_block->hover_region == portrait + 0x5aU)) {
                    g_level_block->hover_region = UpdateRegionMousePosition(point.x, point.y);
                    break;
                }
            }
        }
    } else {
        Function4916C0();
    }
    if (!g_level_block->flag_314 && g_level_block->hover_combat_slot != -1 &&
        g_level_block->hover_region != g_level_block->hover_combat_slot + 10U) {
        g_level_block->hover_combat_slot = -1;
    }
    Function561330(Function5684E0());
    if (!GetFlag69DA6C()) {
        if (!GetFlag68F105() || GetFlag68F104()) {
            HandleManualCameraHotkeys();
        } else if (CanUseCurrentAutomapTool()) {
            ApplyWorldRenderHotkeys();
        }
    }
    if (!g_level_runtime_flag_0065ba70) {
        if (g_flag_0068edd8) {
            if (g_flag_0068edd9) {
                if (!gfKeyState[0x10]) {
                    g_level_block->world_render_flags |= 4;
                } else {
                    g_level_block->world_render_flags |= 0x84;
                }
            }
            if (g_flag_0068edd8) {
                goto render_world;
            }
        }
        if (gfLeftButtonState && !g_modal_owner_0068edd0 && GetFlag68F105()) {
            SGPMouseGetPos(&current);
            Function57E0E0(0x400, &current);
        }
    }
render_world:
    if (!IsScreenTransitionPending()) {
        if (CanUseCurrentAutomapTool()) {
            Function44FC20(g_world, g_level_block->world_render_flags);
            if (g_world_659ab8 && !g_level_runtime_flag_0065ba70) {
                Function44FC20(g_world_659ab8, g_level_block->world_render_flags | 0x40);
            }
        }
        Function450210(g_world, g_level_block->world_update_flags);
        if (g_world_659ab8 && (g_level_block->world_update_flags & 3) == 0) {
            Function450210(g_world_659ab8, g_level_block->world_update_flags);
        }
        g_level_block->world_update_flags = 0;
        g_level_block->world_render_flags = 0;
        if (g_settings_6850c8.field_006) {
            Function59B2D0();
        }
        if (gXStatus.iCurrentCursor != -1 && gXStatus.iCurrentCursor != 7 &&
            g_main_game_resource_slots[gXStatus.iCurrentCursor].frame_count > 1 &&
            !ClockIsTicking(gXStatus.current_cursor_time) && !IsWorldCursorVisible() &&
            !g_flag_0068edd8) {
            ++gXStatus.current_cursor_frame;
            if (gXStatus.current_cursor_frame ==
                g_main_game_resource_slots[gXStatus.iCurrentCursor].frame_count) {
                gXStatus.current_cursor_frame = 0;
            }
            ApplyCurrentCursor();
        }
        ProcessMonsterManagerFrame();
        if (g_debug_monster_cycle_0068f0fc) {
            W8Monster* monster =
                GetMonsterByLocationID(IListGetAt(g_debug_monster_ids_0068f100, 0));
            if (monster) {
                ClearSurfaceRect(0x122, 0x159, 0x226, 0x168);
                SetFont(g_font_683660);
                unsigned char frame = monster->m_pRep->flag_064;
                const char* cycle = g_cycle_names[monster->Query(6)].name;
                unsigned char subcycles = static_cast<unsigned char>(monster->GetNumSubCycles());
                mprintf(0x122, 0x159, (UINT16*)L"%2d/%2d %hs", frame, subcycles, cycle);
            }
        }
        if (!gXStatus.fCombatMode) {
            Function5A1EB0(&current, &value);
            Function5171C0();
        } else if (!g_level_block->transition_active && !gXStatus.fSpellCastMode &&
                   !gXStatus.fNpcDialogueMode && !gXStatus.fItemSelectMode) {
            Function4E8EA0();
        }
        if (IsSightRangeOverridden() && !gXStatus.fCombatMode && AnyCharacterActive() &&
            gXStatus.field_02d) {
            StartCombat(0);
        }
        if (!ClockIsTicking(g_level_block->character_update_timer)) {
            Function530110();
            g_level_block->character_update_timer = SetCountdownClock(500);
        }
        if (!g_flag_006840bc) {
            Function530150(1);
            if (!gXStatus.fCombatMode && AnyCharacterActive() && gXStatus.field_02d &&
                !gXStatus.fNpcDialogueMode) {
                StartCombat(0);
            }
            if (g_navigator_position_changed_659c11) {
                g_navigator_position_changed_659c11 = 0;
                if (g_level_block) {
                    if (gXStatus.fCombatMode) {
                        g_level_block->refresh_combat_panel = 1;
                    }
                    g_level_block->refresh_party_panel = 1;
                }
                g_flag_006840bb = 1;
            }
        }
        if (gXStatus.fCombatMode && g_level_block->refresh_combat_panel &&
            !ClockIsTicking(g_level_block->combat_panel_timer)) {
            Function5398D0();
            g_level_block->combat_panel_timer = SetCountdownClock(500);
            g_level_block->refresh_combat_panel = 0;
        }
        if (gXStatus.iTargetingMode == 4) {
            RefreshSpellTargetHighlightsAtRange();
        } else if (gXStatus.iTargetingMode == 3 && IsWorldCursorVisible()) {
            Function53B1D0();
        } else if (gXStatus.iTargetingMode != 5 && g_level_block->refresh_party_panel) {
            UpdateAllMonsterHighlights(g_status_685170.selected_character,
                                       g_level_block->highlighted_item);
            g_level_block->refresh_party_panel = 0;
        }
        if (!ClockIsTicking(g_level_block->world_update_timer)) {
            Function4F7480();
            DetachAllWorldItems();
            g_level_block->world_update_timer = SetCountdownClock(50);
        }
        if (gXStatus.fSpellCastMode)
            Function5A0BC0();
        if (gXStatus.fItemSelectMode)
            Function59D180();
        if (gXStatus.fNpcDialogueMode)
            Function56E510();
        if (gXStatus.fLockInteractMode)
            Function587960();
        if (gXStatus.fTrapInteractMode)
            UpdateMainGameScreen();
        int active;
        if (!Function445140(g_world) && !Function53A1D0() && !Function4F8650() &&
            !Function57E3C0() && !SelectWorldCursorNode0048EFC0()) {
            active = 0;
        } else {
            active = 1;
        }
        SetWorldModelPickingEnabled(active);
        if (gXStatus.unknown_026[1] && !gXStatus.fNpcDialogueMode && !gXStatus.fCombatMode &&
            !g_level_block->transition_active) {
            Function4EF1F0();
        }
        if (g_status_685170.selected_character == -1) {
            int next = GetNextCharacter(1, 1, -1);
            if (next == -1) {
                srAssertFail("iNextChar != BAD_INDEX",
                             "C:\\Projects\\Wizardry 8\\Local Screens\\MainGameScreen.cpp", 0x1047,
                             0);
            }
            Function565740(next);
        }
    }
    Function562A80();
#pragma clang diagnostic pop
}

/* Suspension retains the allocation and resource strip. A full leave also
   unloads the level and releases the per-screen block. */
// FUNCTION: WIZ8 0x00560660
unsigned char MainGameScreenLeave(int leaving)
{
    int index;

    if (g_main_game_mode_0068eddc == 3) {
        if (gXStatus.fNpcDialogueMode) {
            Function56E800(0);
        }
    } else if (g_main_game_mode_0068eddc == 5) {
        Function5187E0();
    } else if (g_main_game_mode_0068eddc == 6) {
        if (g_level_block->dialogue_owner != 0) {
            ReleaseObject004257F0(g_level_block->dialogue_owner);
            g_level_block->dialogue_owner = 0;
        }
        Function563DD0();
    }
    g_main_game_mode_0068eddc = 0;

    if (g_flag_0068edd8) {
        SetFlag603C60();
        g_flag_0068edd8 = 0;
        gfTrackMousePos = 0;
    }
    if (IsWorldCursorVisible()) {
        Function490AF0();
    }
    Function59B270();
    if (gXStatus.fLockInteractMode)
        Function5879A0(0);
    if (gXStatus.fTrapInteractMode)
        Function58A790(0);
    if (gXStatus.fSpellCastMode)
        Function59F2B0();
    if (gXStatus.fItemSelectMode)
        Function59C9C0();
    if (gXStatus.fReviewCharacterMode)
        CloseReviewCommonUi();
    if (gXStatus.fNpcDialogueMode)
        Function56E800(0);
    if (g_level_block->flag_314)
        Function592E60();
    ReleasePortraitControls();
    ReleaseConditionButtons();
    if (gXStatus.fPartyMovementUi)
        DisableRegionSet1C();
    Function529510();
    if (GetFlag68F105())
        Function57D740();
    MoveTimer(1);
    SetEnvironmentTimeEnabled00482990(0);

    if ((g_gameplay_timer_685067->m_flags & 8) == 0) {
        g_gameplay_timer_685067->m_flags |= 8;
        g_gameplay_timer_685067->m_start =
            g_gameplay_timer_685067->Method00439A60() - g_gameplay_timer_685067->m_start;
    }

    if (static_cast<unsigned char>(leaving)) {
        for (index = 0; index < 17; ++index) {
            if (g_main_game_resource_slots[index].object != 0) {
                g_main_game_resource_slots[index].object->release();
                g_main_game_resource_slots[index].object = 0;
            }
        }
    }

    UpdateHeldItemCursor();
    if (g_level_block->held_item_display_190 != -1) {
        g_level_block->held_item_display_190 = -1;
        UpdateHeldItemCursor();
    }

    if (g_level_block->flag_156) {
        g_level_block->flag_156 = 0;
        RegionSetDisable(0x13);
        ReleaseRuntimeDialogOwners();
        if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
            g_level_block->redraw_flags |= 0x8200;
        }
        if (g_flag_0068edc9) {
            unsigned short mode;
            if (!IsScreenInputBlocked() && !g_level_block->flag_155 && g_level_block->flag_156 &&
                g_level_block->flag_157 && g_settings_6850c8.field_006 == 0) {
                mode = 4;
            } else if (!IsScreenInputBlocked() &&
                       (!g_level_block->flag_156 || !g_level_block->flag_157 ||
                        !g_level_block->flag_155)) {
                mode = 0;
            } else if (g_settings_6850c8.field_006 == 1) {
                mode = 1;
            } else if (g_settings_6850c8.field_006 == 2) {
                mode = 0;
            } else {
                mode = 2;
            }
            SetViewportMode(mode);
        }
        g_flag_0068edc9 = 0;
    }

    if (g_level_block->flag_157) {
        g_level_block->flag_157 = 0;
        DisableRegionInput(0x62);
        RegionSetDisable(0x12);
        Function5A20E0(0);
        Function5A23E0();
        if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
            g_level_block->redraw_flags |= 0x8200;
        }
        if (g_flag_0068edbc) {
            SetViewportMode(Function5698C0());
        }
        g_flag_0068edbc = 0;
    }

    if (g_level_block->flag_155) {
        g_level_block->flag_155 = 0;
        DisableCombatRegions();
        if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
            g_level_block->redraw_flags |= 0x8200;
        }
        if (g_flag_0068edc8) {
            SetViewportMode(Function5698C0());
        }
        g_flag_0068edc8 = 0;
    }

    if (static_cast<unsigned char>(leaving)) {
        if (g_status_685170.current_level != -1) {
            UnloadSkyWorld();
            if (!UnloadLevel("")) {
                return 0;
            }
        }
        SoundEmptyCache();
        free(g_level_block);
        g_level_block = 0;
        ReleaseLoadedVideoFrames();
    }
    NoOp();
    MSYS_Shutdown();
    ResetRegions();
    g_flag_65970d = 0;
    g_monster_shadow_updates_enabled_0065970c = 0;
    DisableSky();
    Function598AE0();
    Function5AEB20();
    return 1;
}

// FUNCTION: WIZ8 0x00560c30
void OnQuitGameDialogClosed(W8DialogBase* dialog)
{
    if (GetDialogResult(dialog)) {
        if (AnyCharacterActive()) {
            AutoSaveIfAllowed(1);
        }
        RequestExitScreen();
    }
}

/* Ask for part of the screen to be redrawn. A request made while another
   screen is up, or before the level block exists, is dropped rather than
   queued - which is what makes the block the only place redraw state lives. */
// FUNCTION: WIZ8 0x00562a50
void RequestRedraw(unsigned int mask)
{
    if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
        g_level_block->redraw_flags |= mask;
    }
}

/* Two callers of that with a fixed bit each, written out rather than
   forwarding - which is what shows the mask is a compile-time constant at
   every one of its callers. */
// FUNCTION: WIZ8 0x00565420
void RequestRedrawParty(void)
{
    if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
        g_level_block->redraw_flags |= 0x20000;
    }
}

// FUNCTION: WIZ8 0x005699b0
void RequestRedrawCombatBar(void)
{
    if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
        g_level_block->redraw_flags |= 0x100;
    }
}

/* Note that the party's state changed. The combat half is only asked for while
   a fight is on; the party half always. */
// FUNCTION: WIZ8 0x005653f0
void RequestRefreshPartyState(void)
{
    if (g_level_block == 0) {
        return;
    }
    if (gXStatus.fCombatMode != 0) {
        g_level_block->refresh_combat_panel = 1;
    }
    g_level_block->refresh_party_panel = 1;
}

/* Whether a modal owner has the screen. */
// FUNCTION: WIZ8 0x0056aa20
bool IsModalOpen(void)
{
    return g_modal_owner_0068edd0 != 0;
}

/* The 3D view's screen rectangle for each of the seven viewport modes, with
   exclusive right and bottom edges. Mode 7, which leaving a level stores, sits
   outside the table so the next change always applies. */
// GLOBAL: WIZ8 0x00647d30
W8ScreenRect g_viewport_modes_647d30[7] = {
    {23, 18, 617, 450},  {23, 18, 617, 358},  {128, 18, 512, 358}, {128, 18, 512, 279},
    {128, 18, 512, 450}, {128, 18, 512, 416}, {23, 18, 617, 416},
};

/* The viewport currently applied, as four separate dwords. */
// GLOBAL: WIZ8 0x00647f44
int g_viewport_left_647f44;
// GLOBAL: WIZ8 0x00647f48
int g_viewport_top_647f48;
// GLOBAL: WIZ8 0x00647f4c
int g_viewport_right_647f4c;
// GLOBAL: WIZ8 0x00647f50
int g_viewport_bottom_647f50;

/* Switch the 3D view to another viewport mode: resize the view region to the
   inclusive rectangle and hand the renderer the exclusive one. */
// FUNCTION: WIZ8 0x005618f0
void SetViewportMode(int mode)
{
    W8ScreenRect* rect;

    if (mode == g_level_block->camera_mode_100) {
        return;
    }
    rect = &g_viewport_modes_647d30[mode];
    SetRegionBounds(0xe6, rect->left, rect->top, rect->right - 1, rect->bottom - 1);
    g_viewport_left_647f44 = rect->left;
    g_viewport_top_647f48 = rect->top;
    g_viewport_right_647f4c = rect->right;
    g_viewport_bottom_647f50 = rect->bottom;
    SetViewport(g_viewport_left_647f44, g_viewport_top_647f48, g_viewport_right_647f4c,
                g_viewport_bottom_647f50);
    g_level_block->camera_mode_100 = mode;
}

/* Take the screen for a modal owner and put its region up. */
// FUNCTION: WIZ8 0x005698a0
void OpenModal(W8DialogBase* owner)
{
    g_modal_owner_0068edd0 = owner;
    ActivateDialogRegion(0x138);
}

/* Put the main region set up, and take it down again with its mode reset -
   the two are not symmetric, which is what the extra call shows. */
// FUNCTION: WIZ8 0x00561fa0
void EnableMainRegionSet(void)
{
    RegionSetEnable(W8_REGION_SET_MAIN);
}

// FUNCTION: WIZ8 0x00561fb0
void DisableMainRegionSet(void)
{
    RegionSetDisable(W8_REGION_SET_MAIN);
    DisableRegionSetInput(W8_REGION_SET_MAIN);
}

/* Clear whatever the screen was waiting on and hand the tenth reason to the
   frame. */
// FUNCTION: WIZ8 0x00565970
void ClearScreenWait(void)
{
    g_pending_screen_state.mode = 0;
    SetPendingScreenState(W8_SCREEN_OPTIONS);
}

/* Forget the whole combat selection - what is picked, what it is aimed at, and
   what is going to be done - and then re-derive who is acting. */
// FUNCTION: WIZ8 0x0056a5a0
void ClearCombatSelection(void)
{
    SetCombatSelection(-1);
    SetCombatTarget(-1);
    SetCombatAction(-1);
    SetTargetCursor(GetTargetingCursorForState(0));
}

/* Drop the highlight when the thing being highlighted is the one going away. */
// FUNCTION: WIZ8 0x0056a2a0
void ClearHighlightIfItIs(const int* item)
{
    if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0 &&
        g_level_block->highlighted_item != -1 && *item == g_level_block->highlighted_item) {
        VideoRemoveToolTip();
    }
}

/* Whether the screen is in one of the states that takes the player's input
   away. The first flag settles it outright; otherwise one state only counts
   while a further check disagrees, and three more count on their own. */
// FUNCTION: WIZ8 0x00562540
int IsScreenInputBlocked(void)
{
    if (gXStatus.fSpellCastMode != 0) {
        return 1;
    }
    if (gXStatus.fNpcDialogueMode != 0 && !Function577850()) {
        return 1;
    }
    if (gXStatus.fLockInteractMode == 0 && gXStatus.fTrapInteractMode == 0 &&
        gXStatus.fItemSelectMode == 0) {
        return 0;
    }
    return 1;
}

/* Whether the screen is idle - none of the six overlays is up. The same six
   flags the input block reads, but all of them and unconditionally. */
// FUNCTION: WIZ8 0x00561440
int IsScreenIdle(void)
{
    if (gXStatus.fCombatMode == 0 && gXStatus.fSpellCastMode == 0 &&
        gXStatus.fItemSelectMode == 0 && gXStatus.fNpcDialogueMode == 0 &&
        gXStatus.fLockInteractMode == 0 && gXStatus.fTrapInteractMode == 0) {
        return 1;
    }
    return 0;
}

/* Load the level the party is on. With no level yet there is nothing to load
   and the answer is yes; otherwise the loading flag is up for the duration so
   whatever watches it knows. */
// FUNCTION: WIZ8 0x00560a20
bool LoadCurrentLevelData(void)
{
    bool loaded = true;

    if (g_status_685170.current_level != -1) {
        SetTargetCursor(W8_CURSOR_MAP_LOAD);
        g_world_cleanup_flag_00659757 = 1;
        UnloadSkyWorld();
        loaded = UnloadLevel("MAP") != 0;
        g_world_cleanup_flag_00659757 = 0;
        UpdateHeldItemCursor();
    }
    return loaded;
}

/* Note what the pointer is hovering over. Moving to anything else restarts the
   tooltip clock; staying put leaves it running, which is what makes the four
   fields one tooltip rather than four settings. */
// FUNCTION: WIZ8 0x00569c60
void SetTooltipSubject(int kind, int subject)
{
    if (g_level_block->tooltip_kind != kind || g_level_block->tooltip_subject != subject) {
        g_level_block->tooltip_pending = 1;
        g_level_block->tooltip_since = GetTickCount();
        g_level_block->tooltip_subject = subject;
        g_level_block->tooltip_kind = kind;
    }
}

/* Put the seven combat regions into their inactive mode, and the eighth with
   its whole set only when the screen says it is not needed. */
// FUNCTION: WIZ8 0x005690c0
void DisableCombatRegions(void)
{
    DisableRegionInput(0x52);
    DisableRegionInput(0x53);
    DisableRegionInput(0x54);
    DisableRegionInput(0x55);
    DisableRegionInput(0x56);
    DisableRegionInput(0x57);
    DisableRegionInput(0x58);
    if (g_level_block->flag_155 == 0) {
        DisableRegionInput(0x59);
        RegionSetDisable(0x14);
    }
}

/* Hand one frame to whichever overlays are up. Each is independent, so more
   than one can take the same frame. */
// FUNCTION: WIZ8 0x0056af20
void UpdateScreenOverlays(int frame)
{
    if (gXStatus.fLockInteractMode != 0) {
        Function5879A0(frame);
    }
    if (gXStatus.fTrapInteractMode != 0) {
        Function58A790(frame);
    }
    if (gXStatus.fSpellCastMode != 0) {
        Function59F2B0();
    }
    if (gXStatus.fItemSelectMode != 0) {
        Function59CAC0();
    }
    if (gXStatus.fReviewCharacterMode != 0) {
        CloseReviewCommonUi();
    }
}

/* Which party portrait the pointer is over, if any. The slots are walked
   against two runs of region numbers at once - one starting at 0x24 six apart
   and one at 0x5a one apart - and only two event kinds are answered. */
// FUNCTION: WIZ8 0x00569c00
unsigned int HitTestPartyPortrait(const InputAtom* event)
{
    unsigned int region = 0x24;
    int slot = 0;
    unsigned int kind;

    while (g_status_685170.buffers.party_rows[slot].occupied == 0 ||
           (g_level_block->hover_region != region &&
            g_level_block->hover_region != (unsigned int)(slot + 0x5a))) {
        region += 6;
        ++slot;
        if (region > 0x53) {
            return 0;
        }
    }
    kind = *(const unsigned short*)((const char*)event + 6);
    if (kind == 8 || kind == 0x10) {
        return DispatchRegionInput(event);
    }
    return 0;
}

/* Shared by the main-game screen and dialog text entries; it lives with the
   main-game text helpers, not with UtilityFunctions.cpp. */
// FUNCTION: WIZ8 0x00577410
void ShortenTextToWidth00577410(wchar_t* output, const wchar_t* text, unsigned int width, int font)
{
    wchar_t buffer[200];
    wcscpy(buffer, text);
    if (static_cast<unsigned int>(StringPixLength(buffer, font)) < width) {
        wcscpy(output, buffer);
        return;
    }
    for (int index = 0; index < static_cast<int>(wcslen(buffer)); ++index) {
        if (width <= static_cast<unsigned int>(StringPixLengthArg(font, index + 1, buffer))) {
            --index;
            while (index >= 0) {
                if (buffer[index] != L' ' && buffer[index - 1] != L' ') {
                    buffer[index] = L'\0';
                    swprintf(output, L"%s...", buffer);
                    return;
                }
                --index;
            }
            return;
        }
    }
}

/* Forward a monster-script notice to the targeting layer unless the screen is
   busy or this NPC kind suppresses it. The suppress flag travels as an int:
   the body forwards the whole dword without masking. */
// FUNCTION: WIZ8 0x0056C590
void ForwardNpcScriptNotice(W8NpcState* npc, int value, int line, int suppress)
{
    if (gXStatus.fNpcDialogueMode == 0 && gXStatus.fCombatMode == 0 &&
        (npc->record->kind != 7 || GetFact(0x1c) != 1)) {
        Function56C5E0(npc, value, line, suppress, 0);
    }
}

/* The panel flags and the modal-dialog frame hooks. The g_flag_006840bc state
   is declared in MainGameScreen.h so the renderer consumes the same object. */

// FUNCTION: WIZ8 0x00568950
unsigned int DispatchMainGameMouseButtons(const InputAtom* input)
{
    POINT point;
    SGPMouseGetPos(&point);
    switch (input->usEvent) {
    case LEFT_BUTTON_DOWN:
    case LEFT_BUTTON_UP:
    case RIGHT_BUTTON_DOWN:
    case RIGHT_BUTTON_UP:
        MSYS_SGP_Mouse_Handler_Hook(input->usEvent, static_cast<unsigned short>(point.x),
                                    static_cast<unsigned short>(point.y), gfLeftButtonState,
                                    gfRightButtonState);
        return 1;
    default:
        return 0;
    }
}

// FUNCTION: WIZ8 0x0056aa30
void PauseMainGameWorld(void)
{
    g_flag_006840bc = 1;
    if (gXStatus.fPartyMovementUi != 0) {
        DisableRegionSet1C();
    }
    if (gXStatus.fCombatMode == 0) {
        if (g_flag_006840bd != 0) {
            MoveTimer(1);
            EnableRegionInput(0x137);
            ActivateDialogRegion(0x137);
        }
        SetEnvironmentTimeEnabled00482990(0);
        MonsterForward453160();
        ResetLevelDataVectors0041F0D0();
    }
    if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
        g_level_block->redraw_flags |= 0x8000;
    }
}

// FUNCTION: WIZ8 0x0056aab0
void ResumeMainGameWorld(void)
{
    if (gXStatus.fSpellCastMode == 0 && gXStatus.fNpcDialogueMode == 0 &&
        gXStatus.fItemSelectMode == 0 && gXStatus.fReviewCharacterMode == 0) {
        if (gXStatus.fCombatMode == 0) {
            if (g_flag_006840bd != 0) {
                MoveTimer(4);
                ClearActiveRegionIfMatches(0x137);
                DisableRegionInput(0x137);
            }
            SetEnvironmentTimeEnabled00482990(1);
            MonsterForward4531A0();
            if (gXStatus.fLockInteractMode == 0 && gXStatus.fTrapInteractMode == 0 &&
                gXStatus.fLockInteract == 0 && gXStatus.fTrapInteract == 0) {
                ClearLevelDataFlag6();
            }
        }
        g_flag_006840bc = 0;
        g_flag_006840bd = 0;
        if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block->flag_327 == 0) {
            if (gXStatus.fPartyMovementUi != 0) {
                Function5A1950();
            }
            ClearSurfaceRect(0xb1, 0x13f, 0x1cf, 0x153);
            InvalidateRegion(0xb1, 0x13f, 0x1cf, 0x153, 0);
            if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
                g_level_block->redraw_flags |= 0x8000;
            }
        }
    }
}
