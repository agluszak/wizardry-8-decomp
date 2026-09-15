#include "line.h"
#include "wiz8/local_screens/MGSFormation.h"
#include "soundman.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/Sight.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/local_code/FormationAndFacing.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/local_code/party_encumbrance.h"
#include "wiz8/npc_interaction.h"
#include "wiz8/text_input.h"
#include "wiz8/video_object_catalog.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/engine_code/Levels.h"
#include "wiz8/local_screens/MGSSpellCasting.h"
#include "wiz8/local_screens/AutomapScreen.h"
#include "wiz8/local_screens/mipe.h"
#include "wiz8/local_code/Targeting.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/MGSRadarMap.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/layouts/npc_state.h"
#include "wiz8/local_code/NPCManager.h"
#include "wiz8/local_code/NPCScripting.h"
#include "wiz8/npc_script_file.h"
#include "wiz8/layouts/item_instance.h"
#include "wiz8/layouts/item_tables.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/fact_state.h"
#include "wiz8/local_screens/MGSKeyboard.h"
#include "wiz8/engine_code/Environment.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/engine_code/stModelInstance.h"
#include "wiz8/local_screens/RCSCommon.h"
#include "wiz8/local_code/LoadSaveGame.h"
#include "wiz8/local_code/MonsterAI.h"
#include "wiz8/dialog_code/DialogInterface.h"
#include "wiz8/dialog_code/MessageDialogBase.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/cursor.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/Navigator.h"
#include "wiz8/engine_code/3d.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/music_playlist.h"
#include "wiz8/sr_api.h"
#include "wiz8/utility.h"
#include "wiz8/notices.h"
#include "wiz8/regions.h"
#include "wiz8/layouts/screen_state.h"
#include "wiz8/local_code/Gameloop.h"
#include "wiz8/fonts.h"
#include "wiz8/local_code/GameplayDatabase.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_screens/MGSUseItemSelect.h"
#include "wiz8/local_screens/RCSItemsPage.h"
#include "wiz8/dialog_code/AssayDialog.h"
#include "wiz8/dialog_code/PortraitQuote.h"
#include "wiz8/local_code/NPCManager.h"
#include "wiz8/npc_script_file.h"
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
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/engine_code/3dapi.h"
#include "wiz8/local_code/ItemManager.h"
#include "wiz8/local_screens/MGSPartyMovement.h"
#include "wiz8/engine_code/Cursor3d.h"
#include "wiz8/local_screens/Screens.h"
#include "wiz8/local_screens/MGSPortraits.h"
#include "wiz8/local_screens/ReviewCharacterScreen.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/string_database.h"
#include "wiz8/local_screens/IntroScreen.h"
#include "wiz8/local_screens/JournalScreen.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/layouts/character.h"
#include "wiz8/character_skills.h"
#include "wiz8/local_code/CharGeneration.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/GameplayMods.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/UtilityFunctions.h"
#include "wiz8/engine_code/Spells.h"
#include "wiz8/float_constants.h"
#include "wiz8/local_code/Traps.h"
#include "wiz8/local_code/ButtonSound.h"
#include "vobject_blitters.h"
#include "random.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wiz8/layouts/game_status.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/local_screens/OptionsScreen.h"

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
/* 0x0068EE60: the queued NPC script notice; see the type comment in the
   header. */
// GLOBAL: WIZ8 0x0068EE60
W8PendingNotice g_pending_notice_68ee60;
// GLOBAL: WIZ8 0x006F04EC
unsigned char g_flag_006f04ec;
// GLOBAL: WIZ8 0x0068f0fc
unsigned char g_debug_monster_cycle_0068f0fc;

// GLOBAL: WIZ8 0x0068f100
W8MipeState* g_mipe_state_0068f100;

// GLOBAL: WIZ8 0x0068f2c8
unsigned int g_main_game_text_panel_region_set_0068f2c8;
// GLOBAL: WIZ8 0x0068f2cc
unsigned int g_main_game_text_key_region_set_0068f2cc;
// GLOBAL: WIZ8 0x0068f2d0
unsigned int g_main_game_action_panel_region_set_0068f2d0;

// GLOBAL: WIZ8 0x0061e9ec
unsigned short g_value_0061e9ec[] = {
    0x542, 0x543, 0x544, 0x545, 0x546, 0x547, 0x548, 0x549, 0x54a, 0x54b,
    0x54c, 0x54d, 0x54e, 0x54f, 0x550, 0,     0x551, 0x552, 0x553, 0x554,
    0x555, 0,     0x556, 0x557, 0x558, 0x559, 0x55a, 0x55b,
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

void Function56E510(void);
// GLOBAL: WIZ8 0x0064BA80
int g_lock_pin_target_height_64ba80[4] = {10, 16, 22, 28};
// GLOBAL: WIZ8 0x0064BAE8
char s_lock_pin_falling_64bae8[] = "Data\\Sound\\Misc\\Lock_Pin_Falling.wav";
// GLOBAL: WIZ8 0x0064BABC
char s_lock_picking_success_64babc[] = "Data\\Sound\\Misc\\Lock_Picking_Success.wav";
// GLOBAL: WIZ8 0x0064BB10
char s_lock_pin_rising_64bb10[] = "Data\\Sound\\Misc\\Lock_Pin_Rising.wav";
// GLOBAL: WIZ8 0x0064BB34
char s_lock_forcing_fail_64bb34[] = "Data\\Sound\\Misc\\Lock_Forcing_Fail.wav";
// GLOBAL: WIZ8 0x0064BB5C
char s_lock_forcing_success_64bb5c[] = "Data\\Sound\\Misc\\Lock_Forcing_Successful.wav";
// GLOBAL: WIZ8 0x0068F2B4
int g_lock_phase_68f2b4;
// GLOBAL: WIZ8 0x0068F2B8
unsigned int g_lock_tumbler_region_set_68f2b8;
// GLOBAL: WIZ8 0x0068F2BC
unsigned int g_lock_action_region_set_68f2bc;
// GLOBAL: WIZ8 0x0068F2C0
W8LockInteraction* g_lock_interaction_68f2c0;
/* 0x004457A0: thiscall on the embedded lock/trap state at Trigger+0x368;
   decrements the charge count at its +0x1c and reports whether one remained. */
int __fastcall Function4457A0(int* lock_state);
/* 0x00586A70: the selected slot's effective power with spell 0x27. */
int GetKnockKnockSpellPower00586A70(int slot);
void Function593360(void);
unsigned char Function57E3C0(void);

// FUNCTION: WIZ8 0x00587960
void ProcessLockInteractMode(void)
{
    g_lock_interaction_68f2c0->Process();
}

// FUNCTION: WIZ8 0x005854B0
void W8LockTumbler::Redraw(int full_redraw)
{
    int left;
    int top;
    int right;
    int bottom;
    int frame;

    if (!m_active) {
        return;
    }
    if (!full_redraw && !m_dirty) {
        return;
    }
    left = m_left + m_pPanel->origin_x;
    right = m_right + m_pPanel->origin_x;
    top = m_top + m_pPanel->origin_y;
    bottom = m_bottom + m_pPanel->origin_y;
    InvalidateRegion(left - 3, top, right + 3, bottom, 0);
    Function402FA0(-0xe, left, top, right, top + 0x24, 0x8000);
    Function402FA0(-0xe, left - 3, top + 0x24, right + 3, bottom, 0x8000);
    DrawCatalogImage(-0xe, 0x1ae, 0, 0, left + 3, top, 2, 0);
    frame = m_pin_index_3c * 4 + 1;
    if (m_pin_set_34) {
        frame += m_at_top_37 ? 2 : 1;
    } else if (m_hovered_38 && !m_rising_35 && !m_falling_36) {
        frame += 3;
    }
    DrawCatalogImage(-0xe, 0x1ae, 0, frame, left, top, 2, 0);
    if (m_at_top_37) {
        DrawCatalogImage(-0xe, 0x1ae, 0, g_lock_phase_68f2b4 + 0x11, right, top - 4, 2, 0);
    }
    m_dirty = 0;
}

// FUNCTION: WIZ8 0x00585610
void W8LockTumbler::OnMouseEnter(int event)
{
    PushButtonSoundScheme005587C0(0, 1);
    m_hovered_38 = 1;
    if (m_enabled && !m_rising_35 && !m_falling_36 && !m_pin_set_34) {
        Invalidate(event);
    }
}

// FUNCTION: WIZ8 0x00585650
void W8LockTumbler::OnMouseLeave(int event)
{
    PushButtonSoundScheme005587C0(0, 1);
    m_hovered_38 = 0;
    if (m_enabled && !m_pin_set_34) {
        Invalidate(event);
    }
}

// FUNCTION: WIZ8 0x00585690
void W8LockTumbler::OnLeftButtonUp(int event)
{
    PushButtonSoundScheme005587C0(0, 1);
    if (m_enabled && m_hovered_38 && !m_rising_35 && !m_falling_36 && !m_pin_set_34 &&
        m_listener_44 != 0) {
        m_listener_44->OnTumblerReleased(this);
    }
}

// FUNCTION: WIZ8 0x005856E0
W8LockTumblerPanel::W8LockTumblerPanel(int tumbler_count, const unsigned char* pin_data)
    : Controls(0xd3, 0x166, 0, 0, 0x1af, 0, 1), m_phase_timer_7c(0.04f, 0),
      m_rise_timer_a0(0.03f, 0), m_fall_timer_c4(0.01f, 0)
{
    int i;
    int x;

    m_tumbler_count_50 = tumbler_count;
    m_animating_74 = 0;
    m_phase_78 = 0;
    m_listener_e8 = 0;
    AcquireRegionSet(&g_lock_tumbler_region_set_68f2b8);
    for (i = 0, x = 0x28; x < 0x108; i++, x += 0x1c) {
        m_tumblers_54[i] = new W8LockTumbler(this, x, 8, x + 0x14, 0x52, pin_data[i]);
        m_tumblers_54[i]->m_listener_44 = this;
    }
    EnableRegionSet(1);
    SetEnabled(1);
    Invalidate(0);
    if (m_tumbler_count_50 < 8) {
        for (i = m_tumbler_count_50; i < 8; i++) {
            m_tumblers_54[i]->SetActive(0);
        }
    }
}

// FUNCTION: WIZ8 0x005858C0
W8LockTumblerPanel::~W8LockTumblerPanel()
{
    EnableRegionSet(0);
    DestroyAllControls();
}

// FUNCTION: WIZ8 0x00585950
void W8LockTumblerPanel::OnTumblerReleased(W8LockTumbler* tumbler)
{
    int index;

    if (m_animating_74) {
        return;
    }
    for (index = 0; index < m_tumbler_count_50; index++) {
        if (m_tumblers_54[index] == tumbler) {
            break;
        }
    }
    if (m_listener_e8 != 0) {
        m_listener_e8->OnTumblerPicked(index);
    }
}

// FUNCTION: WIZ8 0x00585990
void W8LockTumblerPanel::UpdateTumblerAnimation()
{
    W8LockTumbler* tumbler;
    int phase;
    int rise;
    int fall;
    int i;

    phase = static_cast<int>(m_phase_timer_7c.GetProgress());
    rise = static_cast<int>(m_rise_timer_a0.GetProgress());
    fall = static_cast<int>(m_fall_timer_c4.GetProgress());
    if (phase != 0) {
        m_phase_78 += phase;
        g_lock_phase_68f2b4 = m_phase_78 % 0xc;
        if (g_lock_phase_68f2b4 > 6) {
            g_lock_phase_68f2b4 = 0xc - g_lock_phase_68f2b4;
        }
        for (i = 0; i < m_tumbler_count_50; i++) {
            if (m_tumblers_54[i]->m_at_top_37) {
                m_tumblers_54[i]->Invalidate(0);
            }
        }
    }
    if (rise != 0 || fall != 0) {
        m_animating_74 = 0;
        for (i = 0; i < m_tumbler_count_50; i++) {
            tumbler = m_tumblers_54[i];
            if (rise != 0 && tumbler->m_rising_35) {
                tumbler->m_pin_height_40 -= rise;
                if (tumbler->m_pin_height_40 <=
                    g_lock_pin_target_height_64ba80[tumbler->m_pin_index_3c]) {
                    tumbler->m_pin_height_40 =
                        g_lock_pin_target_height_64ba80[tumbler->m_pin_index_3c];
                    tumbler->m_rising_35 = 0;
                    tumbler->m_pin_set_34 = 1;
                }
                tumbler->Invalidate(0);
            }
            if (fall != 0 && tumbler->m_falling_36) {
                tumbler->m_pin_height_40 += fall;
                if (tumbler->m_pin_height_40 > 0x21) {
                    tumbler->m_pin_height_40 = 0x22;
                    tumbler->m_falling_36 = 0;
                }
                tumbler->Invalidate(0);
            }
            if (tumbler->m_rising_35 || tumbler->m_falling_36) {
                m_animating_74 = 1;
            }
        }
    }
}

// FUNCTION: WIZ8 0x00585B00
W8LockInfoPanel::W8LockInfoPanel(int tumbler_count) : Controls(0x17, 0x166, 0, 0, 0x1af, 0, 0)
{
    W8ControlsRect bounds;

    m_tumbler_count_4c = tumbler_count;
    bounds.left = origin_x + 0x25;
    bounds.top = origin_y + 7;
    bounds.right = origin_x + 0xb4;
    bounds.bottom = origin_y + 0x11;
    m_text_050 = new W8TextBuffer(&bounds, 0, g_font_683660, g_W8TextBufferLayoutMask005ED54C, 4);
    bounds.top += 0xe;
    bounds.bottom += 0xe;
    m_text_054 = new W8TextBuffer(&bounds, gppStringList[0x1ea4 / 4], g_font_683660,
                                  g_W8TextBufferLayoutMask005ED548, 4);
    bounds.top += 0xe;
    bounds.bottom += 0xe;
    m_text_05c = new W8TextBuffer(&bounds, gppStringList[0x1ea8 / 4], g_font_683660,
                                  g_W8TextBufferLayoutMask005ED548, 4);
    bounds.top += 0xe;
    bounds.bottom += 0xe;
    m_text_064 = new W8TextBuffer(&bounds, gppStringList[0x1eac / 4], g_font_683660,
                                  g_W8TextBufferLayoutMask005ED548, 4);
    bounds.left = origin_x + 0x98;
    bounds.top = origin_y + 0x15;
    bounds.right = origin_x + 0xb5;
    bounds.bottom = origin_y + 0x1f;
    m_text_058 = new W8TextBuffer(&bounds, 0, g_font_683660, g_W8TextBufferLayoutMask005ED54C, 4);
    bounds.top += 0xe;
    bounds.bottom += 0xe;
    m_text_060 = new W8TextBuffer(&bounds, 0, g_font_683660, g_W8TextBufferLayoutMask005ED54C, 4);
    bounds.top += 0xe;
    bounds.bottom += 0xe;
    m_text_068 = new W8TextBuffer(&bounds, 0, g_font_683660, g_W8TextBufferLayoutMask005ED54C, 4);
    SetEnabled(1);
    Invalidate(0);
}

// FUNCTION: WIZ8 0x00585E20
W8LockInfoPanel::~W8LockInfoPanel()
{
    delete m_text_050;
    delete m_text_054;
    delete m_text_058;
    delete m_text_05c;
    delete m_text_060;
    delete m_text_064;
    delete m_text_068;
}

// FUNCTION: WIZ8 0x00585ED0
void W8LockInfoPanel::RefreshInfo()
{
    W8Character* character =
        &g_status_685170.buffers.characters[g_status_685170.selected_character];
    unsigned int figure;
    int divisor;
    unsigned int book;
    unsigned int realm;

    m_text_050->SetText(character->name, g_font_683660);
    if (!IsPartySlotEligible00524A10(g_status_685170.selected_character) ||
        (character->skills[10].flag_00 == 0 && character->skills[10].level == 0) ||
        static_cast<int>(character->skills[10].level) < 0) {
        m_text_058->SetFontStateIndex(0);
        m_text_058->SetText(g_dash_0064789c, g_font_683660);
    } else {
        m_text_058->SetFontStateIndex(-1);
        m_text_058->SetText(
            FormatWideString(g_format_d_percent_0064bab0, character->skills[10].level),
            g_font_683660);
    }
    if (!IsPartySlotEligible00524A10(g_status_685170.selected_character) ||
        character->spell_learned[0x27] != 1) {
        m_text_060->SetFontStateIndex(0);
        m_text_060->SetText(g_dash_0064789c, g_font_683660);
    } else {
        book = GetBestSpellbookSkillForSpell(character, 0x27, 1, 0, 7, 0);
        realm = character->skills[0x1c + g_spell_records[0x27].realm].level;
        book = character->skills[book].level;
        m_text_060->SetFontStateIndex(-1);
        m_text_060->SetText(FormatWideString(g_format_d_percent_0064bab0, (book + realm * 4) / 5),
                            g_font_683660);
    }
    if (IsPartySlotEligible00524A10(g_status_685170.selected_character) &&
        character->stamina > 0x4f && character->attributes[0].effective > 0x32) {
        divisor = g_settings_6850c8.difficulty - 1 + m_tumbler_count_4c;
        ClampInteger(&divisor, 2, 8);
        figure = (character->attributes[0].effective - 0x32) / IntegerPower(2, divisor - 2);
        if (static_cast<int>(figure) > -1) {
            m_text_068->SetFontStateIndex(-1);
            m_text_068->SetText(FormatWideString(g_format_d_percent_0064bab0, figure),
                                g_font_683660);
            goto done;
        }
    }
    m_text_068->SetFontStateIndex(0);
    m_text_068->SetText(g_dash_0064789c, g_font_683660);
done:
    m_text_054->SetGeometryDirty();
    m_text_05c->SetGeometryDirty();
    m_text_064->SetGeometryDirty();
    Invalidate(0);
}

// FUNCTION: WIZ8 0x00586120
void W8LockInfoPanel::Redraw()
{
    if (m_fEnabled && m_fDirty) {
        Controls::Redraw();
        m_text_050->RenderToTarget(0, 0, -0xe);
        m_text_054->RenderToTarget(0, 0, -0xe);
        m_text_058->RenderToTarget(0, 0, -0xe);
        m_text_05c->RenderToTarget(0, 0, -0xe);
        m_text_060->RenderToTarget(0, 0, -0xe);
        m_text_064->RenderToTarget(0, 0, -0xe);
        m_text_068->RenderToTarget(0, 0, -0xe);
    }
}

// FUNCTION: WIZ8 0x005861A0
W8LockInteraction::W8LockInteraction(Trigger* trigger) : m_timer_80()
{
    W8Character* character;
    int level;
    int power;
    unsigned int figure;
    int divisor;
    unsigned int book;
    unsigned int realm;
    int i;

    m_trigger_08 = trigger;
    m_state_34 = 0;
    m_tumbler_count_0c = trigger->value_36c;
    if (m_tumbler_count_0c < 2) {
        m_tumbler_count_0c = 2;
    } else if (m_tumbler_count_0c > 8) {
        m_tumbler_count_0c = 8;
    }
    m_tumbler_panel_10 = new W8LockTumblerPanel(m_tumbler_count_0c, trigger->state_370.bytes_01);
    m_tumbler_panel_10->m_listener_e8 = this;
    m_info_panel_14 = new W8LockInfoPanel(m_tumbler_count_0c);
    m_action_panel_18 = new Controls(0x1e7, 0x166, 0, 0, 0x1af, 0, 2);
    m_action_panel_18->AcquireRegionSet(&g_lock_action_region_set_68f2bc);
    m_spell_button_20 =
        new W8TextControl(m_action_panel_18, 0xffffffff, 6, 6, 0, 0, 0x1b0, 0, 8, 10, 9, 10, 0xb);
    m_spell_button_20->m_listener = this;
    m_force_button_24 =
        new W8TextControl(m_action_panel_18, 0xffffffff, 6, 0x24, 0, 0, 0x1b0, 0, 0, 2, 1, 2, 3);
    m_force_button_24->m_listener = this;
    m_cancel_button_28 =
        new W8TextControl(m_action_panel_18, 0xffffffff, 6, 0x24, 0, 0, 0x1b0, 0, 4, 6, 5, 6, 7);
    m_cancel_button_28->m_listener = this;
    m_done_button_1c =
        new W8TextControl(m_action_panel_18, 0xffffffff, 0x44, 6, 0, 0, 0x8e, 0, 4, 6, 5, 6, 7);
    m_done_button_1c->m_listener = this;
    m_spell_button_20->EnableRegionHelp(0x7cb);
    m_force_button_24->EnableRegionHelp(0x7cd);
    m_cancel_button_28->EnableRegionHelp(0x7cc);
    m_action_panel_18->EnableRegionSet(1);
    m_action_panel_18->SetEnabled(1);
    m_action_panel_18->Invalidate(0);
    m_selected_slot_2c = -1;
    for (i = 0; i < 8; i++) {
        m_tumbler_owner_38[i] = -1;
        m_tumbler_locked_58[i] = 0;
    }
    for (i = 0; i < 8; i++) {
        m_slot_attempts_60[i] = 0;
    }
    character = &g_status_685170.buffers.characters[g_status_685170.selected_character];
    if (!IsPartySlotEligible00524A10(g_status_685170.selected_character)) {
        level = -1;
    } else if (character->skills[10].flag_00 == 0 && character->skills[10].level == 0) {
        level = -1;
    } else {
        level = character->skills[10].level;
    }
    for (i = 0; i < m_tumbler_panel_10->m_tumbler_count_50; i++) {
        m_tumbler_panel_10->m_tumblers_54[i]->SetEnabled(level > -1);
    }
    if (IsPartySlotEligible00524A10(g_status_685170.selected_character) &&
        character->spell_learned[0x27] == 1) {
        book = GetBestSpellbookSkillForSpell(character, 0x27, 1, 0, 7, 0);
        realm = character->skills[0x1c + g_spell_records[0x27].realm].level;
        book = character->skills[book].level;
        power = (book + realm * 4) / 5;
        if (power > -1) {
            figure = CanCharacterCastSpell(character, 0x27);
        } else {
            figure = 0;
        }
    } else {
        figure = 0;
    }
    m_spell_button_20->SetEnabled(figure);
    if (!IsPartySlotEligible00524A10(g_status_685170.selected_character) ||
        character->stamina < 0x50 || character->attributes[0].effective <= 0x32) {
        figure = 0xffffffff;
    } else {
        divisor = g_settings_6850c8.difficulty - 1 + m_tumbler_count_0c;
        ClampInteger(&divisor, 2, 8);
        figure = (character->attributes[0].effective - 0x32) / IntegerPower(2, divisor - 2);
    }
    m_force_button_24->SetEnabled(static_cast<int>(figure) > -1);
    m_info_panel_14->RefreshInfo();
    m_action_panel_18->Invalidate(0);
}

// FUNCTION: WIZ8 0x005866A0
W8LockInteraction::~W8LockInteraction()
{
    delete m_tumbler_panel_10;
    delete m_info_panel_14;
    m_action_panel_18->EnableRegionSet(0);
    m_action_panel_18->DestroyAllControls();
    delete m_action_panel_18;
}

// FUNCTION: WIZ8 0x00586740
void W8LockInteraction::Process()
{
    W8Character* character;
    int slot;
    int i;
    int dropped;

    if (m_state_34 == 8 && m_timer_80.GetProgress() >= g_float_005ebb38) {
        m_trigger_08->CompleteItemInteraction004447F0();
        m_trigger_08->Run(-1);
        m_state_34 = 9;
    }
    if (m_state_34 == 9) {
        gXStatus.fLockInteractMode = 0;
        if (g_lock_interaction_68f2c0 != 0) {
            delete g_lock_interaction_68f2c0;
        }
        g_lock_interaction_68f2c0 = 0;
        ClearLevelDataFlag6();
        Function58F6B0(0);
        ApplyMainGameModeFlag(g_value_68f2b0, 1);
        RequestRedraw(0x200);
        RequestRedraw(0x100);
        RequestRedraw(0x1000);
        return;
    }
    m_tumbler_panel_10->UpdateTumblerAnimation();
    if (m_tumbler_panel_10->m_animating_74) {
        return;
    }
    switch (m_state_34) {
    case 1:
        m_state_34 = 0;
        ResolvePick();
        return;
    case 2:
        m_state_34 = 0;
        AttemptForce();
        return;
    case 3:
        dropped = 0;
        m_state_34 = 4;
        slot = g_status_685170.selected_character;
        for (i = 0; i < m_tumbler_count_0c; i++) {
            if (m_tumbler_owner_38[i] == slot && m_tumbler_locked_58[i] != 0) {
                m_tumbler_owner_38[i] = -1;
                m_tumbler_locked_58[i] = 0;
                m_tumbler_panel_10->m_tumblers_54[i]->m_at_top_37 = 0;
                m_tumbler_panel_10->m_tumblers_54[i]->m_pin_set_34 = 0;
                m_tumbler_panel_10->m_tumblers_54[i]->m_falling_36 = 1;
                m_tumbler_panel_10->m_animating_74 = 1;
                dropped = 1;
            }
        }
        m_tumbler_panel_10->Invalidate(0);
        goto lock_release_done;
    case 5:
        dropped = 0;
        m_state_34 = 6;
        slot = g_status_685170.selected_character;
        for (i = 0; i < m_tumbler_count_0c; i++) {
            if (m_tumbler_owner_38[i] == slot && m_tumbler_locked_58[i] != 0) {
                m_tumbler_owner_38[i] = -1;
                m_tumbler_locked_58[i] = 0;
                m_tumbler_panel_10->m_tumblers_54[i]->m_at_top_37 = 0;
                m_tumbler_panel_10->m_tumblers_54[i]->m_pin_set_34 = 0;
                m_tumbler_panel_10->m_tumblers_54[i]->m_falling_36 = 1;
                m_tumbler_panel_10->m_animating_74 = 1;
                dropped = 1;
            }
        }
        m_tumbler_panel_10->Invalidate(0);
    lock_release_done:
        if (dropped) {
            SoundPlay(s_lock_pin_falling_64bae8, 0);
            return;
        }
        break;
    case 4:
        m_state_34 = 0;
        slot = g_status_685170.selected_character;
        if (GetKnockKnockSpellPower00586A70(g_status_685170.selected_character) > -1 &&
            CanCharacterCastSpell(&g_status_685170.buffers.characters[slot], 0x27)) {
            m_spell_button_20->SetAlternateTextEnabled(0);
            Function5879A0(1);
            BeginSpellCast005A0110(0x27, -1, -1);
            return;
        }
        break;
    case 6:
        m_state_34 = 0;
        m_cancel_button_28->SetAlternateTextEnabled(0);
        gXStatus.fLockInteractMode = 0;
        EnablePanels(0);
        gXStatus.fLockInteract = 1;
        Function58F6B0(0);
        ApplyMainGameModeFlag(g_value_68f2b0, 1);
        RequestRedraw(0x200);
        RequestRedraw(0x100);
        RequestRedraw(0x1000);
        Function59C930(g_status_685170.selected_character);
        return;
    case 7:
        m_state_34 = 0;
        for (i = 0; i < m_tumbler_count_0c; i++) {
            if (m_tumbler_owner_38[i] == -1) {
                return;
            }
        }
        SoundPlay(s_lock_picking_success_64babc, 0);
        ShowNotice(0xc, gppStringList[0x1eb0 / 4]);
        BeginUnlock();
        break;
    }
}

// FUNCTION: WIZ8 0x00586A70
int GetKnockKnockSpellPower00586A70(int slot)
{
    W8Character* character = &g_status_685170.buffers.characters[slot];
    unsigned int book;

    if (!IsPartySlotEligible00524A10(slot)) {
        return -1;
    }
    if (character->spell_learned[0x27] != 1) {
        return -1;
    }
    book = GetBestSpellbookSkillForSpell(character, 0x27, 1, 0, 7, 0);
    return (character->skills[book].level +
            character->skills[0x1c + g_spell_records[0x27].realm].level * 4) /
           5;
}

// FUNCTION: WIZ8 0x00586AF0
void W8LockInteraction::EnablePanels(int enable)
{
    m_tumbler_panel_10->EnableRegionSet(enable);
    m_action_panel_18->EnableRegionSet(enable);
}

// FUNCTION: WIZ8 0x00586B10
void W8LockInteraction::OnTumblerPicked(int index)
{
    W8Character* character;
    int i;

    if (!IsPartySlotEligible00524A10(g_status_685170.selected_character)) {
        return;
    }
    character = &g_status_685170.buffers.characters[g_status_685170.selected_character];
    if (character->skills[10].flag_00 == 0 && character->skills[10].level == 0) {
        return;
    }
    if (static_cast<int>(character->skills[10].level) < 0) {
        return;
    }
    if (m_selected_slot_2c != g_status_685170.selected_character) {
        if (m_selected_slot_2c != -1) {
            for (i = 0; i < m_tumbler_count_0c; i++) {
                if (m_tumbler_owner_38[i] == m_selected_slot_2c && m_tumbler_locked_58[i] == 0) {
                    m_tumbler_owner_38[i] = -1;
                    m_tumbler_panel_10->m_tumblers_54[i]->m_pin_set_34 = 0;
                    m_tumbler_panel_10->m_tumblers_54[i]->m_falling_36 = 1;
                    m_tumbler_panel_10->m_animating_74 = 1;
                }
            }
        }
        m_selected_slot_2c = g_status_685170.selected_character;
    }
    m_picked_tumbler_30 = index;
    m_state_34 = 1;
    m_tumbler_panel_10->m_tumblers_54[index]->m_hovered_38 = 0;
    m_tumbler_panel_10->m_tumblers_54[index]->m_rising_35 = 1;
    m_tumbler_panel_10->m_animating_74 = 1;
    SoundPlay(s_lock_pin_rising_64bb10, 0);
}

// FUNCTION: WIZ8 0x00586C00
void W8LockInteraction::OnPrimary(W8TextControl* control)
{
    if (control == m_done_button_1c) {
        m_state_34 = 9;
        return;
    }
    if (m_tumbler_panel_10->m_animating_74 && m_state_34 != 0) {
        return;
    }
    if (control == m_force_button_24) {
        m_state_34 = 2;
    } else if (control == m_spell_button_20) {
        m_state_34 = 3;
    } else if (control == m_cancel_button_28) {
        m_state_34 = 5;
    }
}

// FUNCTION: WIZ8 0x00586C60
void W8LockInteraction::ResolvePick()
{
    W8Character* character;
    int chance;
    int level;
    int i;
    int dropped;

    dropped = 0;
    for (i = 0; i < m_tumbler_count_0c; i++) {
        if (m_tumbler_owner_38[i] == m_selected_slot_2c && m_tumbler_locked_58[i] == 0) {
            character = &g_status_685170.buffers.characters[m_selected_slot_2c];
            chance = character->skills[10].level + g_settings_6850c8.difficulty * -5 + 5;
            if ((chance < 0 ? 0 : static_cast<unsigned char>(chance)) <=
                static_cast<int>(Random(100))) {
                m_tumbler_owner_38[i] = -1;
                m_tumbler_panel_10->m_tumblers_54[i]->m_pin_set_34 = 0;
                m_tumbler_panel_10->m_tumblers_54[i]->m_falling_36 = 1;
                m_tumbler_panel_10->m_animating_74 = 1;
                dropped = 1;
            }
        }
    }
    m_slot_attempts_60[m_selected_slot_2c]++;
    m_tumbler_owner_38[m_picked_tumbler_30] = m_selected_slot_2c;
    if (Random(5) == 0 && Function4457A0(&m_trigger_08->value_368) != 0) {
        character = &g_status_685170.buffers.characters[m_selected_slot_2c];
        level = character->skills[10].level;
        PracticeCharacterSkill(character, 10, 1, 0);
        if (level != static_cast<int>(character->skills[10].level)) {
            m_info_panel_14->RefreshInfo();
        }
    }
    if (dropped) {
        SoundPlay(s_lock_pin_falling_64bae8, 0);
    }
    for (i = 0; i < m_tumbler_count_0c; i++) {
        if (m_tumbler_owner_38[i] == -1) {
            return;
        }
    }
    SoundPlay(s_lock_picking_success_64babc, 0);
    ShowNotice(0xc, gppStringList[0x1eb0 / 4]);
    m_tumbler_panel_10->EnableRegionSet(0);
    m_action_panel_18->EnableRegionSet(0);
    m_state_34 = 8;
    m_timer_80.SetDuration(1.0f);
    m_timer_80.Restart();
}

// FUNCTION: WIZ8 0x00586E40
void W8LockInteraction::AttemptForce()
{
    W8Character* character;
    unsigned int chance;
    unsigned int book;
    int level;
    int power;
    int divisor;
    int i;

    character = &g_status_685170.buffers.characters[g_status_685170.selected_character];
    if (IsPartySlotEligible00524A10(g_status_685170.selected_character) &&
        character->stamina > 0x4f && character->attributes[0].effective > 0x32) {
        divisor = g_settings_6850c8.difficulty - 1 + m_tumbler_count_0c;
        ClampInteger(&divisor, 2, 8);
        chance = (character->attributes[0].effective - 0x32) / IntegerPower(2, divisor - 2);
        if (static_cast<int>(chance) > -1) {
            FatigueCharacter(g_status_685170.selected_character, 0x50, 0, 0);
            if (static_cast<int>(Random(100)) < static_cast<int>(chance)) {
                SoundPlay(s_lock_forcing_success_64bb5c, 0);
                ShowString(FormatWideString(g_format_s_space_s_00617584, character->name,
                                            gppStringList[0x1eb8 / 4]));
                m_tumbler_panel_10->EnableRegionSet(0);
                m_action_panel_18->EnableRegionSet(0);
                m_state_34 = 8;
                m_timer_80.SetDuration(1.0f);
                m_timer_80.Restart();
                return;
            }
            SoundPlay(s_lock_forcing_fail_64bb34, 0);
            ShowString(FormatWideString(g_format_s_space_s_00617584, character->name,
                                        gppStringList[0x1eb4 / 4]));
            if (!IsPartySlotEligible00524A10(g_status_685170.selected_character)) {
                level = -1;
            } else if (character->skills[10].flag_00 == 0 && character->skills[10].level == 0) {
                level = -1;
            } else {
                level = character->skills[10].level;
            }
            for (i = 0; i < m_tumbler_panel_10->m_tumbler_count_50; i++) {
                m_tumbler_panel_10->m_tumblers_54[i]->SetEnabled(level > -1);
            }
            if (!IsPartySlotEligible00524A10(g_status_685170.selected_character) ||
                character->spell_learned[0x27] != 1) {
                m_spell_button_20->SetEnabled(0);
            } else {
                book = GetBestSpellbookSkillForSpell(character, 0x27, 1, 0, 7, 0);
                power = (character->skills[book].level +
                         character->skills[0x1c + g_spell_records[0x27].realm].level * 4) /
                        5;
                if (power > -1) {
                    m_spell_button_20->SetEnabled(CanCharacterCastSpell(character, 0x27));
                } else {
                    m_spell_button_20->SetEnabled(0);
                }
            }
            if (!IsPartySlotEligible00524A10(g_status_685170.selected_character) ||
                character->stamina < 0x50 || character->attributes[0].effective <= 0x32) {
                chance = -1;
            } else {
                divisor = g_settings_6850c8.difficulty + m_tumbler_count_0c - 1;
                ClampInteger(&divisor, 2, 8);
                chance = (character->attributes[0].effective - 0x32) / IntegerPower(2, divisor - 2);
            }
            m_force_button_24->SetEnabled(static_cast<int>(chance) > -1);
            m_info_panel_14->RefreshInfo();
            m_action_panel_18->SetEnabled(0);
        }
    }
}

// FUNCTION: WIZ8 0x005874D0
void W8LockInteraction::BeginUnlock()
{
    m_tumbler_panel_10->EnableRegionSet(0);
    m_action_panel_18->EnableRegionSet(0);
    m_state_34 = 8;
    m_timer_80.SetDuration(1.0f);
    m_timer_80.Restart();
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

/* Unlike the base, the six option buttons stay inactive while the expanded
   NPC dialogue layout (value_fc == 4) is not up. */
// FUNCTION: WIZ8 0x0056BC50
void W8MainGamePanel005EE9F0::SetEnabled(bool enable)
{
    int index;

    m_fEnabled = enable;
    for (index = 0; index < m_controls.count; ++index) {
        if (enable &&
            (ControlAt(index) == g_screen_state_00649f1c->option_buttons_170[0] ||
             ControlAt(index) == g_screen_state_00649f1c->option_buttons_170[1] ||
             ControlAt(index) == g_screen_state_00649f1c->option_buttons_170[2] ||
             ControlAt(index) == g_screen_state_00649f1c->option_buttons_170[3] ||
             ControlAt(index) == g_screen_state_00649f1c->option_buttons_170[4] ||
             ControlAt(index) == g_screen_state_00649f1c->option_buttons_170[5]) &&
            g_screen_state_00649f1c->value_fc != 4) {
            continue;
        }
        ControlAt(index)->SetActive(enable);
    }
}

/* Base redraw except the foreground catalog image is m_value_4c rather than
   m_renderArg_20 while the expanded dialogue layout is up. */
// FUNCTION: WIZ8 0x0056BD30
void W8MainGamePanel005EE9F0::Redraw()
{
    int redrawn = 0;
    int index;

    if (!m_fEnabled) {
        return;
    }
    if (m_fDirty) {
        if (m_renderTarget != -1) {
            DrawCatalogImage(-14, m_renderTarget, m_renderArg_1c,
                             g_screen_state_00649f1c->value_fc == 4 ? m_value_4c : m_renderArg_20,
                             origin_x, origin_y, 2, 0);
        }
        if (m_fWholeAreaDirty) {
            if (m_renderTarget != -1) {
                InvalidateCatalogImageRect(m_renderTarget, m_renderArg_1c, m_renderArg_20, origin_x,
                                           origin_y, 2);
            }
        } else {
            InvalidateRegion(m_dirtyRect.left, m_dirtyRect.top, m_dirtyRect.right,
                             m_dirtyRect.bottom, 2);
        }
        m_fDirty = 0;
        m_dirtyRect.left = -1;
        redrawn = 1;
    } else if (!m_fLayoutDirty) {
        return;
    }
    for (index = 0; index < m_controls.count; ++index) {
        if (ControlAt(index)->m_active) {
            ControlAt(index)->Redraw(redrawn);
        }
    }
    m_fLayoutDirty = 0;
}

/* Enabling starts text-input scheme 1 and installs the typed-dialogue field;
   disabling removes it. An already-enabled panel does none of this. */
// FUNCTION: WIZ8 0x0056BAC0
void W8MainGamePanel005EE9E4::SetEnabled(bool enable)
{
    if (!enable || !m_fEnabled) {
        Controls::SetEnabled(enable);
        if (enable) {
            InitTextInputModeWithScheme(1);
            AddTextInputField(0x1e5, 0x170, 0x7a, 0x12, 0x7f, &g_wchar_00689b34, 0xbe, 0xf, 1);
        } else {
            RemoveTextInputField(0);
        }
    }
}

/* Base redraw plus the input-frame image: drawn at y 0x19b while flag_1d9 is
   raised, else 0x18b. */
// FUNCTION: WIZ8 0x0056BB20
void W8MainGamePanel005EE9E4::Redraw()
{
    int redrawn = 0;
    int index;

    if (!m_fEnabled) {
        return;
    }
    if (m_fDirty) {
        if (m_renderTarget != -1) {
            DrawCatalogImage(-14, m_renderTarget, m_renderArg_1c, m_renderArg_20, origin_x,
                             origin_y, 2, 0);
        }
        DrawCatalogImage(-14, 0x1a9, 0, 0x10, 0x1df,
                         g_screen_state_00649f1c->flag_1d9 ? 0x19b : 0x18b, 2, 0);
        if (m_fWholeAreaDirty) {
            if (m_renderTarget != -1) {
                InvalidateCatalogImageRect(m_renderTarget, m_renderArg_1c, m_renderArg_20, origin_x,
                                           origin_y, 2);
            }
        } else {
            InvalidateRegion(m_dirtyRect.left, m_dirtyRect.top, m_dirtyRect.right,
                             m_dirtyRect.bottom, 2);
        }
        m_fDirty = 0;
        m_dirtyRect.left = -1;
        redrawn = 1;
    } else if (!m_fLayoutDirty) {
        return;
    }
    for (index = 0; index < m_controls.count; ++index) {
        if (ControlAt(index)->m_active) {
            ControlAt(index)->Redraw(redrawn);
        }
    }
    m_fLayoutDirty = 0;
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
    BeginSpellCast005A0110(spell, -1, -1);
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
unsigned char CanOpenNpcDialogue(void)
{
    return gXStatus.fNpcDialogueMode != 0 && g_screen_state_00649f1c->flag_252 != 0;
}

/* Whether an NPC dialogue is up in the layout that posts to the main text
   box (mode 4) rather than to the dialogue's own pane. */
// FUNCTION: WIZ8 0x0056efd0
bool IsNpcDialogueTextBoxActive(void)
{
    if (gXStatus.fNpcDialogueMode == 0) {
        return false;
    }
    return g_screen_state_00649f1c->value_fc == 4;
}

// FUNCTION: WIZ8 0x0055E2C0
void __fastcall CollapseNpcDialogueTextArea(W8NpcDialogueTextController* controller)
{
    W8ControlsRect bounds;
    int top;

    controller->visible = 0;
    top = (controller->origin_y -
           (controller->scroll_height / controller->line_height) * controller->line_height) -
          controller->margin;
    ClearSurfaceRect(controller->origin_x, top, controller->right, controller->bottom);
    controller->Invalidate(0);
    InvalidateRegion(controller->origin_x, top, controller->right, controller->bottom, 0);
    if (top < 0x67) {
        RequestRedraw(2);
        RequestRedraw(8);
        RequestRedraw(0x20);
        RequestRedraw(0x80);
    } else if (top < 0xbc) {
        RequestRedraw(8);
        RequestRedraw(0x20);
        RequestRedraw(0x80);
    } else if (top < 0x111) {
        RequestRedraw(0x20);
        RequestRedraw(0x80);
    } else {
        RequestRedraw(0x80);
    }
    controller->scroll_height = controller->line_height;
    bounds.left = controller->origin_x + 6;
    bounds.right = controller->origin_x + 0x7c;
    bounds.bottom = controller->origin_y + 0x12;
    bounds.top = bounds.bottom - controller->line_height;
    controller->text_area.Configure(&bounds, g_font_683660,
                                    g_W8TextBufferLayoutMask005ED548 |
                                        g_W8TextBufferLayoutMask005ED54C |
                                        g_W8TextBufferLayoutMask005ED550);
    RegionSetDisable(3);
    DisableRegionInput(9);
    g_screen_state_00649f1c->npc_dialogue_panel_1b4->SetEnabled(0);
}

// FUNCTION: WIZ8 0x0055E1E0
void __fastcall ExpandNpcDialogueTextArea(W8NpcDialogueTextController* controller)
{
    W8ControlsRect bounds;
    int line_height;
    int text_height;

    controller->visible = 1;
    line_height = controller->text_area.GetLineHeight();
    text_height = line_height * controller->text_area.GetTotalLineCount();
    if (text_height >= 0xff) {
        text_height = 0xff;
    }
    bounds.left = controller->origin_x + 6;
    bounds.right = controller->origin_x + 0x7c;
    bounds.bottom = controller->origin_y + 0x12;
    bounds.top = bounds.bottom - text_height;
    controller->scroll_height = text_height;
    controller->text_area.Configure(&bounds, g_font_683660,
                                    g_W8TextBufferLayoutMask005ED548 |
                                        g_W8TextBufferLayoutMask005ED54C |
                                        g_W8TextBufferLayoutMask005ED550);
    if (controller->scroll_height != 0xff) {
        controller->text_area.SetFirstVisibleEntry(0);
    }
    SetRegionBounds(
        9, static_cast<unsigned short>(bounds.left), static_cast<unsigned short>(bounds.top),
        static_cast<unsigned short>(bounds.right), static_cast<unsigned short>(bounds.bottom));
    RegionSetEnable(3);
    EnableRegionInput(9);
    controller->Invalidate(0);
}

// FUNCTION: WIZ8 0x0055EAE0
void __fastcall ClearNpcDialogueTextBackground(W8NpcDialogueTextController* controller)
{
    int top;

    top = ((1 - controller->scroll_height / controller->line_height) * controller->line_height -
           controller->margin) +
          controller->origin_y;
    ClearSurfaceRect(controller->origin_x, top, controller->right, controller->bottom);
    InvalidateRegion(controller->origin_x, top, controller->right, controller->bottom, 0);
    if (top < 0x67) {
        RequestRedraw(2);
        RequestRedraw(8);
        RequestRedraw(0x20);
        RequestRedraw(0x80);
        return;
    }
    if (top < 0xbc) {
        RequestRedraw(8);
        RequestRedraw(0x20);
        RequestRedraw(0x80);
        return;
    }
    if (top < 0x111) {
        RequestRedraw(0x20);
        RequestRedraw(0x80);
        return;
    }
    RequestRedraw(0x80);
}

// FUNCTION: WIZ8 0x0055E2B0
bool __fastcall IsNpcDialogueTextExpanded(W8NpcDialogueTextController* controller)
{
    return controller->scroll_height == 0xff;
}

// FUNCTION: WIZ8 0x0055E570
W8NpcDialogueScrollWidget::W8NpcDialogueScrollWidget(Controls* panel, unsigned int region, int left,
                                                     int top, int right, int bottom)
    : W8Widget(panel, region, left, top, right, bottom)
{
    m_flags_34 = 0;
}

// FUNCTION: WIZ8 0x0055E5E0
void W8NpcDialogueScrollWidget::OnMouseEnter(int event)
{
    W8NpcDialogueTextController* controller;

    controller = g_screen_state_00649f1c->npc_dialogue_controller_1b0;
    if (controller->text_area.SelectEntry(controller->text_area.m_first_visible_entry)) {
        controller->InvalidateLayout();
    }
}

// FUNCTION: WIZ8 0x0055E610
void W8NpcDialogueScrollWidget::OnMouseLeave(int event)
{
    W8NpcDialogueTextController* controller;

    controller = g_screen_state_00649f1c->npc_dialogue_controller_1b0;
    if (controller->text_area.ClearSelection()) {
        controller->InvalidateLayout();
    }
}

// FUNCTION: WIZ8 0x0055E640
void W8NpcDialogueScrollWidget::OnLeftButtonDown(int event)
{
    if (m_active && m_enabled) {
        m_flags_34 |= 1;
        if (m_leftButtonDownCallback) {
            m_leftButtonDownCallback();
        }
    }
}

// FUNCTION: WIZ8 0x0055E660
void W8NpcDialogueScrollWidget::OnLeftButtonUp(int event)
{
    if (m_active && m_enabled && (m_flags_34 & 1)) {
        m_flags_34 &= ~1u;
        if (m_primaryActivationCallback) {
            m_primaryActivationCallback();
        }
    }
}

// FUNCTION: WIZ8 0x00577880
unsigned char SetNpcDialoguePanelVisible(int value)
{
    W8NpcDialogueTextController* controller;
    unsigned char expanded;

    if (gXStatus.fNpcDialogueMode != 0 && g_screen_state_00649f1c->flag_252 == 0 &&
        g_screen_state_00649f1c->value_fc == 3) {
        if (value == 0) {
            controller = g_screen_state_00649f1c->npc_dialogue_controller_1b0;
            CollapseNpcDialogueTextArea(controller);
            controller = g_screen_state_00649f1c->npc_dialogue_controller_1b0;
            controller->SetEnabled(0);
            g_screen_state_00649f1c->npc_dialogue_panel_1b4->SetEnabled(0);
            controller = g_screen_state_00649f1c->npc_dialogue_controller_1b0;
            ClearNpcDialogueTextBackground(controller);
            ClearSurfaceRect(0x1dc, 0x11b, 0x269, 0x1c2);
            InvalidateRegion(0x1dc, 0x11b, 0x269, 0x1c2, 0);
            RequestRedraw(2);
            RequestRedraw(8);
            RequestRedraw(0x20);
            RequestRedraw(0x80);
            RequestRedraw(0x200);
            g_screen_state_00649f1c->dialogue_panel_hidden = 1;
            return 1;
        }

        controller = g_screen_state_00649f1c->npc_dialogue_controller_1b0;
        controller->SetEnabled(1);
        g_screen_state_00649f1c->npc_dialogue_panel_1b4->SetEnabled(1);
        controller = g_screen_state_00649f1c->npc_dialogue_controller_1b0;
        ExpandNpcDialogueTextArea(controller);
        controller = g_screen_state_00649f1c->npc_dialogue_controller_1b0;
        expanded = IsNpcDialogueTextExpanded(controller);
        if (expanded == 0) {
            g_screen_state_00649f1c->dialogue_widget_134->SetEnabled(0);
        } else {
            g_screen_state_00649f1c->dialogue_widget_134->SetEnabled(1);
        }
        g_screen_state_00649f1c->dialogue_widget_138->SetEnabled(expanded != 0);
        RequestRedraw(0x200);
        g_screen_state_00649f1c->dialogue_panel_hidden = 0;
        return 1;
    }
    return 0;
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
void RefreshFlaggedMainGameState00593330(void)
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
void SyncDialogueNpcStateAndMarkPending00577220(void)
{
    Function56C6D0(g_screen_state_00649f1c->dialogue_npc, 0, -1, 0, 1);
    g_screen_state_00649f1c->value_238 = g_screen_state_00649f1c->value_104;
    g_screen_state_00649f1c->flag_234 = 1;
}

// FUNCTION: WIZ8 0x00577260
void SyncDialogueNpcState00577260(void)
{
    Function56C6D0(g_screen_state_00649f1c->dialogue_npc, 0, -1, 0, 1);
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

/* Translate a typed dialogue keyword through the loaded tables. With no
   tables loaded the input passes through verbatim; otherwise the active
   language's list is scanned and the matching English field is copied out.
   Element one holds the translated file when two loaded, falling back to the
   English list through GetAt's clamped read. */
// FUNCTION: WIZ8 0x0056c440
void TranslateDialogueKeyword0056C440(const wchar_t* source, wchar_t* destination)
{
    W8GrowableVector<W8GrowableVector<wchar_t*>*>* file;
    W8GrowableVector<wchar_t*>* entry;
    W8GrowableVector<wchar_t*>* english;
    int entry_index;
    int word_index;

    if (g_keyword_lists_loaded_68f0f8 == 0) {
        wcscpy(destination, source);
        return;
    }
    file = *g_keyword_lists.GetAt(1);
    for (entry_index = 0; entry_index < file->count; ++entry_index) {
        entry = *file->GetAt(entry_index);
        for (word_index = 0; word_index < entry->count; ++word_index) {
            if (CompareWideTextIgnoreAsciiCase00402920(source, *entry->GetAt(word_index)) == 0) {
                english = *(*g_keyword_lists.GetAt(0))->GetAt(entry_index);
                wcscpy(destination, *english->GetAt(word_index));
                return;
            }
        }
    }
}

/* Reset the screen state block: zero its 0x268 bytes, write its reset values,
   clear the keyword status byte, and reload the keyword lists. */
// FUNCTION: WIZ8 0x0056c520
void ResetMainScreenStateBlock(void)
{
    int unset = -1;

    memset(static_cast<void*>(g_screen_state_00649f1c), 0, sizeof(W8MainScreenState));
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
    CreateConditionButtons();
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
        for (; g_next_link_level_0068ede8 < W8_LEVEL_COUNT; ++g_next_link_level_0068ede8) {
            if (LevelBuildInfoByID(g_next_link_level_0068ede8, &info)) {
                if (g_next_link_level_0068ede8 < W8_LEVEL_COUNT) {
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
        SyncDialogueNpcStateAndMarkPending00577220();
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
            if ((!ShouldDeferCharacterEventForNpcScript(1) || !gXStatus.fNpcDialogueMode) &&
                !g_status_685170.value_2435) {
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
                GetMonsterByLocationID(IListGetAt(&g_mipe_state_0068f100->monster_ids, 0));
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
            float real_elapsed;
            float frame_elapsed;
            HandlePartyMovement005A1EB0(&real_elapsed, &frame_elapsed);
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
            UpdateMonsterSight();
            g_level_block->character_update_timer = SetCountdownClock(500);
        }
        if (!g_flag_006840bc) {
            UpdateMonsterGroups(1);
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
            RefreshMonsterTargetCounts005398D0();
            g_level_block->combat_panel_timer = SetCountdownClock(500);
            g_level_block->refresh_combat_panel = 0;
        }
        if (gXStatus.iTargetingMode == 4) {
            RefreshSpellTargetHighlightsAtRange();
        } else if (gXStatus.iTargetingMode == 3 && IsWorldCursorVisible()) {
            UpdateTargetMarkerHighlight0053B1D0();
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
            CommitSpellCastingSelection005A0BC0();
        if (gXStatus.fItemSelectMode)
            Function59D180();
        if (gXStatus.fNpcDialogueMode)
            Function56E510();
        if (gXStatus.fLockInteractMode)
            ProcessLockInteractMode();
        if (gXStatus.fTrapInteractMode)
            UpdateMainGameScreen();
        int active;
        if (!AnyPropTriggerInView00445140(g_world) && !AnyMonsterVisible0053A1D0() &&
            !Function4F8650() && !Function57E3C0() && !SelectWorldCursorNode0048EFC0()) {
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
        if (g_level_block->highlight_graphic != 0) {
            ReleaseObject004257F0(g_level_block->highlight_graphic);
            g_level_block->highlight_graphic = 0;
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
        CloseSpellCastingView0059F2B0();
    if (gXStatus.fItemSelectMode)
        Function59C9C0();
    if (gXStatus.fReviewCharacterMode)
        CloseFormationPanel();
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
        ToggleMipePanel0057D740();
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

    if (g_level_block->formation_board_visible) {
        g_level_block->formation_board_visible = 0;
        RegionSetDisable(0x13);
        ReleaseFormationBoard();
        if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
            g_level_block->redraw_flags |= 0x8200;
        }
        if (g_flag_0068edc9) {
            unsigned short mode;
            if (!IsScreenInputBlocked() && !g_level_block->flag_155 &&
                g_level_block->formation_board_visible && g_level_block->flag_157 &&
                g_settings_6850c8.field_006 == 0) {
                mode = 4;
            } else if (!IsScreenInputBlocked() &&
                       (!g_level_block->formation_board_visible || !g_level_block->flag_157 ||
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
        EnableRadarMap(0);
        ReleaseRadarMap();
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

// FUNCTION: WIZ8 0x00561a20
void RefreshSelectedPartyPortrait(unsigned int party_slot)
{
    if (g_level_block == 0 || g_level_block->value_0fc == 0 ||
        g_level_block->portrait_refresh_pending[party_slot] != 0) {
        return;
    }
    if (g_level_block->flag_314 != 0 && party_slot == static_cast<unsigned int>(g_value_64c1c8) &&
        gXStatus.monster_manager_entries[party_slot].field_0bd == 0 &&
        gXStatus.monster_manager_entries[party_slot].field_09c == 0) {
        if (gXStatus.monster_manager_entries[party_slot].quote.quote_handle == -1 ||
            g_settings_6850c8.pc_subtitles == 0) {
            return;
        }
    }

    if (g_level_block->values_200[3] == static_cast<int>(party_slot)) {
        g_level_block->values_200[3] = -1;
        if (g_level_block->highlight_graphic != 0) {
            ReleaseObject004257F0(g_level_block->highlight_graphic);
            g_level_block->highlight_graphic = 0;
        }
        Function563DD0();
        g_main_game_mode_0068eddc = 0;
        RequestRedraw(0x8000 | 0xff);
    } else if (g_level_block->values_200[3] != -1) {
        RequestRedraw(0x8000);
    }

    if (g_level_block->values_200[0] == static_cast<int>(party_slot)) {
        g_level_block->values_200[0] = -1;
        if (g_level_block->highlight_graphic != 0) {
            ReleaseObject004257F0(g_level_block->highlight_graphic);
            g_level_block->highlight_graphic = 0;
        }
        Function563DD0();
        g_main_game_mode_0068eddc = 0;
        RequestRedraw(0x8000 | 0xff);
    } else if (g_level_block->values_200[0] != -1) {
        RequestRedraw(0x8000);
    }

    if (g_main_game_mode_0068eddc == 3) {
        if (gXStatus.unknown_026[0] != 0) {
            Function56E800(0);
        }
    } else if (g_main_game_mode_0068eddc == 5) {
        Function5187E0();
    } else if (g_main_game_mode_0068eddc == 6) {
        if (g_level_block->highlight_graphic != 0) {
            ReleaseObject004257F0(g_level_block->highlight_graphic);
            g_level_block->highlight_graphic = 0;
        }
        ClearSurfaceRect(g_level_block->dialogue_x_220, g_level_block->dialogue_y_224,
                         g_level_block->dialogue_x_220 + g_level_block->dialogue_width_238,
                         g_level_block->dialogue_y_224 + g_level_block->dialogue_height_228);
        InvalidateRegion(g_level_block->dialogue_x_220, g_level_block->dialogue_y_224,
                         g_level_block->dialogue_x_220 + g_level_block->dialogue_width_238,
                         g_level_block->dialogue_y_224 + g_level_block->dialogue_height_228, 0);
    }

    g_main_game_mode_0068eddc = 4;
    g_level_block->highlight_override = -1;
    g_level_block->portrait_refresh_pending[party_slot] = 1;
    g_level_block->flag_108 = 1;
    g_level_block->values_114[party_slot] = 0x69;
    g_level_block->values_134[party_slot] = 6;
    RequestRedraw(1u << (party_slot & 0x1f));
    DisableRegionInput(party_slot + 0x5a);
    if (gXStatus.monster_manager_entries[party_slot].field_0d0 == 0) {
        RegionSetEnable(party_slot + 7);
        EnableRegionSetInput(party_slot + 7);
    }
}

/* Take the mode-6 hover overlay down: the portrait highlight graphic is
   released, the dialogue-area rectangle it drew over is cleared and
   invalidated, the rectangle's position against the viewport decides the
   extra redraw flags, and the mode resets to the default. */
// FUNCTION: WIZ8 0x00563EB0
void DismissHighlightOverlay(void)
{
    if (g_level_block->highlight_graphic != 0) {
        ReleaseObject004257F0(g_level_block->highlight_graphic);
        g_level_block->highlight_graphic = 0;
    }
    if (g_main_game_mode_0068eddc == 6) {
        ClearSurfaceRect(g_level_block->dialogue_x_220, g_level_block->dialogue_y_224,
                         g_level_block->dialogue_x_220 + g_level_block->dialogue_width_238,
                         g_level_block->dialogue_y_224 + g_level_block->dialogue_height_228);
        InvalidateRegion(g_level_block->dialogue_x_220, g_level_block->dialogue_y_224,
                         g_level_block->dialogue_x_220 + g_level_block->dialogue_width_238,
                         g_level_block->dialogue_y_224 + g_level_block->dialogue_height_228, 0);
        if (g_level_block->dialogue_y_224 <
                static_cast<unsigned int>(
                    g_viewport_modes_647d30[g_level_block->camera_mode_100].top) &&
            g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
            g_level_block->redraw_flags |= 0x100;
        }
        if (g_level_block->dialogue_y_224 + g_level_block->dialogue_height_228 > 0x166 &&
            g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
            g_level_block->redraw_flags |= 0x800;
        }
    }
    g_main_game_mode_0068eddc = 0;
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
    if (gXStatus.fNpcDialogueMode != 0 && !CanOpenNpcDialogue()) {
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
        CloseSpellCastingView0059F2B0();
    }
    if (gXStatus.fItemSelectMode != 0) {
        Function59CAC0();
    }
    if (gXStatus.fReviewCharacterMode != 0) {
        CloseFormationPanel();
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
void ForwardNpcScriptNotice(W8NpcState* npc, W8ItemInstance* item, int line, int suppress)
{
    if (gXStatus.fNpcDialogueMode == 0 && gXStatus.fCombatMode == 0 &&
        (npc->record->kind != 7 || GetFact(0x1c) != 1)) {
        Function56C5E0(npc, item, line, suppress, 0);
    }
}

/* Queue one NPC script notice for the dispatch pass. A second NPC of the same
   kind already in the world suppresses the notice unless the record carries
   the 0x054 binding flag, a live monster with a condition at or above 0xf
   takes none, and a pending notice blocks the next until it drains. Kind
   0x10/0x11 NPCs with fact 0xbf substitute their own notice line and raise
   the flag byte. */
// FUNCTION: WIZ8 0x0056C5E0
void Function56C5E0(W8NpcState* npc, W8ItemInstance* item, int line, int suppress, int arg)
{
    W8MonsterInfo* info;
    unsigned char flag;

    if (FindNpcOfKind(npc->name_style) != 0 && npc->record->unknown_054 == 0) {
        return;
    }
    info = GetNpcMonsterInfo(npc);
    if (info != 0 && info->highest_condition >= 0xf) {
        return;
    }
    flag = static_cast<unsigned char>(suppress);
    if ((npc->name_style == 0x10 || npc->name_style == 0x11) && GetFact(0xbf) != 0) {
        flag = 1;
        line = npc->name_style == 0x10 ? 0x23 : 0x1d;
    }
    if (g_flag_68f0f9 != 0) {
        return;
    }
    g_pending_notice_68ee60.bytes.flag = flag;
    g_pending_notice_68ee60.npc = npc;
    g_pending_notice_68ee60.line = line;
    g_pending_notice_68ee60.bytes.force = static_cast<unsigned char>(arg);
    if (item != 0) {
        g_pending_notice_68ee60.item = *item;
    } else {
        EmptyItemRecord(&g_pending_notice_68ee60.item, 0, 1);
    }
    Function5289B0(0x37, 0);
    g_flag_68f0f9 = 1;
}

/* The NPC notice and dialogue dispatcher. Talking to a healer NPC (name
   styles 0x0f and 0x12) first lifts every active condition off the two party
   rows bound to RPC NPCs (name styles 0x10 and 0x11) whose condition set has
   reached 0x0f; clearing condition 0x12 - death - leaves them on ten hit
   points. A completed character event is finished off, the dialogue NPC is
   staged outside camp mode, and then the record's 0x054/0x2ea flags choose
   between the item/quote branch and the plain quote branch; both end by
   pausing the world, raising the dialogue flags and pointing the camera at
   the NPC's monster. */
// FUNCTION: WIZ8 0x0056C6D0
void Function56C6D0(W8NpcState* npc, W8ItemInstance* item, int quote, int flags, int force)
{
    W8MainScreenState* state;
    W8MonsterInfo* info;
    W8NpcState* bound;
    W8NpcState* selected;
    W8Character* characters;
    int slot;
    int condition;
    srVector3T<float> position;

    g_flag_68f0f9 = 0;
    g_screen_state_00649f1c->value_25c = 0;
    if (npc->name_style == 0xf || npc->name_style == 0x12) {
        characters = g_status_685170.buffers.characters;
        for (slot = 0; slot < 2; ++slot) {
            if (g_status_685170.buffers.party_rows[slot].occupied != 0) {
                bound = GetNpcState(g_status_685170.buffers.party_rows[slot].animation_0fa);
                if ((bound->name_style == 0x11 || bound->name_style == 0x10) &&
                    characters[slot].highest_condition >= 0xf) {
                    for (condition = 0; condition <= 0x12; ++condition) {
                        if (characters[slot].condition_turns[condition] != 0) {
                            RemoveCharacterCondition(slot, condition, 0);
                            if (condition == 0x12) {
                                characters[slot].hp_current = 10;
                            }
                        }
                    }
                }
            }
        }
    }
    if (gXStatus.character_event_queue->HasActiveEvents() != 0) {
        gXStatus.character_event_queue->CompleteFirstActiveEvent();
    }
    if (gXStatus.fCampMode == 0) {
        Function56D030(npc, flags);
    }
    if (force != 0) {
        Function56CAD0(npc, item, 1);
        return;
    }
    if (npc->record->unknown_054 == 0 && npc->record->flag_2ea == 0) {
        if (GetNpcDispositionBand(npc) == 2 && npc->record->unknown_056 == 0) {
            Function528830(0x18, 0, 0, 0);
            return;
        }
        if (flags == 0) {
            Function56CAD0(npc, item, 0);
            return;
        }
    }
    if (npc->record->flag_2ea != 0) {
        if (item != 0) {
            state = g_screen_state_00649f1c;
            state->pending_item_1ed = *item;
            if (g_status_685170.item_in_cursor != 0) {
                state->flag_1f9 = 1;
            }
            Function575810(&state->pending_item_1ed);
        } else if (quote == -1) {
            Function577290(1);
        } else {
            Function528830(quote, 0, 0, 0);
        }
    } else {
        if (quote == -1) {
            quote = 0;
        }
        Function528830(quote, 0, 0, 0);
    }
    selected = g_screen_state_00649f1c->dialogue_npc;
    if (selected->name_style != 0x84 && selected->name_style != 0x85) {
        info = GetNpcMonsterInfo(selected);
        if (info != 0) {
            MonsterForwardReferencePosition(info->monster, 0);
        }
    }
    PauseMainGameWorld();
    state = g_screen_state_00649f1c;
    state->flag_252 = 1;
    gXStatus.fNpcDialogueMode = 1;
    state->value_25c = 0;
    info = GetNpcMonsterInfo(state->dialogue_npc);
    if (info == 0) {
        return;
    }
    position = info->monster->movement_0c0.position_040;
    position.y += info->monster->movement_0c0.height_offset_0b8;
    g_gd_camera_65a0f8->LookAt(&position, 0);
}

// FUNCTION: WIZ8 0x0056CA60
void Function56CA60(W8NpcState* npc, W8ItemInstance* item, int quote, int flags, int force)
{
    Function56C6D0(npc, item, quote, flags, force);
}

/* Dispatch the queued NPC script notice: the item goes across only while it
   still carries an id, and the flag pair at +0x14 travels as one dword. */
// FUNCTION: WIZ8 0x0056CA90
void Function56CA90(void)
{
    W8ItemInstance* item;

    item = 0;
    if (g_pending_notice_68ee60.item.item_id != -1) {
        item = &g_pending_notice_68ee60.item;
    }
    Function56C6D0(g_pending_notice_68ee60.npc, item, g_pending_notice_68ee60.line,
                   g_pending_notice_68ee60.flags, g_pending_notice_68ee60.bytes.force);
}

/* Open the NPC dialogue panel. After the shared screen reset and the
   dialogue-UI build, a carried item goes through the pending-item path -
   inspecting it decides between the trade switch and a disposition check -
   while everything else falls to the force gate and then the
   dispatch: a record-0x056 NPC takes the plain quote, otherwise the
   disposition band picks the hostile or friendly entry. */
// FUNCTION: WIZ8 0x0056CAD0
unsigned char Function56CAD0(W8NpcState* npc, W8ItemInstance* item, unsigned char force)
{
    W8MainScreenState* state;
    W8MonsterInfo* info;
    W8MonsterInfo* dialogue_info;
    unsigned char band;
    wchar_t space[2];
    srVector3T<float> position;

    UpdateScreenOverlays(0);
    gXStatus.fNpcDialogueMode = 1;
    Function569570();
    if (npc->record->flag_055 != 0) {
        Function55BCC0(npc);
    }
    state = g_screen_state_00649f1c;
    state->value_fc = 0;
    if (gXStatus.fCampMode == 0) {
        state->value_104 = 0;
        state->value_100 = 0;
    }
    state->dialogue_panel_hidden = 0;
    state->flag_252 = 0;
    state->value_108 = 0;
    state->value_1c4 = -1;
    state->value_1c8 = -1;
    state->value_1cc = -1;
    state->value_1d0 = 0;
    state->value_000 = 0;
    state->flag_1d9 = 0;
    state->flag_1f9 = 0;
    state->script_busy = 0;
    state->flag_200 = 0;
    state->flag_201 = 0;
    state->dialogue_cursor_flag = 0;
    state->flag_250 = 0;
    state->unknown_251 = 0;
    state->flag_229 = 0;
    state->value_22c = 0;
    state->value_238 = 0;
    state->flag_23c = 1;
    state->value_258 = -1;
    state->value_25c = 0;
    if (g_settings_6850c8.field_006 != 0) {
        ApplyMainGameModeFlag(0, 0);
    } else {
        SetViewportMode(Function5698C0());
    }
    if (gXStatus.fCampMode == 0) {
        g_screen_state_00649f1c->value_f0 = g_settings_6850c8.field_006;
    }
    Function56D1D0();
    SetRegionBounds(0x8a, 0x17, 0x166, 0x269, 0x1c2);
    g_level_block->flag_271 = 0;
    RegionSetEnable(0x15);
    EnableRegionInput(0x52);
    EnableRegionInput(0x53);
    EnableRegionInput(0x54);
    EnableRegionInput(0x55);
    g_level_block->flag_155 = 1;
    gXStatus.fCampMode = 0;
    if (item != 0) {
        state = g_screen_state_00649f1c;
        state->pending_item_1ed = *item;
        if (g_status_685170.item_in_cursor != 0) {
            state->flag_1f9 = 1;
            ClearHeldItemDisplay();
        }
        if (npc->record->unknown_056 != 0) {
            if (Function575810(&state->pending_item_1ed) == 0) {
                Function570CF0();
                goto dispatch;
            }
        } else if (Function575810(&state->pending_item_1ed) == 0) {
            switch (g_screen_state_00649f1c->value_fc) {
            case 1:
                Function573DD0();
                break;
            case 2:
                RegionSetDisable(0x18);
                g_screen_state_00649f1c->panel_1a8->SetEnabled(0);
                g_screen_state_00649f1c->panel_1ac->SetEnabled(0);
                g_screen_state_00649f1c->panel_1b8->SetEnabled(0);
                g_screen_state_00649f1c->dialogue_text_1a4->SetActive(0);
                g_screen_state_00649f1c->dialogue_text_1a4->m_textBuffer.SetText(
                    &g_wchar_00689b34, g_wiz_text_bold_font_683664);
            /* fall through */
            case 6:
                Function56EDD0(0);
                break;
            case 3:
                Function571370();
                break;
            case 4:
                Function572320();
                break;
            case 5:
                Function573570();
                break;
            }
            if (GetNpcDispositionBand(g_screen_state_00649f1c->dialogue_npc) == 0) {
                Function570CF0();
            } else {
                Function570760();
            }
            goto dispatch;
        }
    }
    if (force != 0) {
        goto tail;
    }
dispatch:
    if (g_screen_state_00649f1c->dialogue_npc->record->unknown_056 != 0) {
        Function528830(0, 0, 0, 0);
        Function570CF0();
    } else {
        band = GetNpcDispositionBand(g_screen_state_00649f1c->dialogue_npc);
        if (g_screen_state_00649f1c->dialogue_npc->name_style == 0xf && GetFact(0x2f1) != 0 &&
            GetFact(0x3c) == 0) {
            SetFact(0x2f1, 0, 0);
        }
        if (band == 0) {
            Function577290(1);
            Function570CF0();
        } else if (band == 1) {
            Function528830(2, 0, 0, 0);
            Function570760();
        }
    }
tail:
    RequestRedraw(0x100);
    RequestRedraw(0x1000);
    if (g_flag_0068edd8 != 0) {
        SetFlag603C60();
        g_flag_0068edd8 = 0;
        g_flag_006f04ec = 0;
    }
    info = GetNpcMonsterInfo(npc);
    if (info != 0) {
        g_screen_state_00649f1c->flag_23d = 1;
        g_screen_state_00649f1c->saved_camera_pitch_240 = g_gd_camera_65a0f8->m_pitch;
        g_screen_state_00649f1c->saved_camera_yaw_244 = g_gd_camera_65a0f8->m_yaw;
        dialogue_info = GetNpcMonsterInfo(g_screen_state_00649f1c->dialogue_npc);
        if (dialogue_info != 0) {
            position = dialogue_info->monster->movement_0c0.position_040;
            position.y += dialogue_info->monster->movement_0c0.height_offset_0b8;
            g_gd_camera_65a0f8->LookAt(&position, 0);
        }
        MonsterForwardReferencePosition(info->monster, 0);
    }
    PauseMainGameWorld();
    RequestRedraw(0x200);
    g_screen_state_00649f1c->flag_261 = 1;
    swprintf(space, L" ");
    Function58AC00(5, space, 3, -1, 0);
    return 1;
}

/* Stage `npc` as the dialogue NPC: bind its monster's location, clear its
   transient flags, kick the script dialogue, refresh the name caption while
   the dialogue UI is already up, and pick the speaking character - the first
   occupied row, overtaken by any occupied row with a higher skill-0x16
   (communication) level. Every occupied portrait then takes target pose 1.
   A stale disposition snapshot on the NPC drops its 0x1c flag. */
// FUNCTION: WIZ8 0x0056D030
void Function56D030(W8NpcState* npc, int flags)
{
    W8MainScreenState* state;
    W8MonsterInfo* info;
    W8NpcState* selected;
    int slot;
    int speaker;
    unsigned int best;

    state = g_screen_state_00649f1c;
    speaker = -1;
    state->target_location_id_f8 = -1;
    best = 0xffffffff;
    state->dialogue_npc = 0;
    info = GetNpcMonsterInfo(npc);
    if (info != 0) {
        state->target_location_id_f8 = info->location_id;
    }
    state->dialogue_npc = npc;
    state->dialogue_npc->flag_22 = 0;
    state->dialogue_npc->flag_23 = 0;
    if (state->dialogue_npc->unknown_1d != 0) {
        state->dialogue_npc->flag_84 = 0;
    }
    BeginNpcScriptDialogue(state->dialogue_npc, 0);
    if ((state->value_fc == 3 || state->value_fc == 2) && gXStatus.fNpcDialogueMode != 0) {
        state->dialogue_text_10c->m_textBuffer.SetText(state->dialogue_npc->record->source_name_004,
                                                       g_wiz_text_bold_font_683664);
        state->dialogue_text_10c->Invalidate(1);
    }
    state->flag_23d = 0;
    for (slot = 0; slot < 8; ++slot) {
        if (g_status_685170.buffers.party_rows[slot].occupied != 0) {
            if (speaker == -1) {
                speaker = slot;
            }
            if (g_status_685170.buffers.characters[slot].skills[0x16].level > best) {
                best = g_status_685170.buffers.characters[slot].skills[0x16].level;
                speaker = slot;
            }
        }
    }
    g_screen_state_00649f1c->dialogue_speaker = speaker;
    for (slot = 0; slot < 8; ++slot) {
        if (g_status_685170.buffers.party_rows[slot].occupied != 0) {
            SetPortraitTargetPose(&gXStatus.monster_manager_entries[slot], 1);
        }
    }
    selected = g_screen_state_00649f1c->dialogue_npc;
    if (selected->unknown_1c != 0 && selected->unknown_ef[0] != GetNpcDispositionBand(selected)) {
        selected->unknown_1c = 0;
    }
}

void Function573660(void);
void Function573730(void);
void Function573800(void);
void Function5738D0(void);
void Function5739A0(void);
void Function573A10(void);
void Function570000(void);
void Function575B00(void);
void Function575B40(void);
void Function570AD0(void);
void Function570B80(void);
void Function570C20(void);
void Function570530(void);
void Function5705B0(void);
void Function570310(void);
void Function575520(W8DialogBase* dialog);

/* Build the NPC dialogue UI: the option-button panel on the left, the
   secondary panel beside it, the scrolling text controller, the three
   right-hand panels and the text-input panel, then every text control each
   of them hosts. Buttons get their option masks, help ids and activation
   callbacks as they are created. */
// FUNCTION: WIZ8 0x0056D1D0
void Function56D1D0(void)
{
    Controls* panel;
    W8MainScreenState* state;

    state = g_screen_state_00649f1c;
    state->panel_1a8 = new W8MainGamePanel005EE9F0(0x17, 0x166, 0xa4, 0x1c2, 0x1a9, 0, 0);
    state->panel_1ac = new Controls(0xa4, 0x166, 0x1dc, 0x1c2, 0x1a9, 0, 1);
    state->npc_dialogue_controller_1b0 =
        new W8NpcDialogueTextController(0x1dc, 0x11b, 0x269, 0x140, 0x1a9, 0, 2, 4, 3);
    state->npc_dialogue_panel_1b4 = new Controls(0x1dc, 0x12f, 0x269, 0x1c2, 0x1a9, 0, 5);
    state->panel_1b8 = new Controls(0x1dc, 0x166, 0x269, 0x1c2, 0x1a9, 0, 7);
    state->panel_1bc = new Controls(0x1dc, 0x166, 0x238, 499, 0x1a9, 0, 6);
    state->text_input_panel_1c0 =
        new W8MainGamePanel005EE9E4(0x1dc, 0x166, 0x269, 0x1c0, 0x1a9, 0, 0xd);

    panel = state->panel_1a8;
    state->dialogue_text_10c =
        new W8TextControl(panel, 0xffffffff, 5, 2, 0x89, 0x12, -1, -1, -1, -1, -1, -1, -1);
    state->dialogue_text_10c->m_textBuffer.SetLayoutMode(g_W8TextBufferLayoutMask005ED554 |
                                                         g_W8TextBufferLayoutMask005ED54C);
    state->dialogue_text_1a4 =
        new W8TextControl(panel, 0xffffffff, 5, 0x47, 0x89, 0x57, -1, -1, -1, -1, -1, -1, -1);
    state->dialogue_text_1a4->m_textBuffer.SetLayoutMode(g_W8TextBufferLayoutMask005ED554 |
                                                         g_W8TextBufferLayoutMask005ED54C);
    state->dialogue_text_110 =
        new W8TextControl(panel, 0x82, 2, 0x11, 0x45, 0x21, 0x1a9, 0, -1, 0xc, 10, 0xb, -1);
    state->dialogue_text_114 =
        new W8TextControl(panel, 0x83, 0x44, 0x11, 0x87, 0x21, 0x1a9, 0, -1, 0xc, 10, 0xb, -1);
    state->dialogue_text_118 =
        new W8TextControl(panel, 0x84, 2, 0x21, 0x45, 0x31, 0x1a9, 0, -1, 0xc, 10, 0xb, -1);
    state->dialogue_text_11c =
        new W8TextControl(panel, 0x85, 0x44, 0x21, 0x87, 0x31, 0x1a9, 0, -1, 0xc, 10, 0xb, -1);
    state->dialogue_text_120 =
        new W8TextControl(panel, 0x86, 2, 0x31, 0x45, 0x41, 0x1a9, 0, -1, 0xc, 10, 0xb, -1);
    state->dialogue_text_124 =
        new W8TextControl(panel, 0x87, 0x44, 0x31, 0x87, 0x41, 0x1a9, 0, -1, 0xc, 10, 0xb, -1);
    state->dialogue_text_124->UpdateTextBounds(0x46, 0x31, 0x89, 0x41);
    state->dialogue_text_128 =
        new W8TextControl(panel, 0x88, 0x72, 0x47, 0x89, 0x57, 0x1aa, 0, 0, 4, 1, 2, 3);
    state->option_buttons_170[0] =
        new W8TextControl(panel, 0x75, 7, 0x46, 0x17, 0x56, 0x1ab, 0, 0, 1, 2, 4, 3);
    state->option_buttons_170[1] =
        new W8TextControl(panel, 0x76, 0x19, 0x46, 0x29, 0x56, 0x1ab, 0, 5, 6, 7, 9, 8);
    state->option_buttons_170[2] =
        new W8TextControl(panel, 0x77, 0x2b, 0x46, 0x3b, 0x56, 0x1ab, 0, 10, 0xb, 0xc, 0xe, 0xd);
    state->option_buttons_170[3] = new W8TextControl(panel, 0x78, 0x3e, 0x46, 0x4e, 0x56, 0x1ab, 0,
                                                     0xf, 0x10, 0x11, 0x13, 0x12);
    state->option_buttons_170[4] = new W8TextControl(panel, 0x79, 0x50, 0x46, 0x60, 0x56, 0x1ab, 0,
                                                     0x14, 0x15, 0x16, 0x18, 0x17);
    state->option_buttons_170[5] = new W8TextControl(panel, 0x7a, 0x62, 0x46, 0x72, 0x56, 0x1ab, 0,
                                                     0x19, 0x1a, 0x1b, 0x1d, 0x1c);
    state->option_buttons_170[0]->AddLayoutFlags(g_W8TextControlMask005ED578);
    state->option_buttons_170[0]->EnableRegionHelp(0x7c2);
    state->option_buttons_170[1]->AddLayoutFlags(g_W8TextControlMask005ED578);
    state->option_buttons_170[1]->EnableRegionHelp(0x7c3);
    state->option_buttons_170[2]->AddLayoutFlags(g_W8TextControlMask005ED578);
    state->option_buttons_170[2]->EnableRegionHelp(0x7c4);
    state->option_buttons_170[3]->AddLayoutFlags(g_W8TextControlMask005ED578);
    state->option_buttons_170[3]->EnableRegionHelp(0x7c5);
    state->option_buttons_170[4]->AddLayoutFlags(g_W8TextControlMask005ED578);
    state->option_buttons_170[4]->EnableRegionHelp(0x7c6);
    state->option_buttons_170[5]->AddLayoutFlags(g_W8TextControlMask005ED578);
    state->option_buttons_170[5]->EnableRegionHelp(0x7c7);
    state->option_buttons_170[0]->m_primaryActivationCallback = Function573660;
    state->option_buttons_170[1]->m_primaryActivationCallback = Function573730;
    state->option_buttons_170[2]->m_primaryActivationCallback = Function5739A0;
    state->option_buttons_170[3]->m_primaryActivationCallback = Function573800;
    state->option_buttons_170[4]->m_primaryActivationCallback = Function5738D0;
    state->option_buttons_170[5]->m_primaryActivationCallback = Function573A10;

    panel = state->panel_1ac;
    state->dialogue_widget_12c = new W8Widget(panel, 0x89, 2, 4, 0x136, 0x57);

    panel = state->npc_dialogue_controller_1b0;
    state->dialogue_scroll_130 = new W8NpcDialogueScrollWidget(panel, 0x65, 6, 6, 0x7c, 0x11);
    state->dialogue_widget_134 =
        new W8TextControl(panel, 0x66, 0x7e, 3, 0x88, 0xb, 0x1aa, 0, 0x14, 0x18, 0x15, 0x16, 0x17);
    state->dialogue_widget_138 = new W8TextControl(panel, 0x67, 0x7e, 0xc, 0x88, 0x14, 0x1aa, 0,
                                                   0x19, 0x1d, 0x1a, 0x1b, 0x1c);

    panel = state->text_input_panel_1c0;
    state->dialogue_text_13c =
        new W8TextControl(panel, 0x68, 0x11, 0x23, 0x22, 0x30, 0x1aa, 0, 5, 9, 6, 7, 8);
    state->dialogue_text_13c->EnableRegionHelp(100);
    state->dialogue_text_140 =
        new W8TextControl(panel, 0x69, 0x26, 0x23, 0x37, 0x30, 0x1aa, 0, 10, 0xe, 0xb, 0xc, 0xd);
    state->dialogue_text_140->EnableRegionHelp(0x65);
    state->dialogue_text_168 =
        new W8TextControl(panel, 0x73, 9, 0x33, 0x84, 0x44, 0x1a9, 0, -1, -1, 0xe, 0xf, -1);
    state->dialogue_text_168->m_textBuffer.SetText(gppStringList[0x741], g_font_683660);
    state->dialogue_text_168->m_primaryActivationCallback = Function575B40;
    state->dialogue_text_168->EnableRegionHelp(0x66);
    state->dialogue_text_164 =
        new W8TextControl(panel, 0x72, 9, 0x43, 0x84, 0x52, 0x1a9, 0, -1, -1, 0xe, 0xf, -1);
    state->dialogue_text_164->m_textBuffer.SetText(gppStringList[0x742], g_font_683660);
    state->dialogue_text_164->m_primaryActivationCallback = Function575B00;
    state->dialogue_text_164->EnableRegionHelp(0x67);

    panel = state->npc_dialogue_panel_1b4;
    state->dialogue_text_148 =
        new W8TextControl(panel, 0x6b, 6, 2, 0x89, 0xe, 0x1a9, 0, 8, 9, -1, -1, -1);
    state->dialogue_text_148->AddLayoutFlags(
        g_W8TextControlMask005ED580 | g_W8TextControlMask005ED57C | g_W8TextControlMask005ED578);
    state->dialogue_text_150 =
        new W8TextControl(panel, 0x6d, 6, 0xf, 0x41, 0x1b, 0x1a9, 0, 8, 9, -1, -1, -1);
    state->dialogue_text_150->AddLayoutFlags(
        g_W8TextControlMask005ED584 | g_W8TextControlMask005ED580 | g_W8TextControlMask005ED57C |
        g_W8TextControlMask005ED578);
    state->dialogue_text_154 =
        new W8TextControl(panel, 0x6e, 0x43, 0xf, 0x89, 0x1b, 0x1a9, 0, 8, 9, -1, -1, -1);
    state->dialogue_text_154->AddLayoutFlags(
        g_W8TextControlMask005ED584 | g_W8TextControlMask005ED580 | g_W8TextControlMask005ED57C |
        g_W8TextControlMask005ED578);
    state->dialogue_text_158 =
        new W8TextControl(panel, 0x6f, 6, 0x1b, 0x41, 0x27, 0x1a9, 0, 8, 9, -1, -1, -1);
    state->dialogue_text_158->AddLayoutFlags(
        g_W8TextControlMask005ED584 | g_W8TextControlMask005ED580 | g_W8TextControlMask005ED57C |
        g_W8TextControlMask005ED578);
    state->dialogue_text_15c =
        new W8TextControl(panel, 0x70, 0x43, 0x1b, 0x89, 0x27, 0x1a9, 0, 8, 9, -1, -1, -1);
    state->dialogue_text_15c->AddLayoutFlags(
        g_W8TextControlMask005ED584 | g_W8TextControlMask005ED580 | g_W8TextControlMask005ED57C |
        g_W8TextControlMask005ED578);
    state->dialogue_text_160 =
        new W8TextControl(panel, 0x71, 6, 0x27, 0x41, 0x33, 0x1a9, 0, 8, 9, -1, -1, -1);
    state->dialogue_text_160->AddLayoutFlags(
        g_W8TextControlMask005ED584 | g_W8TextControlMask005ED580 | g_W8TextControlMask005ED57C |
        g_W8TextControlMask005ED578);

    panel = state->panel_1bc;
    state->dialogue_text_16c =
        new W8TextControl(panel, 0x74, 5, 0x14, 0x32, 0x49, -1, -1, -1, -1, -1, -1, -1);
    state->dialogue_text_16c->m_textBuffer.SetLayoutMode(g_W8TextBufferLayoutMask005ED55C |
                                                         g_W8TextBufferLayoutMask005ED550);
    state->dialogue_text_16c->AddLayoutFlags(g_W8TextControlMask005ED594);
    state->dialogue_text_188 = new W8TextControl(panel, 0x7b, 0x35, 0x36, 0x51, 0x46, 0x1aa, 0,
                                                 0x1e, 0x22, 0x1f, 0x20, 0x21);
    state->dialogue_text_188->EnableRegionHelp(0x7c9);
    state->dialogue_text_190 =
        new W8TextControl(panel, 0x7d, 0x53, 0x4d, 0x86, 0x58, -1, -1, -1, -1, -1, -1, -1);
    state->dialogue_text_190->m_textBuffer.SetLayoutMode(g_W8TextBufferLayoutMask005ED550 |
                                                         g_W8TextBufferLayoutMask005ED554);
    state->dialogue_text_190->SetEnabled(0);
    state->dialogue_text_194 =
        new W8TextControl(panel, 0x7e, 5, 0x4d, 0x51, 0x58, -1, -1, -1, -1, -1, -1, -1);
    state->dialogue_text_194->m_textBuffer.SetLayoutMode(g_W8TextBufferLayoutMask005ED554 |
                                                         g_W8TextBufferLayoutMask005ED54C);
    state->dialogue_text_194->SetEnabled(0);
    state->dialogue_text_198 =
        new W8TextControl(panel, 0x7f, 0x53, 0x3a, 0x86, 0x45, -1, -1, -1, -1, -1, -1, -1);
    state->dialogue_text_198->m_textBuffer.SetLayoutMode(g_W8TextBufferLayoutMask005ED550 |
                                                         g_W8TextBufferLayoutMask005ED554);
    state->dialogue_text_198->SetEnabled(0);
    state->dialogue_text_19c =
        new W8TextControl(panel, 0x80, 4, 4, 0x88, 0x11, -1, -1, -1, -1, -1, -1, -1);
    state->dialogue_text_19c->m_textBuffer.SetLayoutMode(g_W8TextBufferLayoutMask005ED554 |
                                                         g_W8TextBufferLayoutMask005ED54C);
    state->dialogue_text_19c->SetEnabled(0);
    state->dialogue_text_1a0 =
        new W8TextControl(panel, 0x81, 0x38, 0x22, 0x88, 0x34, -1, -1, -1, -1, -1, -1, -1);
    state->dialogue_text_128->m_primaryActivationCallback = Function570000;
    state->dialogue_text_128->EnableRegionHelp(0x7c8);
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
                UpdatePartyMovementPanel005A1950();
            }
            ClearSurfaceRect(0xb1, 0x13f, 0x1cf, 0x153);
            InvalidateRegion(0xb1, 0x13f, 0x1cf, 0x153, 0);
            if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
                g_level_block->redraw_flags |= 0x8000;
            }
        }
    }
}

/* Assay dialog destroy callback: closing the dialog leaves the main game
   screen fully dirty so every region repaints. */
// FUNCTION: WIZ8 0x005670a0
void InvalidateMainGameScreen005670A0(W8DialogBase* dialog)
{
    if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
        g_level_block->redraw_flags = 0xffffffff;
    }
}

/* Open the assay dialog over the main game screen. A live modal dialog is
   parked in the pending slot so it resumes when this one closes. */
// FUNCTION: WIZ8 0x0056ae20
void OpenAssayDialog0056AE20(W8ItemInstance* item, int character_slot)
{
    W8DialogBase* dialog;

    if (g_modal_owner_0068edd0 != 0) {
        g_pending_main_game_dialog_0068edd4 = g_modal_owner_0068edd0;
        g_modal_owner_0068edd0 = 0;
    }
    if (character_slot != -1) {
        dialog = new W8AssayDialog(item, &g_status_685170.buffers.characters[character_slot]);
    } else {
        dialog = new W8AssayDialog(item, 0);
    }
    dialog->SetText(&g_wchar_00689b34);
    dialog->SetOrigin(g_info_dialog_x_005ef958, 0x48);
    dialog->m_destroy_callback = InvalidateMainGameScreen005670A0;
    g_modal_owner_0068edd0 = dialog;
    ActivateDialogRegion(0x138);
}

// FUNCTION: WIZ8 0x00576030
void SetNpcQuoteBubbleVisible(bool visible, const wchar_t* text, W8NpcScriptQuote* quote,
                              int quote_id, unsigned int font_palette)
{
    SetNpcQuoteBubbleVisible(visible, text, quote, quote_id, font_palette, 0, 0, -1);
}

// FUNCTION: WIZ8 0x00576060
void SetNpcQuoteBubbleVisible(bool visible, const wchar_t* text, W8NpcScriptQuote* quote,
                              int quote_id, unsigned int font_palette, unsigned char notice_kind,
                              void* payload, int npc_kind)
{
    if (visible == g_screen_state_00649f1c->quote_visible) {
        return;
    }
    if (visible) {
        wchar_t normalized[2048];
        wchar_t error_text[200];
        unsigned short width;
        unsigned short height;

        if (g_flag_0068edd8) {
            SetFlag603C60();
            g_flag_0068edd8 = 0;
            gfTrackMousePos = 0;
        }
        memset(normalized, 0, sizeof(normalized));
        wchar_t* output = normalized;
        for (unsigned int index = 0; index < wcslen(text); ++index) {
            if (text[index] == L'\n' && index > 0 && text[index - 1] != L' ') {
                *output++ = L' ';
            }
            *output++ = text[index];
        }
        g_screen_state_00649f1c->quote_bubble = LayoutPortraitQuoteBubble(
            -1, 0, 0, normalized, 280, 0, 0, 0, &width, &height, font_palette);
        if (g_screen_state_00649f1c->quote_bubble == -1) {
            if (g_screen_state_00649f1c->dialogue_npc != 0) {
                swprintf(error_text,
                         L"Error creating box - most likely text too large: NPC %s, quote %d",
                         g_screen_state_00649f1c->dialogue_npc->record->source_name_004, quote_id);
            } else {
                swprintf(error_text, L"Error creating box - most likely text too large");
            }
            g_screen_state_00649f1c->quote_bubble =
                LayoutPortraitQuoteBubble(-1, 0, 0, error_text, 300, 0, 0, 0, &width, &height, -1);
            ShowNotice(0xc, error_text, 0, GetTextBoxScrollRange(), 0);
        }
        g_screen_state_00649f1c->quote_width = width;
        g_screen_state_00649f1c->quote_height = height;
        g_screen_state_00649f1c->quote_x = 320 - (width >> 1);
        g_screen_state_00649f1c->quote_y = quote_id < 0 ? 350 - height : 20;
        SetRegionBounds(0x136, g_screen_state_00649f1c->quote_x, g_screen_state_00649f1c->quote_y,
                        g_screen_state_00649f1c->quote_x + g_screen_state_00649f1c->quote_width,
                        g_screen_state_00649f1c->quote_y + g_screen_state_00649f1c->quote_height);
        RegionSetEnable(0x25);
        EnableRegionSetInput(0x25);
        g_screen_state_00649f1c->quote_visible = true;
        if (notice_kind == 0) {
            W8PendingNoticeLine* line = new W8PendingNoticeLine;
            line->text = static_cast<wchar_t*>(malloc((wcslen(normalized) + 1) * sizeof(wchar_t)));
            line->npc_kind = npc_kind;
            wcscpy(line->text, normalized);
            g_screen_state_00649f1c->pending_notice_lines.Add(line);
        }
        g_screen_state_00649f1c->quote_notice_kind = notice_kind;
        g_screen_state_00649f1c->quote_notice_payload = payload;
        if (g_screen_state_00649f1c->quote_notice_kind == 3) {
            SoundPlay(reinterpret_cast<STR>(const_cast<char*>( // reinterpret-ok: SGP text ABI
                          "Data\\Sound\\Misc\\GainLevel.wav")),
                      0);
        }
        return;
    }

    bool flush_notices =
        (quote_id == 0x12 || quote_id < 0) && g_screen_state_00649f1c->quote_notice_kind == 0;
    switch (g_screen_state_00649f1c->quote_notice_kind) {
    case 1: {
        W8ExperienceNoticePayload* experience =
            static_cast<W8ExperienceNoticePayload*>(g_screen_state_00649f1c->quote_notice_payload);
        FormatNotice(0xc, 0, gppStringList[experience->alternate_message ? 0x231 : 0x232],
                     experience->amount);
        delete experience;
        break;
    }
    case 2: {
        W8SkillNoticePayload* skills =
            static_cast<W8SkillNoticePayload*>(g_screen_state_00649f1c->quote_notice_payload);
        for (int index = 0; index < skills->count; ++index) {
            int slot = skills->party_slots[index];
            int skill = skills->skills[index];
            W8Character* character = &g_status_685170.buffers.characters[slot];
            unsigned int value = character->skills[skill].value_02;
            if (skill == g_profession_bonus_skills[character->current_profession]) {
                value = value * 125 / 100;
            }
            PostCharacterNotice(slot, gppStringList[0x1d9],
                                gppStringList[g_character_skill_name_ids_61e454[skill]], value);
        }
        delete skills;
        break;
    }
    case 3: {
        int* slot = static_cast<int*>(g_screen_state_00649f1c->quote_notice_payload);
        PostCharacterNotice(*slot, gppStringList[0x773]);
        delete slot;
        break;
    }
    }
    if (quote != 0) {
        for (unsigned int index = 0; index < quote->entry_count; ++index) {
            if (quote->entries[index].kind_00 == 0x13 || quote->entries[index].kind_00 == 5) {
                flush_notices = true;
            }
        }
    }
    if (flush_notices) {
        FlushPendingNoticeLines005766B0();
    }
    g_screen_state_00649f1c->quote_visible = false;
    if (g_screen_state_00649f1c->quote_bubble != -1) {
        ReleasePortraitQuoteBubble(g_screen_state_00649f1c->quote_bubble);
    }
    RegionSetDisable(0x25);
    DisableRegionSetInput(0x25);
    if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME) {
        if (g_screen_state_00649f1c->quote_bubble != -1) {
            ClearSurfaceRect(
                g_screen_state_00649f1c->quote_x, g_screen_state_00649f1c->quote_y,
                g_screen_state_00649f1c->quote_x + g_screen_state_00649f1c->quote_width,
                g_screen_state_00649f1c->quote_y + g_screen_state_00649f1c->quote_height);
            InvalidateRegion(
                g_screen_state_00649f1c->quote_x, g_screen_state_00649f1c->quote_y,
                g_screen_state_00649f1c->quote_x + g_screen_state_00649f1c->quote_width,
                g_screen_state_00649f1c->quote_y + g_screen_state_00649f1c->quote_height, 0);
        }
        RequestRedraw(0x200);
        RequestRedraw(0xff);
        RequestRedraw(0x8000);
    } else if (g_current_screen_state.id == W8_SCREEN_CAMP) {
        g_camp_screen_0069c0f4->redraw_flags |= 0x0fffffff;
    }
    g_screen_state_00649f1c->quote_bubble = -1;
}

// FUNCTION: WIZ8 0x00576670
void DrawNpcQuoteBubble(void)
{
    IsModalOpen();
    if (g_screen_state_00649f1c->quote_visible) {
        DrawPortraitQuoteBubble(g_screen_state_00649f1c->quote_bubble,
                                g_screen_state_00649f1c->quote_x, g_screen_state_00649f1c->quote_y,
                                -14);
    }
}

// FUNCTION: WIZ8 0x005767f0
void LookAtDialogueNpc(void)
{
    W8MonsterInfo* info = GetNpcMonsterInfo(g_screen_state_00649f1c->dialogue_npc);
    if (info != 0) {
        srVector3T<float> position = info->monster->movement_0c0.position_040;
        position.y += info->monster->movement_0c0.height_offset_0b8;
        g_gd_camera_65a0f8->LookAt(&position, 0);
    }
}

// FUNCTION: WIZ8 0x00576b80
void CloseNpcDialogueIfActive(void)
{
    if (gXStatus.fNpcDialogueMode != 0) {
        Function56E800(0);
    }
}

// FUNCTION: WIZ8 0x005766B0
void FlushPendingNoticeLines005766B0(void)
{
    wchar_t npc_name[100];
    int index;

    for (index = 0; index < g_screen_state_00649f1c->pending_notice_lines.GetCount(); ++index) {
        W8PendingNoticeLine* line = *g_screen_state_00649f1c->pending_notice_lines.GetAt(index);
        if (line->npc_kind != -1 &&
            line->npc_kind != g_screen_state_00649f1c->last_notice_npc_kind) {
            W8NpcState* npc = GetNpcState(line->npc_kind);
            if (npc != 0) {
                swprintf(npc_name, L"%s", npc->record->source_name_004);
                ShowNotice(1, npc_name, 3, -1, 0);
                g_screen_state_00649f1c->last_notice_npc_kind = line->npc_kind;
            }
        }
        ShowNotice(line->npc_kind == -1 ? 0xb : 0xf, line->text, 3, GetTextBoxScrollRange(), 0);
    }
    while (g_screen_state_00649f1c->pending_notice_lines.GetCount() > 0) {
        W8PendingNoticeLine* line = g_screen_state_00649f1c->pending_notice_lines.RemoveAt(0);
        free(line->text);
        delete line;
    }
}

/* Queue `party_slot` for the pending screen and unwind whichever main-game
   mode is live - dialogue, review screen or the mode-6 highlight overlay -
   before handing off. The flag decides whether the pending payload carries
   the slot's character pointer. */
// FUNCTION: WIZ8 0x00560E10
void Function560E10(unsigned int party_slot, int flag)
{
    if (gXStatus.fNpcDialogueMode != 0) {
        Function577020();
    }
    g_pending_screen_state.parameter_2 = party_slot;
    g_pending_screen_state.parameter_3 = g_status_685170.buffers.characters + party_slot;
    g_pending_screen_state.parameter_4 =
        flag != 0 ? reinterpret_cast<int>(g_pending_screen_state.parameter_3)
                  : 0; // reinterpret-ok: the pending slot stores the pointer as an int
    if (g_main_game_mode_0068eddc == 3) {
        if (gXStatus.fNpcDialogueMode != 0) {
            Function56E800(0);
        }
    } else if (g_main_game_mode_0068eddc == 5) {
        Function5187E0();
    } else if (g_main_game_mode_0068eddc == 6) {
        if (g_level_block->highlight_graphic != 0) {
            ReleaseObject004257F0(g_level_block->highlight_graphic);
            g_level_block->highlight_graphic = 0;
            if (g_main_game_mode_0068eddc != 6) {
                goto done;
            }
        }
        ClearSurfaceRect(g_level_block->dialogue_x_220, g_level_block->dialogue_y_224,
                         g_level_block->dialogue_width_238 + g_level_block->dialogue_x_220,
                         g_level_block->dialogue_height_228 + g_level_block->dialogue_y_224);
        InvalidateRegion(g_level_block->dialogue_x_220, g_level_block->dialogue_y_224,
                         g_level_block->dialogue_width_238 + g_level_block->dialogue_x_220,
                         g_level_block->dialogue_height_228 + g_level_block->dialogue_y_224, 0);
        if (g_level_block->dialogue_y_224 <
                static_cast<unsigned int>(
                    g_viewport_modes_647d30[g_level_block->camera_mode_100].top) &&
            g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
            g_level_block->redraw_flags |= 0x100;
        }
        if (g_level_block->dialogue_y_224 + g_level_block->dialogue_height_228 > 0x166 &&
            g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
            g_level_block->redraw_flags |= 0x800;
        }
    }
done:
    g_main_game_mode_0068eddc = 0;
    SetPendingScreenState(6);
    if (gXStatus.fLockInteractMode != 0) {
        Function5879A0(1);
    }
    if (gXStatus.fTrapInteractMode != 0) {
        Function58A790(1);
    }
    if (gXStatus.fSpellCastMode != 0) {
        CloseSpellCastingView0059F2B0();
    }
    if (gXStatus.fItemSelectMode != 0) {
        Function59CAC0();
    }
    if (gXStatus.fReviewCharacterMode != 0) {
        CloseFormationPanel();
    }
    SetPrimarySurfaceTextureHint2Enabled(0);
}

/* The viewport mode the screen falls back to after a raised overlay drops:
   full-3d while every overlay flag is clear and no main-game mode override is
   set, the plain mode when any of the board, radar or combat latches is still
   down, and otherwise the override field's own mapping. */
// FUNCTION: WIZ8 0x005698C0
short Function5698C0(void)
{
    if (gXStatus.fSpellCastMode == 0 &&
        (gXStatus.fNpcDialogueMode == 0 || CanOpenNpcDialogue() != 0) &&
        gXStatus.fLockInteractMode == 0 && gXStatus.fTrapInteractMode == 0 &&
        gXStatus.fItemSelectMode == 0 && g_level_block->flag_155 == 0 &&
        g_level_block->formation_board_visible != 0 && g_level_block->flag_157 != 0 &&
        g_settings_6850c8.field_006 == 0) {
        return 4;
    }
    if (gXStatus.fSpellCastMode == 0 &&
        (gXStatus.fNpcDialogueMode == 0 || CanOpenNpcDialogue() != 0) &&
        gXStatus.fLockInteractMode == 0 && gXStatus.fTrapInteractMode == 0 &&
        gXStatus.fItemSelectMode == 0 &&
        (g_level_block->formation_board_visible == 0 || g_level_block->flag_157 == 0 ||
         g_level_block->flag_155 == 0)) {
        return 0;
    }
    if (g_settings_6850c8.field_006 == 1) {
        return 1;
    }
    if (g_settings_6850c8.field_006 == 2) {
        return 0;
    }
    return 2;
}

/* Drop whichever of the formation board, the radar map and the combat bar is
   up, restoring the viewport mode that was in effect when each was raised. */
// FUNCTION: WIZ8 0x00569570
void Function569570(void)
{
    if (g_level_block->formation_board_visible != 0) {
        g_level_block->formation_board_visible = 0;
        RegionSetDisable(0x13);
        ReleaseFormationBoard();
        if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
            g_level_block->redraw_flags |= 0x8200;
        }
        if (g_flag_0068edc9 != 0) {
            unsigned short mode;
            if (gXStatus.fSpellCastMode == 0 &&
                (gXStatus.fNpcDialogueMode == 0 || CanOpenNpcDialogue() != 0) &&
                gXStatus.fLockInteractMode == 0 && gXStatus.fTrapInteractMode == 0 &&
                gXStatus.fItemSelectMode == 0 && g_level_block->flag_155 == 0 &&
                g_level_block->formation_board_visible != 0 &&
                g_level_block->flag_157 != 0 && g_settings_6850c8.field_006 == 0) {
                mode = 4;
            } else if (gXStatus.fSpellCastMode == 0 &&
                       (gXStatus.fNpcDialogueMode == 0 || CanOpenNpcDialogue() != 0) &&
                       gXStatus.fLockInteractMode == 0 && gXStatus.fTrapInteractMode == 0 &&
                       gXStatus.fItemSelectMode == 0 &&
                       (g_level_block->formation_board_visible == 0 ||
                        g_level_block->flag_157 == 0 || g_level_block->flag_155 == 0)) {
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
    if (g_level_block->flag_157 != 0) {
        g_level_block->flag_157 = 0;
        DisableRegionInput(0x62);
        RegionSetDisable(0x12);
        EnableRadarMap(0);
        ReleaseRadarMap();
        if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
            g_level_block->redraw_flags |= 0x8200;
        }
        if (g_flag_0068edbc != 0) {
            unsigned short mode;
            if (gXStatus.fSpellCastMode == 0 &&
                (gXStatus.fNpcDialogueMode == 0 || CanOpenNpcDialogue() != 0) &&
                gXStatus.fLockInteractMode == 0 && gXStatus.fTrapInteractMode == 0 &&
                gXStatus.fItemSelectMode == 0 && g_level_block->flag_155 == 0 &&
                g_level_block->formation_board_visible != 0 &&
                g_level_block->flag_157 != 0 && g_settings_6850c8.field_006 == 0) {
                mode = 4;
            } else if (gXStatus.fSpellCastMode == 0 &&
                       (gXStatus.fNpcDialogueMode == 0 || CanOpenNpcDialogue() != 0) &&
                       gXStatus.fLockInteractMode == 0 && gXStatus.fTrapInteractMode == 0 &&
                       gXStatus.fItemSelectMode == 0 &&
                       (g_level_block->formation_board_visible == 0 ||
                        g_level_block->flag_157 == 0 || g_level_block->flag_155 == 0)) {
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
        g_flag_0068edbc = 0;
    }
    if (g_level_block->flag_155 != 0) {
        g_level_block->flag_155 = 0;
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
        if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
            g_level_block->redraw_flags |= 0x8200;
        }
        if (g_flag_0068edc8 != 0) {
            SetViewportMode(Function5698C0());
        }
        g_flag_0068edc8 = 0;
    }
}

/* Retire the current dialogue layout mode into value_104 and, when nonzero,
   install `value` as the new one; either way the dialogue cursor helper gets
   re-run while the flag is set. */
// FUNCTION: WIZ8 0x0056EDD0
void Function56EDD0(int value)
{
    if (value == 0) {
        g_screen_state_00649f1c->value_104 = g_screen_state_00649f1c->value_fc;
        g_screen_state_00649f1c->value_fc = 0;
    } else {
        g_screen_state_00649f1c->value_fc = value;
    }
    if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
        Function576850(0);
    }
}

/* Bring up the mode-2 dialogue layout: the option panels come up, the caption
   takes the NPC's name, and the five topics get their strings and callbacks. */
// FUNCTION: WIZ8 0x00570760
void Function570760(void)
{
    g_screen_state_00649f1c->value_fc = 2;
    if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
        Function576850(0);
    }
    g_screen_state_00649f1c->panel_1a8->SetEnabled(1);
    g_screen_state_00649f1c->panel_1ac->SetEnabled(1);
    g_screen_state_00649f1c->panel_1b8->SetEnabled(1);
    RegionSetEnable(0x18);
    g_screen_state_00649f1c->dialogue_text_1a4->SetActive(1);
    g_screen_state_00649f1c->dialogue_text_10c->m_textBuffer.SetText(
        g_screen_state_00649f1c->dialogue_npc->record->source_name_004,
        g_wiz_text_bold_font_683664);
    g_screen_state_00649f1c->dialogue_text_10c->Invalidate(1);
    g_screen_state_00649f1c->dialogue_text_1a4->m_textBuffer.SetText(gppStringList[0x1c98 / 4],
                                                                   g_wiz_text_bold_font_683664);
    g_screen_state_00649f1c->dialogue_text_110->m_textBuffer.SetText(gppStringList[0x1c9c / 4],
                                                                   g_font_683660);
    g_screen_state_00649f1c->dialogue_text_110->m_primaryActivationCallback = Function570AD0;
    g_screen_state_00649f1c->dialogue_text_114->m_textBuffer.SetText(gppStringList[0x1ca0 / 4],
                                                                   g_font_683660);
    g_screen_state_00649f1c->dialogue_text_114->m_primaryActivationCallback = Function570B80;
    g_screen_state_00649f1c->dialogue_text_118->m_textBuffer.SetText(gppStringList[0x1ca4 / 4],
                                                                   g_font_683660);
    g_screen_state_00649f1c->dialogue_text_118->m_primaryActivationCallback = Function570C20;
    g_screen_state_00649f1c->dialogue_text_11c->m_textBuffer.SetText(gppStringList[0x1c8c / 4],
                                                                   g_font_683660);
    g_screen_state_00649f1c->dialogue_text_11c->m_primaryActivationCallback = Function570530;
    g_screen_state_00649f1c->dialogue_text_120->m_textBuffer.SetText(gppStringList[0x1c90 / 4],
                                                                   g_font_683660);
    g_screen_state_00649f1c->dialogue_text_120->m_primaryActivationCallback = Function5705B0;
    g_screen_state_00649f1c->dialogue_text_124->m_textBuffer.SetText(gppStringList[0x1c94 / 4],
                                                                   g_font_683660);
    g_screen_state_00649f1c->dialogue_text_124->m_primaryActivationCallback = Function570310;
    g_screen_state_00649f1c->dialogue_text_128->SetActive(0);
    g_screen_state_00649f1c->dialogue_text_114->SetEnabled(
        g_screen_state_00649f1c->dialogue_npc->unknown_c8[0] == 0);
    if (g_screen_state_00649f1c->dialogue_npc->unknown_c8[1] == 0) {
        g_screen_state_00649f1c->dialogue_text_110->SetEnabled(1);
    } else {
        g_screen_state_00649f1c->dialogue_text_110->SetEnabled(0);
    }
    RequestRedraw(0x200);
    Function58F6B0(3);
}

/* Tear down the mode-3 transcript layout: fold the dialogue text controller
   back up, drop the panels, then re-enable whichever occupied party rows
   still own region slots. */
// FUNCTION: WIZ8 0x00571370
void Function571370(void)
{
    int index;

    Function55E940(g_screen_state_00649f1c->npc_dialogue_controller_1b0);
    CollapseNpcDialogueTextArea(g_screen_state_00649f1c->npc_dialogue_controller_1b0);
    Function55EA40(g_screen_state_00649f1c->npc_dialogue_controller_1b0);
    RegionSetDisable(0x18);
    RegionSetDisable(0x16);
    g_screen_state_00649f1c->panel_1a8->SetEnabled(0);
    g_screen_state_00649f1c->panel_1ac->SetEnabled(0);
    g_screen_state_00649f1c->npc_dialogue_controller_1b0->SetEnabled(0);
    g_screen_state_00649f1c->npc_dialogue_panel_1b4->SetEnabled(0);
    g_screen_state_00649f1c->text_input_panel_1c0->SetEnabled(0);
    g_screen_state_00649f1c->value_104 = g_screen_state_00649f1c->value_fc;
    g_screen_state_00649f1c->value_fc = 0;
    if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
        Function576850(0);
    }
    ClearNpcDialogueTextBackground(g_screen_state_00649f1c->npc_dialogue_controller_1b0);
    ClearSurfaceRect(0x1dc, 0x11b, 0x269, 0x1c2);
    InvalidateRegion(0x1dc, 0x11b, 0x269, 0x1c2, 0);
    for (index = 0; index < 8; ++index) {
        if (g_status_685170.buffers.party_rows[index].occupied != 0) {
            RegionSetEnable(7 + index);
            EnableRegionSetInput(7 + index);
            EnableRegionInput(0x5a + index);
        }
    }
}

/* Tear down the mode-4 option layout: the six option controls lose their
   secondary state and layout flags before everything is disabled. */
// FUNCTION: WIZ8 0x00572320
void Function572320(void)
{
    W8TextControl* text;

    Function56FED0();
    if (gXStatus.fCampMode == 0) {
        g_screen_state_00649f1c->value_100 = 0;
    }
    RegionSetDisable(0x18);
    RegionSetDisable(0x17);
    g_screen_state_00649f1c->dialogue_text_188->SetEnabled(1);
    g_screen_state_00649f1c->dialogue_text_110->DisableSecondaryState(1);
    g_screen_state_00649f1c->dialogue_text_114->DisableSecondaryState(1);
    g_screen_state_00649f1c->dialogue_text_118->DisableSecondaryState(1);
    g_screen_state_00649f1c->dialogue_text_11c->DisableSecondaryState(1);
    g_screen_state_00649f1c->dialogue_text_120->DisableSecondaryState(1);
    text = g_screen_state_00649f1c->dialogue_text_120;
    text->m_textBuffer.SetFontStateIndex(-1);
    text->m_textBuffer.SetGeometryDirty();
    g_screen_state_00649f1c->dialogue_text_124->DisableSecondaryState(1);
    text = g_screen_state_00649f1c->dialogue_text_124;
    text->m_textBuffer.SetFontStateIndex(-1);
    text->m_textBuffer.SetGeometryDirty();
    g_screen_state_00649f1c->dialogue_text_110->RemoveLayoutFlags(g_W8TextControlMask005ED578);
    g_screen_state_00649f1c->dialogue_text_114->RemoveLayoutFlags(g_W8TextControlMask005ED578);
    g_screen_state_00649f1c->dialogue_text_118->RemoveLayoutFlags(g_W8TextControlMask005ED578);
    g_screen_state_00649f1c->dialogue_text_11c->RemoveLayoutFlags(g_W8TextControlMask005ED578);
    g_screen_state_00649f1c->dialogue_text_120->RemoveLayoutFlags(g_W8TextControlMask005ED578);
    g_screen_state_00649f1c->dialogue_text_124->RemoveLayoutFlags(g_W8TextControlMask005ED578);
    g_screen_state_00649f1c->dialogue_text_110->SetEnabled(1);
    g_screen_state_00649f1c->dialogue_text_114->SetEnabled(1);
    g_screen_state_00649f1c->dialogue_text_118->SetEnabled(1);
    g_screen_state_00649f1c->dialogue_text_11c->SetEnabled(1);
    g_screen_state_00649f1c->dialogue_text_120->SetEnabled(1);
    g_screen_state_00649f1c->dialogue_text_124->SetEnabled(1);
    g_screen_state_00649f1c->panel_1a8->SetEnabled(0);
    g_screen_state_00649f1c->panel_1ac->SetEnabled(0);
    g_screen_state_00649f1c->panel_1bc->SetEnabled(0);
    g_screen_state_00649f1c->value_104 = g_screen_state_00649f1c->value_fc;
    g_screen_state_00649f1c->value_fc = 0;
    if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
        Function576850(0);
    }
    g_screen_state_00649f1c->flag_229 = 0;
    Function58F6B0(3);
    Function58BA60();
}

/* Tear down the mode-5 dialogue layout and retire the current mode. */
// FUNCTION: WIZ8 0x00573570
void Function573570(void)
{
    RegionSetDisable(0x18);
    RegionSetDisable(0x17);
    g_screen_state_00649f1c->dialogue_text_188->SetEnabled(1);
    g_screen_state_00649f1c->panel_1a8->SetEnabled(0);
    g_screen_state_00649f1c->panel_1ac->SetEnabled(0);
    g_screen_state_00649f1c->panel_1bc->SetEnabled(0);
    g_screen_state_00649f1c->dialogue_text_110->SetEnabled(1);
    g_screen_state_00649f1c->dialogue_text_114->SetEnabled(1);
    g_screen_state_00649f1c->value_104 = g_screen_state_00649f1c->value_fc;
    g_screen_state_00649f1c->value_fc = 0;
    if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
        Function576850(0);
    }
}

/* Tear down the mode-1 dialogue layout; outside camp the mode is retired as
   well. */
// FUNCTION: WIZ8 0x00573DD0
void Function573DD0(void)
{
    RegionSetDisable(0x18);
    RegionSetDisable(0x17);
    g_screen_state_00649f1c->dialogue_text_11c->RemoveLayoutFlags(g_W8TextControlMask005ED578);
    g_screen_state_00649f1c->panel_1a8->SetEnabled(0);
    g_screen_state_00649f1c->panel_1ac->SetEnabled(0);
    g_screen_state_00649f1c->panel_1b8->SetEnabled(0);
    g_screen_state_00649f1c->dialogue_text_11c->DisableRegionHelp();
    g_screen_state_00649f1c->dialogue_text_110->SetEnabled(1);
    g_screen_state_00649f1c->dialogue_text_114->SetEnabled(1);
    g_screen_state_00649f1c->dialogue_text_118->SetEnabled(1);
    g_screen_state_00649f1c->dialogue_text_11c->SetEnabled(1);
    if (gXStatus.fCampMode == 0) {
        g_screen_state_00649f1c->value_104 = g_screen_state_00649f1c->value_fc;
        g_screen_state_00649f1c->value_fc = 0;
        if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
            Function576850(0);
        }
    }
}

// FUNCTION: WIZ8 0x00575810
unsigned char Function575810(W8ItemInstance* item)
{
    W8MessageDialogBase* dialog;
    wchar_t* message;
    unsigned char result;
    unsigned char flag;
    int fact_result;

    result = 1;
    if (g_screen_state_00649f1c->value_22c != 0 && item == 0) {
        if (g_screen_state_00649f1c->value_22c != (int)g_status_685170.party_gold) {
            Function575710();
            return 1;
        }
        dialog = static_cast<W8MessageDialogBase*>(CreateDialogByKind(1));
        dialog->SetClientExtent(0xfa, 200);
        message = FormatWideString(gppStringList[0x1f58 / 4]);
        dialog->SetMessage(message, 1, 0x32, 1, 1, 1, 1, 0, 0x15e);
        SetDialogDestroyCallback(dialog, Function575520);
        OpenModal(dialog);
        return 1;
    }
    if (GetNpcDispositionBand(g_screen_state_00649f1c->dialogue_npc) == 0 ||
        g_screen_state_00649f1c->dialogue_npc->record->flag_2ea != 0) {
        if (item != 0) {
            fact_result = Function528CD0(item->item_id, 0, &flag);
            if (fact_result == -1) {
                if (g_screen_state_00649f1c->dialogue_npc->record->flag_2ea != 0) {
                    return 1;
                }
                if ((g_item_records[item->item_id].flags_041 & 2) != 0) {
                    goto unavailable;
                }
                /* 0x005B1740 is the folded return-one sentinel; this site
                   calls it through a three-argument predicate spelling. */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-function-type-mismatch"
                if (reinterpret_cast<unsigned char (*)(W8NpcState*, W8ItemInstance*, int)>(
                        ScreenLifecycleSuccess)(g_screen_state_00649f1c->dialogue_npc, item,
                                                1) != 0) { // reinterpret-ok: see above
#pragma clang diagnostic pop
                    Function528830(0x10, 0, 0, 0);
                    goto remove;
                }
                Function528830(0x11, 0, 0, 0);
            } else {
                Function528830(fact_result, 0, 0, 0);
                result = 0;
                if (flag != 0) {
remove:
                    Function528FF0(item, 0, -1);
                    return result;
                }
                if (g_screen_state_00649f1c->flag_1f9 != 0 &&
                    g_screen_state_00649f1c->pending_item_1ed.item_id == item->item_id) {
                    g_screen_state_00649f1c->flag_1f9 = 0;
                    AddItemToParty(&g_screen_state_00649f1c->pending_item_1ed, 1, 0);
                    ClearHeldItemDisplay();
                    return 0;
                }
            }
        }
    } else if (item != 0) {
        if ((g_item_records[item->item_id].flags_041 & 2) == 0) {
            if (WillNpcTradeForItem(g_screen_state_00649f1c->dialogue_npc, item) == 0) {
                Function528830(7, 0, 0, 0);
                return 1;
            }
            fact_result = Function528CD0(item->item_id, 0, &flag);
            if (fact_result == -1) {
                Function50A570(g_screen_state_00649f1c->dialogue_npc, 3,
                               g_screen_state_00649f1c->dialogue_speaker, item);
                Function528830(
                    GetNpcDispositionBand(g_screen_state_00649f1c->dialogue_npc) == 0 ? 0x10 : 7,
                    0, 0, 0);
            } else {
                Function528830(fact_result, 0, 0, 0);
                result = 0;
            }
            Function528FF0(item, 0, -1);
            return result;
        }
    unavailable:
        Function528830(0x11, 0, 0, 0);
        return 1;
    }
    return result;
}

/* Retire the current dialogue layout, then open the layout `interact_id`
   selects. */
// FUNCTION: WIZ8 0x00570120
void Function570120(int interact_id)
{
    switch (g_screen_state_00649f1c->value_fc) {
    case 1:
        Function573DD0();
        break;
    case 2:
        RegionSetDisable(0x18);
        g_screen_state_00649f1c->panel_1a8->SetEnabled(0);
        g_screen_state_00649f1c->panel_1ac->SetEnabled(0);
        g_screen_state_00649f1c->panel_1b8->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_text_1a4->SetActive(0);
        g_screen_state_00649f1c->dialogue_text_1a4->m_textBuffer.SetText(
            &g_wchar_00689b34, g_wiz_text_bold_font_683664);
        g_screen_state_00649f1c->value_104 = g_screen_state_00649f1c->value_fc;
        g_screen_state_00649f1c->value_fc = 0;
        if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
            Function576850(0);
        }
        break;
    case 3:
        Function571370();
        break;
    case 4:
        Function572320();
        break;
    case 5:
        RegionSetDisable(0x18);
        RegionSetDisable(0x17);
        g_screen_state_00649f1c->dialogue_text_188->SetEnabled(1);
        g_screen_state_00649f1c->panel_1a8->SetEnabled(0);
        g_screen_state_00649f1c->panel_1ac->SetEnabled(0);
        g_screen_state_00649f1c->panel_1bc->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_text_110->SetEnabled(1);
        g_screen_state_00649f1c->dialogue_text_114->SetEnabled(1);
        g_screen_state_00649f1c->value_104 = g_screen_state_00649f1c->value_fc;
        g_screen_state_00649f1c->value_fc = 0;
        if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
            Function576850(0);
        }
        break;
    case 6:
        g_screen_state_00649f1c->value_104 = g_screen_state_00649f1c->value_fc;
        g_screen_state_00649f1c->value_fc = 0;
        if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
            Function576850(0);
        }
        break;
    }
    switch (interact_id) {
    case 1:
        Function573AE0();
        return;
    case 2:
        Function570760();
        return;
    case 3:
        Function570CF0();
        return;
    case 4:
        Function571AA0();
        return;
    case 5:
        Function5732A0();
        return;
    }
}

/* The camp-side mirror of Function570120: camp mode is raised, the current
   dialogue layout is retired, and a still-pending item goes back onto the
   item cursor. */
// FUNCTION: WIZ8 0x00577020
void Function577020(void)
{
    gXStatus.fCampMode = 1;
    switch (g_screen_state_00649f1c->value_fc) {
    case 1:
        Function573DD0();
        break;
    case 2:
        RegionSetDisable(0x18);
        g_screen_state_00649f1c->panel_1a8->SetEnabled(0);
        g_screen_state_00649f1c->panel_1ac->SetEnabled(0);
        g_screen_state_00649f1c->panel_1b8->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_text_1a4->SetActive(0);
        g_screen_state_00649f1c->dialogue_text_1a4->m_textBuffer.SetText(
            &g_wchar_00689b34, g_wiz_text_bold_font_683664);
        g_screen_state_00649f1c->value_104 = g_screen_state_00649f1c->value_fc;
        g_screen_state_00649f1c->value_fc = 0;
        if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
            Function576850(0);
        }
        break;
    case 3:
        Function571370();
        break;
    case 4:
        Function572320();
        break;
    case 5:
        RegionSetDisable(0x18);
        RegionSetDisable(0x17);
        g_screen_state_00649f1c->dialogue_text_188->SetEnabled(1);
        g_screen_state_00649f1c->panel_1a8->SetEnabled(0);
        g_screen_state_00649f1c->panel_1ac->SetEnabled(0);
        g_screen_state_00649f1c->panel_1bc->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_text_110->SetEnabled(1);
        g_screen_state_00649f1c->dialogue_text_114->SetEnabled(1);
        g_screen_state_00649f1c->value_104 = g_screen_state_00649f1c->value_fc;
        g_screen_state_00649f1c->value_fc = 0;
        if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
            Function576850(0);
        }
        break;
    case 6:
        g_screen_state_00649f1c->value_104 = g_screen_state_00649f1c->value_fc;
        g_screen_state_00649f1c->value_fc = 0;
        if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
            Function576850(0);
        }
        break;
    }
    Function56E800(0);
    if (g_screen_state_00649f1c->flag_1f9 != 0) {
        g_status_685170.item_in_hand_235b = g_screen_state_00649f1c->pending_item_1ed;
        SetItemCursor(0);
        return;
    }
    SetTargetCursor(-1);
}

/* The dialogue NPC takes the guard script when it is '*' styled and the party
   walks away; otherwise its record flags drive either a combat notice or the
   queued scripted action named by the record. While the dialogue is still up
   the named-action queue hands the speaker's name to 0x00571660 instead.
   Every occupied living character without a maxed condition practices
   communication (skill 0x16). */
// FUNCTION: WIZ8 0x00577290
void Function577290(int value)
{
    W8MonsterInfo* info;
    W8Character* character;
    int index;

    if (g_screen_state_00649f1c->dialogue_npc->name_style == 0x2a &&
        (info = GetNpcMonsterInfo(g_screen_state_00649f1c->dialogue_npc)) != 0) {
        info->monster->SetScript004C7F10("Guard.msf", 1);
    }
    if ((value == 0 || g_screen_state_00649f1c->dialogue_npc->unknown_1c == 0 ||
         g_screen_state_00649f1c->dialogue_npc->record->unknown_2ef[1] != 0) &&
        g_screen_state_00649f1c->value_25c < 1) {
        if (g_screen_state_00649f1c->dialogue_npc->unknown_1d == 0) {
            Function528830(1, 0, 0, 0);
        } else {
            Function528830(0, 0, 0, 0);
            g_screen_state_00649f1c->dialogue_npc->unknown_1d = 0;
            if (g_screen_state_00649f1c->dialogue_npc->record->unknown_054 == 0 &&
                g_screen_state_00649f1c->dialogue_npc->record->flag_2ea == 0 &&
                g_screen_state_00649f1c->dialogue_npc->record->unknown_056 == 0) {
                for (index = 0; index < 8; ++index) {
                    character = &g_status_685170.buffers.characters[index];
                    if (g_status_685170.buffers.party_rows[index].occupied != 0 &&
                        character->hp_current != 0 && character->highest_condition < 0xf) {
                        PracticeCharacterSkill(character, 0x16, 0xf, 0);
                    }
                }
            }
            if (gXStatus.fNpcDialogueMode != 0 && g_screen_state_00649f1c->flag_252 == 0) {
                Function571660(g_screen_state_00649f1c->dialogue_npc->record->source_name_004,
                               -1, 1);
                return;
            }
            if (g_screen_state_00649f1c->dialogue_npc->record->unknown_054 == 0 &&
                (g_screen_state_00649f1c->dialogue_npc->record->flag_2ea == 0 ||
                 g_screen_state_00649f1c->dialogue_npc->is_present != 0)) {
                Function5775D0(g_screen_state_00649f1c->dialogue_npc->record->source_name_004,
                               -1);
            }
        }
    }
}
