#include "line.h"
#include "wiz8/local_screens/MGSFormation.h"
#include "wiz8/local_screens/MGSPortraits.h"
#include "soundman.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/Search.h"
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
#include "wiz8/engine_code/Level.h"
#include "wiz8/local_screens/MGSSpellCasting.h"
#include "wiz8/local_screens/MGSButtons.h"
#include "wiz8/local_screens/MGSPortraitCombat.h"
#include "wiz8/local_screens/AutomapScreen.h"
#include "wiz8/local_screens/mipe.h"
#include "wiz8/local_code/Targeting.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatRange.h"
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
#include "wiz8/engine_code/3dapi.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/stModelInstance.h"
#include "wiz8/engine_code/stTextureAnim.h"
#include "vobject.h"
#include "wiz8/local_screens/RCSCommon.h"
#include "wiz8/local_code/LoadSaveGame.h"
#include "wiz8/local_code/MonsterAI.h"
#include "wiz8/dialog_code/DialogInterface.h"
#include "wiz8/dialog_code/MessageDialogBase.h"
#include "wiz8/dialog_code/NpcDialog.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/cursor.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/Navigator.h"
#include "wiz8/engine_code/3d.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/GameplayTime.h"
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
#include "wiz8/dialog_code/MonsterInfoDialog.h"
#include "wiz8/dialog_code/PortraitQuote.h"
#include "wiz8/character_event_queue.h"
#include "wiz8/xstatus.h"
#include "wiz8/wiz8_windows.h"
#include "wiz8/world_cursor.h"
#include "wiz8/local_code/MonsterGroup.h"

#include "font.h"
#include "FileMan.h"
#include "input.h"
#include "timer.h"
#include "Types.h"
#include "mousesystem.h"
#include "mousesystem_macros.h"
#include "surrender/srTypeRegistry.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/engine_code/3dapi.h"
#include "wiz8/local_code/ItemManager.h"
#include "wiz8/item_spawning.h"
#include "wiz8/local_screens/MGSPartyMovement.h"
#include "wiz8/engine_code/Cursor3d.h"
#include "wiz8/local_screens/Screens.h"
#include "wiz8/local_screens/MGSPortraits.h"
#include "wiz8/local_screens/ReviewCharacterScreen.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/string_database.h"
#include "wiz8/local_screens/MGSSpellIcons.h"
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
#include "wiz8/monster_generators.h"
#include "wiz8/local_code/Traps.h"
#include "wiz8/local_code/ButtonSound.h"
#include "wiz8/local_code/GameplayInit.h"
#include "vobject_blitters.h"
#include "random.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wiz8/layouts/game_status.h"
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
W8ItemInstance* g_value_00685072;

// GLOBAL: WIZ8 0x00685076
unsigned char g_flag_00685076;

// GLOBAL: WIZ8 0x00685077
signed char g_value_00685077;

// GLOBAL: WIZ8 0x0068edb0
unsigned int g_mouselook_last_tick_0068edb0;
// GLOBAL: WIZ8 0x0068edb4
unsigned char g_mouselook_tick_init_0068edb4;
// GLOBAL: WIZ8 0x0068edbc
unsigned char g_flag_0068edbc;

// GLOBAL: WIZ8 0x0068ede0
float g_mouselook_pending_yaw_0068ede0;
// GLOBAL: WIZ8 0x0068ede4
float g_mouselook_pending_pitch_0068ede4;

// GLOBAL: WIZ8 0x005ee9a0
const float g_mouselook_smooth_max_005ee9a0 = 0.39269906f;
// GLOBAL: WIZ8 0x005ee9a4
const float g_mouselook_smooth_min_005ee9a4 = 0.006135923f;

// GLOBAL: WIZ8 0x0068edc8
unsigned char g_flag_0068edc8;

// GLOBAL: WIZ8 0x0068edc9
unsigned char g_flag_0068edc9;

// GLOBAL: WIZ8 0x0068edd8
unsigned char g_flag_0068edd8;

/* Saved mouse position while mouselook is latched (WarpSystemCursor restore). */
// GLOBAL: WIZ8 0x0068edc0
POINT g_mouselook_cursor_pos_0068edc0;

// GLOBAL: WIZ8 0x0068eddc
int g_main_game_mode_0068eddc;

// GLOBAL: WIZ8 0x00648278
W8MainGameResourceSlot g_main_game_resource_slots[17] = {
    {1, 0, 1, 0, 0, 0, 0},  {2, 0, 5, 0, 0, 0, 0},  {3, 0, 1, 0, 0, 0, 0},  {4, 0, 1, 0, 0, 0, 0},
    {5, 0, 1, 0, 0, 0, 0},  {6, 0, 4, 0, 0, 0, 0},  {7, 0, 4, 0, 0, 0, 0},  {0, 0, 0, 0, 0, 0, 0},
    {8, 0, 1, 0, 0, 0, 0},  {9, 0, 1, 0, 0, 0, 0},  {10, 0, 5, 0, 0, 0, 0}, {11, 0, 1, 0, 0, 0, 0},
    {12, 0, 1, 0, 0, 0, 0}, {13, 0, 4, 0, 0, 0, 0}, {15, 0, 1, 0, 0, 0, 0}, {14, 0, 1, 0, 0, 0, 0},
    {16, 0, 1, 0, 0, 0, 0},
};

// GLOBAL: WIZ8 0x0065bd2c
unsigned char g_build_level_links_0065bd2c;

// GLOBAL: WIZ8 0x0068ede8
int g_next_link_level_0068ede8;

// GLOBAL: WIZ8 0x0068edd9
unsigned char g_flag_0068edd9;

/* When set, the draw path clears FLAG_DISABLE on level model instances before
   RenderFrame, then re-evaluates each instance afterward. */
// GLOBAL: WIZ8 0x0068edda
unsigned char g_flag_0068edda;

// GLOBAL: WIZ8 0x006480f4
const wchar_t g_format_mouselook_angles_006480f4[] = L"%.3f, %.3f";

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
/* 0x0068EE58: empty wide string used to clear dialogue editor text. */
// GLOBAL: WIZ8 0x0068EE58
wchar_t g_wchar_0068ee58[4];
/* 0x0068EE60: the queued NPC script notice; see the type comment in the
   header. */
// GLOBAL: WIZ8 0x0068EE60
W8PendingNotice g_pending_notice_68ee60;
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

// GLOBAL: WIZ8 0x00648170
const wchar_t g_format_d_s_paren_d_slash_d_slash_d_00648170[] = L"%d %s (%d/%d/%d)";

// GLOBAL: WIZ8 0x00647f84
int g_monster_list_right_647f84;

// GLOBAL: WIZ8 0x00647f88
int g_monster_list_bottom_647f88;

// GLOBAL: WIZ8 0x006481b4
const wchar_t g_format_s_colon_s_paren_d_006481b4[] = L"%s: %s (%d)";

// GLOBAL: WIZ8 0x0061c3e0
const wchar_t g_format_s_colon_s_0061c3e0[] = L"%s: %s";
// GLOBAL: WIZ8 0x0064da8c
const wchar_t g_format_s_spaced_colon_0064da8c[] = L" %s : ";

// GLOBAL: WIZ8 0x005ec258
const float g_float_005ec258 = 0.019999999552965164f;
// GLOBAL: WIZ8 0x005eebbc
const float g_float_005eebbc = 120.0f;

// GLOBAL: WIZ8 0x00659c11
bool g_navigator_position_changed_659c11;

// GLOBAL: WIZ8 0x006840BB
unsigned char g_flag_006840bb;

void ApplyPendingMouselook(void);
void ApplyPendingTooltip(void);
void Function5A6970(void);
unsigned char Function5A6790(void);
void Function5A68C0(void);
void Function50B3B0(int value);
void Function59B4C0(void);
void Function59B390(void);
void ApplySavedRedrawInvalidates(void);                                   /* 0x00563D00 */
void RedrawPanel69B940(void);                                             /* 0x0059BC00 */
short Function4EC610(int value);                                          /* 0x004EC610 */
void Function564BA0(int party_slot);                                      /* 0x00564BA0 */
void Function564D80(int party_slot);                                      /* 0x00564D80 */
void Function564710(int party_slot);                                      /* 0x00564710 */
void Function5651F0(int party_slot);                                      /* 0x005651F0 */
void RedrawPartyPortraitBars(unsigned int party_slot, char slot_enabled); /* 0x0059A540 */
void Function5B2980(unsigned char show_portraits);                        /* 0x005B2980 */
void Function56AC80(void);                                                /* 0x0056AC80 */
unsigned char Function56ED80(void);                                       /* 0x0056ED80 */
void Function56ECF0(unsigned char active);                                /* 0x0056ECF0 */
void Function58C790(void);                                                /* 0x0058C790 */
void Function59CF50(int active);                                          /* 0x0059CF50 */
void Function587C50(void);                                                /* 0x00587C50 */
unsigned char GetOpenDialogueFlag(void);                                  /* 0x0058D7C0 */
void RedrawTextBoxComplete(void);                                         /* 0x0058A8C0 */
unsigned char Function568B50(const InputAtom* input);
unsigned char Function591890(const InputAtom* input);
void Function5029A0(void);

bool IsPartyPortraitUnderCursor00561980(unsigned int party_slot);
void UpdateFormationPortraitRefresh0059B2D0(void);
extern unsigned char g_flag_00652da7;
/* Insanity (spell 0x3c) world-cursor extent rows: six doubles per row.
   Three rows fill through 0x00616f40, immediately before the power index. */
// GLOBAL: WIZ8 0x00616eb0
double g_world_cursor_extent_table_00616eb0[18];
/* Per spell-power index into g_world_cursor_extent_table_00616eb0. The
   Insanity cursor reads it byte-indexed by the power field; the eleven bytes
   run to 0x00616f4c, where the separate Magic Effects dword table starts. */
// GLOBAL: WIZ8 0x00616f41
signed char g_spell_power_extent_index_00616f41[11] = {0, 0, 0, 1, 1, 2, 2, 0, 0, 0, 0};
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
// FUNCTION: WIZ8 0x00587960
void ProcessLockInteractMode(void)
{
    g_lock_interaction_68f2c0->Process();
}

/* Redraw the lock interaction's tumbler, info and action panels. */
// FUNCTION: WIZ8 0x00587970
void RedrawLockInteractionPanels(void)
{
    g_lock_interaction_68f2c0->m_tumbler_panel_10->Redraw();
    g_lock_interaction_68f2c0->m_info_panel_14->Redraw();
    g_lock_interaction_68f2c0->m_action_panel_18->Redraw();
}

/* Re-arm the lock tumbler and action region sets while lock interact is up. */
// FUNCTION: WIZ8 0x00587C20
void EnableLockInteractionPanels(void)
{
    if (g_lock_interaction_68f2c0 != 0) {
        g_lock_interaction_68f2c0->EnablePanels(1);
    }
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
    ColorFillVideoSurfaceArea(-0xe, left, top, right, top + 0x24, 0x8000);
    ColorFillVideoSurfaceArea(-0xe, left - 3, top + 0x24, right + 3, bottom, 0x8000);
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

/* Retail ICF folds this onto W8HorizontalRangeThumb::OnMouseEnter. */
// FUNCTION: WIZ8 0x004f58c0 FOLDED
void W8LockTumbler::OnLeftButtonDown(int)
{
    PushButtonSoundScheme005587C0(0, 1);
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
        ApplyMainGameModeFlag(static_cast<W8MainUiMode>(g_value_68f2b0), 1);
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
        ApplyMainGameModeFlag(static_cast<W8MainUiMode>(g_value_68f2b0), 1);
        RequestRedraw(0x200);
        RequestRedraw(0x100);
        RequestRedraw(0x1000);
        OpenUseItemSelectView(g_status_685170.selected_character);
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
   NPC dialogue layout (value_fc == W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX) is not up. */
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
            g_screen_state_00649f1c->value_fc != W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX) {
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
                             g_screen_state_00649f1c->value_fc == W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX
                                 ? m_value_4c
                                 : m_renderArg_20,
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
        ApplyMainGameModeFlag(static_cast<W8MainUiMode>(g_value_68f2c4), 1);
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
        ApplyMainGameModeFlag(static_cast<W8MainUiMode>(g_value_68f2c4), 1);
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

/* Re-arm the trap text/action panel region sets while trap interact is up. */
// FUNCTION: WIZ8 0x0058A880
void EnableTrapInteractionPanelRegions(void)
{
    if (g_main_game_screen != 0) {
        g_main_game_screen->EnablePanelRegionSets(true);
    }
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
    ApplyMainGameModeFlag(static_cast<W8MainUiMode>(g_value_68f2c4), 1);
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
    ApplyMainGameModeFlag(static_cast<W8MainUiMode>(g_value_68f2c4), 1);
    RequestRedraw(0x200);
    RequestRedraw(0x100);
    RequestRedraw(0x1000);
    OpenUseItemSelectView(g_status_685170.selected_character);
}

// FUNCTION: WIZ8 0x0058A750
void UpdateMainGameScreen(void)
{
    g_main_game_screen->Update();
}

/* Redraw the trap interaction's text, status and action panels. */
// FUNCTION: WIZ8 0x0058A760
void RedrawTrapInteractionPanels(void)
{
    g_main_game_screen->m_text_panel_00c->Redraw();
    g_main_game_screen->m_status_panel_010->Redraw();
    g_main_game_screen->m_action_panel_014->Redraw();
}

/* Same NPC-dialogue text-box-layout predicate as IsNpcDialogueTextBoxActive,
   emitted as a second copy for the dialogue text input callers. */
// FUNCTION: WIZ8 0x00577830
bool IsNpcDialogueTextBoxActive577830(void)
{
    return gXStatus.fNpcDialogueMode != 0 &&
           g_screen_state_00649f1c->value_fc == W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX;
}

// FUNCTION: WIZ8 0x00577850
bool CanOpenNpcDialogue(void)
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
    return g_screen_state_00649f1c->value_fc == W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX;
}

// FUNCTION: WIZ8 0x0055DE40
W8NpcDialogueTextController::W8NpcDialogueTextController(int panel_left, int panel_top,
                                                         int panel_right, int panel_bottom,
                                                         int render_target, int render_arg_1c,
                                                         int render_arg_20, int margin_image_id,
                                                         int line_image_id)
    : Controls(panel_left, panel_top, panel_right, panel_bottom, render_target, render_arg_1c,
               render_arg_20),
      text_area()
{
    W8ControlsRect bounds;
    short width;
    short height;

    margin_image = margin_image_id;
    line_image = line_image_id;
    visible = 0;
    GetCatalogImageSize(m_renderTarget, m_renderArg_1c, line_image, &width, &height);
    line_height = height & 0xffff;
    scroll_height = height & 0xffff;
    GetCatalogImageSize(m_renderTarget, m_renderArg_1c, margin_image, &width, &height);
    margin = height & 0xffff;
    bounds.bottom = panel_top + 0x12;
    bounds.left = panel_left + 6;
    bounds.top = bounds.bottom - line_height;
    bounds.right = panel_left + 0x7c;
    text_area.Configure(&bounds, g_font_683660,
                        g_W8TextBufferLayoutMask005EF890 | g_W8TextBufferLayoutMask005EF88C |
                            g_W8TextBufferLayoutMask005EF888);
    text_area.SetLineHeight(scroll_height);
}

/* Expanded transcript paint: when dirty, redraw the panel backdrop, optionally
   tile the line/margin catalog images for the open layout, then forward to
   active children and the embedded text area. */
// FUNCTION: WIZ8 0x0055DF80
void W8NpcDialogueTextController::Redraw()
{
    unsigned char was_dirty;
    int force;
    int height;
    int top;
    int index;

    was_dirty = m_fDirty;
    if (m_fEnabled) {
        force = was_dirty != 0;
        if (force) {
            DrawCatalogImageAndInvalidate(-0xe, m_renderTarget, m_renderArg_1c, m_renderArg_20,
                                          origin_x, origin_y, 2, 0);
            m_fDirty = 0;
        }
        if (line_image != -1 && margin_image != -1) {
            if (visible == 1 && was_dirty != 0) {
                height = line_height;
                top = (origin_y - height) + margin;
                if (height < scroll_height) {
                    do {
                        DrawCatalogImageAndInvalidate(-0xe, m_renderTarget, m_renderArg_1c,
                                                      line_image, origin_x, top, 2, 0);
                        top -= line_height;
                        height += line_height;
                    } while (height < scroll_height);
                }
                DrawCatalogImageAndInvalidate(-0xe, m_renderTarget, m_renderArg_1c, margin_image,
                                              origin_x, (line_height - margin) + top, 2, 0);
                force = 1;
            } else if (force == 0 && m_fLayoutDirty == 0) {
                return;
            }
            for (index = 0; index < m_controls.count; ++index) {
                if (ControlAt(index)->m_active) {
                    ControlAt(index)->Redraw(force);
                }
            }
            m_fLayoutDirty = 0;
            text_area.Draw(static_cast<unsigned char>(force));
        }
    }
}

// FUNCTION: WIZ8 0x0055E0C0
unsigned char W8NpcDialogueTextController::AddTranscriptEntry(const wchar_t* text,
                                                              signed char category, char mark)
{
    wchar_t existing[200];
    int index;
    int added;

    for (index = 0; index < text_area.m_all_lines_01c.count; ++index) {
        text_area.GetEntry(index);
        text_area.CopyEntryText(index, existing);
        if (CompareWideTextIgnoreAsciiCase00402920(existing, text) == 0) {
            return 0;
        }
    }
    added = text_area.AddEntry(0, text, 0, 7, category);
    InvalidateLayout();
    if (mark != 0) {
        text_area.SetEntryState60(added, 1);
    }
    if (1u < (unsigned)text_area.m_all_lines_01c.count) {
        for (index = 0; index < text_area.m_all_lines_01c.count; ++index) {
            text_area.GetEntry(index);
            text_area.CopyEntryText(index, existing);
            if (CompareWideTextIgnoreAsciiCase00402920(existing, L" [No Keywords]") == 0) {
                text_area.RemoveEntry(index);
                Invalidate(0);
                break;
            }
        }
    }
    return 1;
}

// FUNCTION: WIZ8 0x0055E410
unsigned char W8NpcDialogueTextController::IsSlotPortraitTranscriptCovered(unsigned int party_slot)
{
    int top;

    if (visible == 1) {
        top = ((1 - scroll_height / line_height) * line_height - margin) + origin_y;
        switch (party_slot) {
        case 1:
            if (top < 0x67) {
                return 1;
            }
            break;
        case 3:
            if (top < 0xbc) {
                return 1;
            }
            break;
        case 5:
            if (top < 0x111) {
                return 1;
            }
            break;
        case 7:
            return 1;
        }
    }
    return 0;
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

// FUNCTION: WIZ8 0x0055E7C0
void W8NpcDialogueTextController::SetTranscriptCategoryFilter(signed char category)
{
    text_area.SetCategoryFilter(category);
    CollapseNpcDialogueTextArea(this);
    ExpandNpcDialogueTextArea(this);
    if (g_screen_state_00649f1c->npc_dialogue_controller_1b0->scroll_height == 0xff) {
        g_screen_state_00649f1c->dialogue_scroll_up_button->SetEnabled(1);
        g_screen_state_00649f1c->dialogue_scroll_down_button->SetEnabled(1);
    } else {
        g_screen_state_00649f1c->dialogue_scroll_up_button->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_scroll_down_button->SetEnabled(0);
    }
}

// FUNCTION: WIZ8 0x0055E840
void W8NpcDialogueTextController::RestoreTranscriptEntries()
{
    W8DialogueTranscriptRecord* record;
    W8NpcDialogueTextController* controller;
    int index;

    for (index = 0; index < g_screen_state_00649f1c->dialogue_transcript.count; ++index) {
        record = *g_screen_state_00649f1c->dialogue_transcript.GetAt(index);
        AddTranscriptEntry(record->text, record->category, 0);
    }
    Invalidate(0);
    if (g_screen_state_00649f1c->dialogue_transcript.count == 0) {
        g_screen_state_00649f1c->dialogue_category_filter = W8_DIALOGUE_CATEGORY_ALL;
        controller = g_screen_state_00649f1c->npc_dialogue_controller_1b0;
        controller->text_area.SetCategoryFilter(g_screen_state_00649f1c->dialogue_category_filter);
        CollapseNpcDialogueTextArea(controller);
        ExpandNpcDialogueTextArea(controller);
        if (g_screen_state_00649f1c->npc_dialogue_controller_1b0->scroll_height == 0xff) {
            g_screen_state_00649f1c->dialogue_scroll_up_button->SetEnabled(1);
            g_screen_state_00649f1c->dialogue_scroll_down_button->SetEnabled(1);
        } else {
            g_screen_state_00649f1c->dialogue_scroll_up_button->SetEnabled(0);
            g_screen_state_00649f1c->dialogue_scroll_down_button->SetEnabled(0);
        }
        SyncDialogueCategoryButtons();
        AddTranscriptEntry(L" [No Keywords]", W8_DIALOGUE_CATEGORY_ALL, 0);
    }
}

// FUNCTION: WIZ8 0x00577880
unsigned char SetNpcDialoguePanelVisible(int value)
{
    W8NpcDialogueTextController* controller;
    unsigned char expanded;

    if (gXStatus.fNpcDialogueMode != 0 && g_screen_state_00649f1c->flag_252 == 0 &&
        g_screen_state_00649f1c->value_fc == W8_DIALOGUE_LAYOUT_TRANSCRIPT) {
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
            g_screen_state_00649f1c->dialogue_scroll_up_button->SetEnabled(0);
        } else {
            g_screen_state_00649f1c->dialogue_scroll_up_button->SetEnabled(1);
        }
        g_screen_state_00649f1c->dialogue_scroll_down_button->SetEnabled(expanded != 0);
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
        if (g_mgs_keyboard->IsCommandPressed(W8_MGS_COMMAND_CAMERA_LOCK)) {
            BeginManualCameraControl();
        }
        ApplyWorldRenderHotkeys();
    }
}

// FUNCTION: WIZ8 0x00592a10
void ApplyWorldRenderHotkeys(void)
{
    if (g_mgs_keyboard->IsCommandPressed(W8_MGS_COMMAND_TURN_LEFT) ||
        g_mgs_keyboard->IsCommandPressed(W8_MGS_COMMAND_TURN_LEFT_ALT)) {
        g_level_block->world_render_flags |= 0x100;
    }
    if (g_mgs_keyboard->IsCommandPressed(W8_MGS_COMMAND_TURN_RIGHT) ||
        g_mgs_keyboard->IsCommandPressed(W8_MGS_COMMAND_TURN_RIGHT_ALT)) {
        g_level_block->world_render_flags |= 0x200;
    }
    if (g_mgs_keyboard->IsCommandPressed(W8_MGS_COMMAND_MOVE_FORWARD)) {
        g_level_block->world_render_flags |= 4;
    }
    if (g_mgs_keyboard->IsCommandPressed(W8_MGS_COMMAND_MOVE_FORWARD_RUN)) {
        g_level_block->world_render_flags |= 0x84;
    }
    if (g_mgs_keyboard->IsCommandPressed(W8_MGS_COMMAND_MOVE_BACKWARD)) {
        g_level_block->world_render_flags |= 8;
    }
    if (g_mgs_keyboard->IsCommandPressed(W8_MGS_COMMAND_MOVE_BACKWARD_RUN)) {
        g_level_block->world_render_flags |= 0x88;
    }
    if (g_mgs_keyboard->IsCommandPressed(W8_MGS_COMMAND_LOOK_UP)) {
        g_level_block->world_render_flags |= 0x400;
    }
    if (g_mgs_keyboard->IsCommandPressed(W8_MGS_COMMAND_LOOK_DOWN)) {
        g_level_block->world_render_flags |= 0x800;
    }
    if (g_mgs_keyboard->IsCommandPressed(W8_MGS_COMMAND_STRAFE_LEFT)) {
        g_level_block->world_render_flags |= 1;
    }
    if (g_mgs_keyboard->IsCommandPressed(W8_MGS_COMMAND_STRAFE_RIGHT)) {
        g_level_block->world_render_flags |= 2;
    }
    if (g_mgs_keyboard->IsCommandPressed(W8_MGS_COMMAND_STRAFE_LEFT_RUN)) {
        g_level_block->world_render_flags |= 0x81;
    }
    if (g_mgs_keyboard->IsCommandPressed(W8_MGS_COMMAND_STRAFE_RIGHT_RUN)) {
        g_level_block->world_render_flags |= 0x82;
    }
}

// FUNCTION: WIZ8 0x00593330
void RefreshFlaggedMainGameState00593330(void)
{
    if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0 &&
        g_level_block->keyboard_menu_open != 0) {
        RefreshKeyboardMenuRows();
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
    BeginNpcDialogueInternal(g_screen_state_00649f1c->dialogue_npc, 0, -1, 0, 1);
    g_screen_state_00649f1c->value_238 = g_screen_state_00649f1c->value_104;
    g_screen_state_00649f1c->flag_234 = 1;
}

// FUNCTION: WIZ8 0x00577260
void SyncDialogueNpcState00577260(void)
{
    BeginNpcDialogueInternal(g_screen_state_00649f1c->dialogue_npc, 0, -1, 0, 1);
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
    g_screen_state_00649f1c->dialogue_category_filter = unset;
    g_screen_state_00649f1c->transcript_sorted = 0;
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
        LoadMainGameCursorResources();
    }
    gXStatus.unknown_026[1] = 1;
    MSYS_Init();
    ResetRegions();
    CreateMainGameInterfaceButtons();
    CreateSpellIconHudControls();
    CreateLevelButtons();
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
            timer->m_start = timer->GetTime00439A60() - timer->m_start;
            timer->SetDuration(-1.0f);
        }
    }
    DrainInputEventQueue();
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
            SelectPartyCharacter(g_value_00685077);
            OpenUseItemSelectView(g_value_00685077);
            SelectCurrentUseItemLine0059E0E0();
        } else {
            g_flag_00685071 = 0;
            g_value_00685072 = 0;
            g_flag_00685076 = 0xff;
            g_value_00685077 = -1;
        }
    }
    if (!gXStatus.fCombatMode) {
        StartLevelMusic(1, 1);
    }
    return 1;
}

/* While the party is idle, arm a one-minute countdown after input and, once the
   cursor has also been still for a minute and that countdown expires, advance
   ambient follow-up chatter. Busy modes keep refreshing the countdown and
   clear any armed follow-up bits. */
// FUNCTION: WIZ8 0x00561330
void TickAmbientFollowUpIdle(unsigned char input_handled)
{
    if (gXStatus.fCombatMode == 0 && gXStatus.fSpellCastMode == 0 &&
        gXStatus.fItemSelectMode == 0 && gXStatus.fNpcDialogueMode == 0 &&
        gXStatus.fLockInteractMode == 0 && gXStatus.fTrapInteractMode == 0 &&
        gXStatus.fSurprisePossible == 0) {
        if (input_handled == 0) {
            if (GetMillisecondsSinceCursorMove() > 60000) {
                if (ClockIsTicking(g_level_block->countdown_258) != 0) {
                    return;
                }
                gXStatus.character_event_queue->ProcessFollowUpEvents();
                return;
            }
        } else {
            g_level_block->countdown_258 = SetCountdownClock(60000);
        }
        if ((gXStatus.character_event_queue->follow_up_flags & 1) != 0) {
            gXStatus.character_event_queue->follow_up_flags &= ~3;
            return;
        }
    } else {
        g_level_block->countdown_258 = SetCountdownClock(60000);
        if ((gXStatus.character_event_queue->follow_up_flags & 1) != 0) {
            gXStatus.character_event_queue->follow_up_flags &= ~3;
        }
    }
}

// FUNCTION: WIZ8 0x005684E0
unsigned char ProcessMainGameInput(void)
{
    POINT mouse;
    SGPMouseGetPos(&mouse);
    if (GetForcedRegion() == 0) {
        MSYS_SGP_Mouse_Handler_Hook(MOUSE_POS, mouse.x, mouse.y, gfLeftButtonState,
                                    gfRightButtonState);
    }

    unsigned char handled = 0;
    InputAtom input;
    while (DequeueEvent(&input)) {
        handled = 1;
        if ((g_flag_0068edd8 == 0 || Function568B50(&input) == 0) &&
            DispatchRegionInput(&input) == 0 && GetForcedRegion() == 0) {
            SGPMouseGetPos(&mouse);
            switch (input.usEvent) {
            case LEFT_BUTTON_DOWN:
            case LEFT_BUTTON_UP:
            case RIGHT_BUTTON_DOWN:
            case RIGHT_BUTTON_UP:
                MSYS_SGP_Mouse_Handler_Hook(input.usEvent, mouse.x, mouse.y, gfLeftButtonState,
                                            gfRightButtonState);
                break;
            default:
                if (HandleDialogueTextInput(&input) == 0 && Function591890(&input) == 0) {
                    if (g_main_game_mode_0068eddc == 3) {
                        if (gXStatus.fNpcDialogueMode != 0) {
                            Function56E800(0);
                        }
                    } else if (g_main_game_mode_0068eddc == 5) {
                        Function5187E0();
                    } else if (g_main_game_mode_0068eddc == 6) {
                        DismissHighlightOverlay();
                    }
                    g_main_game_mode_0068eddc = 0;
                    if (IsMessageBoxActive()) {
                        Function5187E0();
                    }
                    if (gXStatus.fCombatMode == 0) {
                        if (AnyCharacterActive() && g_party_moving_006850b5 == 0) {
                            AutoSaveIfAllowed(1);
                        }
                    } else {
                        EndCombat004EA310(1);
                    }
                    if (gXStatus.fSurprisePossible != 0) {
                        Function5029A0();
                    }
                    g_status_685170.game_started = 0;
                    ClearHeldItemDisplay();
                    RequestScreenTransition();
                    SetPrimarySurfaceTextureHint2Enabled(0);
                    return 1;
                }
                break;
            }
        }
    }
    return handled;
}

/* While NPC script deferral holds character events during an open dialogue,
   pump Escape and left-click so layout dismissals still run. */
// FUNCTION: WIZ8 0x00575C50
void DrainNpcDialogueDeferralInput(void)
{
    POINT mouse;
    InputAtom input;
    int prior_layout;
    char reopen_topics;

    if (ShouldDeferCharacterEventForNpcScript(1) == 0 || gXStatus.fNpcDialogueMode == 0) {
        return;
    }
    SGPMouseGetPos(&mouse);
    MSYS_SGP_Mouse_Handler_Hook(MOUSE_POS, static_cast<unsigned short>(mouse.x),
                                static_cast<unsigned short>(mouse.y), gfLeftButtonState,
                                gfRightButtonState);
    while (DequeueEvent(&input) == 1) {
        if (input.usEvent == KEY_DOWN) {
            if (input.usParam == 0x1b) {
                if (g_screen_state_00649f1c->dialogue_cursor_flag == 0) {
                    if (IsNpcScriptSessionActive() == 0) {
                        switch (g_screen_state_00649f1c->value_fc) {
                        case 2:
                        case 3:
                            Function56E800(0);
                            break;
                        case 1:
                            prior_layout = g_screen_state_00649f1c->value_104;
                            CloseNpcDialogueMode1Layout();
                            if (prior_layout == 2) {
                                ShowNpcDialogueTopicMenu();
                            } else if (prior_layout == 3) {
                                OpenNpcDialogueTranscriptLayout();
                            }
                            break;
                        case 4:
                            reopen_topics = g_screen_state_00649f1c->flag_229;
                            CloseNpcDialogueOptionLayout();
                            if (reopen_topics != 0) {
                                ShowNpcDialogueTopicMenu();
                            } else {
                                OpenNpcDialogueTranscriptLayout();
                            }
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
                                SetNpcDialogueHidden(0);
                            }
                            OpenNpcDialogueTranscriptLayout();
                            break;
                        }
                    } else {
                        TryFinishNpcVoicePlayback(0);
                    }
                } else {
                    SetNpcDialogueHidden(0);
                }
            }
        } else if (input.usEvent == LEFT_BUTTON_DOWN) {
            TryFinishNpcVoicePlayback(0);
        }
    }
}

/* When the world-cursor gate (value_2435) is raised, discard queued input
   after refreshing the mouse-system position so stale events do not fire. */
// FUNCTION: WIZ8 0x00577560
void FlushInputWhileWorldCursorGate(void)
{
    POINT mouse;
    InputAtom input;

    if (g_status_685170.value_2435 == 0) {
        return;
    }
    SGPMouseGetPos(&mouse);
    MSYS_SGP_Mouse_Handler_Hook(MOUSE_POS, static_cast<unsigned short>(mouse.x),
                                static_cast<unsigned short>(mouse.y), gfLeftButtonState,
                                gfRightButtonState);
    while (DequeueEvent(&input) == 1) {
    }
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
            g_world->octree->BuildRegionLinks(1);
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
    ApplyPendingMouselook();
    ApplyPendingTooltip();
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
        if (g_level_block->review_transition_active) {
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
        FlushDeferredSkillNotices();
    }
    if (!g_level_block->transition_active && !gXStatus.fCombatMode && gXStatus.fEncumbranceDirty) {
        RedistributePartyEncumbrance();
    }
    SyncPartyPortraitVitalsBars();
    if (g_modal_owner_0068edd0) {
        if (!g_flag_006840bc) {
            g_flag_006840bc = 1;
            if (gXStatus.fPartyMovementUi) {
                DisablePartyMovementRegions();
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
    DrainNpcDialogueDeferralInput();
    NoOp();
    FlushInputWhileWorldCursorGate();
    gXStatus.character_event_queue->ProcessDeferredCharacterEvents();
    UpdateCharacterEventState();
    TickPartyPortraitFx();
    if (gXStatus.fCombatMode) {
        Function59B4C0();
    }
    g_status_685170.value_2390 = 0;
    if (g_level_block->keyboard_menu_open || g_level_block->combat_slot != -1) {
        Function59B390();
    }
    UpdateSurpriseMode();
    if (gXStatus.fCombatMode) {
        for (int slot = 0; slot < 8; ++slot) {
            if (!g_status_685170.buffers.party_rows[slot].occupied ||
                g_status_685170.buffers.characters[slot].highest_condition > 0x11 ||
                (g_level_block->keyboard_menu_open && g_level_block->combat_slot == slot)) {
                DisableRegionInput(slot + 10);
            } else {
                EnableRegionInput(slot + 10);
            }
        }
    }
    if (IsScreenTransitionPending()) {
        g_level_block->transition_pending = 1;
        DrawMainGameScreen();
        return;
    }
    ProcessMainGameAutoSave();
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
        UpdateWorldCursor004916C0();
    }
    if (!g_level_block->keyboard_menu_open && g_level_block->hover_combat_slot != -1 &&
        g_level_block->hover_region != g_level_block->hover_combat_slot + 10U) {
        g_level_block->hover_combat_slot = -1;
    }
    TickAmbientFollowUpIdle(ProcessMainGameInput());
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
            MipeWorldViewEvent0057E0E0(MOUSE_POS, &current);
        }
    }
render_world:
    if (!IsScreenTransitionPending()) {
        if (CanUseCurrentAutomapTool()) {
            UpdateWorldCameraAndPaths0044FC20(g_world, g_level_block->world_render_flags);
            if (g_world_659ab8 && !g_level_runtime_flag_0065ba70) {
                UpdateWorldCameraAndPaths0044FC20(g_world_659ab8,
                                                  g_level_block->world_render_flags | 0x40);
            }
        }
        ApplyWorldUpdateFlags(g_world, g_level_block->world_update_flags);
        if (g_world_659ab8 && (g_level_block->world_update_flags & 3) == 0) {
            ApplyWorldUpdateFlags(g_world_659ab8, g_level_block->world_update_flags);
        }
        g_level_block->world_update_flags = 0;
        g_level_block->world_render_flags = 0;
        if (g_settings_6850c8.main_ui_mode != W8_MAIN_UI_MODE_PORTRAITS) {
            UpdateFormationPortraitRefresh0059B2D0();
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
            HandlePartyMovement(&real_elapsed, &frame_elapsed);
            RunSearchPulse();
        } else if (!g_level_block->transition_active && !gXStatus.fSpellCastMode &&
                   !gXStatus.fNpcDialogueMode && !gXStatus.fItemSelectMode) {
            UpdateCombat004E8EA0();
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
            UpdateNearbyWorldItems();
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
            !AnyWorldItemVisible() && !AnyMonsterGeneratorMarkerWithinReach() &&
            !SelectWorldCursorNode0048EFC0()) {
            active = 0;
        } else {
            active = 1;
        }
        SetWorldModelPickingEnabled(active);
        if (gXStatus.unknown_026[1] && !gXStatus.fNpcDialogueMode && !gXStatus.fCombatMode &&
            !g_level_block->transition_active) {
            RefreshLevelUpReadyNotices();
        }
        if (g_status_685170.selected_character == -1) {
            int next = GetNextCharacter(1, 1, -1);
            if (next == -1) {
                srAssertFail("iNextChar != BAD_INDEX",
                             "C:\\Projects\\Wizardry 8\\Local Screens\\MainGameScreen.cpp", 0x1047,
                             0);
            }
            SelectPartyCharacter(next);
        }
    }
    DrawMainGameScreen();
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
        ClearHighlightOverlayRegion();
    }
    g_main_game_mode_0068eddc = 0;

    if (g_flag_0068edd8) {
        SetFlag603C60();
        g_flag_0068edd8 = 0;
        gfTrackMousePos = 0;
    }
    if (IsWorldCursorVisible()) {
        ToggleWorldCursor();
    }
    Function59B270();
    if (gXStatus.fLockInteractMode)
        Function5879A0(0);
    if (gXStatus.fTrapInteractMode)
        Function58A790(0);
    if (gXStatus.fSpellCastMode)
        CloseSpellCastingView();
    if (gXStatus.fItemSelectMode)
        Function59C9C0();
    if (gXStatus.fReviewCharacterMode)
        CloseFormationPanel();
    if (gXStatus.fNpcDialogueMode)
        Function56E800(0);
    if (g_level_block->keyboard_menu_open)
        CloseKeyboardMenu();
    ReleasePortraitControls();
    ReleaseConditionButtons();
    if (gXStatus.fPartyMovementUi)
        DisablePartyMovementRegions();
    RestoreCurrentNpcQuoteBubble();
    if (GetFlag68F105())
        ToggleMipePanel0057D740();
    MoveTimer(1);
    SetEnvironmentTimeEnabled00482990(0);

    if ((g_gameplay_timer_685067->m_flags & 8) == 0) {
        g_gameplay_timer_685067->m_flags |= 8;
        g_gameplay_timer_685067->m_start =
            g_gameplay_timer_685067->GetTime00439A60() - g_gameplay_timer_685067->m_start;
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
            if (!IsScreenInputBlocked() && !g_level_block->action_panel_visible &&
                g_level_block->formation_board_visible && g_level_block->radar_map_visible &&
                g_settings_6850c8.main_ui_mode == W8_MAIN_UI_MODE_PORTRAITS) {
                mode = 4;
            } else if (!IsScreenInputBlocked() && (!g_level_block->formation_board_visible ||
                                                   !g_level_block->radar_map_visible ||
                                                   !g_level_block->action_panel_visible)) {
                mode = 0;
            } else if (g_settings_6850c8.main_ui_mode == W8_MAIN_UI_MODE_FORMATION) {
                mode = 1;
            } else if (g_settings_6850c8.main_ui_mode == W8_MAIN_UI_MODE_RADAR) {
                mode = 0;
            } else {
                mode = 2;
            }
            SetViewportMode(mode);
        }
        g_flag_0068edc9 = 0;
    }

    if (g_level_block->radar_map_visible) {
        g_level_block->radar_map_visible = 0;
        DisableRegionInput(0x62);
        RegionSetDisable(0x12);
        EnableRadarMap(0);
        ReleaseRadarMap();
        if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
            g_level_block->redraw_flags |= 0x8200;
        }
        if (g_flag_0068edbc) {
            SetViewportMode(GetMainGameViewportMode());
        }
        g_flag_0068edbc = 0;
    }

    if (g_level_block->action_panel_visible) {
        g_level_block->action_panel_visible = 0;
        DisableCombatRegions();
        if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
            g_level_block->redraw_flags |= 0x8200;
        }
        if (g_flag_0068edc8) {
            SetViewportMode(GetMainGameViewportMode());
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
    DestroySpellIconHudControls();
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

// GLOBAL: WIZ8 0x0064810c
const char g_warning_drawing_text_box_while_text_buffer_0064810c[] =
    "WARNING: Drawing text box while Text Buffer = %d (AlexP)";

/* Consume the live redraw_flags word: clear the primary surface, refresh the
   chrome panels the current UI mode owns, and repaint any dirty party slots. */
// FUNCTION: WIZ8 0x00562E40
void ApplyMainGameRedrawFlags(void)
{
    unsigned int redraw_flags;
    unsigned char keyboard_menu_invalidate;
    int party_slot;
    int text_box_variant;
    unsigned char text_box_mode;
    unsigned char show_portraits;
    SGPRect saved_clip;
    SGPRect combat_clip;
    short health_percent;
    W8StartupGridRow* portrait_rect;

    redraw_flags = g_level_block->redraw_flags;
    if (redraw_flags == static_cast<unsigned int>(-1)) {
        ClearPrimarySurface();
    } else if ((redraw_flags & 0x200) != 0) {
        g_level_block->redraw_flags = redraw_flags | 0x2c7c00U;
        g_level_block->saved_redraw_flags &= 0xffd383ffU;
        SetRendererMode6596EC();
    }
    if ((g_level_block->redraw_flags & 0x8000) != 0) {
        if (gXStatus.fCombatMode != 0) {
            if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
                g_level_block->redraw_flags |= 0x800000;
            }
        } else {
            RedrawPanel69B940();
        }
    }
    if ((g_level_block->redraw_flags & 0x800000) != 0) {
        RefreshCombatEffectHud();
        if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
            g_level_block->redraw_flags |= 0x8000;
        }
        for (party_slot = 0; party_slot < 8; ++party_slot) {
            if (g_level_block->portrait_refresh_pending[party_slot] != 0 &&
                g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
                g_level_block->redraw_flags |= 1U << (party_slot & 0x1f);
            }
        }
    }
    if ((g_level_block->redraw_flags & 0x8000) != 0) {
        if (g_flag_006840bc == 0) {
            if (gXStatus.fPartyMovementUi != 0) {
                InvalidatePartyMovementPanel();
                goto mark_modal_dirty;
            }
        refresh_tracked_portrait_slot:
            if (g_level_block->condition_orb_party_slot == -1) {
                if (g_level_block->enchantment_orb_party_slot == -1) {
                    if (g_level_block->portrait_overlay_party_slot == -1) {
                        if (g_level_block->condition_highlight_party_slot != -1) {
                            if (g_level_block->highlight_graphic != 0) {
                                ReleaseObject004257F0(g_level_block->highlight_graphic);
                                g_level_block->highlight_graphic = 0;
                            }
                            if (g_main_game_mode_0068eddc == 6) {
                                ClearSurfaceRect(g_level_block->dialogue_x_220,
                                                 g_level_block->dialogue_y_224,
                                                 g_level_block->dialogue_x_220 +
                                                     g_level_block->dialogue_width_238,
                                                 g_level_block->dialogue_y_224 +
                                                     g_level_block->dialogue_height_228);
                                InvalidateRegion(g_level_block->dialogue_x_220,
                                                 g_level_block->dialogue_y_224,
                                                 g_level_block->dialogue_x_220 +
                                                     g_level_block->dialogue_width_238,
                                                 g_level_block->dialogue_y_224 +
                                                     g_level_block->dialogue_height_228,
                                                 0);
                                if (g_level_block->dialogue_y_224 <
                                        static_cast<unsigned int>(
                                            g_viewport_modes_647d30[g_level_block->camera_mode_100]
                                                .top) &&
                                    g_current_screen_state.id == W8_SCREEN_MAIN_GAME &&
                                    g_level_block != 0) {
                                    g_level_block->redraw_flags |= 0x100;
                                }
                                if (g_level_block->dialogue_y_224 +
                                            g_level_block->dialogue_height_228 >
                                        0x166 &&
                                    g_current_screen_state.id == W8_SCREEN_MAIN_GAME &&
                                    g_level_block != 0) {
                                    g_level_block->redraw_flags |= 0x800;
                                }
                            }
                            g_main_game_mode_0068eddc = 0;
                            Function564D80(g_level_block->condition_highlight_party_slot);
                        }
                    } else {
                        if (g_level_block->highlight_graphic != 0) {
                            ReleaseObject004257F0(g_level_block->highlight_graphic);
                            g_level_block->highlight_graphic = 0;
                        }
                        if (g_main_game_mode_0068eddc == 6) {
                            ClearSurfaceRect(
                                g_level_block->dialogue_x_220, g_level_block->dialogue_y_224,
                                g_level_block->dialogue_x_220 + g_level_block->dialogue_width_238,
                                g_level_block->dialogue_y_224 + g_level_block->dialogue_height_228);
                            InvalidateRegion(
                                g_level_block->dialogue_x_220, g_level_block->dialogue_y_224,
                                g_level_block->dialogue_x_220 + g_level_block->dialogue_width_238,
                                g_level_block->dialogue_y_224 + g_level_block->dialogue_height_228,
                                0);
                            if (g_level_block->dialogue_y_224 <
                                    static_cast<unsigned int>(
                                        g_viewport_modes_647d30[g_level_block->camera_mode_100]
                                            .top) &&
                                g_current_screen_state.id == W8_SCREEN_MAIN_GAME &&
                                g_level_block != 0) {
                                g_level_block->redraw_flags |= 0x100;
                            }
                            if (g_level_block->dialogue_y_224 + g_level_block->dialogue_height_228 >
                                    0x166 &&
                                g_current_screen_state.id == W8_SCREEN_MAIN_GAME &&
                                g_level_block != 0) {
                                g_level_block->redraw_flags |= 0x800;
                            }
                        }
                        g_main_game_mode_0068eddc = 0;
                        Function564710(g_level_block->portrait_overlay_party_slot);
                    }
                } else {
                    if (g_level_block->highlight_graphic != 0) {
                        ReleaseObject004257F0(g_level_block->highlight_graphic);
                        g_level_block->highlight_graphic = 0;
                    }
                    if (g_main_game_mode_0068eddc == 6) {
                        ClearSurfaceRect(
                            g_level_block->dialogue_x_220, g_level_block->dialogue_y_224,
                            g_level_block->dialogue_x_220 + g_level_block->dialogue_width_238,
                            g_level_block->dialogue_y_224 + g_level_block->dialogue_height_228);
                        InvalidateRegion(
                            g_level_block->dialogue_x_220, g_level_block->dialogue_y_224,
                            g_level_block->dialogue_x_220 + g_level_block->dialogue_width_238,
                            g_level_block->dialogue_y_224 + g_level_block->dialogue_height_228, 0);
                        if (g_level_block->dialogue_y_224 <
                                static_cast<unsigned int>(
                                    g_viewport_modes_647d30[g_level_block->camera_mode_100].top) &&
                            g_current_screen_state.id == W8_SCREEN_MAIN_GAME &&
                            g_level_block != 0) {
                            g_level_block->redraw_flags |= 0x100;
                        }
                        if (g_level_block->dialogue_y_224 + g_level_block->dialogue_height_228 >
                                0x166 &&
                            g_current_screen_state.id == W8_SCREEN_MAIN_GAME &&
                            g_level_block != 0) {
                            g_level_block->redraw_flags |= 0x800;
                        }
                    }
                    g_main_game_mode_0068eddc = 0;
                    Function5651F0(g_level_block->enchantment_orb_party_slot);
                }
            } else {
                if (g_level_block->highlight_graphic != 0) {
                    ReleaseObject004257F0(g_level_block->highlight_graphic);
                    g_level_block->highlight_graphic = 0;
                }
                ClearHighlightOverlayRegion();
                g_main_game_mode_0068eddc = 0;
                Function564BA0(g_level_block->condition_orb_party_slot);
            }
        } else {
            Function56AC80();
            if (gXStatus.fPartyMovementUi == 0) {
                goto refresh_tracked_portrait_slot;
            }
        }
    mark_modal_dirty:
        if (g_modal_owner_0068edd0 != 0) {
            g_modal_owner_0068edd0->m_dirty_flags |= 1;
        }
    }
    if ((g_level_block->redraw_flags & 0x20000) != 0) {
        RedrawCombatMonsterList();
    }
    if ((g_level_block->redraw_flags & 0x100) != 0) {
        g_level_block->redraw_flags |= 0x100000;
        RedrawRoofButtons();
        RefreshSpellIconHudRows();
    }
    if ((g_level_block->redraw_flags & 0x100000) != 0 && gXStatus.fCombatMode != 0 &&
        g_combat_state->flag_001 != 0) {
        health_percent = Function4EC610(0);
        GetClippingRect(&saved_clip);
        combat_clip.iRight = (health_percent * 0x11e) / 100 + 0xb1;
        combat_clip.iTop = 0;
        combat_clip.iBottom = 0x1e0;
        combat_clip.iLeft = 0;
        SetClippingRect(&combat_clip);
        DrawCatalogImageAndInvalidate(-0xe, 0x8d, 0, 0, 0xb1, 2, 2, 0);
        SetClippingRect(&saved_clip);
    }
    if (gXStatus.fReviewCharacterMode != 0) {
        if (((g_level_block->redraw_flags & 0x8000) == 0) ||
            g_level_block->condition_orb_party_slot != -1 ||
            g_level_block->enchantment_orb_party_slot != -1 ||
            g_level_block->portrait_overlay_party_slot != -1) {
            show_portraits = 0;
        } else {
            show_portraits = 1;
        }
        Function5B2980(show_portraits);
    }
    for (party_slot = 0; party_slot < 8; ++party_slot) {
        portrait_rect = &g_startup_grid_647da0[party_slot];
        if ((g_level_block->redraw_flags & (1U << (party_slot & 0x1f))) == 0) {
            if (gXStatus.monster_manager_entries[party_slot].field_0bc != 0) {
                RedrawPartyPortraitBars(party_slot,
                                        g_level_block->portrait_refresh_pending[party_slot] == 0);
            }
            if (gXStatus.monster_manager_entries[party_slot].field_0d0 != 0) {
                RedrawKeyboardMenuPanel(0);
            }
        } else {
            keyboard_menu_invalidate = 0;
            if (party_slot == g_level_block->highlight_override ||
                party_slot == g_level_block->formation_highlight_party_slot ||
                party_slot == g_level_block->held_item_display_190) {
                keyboard_menu_invalidate = 1;
            }
            RedrawPartyPortraitOverlay(party_slot, keyboard_menu_invalidate, 1,
                                       g_level_block->portrait_refresh_pending[party_slot] == 0);
            if (g_level_block->portrait_refresh_pending[party_slot] != 0) {
                InvalidateRegion(portrait_rect->x1, portrait_rect->y1, portrait_rect->x2,
                                 portrait_rect->y2, 1);
            }
            if (gXStatus.monster_manager_entries[party_slot].field_0d0 != 0) {
                RedrawKeyboardMenuPanel(1);
            }
        }
    }
    if ((g_level_block->redraw_flags & 0x200) != 0) {
        RedrawLayoutArrowButtons();
        DrawCatalogImageAndInvalidate(-0xe, 0x7e, 0, 1, 0x269, 0x166, 2, 0);
        InvalidateRegion(0x269, 0x166, 0x27f, 0x1c1, 0);
    }
    if (gXStatus.fSpellCastMode != 0) {
        if ((g_level_block->redraw_flags & 0x800) != 0) {
            InvalidateSpellCastingDescription005A0300();
        }
        if ((g_level_block->redraw_flags & 0x200) == 0) {
            SetSpellCastingPanelsActive005A0270(0);
        } else {
            SetSpellCastingPanelsActive005A0270(1);
        }
        goto finish_mode_overlays;
    }
    if (gXStatus.fItemSelectMode != 0) {
        if ((g_level_block->redraw_flags & 0x800) != 0) {
            RedrawPanel69B998();
        }
        if ((g_level_block->redraw_flags & 0x200) == 0) {
            Function59CF50(0);
        } else {
            Function59CF50(1);
        }
        goto finish_mode_overlays;
    }
    if (gXStatus.fNpcDialogueMode == 0 || CanOpenNpcDialogue() != 0) {
        if (gXStatus.fLockInteractMode != 0) {
            if ((g_level_block->redraw_flags & 0x200) != 0) {
                Function587C50();
            }
            goto finish_mode_overlays;
        }
        if (gXStatus.fTrapInteractMode != 0) {
            if ((g_level_block->redraw_flags & 0x200) != 0) {
                RedrawTextBoxComplete();
            }
            goto finish_mode_overlays;
        }
        redraw_flags = g_level_block->redraw_flags & 0x400;
        if (redraw_flags == 0 || g_level_block->radar_map_visible == 0) {
            if (redraw_flags != 0) {
                ClearSurfaceRect(0x17, 0x166, 0x80, 0x1c2);
            }
        } else {
            DrawCatalogImageAndInvalidate(-0xe, 0x84, 0, 0, 0x17, 0x166, 2, 0);
        }
        redraw_flags = g_level_block->redraw_flags & 0x800;
        if (redraw_flags == 0 || g_level_block->action_panel_visible == 0) {
            if (redraw_flags != 0) {
                ClearSurfaceRect(0x80, 0x166, 0x200, 0x1c2);
            }
        } else {
            DrawCatalogImageAndInvalidate(-0xe, 0x87, 0, 0, 0x80, 0x166, 2, 0);
            text_box_mode = GetTextBoxMode();
            if (text_box_mode == 0 || text_box_mode == 1) {
                text_box_variant = text_box_mode;
            } else if (text_box_mode == 3) {
                text_box_variant = 2;
            } else {
                FormatDebugMessage(0, g_warning_drawing_text_box_while_text_buffer_0064810c,
                                   text_box_mode);
                goto draw_formation_panel;
            }
            DrawCatalogImageAndInvalidate(-0xe, 0x88, 0, text_box_variant, 0x8a, 0x168, 2, 0);
        }
    draw_formation_panel:
        redraw_flags = g_level_block->redraw_flags & 0x4000;
        if (redraw_flags == 0 || g_level_block->formation_board_visible == 0) {
            if (redraw_flags != 0) {
                ClearSurfaceRect(0x200, 0x166, 0x268, 0x1c2);
            }
        } else {
            DrawCatalogImageAndInvalidate(-0xe, 0x83, 0, 0, 0x200, 0x166, 2, 0);
        }
        goto finish_mode_overlays;
    }
    if ((g_level_block->redraw_flags & 0x800) != 0) {
        InvalidateMainGameActionPanelRect(0);
    }
    if ((g_level_block->redraw_flags & 0x200) == 0) {
        if (Function56ED80() != 0 || GetOpenDialogueFlag() != 0) {
            Function56ECF0(0);
        }
    } else {
        Function56ECF0(1);
    }
    if (GetOpenDialogueFlag() != 0) {
        Function58C790();
    }
finish_mode_overlays:
    if ((g_level_block->redraw_flags & 0x800) != 0) {
        RedrawTextBoxScrollChrome();
    }
    if ((g_level_block->redraw_flags & 0x1000) != 0) {
        RedrawSubMenuButtons();
    }
    if ((g_level_block->redraw_flags & 0x200000) != 0) {
        DrawSubMenuCharacterAction();
    }
    if (g_level_block->combat_end_notification != -1) {
        RefreshSubMenuPanel((g_level_block->redraw_flags & 0x2000) != 0);
    }
    if ((g_level_block->redraw_flags & 0x40000) != 0) {
        RedrawOptionsDiskButton();
    }
    if ((g_level_block->redraw_flags & 0x80000) != 0) {
        RedrawCombatStanceButtons();
    }
}

/* Finish the main-game frame: sync facing/radar, advance notices, run the two
   redraw passes, paint overlays, and present. Callers: MainGameScreenFrame at
   0x0055ff1e (early exit on pending transition) and 0x00560649 (normal end).
   The assert path that can fire just before the normal call is
   "iNextChar != BAD_INDEX" in MainGameScreenFrame (retail line 0x1047). */
// FUNCTION: WIZ8 0x00562A80
void DrawMainGameScreen(void)
{
    int stopped_line;
    srNode* node;

    SyncPartyFacingFromCamera();
    UpdateRadarBlips();
    if (g_level_block->flag_218 != 0 && ClockIsTicking(g_level_block->clock_214) == 0) {
        g_level_block->flag_218 = 0;
        RequestPartySlotRedraw(g_status_685170.selected_character);
    }
    if (g_status_685170.text_box_lines_shown_49a7[g_status_685170.text_line_cursor_1795] <
            g_status_685170.text_box_lines_used_4997[g_status_685170.text_line_cursor_1795] &&
        ScreenLifecycleSuccess() != 0) {
        AdvanceNoticeLine(g_status_685170.text_line_cursor_1795);
    }
    if (g_level_block->action_panel_visible == 0 && gXStatus.fNpcDialogueMode == 0 &&
        gXStatus.fLockInteractMode == 0 && gXStatus.fTrapInteractMode == 0 &&
        gXStatus.fSpellCastMode == 0 && gXStatus.fItemSelectMode == 0) {
        stopped_line = FindStoppedTextLine();
        if (stopped_line != -1 &&
            g_level_block->text_lines[4 + g_status_685170.text_line_cursor_1795] !=
                static_cast<unsigned int>(stopped_line)) {
            if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
                g_level_block->redraw_flags |= 0x800;
            }
            SetRendererModePair();
        }
    }
    if (IsMessageBoxActive() != 0) {
        g_level_block->flag_0f0 = 1;
    } else if (g_level_block->flag_0f0 != 0) {
        g_level_block->flag_0f0 = 0;
    }
    g_level_block->saved_redraw_flags = g_level_block->redraw_flags;
    if (g_level_block->redraw_flags != 0) {
        ApplyMainGameRedrawFlags();
    }
    if (g_level_block->saved_redraw_flags != 0) {
        ApplySavedRedrawInvalidates();
    }
    g_level_block->redraw_flags = 0;
    g_level_block->saved_redraw_flags = 0;
    if (g_level_block->flag_0f0 != 0) {
        RenderMessageBox();
    }
    if (IsScreenTransitionPending() == 0) {
        if (g_modal_owner_0068edd0 != 0) {
            DrawDialog(g_modal_owner_0068edd0);
        }
        if (g_level_block->combat_end_notification != -1) {
            UpdateSubMenuAutoClose00598FA0();
        }
        if (gXStatus.fCombatMode == 0) {
            UpdatePortraitAdvanceButtons0059BC10();
        } else {
            RedrawCombatPortraits0059B720();
        }
        UpdateConditionButtons0059C080();
        if (gXStatus.fPartyMovementUi != 0 && g_flag_006840bc == 0) {
            DrawPartyMovementPanel();
        }
        DrawNpcQuoteBubble();
        if (gXStatus.fLockInteractMode != 0) {
            RedrawLockInteractionPanels();
        }
        if (gXStatus.fTrapInteractMode != 0) {
            RedrawTrapInteractionPanels();
        }
        UpdateMainGameButtons005989B0();
        RedrawPortraitQuoteBubbles();
        RenderAllTextFields();
        if (g_level_block->unknown_24c != 0) {
            ClearSurfaceRect(0xdc, 0x1e, 0x154, 0x26);
            SetFont(g_smfnt_font_683694);
            SetFontObjectPalette16BPP(g_smfnt_font_683694, g_font_palette_smfnt_68ee10);
            gprintfDirty(0xdc, 0x1e, const_cast<UINT16*>(g_format_mouselook_angles_006480f4),
                         g_mouselook_pending_pitch_0068ede4, g_mouselook_pending_yaw_0068ede0);
        }
        if (GetTickCount() - g_level_block->tick_274 > 499) {
            if (g_level_block->flag_24d != 0 && gXStatus.fSpellCastMode == 0 &&
                gXStatus.fNpcDialogueMode == 0 && gXStatus.fItemSelectMode == 0 &&
                gXStatus.fLockInteractMode == 0 && gXStatus.fTrapInteractMode == 0) {
                DrawVideoInspector00427460(0xdc, 0x32);
            }
            g_level_block->tick_274 = GetTickCount();
        }
        if (g_level_block->formation_board_visible != 0) {
            CreateFormationBoardOverlay();
        }
        if (g_level_block->radar_map_visible != 0) {
            EnsureRadarMapOverlay();
        }
        if (g_flag_006840bd == 0 || gXStatus.fCombatMode != 0) {
            UpdateWorlds0044F400();
        }
        if (g_flag_0068edda != 0) {
            for (node = g_world->level->firstChild(); node != 0; node = node->nextSibling()) {
                if (node->getClassID() == 0x10004) {
                    node->clearFlag(srNode::FLAG_DISABLE);
                }
            }
        }
        RenderFrame();
        if (g_flag_0068edda != 0) {
            for (node = g_world->level->firstChild(); node != 0; node = node->nextSibling()) {
                if (node->getClassID() == 0x10004) {
                    if (MeasureNodeRender00428830(node) == 0) {
                        node->setFlag(srNode::FLAG_DISABLE);
                    } else {
                        node->clearFlag(srNode::FLAG_DISABLE);
                    }
                }
            }
            g_flag_0068edda = 0;
        }
        if (g_flag_689b32 != 0) {
            g_status_685170.flag_49c1 = 1;
        }
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

#define MAIN_GAME_SCREEN_CPP "C:\\Projects\\Wizardry 8\\Local Screens\\MainGameScreen.cpp"

/* Redraw the combat monster-group list at the left edge of the main screen:
   clear the previous rows, walk live groups, format each line, tint the
   hovered action group and current target, and resize input region 0xe5. */
// FUNCTION: WIZ8 0x00565440
void RedrawCombatMonsterList(void)
{
    unsigned int group_list_index;
    unsigned int live_row_count;
    unsigned int max_text_width;
    unsigned int list_length;
    int row_y;
    W8MonsterGroup* monster_group;
    short text_width;
    bool is_action_group;
    bool is_target_group;
    int context_slot;
    W8CombatSlot* target;
    unsigned short* palette;
    unsigned int monster_index;
    W8MonsterInfo* monster_info;
    wchar_t* scratch_text;

    live_row_count = 0;
    max_text_width = 0;
    row_y = 0x19;
    scratch_text = g_level_block->text_paint_scratch_000;
    if (g_level_block->value_284 != 0) {
        unsigned int clear_bottom = g_level_block->value_284 * 0xb + 0x1a;
        int clear_right = g_level_block->value_288 + 0xfb;
        ClearSurfaceRect(0xfa, 0x19, clear_right, clear_bottom);
        InvalidateRegion(0xfa, 0x19, clear_right, clear_bottom, 1);
    }
    if (gXStatus.active_monster_count != 0) {
        SetFont(g_font_683660);
        group_list_index = 0;
        list_length = PLLength(gXStatus.plsMonsterGroupList);
        if (list_length != 0) {
            do {
                monster_group = GetMonsterGroupByListIndex(group_list_index);
                if (IsMonsterGroupLive(monster_group) != 0) {
                    swprintf(scratch_text, g_format_d_s_paren_d_slash_d_slash_d_00648170,
                             monster_group->member_count, GetMonsterGroupName(monster_group),
                             monster_group->active_member_count,
                             monster_group->selectable_member_count,
                             monster_group->visible_member_count);
                    text_width = StringPixLength(scratch_text, g_font_683660);
                    is_action_group = 0;
                    if (monster_group->group_id == g_level_block->value_28c) {
                        is_action_group = 1;
                    } else if (g_level_block->highlighted_item != -1) {
                        monster_index = MonsterGetIndexByLocationID(
                            0xfc8, MAIN_GAME_SCREEN_CPP, g_level_block->highlighted_item, 1);
                        monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
                        is_action_group = monster_group->group_id == monster_info->monster_group_id;
                    }
                    is_target_group = 0;
                    context_slot = g_level_block->tooltip_subject;
                    if (g_level_block->tooltip_kind != 0 || context_slot == -1) {
                        context_slot = g_status_685170.selected_character;
                    }
                    target = GetTargetBlockForContext(context_slot, W8_TARGETING_CONTEXT_CURRENT);
                    if (target->iType == W8_TARGET_KIND_GROUP &&
                        monster_group->group_id == target->iGroupID) {
                        is_target_group = 1;
                    } else if (target->iType == W8_TARGET_KIND_MONSTER) {
                        monster_index = MonsterGetIndexByLocationID(0xfe4, MAIN_GAME_SCREEN_CPP,
                                                                    target->iMonsterID, 0);
                        if (monster_index != 0xffffffff) {
                            monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
                            is_target_group =
                                monster_group->group_id == monster_info->monster_group_id;
                        }
                    }
                    if (is_action_group != 0) {
                        palette = g_font_state_palettes_68ee1c[0];
                        if (g_level_block->unknown_290[0] != 0) {
                            palette = g_font_state_palettes_68ee1c[1];
                        }
                    } else {
                        palette = g_font_state_palettes_68ee1c[3];
                        if (is_target_group == 0) {
                            palette = g_colour_68ee08;
                        }
                    }
                    SetFontObjectPalette16BPP(g_font_683660, palette);
                    gprintfDirty(0xfa, row_y, const_cast<UINT16*>(g_format_s_006068e4),
                                 scratch_text);
                    row_y = row_y + 0xb;
                    live_row_count = live_row_count + 1;
                    if (max_text_width < static_cast<unsigned int>(text_width)) {
                        max_text_width = static_cast<unsigned int>(text_width);
                    }
                    if (0x18 < live_row_count) {
                        break;
                    }
                }
                group_list_index = group_list_index + 1;
                list_length = PLLength(gXStatus.plsMonsterGroupList);
            } while (group_list_index < list_length);
        }
    }
    if (live_row_count != static_cast<unsigned int>(g_level_block->value_284) ||
        max_text_width != static_cast<unsigned int>(g_level_block->value_288)) {
        g_monster_list_right_647f84 = max_text_width + 0xfa;
        g_monster_list_bottom_647f88 = row_y;
        if (live_row_count == 0) {
            DisableRegionInput(0xe5);
        } else {
            SetRegionBounds(0xe5, 0xfa, 0x19, static_cast<unsigned short>(max_text_width) + 0xf9,
                            static_cast<unsigned short>(live_row_count) * 0xb + 0x18);
            EnableRegionInput(0xe5);
        }
        g_level_block->value_284 = live_row_count;
        g_level_block->value_288 = max_text_width;
    }
}

/* Move the selection to another party slot. Re-selecting the current slot
   only re-aims the camera; a real change redraws both slots, hands the new
   slot to each open mode, drops the submenu and keyboard menu state and
   queues the slot's pick quote when not in combat. */
// FUNCTION: WIZ8 0x00565740
void SelectPartyCharacter(int party_slot)
{
    W8Character* character;
    int previous;

    if (g_status_685170.buffers.party_rows[party_slot].occupied == 0) {
        ReportAssertion("gStatus->XChar[uiChar].fOccupied",
                        "C:\\Projects\\Wizardry 8\\Local Screens\\MainGameScreen.cpp", 0x1053);
    }
    character = &g_status_685170.buffers.characters[party_slot];
    if (character->highest_condition >= 0x13) {
        return;
    }
    if (g_status_685170.selected_character == party_slot) {
        FaceCameraToSelection(party_slot);
        return;
    }
    if (gXStatus.fCombatMode == 0) {
        QueueCharacterEvent(character, g_special_event_0068c568, 0, g_effect_argument_005ed8c8,
                            g_effect_argument_005ed914);
    }
    if (g_status_685170.selected_character != -1) {
        RequestPartySlotRedraw(g_status_685170.selected_character);
    }
    previous = g_status_685170.selected_character;
    g_status_685170.selected_character = party_slot;
    RequestPartySlotRedraw(party_slot);
    if (previous != -1 && (gXStatus.fSpellCastMode != 0 || gXStatus.fItemSelectMode != 0)) {
        Function53B050(previous);
    }
    g_level_block->clock_214 = SetCountdownClock(500);
    g_level_block->flag_218 = 1;
    if (g_level_block != 0) {
        if (gXStatus.fCombatMode != 0) {
            g_level_block->refresh_combat_panel = 1;
        }
        g_level_block->refresh_party_panel = 1;
    }
    FaceCameraToSelection(party_slot);
    if (gXStatus.fSpellCastMode != 0) {
        SelectSpellCastingCharacter(g_status_685170.selected_character);
    }
    if (gXStatus.fItemSelectMode != 0) {
        Function59CC40(g_status_685170.selected_character);
    }
    if (gXStatus.fNpcDialogueMode != 0) {
        Function56EE20(g_status_685170.selected_character);
    }
    if (gXStatus.fLockInteractMode != 0) {
        Function587A30();
    }
    if (gXStatus.fTrapInteractMode != 0) {
        Function58A860();
    }
    if (gXStatus.fReviewCharacterMode != 0) {
        SelectFormationSlotCell(g_status_685170.selected_character);
    }
    if (g_level_block->combat_end_notification != -1) {
        ResetSubMenuPanel();
    }
    if (g_level_block->keyboard_menu_open != 0 && party_slot != GetValue64C1C8()) {
        CloseKeyboardMenu();
    }
    SetTargetingMode(0);
    if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
        g_level_block->redraw_flags |= 1 << (party_slot & 0x1f) | 0x201000;
    }
    g_level_block->pick_changed_154 = 1;
}

// FUNCTION: WIZ8 0x00561a20
void RefreshSelectedPartyPortrait(unsigned int party_slot)
{
    if (g_level_block == 0 || g_level_block->main_ui_mode == W8_MAIN_UI_MODE_PORTRAITS ||
        g_level_block->portrait_refresh_pending[party_slot] != 0) {
        return;
    }
    if (g_level_block->keyboard_menu_open != 0 &&
        party_slot == static_cast<unsigned int>(g_value_64c1c8) &&
        gXStatus.monster_manager_entries[party_slot].field_0bd == 0 &&
        gXStatus.monster_manager_entries[party_slot].field_09c == 0) {
        if (gXStatus.monster_manager_entries[party_slot].quote.quote_handle == -1 ||
            g_settings_6850c8.pc_subtitles == 0) {
            return;
        }
    }

    if (g_level_block->condition_highlight_party_slot == static_cast<int>(party_slot)) {
        g_level_block->condition_highlight_party_slot = -1;
        if (g_level_block->highlight_graphic != 0) {
            ReleaseObject004257F0(g_level_block->highlight_graphic);
            g_level_block->highlight_graphic = 0;
        }
        ClearHighlightOverlayRegion();
        g_main_game_mode_0068eddc = 0;
        RequestRedraw(0x8000 | 0xff);
    } else if (g_level_block->condition_highlight_party_slot != -1) {
        RequestRedraw(0x8000);
    }

    if (g_level_block->portrait_overlay_party_slot == static_cast<int>(party_slot)) {
        g_level_block->portrait_overlay_party_slot = -1;
        if (g_level_block->highlight_graphic != 0) {
            ReleaseObject004257F0(g_level_block->highlight_graphic);
            g_level_block->highlight_graphic = 0;
        }
        ClearHighlightOverlayRegion();
        g_main_game_mode_0068eddc = 0;
        RequestRedraw(0x8000 | 0xff);
    } else if (g_level_block->portrait_overlay_party_slot != -1) {
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
    g_level_block->portrait_refresh_image[party_slot] = 0x69;
    g_level_block->portrait_refresh_mode[party_slot] = 6;
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

/* Build the mode-6 hover panel over a portrait: the framed plate carrying the
   character's name and `row_count` content rows, plus the lazily created 0x72
   highlight sprite pinned to highlight_row. An active mode-3 NPC dialogue,
   mode-5 panel or previous overlay is torn down first; the panel is anchored
   off the slot's portrait column and clamped inside the viewport. The retail
   assertion names the sprite gpMGSV->pHighlightGraphic. */
// FUNCTION: WIZ8 0x00563FC0
void DrawHighlightOverlay(unsigned int party_slot, int row_count, unsigned int min_width)
{
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
    }
    g_main_game_mode_0068eddc = 6;

    wchar_t* name = g_status_685170.buffers.characters[party_slot].name;
    int width = StringPixLength(name, g_wiz_text_font_683640);
    if (min_width < static_cast<unsigned int>(width)) {
        width = StringPixLength(name, g_wiz_text_font_683640);
        min_width = width;
    }
    min_width += 6;
    unsigned int tiles = min_width / 6;
    if (min_width % 6 != 0) {
        ++tiles;
        min_width = tiles * 6;
    }
    unsigned int panel_width = min_width + 0xc;

    int left;
    unsigned char pending = g_level_block->portrait_refresh_pending[party_slot];
    if ((party_slot & 1) == 0) {
        left = g_level_block->portrait_refresh_image[party_slot] -
               g_level_block->portrait_hover_x_origin + 0x80;
        if (pending == 0 && (g_level_block->condition_highlight_party_slot != -1 ||
                             g_level_block->portrait_overlay_party_slot != -1)) {
            left += 0x18;
        }
    } else {
        left = g_level_block->portrait_hover_x_origin -
               g_level_block->portrait_refresh_image[party_slot] - static_cast<int>(panel_width) +
               0x200;
        if (pending == 0 && (g_level_block->condition_highlight_party_slot != -1 ||
                             g_level_block->portrait_overlay_party_slot != -1)) {
            left -= 0x18;
        }
    }

    unsigned int content_height = row_count * 9 + 0x12;
    int row_y = (party_slot >> 1) * 0x55;
    int top = g_viewport_modes_647d30[g_level_block->camera_mode_100].top;
    int candidate = row_y - static_cast<int>(content_height) + 0x3c;
    if (candidate >= top) {
        if (candidate + content_height * 2 > 0x166) {
            top = 0x166 - content_height * 2;
        } else {
            top = candidate;
            if (pending == 0 && g_level_block->condition_highlight_party_slot != -1) {
                top = row_y + 0x12;
            }
        }
    }

    g_level_block->hover_overlay_row_count = row_count;
    g_level_block->dialogue_x_220 = left;
    g_level_block->dialogue_y_224 = top;
    g_level_block->dialogue_height_228 = content_height * 2;
    g_level_block->dialogue_width_238 = panel_width;
    g_level_block->dialogue_text_x_230 = left + 9;
    g_level_block->dialogue_text_width_234 = panel_width - 0x12;

    unsigned int tile;
    int row;
    int inner;
    DrawCatalogImage(-0xe, 0x70, 0, 0, left, top, 2, 0);
    inner = left + 6;
    for (tile = tiles; tile != 0; --tile) {
        DrawCatalogImage(-0xe, 0x70, 0, 1, inner, top, 2, 0);
        inner += 6;
    }
    int right = left + (tiles * 3 + 3) * 2;
    DrawCatalogImage(-0xe, 0x70, 0, 2, right, top, 2, 0);
    int row_y_pos = top + 6;
    for (row = 0; row < 2; ++row) {
        DrawCatalogImage(-0xe, 0x70, 0, 3, left, row_y_pos, 2, 0);
        inner = left + 6;
        for (tile = tiles; tile != 0; --tile) {
            DrawCatalogImage(-0xe, 0x70, 0, 8, inner, row_y_pos, 2, 0);
            inner += 6;
        }
        DrawCatalogImage(-0xe, 0x70, 0, 4, right, row_y_pos, 2, 0);
        row_y_pos += 6;
    }

    SetFont(g_wiz_text_font_683640);
    SetFontObjectPalette16BPP(
        g_wiz_text_font_683640,
        g_font_state_palettes_68ee1c[g_status_685170.buffers.party_rows[party_slot]
                                         .party_order_index]);
    int font_height = GetFontHeight(g_wiz_text_font_683640);
    int name_width = StringPixLength(name, g_wiz_text_font_683640);
    gprintf(left + static_cast<int>(panel_width >> 1) - name_width / 2,
            (0xc - font_height) / 2 + top + 6, const_cast<wchar_t*>(g_format_s_006068e4), name);
    SetFontObjectPalette16BPP(g_wiz_text_font_683640, g_font_palette_wiz_text_68ee14);

    row_y_pos = top + 0x12;
    DrawCatalogImage(-0xe, 0x70, 0, 3, left, row_y_pos, 2, 0);
    DrawCatalogImage(-0xe, 0x71, 0, 0, left + 6, row_y_pos, 2, 0);
    if (tiles > 2) {
        inner = left + 0xc;
        for (tile = 0; tile < tiles - 2; ++tile) {
            DrawCatalogImage(-0xe, 0x71, 0, 1, inner, row_y_pos, 2, 0);
            inner += 6;
        }
    }
    DrawCatalogImage(-0xe, 0x71, 0, 2, left + tiles * 6, row_y_pos, 2, 0);
    DrawCatalogImage(-0xe, 0x70, 0, 4, right, row_y_pos, 2, 0);

    row_y_pos += 6;
    g_level_block->dialogue_row_y_22c = row_y_pos + 6;
    for (row = row_count * 3 + 1; row != 0; --row) {
        DrawCatalogImage(-0xe, 0x70, 0, 3, left, row_y_pos, 2, 0);
        inner = left + 6;
        for (tile = tiles; tile != 0; --tile) {
            DrawCatalogImage(-0xe, 0x70, 0, 8, inner, row_y_pos, 2, 0);
            inner += 6;
        }
        DrawCatalogImage(-0xe, 0x70, 0, 4, right, row_y_pos, 2, 0);
        row_y_pos += 6;
    }
    DrawCatalogImage(-0xe, 0x70, 0, 5, left, row_y_pos, 2, 0);
    inner = left + 6;
    for (tile = tiles; tile != 0; --tile) {
        DrawCatalogImage(-0xe, 0x70, 0, 6, inner, row_y_pos, 2, 0);
        inner += 6;
    }
    DrawCatalogImage(-0xe, 0x70, 0, 7, right, row_y_pos, 2, 0);

    if (g_level_block->highlight_graphic == 0) {
        unsigned int video_object = GetCatalogVideoObjectHandle(0x72, 0);
        if (video_object != 0) {
            unsigned int surface;
            if (MakeVSurfaceFromVObject(video_object, 0, &surface) != 0) {
                g_level_block->highlight_graphic = CreateSpriteFromSurface(surface, 0, 0, 0, 1);
                ReleaseVideoSurface(surface);
            }
        }
    }
    if (g_level_block->highlight_graphic == 0) {
        srAssertFail("gpMGSV->pHighlightGraphic",
                     "C:\\Projects\\Wizardry 8\\Local Screens\\MainGameScreen.cpp", 0xd84, 0);
    }
    SetModelInstance2DDisplayState004264F0(g_level_block->highlight_graphic, 4);
    Position2DNodeUnsnapped004257D0(
        g_level_block->highlight_graphic, g_level_block->dialogue_x_220 + 7,
        g_level_block->dialogue_row_y_22c - 1 + g_level_block->highlight_row * 0x12);
    InvalidateRegion(g_level_block->dialogue_x_220, g_level_block->dialogue_y_224,
                     g_level_block->dialogue_x_220 + g_level_block->dialogue_width_238,
                     g_level_block->dialogue_y_224 + g_level_block->dialogue_height_228, 1);
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

/* Second-pass invalidate rectangles keyed by saved_redraw_flags bits. The
   first eight entries are the portrait-slot columns (zero base rects, scaled
   by portrait_hover_x_origin and shifted by portrait_refresh_image). */
struct W8MainGameInvalidateRect {
    int left;
    int top;
    int right;
    int bottom;
    int x_scale;
    int apply_y_offset;
    unsigned int refresh_image_index;
};

// GLOBAL: WIZ8 0x00647DA0
W8MainGameInvalidateRect g_main_game_invalidate_rects_647da0[23] = {
    {0, 0, 0, 0, -1, 0, 0},          {0, 0, 0, 0, 1, 0, 1},
    {0, 0, 0, 0, -1, 0, 2},          {0, 0, 0, 0, 1, 0, 3},
    {0, 0, 0, 0, -1, 0, 4},          {0, 0, 0, 0, 1, 0, 5},
    {0, 0, 0, 0, -1, 0, 6},          {0, 0, 0, 0, 1, 0, 7},
    {0, 0, 639, 16, 0, 0, ~0u},      {0, 358, 639, 449, 0, 1, ~0u},
    {23, 360, 127, 449, 0, 1, ~0u},  {129, 358, 510, 449, 0, 1, ~0u},
    {31, 450, 606, 476, 0, 1, ~0u},  {469, 382, 500, 461, 0, 1, ~0u},
    {512, 358, 616, 449, 0, 1, ~0u}, {0, 0, 0, 0, 0, 0, ~0u},
    {127, 260, 513, 356, 0, 0, ~0u}, {250, 25, 0, 0, 0, 0, ~0u},
    {1, 450, 29, 476, 0, 1, ~0u},    {293, 450, 524, 476, 0, 1, ~0u},
    {612, 452, 636, 473, 0, 1, ~0u}, {0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0},
};

/* Second redraw pass: invalidate each rectangle whose bit is set in
   saved_redraw_flags, applying the mode-6 portrait layout offsets. */
// FUNCTION: WIZ8 0x00563D00
void ApplySavedRedrawInvalidates(void)
{
    unsigned char bit;
    W8MainGameInvalidateRect* entry;
    int left;
    int top;
    int right;
    int bottom;
    int delta;

    if (g_level_block->saved_redraw_flags == static_cast<unsigned int>(-1)) {
        ClearVideoDirtyBlocks00423150();
        return;
    }

    for (bit = 0, entry = g_main_game_invalidate_rects_647da0; bit < 23; ++bit, ++entry) {
        if ((g_level_block->saved_redraw_flags & (1u << (bit & 0x1f))) == 0) {
            continue;
        }
        left = entry->left;
        top = entry->top;
        right = entry->right;
        bottom = entry->bottom;
        if (entry->x_scale != 0) {
            delta = g_level_block->portrait_hover_x_origin * entry->x_scale;
            left += delta;
            right += delta;
        }
        if (entry->apply_y_offset != 0) {
            top += g_level_block->unknown_160;
            bottom += g_level_block->unknown_160;
        }
        if (entry->refresh_image_index != ~0u) {
            delta = g_level_block->portrait_refresh_image[entry->refresh_image_index];
            if ((entry->refresh_image_index & 1) == 0) {
                left += delta;
                right += delta;
            } else {
                left -= delta;
                right -= delta;
            }
        }
        InvalidateRegion(left, top, right + 1, bottom + 1, 1);
    }
}

/* Active viewport rectangle plus the preceding retail dword at 0x00647f40
   (-1). Bundling the non-zero sentinel forces VC6 to emit the block in
   .data; separate `int x = 0` definitions land in .bss and fail datacmp. */
struct W8ActiveViewport647F40 {
    int sentinel_647f40;
    int left;
    int top;
    int right;
    int bottom;
};

// GLOBAL: WIZ8 0x00647f40
W8ActiveViewport647F40 g_active_viewport_647f40 = {-1, 0, 0, 0, 0};

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
    g_active_viewport_647f40.left = rect->left;
    g_active_viewport_647f40.top = rect->top;
    g_active_viewport_647f40.right = rect->right;
    g_active_viewport_647f40.bottom = rect->bottom;
    SetViewport(g_active_viewport_647f40.left, g_active_viewport_647f40.top,
                g_active_viewport_647f40.right, g_active_viewport_647f40.bottom);
    g_level_block->camera_mode_100 = mode;
}

/* True when the cursor hotspot lies in the formation-mode portrait rect for
   `party_slot`, accounting for the hover panel's x origin and the slot's
   mirrored even/odd layout. */
// FUNCTION: WIZ8 0x00561980
bool IsPartyPortraitUnderCursor00561980(unsigned int party_slot)
{
    W8StartupGridRow& row = g_startup_grid_647da0[party_slot];
    int image = g_level_block->portrait_refresh_image[party_slot];
    int left;
    int right;

    if ((party_slot & 1) == 0) {
        left = (row.x1 - g_level_block->portrait_hover_x_origin) + image;
        right = (row.x2 - g_level_block->portrait_hover_x_origin) + image;
    } else {
        left = (row.x1 - image) + g_level_block->portrait_hover_x_origin;
        right = (row.x2 - image) + g_level_block->portrait_hover_x_origin;
    }
    if (right < 0) {
        return 0;
    }
    if (left < 0) {
        left = 0;
    }
    return IsCursorInRectangle(left, row.y1, right, row.y2) != 0;
}

/* While the main UI is not in portrait mode, keep each party slot's formation
   portrait refresh latch in sync with dirty flags, occupancy, and hover. */
// FUNCTION: WIZ8 0x0059B2D0
void UpdateFormationPortraitRefresh0059B2D0(void)
{
    unsigned int party_slot;
    W8MonsterManagerEntry* entry;

    for (party_slot = 0; party_slot < 8; ++party_slot) {
        entry = &gXStatus.monster_manager_entries[party_slot];
        if (g_level_block->portrait_refresh_pending[party_slot] == 0) {
            if (entry->field_09c != 0 || entry->field_0bd != 0 ||
                (entry->portrait_event_active != 0 && gfCapturingVideo == 0)) {
                RefreshSelectedPartyPortrait(party_slot);
                entry->field_0cf = 1;
            }
        } else if (g_status_685170.buffers.party_rows[party_slot].occupied == 0) {
            ClearPortraitRefreshSlot(static_cast<int>(party_slot));
            entry->field_0cf = 0;
            DisableRegionInput(party_slot + 0x5a);
        } else if (entry->field_0ce == 0 && entry->field_09c == 0 && entry->field_0bd == 0 &&
                   entry->portrait_event_active == 0) {
            if (IsPartyPortraitUnderCursor00561980(party_slot) == 0 || entry->field_0cf != 0) {
                ClearPortraitRefreshSlot(static_cast<int>(party_slot));
                entry->field_0cf = 0;
            }
        }
    }
}

/* Clear one party slot's pending portrait refresh while the screen is not in
   portrait mode: drop the pending latch and cached image/mode, wipe the
   startup-grid portrait rect, re-enable the portrait input region and disable
   the slot's region set. */
// FUNCTION: WIZ8 0x00561DB0
void ClearPortraitRefreshSlot(int slot)
{
    W8StartupGridRow* row;

    if (g_level_block->main_ui_mode != W8_MAIN_UI_MODE_PORTRAITS &&
        g_level_block->portrait_refresh_pending[slot] != 0) {
        g_level_block->portrait_refresh_pending[slot] = 0;
        g_level_block->flag_108 = 1;
        gXStatus.monster_manager_entries[slot].field_0ce = 0;
        g_level_block->portrait_refresh_mode[slot] = 0;
        g_level_block->portrait_refresh_image[slot] = 0;
        row = &g_startup_grid_647da0[slot];
        ClearSurfaceRect(row->x1 - 1, row->y1, row->x2 + 1, row->y2);
        InvalidateRegion(row->x1 - 1, row->y1, row->x2 + 1, row->y2, 1);
        if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
            g_level_block->redraw_flags |= 1 << (slot & 0x1f);
        }
        EnableRegionInput(slot + 0x5a);
        RegionSetDisable(slot + 7U);
        DisableRegionSetInput(slot + 7U);
        if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
            g_level_block->redraw_flags |= 0x8000;
        }
    }
}

/* Apply a main-game UI mode: drop any raised formation/radar/action panels,
   optionally re-raise them from settings prefs, clear portrait refresh when
   entering portrait mode, refresh tooltip kind, and sync region/layout state. */
// FUNCTION: WIZ8 0x00562580
void ApplyMainGameModeFlag(W8MainUiMode mode, char enable)
{
    unsigned int slot;

    if (g_settings_6850c8.main_ui_mode == W8_MAIN_UI_MODE_PORTRAITS &&
        (g_level_block->tooltip_kind != 4 || g_level_block->tooltip_subject != -1)) {
        g_level_block->tooltip_pending = 1;
        g_level_block->tooltip_since = GetTickCount();
        g_level_block->tooltip_subject = -1;
        g_level_block->tooltip_kind = 4;
    }
    if (mode == W8_MAIN_UI_MODE_PORTRAITS &&
        (g_level_block->tooltip_kind != 0 || g_level_block->tooltip_subject != -1)) {
        g_level_block->tooltip_pending = 1;
        g_level_block->tooltip_since = GetTickCount();
        g_level_block->tooltip_subject = -1;
        g_level_block->tooltip_kind = 0;
    }
    if (g_level_block->formation_board_visible != 0) {
        g_level_block->formation_board_visible = 0;
        RegionSetDisable(0x13);
        ReleaseFormationBoard();
        if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
            g_level_block->redraw_flags |= 0x8200;
        }
        if (g_flag_0068edc9 != 0) {
            SetViewportMode(GetMainGameViewportMode());
        }
        g_flag_0068edc9 = 0;
    }
    if (g_level_block->radar_map_visible != 0) {
        SetRadarMapVisible(0);
    }
    if (g_level_block->action_panel_visible != 0) {
        SetActionPanelVisible(0);
    }
    if (enable == 0 || gXStatus.fSpellCastMode != 0 ||
        (gXStatus.fNpcDialogueMode != 0 && CanOpenNpcDialogue() == 0) ||
        gXStatus.fLockInteractMode != 0 || gXStatus.fTrapInteractMode != 0 ||
        gXStatus.fItemSelectMode != 0) {
        if (mode == W8_MAIN_UI_MODE_PORTRAITS) {
            for (slot = 0; slot < 8; ++slot) {
                ClearPortraitRefreshSlot(slot);
            }
            g_level_block->unknown_158 = 1;
            goto apply_mode_tail;
        }
    } else {
        if (mode == W8_MAIN_UI_MODE_PORTRAITS) {
            g_level_block->radar_map_visible = 1;
            g_level_block->action_panel_visible =
                g_settings_6850c8.portraits_action_panel_preference;
            g_level_block->formation_board_visible = 1;
            if (g_level_block->formation_board_visible != 0) {
                g_level_block->formation_board_visible = 1;
                g_level_block->formation_board_alternate = 0;
                RegionSetEnable(0x13);
                RefreshFormationBoard();
                if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
                    g_level_block->redraw_flags |= 0x8200;
                }
                if (g_flag_0068edc9 != 1) {
                    SetViewportMode(GetMainGameViewportMode());
                }
                g_flag_0068edc9 = 1;
            }
            if (g_level_block->radar_map_visible != 0) {
                SetRadarMapVisible(1);
            }
            if (g_level_block->action_panel_visible != 0) {
                SetActionPanelVisible(1);
            }
            for (slot = 0; slot < 8; ++slot) {
                ClearPortraitRefreshSlot(slot);
            }
            g_level_block->unknown_158 = 1;
            goto apply_mode_tail;
        }
        if (mode == W8_MAIN_UI_MODE_FORMATION) {
            g_level_block->radar_map_visible = g_settings_6850c8.formation_radar_map_preference;
            g_level_block->action_panel_visible =
                g_settings_6850c8.formation_action_panel_preference;
            g_level_block->formation_board_visible = g_settings_6850c8.formation_board_preference;
            if (g_level_block->formation_board_visible != 0) {
                g_level_block->formation_board_visible = 1;
                g_level_block->formation_board_alternate = 0;
                RegionSetEnable(0x13);
                RefreshFormationBoard();
                if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
                    g_level_block->redraw_flags |= 0x8200;
                }
                if (g_flag_0068edc9 != 1) {
                    SetViewportMode(GetMainGameViewportMode());
                }
                g_flag_0068edc9 = 1;
            }
            if (g_level_block->radar_map_visible != 0) {
                SetRadarMapVisible(1);
            }
            if (g_level_block->action_panel_visible != 0) {
                SetActionPanelVisible(1);
            }
        } else if (mode == W8_MAIN_UI_MODE_RADAR) {
            g_level_block->radar_map_visible = 0;
            g_level_block->action_panel_visible = 0;
            g_level_block->formation_board_visible = 0;
        }
    }
    g_level_block->unknown_158 = 0;
apply_mode_tail:
    g_settings_6850c8.main_ui_mode = mode;
    g_level_block->main_ui_mode = mode;
    ClearHotRegion004F2A80();
    if (gXStatus.fSpellCastMode == 0) {
        if (gXStatus.fNpcDialogueMode == 0) {
            if (gXStatus.fItemSelectMode == 0) {
                if (gXStatus.fLockInteractMode == 0) {
                    if (gXStatus.fTrapInteractMode != 0) {
                        SetValue68F2C4(g_settings_6850c8.main_ui_mode);
                    }
                } else {
                    SetValue68F2B0(g_settings_6850c8.main_ui_mode);
                }
            } else {
                SetValue69B988(g_settings_6850c8.main_ui_mode);
            }
        } else {
            NoOp();
        }
    } else {
        SetSpellCastingMode005A1330(g_settings_6850c8.main_ui_mode);
    }
    if (g_level_block->unknown_158 == 0) {
        g_level_block->portrait_hover_x_origin = 0x69;
        g_level_block->unknown_164 = 6;
    } else {
        g_level_block->portrait_hover_x_origin = 0;
        g_level_block->unknown_164 = 0;
    }
    if (g_level_block->action_panel_visible == 0) {
        g_level_block->unknown_160 = 0x76;
        g_level_block->unknown_168 = 6;
    } else {
        g_level_block->unknown_160 = 0;
        g_level_block->unknown_168 = 0;
    }
    if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
        g_level_block->redraw_flags = 0xffffffff;
    }
    ResetRegions();
    SyncMainGameModeRegions();
}

/* Take the screen for a modal owner and put its region up. */
// FUNCTION: WIZ8 0x005698a0
void OpenModal(W8DialogBase* owner)
{
    g_modal_owner_0068edd0 = owner;
    ActivateDialogRegion(0x138);
}

/* Re-sync each party slot's region set and portrait hit region with the
   slot's occupancy, its monster-entry flag and the display mode - the party
   add/remove entries and the keyboard menu's close run it after slot state
   changes. */
// FUNCTION: WIZ8 0x00561ec0
void RefreshPartySlotRegions(void)
{
    int region_set;
    int slot;

    for (slot = 0; slot < W8_PARTY_SLOT_COUNT; ++slot) {
        region_set = slot + 7;
        if (g_status_685170.buffers.party_rows[slot].occupied == 0) {
            if (g_settings_6850c8.main_ui_mode == W8_MAIN_UI_MODE_PORTRAITS) {
                DisableRegionSetInput(region_set);
            } else if (static_cast<unsigned int>(g_settings_6850c8.main_ui_mode) <=
                       static_cast<unsigned int>(W8_MAIN_UI_MODE_RADAR)) {
                DisableRegionInput(region_set + 0x53);
            }
        } else if (g_settings_6850c8.main_ui_mode == W8_MAIN_UI_MODE_PORTRAITS) {
            if (gXStatus.monster_manager_entries[slot].field_0d0 == 0) {
                EnableRegionSetInput(region_set);
            } else {
                DisableRegionSetInput(region_set);
            }
        } else if (static_cast<unsigned int>(g_settings_6850c8.main_ui_mode) <=
                   static_cast<unsigned int>(W8_MAIN_UI_MODE_RADAR)) {
            if (gXStatus.monster_manager_entries[slot].field_0d0 == 0) {
                if (g_level_block->portrait_refresh_pending[slot] == 0) {
                    EnableRegionInput(region_set + 0x53);
                    RegionSetDisable(region_set);
                } else {
                    DisableRegionInput(region_set + 0x53);
                    RegionSetEnable(region_set);
                    EnableRegionSetInput(region_set);
                }
            } else {
                DisableRegionInput(region_set + 0x53);
                RegionSetDisable(region_set);
                DisableRegionSetInput(region_set);
            }
        }
    }
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

/* Re-arm the main-game region sets after ResetRegions: pick the viewport and
   layout band from the current mode, then re-enable whichever overlays are
   up (combat portraits, condition buttons, spell/NPC/lock/trap/formation/
   radar, party movement, review, keyboard menu, combat submenu). */
// FUNCTION: WIZ8 0x00561FD0
void SyncMainGameModeRegions(void)
{
    SetMouseCursorHotspot(0, 0);
    SyncSystemCursor();
    RegionSetEnable(0x28);
    if (g_level_block->value_284 == 0) {
        DisableRegionInput(0xe5);
    } else {
        EnableRegionInput(0xe5);
    }

    if (g_settings_6850c8.main_ui_mode == W8_MAIN_UI_MODE_PORTRAITS) {
        SetViewportMode(GetMainGameViewportMode());
        RegionSetEnable(0xf);
    } else if (g_settings_6850c8.main_ui_mode == W8_MAIN_UI_MODE_FORMATION) {
        SetViewportMode(GetMainGameViewportMode());
        RegionSetEnable(0x10);
    } else if (g_settings_6850c8.main_ui_mode == W8_MAIN_UI_MODE_RADAR) {
        SetViewportMode(GetMainGameViewportMode());
        RegionSetEnable(0x11);
    }

    RefreshPartySlotRegions();
    if (gXStatus.fCombatMode == 0) {
        RegionSetDisable(4);
        DisableRegionSetInput(4);
        EnablePortraitAdvanceRegions0059BB70();
    } else {
        RegionSetEnable(4);
        DisablePortraitControls0059BB40();
    }
    if (g_settings_6850c8.main_ui_mode == W8_MAIN_UI_MODE_PORTRAITS) {
        DisableConditionButtons0059C030();
    } else {
        EnableConditionButtons0059BFC0();
    }

    if (g_settings_6850c8.main_ui_mode != W8_MAIN_UI_MODE_RADAR) {
        if (g_level_block->action_panel_visible != 0) {
            RegionSetEnable(0x14);
            EnableRegionInput(0x59);
            EnableRegionInput(0x52);
            EnableRegionInput(0x53);
            EnableRegionInput(0x54);
            EnableRegionInput(0x55);
            EnableRegionInput(0x56);
            EnableRegionInput(0x57);
            EnableRegionInput(0x58);
        }
        if (gXStatus.fSpellCastMode != 0) {
            RegionSetEnable(0x19);
            RestoreSpellCastingRegions();
        } else if (gXStatus.fNpcDialogueMode != 0 && CanOpenNpcDialogue() == 0) {
            RegionSetEnable(0x18);
        } else if (gXStatus.fItemSelectMode != 0) {
            RegionSetEnable(0x1a);
            RestoreSpellCastingRegions();
        } else if (gXStatus.fLockInteractMode != 0) {
            EnableLockInteractionPanels();
        } else if (gXStatus.fTrapInteractMode != 0) {
            EnableTrapInteractionPanelRegions();
        } else {
            if (g_level_block->formation_board_visible != 0) {
                RegionSetEnable(0x13);
            }
            if (g_level_block->radar_map_visible != 0) {
                RegionSetEnable(0x12);
            }
        }
    }

    if (gXStatus.fPartyMovementUi != 0 && g_flag_006840bc == 0) {
        UpdatePartyMovementPanel();
    }
    if (gXStatus.fReviewCharacterMode != 0) {
        RegionSetEnable(0x1b);
    }
    if (g_level_block->keyboard_menu_open != 0) {
        EnableKeyboardMenuInput();
    }
    if (g_level_block->combat_end_notification != -1) {
        EnableSubMenuRegions();
    }
    if (g_level_block != 0) {
        if (gXStatus.fCombatMode != 0) {
            g_level_block->refresh_combat_panel = 1;
        }
        g_level_block->refresh_party_panel = 1;
    }
    if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
        g_level_block->redraw_flags |= 0x100;
    }
}

/* Clear whatever the screen was waiting on and hand the tenth reason to the
   frame. */
// FUNCTION: WIZ8 0x00565970
void ClearScreenWait(void)
{
    g_pending_screen_state.mode = 0;
    SetPendingScreenState(W8_SCREEN_OPTIONS);
}

/* Party portrait hit region: left-click selects / aims, right-hold opens camp,
   drag-hover arms the mode-6 overlay, and MOUSE_POS drives help plus targeting
   cursor. Skips the slot parked in g_value_006840be. */
// FUNCTION: WIZ8 0x00565990
unsigned char PortraitSelectRegionEvent(const InputAtom* event, W8Region* region)
{
    unsigned int slot = region->callback_id;
    unsigned char targeting = 0;
    unsigned char aim_ok = 0;
    unsigned char front_rank = 0;
    int needed;
    int action_kind;
    unsigned int us_event;
    const wchar_t* help_text;

    if (slot == static_cast<unsigned int>(static_cast<short>(g_value_006840be))) {
        return 0;
    }

    needed = GetTargetNeededForCurrentAction(g_status_685170.selected_character);
    if (gXStatus.iTargetingMode == 1 || (needed == 1 && (event->usKeyState & CTRL_DOWN) != 0)) {
        targeting = 1;
        if (CanPartySlotParticipate(slot) != 0) {
            front_rank = 0;
            ChooseCombatAction(g_status_685170.selected_character, W8_TARGETING_CONTEXT_CURRENT,
                               &action_kind, 0, 0, 0);
            aim_ok = 1;
            if (action_kind == 5) {
                if (g_status_685170.selected_character == static_cast<int>(slot)) {
                    front_rank = 0;
                    aim_ok = 0;
                } else if (FrontRankScreens(g_status_685170.selected_character, slot) != 0) {
                    front_rank = 1;
                    aim_ok = 1;
                }
            }
        }
    } else if (gXStatus.iTargetingMode == 7 ||
               (needed == 7 && (event->usKeyState & CTRL_DOWN) != 0)) {
        targeting = 1;
        IsDeadCharacterTargetable(slot);
    }

    us_event = event->usEvent;
    if (us_event < LEFT_BUTTON_REPEAT + 1) {
        if (us_event == LEFT_BUTTON_REPEAT) {
            if (g_level_block->portrait_refresh_pending[slot] == 0 &&
                g_settings_6850c8.main_ui_mode != W8_MAIN_UI_MODE_PORTRAITS &&
                g_level_block->portrait_overlay_party_slot == -1 &&
                ClockIsTicking(g_level_block->countdown_32c) == 0 &&
                (region->flags & W8_REGION_LEFT_BUTTON_HELD) != 0) {
                g_level_block->portrait_overlay_party_slot = slot;
                if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
                    g_level_block->redraw_flags |= 0x8000;
                    return 1;
                }
            }
            /* retail falls through to return 1 at the end when the arm did not
               redraw */
        } else if (us_event == LEFT_BUTTON_DOWN) {
            region->flags |= W8_REGION_LEFT_BUTTON_HELD;
            if (g_level_block->portrait_refresh_pending[slot] == 0 &&
                g_settings_6850c8.main_ui_mode != W8_MAIN_UI_MODE_PORTRAITS &&
                g_level_block->portrait_overlay_party_slot == -1) {
                g_level_block->countdown_32c = SetCountdownClock(500);
                return 1;
            }
        } else if (us_event == LEFT_BUTTON_UP) {
            if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) != 0) {
                if (g_level_block->portrait_overlay_party_slot == static_cast<int>(slot)) {
                    g_level_block->portrait_overlay_party_slot = -1;
                    if (g_level_block->highlight_graphic != 0) {
                        ReleaseObject004257F0(g_level_block->highlight_graphic);
                        g_level_block->highlight_graphic = 0;
                    }
                    if (g_main_game_mode_0068eddc == 6) {
                        ClearSurfaceRect(
                            g_level_block->dialogue_x_220, g_level_block->dialogue_y_224,
                            g_level_block->dialogue_x_220 + g_level_block->dialogue_width_238,
                            g_level_block->dialogue_y_224 + g_level_block->dialogue_height_228);
                        InvalidateRegion(
                            g_level_block->dialogue_x_220, g_level_block->dialogue_y_224,
                            g_level_block->dialogue_x_220 + g_level_block->dialogue_width_238,
                            g_level_block->dialogue_y_224 + g_level_block->dialogue_height_228, 0);
                        if (g_level_block->dialogue_y_224 <
                                static_cast<unsigned int>(
                                    g_viewport_modes_647d30[g_level_block->camera_mode_100].top) &&
                            g_current_screen_state.id == W8_SCREEN_MAIN_GAME &&
                            g_level_block != 0) {
                            g_level_block->redraw_flags |= 0x100;
                        }
                        if (g_level_block->dialogue_y_224 + g_level_block->dialogue_height_228 >
                                0x166 &&
                            g_current_screen_state.id == W8_SCREEN_MAIN_GAME &&
                            g_level_block != 0) {
                            g_level_block->redraw_flags |= 0x800;
                        }
                    }
                    g_main_game_mode_0068eddc = 0;
                    if (g_current_screen_state.id != W8_SCREEN_MAIN_GAME) {
                        g_main_game_mode_0068eddc = 0;
                        return 1;
                    }
                    if (g_level_block == 0) {
                        g_main_game_mode_0068eddc = 0;
                        return 1;
                    }
                    g_level_block->redraw_flags |= 0x8000;
                    if (g_current_screen_state.id != W8_SCREEN_MAIN_GAME) {
                        return 1;
                    }
                    if (g_level_block == 0) {
                        return 1;
                    }
                    g_level_block->redraw_flags |= 0xff;
                } else if (IsNpcDialogueCursorActive() == 0) {
                    if (targeting != 0) {
                        if (aim_ok != 0) {
                            if (g_flag_68506f == 0) {
                                if (CanPartySlotParticipate(slot) == 0) {
                                    AimAtCharacterIndirect(g_status_685170.selected_character, slot,
                                                           W8_TARGETING_CONTEXT_CURRENT);
                                    StartBreathCycle(g_status_685170.selected_character, 0);
                                } else if (front_rank != 0) {
                                    QueueCharacterEvent(
                                        &g_status_685170.buffers
                                             .characters[g_status_685170.selected_character],
                                        g_character_event_kind_005ee65c, 0,
                                        g_effect_argument_005ed8c8, g_effect_argument_005ed914);
                                    ShowNotice(0xc, gppStringList[0x1f70 / 4], -1, -1, 0);
                                } else {
                                    AimAtCharacter(g_status_685170.selected_character, slot,
                                                   W8_TARGETING_CONTEXT_CURRENT);
                                    StartBreathCycle(g_status_685170.selected_character, 0);
                                }
                            } else {
                                EndScriptedPortraitPick00529C40(slot);
                            }
                        } else {
                            QueueCharacterEvent(
                                &g_status_685170.buffers
                                     .characters[g_status_685170.selected_character],
                                g_character_event_kind_005ee65c, 0, g_effect_argument_005ed8c8,
                                g_effect_argument_005ed914);
                        }
                    } else if (g_status_685170.item_in_cursor == 0 ||
                               gXStatus.iCurrentCursor != 7 ||
                               g_value_00685072 == &g_status_685170.item_in_hand_235b) {
                        if (g_settings_6850c8.main_ui_mode != W8_MAIN_UI_MODE_PORTRAITS) {
                            RefreshSelectedPartyPortrait(slot);
                        }
                        SelectPartyCharacter(slot);
                    } else {
                        int force_to_party;
                        if (gXStatus.fCombatMode == 0 && IsPartySlotEligible00524A10(slot) != 0) {
                            force_to_party = 0;
                        } else {
                            force_to_party = 1;
                        }
                        GiveHeldItemToCharacterOrParty(slot,
                                                       static_cast<unsigned char>(force_to_party));
                        if (gXStatus.fItemSelectMode != 0) {
                            Function59D690();
                        }
                    }
                } else {
                    Function56EFF0(slot);
                }
            }
            if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
                g_level_block->redraw_flags |= 1 << (slot & 0x1f);
                if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
                    g_level_block->redraw_flags |= 0x200000;
                    return 1;
                }
            }
        } else {
            if (us_event != LEFT_BUTTON_DBL_CLK) {
                return 0;
            }
            if (IsNpcDialogueCursorActive() == 0 && g_flag_68506f == 0) {
                if (gXStatus.fNpcDialogueMode != 0) {
                    CloseNpcDialogueForCamp();
                }
                g_pending_screen_state.parameter_3 = &g_status_685170.buffers.characters[slot];
                g_pending_screen_state.parameter_4 = 0;
                if (g_main_game_mode_0068eddc == 3) {
                    g_pending_screen_state.parameter_2 = slot;
                    if (gXStatus.fNpcDialogueMode != 0) {
                        Function56E800(0);
                    }
                } else if (g_main_game_mode_0068eddc == 5) {
                    g_pending_screen_state.parameter_2 = slot;
                    Function5187E0();
                } else {
                    g_pending_screen_state.parameter_2 = slot;
                    if (g_main_game_mode_0068eddc == 6) {
                        if (g_level_block->highlight_graphic != 0) {
                            ReleaseObject004257F0(g_level_block->highlight_graphic);
                            g_level_block->highlight_graphic = 0;
                            if (g_main_game_mode_0068eddc != 6) {
                                goto open_camp_after_mode6;
                            }
                        }
                        ClearSurfaceRect(
                            g_level_block->dialogue_x_220, g_level_block->dialogue_y_224,
                            g_level_block->dialogue_x_220 + g_level_block->dialogue_width_238,
                            g_level_block->dialogue_y_224 + g_level_block->dialogue_height_228);
                        InvalidateRegion(
                            g_level_block->dialogue_x_220, g_level_block->dialogue_y_224,
                            g_level_block->dialogue_x_220 + g_level_block->dialogue_width_238,
                            g_level_block->dialogue_y_224 + g_level_block->dialogue_height_228, 0);
                        if (g_level_block->dialogue_y_224 <
                                static_cast<unsigned int>(
                                    g_viewport_modes_647d30[g_level_block->camera_mode_100].top) &&
                            g_current_screen_state.id == W8_SCREEN_MAIN_GAME &&
                            g_level_block != 0) {
                            g_level_block->redraw_flags |= 0x100;
                        }
                        if (g_level_block->dialogue_y_224 + g_level_block->dialogue_height_228 >
                                0x166 &&
                            g_current_screen_state.id == W8_SCREEN_MAIN_GAME &&
                            g_level_block != 0) {
                            g_level_block->redraw_flags |= 0x800;
                        }
                    }
                }
            open_camp_after_mode6:
                g_main_game_mode_0068eddc = 0;
                SetPendingScreenState(W8_SCREEN_CAMP);
                if (gXStatus.fLockInteractMode != 0) {
                    Function5879A0(1);
                }
                if (gXStatus.fTrapInteractMode != 0) {
                    Function58A790(1);
                }
                if (gXStatus.fSpellCastMode != 0) {
                    CloseSpellCastingView();
                }
                if (gXStatus.fItemSelectMode != 0) {
                    CloseUseItemSelectView();
                }
                if (gXStatus.fReviewCharacterMode != 0) {
                    CloseFormationPanel();
                }
                SetPrimarySurfaceTextureHint2Enabled(0);
                return 1;
            }
        }
    } else if (us_event == RIGHT_BUTTON_DOWN) {
        region->flags |= W8_REGION_RIGHT_BUTTON_HELD;
        if (targeting == 0 && IsNpcDialogueCursorActive() == 0 && g_flag_68506f == 0 &&
            g_level_block->portrait_right_hold_armed == 0) {
            g_level_block->countdown_320 = SetCountdownClock(1000);
            g_level_block->portrait_right_hold_armed = 1;
        }
    } else {
        if (us_event != RIGHT_BUTTON_UP) {
            if (us_event != MOUSE_POS) {
                return 0;
            }
            if (g_level_block->portrait_right_hold_armed != 0 &&
                ClockIsTicking(g_level_block->countdown_320) == 0) {
                g_level_block->portrait_right_hold_armed = 0;
                if (gXStatus.fNpcDialogueMode != 0) {
                    CloseNpcDialogueForCamp();
                }
                g_pending_screen_state.parameter_3 = &g_status_685170.buffers.characters[slot];
                g_pending_screen_state.parameter_4 = 0;
                if (g_main_game_mode_0068eddc == 3) {
                    g_pending_screen_state.parameter_2 = slot;
                    if (gXStatus.fNpcDialogueMode != 0) {
                        Function56E800(0);
                    }
                } else if (g_main_game_mode_0068eddc == 5) {
                    g_pending_screen_state.parameter_2 = slot;
                    Function5187E0();
                } else {
                    g_pending_screen_state.parameter_2 = slot;
                    if (g_main_game_mode_0068eddc == 6) {
                        DismissHighlightOverlay();
                    }
                }
                g_main_game_mode_0068eddc = 0;
                SetPendingScreenState(W8_SCREEN_CAMP);
                UpdateScreenOverlays(1);
                SetPrimarySurfaceTextureHint2Enabled(0);
            }
            if ((region->flags & W8_REGION_MOUSE_LEAVE) == 0) {
                if ((region->flags & W8_REGION_MOUSE_ENTER) == 0) {
                    if (IsNpcDialogueCursorActive() == 0) {
                        if (targeting != 0 && aim_ok != 0) {
                            if (gXStatus.iTargetingMode == 0) {
                                RevalidateSelectedTarget(g_status_685170.selected_character);
                            }
                            SetTargetCursor(4);
                            return 0;
                        }
                        SetTargetCursor(GetTargetingCursorForState(0));
                    }
                } else {
                    if (g_level_block->portrait_refresh_pending[slot] == 0) {
                        if (g_status_685170.item_in_cursor == 0 || gXStatus.iCurrentCursor != 7 ||
                            g_value_00685072 == &g_status_685170.item_in_hand_235b) {
                            help_text = gppStringList[0x74 / 4];
                        } else {
                            help_text = gppStringList[0x78 / 4];
                        }
                        SetRegionHelpText(help_text);
                    }
                    PushButtonSoundScheme005587C0(0, 1);
                    g_level_block->portrait_right_hold_armed = 0;
                    NoOp();
                    if (IsNpcDialogueCursorActive() == 0) {
                        if (g_level_block->tooltip_kind != 0 ||
                            g_level_block->tooltip_subject != static_cast<int>(slot)) {
                            g_level_block->tooltip_pending = 1;
                            g_level_block->tooltip_since = GetTickCount();
                            g_level_block->tooltip_subject = slot;
                            g_level_block->tooltip_kind = 0;
                        }
                        if (gXStatus.iTargetingMode == 0) {
                            Function53C130(slot, 1);
                            return 0;
                        }
                    }
                }
            } else {
                g_level_block->portrait_right_hold_armed = 0;
                NoOp();
                if (IsNpcDialogueCursorActive() == 0) {
                    if (g_level_block->tooltip_kind != 0 || g_level_block->tooltip_subject != -1) {
                        g_level_block->tooltip_pending = 1;
                        g_level_block->tooltip_since = GetTickCount();
                        g_level_block->tooltip_subject = -1;
                        g_level_block->tooltip_kind = 0;
                    }
                    SetTargetCursor(GetTargetingCursorForState(0));
                    if (gXStatus.iTargetingMode == 0) {
                        Function53C130(slot, 0);
                    }
                }
                if (g_level_block->portrait_overlay_party_slot == static_cast<int>(slot)) {
                    g_level_block->portrait_overlay_party_slot = -1;
                    if (g_level_block->highlight_graphic != 0) {
                        ReleaseObject004257F0(g_level_block->highlight_graphic);
                        g_level_block->highlight_graphic = 0;
                    }
                    if (g_main_game_mode_0068eddc == 6) {
                        ClearSurfaceRect(
                            g_level_block->dialogue_x_220, g_level_block->dialogue_y_224,
                            g_level_block->dialogue_x_220 + g_level_block->dialogue_width_238,
                            g_level_block->dialogue_y_224 + g_level_block->dialogue_height_228);
                        InvalidateRegion(
                            g_level_block->dialogue_x_220, g_level_block->dialogue_y_224,
                            g_level_block->dialogue_x_220 + g_level_block->dialogue_width_238,
                            g_level_block->dialogue_y_224 + g_level_block->dialogue_height_228, 0);
                        if (g_level_block->dialogue_y_224 <
                                static_cast<unsigned int>(
                                    g_viewport_modes_647d30[g_level_block->camera_mode_100].top) &&
                            g_current_screen_state.id == W8_SCREEN_MAIN_GAME &&
                            g_level_block != 0) {
                            g_level_block->redraw_flags |= 0x100;
                        }
                        if (g_level_block->dialogue_y_224 + g_level_block->dialogue_height_228 >
                                0x166 &&
                            g_current_screen_state.id == W8_SCREEN_MAIN_GAME &&
                            g_level_block != 0) {
                            g_level_block->redraw_flags |= 0x800;
                        }
                    }
                    g_main_game_mode_0068eddc = 0;
                    if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
                        g_level_block->redraw_flags |= 0x8000;
                        if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME &&
                            g_level_block != 0) {
                            g_level_block->redraw_flags |= 0xff;
                            return 0;
                        }
                    }
                }
            }
            return 0;
        }
        if ((region->flags & W8_REGION_RIGHT_BUTTON_HELD) != 0 && targeting == 0 &&
            IsNpcDialogueCursorActive() == 0 && g_flag_68506f == 0) {
            g_level_block->portrait_right_hold_armed = 0;
            if (g_status_685170.item_in_cursor == 0 || gfKeyState[0x11] != 0) {
                if (gXStatus.fNpcDialogueMode != 0) {
                    CloseNpcDialogueForCamp();
                }
                g_pending_screen_state.parameter_3 = &g_status_685170.buffers.characters[slot];
                g_pending_screen_state.parameter_4 = 0;
                if (g_main_game_mode_0068eddc == 3) {
                    g_pending_screen_state.parameter_2 = slot;
                    if (gXStatus.fNpcDialogueMode != 0) {
                        Function56E800(0);
                    }
                } else if (g_main_game_mode_0068eddc == 5) {
                    g_pending_screen_state.parameter_2 = slot;
                    Function5187E0();
                } else {
                    g_pending_screen_state.parameter_2 = slot;
                    if (g_main_game_mode_0068eddc == 6) {
                        if (g_level_block->highlight_graphic != 0) {
                            ReleaseObject004257F0(g_level_block->highlight_graphic);
                            g_level_block->highlight_graphic = 0;
                            if (g_main_game_mode_0068eddc != 6) {
                                goto open_camp_right_up;
                            }
                        }
                        ClearSurfaceRect(
                            g_level_block->dialogue_x_220, g_level_block->dialogue_y_224,
                            g_level_block->dialogue_x_220 + g_level_block->dialogue_width_238,
                            g_level_block->dialogue_y_224 + g_level_block->dialogue_height_228);
                        InvalidateRegion(
                            g_level_block->dialogue_x_220, g_level_block->dialogue_y_224,
                            g_level_block->dialogue_x_220 + g_level_block->dialogue_width_238,
                            g_level_block->dialogue_y_224 + g_level_block->dialogue_height_228, 0);
                        if (g_level_block->dialogue_y_224 <
                                static_cast<unsigned int>(
                                    g_viewport_modes_647d30[g_level_block->camera_mode_100].top) &&
                            g_current_screen_state.id == W8_SCREEN_MAIN_GAME &&
                            g_level_block != 0) {
                            g_level_block->redraw_flags |= 0x100;
                        }
                        if (g_level_block->dialogue_y_224 + g_level_block->dialogue_height_228 >
                                0x166 &&
                            g_current_screen_state.id == W8_SCREEN_MAIN_GAME &&
                            g_level_block != 0) {
                            g_level_block->redraw_flags |= 0x800;
                        }
                    }
                }
            open_camp_right_up:
                g_main_game_mode_0068eddc = 0;
                SetPendingScreenState(W8_SCREEN_CAMP);
                if (gXStatus.fLockInteractMode != 0) {
                    Function5879A0(1);
                }
                if (gXStatus.fTrapInteractMode != 0) {
                    Function58A790(1);
                }
                if (gXStatus.fSpellCastMode != 0) {
                    CloseSpellCastingView();
                }
                if (gXStatus.fItemSelectMode != 0) {
                    CloseUseItemSelectView();
                }
                if (gXStatus.fReviewCharacterMode != 0) {
                    CloseFormationPanel();
                }
                SetPrimarySurfaceTextureHint2Enabled(0);
                return 1;
            }
            GiveHeldItemToCharacterOrParty(slot, 1);
            if (gXStatus.fItemSelectMode != 0) {
                Function59D690();
                return 1;
            }
        }
    }
    return 1;
}

/* Condition orb on a party portrait (help 25): press while highest_condition
   is set arms the overlay slot; release and leave dismiss the hover plate;
   enter drives tooltip kind 1 and region help. */
// FUNCTION: WIZ8 0x005667A0
unsigned char PortraitConditionOrbRegionEvent(const InputAtom* event, W8Region* region)
{
    unsigned int slot = region->callback_id;
    W8Character* character = &g_status_685170.buffers.characters[slot];
    unsigned int us_event;

    if (character->highest_condition == 0) {
        PushButtonSoundScheme005587C0(0, 1);
    }

    us_event = event->usEvent;
    if (us_event != LEFT_BUTTON_DOWN) {
        if (us_event != LEFT_BUTTON_UP) {
            if (us_event != MOUSE_POS) {
                return 0;
            }
            if ((region->flags & W8_REGION_MOUSE_LEAVE) != 0) {
                if (g_level_block->tooltip_kind != 1 || g_level_block->tooltip_subject != -1) {
                    g_level_block->tooltip_pending = 1;
                    g_level_block->tooltip_since = GetTickCount();
                    g_level_block->tooltip_subject = -1;
                    g_level_block->tooltip_kind = 1;
                }
                if (g_level_block->condition_orb_party_slot != -1) {
                    g_level_block->condition_orb_party_slot = -1;
                    if (g_level_block->highlight_graphic != 0) {
                        ReleaseObject004257F0(g_level_block->highlight_graphic);
                        g_level_block->highlight_graphic = 0;
                    }
                    ClearHighlightOverlayRegion();
                    g_main_game_mode_0068eddc = 0;
                    if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
                        g_level_block->redraw_flags |= 0x8000;
                    }
                }
            } else {
                if ((region->flags & W8_REGION_MOUSE_ENTER) != 0) {
                    if (character->highest_condition != 0) {
                        if (g_level_block->tooltip_kind != 1 ||
                            g_level_block->tooltip_subject != static_cast<int>(slot)) {
                            g_level_block->tooltip_pending = 1;
                            g_level_block->tooltip_since = GetTickCount();
                            g_level_block->tooltip_subject = slot;
                            g_level_block->tooltip_kind = 1;
                        }
                        if (gfLeftButtonState != 0 && character->highest_condition != 0) {
                            g_level_block->condition_orb_party_slot = slot;
                            if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME &&
                                g_level_block != 0) {
                                g_level_block->redraw_flags |= 0x8000;
                            }
                        }
                        EnableRegionHelpFlag004F27D0(region);
                        return 0;
                    }
                } else {
                    return 0;
                }
            }
            DisableRegionHelpFlag004F27E0(region);
            return 0;
        }
        if (g_level_block->condition_orb_party_slot == -1) {
            return 1;
        }
        g_level_block->condition_orb_party_slot = -1;
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
        if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
            g_level_block->redraw_flags |= 0x8000;
        }
        return 1;
    }

    if (character->highest_condition == 0) {
        return 1;
    }
    g_level_block->condition_orb_party_slot = slot;
    if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
        g_level_block->redraw_flags |= 0x8000;
    }
    return 1;
}

/* Enchantment orb on a party portrait (help 26): near-clone of the condition
   orb against enchantment_top and tooltip kind 2. */
// FUNCTION: WIZ8 0x00566AE0
unsigned char PortraitEnchantmentOrbRegionEvent(const InputAtom* event, W8Region* region)
{
    unsigned int slot = region->callback_id;
    W8Character* character = &g_status_685170.buffers.characters[slot];
    unsigned int us_event;

    if (character->enchantment_top == 0) {
        PushButtonSoundScheme005587C0(0, 1);
    }

    us_event = event->usEvent;
    if (us_event != LEFT_BUTTON_DOWN) {
        if (us_event != LEFT_BUTTON_UP) {
            if (us_event != MOUSE_POS) {
                return 0;
            }
            if ((region->flags & W8_REGION_MOUSE_LEAVE) != 0) {
                if (g_level_block->tooltip_kind != 2 || g_level_block->tooltip_subject != -1) {
                    g_level_block->tooltip_pending = 1;
                    g_level_block->tooltip_since = GetTickCount();
                    g_level_block->tooltip_subject = -1;
                    g_level_block->tooltip_kind = 2;
                }
                if (g_level_block->enchantment_orb_party_slot != -1) {
                    g_level_block->enchantment_orb_party_slot = -1;
                    if (g_level_block->highlight_graphic != 0) {
                        ReleaseObject004257F0(g_level_block->highlight_graphic);
                        g_level_block->highlight_graphic = 0;
                    }
                    ClearHighlightOverlayRegion();
                    g_main_game_mode_0068eddc = 0;
                    if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
                        g_level_block->redraw_flags |= 0x8000;
                    }
                }
            } else {
                if ((region->flags & W8_REGION_MOUSE_ENTER) != 0) {
                    if (character->enchantment_top != 0) {
                        if (g_level_block->tooltip_kind != 2 ||
                            g_level_block->tooltip_subject != static_cast<int>(slot)) {
                            g_level_block->tooltip_pending = 1;
                            g_level_block->tooltip_since = GetTickCount();
                            g_level_block->tooltip_subject = slot;
                            g_level_block->tooltip_kind = 2;
                        }
                        if (gfLeftButtonState != 0 && character->enchantment_top != 0) {
                            g_level_block->enchantment_orb_party_slot = slot;
                            if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME &&
                                g_level_block != 0) {
                                g_level_block->redraw_flags |= 0x8000;
                            }
                        }
                        EnableRegionHelpFlag004F27D0(region);
                        return 0;
                    }
                } else {
                    return 0;
                }
            }
            DisableRegionHelpFlag004F27E0(region);
            return 0;
        }
        if (g_level_block->enchantment_orb_party_slot == -1) {
            return 1;
        }
        g_level_block->enchantment_orb_party_slot = -1;
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
        if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
            g_level_block->redraw_flags |= 0x8000;
        }
        return 1;
    }

    if (character->enchantment_top == 0) {
        return 1;
    }
    g_level_block->enchantment_orb_party_slot = slot;
    if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
        g_level_block->redraw_flags |= 0x8000;
    }
    return 1;
}

/* Help 28: portrait side bar — hover picks upper/lower weapon or assayable
   item; click opens Assay or binds items with Shift. */
// FUNCTION: WIZ8 0x00566E20
unsigned char PortraitAssaySidebarRegionEvent(const InputAtom* event, W8Region* region)
{
    int previous_mode = g_level_block->portrait_assay_hover_mode;
    unsigned int slot = region->callback_id;
    W8Character* character;
    W8ItemInstance* item;
    int item_id;
    unsigned int us_event;

    if (IsPartySlotEligible00524A10(slot) == 0) {
        return 0;
    }

    character = &g_status_685170.buffers.characters[slot];
    item_id = character->equipment[W8_EQUIP_SLOT_PRIMARY_WEAPON].item_id;
    if (item_id == -1 || (g_item_records[item_id].flags_041 & 4) == 0) {
        if (GetAtomCursorY004285A0(event) - region->y1 < 0x19) {
            g_level_block->portrait_assay_hover_mode = 1;
        } else {
            g_level_block->portrait_assay_hover_mode = 2;
        }
    } else {
        g_level_block->portrait_assay_hover_mode = 3;
    }

    if (previous_mode != g_level_block->portrait_assay_hover_mode &&
        g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
        g_level_block->redraw_flags |= 1 << (slot & 0x1f);
    }

    us_event = event->usEvent;
    if (us_event < RIGHT_BUTTON_UP) {
        if (us_event == RIGHT_BUTTON_DOWN) {
            region->flags |= W8_REGION_RIGHT_BUTTON_HELD;
            return 1;
        }
        if (us_event == LEFT_BUTTON_DOWN) {
            region->flags |= W8_REGION_LEFT_BUTTON_HELD;
            return 1;
        }
        if (us_event != LEFT_BUTTON_UP) {
            return 0;
        }
        if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) == 0) {
            return 1;
        }
        if ((event->usKeyState & SHIFT_DOWN) == 0) {
            BindCharacterItems(slot, 1);
            return 1;
        }
        BindEveryPartyItem();
        return 1;
    }
    if (us_event != RIGHT_BUTTON_UP) {
        if (us_event != MOUSE_POS) {
            return 0;
        }
        if ((region->flags & W8_REGION_MOUSE_LEAVE) == 0) {
            if ((region->flags & W8_REGION_MOUSE_ENTER) != 0 &&
                (PushButtonSoundScheme005587C0(0, 1),
                 g_level_block->tooltip_kind != 5 ||
                     g_level_block->tooltip_subject != static_cast<int>(slot))) {
                g_level_block->tooltip_pending = 1;
                g_level_block->tooltip_since = GetTickCount();
                g_level_block->tooltip_subject = slot;
                g_level_block->tooltip_kind = 5;
            }
        } else if (g_level_block->tooltip_kind != 5 || g_level_block->tooltip_subject != -1) {
            g_level_block->tooltip_pending = 1;
            g_level_block->tooltip_since = GetTickCount();
            g_level_block->tooltip_subject = -1;
            g_level_block->tooltip_kind = 5;
            return 0;
        }
        return 0;
    }
    if ((region->flags & W8_REGION_RIGHT_BUTTON_HELD) == 0) {
        return 1;
    }
    {
        int mode = g_level_block->portrait_assay_hover_mode;
        if (mode != 1) {
            if (mode == 2) {
                item = &character->equipment[W8_EQUIP_SLOT_SECONDARY_WEAPON];
                goto open_assay;
            }
            if (mode != 3) {
                return 1;
            }
        }
        item = &character->equipment[W8_EQUIP_SLOT_PRIMARY_WEAPON];
    open_assay:
        if (item != 0 && item->item_id != -1) {
            OpenAssayDialog0056AE20(item, slot);
        }
    }
    return 1;
}

/* Help 24: portrait overlay hover strip; left-down and enter (while held)
   set portrait_overlay_party_slot, leave and up dismiss the mode-6 plate. */
// FUNCTION: WIZ8 0x005670C0
unsigned char PortraitOverlayHoverRegionEvent(const InputAtom* event, W8Region* region)
{
    unsigned int us_event = event->usEvent;
    unsigned int slot = region->callback_id;

    if (us_event == LEFT_BUTTON_DOWN) {
        g_level_block->portrait_overlay_party_slot = slot;
    } else {
        if (us_event != LEFT_BUTTON_UP) {
            if (us_event != MOUSE_POS) {
                return 0;
            }
            if ((region->flags & W8_REGION_MOUSE_LEAVE) == 0) {
                if ((region->flags & W8_REGION_MOUSE_ENTER) == 0) {
                    return 0;
                }
                PushButtonSoundScheme005587C0(0, 1);
                if (g_level_block->tooltip_kind != 4 ||
                    g_level_block->tooltip_subject != static_cast<int>(slot)) {
                    g_level_block->tooltip_pending = 1;
                    g_level_block->tooltip_since = GetTickCount();
                    g_level_block->tooltip_subject = slot;
                    g_level_block->tooltip_kind = 4;
                }
                if (gfLeftButtonState != 0) {
                    g_level_block->portrait_overlay_party_slot = slot;
                }
            } else {
                if (g_level_block->tooltip_kind != 4 || g_level_block->tooltip_subject != -1) {
                    g_level_block->tooltip_pending = 1;
                    g_level_block->tooltip_since = GetTickCount();
                    g_level_block->tooltip_subject = -1;
                    g_level_block->tooltip_kind = 4;
                }
                if (g_level_block->portrait_overlay_party_slot == -1) {
                    return 0;
                }
                g_level_block->portrait_overlay_party_slot = -1;
                if (g_level_block->highlight_graphic != 0) {
                    ReleaseObject004257F0(g_level_block->highlight_graphic);
                    g_level_block->highlight_graphic = 0;
                }
                ClearHighlightOverlayRegion();
                g_main_game_mode_0068eddc = 0;
            }
            if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
                g_level_block->redraw_flags |= 0x8000;
            }
            return 0;
        }
        g_level_block->portrait_overlay_party_slot = -1;
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
    if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
        g_level_block->redraw_flags |= 0x8000;
    }
    return 1;
}

/* Combat-action hit regions beside the eight party portraits (region set 4):
   open the keyboard menu on release while combat input is live, drive hover
   help for a queued spell/item, and right-click falls back when an action is
   unreachable. */
// FUNCTION: WIZ8 0x005673B0
unsigned char PartyCombatActionRegionEvent(const InputAtom* event, W8Region* region)
{
    unsigned int slot = region->callback_id;
    if (IsPartySlotEligible00524A10(slot) == 0) {
        return 0;
    }

    switch (event->usEvent) {
    case RIGHT_BUTTON_DOWN:
        region->flags |= W8_REGION_RIGHT_BUTTON_HELD;
        return 1;
    case LEFT_BUTTON_DOWN:
        region->flags |= W8_REGION_LEFT_BUTTON_HELD;
        return 1;
    case LEFT_BUTTON_UP:
        if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) != 0 &&
            g_status_685170.buffers.party_rows[slot].occupied != 0) {
            if (g_level_block->keyboard_menu_open != 0 &&
                slot != static_cast<unsigned int>(g_level_block->combat_slot)) {
                CloseKeyboardMenu();
            }
            if (g_combat_state->flag_001 != 0) {
                g_level_block->combat_slot = slot;
                OpenKeyboardMenuForSlot(slot);
                return 1;
            }
        }
        return 1;
    case RIGHT_BUTTON_UP:
        if ((region->flags & W8_REGION_RIGHT_BUTTON_HELD) != 0 && gXStatus.fSpellCastMode == 0 &&
            gXStatus.fItemSelectMode == 0) {
            FallbackFromUnreachableAction(slot);
        }
        return 1;
    case MOUSE_POS:
        if ((region->flags & W8_REGION_MOUSE_LEAVE) != 0) {
            if (g_level_block->keyboard_menu_open == 0) {
                g_level_block->combat_slot = -1;
                g_level_block->countdown_30c = SetCountdownClock(0);
                g_level_block->hover_combat_slot = -1;
            }
            g_level_block->party_slots_170[4] = -1;
            gXStatus.monster_manager_entries[slot].field_0d1 = 1;
            DisableRegionHelpFlag004F27E0(region);
            return 0;
        }
        if ((region->flags & W8_REGION_MOUSE_ENTER) == 0) {
            return 0;
        }
        if (g_combat_state->flag_001 != 0) {
            g_level_block->party_slots_170[4] = slot;
            gXStatus.monster_manager_entries[slot].field_0d1 = 1;
        }
        {
            W8PartySlotRow* row = &g_status_685170.buffers.party_rows[slot];
            if (row->action_03d == W8_ACTION_CAST_SPELL) {
                EnableRegionHelpFlag004F27D0(region);
                SetRegionHelpText(
                    FormatWideString(g_format_s_colon_s_paren_d_006481b4, gppStringList[0x1a0 / 4],
                                     g_spell_records[row->action_detail_041].display_name,
                                     row->action_detail_045.spell.power_level));
                return 0;
            }
            if (row->action_03d == W8_ACTION_USE_ITEM) {
                EnableRegionHelpFlag004F27D0(region);
                SetRegionHelpText(FormatWideString(
                    g_format_s_colon_s_0061c3e0, gppStringList[0x1a4 / 4],
                    FormatItemDisplayName(row->action_detail_045.item_use.item, 0)));
                return 0;
            }
        }
        DisableRegionHelpFlag004F27E0(region);
        return 0;
    }
    return 0;
}

/* Help 31: radar-map button near the text area — left-up opens automap after
   tearing down mode 3/5/6 overlays; right-up toggles zoom; enter/leave hover
   the map button art. */
// FUNCTION: WIZ8 0x00567600
unsigned char RadarMapButtonRegionEvent(const InputAtom* event, W8Region* region)
{
    unsigned int us_event = event->usEvent;

    if (us_event < RIGHT_BUTTON_UP) {
        if (us_event == RIGHT_BUTTON_DOWN) {
            region->flags |= W8_REGION_RIGHT_BUTTON_HELD;
            return 1;
        }
        if (us_event == LEFT_BUTTON_DOWN) {
            region->flags |= W8_REGION_LEFT_BUTTON_HELD;
            return 1;
        }
        if (us_event != LEFT_BUTTON_UP) {
            return 0;
        }
        if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) != 0) {
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
                        goto open_automap;
                    }
                }
                ClearSurfaceRect(g_level_block->dialogue_x_220, g_level_block->dialogue_y_224,
                                 g_level_block->dialogue_x_220 + g_level_block->dialogue_width_238,
                                 g_level_block->dialogue_y_224 +
                                     g_level_block->dialogue_height_228);
                InvalidateRegion(g_level_block->dialogue_x_220, g_level_block->dialogue_y_224,
                                 g_level_block->dialogue_x_220 + g_level_block->dialogue_width_238,
                                 g_level_block->dialogue_y_224 + g_level_block->dialogue_height_228,
                                 0);
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
        open_automap:
            g_main_game_mode_0068eddc = 0;
            SetPendingScreenState(W8_SCREEN_AUTOMAP);
            return 1;
        }
    } else if (us_event == RIGHT_BUTTON_UP) {
        if ((region->flags & W8_REGION_RIGHT_BUTTON_HELD) != 0) {
            ToggleRadarMapZoom();
        }
    } else {
        if (us_event != MOUSE_POS) {
            return 0;
        }
        if ((region->flags & W8_REGION_MOUSE_LEAVE) != 0) {
            g_level_block->radar_map_alternate = 0;
            RefreshRadarMap();
            return 1;
        }
        if ((region->flags & W8_REGION_MOUSE_ENTER) != 0) {
            g_level_block->radar_map_alternate = 1;
            RefreshRadarMap();
            return 1;
        }
    }
    return 1;
}

/* Region 23: the 3D world view. Mouse-move refreshes the combat hover, the
   world-item hover and the portrait strip; left-up runs the targeting, item
   and monster dispatch; right-down opens monster info or the assay dialog or
   toggles the mouselook latch. */
// FUNCTION: WIZ8 0x00567800
unsigned char WorldViewRegionEvent(const InputAtom* event, W8Region* region)
{
    POINT cursor_pos;
    int cursor_x;
    int cursor_y;
    int needed;
    int slot;
    unsigned int us_event;

    PushButtonSoundScheme005587C0(0, 1);
    if (IsMessageBoxActive() != 0 || gXStatus.fReviewCharacterMode != 0 ||
        g_level_runtime_flag_0065ba70 != 0) {
        return 0;
    }
    needed = GetTargetNeededForCurrentAction(g_status_685170.selected_character);
    us_event = event->usEvent;
    if (us_event > RIGHT_BUTTON_DOWN) {
        if (us_event == RIGHT_BUTTON_UP) {
            if (g_flag_0068edd8 != 0 && g_settings_6850c8.mouselook_toggle == 0) {
                WarpSystemCursor(g_mouselook_cursor_pos_0068edc0.x,
                                 g_mouselook_cursor_pos_0068edc0.y);
                SetFlag603C60();
                g_flag_0068edd8 = 0;
                g_flag_0068edd9 = 0;
                gfTrackMousePos = 0;
                g_flag_00652da7 = 0;
                return 1;
            }
            if (GetFlag68F105() == 0) {
                return 1;
            }
            SGPMouseGetPos(&cursor_pos);
            MipeWorldViewEvent0057E0E0(RIGHT_BUTTON_UP, &cursor_pos);
            return 1;
        }
        if (us_event != MOUSE_POS) {
            return 0;
        }
        cursor_y = GetAtomCursorY004285A0(event);
        cursor_x = GetAtomCursorX00428580(event);
        if (GetFlag68F105() != 0) {
            UpdateMipeSelection0057DC20();
        } else {
            int hover;
            if (gXStatus.fNpcDialogueMode == 0 && gXStatus.iTargetingMode != 3 &&
                gXStatus.iTargetingMode != 4 && gXStatus.iTargetingMode != 6 &&
                gXStatus.active_monster_count != 0 && IsWorldCursorVisible() == 0 &&
                g_flag_0068edd8 == 0) {
                hover = PickNearestMonsterUnderCursor005396D0(cursor_x, cursor_y);
            } else {
                hover = -1;
            }
            SetCombatSelection(hover);
            if (g_level_block->highlighted_item == -1) {
                if (gXStatus.item_manager_pending != 0 && IsWorldCursorVisible() == 0 &&
                    g_flag_0068edd8 == 0) {
                    hover = PickNearestItemUnderCursor004F7370(cursor_x, cursor_y, 5000.0f);
                } else {
                    hover = -1;
                }
                SetCombatTarget(hover);
            }
        }
        if ((region->flags & W8_REGION_MOUSE_LEAVE) == 0 && g_modal_owner_0068edd0 == 0) {
            for (slot = 0; slot < 8; ++slot) {
                if (g_status_685170.buffers.party_rows[slot].occupied != 0 &&
                    g_level_block->portrait_refresh_pending[slot] != 0 &&
                    IsPartyPortraitUnderCursor00561980(slot) != 0) {
                    return 0;
                }
            }
            UpdateWorldViewCursor0056A5D0(event, needed);
            return 1;
        }
        SetCombatSelection(-1);
        SetCombatTarget(-1);
        SetCombatAction(-1);
        SetTargetCursor(GetTargetingCursorForState(0));
        return 1;
    }
    if (us_event == RIGHT_BUTTON_DOWN) {
        region->flags |= W8_REGION_RIGHT_BUTTON_HELD;
        if (GetFlag68F105() != 0) {
            SGPMouseGetPos(&cursor_pos);
            MipeWorldViewEvent0057E0E0(RIGHT_BUTTON_DOWN, &cursor_pos);
            return 1;
        }
        if (g_level_block->highlighted_item != -1 && gXStatus.fSpellCastMode == 0 &&
            gXStatus.fNpcDialogueMode == 0 && gXStatus.fItemSelectMode == 0 &&
            gXStatus.fLockInteractMode == 0 && gXStatus.fTrapInteractMode == 0 &&
            (g_settings_6850c8.ctrl_right_click_info == 0 ||
             g_monster_combat_timer_enabled_006f0531 != 0)) {
            OpenMonsterInfoDialog0056AD60(g_level_block->highlighted_item);
            return 1;
        }
        if (g_status_685170.item_in_cursor == 0 && g_level_block->selected_item != -1 &&
            gXStatus.fSpellCastMode == 0 && gXStatus.fNpcDialogueMode == 0 &&
            gXStatus.fItemSelectMode == 0 && gXStatus.fLockInteractMode == 0 &&
            gXStatus.fTrapInteractMode == 0 &&
            (g_settings_6850c8.ctrl_right_click_info == 0 ||
             g_monster_combat_timer_enabled_006f0531 != 0)) {
            W8WorldItem* world_item = ItemInfo(ItemIndex(g_level_block->selected_item));
            PartyAttemptsToIdentifyItem(&world_item->item, 0);
            OpenAssayDialog0056AE20(&world_item->item, -1);
            return 1;
        }
        if (IsWorldCursorVisible() == 0 && gXStatus.fNpcDialogueMode == 0) {
            if (g_settings_6850c8.mouselook_toggle != 0) {
                if (g_flag_0068edd8 != 0) {
                    WarpSystemCursor(g_mouselook_cursor_pos_0068edc0.x,
                                     g_mouselook_cursor_pos_0068edc0.y);
                    SetFlag603C60();
                    g_flag_0068edd8 = 0;
                    g_flag_0068edd9 = 0;
                    gfTrackMousePos = 0;
                    g_flag_00652da7 = 0;
                    return 1;
                }
            } else if (g_flag_0068edd8 != 0) {
                return 1;
            }
            SGPMouseGetPos(&g_mouselook_cursor_pos_0068edc0);
            ClearFlag603C60();
            SetMouseCursorHotspot(0, 0);
            WarpSystemCursor(0x140, 0xf0);
            g_flag_0068edd8 = 1;
            g_flag_0068edd9 = 0;
            g_flag_00652da7 = 1;
            gfTrackMousePos = 1;
        }
        return 1;
    }
    if (us_event == LEFT_BUTTON_DOWN) {
        if (g_flag_0068edd8 != 0) {
            return 0;
        }
        region->flags |= W8_REGION_LEFT_BUTTON_HELD;
        if (GetFlag68F105() == 0) {
            return 1;
        }
        SGPMouseGetPos(&cursor_pos);
        MipeWorldViewEvent0057E0E0(LEFT_BUTTON_DOWN, &cursor_pos);
        return 1;
    }
    if (us_event != LEFT_BUTTON_UP) {
        return 0;
    }
    if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) == 0) {
        return 1;
    }
    if (g_flag_0068edd8 != 0) {
        return 1;
    }
    region->flags &= ~W8_REGION_LEFT_BUTTON_HELD;
    if (FinishNpcVoiceIfSessionActive00577A20() != 0) {
        return 1;
    }
    if (gXStatus.iTargetingMode == 3 || (needed == 3 && (event->usKeyState & CTRL_DOWN) != 0)) {
        if (IsWorldCursorVisible() == 0) {
            InitializeWorldCursor00490210();
        }
        SetWorldCursorRange00491650(
            CalcRangeDistanceFromParty0051AB50(static_cast<W8RangeCategory>(GetCharActionRange(
                g_status_685170.selected_character, 0, W8_TARGETING_CONTEXT_CURRENT))));
        if (gpSCSV != 0 && GetActionSpellLikeId(g_status_685170.selected_character,
                                                W8_TARGETING_CONTEXT_CURRENT) == 0x3c) {
            signed char extent_index;
            srVector3T<float> minimum;
            srVector3T<float> maximum;
            if (gpSCSV->iSpellPower == -1) {
                extent_index = 2;
            } else {
                extent_index = g_spell_power_extent_index_00616f41[gpSCSV->iSpellPower];
            }
            minimum.x = static_cast<float>(g_world_cursor_extent_table_00616eb0[extent_index * 6]);
            minimum.y =
                static_cast<float>(g_world_cursor_extent_table_00616eb0[extent_index * 6 + 1]);
            minimum.z =
                static_cast<float>(g_world_cursor_extent_table_00616eb0[extent_index * 6 + 2]);
            maximum.x =
                static_cast<float>(g_world_cursor_extent_table_00616eb0[extent_index * 6 + 3]);
            maximum.y =
                static_cast<float>(g_world_cursor_extent_table_00616eb0[extent_index * 6 + 4]);
            maximum.z =
                static_cast<float>(g_world_cursor_extent_table_00616eb0[extent_index * 6 + 5]);
            SetWorldCursorExtents00492190(&minimum, &maximum);
        }
    } else if (gXStatus.iTargetingMode == 4 ||
               (needed == 4 && (event->usKeyState & CTRL_DOWN) != 0)) {
        AimAtGroundTarget00538770(g_status_685170.selected_character);
    } else if (g_level_block->highlighted_item == -1) {
        if (g_status_685170.item_in_cursor != 0 &&
            ForwardSelectedPropIndex004503B0(GetWorld(), GetAtomCursorX00428580(event),
                                             GetAtomCursorY004285A0(event)) == -1) {
            if (g_flag_006840bc == 0) {
                DropItemInHand(1);
            }
        } else if (g_level_block->selected_item != -1) {
            if (g_flag_006840bc == 0 &&
                InteractWithWorldItem004F7910(g_level_block->selected_item) != 0) {
                g_level_block->selected_item = -1;
                VideoRemoveToolTip();
                SetTargetingMode(0);
            }
        } else if (g_flag_006840bc == 0) {
            ForwardActivateSelectedProp00451150(GetWorld(), 2, GetAtomCursorX00428580(event),
                                                GetAtomCursorY004285A0(event));
        }
    } else {
        unsigned int monster_index = MonsterGetIndexByLocationID(
            0x16c2, MAIN_GAME_SCREEN_CPP, g_level_block->highlighted_item, 1);
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
        unsigned char assign = 1;
        if (gXStatus.fCombatMode == 0) {
            if (needed != 2 && needed != 1 && needed != 5) {
                assign = 0;
                if ((gXStatus.iCurrentCursor == 6 || g_status_685170.item_in_cursor != 0) &&
                    g_status_685170.selected_character != -1 &&
                    CanPartyMemberAimAtMonster(g_status_685170.selected_character, 2, monster_info,
                                               6, 0) != 0) {
                    if ((GetMonsterDataForInfo(monster_info)->flags_0d0 & 1) == 0) {
                        ShowNotice(0xc, gppStringList[0x1f78 / 4], -1, -1, 0);
                    } else {
                        W8ItemInstance* item = 0;
                        if (g_status_685170.item_in_cursor != 0) {
                            item = &g_status_685170.item_in_hand_235b;
                        }
                        if (monster_info->highest_condition < 0xf) {
                            W8NpcState* npc = FindNpcBindingForMonster(MonsterGetIndexByLocationID(
                                0x16fe, MAIN_GAME_SCREEN_CPP, g_level_block->highlighted_item, 1));
                            QueueNpcScriptNotice(npc, item, -1, 0, 0);
                            SetCombatSelection(-1);
                        } else {
                            ShowNotice(0xc, gppStringList[0x1f78 / 4], -1, -1, 0);
                        }
                    }
                }
            }
        } else if (g_combat_state->flag_001 == 0 && gXStatus.fPartyMovementMode == 0) {
            ShowNotice(0xc, gppStringList[0x1f74 / 4], -1, -1, 0);
            assign = 0;
        } else if (g_flag_006f0530 != 0) {
            for (slot = 0; slot < 8; ++slot) {
                if (g_status_685170.buffers.party_rows[slot].occupied != 0 &&
                    g_status_685170.buffers.characters[slot].hp_current != 0) {
                    AimAtMonsterLocation00537950(slot, g_level_block->highlighted_item, 0);
                }
            }
            assign = 0;
        }
        if (assign != 0) {
            if (g_status_685170.selected_character == -1) {
                srAssertFail("gStatus.iSelectedCharacter != -1", MAIN_GAME_SCREEN_CPP, 0x16db, 0);
            }
            AimAtMonsterLocation00537950(g_status_685170.selected_character,
                                         g_level_block->highlighted_item, 1);
        }
    }
    if (GetFlag68F105() == 0) {
        return 1;
    }
    SGPMouseGetPos(&cursor_pos);
    MipeWorldViewEvent0057E0E0(LEFT_BUTTON_UP, &cursor_pos);
    return 1;
}

/* Help 36: combat monster-list rows. Hit-test by 11-pixel row, hover via
   SetCombatAction, left-up aims, right-up posts the group info notice. */
// FUNCTION: WIZ8 0x00568100
unsigned char MonsterListRegionEvent(const InputAtom* event, W8Region* region)
{
    int row = -1;
    W8MonsterGroup* group = 0;
    int group_id = -1;

    if ((region->flags & W8_REGION_MOUSE_LEAVE) == 0 && gXStatus.fNpcDialogueMode == 0 &&
        gXStatus.iTargetingMode != 3 && gXStatus.iTargetingMode != 4 &&
        gXStatus.iTargetingMode != 6 && gXStatus.active_monster_count != 0 &&
        IsWorldCursorVisible() == 0 && g_flag_0068edd8 == 0) {
        row = (GetAtomCursorY004285A0(event) - region->y1) / 0xb;
        group = GetLiveMonsterGroupAtIndex(row);
        if (group == 0) {
            row = -1;
        } else {
            group_id = group->group_id;
        }
    }
    SetCombatAction(group_id);
    switch (event->usEvent) {
    case LEFT_BUTTON_DOWN:
        region->flags |= W8_REGION_LEFT_BUTTON_HELD;
        return 1;
    case LEFT_BUTTON_UP:
        if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) != 0 &&
            g_status_685170.selected_character != -1 && group_id != -1 &&
            g_level_block->unknown_290[0] != 0) {
            AimAtMonsterGroupMember(g_status_685170.selected_character, group);
        }
        return 1;
    case RIGHT_BUTTON_DOWN:
        region->flags |= W8_REGION_RIGHT_BUTTON_HELD;
        return 1;
    case RIGHT_BUTTON_UP:
        if ((region->flags & W8_REGION_RIGHT_BUTTON_HELD) != 0 && group_id != -1 &&
            gXStatus.fSpellCastMode == 0 && gXStatus.fNpcDialogueMode == 0 &&
            gXStatus.fItemSelectMode == 0 && gXStatus.fLockInteractMode == 0 &&
            gXStatus.fTrapInteractMode == 0) {
            ShowMonsterGroupInfoNotice(group_id);
        }
        return 1;
    default:
        return row == -1;
    }
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

/* Refresh the target cursor as the mouse moves over the world view. A
   highlighted monster checks targetability and the mode flags; a picked prop
   checks whether its trigger takes the in-cursor item or shows a message;
   otherwise the cursor comes from the current targeting state. */
// FUNCTION: WIZ8 0x0056a5d0
void UpdateWorldViewCursor0056A5D0(const InputAtom* event, int target_needed)
{
    int cursor = gXStatus.iCurrentCursor;
    if (cursor == W8_CURSOR_INVALID_TARGET) {
        return;
    }
    if (g_level_block->highlighted_item == -1) {
        if (gXStatus.iTargetingMode == 0) {
            if (g_level_block->selected_item != -1) {
                if (g_flag_006840bc == 0) {
                    SetTargetCursor(5);
                    return;
                }
                SetTargetCursor(cursor);
                return;
            }
            int cursor_y = GetAtomCursorY004285A0(event);
            int cursor_x = GetAtomCursorX00428580(event);
            int prop_index = ForwardSelectedPropIndex004503B0(g_world, cursor_x, cursor_y);
            if (prop_index > -1) {
                W8Prop* prop = static_cast<W8Prop*>(PLGet(g_world->plsProps, prop_index));
                if (g_flag_006840bc == 0) {
                    if (prop != 0) {
                        if (prop->TriggerRequiresItem0044E380() &&
                            g_status_685170.item_in_cursor != 0) {
                            SetTargetCursor(0x10);
                            return;
                        }
                        SetTargetCursor(prop->TriggerHasActionMessage0044E360() ? 13 : 5);
                        return;
                    }
                    SetTargetCursor(5);
                    return;
                }
                SetTargetCursor(cursor);
                return;
            }
        }
        cursor = 0;
    } else {
        unsigned int monster_index = MonsterGetIndexByLocationID(
            0x1e1a, MAIN_GAME_SCREEN_CPP, g_level_block->highlighted_item, 1);
        MonsterGetScriptPartByLocationIndex(monster_index);
        if (!CanTargetMonster(g_status_685170.selected_character, g_level_block->highlighted_item,
                              1, 0)) {
            SetTargetCursor(cursor);
            return;
        }
        if (target_needed == 0 && gXStatus.fCombatMode == 0 && gXStatus.fSpellCastMode == 0 &&
            gXStatus.fItemSelectMode == 0 && gXStatus.fNpcDialogueMode == 0 &&
            gXStatus.fLockInteractMode == 0 && gXStatus.fTrapInteractMode == 0) {
            SetTargetCursor(W8_CURSOR_VALID_TARGET);
            return;
        }
        cursor = 1;
    }
    SetTargetCursor(GetTargetingCursorForState(cursor));
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
bool IsScreenIdle(void)
{
    if (gXStatus.fCombatMode == 0 && gXStatus.fSpellCastMode == 0 &&
        gXStatus.fItemSelectMode == 0 && gXStatus.fNpcDialogueMode == 0 &&
        gXStatus.fLockInteractMode == 0 && gXStatus.fTrapInteractMode == 0) {
        return true;
    }
    return false;
}

/* The automap key: tear down whichever dialogue overlay mode is active - the
   mode-3 NPC dialogue pane, the mode-5 spell panel or the mode-6
   portrait-hover overlay - reset the mode and transition to the automap
   screen. */
// FUNCTION: WIZ8 0x00561480
void OpenAutomapScreen(void)
{
    switch (g_main_game_mode_0068eddc) {
    case 3:
        if (gXStatus.fNpcDialogueMode != 0) {
            Function56E800(0);
        }
        break;
    case 5:
        Function5187E0();
        break;
    case 6:
        DismissHighlightOverlay();
        break;
    }
    g_main_game_mode_0068eddc = 0;
    SetPendingScreenState(W8_SCREEN_AUTOMAP);
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
    if (g_level_block->action_panel_visible == 0) {
        DisableRegionInput(0x59);
        RegionSetDisable(0x14);
    }
}

/* Drain the pending mouselook yaw/pitch into the camera, optionally scaling
   mid-range deltas by frame time when mouselook smoothing is enabled. */
// FUNCTION: WIZ8 0x00568C40
void ApplyPendingMouselook(void)
{
    unsigned int now;
    unsigned int elapsed;
    float scale;
    float pitch_step;
    float yaw_step;
    float yaw;
    float pitch;

    if ((g_mouselook_tick_init_0068edb4 & 1) == 0) {
        g_mouselook_tick_init_0068edb4 =
            static_cast<unsigned char>(g_mouselook_tick_init_0068edb4 | 1);
        g_mouselook_last_tick_0068edb0 = GetTickCount();
    }
    now = GetTickCount();
    elapsed = now - g_mouselook_last_tick_0068edb0;
    g_mouselook_last_tick_0068edb0 = now;
    scale = g_generator_jitter_fraction;
    if (elapsed > 9 && elapsed < 0x33) {
        scale = (elapsed / 10) * g_generator_jitter_fraction;
    } else if (elapsed > 9) {
        scale = g_float_005ebb38;
    }
    if (g_mouselook_pending_pitch_0068ede4 == g_float_005ebb34 &&
        g_mouselook_pending_yaw_0068ede0 == g_float_005ebb34) {
        return;
    }
    if (g_settings_6850c8.mouselook_smoothing == 0) {
        pitch_step = g_mouselook_pending_pitch_0068ede4;
        yaw_step = g_mouselook_pending_yaw_0068ede0;
    } else {
        if (fabs(g_mouselook_pending_pitch_0068ede4) < g_mouselook_smooth_min_005ee9a4 ||
            fabs(g_mouselook_pending_pitch_0068ede4) > g_mouselook_smooth_max_005ee9a0) {
            pitch_step = g_mouselook_pending_pitch_0068ede4;
        } else {
            pitch_step = g_mouselook_pending_pitch_0068ede4 * scale;
        }
        if (fabs(g_mouselook_pending_yaw_0068ede0) < g_mouselook_smooth_min_005ee9a4 ||
            fabs(g_mouselook_pending_yaw_0068ede0) > g_mouselook_smooth_max_005ee9a0) {
            yaw_step = g_mouselook_pending_yaw_0068ede0;
        } else {
            yaw_step = g_mouselook_pending_yaw_0068ede0 * scale;
        }
    }
    GetCameraOrientation(&yaw, &pitch);
    yaw += yaw_step;
    pitch += pitch_step;
    g_mouselook_pending_pitch_0068ede4 -= pitch_step;
    g_mouselook_pending_yaw_0068ede0 -= yaw_step;
    SetCameraOrientation(&yaw, &pitch, 0);
}

/* After the tooltip delay elapses, clear any previous highlight slots (and
   redraw them), then park the pending subject on the slot its kind selects. */
// FUNCTION: WIZ8 0x00569CC0
void ApplyPendingTooltip(void)
{
    bool refresh_formation = false;

    if (g_level_block->tooltip_pending == 0) {
        return;
    }
    if (GetTickCount() - g_level_block->tooltip_since < 0x33) {
        return;
    }
    if (g_level_block->highlight_override != -1) {
        RequestRedraw(1u << (g_level_block->highlight_override & 0x1f));
        g_level_block->highlight_override = -1;
    }
    if (g_level_block->party_slots_170[0] != -1) {
        RequestRedraw(1u << (g_level_block->party_slots_170[0] & 0x1f));
        g_level_block->party_slots_170[0] = -1;
    }
    if (g_level_block->party_slots_170[1] != -1) {
        RequestRedraw(1u << (g_level_block->party_slots_170[1] & 0x1f));
        g_level_block->party_slots_170[1] = -1;
    }
    if (g_level_block->party_slots_170[2] != -1) {
        RequestRedraw(1u << (g_level_block->party_slots_170[2] & 0x1f));
        g_level_block->party_slots_170[2] = -1;
    }
    if (g_level_block->party_slots_170[3] != -1) {
        RequestRedraw(1u << (g_level_block->party_slots_170[3] & 0x1f));
        g_level_block->party_slots_170[3] = -1;
    }
    if (g_level_block->party_slots_170[5] != -1) {
        RequestRedraw(1u << (g_level_block->party_slots_170[5] & 0x1f));
        g_level_block->party_slots_170[5] = -1;
    }
    if (g_level_block->formation_highlight_party_slot != -1) {
        RequestRedraw(1u << (g_level_block->formation_highlight_party_slot & 0x1f));
        g_level_block->formation_highlight_party_slot = -1;
        refresh_formation = true;
    }
    if (g_level_block->tooltip_subject != -1) {
        RequestRedraw(1u << (g_level_block->tooltip_subject & 0x1f));
        switch (g_level_block->tooltip_kind) {
        case 0:
            g_level_block->highlight_override = g_level_block->tooltip_subject;
            break;
        case 1:
            g_level_block->party_slots_170[0] = g_level_block->tooltip_subject;
            break;
        case 2:
            g_level_block->party_slots_170[1] = g_level_block->tooltip_subject;
            break;
        case 3:
            g_level_block->party_slots_170[2] = g_level_block->tooltip_subject;
            break;
        case 4:
            g_level_block->party_slots_170[3] = g_level_block->tooltip_subject;
            break;
        case 5:
            g_level_block->party_slots_170[5] = g_level_block->tooltip_subject;
            break;
        case 6:
            g_level_block->formation_highlight_party_slot = g_level_block->tooltip_subject;
            refresh_formation = true;
            break;
        case 7:
            g_level_block->formation_highlight_party_slot = g_level_block->tooltip_subject;
            break;
        }
    }
    if (refresh_formation) {
        RefreshFormationBoard();
    }
    g_level_block->tooltip_pending = 0;
    g_level_block->tooltip_subject = -1;
    g_level_block->tooltip_kind = -1;
}

/* Raise or drop the radar map panel and restore the viewport mode when the
   raise latch changes. */
// FUNCTION: WIZ8 0x00568EB0
void SetRadarMapVisible(unsigned char visible)
{
    if (visible == 1) {
        g_level_block->radar_map_visible = visible;
        g_level_block->radar_map_alternate = 0;
        RegionSetEnable(0x12);
        EnableRegionInput(0x62);
        EnableRadarMap(1);
        if (gXStatus.fCombatMode != 0) {
            ZoomRadarMapIn();
        } else {
            ZoomRadarMapOut();
        }
        if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
            g_level_block->redraw_flags |= 0x8200;
        }
    } else {
        g_level_block->radar_map_visible = 0;
        DisableRegionInput(0x62);
        RegionSetDisable(0x12);
        EnableRadarMap(0);
        ReleaseRadarMap();
        if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
            g_level_block->redraw_flags |= 0x8200;
        }
    }
    if (visible != g_flag_0068edbc) {
        if (gXStatus.fSpellCastMode == 0) {
            if ((gXStatus.fNpcDialogueMode == 0 || CanOpenNpcDialogue() != 0) &&
                gXStatus.fLockInteractMode == 0 && gXStatus.fTrapInteractMode == 0 &&
                gXStatus.fItemSelectMode == 0 && g_level_block->action_panel_visible == 0 &&
                g_level_block->formation_board_visible != 0 &&
                g_level_block->radar_map_visible != 0 &&
                g_settings_6850c8.main_ui_mode == W8_MAIN_UI_MODE_PORTRAITS) {
                SetViewportMode(4);
                g_flag_0068edbc = visible;
                return;
            }
            if (gXStatus.fSpellCastMode == 0 &&
                (gXStatus.fNpcDialogueMode == 0 || CanOpenNpcDialogue() != 0) &&
                gXStatus.fLockInteractMode == 0 && gXStatus.fTrapInteractMode == 0 &&
                gXStatus.fItemSelectMode == 0 &&
                (g_level_block->formation_board_visible == 0 ||
                 g_level_block->radar_map_visible == 0 ||
                 g_level_block->action_panel_visible == 0)) {
                SetViewportMode(0);
                g_flag_0068edbc = visible;
                return;
            }
        }
        if (g_settings_6850c8.main_ui_mode == W8_MAIN_UI_MODE_FORMATION) {
            SetViewportMode(1);
            g_flag_0068edbc = visible;
            return;
        }
        if (g_settings_6850c8.main_ui_mode == W8_MAIN_UI_MODE_RADAR) {
            SetViewportMode(0);
            g_flag_0068edbc = visible;
            return;
        }
        SetViewportMode(2);
    }
    g_flag_0068edbc = visible;
}

/* Raise or drop the combat action panel and restore the viewport mode when the
   raise latch changes. */
// FUNCTION: WIZ8 0x00569120
void SetActionPanelVisible(unsigned char visible)
{
    if (visible == 1) {
        g_level_block->action_panel_visible = visible;
        if (g_level_block->action_panel_visible != 0) {
            RegionSetEnable(0x14);
            EnableRegionInput(0x59);
            EnableRegionInput(0x52);
            EnableRegionInput(0x53);
            EnableRegionInput(0x54);
            EnableRegionInput(0x55);
            EnableRegionInput(0x56);
            EnableRegionInput(0x57);
            EnableRegionInput(0x58);
        }
        if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
            g_level_block->redraw_flags |= 0x8200;
        }
    } else {
        g_level_block->action_panel_visible = 0;
        DisableRegionInput(0x52);
        DisableRegionInput(0x53);
        DisableRegionInput(0x54);
        DisableRegionInput(0x55);
        DisableRegionInput(0x56);
        DisableRegionInput(0x57);
        DisableRegionInput(0x58);
        if (g_level_block->action_panel_visible == 0) {
            DisableRegionInput(0x59);
            RegionSetDisable(0x14);
        }
        if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
            g_level_block->redraw_flags |= 0x8200;
        }
    }
    if (visible != g_flag_0068edc8) {
        if (gXStatus.fSpellCastMode == 0) {
            if ((gXStatus.fNpcDialogueMode == 0 || CanOpenNpcDialogue() != 0) &&
                gXStatus.fLockInteractMode == 0 && gXStatus.fTrapInteractMode == 0 &&
                gXStatus.fItemSelectMode == 0 && g_level_block->action_panel_visible == 0 &&
                g_level_block->formation_board_visible != 0 &&
                g_level_block->radar_map_visible != 0 &&
                g_settings_6850c8.main_ui_mode == W8_MAIN_UI_MODE_PORTRAITS) {
                SetViewportMode(4);
                g_flag_0068edc8 = visible;
                return;
            }
            if (gXStatus.fSpellCastMode == 0 &&
                (gXStatus.fNpcDialogueMode == 0 || CanOpenNpcDialogue() != 0) &&
                gXStatus.fLockInteractMode == 0 && gXStatus.fTrapInteractMode == 0 &&
                gXStatus.fItemSelectMode == 0 &&
                (g_level_block->formation_board_visible == 0 ||
                 g_level_block->radar_map_visible == 0 ||
                 g_level_block->action_panel_visible == 0)) {
                SetViewportMode(0);
                g_flag_0068edc8 = visible;
                return;
            }
        }
        if (g_settings_6850c8.main_ui_mode == W8_MAIN_UI_MODE_FORMATION) {
            SetViewportMode(1);
            g_flag_0068edc8 = visible;
            return;
        }
        if (g_settings_6850c8.main_ui_mode == W8_MAIN_UI_MODE_RADAR) {
            SetViewportMode(0);
            g_flag_0068edc8 = visible;
            return;
        }
        SetViewportMode(2);
    }
    g_flag_0068edc8 = visible;
}

/* Raise or drop the formation board and restore the viewport mode when the
   raise latch changes. */
// FUNCTION: WIZ8 0x00569390
void SetFormationBoardVisible(unsigned char visible)
{
    if (visible == 1) {
        g_level_block->formation_board_visible = 1;
        g_level_block->formation_board_alternate = 0;
        RegionSetEnable(0x13);
        RefreshFormationBoard();
    } else {
        g_level_block->formation_board_visible = 0;
        RegionSetDisable(0x13);
        ReleaseFormationBoard();
    }
    if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
        g_level_block->redraw_flags |= 0x8200;
    }
    if (visible != g_flag_0068edc9) {
        if (gXStatus.fSpellCastMode == 0) {
            if ((gXStatus.fNpcDialogueMode == 0 || CanOpenNpcDialogue() != 0) &&
                gXStatus.fLockInteractMode == 0 && gXStatus.fTrapInteractMode == 0 &&
                gXStatus.fItemSelectMode == 0 && g_level_block->action_panel_visible == 0 &&
                g_level_block->formation_board_visible != 0 &&
                g_level_block->radar_map_visible != 0 &&
                g_settings_6850c8.main_ui_mode == W8_MAIN_UI_MODE_PORTRAITS) {
                SetViewportMode(4);
                g_flag_0068edc9 = visible;
                return;
            }
            if (gXStatus.fSpellCastMode == 0 &&
                (gXStatus.fNpcDialogueMode == 0 || CanOpenNpcDialogue() != 0) &&
                gXStatus.fLockInteractMode == 0 && gXStatus.fTrapInteractMode == 0 &&
                gXStatus.fItemSelectMode == 0 &&
                (g_level_block->formation_board_visible == 0 ||
                 g_level_block->radar_map_visible == 0 ||
                 g_level_block->action_panel_visible == 0)) {
                SetViewportMode(0);
                g_flag_0068edc9 = visible;
                return;
            }
        }
        if (g_settings_6850c8.main_ui_mode == W8_MAIN_UI_MODE_FORMATION) {
            SetViewportMode(1);
            g_flag_0068edc9 = visible;
            return;
        }
        if (g_settings_6850c8.main_ui_mode == W8_MAIN_UI_MODE_RADAR) {
            SetViewportMode(0);
            g_flag_0068edc9 = visible;
            return;
        }
        SetViewportMode(2);
    }
    g_flag_0068edc9 = visible;
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
        CloseSpellCastingView();
    }
    if (gXStatus.fItemSelectMode != 0) {
        CloseUseItemSelectView();
    }
    if (gXStatus.fReviewCharacterMode != 0) {
        CloseFormationPanel();
    }
}

/* Whether the typed-text cursor of the active NPC dialogue is up; only the
   main-game action keys keep working while it owns input. */
// FUNCTION: WIZ8 0x0056efb0
unsigned char IsNpcDialogueCursorActive(void)
{
    if (gXStatus.fNpcDialogueMode == 0) {
        return 0;
    }
    return g_screen_state_00649f1c->dialogue_cursor_flag;
}

/* Whether the numbered action-key command may run in the current UI mode:
   dialogue, interaction, camp and surprise lock most of them out, the item
   and spell views admit only their own toggle, and combat commands check the
   entry's submenu state. Command 0x10 re-checks the slot's queued action's
   own command. */
// FUNCTION: WIZ8 0x0056af80
unsigned char IsMGSActionKeyEnabled(short command)
{
    short state;

    state = -1;
recheck:
    if (gXStatus.fNpcDialogueMode != 0) {
        if (command != W8_MGS_ACTION_JOURNAL || IsNpcDialogueCursorActive() != 0 ||
            CanOpenNpcDialogue() != 0) {
            return 0;
        }
    }
    if (gXStatus.fLockInteractMode != 0 || gXStatus.fTrapInteractMode != 0 ||
        gXStatus.fReviewCharacterMode != 0) {
        return 0;
    }
    if (gXStatus.fItemSelectMode != 0 && command != W8_MGS_ACTION_USE_ITEM_VIEW) {
        return 0;
    }
    if (gXStatus.fSpellCastMode != 0 && command != W8_MGS_ACTION_SPELL_VIEW) {
        return 0;
    }
    if (gXStatus.fSurprisePossible != 0 || gXStatus.fCampMode != 0) {
        return 0;
    }
    if (gXStatus.fCombatMode == 0) {
        switch (command) {
        case W8_MGS_ACTION_JOURNAL:
            return 1;
        case W8_MGS_ACTION_USE_ITEM_VIEW:
            state = GetSubMenuEntryState(W8_SUBMENU_ITEMS, 1, g_status_685170.selected_character);
            break;
        case W8_MGS_ACTION_USE_RECORDED_ITEM:
            state = GetSubMenuEntryState(W8_SUBMENU_ITEMS, 2, g_status_685170.selected_character);
            break;
        case W8_MGS_ACTION_SPELL_VIEW:
            state = GetSubMenuEntryState(W8_SUBMENU_SPELLS, 0, g_status_685170.selected_character);
            break;
        case W8_MGS_ACTION_CAST_RECORDED_SPELL:
            state = GetSubMenuEntryState(W8_SUBMENU_SPELLS, 1, g_status_685170.selected_character);
            break;
        default:
            return 0;
        }
    } else {
        if (g_combat_state->flag_001 == 0) {
            return 0;
        }
        switch (command) {
        case W8_MGS_ACTION_JOURNAL:
            return 0;
        case W8_MGS_ACTION_USE_ITEM_VIEW:
            state = GetSubMenuEntryState(W8_SUBMENU_ITEMS, 1, g_status_685170.selected_character);
            break;
        case W8_MGS_ACTION_USE_RECORDED_ITEM:
            state = GetSubMenuEntryState(W8_SUBMENU_ITEMS, 2, g_status_685170.selected_character);
            break;
        case W8_MGS_ACTION_SPELL_VIEW:
            state = GetSubMenuEntryState(W8_SUBMENU_SPELLS, 0, g_status_685170.selected_character);
            break;
        case W8_MGS_ACTION_CAST_RECORDED_SPELL:
            state = GetSubMenuEntryState(W8_SUBMENU_SPELLS, 1, g_status_685170.selected_character);
            break;
        case W8_MGS_ACTION_BREATHE:
            state = GetSubMenuEntryState(W8_SUBMENU_ATTACK, 2, g_status_685170.selected_character);
            break;
        case W8_MGS_ACTION_BREATH_ATTACK:
            return IsPartySlotEligible00524A10(g_status_685170.selected_character) != 0 &&
                   CanPartySlotReBreathe(g_status_685170.selected_character) != 0;
        case W8_MGS_ACTION_ATTACK:
            state = GetSubMenuEntryState(W8_SUBMENU_ATTACK, 0, g_status_685170.selected_character);
            break;
        case W8_MGS_ACTION_BERSERK:
            state = GetSubMenuEntryState(W8_SUBMENU_ATTACK, 1, g_status_685170.selected_character);
            break;
        case W8_MGS_ACTION_TURN_UNDEAD:
            state = GetSubMenuEntryState(W8_SUBMENU_ATTACK, 3, g_status_685170.selected_character);
            break;
        case W8_MGS_ACTION_PRAY:
            state = GetSubMenuEntryState(W8_SUBMENU_ATTACK, 4, g_status_685170.selected_character);
            break;
        case W8_MGS_ACTION_DEFEND:
            state = GetSubMenuEntryState(W8_SUBMENU_DEFEND, 0, g_status_685170.selected_character);
            break;
        case W8_MGS_ACTION_PROTECT:
            state = GetSubMenuEntryState(W8_SUBMENU_DEFEND, 1, g_status_685170.selected_character);
            break;
        case W8_MGS_ACTION_EQUIP:
            state = GetSubMenuEntryState(W8_SUBMENU_ITEMS, 0, g_status_685170.selected_character);
            break;
        case W8_MGS_ACTION_WALK:
        case W8_MGS_ACTION_RUN:
            return AnyCharacterEngaged();
        case W8_MGS_ACTION_REPEAT:
            switch (g_status_685170.buffers.party_rows[g_status_685170.selected_character]
                        .queued_action -
                    2) {
            case W8_MGS_ACTION_JOURNAL:
                command = W8_MGS_ACTION_BREATH_ATTACK;
                goto recheck;
            case W8_MGS_ACTION_USE_ITEM_VIEW:
                command = W8_MGS_ACTION_TURN_UNDEAD;
                goto recheck;
            case W8_MGS_ACTION_CAST_RECORDED_SPELL:
                command = W8_MGS_ACTION_PRAY;
                goto recheck;
            case W8_MGS_ACTION_BREATHE:
                command = W8_MGS_ACTION_CAST_RECORDED_SPELL;
                goto recheck;
            case W8_MGS_ACTION_BREATH_ATTACK:
                command = W8_MGS_ACTION_USE_RECORDED_ITEM;
                goto recheck;
            case W8_MGS_ACTION_ATTACK:
                command = W8_MGS_ACTION_EQUIP;
                goto recheck;
            default:
                return 0;
            }
        default:
            srAssertFail("FALSE", "C:\\Projects\\Wizardry 8\\Local Screens\\MainGameScreen.cpp",
                         0x207d, "Error with handling main game action keyboard equivalent");
            break;
        }
    }
    if (state == -1) {
        srAssertFail("iActionState != -1",
                     "C:\\Projects\\Wizardry 8\\Local Screens\\MainGameScreen.cpp", 0x20aa,
                     "Error with handling main game action keyboard equivalent");
    }
    if (IsPartySlotEligible00524A10(g_status_685170.selected_character) == 0) {
        return 0;
    }
    return state == 0 || state == 1;
}

/* Run the numbered action-key command: dismiss the transient menu first, then
   act - toggle the item or spell view, fire the recorded spell/item, choose
   the combat action, or dispatch the slot's queued action's own command. */
// FUNCTION: WIZ8 0x0056b270
void RunMGSActionKey(short command)
{
    if (g_level_block->combat_end_notification != -1) {
        DestroySubMenuControls();
    }
    if (g_level_block->keyboard_menu_open != 0) {
        CloseKeyboardMenu();
    }
    switch (command) {
    case W8_MGS_ACTION_JOURNAL:
        if (gXStatus.fNpcDialogueMode != 0) {
            gXStatus.fCampMode = 1;
        }
        SetPendingScreenState(W8_SCREEN_JOURNAL);
        break;
    case W8_MGS_ACTION_USE_ITEM_VIEW:
        if (gXStatus.fItemSelectMode != 0) {
            CloseUseItemSelectView();
        } else {
            OpenUseItemSelectView(g_status_685170.selected_character);
        }
        break;
    case W8_MGS_ACTION_USE_RECORDED_ITEM:
        StartCharacterItemUse(g_status_685170.selected_character);
        break;
    case W8_MGS_ACTION_SPELL_VIEW:
        if (gXStatus.fSpellCastMode != 0) {
            CloseSpellCastingView();
        } else {
            OpenSpellCastingView(g_status_685170.selected_character);
        }
        break;
    case W8_MGS_ACTION_CAST_RECORDED_SPELL:
        StartCharacterSpellCast(g_status_685170.selected_character, 0);
        break;
    case W8_MGS_ACTION_BREATHE:
        ChooseAction(g_status_685170.selected_character, W8_ACTION_BREATHE, -1, 0, 0, 1);
        DrawSubMenuCharacterAction();
        break;
    case W8_MGS_ACTION_BREATH_ATTACK:
        StartCharacterBreathAttack(g_status_685170.selected_character);
        break;
    case W8_MGS_ACTION_ATTACK:
        ChooseAction(g_status_685170.selected_character, W8_ACTION_ATTACK, -1, 0, 0, 1);
        DrawSubMenuCharacterAction();
        break;
    case W8_MGS_ACTION_BERSERK:
        ChooseAction(g_status_685170.selected_character, W8_ACTION_BERSERK, -1, 0, 0, 1);
        DrawSubMenuCharacterAction();
        break;
    case W8_MGS_ACTION_TURN_UNDEAD:
        ChooseAction(g_status_685170.selected_character, W8_ACTION_TURN_UNDEAD, -1, 0, 0, 1);
        DrawSubMenuCharacterAction();
        break;
    case W8_MGS_ACTION_PRAY:
        ChooseAction(g_status_685170.selected_character, W8_ACTION_PRAY, -1, 0, 0, 1);
        DrawSubMenuCharacterAction();
        break;
    case W8_MGS_ACTION_DEFEND:
        ChooseAction(g_status_685170.selected_character, W8_ACTION_DEFEND, -1, 0, 0, 1);
        DrawSubMenuCharacterAction();
        break;
    case W8_MGS_ACTION_PROTECT:
        ChooseAction(g_status_685170.selected_character, W8_ACTION_PROTECT, -1, 0, 0, 1);
        DrawSubMenuCharacterAction();
        break;
    case W8_MGS_ACTION_EQUIP:
        ChooseAction(g_status_685170.selected_character, W8_ACTION_EQUIP, -1, 0, 0, 1);
        DrawSubMenuCharacterAction();
        break;
    case W8_MGS_ACTION_WALK:
        ChooseAction(g_status_685170.selected_character, W8_ACTION_WALK, -1, 0, 0, 1);
        DrawSubMenuCharacterAction();
        break;
    case W8_MGS_ACTION_RUN:
        ChooseAction(g_status_685170.selected_character, W8_ACTION_RUN, -1, 0, 0, 1);
        DrawSubMenuCharacterAction();
        break;
    case W8_MGS_ACTION_REPEAT:
        switch (
            g_status_685170.buffers.party_rows[g_status_685170.selected_character].queued_action -
            2) {
        case W8_MGS_ACTION_JOURNAL:
            RunMGSActionKey(W8_MGS_ACTION_BREATH_ATTACK);
            break;
        case W8_MGS_ACTION_USE_ITEM_VIEW:
            RunMGSActionKey(W8_MGS_ACTION_TURN_UNDEAD);
            break;
        case W8_MGS_ACTION_CAST_RECORDED_SPELL:
            RunMGSActionKey(W8_MGS_ACTION_PRAY);
            break;
        case W8_MGS_ACTION_BREATHE:
            RunMGSActionKey(W8_MGS_ACTION_CAST_RECORDED_SPELL);
            break;
        case W8_MGS_ACTION_BREATH_ATTACK:
            RunMGSActionKey(W8_MGS_ACTION_USE_RECORDED_ITEM);
            break;
        case W8_MGS_ACTION_ATTACK:
            RunMGSActionKey(W8_MGS_ACTION_EQUIP);
            break;
        }
        break;
    default:
        srAssertFail("FALSE", "C:\\Projects\\Wizardry 8\\Local Screens\\MainGameScreen.cpp", 0x2151,
                     "Error executing main game screen action");
        break;
    }
}

/* Fire the numbered action-key command when the current UI mode allows it. */
// FUNCTION: WIZ8 0x0056b4c0
void TryMGSActionKey(int command)
{
    if (IsMGSActionKeyEnabled(command) != 0) {
        RunMGSActionKey(command);
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
        (npc->record->kind != 7 || GetFact(W8_FACT_ARNIKA_MYLES_MEET_ONCE) != 1)) {
        QueueNpcScriptNotice(npc, item, line, suppress, 0);
    }
}

/* Queue one NPC script notice for the dispatch pass. A second NPC of the same
   kind already in the world suppresses the notice unless the record carries
   the 0x054 binding flag, a live monster with a condition at or above 0xf
   takes none, and a pending notice blocks the next until it drains. Kind
   0x10/0x11 NPCs with fact 0xbf substitute their own notice line and raise
   the flag byte. */
// FUNCTION: WIZ8 0x0056C5E0
void QueueNpcScriptNotice(W8NpcState* npc, W8ItemInstance* item, int line, int suppress, int arg)
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
    if ((npc->name_style == W8_NPC_DRAZIC || npc->name_style == W8_NPC_RODAN) &&
        GetFact(W8_FACT_PEACE_ACHIEVED) != 0) {
        flag = 1;
        line = npc->name_style == W8_NPC_DRAZIC ? 0x23 : 0x1d;
    }
    if (g_flag_68f0f9 != 0) {
        return;
    }
    g_pending_notice_68ee60.flag = flag;
    g_pending_notice_68ee60.npc = npc;
    g_pending_notice_68ee60.line = line;
    g_pending_notice_68ee60.force = static_cast<unsigned char>(arg);
    if (item != 0) {
        g_pending_notice_68ee60.item = *item;
    } else {
        EmptyItemRecord(&g_pending_notice_68ee60.item, 0, 1);
    }
    QueueNpcMessageLine(W8_NPC_MSG_DISPATCH_PENDING_NOTICE, 0);
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
void BeginNpcDialogueInternal(W8NpcState* npc, W8ItemInstance* item, int quote, int flags,
                              int force)
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
        SelectNpcDialogueSpeaker(npc, flags);
    }
    if (force != 0) {
        OpenNpcDialoguePanel(npc, item, 1);
        return;
    }
    if (npc->record->unknown_054 == 0 && npc->record->flag_2ea == 0) {
        if (GetNpcDispositionBand(npc) == 2 && npc->record->unknown_056 == 0) {
            QueueNpcScriptLine(0x18, 0, 0, 0);
            return;
        }
        if (flags == 0) {
            OpenNpcDialoguePanel(npc, item, 0);
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
            HandleNpcDialogueItem(&state->pending_item_1ed);
        } else if (quote == -1) {
            HandleNpcDialogueDeparture(1);
        } else {
            QueueNpcScriptLine(quote, 0, 0, 0);
        }
    } else {
        if (quote == -1) {
            quote = 0;
        }
        QueueNpcScriptLine(quote, 0, 0, 0);
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
void BeginNpcDialogue(W8NpcState* npc, W8ItemInstance* item, int quote, int flags, int force)
{
    BeginNpcDialogueInternal(npc, item, quote, flags, force);
}

/* Dispatch the queued NPC script notice: the item goes across only while it
   still carries an id, and the flag pair at +0x14 travels as one dword. */
// FUNCTION: WIZ8 0x0056CA90
void DispatchPendingNpcScriptNotice(void)
{
    W8ItemInstance* item;
    int flags;

    item = 0;
    if (g_pending_notice_68ee60.item.item_id != -1) {
        item = &g_pending_notice_68ee60.item;
    }
    flags = *reinterpret_cast<int*>(&g_pending_notice_68ee60.flag); /* reinterpret-ok: the queued
            flag/force bytes are dispatched to BeginNpcDialogueInternal as one packed dword */
    BeginNpcDialogueInternal(g_pending_notice_68ee60.npc, item, g_pending_notice_68ee60.line, flags,
                             (flags >> 8) & 0xff);
}

/* Open the NPC dialogue panel. After the shared screen reset and the
   dialogue-UI build, a carried item goes through the pending-item path -
   inspecting it decides between the trade switch and a disposition check -
   while everything else falls to the force gate and then the
   dispatch: a record-0x056 NPC takes the plain quote, otherwise the
   disposition band picks the hostile or friendly entry. */
// FUNCTION: WIZ8 0x0056CAD0
unsigned char OpenNpcDialoguePanel(W8NpcState* npc, W8ItemInstance* item, unsigned char force)
{
    W8MainScreenState* state;
    W8MonsterInfo* info;
    W8MonsterInfo* dialogue_info;
    unsigned char band;
    wchar_t space[2];
    srVector3T<float> position;

    UpdateScreenOverlays(0);
    gXStatus.fNpcDialogueMode = 1;
    CloseMainGameOverlays();
    if (npc->record->flag_055 != 0) {
        RestockNpcInventory(npc);
    }
    state = g_screen_state_00649f1c;
    state->value_fc = W8_DIALOGUE_LAYOUT_NONE;
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
    state->flag_251 = 0;
    state->flag_229 = 0;
    state->value_22c = 0;
    state->value_238 = 0;
    state->flag_23c = 1;
    state->value_258 = -1;
    state->value_25c = 0;
    if (g_settings_6850c8.main_ui_mode != W8_MAIN_UI_MODE_PORTRAITS) {
        ApplyMainGameModeFlag(W8_MAIN_UI_MODE_PORTRAITS, 0);
    } else {
        SetViewportMode(GetMainGameViewportMode());
    }
    if (gXStatus.fCampMode == 0) {
        g_screen_state_00649f1c->value_f0 = g_settings_6850c8.main_ui_mode;
    }
    CreateNpcDialogueControls();
    SetRegionBounds(0x8a, 0x17, 0x166, 0x269, 0x1c2);
    g_level_block->flag_271 = 0;
    RegionSetEnable(0x15);
    EnableRegionInput(0x52);
    EnableRegionInput(0x53);
    EnableRegionInput(0x54);
    EnableRegionInput(0x55);
    g_level_block->action_panel_visible = 1;
    gXStatus.fCampMode = 0;
    if (item != 0) {
        state = g_screen_state_00649f1c;
        state->pending_item_1ed = *item;
        if (g_status_685170.item_in_cursor != 0) {
            state->flag_1f9 = 1;
            ClearHeldItemDisplay();
        }
        if (npc->record->unknown_056 != 0) {
            if (HandleNpcDialogueItem(&state->pending_item_1ed) == 0) {
                OpenNpcDialogueTranscriptLayout();
                goto dispatch;
            }
        } else if (HandleNpcDialogueItem(&state->pending_item_1ed) == 0) {
            switch (g_screen_state_00649f1c->value_fc) {
            case 1:
                CloseNpcDialogueMode1Layout();
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
                SetNpcDialogueLayoutMode(0);
                break;
            case 3:
                CloseNpcDialogueTranscriptLayout();
                break;
            case 4:
                CloseNpcDialogueOptionLayout();
                break;
            case 5:
                CloseNpcDialogueMode5Layout();
                break;
            }
            if (GetNpcDispositionBand(g_screen_state_00649f1c->dialogue_npc) == 0) {
                OpenNpcDialogueTranscriptLayout();
            } else {
                ShowNpcDialogueTopicMenu();
            }
            goto dispatch;
        }
    }
    if (force != 0) {
        goto tail;
    }
dispatch:
    if (g_screen_state_00649f1c->dialogue_npc->record->unknown_056 != 0) {
        QueueNpcScriptLine(0, 0, 0, 0);
        OpenNpcDialogueTranscriptLayout();
    } else {
        band = GetNpcDispositionBand(g_screen_state_00649f1c->dialogue_npc);
        if (g_screen_state_00649f1c->dialogue_npc->name_style == W8_NPC_ZANT &&
            GetFact(W8_FACT_TRANG_YOU_ARE_BUSTED) != 0 && GetFact(W8_FACT_ALIGNMENT_UMPANI) == 0) {
            SetFact(W8_FACT_TRANG_YOU_ARE_BUSTED, 0, 0);
        }
        if (band == 0) {
            HandleNpcDialogueDeparture(1);
            OpenNpcDialogueTranscriptLayout();
        } else if (band == 1) {
            QueueNpcScriptLine(2, 0, 0, 0);
            ShowNpcDialogueTopicMenu();
        }
    }
tail:
    RequestRedraw(0x100);
    RequestRedraw(0x1000);
    if (g_flag_0068edd8 != 0) {
        SetFlag603C60();
        g_flag_0068edd8 = 0;
        gfTrackMousePos = 0;
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
    ShowNotice(5, space, 3, -1, 0);
    return 1;
}

/* Stage `npc` as the dialogue NPC: bind its monster's location, clear its
   transient flags, kick the script dialogue, refresh the name caption while
   the dialogue UI is already up, and pick the speaking character - the first
   occupied row, overtaken by any occupied row with a higher skill-0x16
   (communication) level. Every occupied portrait then takes target pose 1.
   A stale disposition snapshot on the NPC drops its 0x1c flag. */
// FUNCTION: WIZ8 0x0056D030
void SelectNpcDialogueSpeaker(W8NpcState* npc, int flags)
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
    if (state->dialogue_npc->greeting_pending != 0) {
        state->dialogue_npc->flag_84 = 0;
    }
    BeginNpcScriptDialogue(state->dialogue_npc, 0);
    if ((state->value_fc == W8_DIALOGUE_LAYOUT_TRANSCRIPT ||
         state->value_fc == W8_DIALOGUE_LAYOUT_TOPIC_MENU) &&
        gXStatus.fNpcDialogueMode != 0) {
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
void PromptNpcDispositionChange(void);
void OnNpcDispositionPromptClosed(W8DialogBase* dialog);
void EnterNpcServiceLayout(void);
void LeaveNpcDialogueLayout(void);
void EnterNpcTradeOptions(void);
void ShowNpcDialogueNotice(void);
void RequestNpcJoinParty(void);
void ScrollNpcDialogueUp(void);
void ScrollNpcDialogueDown(void);
void SubmitNpcDialogueKeyword(void);
void RefreshNpcDialogueTranscript(void);
void SyncNpcDialogueListFilter(void);
void SelectNpcDialogueCategory1(void);
void SelectNpcDialogueCategory2(void);
void SelectNpcDialogueCategory0(void);
void SelectNpcDialogueCategory3(void);
void SelectNpcDialogueCategoryAll(void);
void SetNpcDialogueSubMode4(void);
void Function5AD290(void);
void RestockNpcTradeStock(void);
void SetNpcDialogueSubMode3(void);
void SetNpcDialogueSubMode2(void);
void SetNpcDialogueSubMode5(void);
void SelectNpcTradeMode1(void);
void SelectNpcTradeMode0(void);
void OpenNpcItemAssay(void);
void OnNpcAssayDialogClosed(W8DialogBase* dialog);
void ConfirmNpcTradeSlot(void);
void RequestNpcSpellService3(void);
void RequestNpcSpellService41(void);
void RequestNpcCharacterService(void);
void Function575520(W8DialogBase* dialog);
void Function56FAC0(int index, int, int);
void Function5AE040(void);
void Function572780(void);

/* Build the NPC dialogue UI: the option-button panel on the left, the
   secondary panel beside it, the scrolling text controller, the three
   right-hand panels and the text-input panel, then every text control each
   of them hosts. Buttons get their option masks, help ids and activation
   callbacks as they are created. */
// FUNCTION: WIZ8 0x0056D1D0
void CreateNpcDialogueControls(void)
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
    state->dialogue_scroll_up_button =
        new W8TextControl(panel, 0x66, 0x7e, 3, 0x88, 0xb, 0x1aa, 0, 0x14, 0x18, 0x15, 0x16, 0x17);
    state->dialogue_scroll_down_button = new W8TextControl(panel, 0x67, 0x7e, 0xc, 0x88, 0x14,
                                                           0x1aa, 0, 0x19, 0x1d, 0x1a, 0x1b, 0x1c);

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
    state->dialogue_sort_button =
        new W8TextControl(panel, 0x6b, 6, 2, 0x89, 0xe, 0x1a9, 0, 8, 9, -1, -1, -1);
    state->dialogue_sort_button->AddLayoutFlags(
        g_W8TextControlMask005ED580 | g_W8TextControlMask005ED57C | g_W8TextControlMask005ED578);
    state->dialogue_people_button =
        new W8TextControl(panel, 0x6d, 6, 0xf, 0x41, 0x1b, 0x1a9, 0, 8, 9, -1, -1, -1);
    state->dialogue_people_button->AddLayoutFlags(
        g_W8TextControlMask005ED584 | g_W8TextControlMask005ED580 | g_W8TextControlMask005ED57C |
        g_W8TextControlMask005ED578);
    state->dialogue_places_button =
        new W8TextControl(panel, 0x6e, 0x43, 0xf, 0x89, 0x1b, 0x1a9, 0, 8, 9, -1, -1, -1);
    state->dialogue_places_button->AddLayoutFlags(
        g_W8TextControlMask005ED584 | g_W8TextControlMask005ED580 | g_W8TextControlMask005ED57C |
        g_W8TextControlMask005ED578);
    state->dialogue_items_button =
        new W8TextControl(panel, 0x6f, 6, 0x1b, 0x41, 0x27, 0x1a9, 0, 8, 9, -1, -1, -1);
    state->dialogue_items_button->AddLayoutFlags(
        g_W8TextControlMask005ED584 | g_W8TextControlMask005ED580 | g_W8TextControlMask005ED57C |
        g_W8TextControlMask005ED578);
    state->dialogue_misc_button =
        new W8TextControl(panel, 0x70, 0x43, 0x1b, 0x89, 0x27, 0x1a9, 0, 8, 9, -1, -1, -1);
    state->dialogue_misc_button->AddLayoutFlags(
        g_W8TextControlMask005ED584 | g_W8TextControlMask005ED580 | g_W8TextControlMask005ED57C |
        g_W8TextControlMask005ED578);
    state->dialogue_all_button =
        new W8TextControl(panel, 0x71, 6, 0x27, 0x41, 0x33, 0x1a9, 0, 8, 9, -1, -1, -1);
    state->dialogue_all_button->AddLayoutFlags(
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

/* Load each main-game cursor catalog entry into an stTextureAnim and record the
   ETRLE frame size onto the slot. Called once when the level block is first
   allocated. */
// FUNCTION: WIZ8 0x00568E10
void LoadMainGameCursorResources(void)
{
    HVOBJECT video_object;
    ETRLEObject properties;
    W8MainGameResourceSlot* slot = g_main_game_resource_slots;
    W8MainGameResourceSlot* end = g_main_game_resource_slots + 17;

    for (; slot < end; ++slot) {
        unsigned int handle = GetCatalogVideoObjectHandle(slot->image_id, 0);
        short frame = GetCatalogVideoObjectYOffset(slot->image_id);
        if (GetVideoObject(&video_object, handle)) {
            GetVideoObjectETRLEProperties(video_object, &properties,
                                          static_cast<unsigned short>(frame));
            slot->size_x = properties.usWidth;
            slot->size_y = properties.usHeight;
            stTextureAnim* animation =
                VideoVObjectToTextureAnim(video_object, static_cast<unsigned short>(frame),
                                          static_cast<unsigned short>(slot->frame_count), 1);
            slot->object = animation;
            animation->addReference();
            animation->flag_60 = 3;
        }
    }
}

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
        DisablePartyMovementRegions();
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
        if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME &&
            g_level_block->review_transition_active == 0) {
            if (gXStatus.fPartyMovementUi != 0) {
                UpdatePartyMovementPanel();
            }
            ClearSurfaceRect(0xb1, 0x13f, 0x1cf, 0x153);
            InvalidateRegion(0xb1, 0x13f, 0x1cf, 0x153, 0);
            if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
                g_level_block->redraw_flags |= 0x8000;
            }
        }
    }
}

/* The pause-key command: resume the world when it was paused this way,
   otherwise mark the pause and freeze the world. */
// FUNCTION: WIZ8 0x0056abe0
void ToggleMainGamePause(void)
{
    if (g_flag_006840bd != 0) {
        ResumeMainGameWorld();
        g_flag_006840bd = 0;
        return;
    }
    if (g_flag_006840bc == 0) {
        g_flag_006840bd = 1;
        PauseMainGameWorld();
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

/* Open the monster information dialog over the main game screen for the
   highlighted monster: only while the entry is live, not dying and still has
   hit points. Closing leaves the whole screen dirty via the destroy
   callback. */
// FUNCTION: WIZ8 0x0056ad60
void OpenMonsterInfoDialog0056AD60(int location_id)
{
    unsigned int monster_index =
        MonsterGetIndexByLocationID(0x1fa3, MAIN_GAME_SCREEN_CPP, location_id, 1);
    W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
    if (monster_info->fActive == 0 || monster_info->monster->IsDying() != 0 ||
        monster_info->hp_current == 0) {
        return;
    }
    W8MonsterInfoDialog* dialog = new W8MonsterInfoDialog(location_id);
    dialog->SetText(&g_wchar_00689b34);
    dialog->m_destroy_callback = InvalidateMainGameScreen005670A0;
    g_modal_owner_0068edd0 = dialog;
    ActivateDialogRegion(0x138);
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

/* Open the modal NPC sub-dialog for a script request: option list (0x05),
   price check (0x12/0x1e) or keyword entry (0x13). The text-input stack is
   suspended while the modal is up unless the dialogue stays live; for the
   price-check opcodes the base price in the request is discounted by the
   NPC's effect percentage and the party's best haggle skill. */
// FUNCTION: WIZ8 0x00575E60
void OpenNpcDialog(W8NpcDialogRequest* request, int aux_data)
{
    W8MonsterInfo* monster_info;
    W8NpcDialog* dialog;

    if (gXStatus.fNpcDialogueMode == 0 || g_screen_state_00649f1c->flag_252 != 0) {
        InitTextInputMode();
    } else {
        SetNpcDialoguePanelVisible(0);
    }
    dialog = new W8NpcDialog(request, aux_data);
    dialog->SetText(&g_wchar_00689b34);
    dialog->m_destroy_callback = OnNpcDialogClosed;
    OpenModal(dialog);
    g_screen_state_00649f1c->script_busy = 1;
    if (request->opcode == 0x12 || request->opcode == 0x1e) {
        g_screen_state_00649f1c->pending_fact_1fc = aux_data;
        g_screen_state_00649f1c->flag_200 = 1;
        g_screen_state_00649f1c->pending_price_204 = request->base_price;
        monster_info = GetNpcMonsterInfo(g_screen_state_00649f1c->dialogue_npc);
        if (monster_info != 0) {
            g_screen_state_00649f1c->pending_price_204 -= static_cast<int>(
                monster_info->effect_2de * 0.01f * g_screen_state_00649f1c->pending_price_204);
        }
        g_screen_state_00649f1c->pending_price_204 -=
            GetBestPartySkillLevel(0x16, 0) * g_screen_state_00649f1c->pending_price_204 / 500;
        if (g_screen_state_00649f1c->pending_price_204 < 1) {
            g_screen_state_00649f1c->pending_price_204 = 1;
        }
        if (g_screen_state_00649f1c->pending_price_204 > 0x1e) {
            g_screen_state_00649f1c->pending_price_204 =
                (g_screen_state_00649f1c->pending_price_204 * 10 + 9) / 10;
        }
        if (request->opcode == 0x1e) {
            g_screen_state_00649f1c->flag_201 = 1;
        }
    }
    SetTargetCursor(-1);
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

/* Show (0) or hide (nonzero) the NPC dialogue UI: on show the party portrait
   region sets lose input, the dialogue regions and text controls come up, and
   the transcript expands; on hide the party sets come back, the transcript
   collapses, its background is cleared and the rectangle invalidated. The new
   state lands in dialogue_cursor_flag. */
// FUNCTION: WIZ8 0x00576850
void SetNpcDialogueHidden(char value)
{
    int index;

    if (value != 0) {
        g_screen_state_00649f1c->saved_mode_230 = g_settings_6850c8.main_ui_mode;
        ApplyMainGameModeFlag(W8_MAIN_UI_MODE_PORTRAITS, 0);
        g_screen_state_00649f1c->npc_dialogue_controller_1b0->SetEnabled(0);
        CollapseNpcDialogueTextArea(g_screen_state_00649f1c->npc_dialogue_controller_1b0);
        g_screen_state_00649f1c->npc_dialogue_panel_1b4->SetEnabled(0);
        for (index = 0; index < 8; ++index) {
            if (g_status_685170.buffers.party_rows[index].occupied != 0) {
                RegionSetEnable(index + 7);
                EnableRegionSetInput(index + 7);
            }
        }
        RegionSetDisable(0x16);
        ClearNpcDialogueTextBackground(g_screen_state_00649f1c->npc_dialogue_controller_1b0);
        ClearSurfaceRect(0x1dc, 0x11b, 0x269, 0x1c2);
        InvalidateRegion(0x1dc, 0x11b, 0x269, 0x1c2, 0);
        RequestRedraw(2);
        RequestRedraw(8);
        RequestRedraw(0x20);
        RequestRedraw(0x80);
        SetInputFieldBlocksMouseCallback(0, 1);
        g_screen_state_00649f1c->dialogue_text_110->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_text_114->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_text_118->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_text_11c->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_text_120->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_text_124->SetEnabled(0);
    } else {
        ApplyMainGameModeFlag(g_screen_state_00649f1c->saved_mode_230, 0);
        for (index = 0; index < 8; ++index) {
            if (g_status_685170.buffers.party_rows[index].occupied != 0) {
                RegionSetDisable(index + 7);
                DisableRegionSetInput(index + 7);
            }
        }
        RegionSetEnable(0x16);
        RegionSetEnable(0x18);
        RegionSetEnable(0x15);
        EnableRegionInput(0x52);
        EnableRegionInput(0x53);
        EnableRegionInput(0x54);
        EnableRegionInput(0x55);
        g_level_block->action_panel_visible = 1;
        g_screen_state_00649f1c->npc_dialogue_controller_1b0->SetEnabled(1);
        g_screen_state_00649f1c->npc_dialogue_panel_1b4->SetEnabled(1);
        ExpandNpcDialogueTextArea(g_screen_state_00649f1c->npc_dialogue_controller_1b0);
        if (IsNpcDialogueTextExpanded(g_screen_state_00649f1c->npc_dialogue_controller_1b0) != 0) {
            g_screen_state_00649f1c->dialogue_scroll_up_button->SetEnabled(1);
            g_screen_state_00649f1c->dialogue_scroll_down_button->SetEnabled(1);
        } else {
            g_screen_state_00649f1c->dialogue_scroll_up_button->SetEnabled(0);
            g_screen_state_00649f1c->dialogue_scroll_down_button->SetEnabled(0);
        }
        SetInputFieldBlocksMouseCallback(0, 0);
        g_screen_state_00649f1c->dialogue_text_110->SetEnabled(1);
        g_screen_state_00649f1c->dialogue_text_114->SetEnabled(1);
        g_screen_state_00649f1c->dialogue_text_118->SetEnabled(1);
        g_screen_state_00649f1c->dialogue_text_11c->SetEnabled(1);
        g_screen_state_00649f1c->dialogue_text_120->SetEnabled(1);
        g_screen_state_00649f1c->dialogue_text_124->SetEnabled(1);
    }
    RequestRedraw(0x200);
    g_screen_state_00649f1c->dialogue_cursor_flag = value;
}

// FUNCTION: WIZ8 0x00576b80
void CloseNpcDialogueIfActive(void)
{
    if (gXStatus.fNpcDialogueMode != 0) {
        Function56E800(0);
    }
}

/* Destroy callback OpenNpcDialog installs on the modal: pops or re-schemes the
   text-input level the dialog pushed, restores the quote bubble, and routes the
   choice back to the script - the picked option's label, the price-check
   yes/no string, or the typed keyword text. Inside a live dialogue the text is
   injected into input field 0 and processed as if typed; otherwise it is
   submitted to the script line queue directly. */
// FUNCTION: WIZ8 0x00576E20
void OnNpcDialogClosed(W8DialogBase* dialog)
{
    W8NpcDialog* npc_dialog = static_cast<W8NpcDialog*>(dialog);
    W8NpcDialogRequest* request = npc_dialog->m_message;
    wchar_t field_text[200];
    wchar_t entry_text[1020];
    int index;

    if (gXStatus.fNpcDialogueMode == 0 || g_screen_state_00649f1c->flag_252 != 0) {
        KillTextInputMode();
    } else {
        SetTextInputScheme(1);
    }
    RestoreCurrentNpcQuoteBubble();
    if (g_screen_state_00649f1c->value_fc == W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX) {
        CloseNpcDialogueOptionLayout();
        OpenNpcDialogueTranscriptLayout();
    }
    if (request->opcode == 5) {
        for (index = 0; index < request->option_count; ++index) {
            if (index == npc_dialog->m_selected_option) {
                swprintf(entry_text, L"%S", request->options[index].text);
                if (gXStatus.fNpcDialogueMode == 0 || g_screen_state_00649f1c->flag_252 != 0) {
                    HandleNpcDialogueReply(entry_text, 0);
                } else {
                    Get16BitStringFromField(0, field_text);
                    StripNpcKeywordPunctuation(entry_text);
                    goto inject;
                }
                goto done;
            }
        }
        goto done;
    }
    if (request->opcode == 0x12 || request->opcode == 0x1e) {
        if (npc_dialog->m_selected_option == 0) {
            wcscpy(entry_text, gppStringList[0x1f7c / 4]);
        } else {
            wcscpy(entry_text, gppStringList[0x1f80 / 4]);
        }
        if (gXStatus.fNpcDialogueMode == 0 || g_screen_state_00649f1c->flag_252 != 0) {
            HandleNpcDialogueReply(entry_text, 0);
        } else {
            Get16BitStringFromField(0, field_text);
            StripNpcKeywordPunctuation(entry_text);
        inject:
            static_cast<void>(wcslen(field_text));
            SetInputFieldStringWith16BitString(0, entry_text);
            HandleNpcDialogueInput();
        }
    } else if (request->opcode == 0x13) {
        if (gXStatus.fNpcDialogueMode == 0 || g_screen_state_00649f1c->flag_252 != 0) {
            HandleNpcDialogueReply(npc_dialog->m_input_text, 0);
        } else {
            Get16BitStringFromField(0, field_text);
            StripNpcKeywordPunctuation(npc_dialog->m_input_text);
            static_cast<void>(wcslen(field_text));
            SetInputFieldStringWith16BitString(0, npc_dialog->m_input_text);
            HandleNpcDialogueInput();
        }
    }
done:
    g_screen_state_00649f1c->script_busy = 0;
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
void OpenCharacterScreenForPartySlot(unsigned int party_slot, int flag)
{
    if (gXStatus.fNpcDialogueMode != 0) {
        CloseNpcDialogueForCamp();
    }
    g_pending_screen_state.parameter_2 = party_slot;
    g_pending_screen_state.parameter_3 = g_status_685170.buffers.characters + party_slot;
    g_pending_screen_state.parameter_4 =
        flag != 0 ? static_cast<W8Character*>(g_pending_screen_state.parameter_3) : 0;
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
        CloseSpellCastingView();
    }
    if (gXStatus.fItemSelectMode != 0) {
        CloseUseItemSelectView();
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
short GetMainGameViewportMode(void)
{
    if (gXStatus.fSpellCastMode == 0 &&
        (gXStatus.fNpcDialogueMode == 0 || CanOpenNpcDialogue() != 0) &&
        gXStatus.fLockInteractMode == 0 && gXStatus.fTrapInteractMode == 0 &&
        gXStatus.fItemSelectMode == 0 && g_level_block->action_panel_visible == 0 &&
        g_level_block->formation_board_visible != 0 && g_level_block->radar_map_visible != 0 &&
        g_settings_6850c8.main_ui_mode == W8_MAIN_UI_MODE_PORTRAITS) {
        return 4;
    }
    if (gXStatus.fSpellCastMode == 0 &&
        (gXStatus.fNpcDialogueMode == 0 || CanOpenNpcDialogue() != 0) &&
        gXStatus.fLockInteractMode == 0 && gXStatus.fTrapInteractMode == 0 &&
        gXStatus.fItemSelectMode == 0 &&
        (g_level_block->formation_board_visible == 0 || g_level_block->radar_map_visible == 0 ||
         g_level_block->action_panel_visible == 0)) {
        return 0;
    }
    if (g_settings_6850c8.main_ui_mode == W8_MAIN_UI_MODE_FORMATION) {
        return 1;
    }
    if (g_settings_6850c8.main_ui_mode == W8_MAIN_UI_MODE_RADAR) {
        return 0;
    }
    return 2;
}

/* Drop whichever of the formation board, the radar map and the combat bar is
   up, restoring the viewport mode that was in effect when each was raised. */
// FUNCTION: WIZ8 0x00569570
void CloseMainGameOverlays(void)
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
                gXStatus.fItemSelectMode == 0 && g_level_block->action_panel_visible == 0 &&
                g_level_block->formation_board_visible != 0 &&
                g_level_block->radar_map_visible != 0 &&
                g_settings_6850c8.main_ui_mode == W8_MAIN_UI_MODE_PORTRAITS) {
                mode = 4;
            } else if (gXStatus.fSpellCastMode == 0 &&
                       (gXStatus.fNpcDialogueMode == 0 || CanOpenNpcDialogue() != 0) &&
                       gXStatus.fLockInteractMode == 0 && gXStatus.fTrapInteractMode == 0 &&
                       gXStatus.fItemSelectMode == 0 &&
                       (g_level_block->formation_board_visible == 0 ||
                        g_level_block->radar_map_visible == 0 ||
                        g_level_block->action_panel_visible == 0)) {
                mode = 0;
            } else if (g_settings_6850c8.main_ui_mode == W8_MAIN_UI_MODE_FORMATION) {
                mode = 1;
            } else if (g_settings_6850c8.main_ui_mode == W8_MAIN_UI_MODE_RADAR) {
                mode = 0;
            } else {
                mode = 2;
            }
            SetViewportMode(mode);
        }
        g_flag_0068edc9 = 0;
    }
    if (g_level_block->radar_map_visible != 0) {
        g_level_block->radar_map_visible = 0;
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
                gXStatus.fItemSelectMode == 0 && g_level_block->action_panel_visible == 0 &&
                g_level_block->formation_board_visible != 0 &&
                g_level_block->radar_map_visible != 0 &&
                g_settings_6850c8.main_ui_mode == W8_MAIN_UI_MODE_PORTRAITS) {
                mode = 4;
            } else if (gXStatus.fSpellCastMode == 0 &&
                       (gXStatus.fNpcDialogueMode == 0 || CanOpenNpcDialogue() != 0) &&
                       gXStatus.fLockInteractMode == 0 && gXStatus.fTrapInteractMode == 0 &&
                       gXStatus.fItemSelectMode == 0 &&
                       (g_level_block->formation_board_visible == 0 ||
                        g_level_block->radar_map_visible == 0 ||
                        g_level_block->action_panel_visible == 0)) {
                mode = 0;
            } else if (g_settings_6850c8.main_ui_mode == W8_MAIN_UI_MODE_FORMATION) {
                mode = 1;
            } else if (g_settings_6850c8.main_ui_mode == W8_MAIN_UI_MODE_RADAR) {
                mode = 0;
            } else {
                mode = 2;
            }
            SetViewportMode(mode);
        }
        g_flag_0068edbc = 0;
    }
    if (g_level_block->action_panel_visible != 0) {
        g_level_block->action_panel_visible = 0;
        DisableRegionInput(0x52);
        DisableRegionInput(0x53);
        DisableRegionInput(0x54);
        DisableRegionInput(0x55);
        DisableRegionInput(0x56);
        DisableRegionInput(0x57);
        DisableRegionInput(0x58);
        if (g_level_block->action_panel_visible == 0) {
            DisableRegionInput(0x59);
            RegionSetDisable(0x14);
        }
        if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
            g_level_block->redraw_flags |= 0x8200;
        }
        if (g_flag_0068edc8 != 0) {
            SetViewportMode(GetMainGameViewportMode());
        }
        g_flag_0068edc8 = 0;
    }
}

/* Whether the open NPC dialogue transcript covers the party slot's
   portrait: dialogue mode up, panel invalidation not suppressed and the
   controller enabled, then the slot's band check. Portrait and
   character-update paths skip the covered rows through this. */
// FUNCTION: WIZ8 0x0056EC90
unsigned char IsPortraitObscuredByNpcDialogue(unsigned int party_slot)
{
    if (gXStatus.fNpcDialogueMode != 0 && g_screen_state_00649f1c->flag_252 == 0 &&
        g_screen_state_00649f1c->npc_dialogue_controller_1b0->m_fEnabled != 0) {
        return g_screen_state_00649f1c->npc_dialogue_controller_1b0
            ->IsSlotPortraitTranscriptCovered(party_slot);
    }
    return 0;
}

/* Forward one text/cursor rectangle to the action panel while the dialogue
   screen is not suppressing panel invalidation. */
// FUNCTION: WIZ8 0x0056ECD0
void InvalidateMainGameActionPanelRect(const W8ControlsRect* rect)
{
    if (g_screen_state_00649f1c->flag_252 == 0) {
        g_screen_state_00649f1c->panel_1ac->Invalidate(rect);
    }
}

/* Retire the current dialogue layout mode into value_104 and, when nonzero,
   install `value` as the new one; either way the dialogue cursor helper gets
   re-run while the flag is set. */
// FUNCTION: WIZ8 0x0056EDD0
void SetNpcDialogueLayoutMode(int value)
{
    if (value == 0) {
        g_screen_state_00649f1c->value_104 = g_screen_state_00649f1c->value_fc;
        g_screen_state_00649f1c->value_fc = W8_DIALOGUE_LAYOUT_NONE;
    } else {
        g_screen_state_00649f1c->value_fc = value;
    }
    if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
        SetNpcDialogueHidden(0);
    }
}

/* Bring up the mode-2 dialogue layout: the option panels come up, the caption
   takes the NPC's name, and the five topics get their strings and callbacks. */
// FUNCTION: WIZ8 0x00570760
void ShowNpcDialogueTopicMenu(void)
{
    g_screen_state_00649f1c->value_fc = W8_DIALOGUE_LAYOUT_TOPIC_MENU;
    if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
        SetNpcDialogueHidden(0);
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
    g_screen_state_00649f1c->dialogue_text_11c->m_primaryActivationCallback =
        PromptNpcDispositionChange;
    g_screen_state_00649f1c->dialogue_text_120->m_textBuffer.SetText(gppStringList[0x1c90 / 4],
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_120->m_primaryActivationCallback = EnterNpcServiceLayout;
    g_screen_state_00649f1c->dialogue_text_124->m_textBuffer.SetText(gppStringList[0x1c94 / 4],
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_124->m_primaryActivationCallback =
        LeaveNpcDialogueLayout;
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
void CloseNpcDialogueTranscriptLayout(void)
{
    int index;

    g_screen_state_00649f1c->npc_dialogue_controller_1b0->SaveTranscriptEntries();
    CollapseNpcDialogueTextArea(g_screen_state_00649f1c->npc_dialogue_controller_1b0);
    g_screen_state_00649f1c->npc_dialogue_controller_1b0->ClearTranscriptEntries();
    RegionSetDisable(0x18);
    RegionSetDisable(0x16);
    g_screen_state_00649f1c->panel_1a8->SetEnabled(0);
    g_screen_state_00649f1c->panel_1ac->SetEnabled(0);
    g_screen_state_00649f1c->npc_dialogue_controller_1b0->SetEnabled(0);
    g_screen_state_00649f1c->npc_dialogue_panel_1b4->SetEnabled(0);
    g_screen_state_00649f1c->text_input_panel_1c0->SetEnabled(0);
    g_screen_state_00649f1c->value_104 = g_screen_state_00649f1c->value_fc;
    g_screen_state_00649f1c->value_fc = W8_DIALOGUE_LAYOUT_NONE;
    if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
        SetNpcDialogueHidden(0);
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
void CloseNpcDialogueOptionLayout(void)
{
    W8TextControl* text;

    ResetNpcDialogueItemEditor();
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
    g_screen_state_00649f1c->value_fc = W8_DIALOGUE_LAYOUT_NONE;
    if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
        SetNpcDialogueHidden(0);
    }
    g_screen_state_00649f1c->flag_229 = 0;
    Function58F6B0(3);
    Function58BA60();
}

/* Tear down the mode-5 dialogue layout and retire the current mode. */
// FUNCTION: WIZ8 0x00573570
void CloseNpcDialogueMode5Layout(void)
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
    g_screen_state_00649f1c->value_fc = W8_DIALOGUE_LAYOUT_NONE;
    if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
        SetNpcDialogueHidden(0);
    }
}

/* Tear down the mode-1 dialogue layout; outside camp the mode is retired as
   well. */
// FUNCTION: WIZ8 0x00573DD0
void CloseNpcDialogueMode1Layout(void)
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
        g_screen_state_00649f1c->value_fc = W8_DIALOGUE_LAYOUT_NONE;
        if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
            SetNpcDialogueHidden(0);
        }
    }
}

/* Route the player's reply text while a modal answer is pending. With no
   price offer outstanding the text resolves through the current quote's
   keyword tables and queues the matching line; during a 0x12/0x1e price check
   the affirmative string spends pending_price_204 (or runs line 0x14 when the
   party cannot pay), tells the offered fact unless opcode 0x1e suppressed it,
   and a refusal runs the pending fact's kind-0x17 decline entries. A nonzero
   echo posts the reply text back as a notice. */
// FUNCTION: WIZ8 0x00574250
void HandleNpcDialogueReply(wchar_t* text, char echo)
{
    wchar_t notice[200];
    int line;

    if (g_screen_state_00649f1c->script_busy != 0) {
        if (g_screen_state_00649f1c->flag_200 == 0) {
            line = FindNpcReplyQuote(text);
            if (line != -1) {
                QueueNpcScriptLine(line, 0, 0, 0);
            }
        } else {
            if (CompareWideTextIgnoreAsciiCase00402920(text, gppStringList[0x1f7c / 4]) == 0) {
                if (static_cast<unsigned int>(g_screen_state_00649f1c->pending_price_204) >
                    g_status_685170.party_gold) {
                    RunNpcScriptLine(0x14, 0);
                    g_screen_state_00649f1c->flag_200 = 0;
                } else {
                    SpendPartyGold(g_screen_state_00649f1c->pending_price_204);
                    if (g_screen_state_00649f1c->flag_201 == 0) {
                        TellNpcFact(g_screen_state_00649f1c->dialogue_npc,
                                    g_screen_state_00649f1c->pending_fact_1fc);
                    }
                    RunNpcScriptLine(g_screen_state_00649f1c->pending_fact_1fc, 0);
                    g_screen_state_00649f1c->flag_200 = 0;
                }
            } else {
                RunNpcQuoteDeclineActions(g_screen_state_00649f1c->pending_fact_1fc);
                g_screen_state_00649f1c->flag_200 = 0;
            }
        }
        if (echo != 0) {
            swprintf(notice, L"%s...", text);
            ShowNotice(0xa, notice, 3, GetTextBoxScrollRange(), 0);
        }
        g_screen_state_00649f1c->script_busy = 0;
    }
}

extern int g_dialogue_fallback_ids_00649f64[5]; /* 0x00649F64 */
extern int g_dialogue_fallback_ids_00649f78[5]; /* 0x00649F78 */

/* The NPC dialogue's typed-input processor. Strips punctuation, tokenizes into
   words, and resolves the text to one or two quote ids: the "join"-style
   keywords go straight to RequestNpcJoinParty, name/place prefixes are stripped
   and resolved through FindNpcNameOrPlaceQuote, and otherwise every word pair
   then every single word is tried through FindNpcScriptQuoteByKeyword. Unresolved input
   shows a fallback notice built from the random reply tables; resolved input
   queues the script line(s). */
// FUNCTION: WIZ8 0x005743B0
void HandleNpcDialogueInput(void)
{
    wchar_t field_text[200];
    wchar_t word[200];
    wchar_t buf[200];
    wchar_t word2[200];
    wchar_t notice[200];
    wchar_t* cursor;
    wchar_t* out;
    wchar_t ch;
    int quote_id = -1;
    int second_quote_id = -1;
    bool show_fallback = true;
    bool plain_text = true;
    int quote;
    int len;
    int word_count;
    int matches;

    Get16BitStringFromField(0, field_text);
    ClearActiveField();
    StripNpcKeywordPunctuation(field_text);
    if (wcslen(field_text) == 0 && g_screen_state_00649f1c->script_busy == 0) {
        return;
    }
    if (g_screen_state_00649f1c->script_busy != 0) {
        HandleNpcDialogueReply(field_text, 1);
        return;
    }
    word_count = 0;
    cursor = field_text;
    if (cursor != 0) {
        for (;;) {
            len = 0;
            word[0] = 0;
            if (*cursor == L' ') {
                ch = L' ';
                do {
                    if (ch == 0)
                        break;
                    ch = *++cursor;
                } while (ch == L' ');
            }
            ch = *cursor;
            if (ch == L' ')
                break;
            out = word;
            do {
                if (ch == 0)
                    break;
                *out++ = ch;
                ch = *++cursor;
                ++len;
            } while (ch != L' ');
            if (len == 0)
                break;
            word[len] = 0;
            if (cursor == 0)
                break;
            ++word_count;
        }
    }
    if (word_count > 2) {
        show_fallback = false;
    }
    if (_wcsnicmp(field_text, gppStringList[0x1dac / 4], 4) == 0 ||
        _wcsnicmp(field_text, gppStringList[0x1db0 / 4], 7) == 0) {
        RequestNpcJoinParty();
        return;
    }
    if (g_screen_state_00649f1c->flag_1d9 == 1) {
        const wchar_t* fmt;
        if (_wcsnicmp(field_text, gppStringList[0x1db4 / 4], 9) == 0 ||
            _wcsnicmp(field_text, gppStringList[0x1db8 / 4], 0xa) == 0 ||
            _wcsnicmp(field_text, gppStringList[0x1dbc / 4], 8) == 0) {
            fmt = g_format_s_006068e4;
        } else {
            fmt = gppStringList[0x1da8 / 4];
        }
        swprintf(buf, fmt, field_text);
        quote = FindNpcScriptQuoteByKeyword(buf, 0, 0);
        if (quote != -1) {
            plain_text = false;
            quote_id = quote;
            goto found;
        }
    } else {
        swprintf(buf, g_format_s_006068e4, field_text);
        quote = FindNpcScriptQuoteByKeyword(buf, 0, 0);
        if (quote != -1) {
            quote_id = quote;
            goto found;
        }
    }
    wcscpy(buf, field_text);
    if (_wcsnicmp(buf, gppStringList[0x1dc0 / 4], 0xb) != 0) {
        if (_wcsnicmp(buf, gppStringList[0x1db4 / 4], 9) == 0 ||
            _wcsnicmp(buf, gppStringList[0x1db8 / 4], 0xa) == 0 ||
            _wcsnicmp(buf, gppStringList[0x1dbc / 4], 8) == 0) {
            wcscpy(field_text, buf + 9);
            plain_text = false;
            show_fallback = false;
        } else if (g_screen_state_00649f1c->flag_1d9 == 0) {
            plain_text = true;
            goto pair_scan;
        } else {
            plain_text = false;
        }
        quote_id = FindNpcNameOrPlaceQuote(g_screen_state_00649f1c->dialogue_npc, field_text);
        if (quote_id == -1) {
            quote_id = 0x76;
            goto fallback;
        }
        goto found;
    }
    wcscpy(field_text, buf + 0xb);
    show_fallback = false;
pair_scan:
    cursor = field_text;
    word[0] = 0;
    if (cursor != 0) {
        do {
            if (wcslen(word) == 0) {
                len = 0;
                word[0] = 0;
                if (*cursor == L' ') {
                    ch = L' ';
                    do {
                        if (ch == 0)
                            break;
                        ch = *++cursor;
                    } while (ch == L' ');
                }
                ch = *cursor;
                if (ch == L' ')
                    goto multi_scan;
                out = word;
                do {
                    if (ch == 0)
                        break;
                    *out++ = ch;
                    ch = *++cursor;
                    ++len;
                } while (ch != L' ');
                if (len == 0)
                    goto multi_scan;
                word[len] = 0;
            } else {
                wcscpy(word, word2);
            }
            if (cursor == 0)
                goto multi_scan;
            len = 0;
            word2[0] = 0;
            if (*cursor == L' ') {
                ch = L' ';
                do {
                    if (ch == 0)
                        break;
                    ch = *++cursor;
                } while (ch == L' ');
            }
            ch = *cursor;
            if (ch == L' ')
                goto multi_scan;
            out = word2;
            do {
                if (ch == 0)
                    break;
                *out++ = ch;
                ch = *++cursor;
                ++len;
            } while (ch != L' ');
            if (len == 0)
                goto multi_scan;
            word2[len] = 0;
            if (cursor == 0)
                goto multi_scan;
            swprintf(buf, g_format_s_space_s_00617584, word, word2);
            quote = FindNpcScriptQuoteByKeyword(buf, 0, 0);
        } while (quote == -1);
        quote_id = quote;
        goto found;
    }
multi_scan:
    matches = 0;
    cursor = field_text;
    if (cursor == 0)
        goto fallback;
    for (;;) {
        len = 0;
        word[0] = 0;
        if (*cursor == L' ') {
            ch = L' ';
            do {
                if (ch == 0)
                    break;
                ch = *++cursor;
            } while (ch == L' ');
        }
        ch = *cursor;
        if (ch == L' ')
            break;
        out = word;
        do {
            if (ch == 0)
                break;
            *out++ = ch;
            ch = *++cursor;
            ++len;
        } while (ch != L' ');
        if (len == 0)
            break;
        word[len] = 0;
        if (cursor == 0)
            break;
        quote = FindNpcScriptQuoteByKeyword(word, 0, 0);
        if (quote != -1) {
            ++matches;
        }
    }
    if (matches > 2) {
        quote_id = 0x20;
        goto echo;
    }
    if (matches <= 0) {
        goto fallback;
    }
    {
        int found_count = 0;
        cursor = field_text;
        do {
            do {
                len = 0;
                word[0] = 0;
                if (*cursor == L' ') {
                    ch = L' ';
                    do {
                        if (ch == 0)
                            break;
                        ch = *++cursor;
                    } while (ch == L' ');
                }
                ch = *cursor;
                if (ch == L' ')
                    goto found;
                out = word;
                do {
                    if (ch == 0)
                        break;
                    *out++ = ch;
                    ch = *++cursor;
                    ++len;
                } while (ch != L' ');
                if (len == 0)
                    goto found;
                word[len] = 0;
                if (cursor == 0)
                    goto found;
                quote = FindNpcScriptQuoteByKeyword(word, 0, 0);
            } while (quote == -1);
            ++found_count;
            if (found_count == 1) {
                quote_id = quote;
            } else if (found_count == 2 && quote != quote_id) {
                second_quote_id = quote;
            }
        } while (matches != 1);
    }
found:
    if (quote_id >= 0x59 && quote_id < 0x69 && second_quote_id == -1) {
        goto echo;
    }
fallback:
    if (show_fallback) {
        unsigned int roll;
        const wchar_t* fmt;
        const wchar_t* text;
        if (plain_text) {
            roll = Random(5);
            if (roll == 2 || roll == 3 || roll == 4) {
                fmt = L"%s %s?";
            } else {
                fmt = L"%s %s.";
            }
            text = gppStringList[g_dialogue_fallback_ids_00649f64[roll]];
        } else {
            roll = Random(5);
            if (roll == 3 || roll == 4) {
                fmt = L"%s %s?";
            } else {
                fmt = L"%s %s.";
            }
            text = gppStringList[g_dialogue_fallback_ids_00649f78[roll]];
        }
        swprintf(notice, fmt, text, field_text);
        ShowNotice(0xa, notice, 3, GetTextBoxScrollRange(), 0);
        goto dispatch;
    }
echo:
    ShowNotice(0xa, field_text, 3, GetTextBoxScrollRange(), 0);
dispatch:
    if (quote_id == -1) {
        QueueNpcScriptLine(Random(2) + 0x23, 0, 0, 0);
        return;
    }
    if (quote_id == 0x59 || second_quote_id == 0x59) {
        QueueNpcScriptLine(0x59, 1, 0, 0);
        return;
    }
    QueueNpcScriptLine(quote_id, 1, 0, 0);
    if (second_quote_id == -1) {
        return;
    }
    QueueNpcScriptLine(0x21, 0, 0, 0);
    QueueNpcScriptLine(second_quote_id, 1, 0, 0);
}

// FUNCTION: WIZ8 0x00575810
unsigned char HandleNpcDialogueItem(W8ItemInstance* item)
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
            fact_result = FindNpcScriptItemQuote(item->item_id, 0, &flag);
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
                    QueueNpcScriptLine(0x10, 0, 0, 0);
                    goto remove;
                }
                QueueNpcScriptLine(0x11, 0, 0, 0);
            } else {
                QueueNpcScriptLine(fact_result, 0, 0, 0);
                result = 0;
                if (flag != 0) {
                remove:
                    RemoveNpcScriptItem(item, 0, -1);
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
                QueueNpcScriptLine(7, 0, 0, 0);
                return 1;
            }
            fact_result = FindNpcScriptItemQuote(item->item_id, 0, &flag);
            if (fact_result == -1) {
                Function50A570(g_screen_state_00649f1c->dialogue_npc, 3,
                               g_screen_state_00649f1c->dialogue_speaker, item, 0);
                QueueNpcScriptLine(
                    GetNpcDispositionBand(g_screen_state_00649f1c->dialogue_npc) == 0 ? 0x10 : 7, 0,
                    0, 0);
            } else {
                QueueNpcScriptLine(fact_result, 0, 0, 0);
                result = 0;
            }
            RemoveNpcScriptItem(item, 0, -1);
            return result;
        }
    unavailable:
        QueueNpcScriptLine(0x11, 0, 0, 0);
        return 1;
    }
    return result;
}

/* Retire the current dialogue layout, then open the layout `interact_id`
   selects. */
// FUNCTION: WIZ8 0x00570120
void SwitchNpcDialogueLayout(int interact_id)
{
    switch (g_screen_state_00649f1c->value_fc) {
    case 1:
        CloseNpcDialogueMode1Layout();
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
        g_screen_state_00649f1c->value_fc = W8_DIALOGUE_LAYOUT_NONE;
        if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
            SetNpcDialogueHidden(0);
        }
        break;
    case 3:
        CloseNpcDialogueTranscriptLayout();
        break;
    case 4:
        CloseNpcDialogueOptionLayout();
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
        g_screen_state_00649f1c->value_fc = W8_DIALOGUE_LAYOUT_NONE;
        if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
            SetNpcDialogueHidden(0);
        }
        break;
    case 6:
        g_screen_state_00649f1c->value_104 = g_screen_state_00649f1c->value_fc;
        g_screen_state_00649f1c->value_fc = W8_DIALOGUE_LAYOUT_NONE;
        if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
            SetNpcDialogueHidden(0);
        }
        break;
    }
    switch (interact_id) {
    case 1:
        OpenNpcDialogueMode1Layout();
        return;
    case 2:
        ShowNpcDialogueTopicMenu();
        return;
    case 3:
        OpenNpcDialogueTranscriptLayout();
        return;
    case 4:
        OpenNpcDialogueOptionLayout();
        return;
    case 5:
        OpenNpcDialogueMode5Layout();
        return;
    }
}

/* The camp-side mirror of SwitchNpcDialogueLayout: camp mode is raised, the current
   dialogue layout is retired, and a still-pending item goes back onto the
   item cursor. */
// FUNCTION: WIZ8 0x00577020
void CloseNpcDialogueForCamp(void)
{
    gXStatus.fCampMode = 1;
    switch (g_screen_state_00649f1c->value_fc) {
    case 1:
        CloseNpcDialogueMode1Layout();
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
        g_screen_state_00649f1c->value_fc = W8_DIALOGUE_LAYOUT_NONE;
        if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
            SetNpcDialogueHidden(0);
        }
        break;
    case 3:
        CloseNpcDialogueTranscriptLayout();
        break;
    case 4:
        CloseNpcDialogueOptionLayout();
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
        g_screen_state_00649f1c->value_fc = W8_DIALOGUE_LAYOUT_NONE;
        if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
            SetNpcDialogueHidden(0);
        }
        break;
    case 6:
        g_screen_state_00649f1c->value_104 = g_screen_state_00649f1c->value_fc;
        g_screen_state_00649f1c->value_fc = W8_DIALOGUE_LAYOUT_NONE;
        if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
            SetNpcDialogueHidden(0);
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
void HandleNpcDialogueDeparture(int value)
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
        if (g_screen_state_00649f1c->dialogue_npc->greeting_pending == 0) {
            QueueNpcScriptLine(1, 0, 0, 0);
        } else {
            QueueNpcScriptLine(0, 0, 0, 0);
            g_screen_state_00649f1c->dialogue_npc->greeting_pending = 0;
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
                AddNpcDialogueKeyword(
                    g_screen_state_00649f1c->dialogue_npc->record->source_name_004, -1, 1);
                return;
            }
            if (g_screen_state_00649f1c->dialogue_npc->record->unknown_054 == 0 &&
                (g_screen_state_00649f1c->dialogue_npc->record->flag_2ea == 0 ||
                 g_screen_state_00649f1c->dialogue_npc->is_present != 0)) {
                AddDialogueTranscriptKeyword(
                    g_screen_state_00649f1c->dialogue_npc->record->source_name_004, -1);
            }
        }
    }
}
// FUNCTION: WIZ8 0x00563DD0
void ClearHighlightOverlayRegion(void)
{
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
}

// FUNCTION: WIZ8 0x00577520
void BeginScriptedWorldAction(void)
{
    g_status_685170.value_2435 = 1;
    ResetLevelDataVectors0041F0D0();
    SetTargetCursor(W8_CURSOR_MAP_LOAD);
}

// GLOBAL: WIZ8 0x00649f20
int g_dialogue_place_keyword_count = 15;
// GLOBAL: WIZ8 0x00649f24
int g_dialogue_place_keyword_ids[15] = {0x751, 0x752, 0x753, 0x754, 0x755, 0x756, 0x757, 0x758,
                                        0x759, 0x75a, 0x75b, 0x75c, 0x75d, 0x75e, 0x75f};
// GLOBAL: WIZ8 0x00649F64
int g_dialogue_fallback_ids_00649f64[5] = {0x760, 0x761, 0x762, 0x763, 0x764};
// GLOBAL: WIZ8 0x00649F78
int g_dialogue_fallback_ids_00649f78[5] = {0x765, 0x766, 0x767, 0x768, 0x769};
// GLOBAL: WIZ8 0x00649f8c
const wchar_t* g_dialogue_person_keywords[] = {L"BALBRAK", L"BILDUBLU", L"EWAXX",  L"KUNAR",
                                               L"PANRACK", L"RODAN",    L"RUBBLE", L"SAXX",
                                               L"SPARKLE", L"YAMIR",    L""};

// FUNCTION: WIZ8 0x00575020
bool IsDialoguePlaceKeyword(const wchar_t* name)
{
    for (int index = 0; index < g_dialogue_place_keyword_count; ++index) {
        if (CompareWideTextIgnoreAsciiCase00402920(
                name, gppStringList[g_dialogue_place_keyword_ids[index]]) == 0) {
            return 1;
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x005775d0
void AddDialogueTranscriptKeyword(const wchar_t* name, signed char category)
{
    wchar_t keyword[100];
    wcscpy(keyword, name);
    StripNpcKeywordPunctuation(keyword);
    if (category == -1) {
        unsigned int index;
        for (index = 0; index < gXStatus.uiItemsInDatabase; ++index) {
            if (CompareWideTextIgnoreAsciiCase00402920(keyword,
                                                       g_item_records[index].display_name) == 0) {
                category = W8_DIALOGUE_CATEGORY_ITEMS;
                break;
            }
        }
        if (category == -1) {
            for (index = 0; index < gXStatus.uiNpcsInDatabase; ++index) {
                if (CompareWideTextIgnoreAsciiCase00402920(
                        keyword, g_npc_records[index].source_name_004) == 0) {
                    category = W8_DIALOGUE_CATEGORY_PEOPLE;
                    break;
                }
            }
        }
        if (category == -1) {
            for (index = 0; g_dialogue_person_keywords[index][0] != 0; ++index) {
                if (CompareWideTextIgnoreAsciiCase00402920(
                        keyword, g_dialogue_person_keywords[index]) == 0) {
                    category = W8_DIALOGUE_CATEGORY_PEOPLE;
                    break;
                }
            }
        }
        if (category == -1) {
            category = IsDialoguePlaceKeyword(keyword) ? W8_DIALOGUE_CATEGORY_PLACES
                                                       : W8_DIALOGUE_CATEGORY_MISC;
        }
    }
    for (int index = 0; index < g_screen_state_00649f1c->dialogue_transcript.GetCount(); ++index) {
        if (CompareWideTextIgnoreAsciiCase00402920(
                (*g_screen_state_00649f1c->dialogue_transcript.GetAt(index))->text, keyword) == 0) {
            return;
        }
    }
    W8DialogueTranscriptRecord* record =
        static_cast<W8DialogueTranscriptRecord*>(malloc(sizeof(W8DialogueTranscriptRecord)));
    memset(record, 0, sizeof(*record));
    wcscpy(record->text, keyword);
    record->category = category;
    g_screen_state_00649f1c->dialogue_transcript.Add(record);
}

// FUNCTION: WIZ8 0x005777c0
void RecordLevelEntryDialogueState(void)
{
    wchar_t region_name[100];
    int region = GetLevelBand(g_status_685170.current_level);
    if (GetNpcScriptRegionName(region, region_name)) {
        AddDialogueTranscriptKeyword(region_name, W8_DIALOGUE_CATEGORY_PLACES);
    }
    if (region == 14) {
        SetFact(0x25b, 0, 0);
        if (!NpcLeadHasNameStyle(0x18)) {
            SetFact(0x216, 1, 0);
        }
    }
}

// FUNCTION: WIZ8 0x00575070
void ClearNpcDialogueTranscript(void)
{
    for (int index = 0; index < g_screen_state_00649f1c->dialogue_transcript.count; ++index) {
        free(*g_screen_state_00649f1c->dialogue_transcript.GetAt(index));
    }
    g_screen_state_00649f1c->dialogue_transcript.count = 0;
}

/* Restate the five transcript category buttons so only the active
   dialogue_category_filter's button shows its secondary state. */
// FUNCTION: WIZ8 0x00575390
void SyncDialogueCategoryButtons(void)
{
    switch (g_screen_state_00649f1c->dialogue_category_filter) {
    case W8_DIALOGUE_CATEGORY_ITEMS:
        g_screen_state_00649f1c->dialogue_people_button->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_places_button->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_items_button->EnableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_misc_button->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_all_button->DisableSecondaryState(1);
        break;
    case W8_DIALOGUE_CATEGORY_PEOPLE:
        g_screen_state_00649f1c->dialogue_people_button->EnableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_places_button->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_items_button->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_misc_button->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_all_button->DisableSecondaryState(1);
        break;
    case W8_DIALOGUE_CATEGORY_PLACES:
        g_screen_state_00649f1c->dialogue_people_button->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_places_button->EnableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_items_button->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_misc_button->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_all_button->DisableSecondaryState(1);
        break;
    case W8_DIALOGUE_CATEGORY_MISC:
        g_screen_state_00649f1c->dialogue_people_button->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_places_button->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_items_button->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_misc_button->EnableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_all_button->DisableSecondaryState(1);
        break;
    case W8_DIALOGUE_CATEGORY_ALL:
        g_screen_state_00649f1c->dialogue_people_button->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_places_button->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_items_button->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_misc_button->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_all_button->EnableSecondaryState(1);
        break;
    default:
        break;
    }
}

/* Open the mode-3 NPC dialogue layout: the caption resolves through the
   alternate-name style, the full text-control grid is loaded with its labels
   and callbacks, the transcript controller replays and re-expands, the scroll
   widgets follow the expansion state and the party portrait regions are
   parked for the duration. */
// FUNCTION: WIZ8 0x00570CF0
void OpenNpcDialogueTranscriptLayout(void)
{
    int index;

    g_screen_state_00649f1c->value_fc = W8_DIALOGUE_LAYOUT_TRANSCRIPT;
    if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
        SetNpcDialogueHidden(0);
    }
    g_screen_state_00649f1c->panel_1a8->SetEnabled(1);
    g_screen_state_00649f1c->panel_1ac->SetEnabled(1);
    g_screen_state_00649f1c->npc_dialogue_controller_1b0->SetEnabled(1);
    g_screen_state_00649f1c->npc_dialogue_panel_1b4->SetEnabled(1);
    g_screen_state_00649f1c->text_input_panel_1c0->SetEnabled(1);
    g_screen_state_00649f1c->dialogue_text_1a4->SetActive(0);
    g_screen_state_00649f1c->dialogue_text_1a4->m_textBuffer.SetText(&g_wchar_00689b34,
                                                                     g_wiz_text_bold_font_683664);
    RegionSetEnable(0x18);
    RegionSetEnable(0x16);
    if (g_screen_state_00649f1c->dialogue_npc->name_style == 0x32) {
        swprintf(g_status_685170.monster_name_buffer_2453, L"Al-%s",
                 g_status_685170.buffers.characters[g_status_685170.alternate_name_slot_247f].name);
        g_screen_state_00649f1c->dialogue_text_10c->m_textBuffer.SetText(
            g_status_685170.monster_name_buffer_2453, g_wiz_text_bold_font_683664);
    } else {
        g_screen_state_00649f1c->dialogue_text_10c->m_textBuffer.SetText(
            g_screen_state_00649f1c->dialogue_npc->record->source_name_004,
            g_wiz_text_bold_font_683664);
    }
    g_screen_state_00649f1c->dialogue_text_110->m_textBuffer.SetText(gppStringList[0x1cd4 / 4],
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_110->m_primaryActivationCallback = EnterNpcTradeOptions;
    g_screen_state_00649f1c->dialogue_text_114->m_textBuffer.SetText(gppStringList[0x1ce0 / 4],
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_114->m_primaryActivationCallback = ShowNpcDialogueNotice;
    g_screen_state_00649f1c->dialogue_text_118->m_textBuffer.SetText(gppStringList[0x1c90 / 4],
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_118->m_primaryActivationCallback = EnterNpcServiceLayout;
    g_screen_state_00649f1c->dialogue_text_11c->m_textBuffer.SetText(gppStringList[0x1cdc / 4],
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_11c->m_primaryActivationCallback = RequestNpcJoinParty;
    g_screen_state_00649f1c->dialogue_text_120->m_textBuffer.SetText(gppStringList[0x1c8c / 4],
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_120->m_primaryActivationCallback =
        PromptNpcDispositionChange;
    g_screen_state_00649f1c->dialogue_text_124->m_textBuffer.SetText(gppStringList[0x1c94 / 4],
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_124->m_primaryActivationCallback =
        LeaveNpcDialogueLayout;
    g_screen_state_00649f1c->dialogue_scroll_130->m_primaryActivationCallback = NoOp;
    g_screen_state_00649f1c->dialogue_scroll_up_button->m_leftButtonDownCallback =
        ScrollNpcDialogueUp;
    g_screen_state_00649f1c->dialogue_scroll_down_button->m_leftButtonDownCallback =
        ScrollNpcDialogueDown;
    g_screen_state_00649f1c->dialogue_text_13c->m_primaryActivationCallback =
        SubmitNpcDialogueKeyword;
    g_screen_state_00649f1c->dialogue_text_140->m_primaryActivationCallback =
        RefreshNpcDialogueTranscript;
    g_screen_state_00649f1c->dialogue_sort_button->m_textBuffer.SetText(gppStringList[0x1d00 / 4],
                                                                        g_font_683660);
    g_screen_state_00649f1c->dialogue_sort_button->m_primaryActivationCallback =
        SyncNpcDialogueListFilter;
    g_screen_state_00649f1c->dialogue_people_button->m_textBuffer.SetText(gppStringList[0x1d0c / 4],
                                                                          g_font_683660);
    g_screen_state_00649f1c->dialogue_people_button->m_primaryActivationCallback =
        SelectNpcDialogueCategory1;
    g_screen_state_00649f1c->dialogue_places_button->m_textBuffer.SetText(gppStringList[0x1d10 / 4],
                                                                          g_font_683660);
    g_screen_state_00649f1c->dialogue_places_button->m_primaryActivationCallback =
        SelectNpcDialogueCategory2;
    g_screen_state_00649f1c->dialogue_items_button->m_textBuffer.SetText(gppStringList[0x1d14 / 4],
                                                                         g_font_683660);
    g_screen_state_00649f1c->dialogue_items_button->m_primaryActivationCallback =
        SelectNpcDialogueCategory0;
    g_screen_state_00649f1c->dialogue_misc_button->m_textBuffer.SetText(gppStringList[0x1d18 / 4],
                                                                        g_font_683660);
    g_screen_state_00649f1c->dialogue_misc_button->m_primaryActivationCallback =
        SelectNpcDialogueCategory3;
    g_screen_state_00649f1c->dialogue_all_button->m_textBuffer.SetText(gppStringList[0x1d1c / 4],
                                                                       g_font_683660);
    g_screen_state_00649f1c->dialogue_all_button->m_primaryActivationCallback =
        SelectNpcDialogueCategoryAll;
    g_screen_state_00649f1c->dialogue_text_128->SetActive(0);
    g_screen_state_00649f1c->dialogue_text_110->SetEnabled(1);
    g_screen_state_00649f1c->dialogue_text_114->SetEnabled(1);
    g_screen_state_00649f1c->npc_dialogue_controller_1b0->RestoreTranscriptEntries();
    g_screen_state_00649f1c->npc_dialogue_controller_1b0->SetTranscriptCategoryFilter(
        g_screen_state_00649f1c->dialogue_category_filter);
    g_screen_state_00649f1c->npc_dialogue_controller_1b0->SetTranscriptSorted(
        g_screen_state_00649f1c->transcript_sorted);
    ExpandNpcDialogueTextArea(g_screen_state_00649f1c->npc_dialogue_controller_1b0);
    if (IsNpcDialogueTextExpanded(g_screen_state_00649f1c->npc_dialogue_controller_1b0)) {
        g_screen_state_00649f1c->dialogue_scroll_up_button->SetEnabled(1);
        g_screen_state_00649f1c->dialogue_scroll_down_button->SetEnabled(1);
    } else {
        g_screen_state_00649f1c->dialogue_scroll_up_button->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_scroll_down_button->SetEnabled(0);
    }
    SyncDialogueCategoryButtons();
    if (g_screen_state_00649f1c->transcript_sorted != 0) {
        g_screen_state_00649f1c->dialogue_sort_button->EnableSecondaryState(1);
    } else {
        g_screen_state_00649f1c->dialogue_sort_button->DisableSecondaryState(1);
    }
    RequestRedraw(0x200);
    for (index = 0; index < 8; ++index) {
        if (g_status_685170.buffers.party_rows[index].occupied != 0) {
            RegionSetDisable(7 + index);
            DisableRegionSetInput(7 + index);
            DisableRegionInput(0x5a + index);
        }
    }
    if (g_screen_state_00649f1c->dialogue_npc->record->unknown_056 != 0) {
        g_screen_state_00649f1c->dialogue_text_114->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_text_118->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_text_11c->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_text_120->SetEnabled(0);
    }
    if (g_screen_state_00649f1c->dialogue_npc->name_style == 0x17) {
        g_screen_state_00649f1c->dialogue_text_114->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_text_118->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_text_11c->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_text_120->SetEnabled(0);
    }
    g_screen_state_00649f1c->value_25c++;
    Function58F6B0(3);
}

/* Open the mode-4 trade-option layout: the six option controls get their
   labels, callbacks and secondary state, the party gold is formatted into the
   purse readout and the option-button row is cleared before the refresh. */
// FUNCTION: WIZ8 0x00571AA0
void OpenNpcDialogueOptionLayout(void)
{
    wchar_t buffer[0x20];
    int index;

    g_screen_state_00649f1c->value_fc = 4;
    if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
        SetNpcDialogueHidden(0);
    }
    g_screen_state_00649f1c->dialogue_text_1a4->SetActive(0);
    g_screen_state_00649f1c->dialogue_text_1a4->m_textBuffer.SetText(&g_wchar_00689b34,
                                                                     g_wiz_text_bold_font_683664);
    g_screen_state_00649f1c->panel_1a8->SetEnabled(1);
    g_screen_state_00649f1c->panel_1ac->SetEnabled(1);
    g_screen_state_00649f1c->panel_1bc->SetEnabled(1);
    RegionSetEnable(0x18);
    RegionSetEnable(0x17);
    g_screen_state_00649f1c->dialogue_text_194->m_textBuffer.SetText(gppStringList[0x1cb4 / 4],
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_110->m_textBuffer.SetText(gppStringList[0x1cf4 / 4],
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_110->m_primaryActivationCallback =
        SetNpcDialogueSubMode4;
    g_screen_state_00649f1c->dialogue_text_114->m_textBuffer.SetText(gppStringList[0x1cf8 / 4],
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_114->m_primaryActivationCallback =
        SetNpcDialogueSubMode3;
    g_screen_state_00649f1c->dialogue_text_118->m_textBuffer.SetText(gppStringList[0x1cfc / 4],
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_118->m_primaryActivationCallback =
        SetNpcDialogueSubMode2;
    g_screen_state_00649f1c->dialogue_text_11c->m_textBuffer.SetText(gppStringList[0x1cb8 / 4],
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_11c->m_primaryActivationCallback =
        SetNpcDialogueSubMode5;
    g_screen_state_00649f1c->dialogue_text_120->m_textBuffer.SetText(gppStringList[0x1cac / 4],
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_120->m_primaryActivationCallback = SelectNpcTradeMode1;
    g_screen_state_00649f1c->dialogue_text_124->m_textBuffer.SetText(gppStringList[0x1cb0 / 4],
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_124->m_primaryActivationCallback = SelectNpcTradeMode0;
    g_screen_state_00649f1c->dialogue_text_16c->m_primaryActivationCallback = OpenNpcItemAssay;
    g_screen_state_00649f1c->dialogue_text_16c->m_secondaryActivationCallback = OpenNpcItemAssay;
    g_screen_state_00649f1c->dialogue_text_188->SetEnabled(0);
    g_screen_state_00649f1c->dialogue_text_188->m_primaryActivationCallback = ConfirmNpcTradeSlot;
    swprintf(buffer, L"%dg", g_status_685170.party_gold);
    g_screen_state_00649f1c->dialogue_text_190->m_textBuffer.SetText(buffer, g_font_683660);
    g_screen_state_00649f1c->dialogue_text_1a0->SetEnabled(0);
    g_screen_state_00649f1c->dialogue_text_1a0->m_primaryActivationCallback = Function5AD290;
    g_screen_state_00649f1c->dialogue_text_120->AddLayoutFlags(g_W8TextControlMask005ED578);
    g_screen_state_00649f1c->dialogue_text_124->AddLayoutFlags(g_W8TextControlMask005ED578);
    g_screen_state_00649f1c->dialogue_text_120->DisableSecondaryState(1);
    g_screen_state_00649f1c->dialogue_text_124->DisableSecondaryState(1);
    g_screen_state_00649f1c->dialogue_text_110->AddLayoutFlags(g_W8TextControlMask005ED578);
    g_screen_state_00649f1c->dialogue_text_114->AddLayoutFlags(g_W8TextControlMask005ED578);
    g_screen_state_00649f1c->dialogue_text_118->AddLayoutFlags(g_W8TextControlMask005ED578);
    g_screen_state_00649f1c->dialogue_text_11c->AddLayoutFlags(g_W8TextControlMask005ED578);
    UpdateNpcDialogueSubMode();
    g_screen_state_00649f1c->flag_234 = 1;
    if (g_screen_state_00649f1c->flag_229 != 0) {
        g_screen_state_00649f1c->dialogue_text_110->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_text_114->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_text_11c->SetEnabled(0);
    } else {
        g_screen_state_00649f1c->dialogue_text_110->SetEnabled(1);
        g_screen_state_00649f1c->dialogue_text_114->SetEnabled(1);
        g_screen_state_00649f1c->dialogue_text_11c->SetEnabled(1);
    }
    if (g_screen_state_00649f1c->dialogue_npc->record->flag_055 == 0) {
        g_screen_state_00649f1c->dialogue_text_110->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_text_114->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_text_11c->SetEnabled(0);
    }
    g_screen_state_00649f1c->dialogue_text_118->SetEnabled(1);
    Function58F6B0(2);
    if (gXStatus.fCampMode == 0) {
        ResetEditorStatusLine0058AA20(2);
    }
    g_screen_state_00649f1c->value_1d0 = 0;
    for (index = 0; index < 6; ++index) {
        g_screen_state_00649f1c->option_buttons_170[index]->SetEnabled(0);
    }
    Function5ADB10(1);
    RequestRedraw(0x200);
}

/* Open the mode-5 dialogue layout: the smaller control set shares the
   mode-4 labels and callbacks, the purse readout is refreshed and the
   mode-4-only controls are parked. */
// FUNCTION: WIZ8 0x005732A0
void OpenNpcDialogueMode5Layout(void)
{
    wchar_t buffer[0x20];
    int index;

    g_screen_state_00649f1c->value_fc = 5;
    if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
        SetNpcDialogueHidden(0);
    }
    g_screen_state_00649f1c->panel_1a8->SetEnabled(1);
    g_screen_state_00649f1c->panel_1ac->SetEnabled(1);
    g_screen_state_00649f1c->panel_1bc->SetEnabled(1);
    RegionSetEnable(0x18);
    RegionSetEnable(0x17);
    g_screen_state_00649f1c->dialogue_text_10c->m_textBuffer.SetText(gppStringList[0x1ce4 / 4],
                                                                     g_wiz_text_bold_font_683664);
    g_screen_state_00649f1c->dialogue_text_110->m_textBuffer.SetText(gppStringList[0x1cf4 / 4],
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_110->m_primaryActivationCallback =
        SetNpcDialogueSubMode4;
    g_screen_state_00649f1c->dialogue_text_114->m_textBuffer.SetText(gppStringList[0x1cf8 / 4],
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_114->m_primaryActivationCallback =
        SetNpcDialogueSubMode3;
    g_screen_state_00649f1c->dialogue_text_118->m_textBuffer.SetText(gppStringList[0x1cfc / 4],
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_118->m_primaryActivationCallback =
        SetNpcDialogueSubMode2;
    g_screen_state_00649f1c->dialogue_text_11c->m_textBuffer.SetText(gppStringList[0x1cb8 / 4],
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_11c->m_primaryActivationCallback =
        SetNpcDialogueSubMode5;
    g_screen_state_00649f1c->dialogue_text_188->SetEnabled(0);
    g_screen_state_00649f1c->dialogue_text_188->m_primaryActivationCallback = ConfirmNpcTradeSlot;
    swprintf(buffer, L"%dg", g_status_685170.party_gold);
    g_screen_state_00649f1c->dialogue_text_190->m_textBuffer.SetText(buffer, g_font_683660);
    g_screen_state_00649f1c->dialogue_text_1a0->SetEnabled(0);
    g_screen_state_00649f1c->dialogue_text_1a0->m_primaryActivationCallback = RestockNpcTradeStock;
    g_screen_state_00649f1c->dialogue_text_120->SetActive(0);
    g_screen_state_00649f1c->dialogue_text_124->SetActive(0);
    g_screen_state_00649f1c->value_1d0 = 0;
    for (index = 0; index < 6; ++index) {
        g_screen_state_00649f1c->option_buttons_170[index]->SetEnabled(0);
    }
    if (g_screen_state_00649f1c->dialogue_npc->record->flag_055 == 0) {
        g_screen_state_00649f1c->dialogue_text_110->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_text_114->SetEnabled(0);
    }
    g_screen_state_00649f1c->dialogue_text_118->SetEnabled(1);
    RequestRedraw(0x200);
    Function58F6B0(2);
    if (gXStatus.fCampMode == 0) {
        ResetEditorStatusLine0058AA20(2);
    }
}

/* Open the mode-1 service layout: the caption and three service options, the
   mode-4-only controls parked, and each option enabled from what the selected
   character can actually cast or use. */
// FUNCTION: WIZ8 0x00573AE0
void OpenNpcDialogueMode1Layout(void)
{
    W8Character* character;

    g_screen_state_00649f1c->value_fc = 1;
    if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
        SetNpcDialogueHidden(0);
    }
    g_screen_state_00649f1c->panel_1a8->SetEnabled(1);
    g_screen_state_00649f1c->panel_1ac->SetEnabled(1);
    g_screen_state_00649f1c->panel_1b8->SetEnabled(1);
    g_screen_state_00649f1c->dialogue_text_1a4->SetActive(0);
    g_screen_state_00649f1c->dialogue_text_1a4->m_textBuffer.SetText(&g_wchar_00689b34,
                                                                     g_wiz_text_bold_font_683664);
    RegionSetEnable(0x18);
    g_screen_state_00649f1c->dialogue_text_10c->m_textBuffer.SetText(gppStringList[0x1cbc / 4],
                                                                     g_wiz_text_bold_font_683664);
    g_screen_state_00649f1c->dialogue_text_110->m_textBuffer.SetText(gppStringList[0x1cc0 / 4],
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_110->m_primaryActivationCallback =
        RequestNpcSpellService3;
    g_screen_state_00649f1c->dialogue_text_114->m_textBuffer.SetText(gppStringList[0x1cc8 / 4],
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_114->m_primaryActivationCallback =
        RequestNpcSpellService41;
    g_screen_state_00649f1c->dialogue_text_11c->m_textBuffer.SetText(gppStringList[0x1ccc / 4],
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_11c->m_primaryActivationCallback =
        RequestNpcCharacterService;
    g_screen_state_00649f1c->dialogue_text_11c->AddLayoutFlags(g_W8TextControlMask005ED578);
    g_screen_state_00649f1c->dialogue_text_11c->EnableRegionHelp(0x7ca);
    g_screen_state_00649f1c->dialogue_text_118->SetActive(0);
    g_screen_state_00649f1c->dialogue_text_120->SetActive(0);
    g_screen_state_00649f1c->dialogue_text_124->SetActive(0);
    if (g_screen_state_00649f1c->value_fc == 1) {
        character = &g_status_685170.buffers.characters[g_status_685170.selected_character];
        g_screen_state_00649f1c->dialogue_text_110->SetEnabled(CanCharacterCastSpell(character, 3));
        g_screen_state_00649f1c->dialogue_text_110->W8Widget::Invalidate(1);
        g_screen_state_00649f1c->dialogue_text_114->SetEnabled(
            CanCharacterCastSpell(character, 0x29));
        g_screen_state_00649f1c->dialogue_text_114->W8Widget::Invalidate(1);
        g_screen_state_00649f1c->dialogue_text_11c->SetEnabled(CharacterHasServiceItem(character));
        g_screen_state_00649f1c->dialogue_text_11c->W8Widget::Invalidate(1);
    }
    RequestRedraw(0x200);
    Function58F6B0(2);
    if (gXStatus.fCampMode == 0) {
        ResetEditorStatusLine0058AA20(2);
    }
}

/* Remove the transcript entry that owns the selected visible line, then
   redraw the controller. */
// FUNCTION: WIZ8 0x0055E940
void W8NpcDialogueTextController::SaveTranscriptEntries()
{
    W8DialogueTranscriptRecord* record;
    W8DialogTextEntry* entry;
    int index;

    ClearNpcDialogueTranscript();
    for (index = 0; index < text_area.m_all_lines_01c.count; ++index) {
        record =
            static_cast<W8DialogueTranscriptRecord*>(malloc(sizeof(W8DialogueTranscriptRecord)));
        memset(record, 0, sizeof(W8DialogueTranscriptRecord));
        entry = text_area.GetEntry(index);
        entry->CopyTextTo(record->text);
        record->category = entry->m_category;
        g_screen_state_00649f1c->dialogue_transcript.Add(record);
    }
}

// FUNCTION: WIZ8 0x0055EA40
void W8NpcDialogueTextController::ClearTranscriptEntries()
{
    while (0u < (unsigned)text_area.m_all_lines_01c.count) {
        text_area.RemoveEntry(0);
    }
    Invalidate(0);
}

// FUNCTION: WIZ8 0x0055EA70
void W8NpcDialogueTextController::RemoveSelectedTranscriptEntry()
{
    if (text_area.m_state_5d_entry != -1) {
        int index = text_area.GetOwningEntryIndex(text_area.m_state_5d_entry);
        if (index != -1) {
            text_area.RemoveEntry(index);
            Invalidate(0);
        }
    }
}

// FUNCTION: WIZ8 0x0055EAB0
int W8NpcDialogueTextController::GetSelectedTranscriptEntryIndex()
{
    return text_area.m_state_5d_entry;
}

// FUNCTION: WIZ8 0x0055EAC0
void W8NpcDialogueTextController::SetTranscriptSorted(unsigned char sorted)
{
    text_area.SetSorted(sorted);
    Invalidate(0);
}

/* Forward mouse enter/leave and button events to the main-screen control whose
   pointer sits at W8MainScreenState::dialogue_text_10c[callback_id]. Id 0x27
   is the panel slot at +0x1a8 and is ignored. Ids 1..6 and 0x16/0x17 pass 1
   into the widget hooks; every other id passes 0. Mouse-enter always passes 0. */
// FUNCTION: WIZ8 0x0056F020
unsigned char MainScreenControlRegionEvent(const InputAtom* event, W8Region* region)
{
    unsigned int callback_id = region->callback_id;
    W8Widget* control;
    unsigned char arg;

    if (callback_id == 0x27) {
        return 0;
    }
    control =
        // reinterpret-ok: retail indexes contiguous control* slots from dialogue_text_10c
        reinterpret_cast<W8Widget**>(&g_screen_state_00649f1c->dialogue_text_10c)[callback_id];
    if (control == 0) {
        return 0;
    }
    switch (event->usEvent) {
    case LEFT_BUTTON_DOWN:
    case LEFT_BUTTON_REPEAT:
        if ((callback_id >= 1 && callback_id <= 6) || callback_id == 0x16 || callback_id == 0x17) {
            arg = 1;
        } else {
            arg = 0;
        }
        control->OnLeftButtonDown(arg);
        region->flags |= W8_REGION_LEFT_BUTTON_HELD;
        return 1;
    case LEFT_BUTTON_UP:
        if ((callback_id >= 1 && callback_id <= 6) || callback_id == 0x16 || callback_id == 0x17) {
            arg = 1;
        } else {
            arg = 0;
        }
        control->OnLeftButtonUp(arg);
        if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) == 0) {
            return 1;
        }
        region->flags &= ~W8_REGION_LEFT_BUTTON_HELD;
        return 1;
    case RIGHT_BUTTON_DOWN:
    case RIGHT_BUTTON_REPEAT:
        if ((callback_id >= 1 && callback_id <= 6) || callback_id == 0x16 || callback_id == 0x17) {
            arg = 1;
        } else {
            arg = 0;
        }
        control->OnRightButtonDown(arg);
        region->flags |= W8_REGION_RIGHT_BUTTON_HELD;
        return 1;
    case RIGHT_BUTTON_UP:
        if ((callback_id >= 1 && callback_id <= 6) || callback_id == 0x16 || callback_id == 0x17) {
            arg = 1;
        } else {
            arg = 0;
        }
        control->OnRightButtonUp(arg);
        if ((region->flags & W8_REGION_RIGHT_BUTTON_HELD) != 0) {
            region->flags &= ~W8_REGION_RIGHT_BUTTON_HELD;
        }
        return 1;
    case MOUSE_POS:
        if ((region->flags & W8_REGION_MOUSE_LEAVE) != 0) {
            if ((callback_id >= 1 && callback_id <= 6) || callback_id == 0x16 ||
                callback_id == 0x17) {
                arg = 1;
            } else {
                arg = 0;
            }
            control->OnMouseLeave(arg);
            return 1;
        }
        if ((region->flags & W8_REGION_MOUSE_ENTER) != 0) {
            control->OnMouseEnter(0);
            return 1;
        }
        break;
    }
    return 0;
}

/* Reset the secondary dialogue editor: drop the pending item, clear the item
   notice control and both secondary labels, and restore the option row. */
// FUNCTION: WIZ8 0x0056FED0
void ResetNpcDialogueItemEditor(void)
{
    ClearTextSlot1E8(2);
    g_screen_state_00649f1c->value_108 = 0;
    g_screen_state_00649f1c->dialogue_text_1a0->SetEnabled(0);
    g_screen_state_00649f1c->dialogue_text_16c->m_imageObject = -1;
    g_screen_state_00649f1c->dialogue_text_16c->m_measured_w = -1;
    g_screen_state_00649f1c->dialogue_text_16c->m_measured_h = -1;
    g_screen_state_00649f1c->dialogue_text_16c->m_imageFrame = -1;
    g_screen_state_00649f1c->dialogue_text_16c->m_normalSprite = -1;
    g_screen_state_00649f1c->dialogue_text_16c->m_pressedSprite = -1;
    g_screen_state_00649f1c->dialogue_text_16c->m_textBuffer.SetText(g_wchar_0068ee58, 0);
    g_screen_state_00649f1c->dialogue_text_16c->Invalidate(1);
    g_screen_state_00649f1c->dialogue_text_198->m_textBuffer.SetText(&g_wchar_00689b34,
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_19c->m_textBuffer.SetText(&g_wchar_00689b34,
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_188->SetEnabled(0);
    g_screen_state_00649f1c->value_258 = -1;
    g_screen_state_00649f1c->value_254 = 1;
    g_screen_state_00649f1c->dialogue_text_1a0->m_textBuffer.m_fontStateIndex = -1;
    g_screen_state_00649f1c->dialogue_text_1a0->m_textBuffer.m_geometryDirty = 1;
}

// FUNCTION: WIZ8 0x00570310
void LeaveNpcDialogueLayout(void)
{
    if (g_screen_state_00649f1c->flag_250 == 0) {
        if (GetNpcDispositionBand(g_screen_state_00649f1c->dialogue_npc) == 0) {
            g_screen_state_00649f1c->flag_23c = 0;
            g_screen_state_00649f1c->flag_251 = 1;
            QueueNpcScriptLine(0x5c, 0, 0, 0);
            return;
        }
        switch (g_screen_state_00649f1c->value_fc) {
        case 1:
            CloseNpcDialogueMode1Layout();
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
            g_screen_state_00649f1c->value_fc = W8_DIALOGUE_LAYOUT_NONE;
            if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
                SetNpcDialogueHidden(0);
            }
            break;
        case 3:
            CloseNpcDialogueTranscriptLayout();
            break;
        case 4:
            CloseNpcDialogueOptionLayout();
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
            g_screen_state_00649f1c->value_fc = W8_DIALOGUE_LAYOUT_NONE;
            if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
                SetNpcDialogueHidden(0);
            }
            break;
        case 6:
            g_screen_state_00649f1c->value_104 = g_screen_state_00649f1c->value_fc;
            g_screen_state_00649f1c->value_fc = W8_DIALOGUE_LAYOUT_NONE;
            if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
                SetNpcDialogueHidden(0);
            }
            break;
        }
    }
    Function56E800(0);
}

// FUNCTION: WIZ8 0x00570530
void PromptNpcDispositionChange(void)
{
    W8MessageDialogBase* dialog = static_cast<W8MessageDialogBase*>(CreateDialogByKind(1));

    SetDialogPrompt(dialog, gppStringList[0x1e60 / 4], 0, 0);
    dialog->m_destroy_callback = OnNpcDispositionPromptClosed;
    OpenModal(dialog);
}

// FUNCTION: WIZ8 0x00570570
void OnNpcDispositionPromptClosed(W8DialogBase* dialog)
{
    if (GetDialogResult(dialog) != 0) {
        SetNpcDispositionBand(g_screen_state_00649f1c->dialogue_npc, 2);
        QueueNpcScriptLine(0x18, 0, 0, 0);
        QueueNpcMessageLine(W8_NPC_MSG_CLOSE_RESUME_NPC, 0);
    }
}

// FUNCTION: WIZ8 0x005705B0
void EnterNpcServiceLayout(void)
{
    switch (g_screen_state_00649f1c->value_fc) {
    case 1:
        CloseNpcDialogueMode1Layout();
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
        g_screen_state_00649f1c->value_fc = W8_DIALOGUE_LAYOUT_NONE;
        if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
            SetNpcDialogueHidden(0);
        }
        break;
    case 3:
        CloseNpcDialogueTranscriptLayout();
        break;
    case 4:
        CloseNpcDialogueOptionLayout();
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
        g_screen_state_00649f1c->value_fc = W8_DIALOGUE_LAYOUT_NONE;
        if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
            SetNpcDialogueHidden(0);
        }
        break;
    case 6:
        g_screen_state_00649f1c->value_104 = g_screen_state_00649f1c->value_fc;
        g_screen_state_00649f1c->value_fc = W8_DIALOGUE_LAYOUT_NONE;
        if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
            SetNpcDialogueHidden(0);
        }
        break;
    }
    OpenNpcDialogueMode1Layout();
}

// FUNCTION: WIZ8 0x005714D0
void RequestNpcJoinParty(void)
{
    unsigned int slot;

    ShowNotice(0xa, gppStringList[0x1d20 / 4], 3, GetTextBoxScrollRange(), 0);
    if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
        SetNpcDialogueHidden(0);
    }
    if (g_screen_state_00649f1c->dialogue_npc->record->has_group != 0) {
        slot = FindFreePartySlot(0, 2);
        if (slot == 0xffffffff) {
            QueueNpcScriptLine(0xc, 0, 0, 0);
            return;
        }
        if (CanNpcJoinParty(g_screen_state_00649f1c->dialogue_npc) != 0) {
            QueueNpcScriptLine(0xd, 0, 0, 0);
            QueueNpcMessageLine(W8_NPC_MSG_FOCUS_NPC,
                                g_screen_state_00649f1c->dialogue_npc->name_style);
            return;
        }
    }
    QueueNpcScriptLine(0xb, 0, 0, 0);
}

// FUNCTION: WIZ8 0x005715A0
void ShowNpcDialogueNotice(void)
{
    ShowNotice(0xa, gppStringList[0x1d24 / 4], 3, GetTextBoxScrollRange(), 0);
    SetNpcDialogueHidden(1);
}

// FUNCTION: WIZ8 0x005715D0
void EnterNpcTradeOptions(void)
{
    CloseNpcDialogueTranscriptLayout();
    g_screen_state_00649f1c->value_100 = 4;
    OpenNpcDialogueOptionLayout();
}

// FUNCTION: WIZ8 0x005715F0
void ScrollNpcDialogueUp(void)
{
    g_screen_state_00649f1c->npc_dialogue_controller_1b0->HandleScrollUpCommand(0);
}

// FUNCTION: WIZ8 0x00571610
void ScrollNpcDialogueDown(void)
{
    g_screen_state_00649f1c->npc_dialogue_controller_1b0->HandleScrollDownCommand(0);
}

// FUNCTION: WIZ8 0x00571630
void SubmitNpcDialogueKeyword(void)
{
    wchar_t keyword[200];

    Get16BitStringFromField(0, keyword);
    AddNpcDialogueKeyword(keyword, -1, 0);
}

// FUNCTION: WIZ8 0x00571880
void RefreshNpcDialogueTranscript(void)
{
    g_screen_state_00649f1c->npc_dialogue_controller_1b0->RemoveSelectedTranscriptEntry();
    CollapseNpcDialogueTextArea(g_screen_state_00649f1c->npc_dialogue_controller_1b0);
    ExpandNpcDialogueTextArea(g_screen_state_00649f1c->npc_dialogue_controller_1b0);
    if (IsNpcDialogueTextExpanded(g_screen_state_00649f1c->npc_dialogue_controller_1b0)) {
        g_screen_state_00649f1c->dialogue_scroll_up_button->SetEnabled(1);
        g_screen_state_00649f1c->dialogue_scroll_down_button->SetEnabled(1);
    } else {
        g_screen_state_00649f1c->dialogue_scroll_up_button->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_scroll_down_button->SetEnabled(0);
    }
}

// FUNCTION: WIZ8 0x00571920
void SyncNpcDialogueListFilter(void)
{
    if (static_cast<unsigned char>(g_screen_state_00649f1c->dialogue_sort_button->m_stateFlags &
                                   g_W8TextControlMask005ED570) != 0) {
        g_screen_state_00649f1c->transcript_sorted = 1;
    } else {
        g_screen_state_00649f1c->transcript_sorted = 0;
    }
    g_screen_state_00649f1c->npc_dialogue_controller_1b0->SetTranscriptSorted(
        g_screen_state_00649f1c->transcript_sorted);
}

// FUNCTION: WIZ8 0x00571960
void SelectNpcDialogueCategory1(void)
{
    if (static_cast<unsigned char>(g_screen_state_00649f1c->dialogue_people_button->m_stateFlags &
                                   g_W8TextControlMask005ED570) != 0) {
        g_screen_state_00649f1c->dialogue_category_filter = W8_DIALOGUE_CATEGORY_PEOPLE;
        SyncDialogueCategoryButtons();
    }
    g_screen_state_00649f1c->npc_dialogue_controller_1b0->SetTranscriptCategoryFilter(
        W8_DIALOGUE_CATEGORY_PEOPLE);
}

// FUNCTION: WIZ8 0x005719A0
void SelectNpcDialogueCategory2(void)
{
    if (static_cast<unsigned char>(g_screen_state_00649f1c->dialogue_places_button->m_stateFlags &
                                   g_W8TextControlMask005ED570) != 0) {
        g_screen_state_00649f1c->dialogue_category_filter = W8_DIALOGUE_CATEGORY_PLACES;
        SyncDialogueCategoryButtons();
    }
    g_screen_state_00649f1c->npc_dialogue_controller_1b0->SetTranscriptCategoryFilter(
        W8_DIALOGUE_CATEGORY_PLACES);
}

// FUNCTION: WIZ8 0x005719E0
void SelectNpcDialogueCategory0(void)
{
    if (static_cast<unsigned char>(g_screen_state_00649f1c->dialogue_items_button->m_stateFlags &
                                   g_W8TextControlMask005ED570) != 0) {
        g_screen_state_00649f1c->dialogue_category_filter = W8_DIALOGUE_CATEGORY_ITEMS;
        SyncDialogueCategoryButtons();
    }
    g_screen_state_00649f1c->npc_dialogue_controller_1b0->SetTranscriptCategoryFilter(
        W8_DIALOGUE_CATEGORY_ITEMS);
}

// FUNCTION: WIZ8 0x00571A20
void SelectNpcDialogueCategoryAll(void)
{
    if (static_cast<unsigned char>(g_screen_state_00649f1c->dialogue_all_button->m_stateFlags &
                                   g_W8TextControlMask005ED570) != 0) {
        g_screen_state_00649f1c->dialogue_category_filter = W8_DIALOGUE_CATEGORY_ALL;
        SyncDialogueCategoryButtons();
    }
    g_screen_state_00649f1c->npc_dialogue_controller_1b0->SetTranscriptCategoryFilter(
        W8_DIALOGUE_CATEGORY_ALL);
}

// FUNCTION: WIZ8 0x00571A60
void SelectNpcDialogueCategory3(void)
{
    if (static_cast<unsigned char>(g_screen_state_00649f1c->dialogue_misc_button->m_stateFlags &
                                   g_W8TextControlMask005ED570) != 0) {
        g_screen_state_00649f1c->dialogue_category_filter = W8_DIALOGUE_CATEGORY_MISC;
        SyncDialogueCategoryButtons();
    }
    g_screen_state_00649f1c->npc_dialogue_controller_1b0->SetTranscriptCategoryFilter(
        W8_DIALOGUE_CATEGORY_MISC);
}

// FUNCTION: WIZ8 0x00571F60
void UpdateNpcDialogueSubMode(void)
{
    switch (g_screen_state_00649f1c->value_100) {
    case 2:
        g_screen_state_00649f1c->dialogue_text_118->EnableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_text_110->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_text_114->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_text_11c->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_text_10c->m_textBuffer.SetText(
            gppStringList[0x1cec / 4], g_wiz_text_bold_font_683664);
        g_screen_state_00649f1c->dialogue_text_1a0->m_textBuffer.SetText(
            gppStringList[0x1cec / 4], g_wiz_text_bold_font_683664);
        g_screen_state_00649f1c->dialogue_text_120->SetEnabled(1);
        g_screen_state_00649f1c->dialogue_text_124->SetEnabled(1);
        break;
    case 3:
        g_screen_state_00649f1c->dialogue_text_114->EnableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_text_110->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_text_118->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_text_11c->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_text_120->SetEnabled(1);
        g_screen_state_00649f1c->dialogue_text_124->SetEnabled(1);
        g_screen_state_00649f1c->dialogue_text_10c->m_textBuffer.SetText(
            gppStringList[0x1ce8 / 4], g_wiz_text_bold_font_683664);
        g_screen_state_00649f1c->dialogue_text_1a0->m_textBuffer.SetText(
            gppStringList[0x1ce8 / 4], g_wiz_text_bold_font_683664);
        break;
    case 4:
        g_screen_state_00649f1c->dialogue_text_110->EnableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_text_114->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_text_118->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_text_11c->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_text_10c->m_textBuffer.SetText(
            gppStringList[0x1cf0 / 4], g_wiz_text_bold_font_683664);
        g_screen_state_00649f1c->dialogue_text_1a0->m_textBuffer.SetText(
            gppStringList[0x1cf0 / 4], g_wiz_text_bold_font_683664);
        g_screen_state_00649f1c->dialogue_text_120->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_text_124->SetEnabled(0);
        break;
    case 5:
        g_screen_state_00649f1c->dialogue_text_11c->EnableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_text_114->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_text_118->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_text_110->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_text_10c->m_textBuffer.SetText(
            gppStringList[0x1cb8 / 4], g_wiz_text_bold_font_683664);
        g_screen_state_00649f1c->dialogue_text_1a0->m_textBuffer.SetText(
            gppStringList[0x1cb8 / 4], g_wiz_text_bold_font_683664);
        g_screen_state_00649f1c->dialogue_text_120->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_text_124->SetEnabled(0);
        break;
    }
    if (g_screen_state_00649f1c->flag_234 != 0 &&
        (g_screen_state_00649f1c->value_100 == 3 || g_screen_state_00649f1c->value_100 == 2)) {
        if (g_screen_state_00649f1c->flag_260 != 0) {
            g_screen_state_00649f1c->dialogue_text_124->DisableSecondaryState(1);
            g_screen_state_00649f1c->dialogue_text_124->m_textBuffer.m_fontStateIndex = -1;
            g_screen_state_00649f1c->dialogue_text_124->m_textBuffer.m_geometryDirty = 1;
            g_screen_state_00649f1c->dialogue_text_120->EnableSecondaryState(1);
            g_screen_state_00649f1c->dialogue_text_120->m_textBuffer.m_fontStateIndex = 3;
            g_screen_state_00649f1c->dialogue_text_120->m_textBuffer.m_geometryDirty = 1;
        } else {
            g_screen_state_00649f1c->dialogue_text_120->DisableSecondaryState(1);
            g_screen_state_00649f1c->dialogue_text_120->m_textBuffer.m_fontStateIndex = -1;
            g_screen_state_00649f1c->dialogue_text_120->m_textBuffer.m_geometryDirty = 1;
            g_screen_state_00649f1c->dialogue_text_124->EnableSecondaryState(1);
            g_screen_state_00649f1c->dialogue_text_124->m_textBuffer.m_fontStateIndex = 3;
            g_screen_state_00649f1c->dialogue_text_124->m_textBuffer.m_geometryDirty = 1;
        }
        g_screen_state_00649f1c->flag_234 = 0;
    }
    Function5ADB10(1);
}

// FUNCTION: WIZ8 0x00572590
void OpenNpcItemAssay(void)
{
    W8AssayDialog* dialog;

    if (g_screen_state_00649f1c->value_108 != 0 &&
        g_screen_state_00649f1c->dialogue_text_16c->m_imageObject != -1 &&
        g_screen_state_00649f1c->dialogue_text_16c->m_imageObject != 0x1ac) {
        dialog = new W8AssayDialog(
            g_screen_state_00649f1c->value_108,
            &g_status_685170.buffers.characters[g_status_685170.selected_character]);
        dialog->SetText(&g_wchar_00689b34);
        dialog->SetOrigin(g_info_dialog_x_005ef958, 0x48);
        dialog->m_destroy_callback = OnNpcAssayDialogClosed;
        OpenModal(dialog);
    }
}

// FUNCTION: WIZ8 0x00572670
void OnNpcAssayDialogClosed(W8DialogBase*)
{
    RequestRedraw(-1);
}

// FUNCTION: WIZ8 0x00572680
void SelectNpcTradeMode1(void)
{
    ResetNpcDialogueItemEditor();
    g_screen_state_00649f1c->dialogue_text_124->DisableSecondaryState(1);
    g_screen_state_00649f1c->dialogue_text_124->m_textBuffer.m_fontStateIndex = -1;
    g_screen_state_00649f1c->dialogue_text_124->m_textBuffer.m_geometryDirty = 1;
    g_screen_state_00649f1c->dialogue_text_120->EnableSecondaryState(1);
    g_screen_state_00649f1c->dialogue_text_120->m_textBuffer.m_fontStateIndex = 3;
    g_screen_state_00649f1c->dialogue_text_120->m_textBuffer.m_geometryDirty = 1;
    Function5ADB10(1);
    g_screen_state_00649f1c->flag_260 = 1;
}

// FUNCTION: WIZ8 0x00572700
void SelectNpcTradeMode0(void)
{
    ResetNpcDialogueItemEditor();
    g_screen_state_00649f1c->dialogue_text_120->DisableSecondaryState(1);
    g_screen_state_00649f1c->dialogue_text_120->m_textBuffer.m_fontStateIndex = -1;
    g_screen_state_00649f1c->dialogue_text_120->m_textBuffer.m_geometryDirty = 1;
    g_screen_state_00649f1c->dialogue_text_124->EnableSecondaryState(1);
    g_screen_state_00649f1c->dialogue_text_124->m_textBuffer.m_fontStateIndex = 3;
    g_screen_state_00649f1c->dialogue_text_124->m_textBuffer.m_geometryDirty = 1;
    Function5ADB10(1);
    g_screen_state_00649f1c->flag_260 = 0;
}

// FUNCTION: WIZ8 0x00572960
void ConfirmNpcTradeSlot(void)
{
    int slot;

    if (g_screen_state_00649f1c->value_fc == W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX) {
        slot = GetTextSlot1E8(2);
        if (slot != -1) {
            Function56FAC0(slot, 0, 0);
            if (g_screen_state_00649f1c->value_100 == 2 && slot == 0) {
                Function572780();
            } else if (g_screen_state_00649f1c->value_108->stack_count > 1) {
                Function5AE040();
            }
        }
    }
}

// FUNCTION: WIZ8 0x00573A80
void SetNpcDialogueSubMode3(void)
{
    g_screen_state_00649f1c->value_100 = 3;
    UpdateNpcDialogueSubMode();
}

// FUNCTION: WIZ8 0x00573AA0
void SetNpcDialogueSubMode2(void)
{
    g_screen_state_00649f1c->value_100 = 2;
    UpdateNpcDialogueSubMode();
}

// FUNCTION: WIZ8 0x00573AC0
void SetNpcDialogueSubMode5(void)
{
    g_screen_state_00649f1c->value_100 = 5;
    UpdateNpcDialogueSubMode();
}

// FUNCTION: WIZ8 0x00573ED0
void RequestNpcSpellService3(void)
{
    int location = g_screen_state_00649f1c->target_location_id_f8;
    int mode = g_screen_state_00649f1c->value_fc;

    gXStatus.fCampMode = 1;
    CloseNpcDialogueMode1Layout();
    Function56E800(0);
    BeginSpellCast005A0110(3, location, mode);
}

// FUNCTION: WIZ8 0x00573F10
void RequestNpcSpellService41(void)
{
    int location = g_screen_state_00649f1c->target_location_id_f8;
    int mode = g_screen_state_00649f1c->value_fc;

    gXStatus.fCampMode = 1;
    CloseNpcDialogueMode1Layout();
    Function56E800(0);
    BeginSpellCast005A0110(0x29, location, mode);
}

// FUNCTION: WIZ8 0x00573F50
void RequestNpcCharacterService(void)
{
    gXStatus.fCampMode = 1;
    CloseNpcDialogueMode1Layout();
    Function56E800(0);
    OpenUseItemSelectView(g_status_685170.selected_character);
}

// FUNCTION: WIZ8 0x005AE000
void SetNpcDialogueSubMode4(void)
{
    g_screen_state_00649f1c->value_100 = 4;
    UpdateNpcDialogueSubMode();
}

// FUNCTION: WIZ8 0x005AE020
void RestockNpcTradeStock(void)
{
    RestockNpcInventory(g_screen_state_00649f1c->dialogue_npc);
}

/* Condition orb on a party portrait (help 25): press while highest_condition
   is set arms the overlay slot; release and leave dismiss the hover plate;
   enter drives tooltip kind 1 and region help. */

// FUNCTION: WIZ8 0x0055E490
void W8NpcDialogueTextController::SelectTranscriptKeywordAtPoint(int x, int y)
{
    wchar_t keyword[200];
    unsigned int hit;

    if (g_screen_state_00649f1c->flag_250 != 0) {
        return;
    }

    hit = text_area.HitTestEntry(x, y);
    if (hit == static_cast<unsigned int>(-1)) {
        return;
    }

    if (text_area.m_state_5d_entry == static_cast<int>(hit)) {
        text_area.CopyVisibleEntryText(hit, keyword);
        text_area.ClearEntryState5D();
        text_area.SetEntryState5D(static_cast<int>(hit));
        SetDialogueFieldKeyword(keyword, 0);
        HandleNpcDialogueInput();
        text_area.SetEntryState60(text_area.GetOwningEntryIndex(static_cast<int>(hit)), 0);
        InvalidateLayout();
        return;
    }

    text_area.CopyVisibleEntryText(hit, keyword);
    text_area.ClearEntryState5D();
    text_area.SetEntryState5D(static_cast<int>(hit));
    SetDialogueFieldKeyword(keyword, 0);
    InvalidateLayout();
}

// FUNCTION: WIZ8 0x0055E690
unsigned char DialogueTranscriptRegionEvent(const InputAtom* event, W8Region* region)
{
    int us_event = event->usEvent;
    W8NpcDialogueTextController* controller;
    unsigned char scrolled;
    short delta;

    if (us_event <= LEFT_BUTTON_REPEAT) {
        if (us_event == LEFT_BUTTON_REPEAT || us_event == LEFT_BUTTON_DOWN) {
            region->flags |= W8_REGION_LEFT_BUTTON_HELD;
            return 1;
        }
        if (us_event != LEFT_BUTTON_UP) {
            return 0;
        }
        if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) != 0) {
            region->flags &= ~W8_REGION_LEFT_BUTTON_HELD;
            g_screen_state_00649f1c->npc_dialogue_controller_1b0->SelectTranscriptKeywordAtPoint(
                static_cast<unsigned short>(event->uiParam),
                static_cast<unsigned short>(event->uiParam >> 16));
            return 1;
        }
    } else {
        if (us_event != MOUSE_POS) {
            if (us_event != MOUSE_WHEEL) {
                return 0;
            }
            delta = GetMouseWheelDeltaValue(event->usParam);
            if (delta > 0) {
                controller = g_screen_state_00649f1c->npc_dialogue_controller_1b0;
                scrolled = controller->text_area.ScrollUp(0);
                if (scrolled == 0) {
                    return 0;
                }
                controller->Invalidate(0);
                return 0;
            }
            controller = g_screen_state_00649f1c->npc_dialogue_controller_1b0;
            scrolled = controller->text_area.ScrollDown(0);
            if (scrolled == 0) {
                return 0;
            }
            controller->Invalidate(0);
            return 0;
        }
        if ((region->flags & W8_REGION_MOUSE_LEAVE) == 0) {
            if ((region->flags & W8_REGION_MOUSE_ENTER) == 0) {
                controller = g_screen_state_00649f1c->npc_dialogue_controller_1b0;
                if (controller->text_area.UpdateSelectionFromPoint(
                        static_cast<unsigned short>(event->uiParam),
                        static_cast<unsigned short>(event->uiParam >> 16)) != 0) {
                    controller->InvalidateLayout();
                }
            }
        } else {
            controller = g_screen_state_00649f1c->npc_dialogue_controller_1b0;
            if (controller->text_area.ClearPointSelection() != 0) {
                controller->InvalidateLayout();
                return 1;
            }
        }
    }
    return 1;
}

// FUNCTION: WIZ8 0x005699D0
unsigned char CombatBarRegionEvent(const InputAtom* event)
{
    int us_event = event->usEvent;

    if (us_event <= RIGHT_BUTTON_DOWN) {
        if (us_event == RIGHT_BUTTON_DOWN || us_event == LEFT_BUTTON_DOWN ||
            us_event == LEFT_BUTTON_UP) {
            goto resume_world;
        }
    } else {
        if (us_event == RIGHT_BUTTON_UP) {
            goto resume_world;
        }
        if (us_event == MOUSE_POS) {
            return 1;
        }
    }

    if (g_mgs_keyboard->FindCommandForEvent(event) == -1) {
        return 0;
    }

resume_world:
    if (gXStatus.fSurprisePossible != 0) {
        Function502790();
    }
    if (g_flag_006840bd == 0) {
        return 1;
    }
    ResumeMainGameWorld();
    return 1;
}

/* Note that the party's state changed. The combat half is only asked for while
   a fight is on; the party half always. */

// FUNCTION: WIZ8 0x00574F90
void SetDialogueFieldKeyword(wchar_t* keyword, unsigned char append)
{
    wchar_t field_text[200];
    wchar_t combined[200];

    Get16BitStringFromField(0, field_text);
    StripNpcKeywordPunctuation(keyword);
    if (wcslen(field_text) != 0 && append != 0) {
        swprintf(combined, g_format_s_space_s_00617584, field_text, keyword);
        SetInputFieldStringWith16BitString(0, combined);
    } else {
        SetInputFieldStringWith16BitString(0, keyword);
    }
}

// FUNCTION: WIZ8 0x00576650
unsigned char NpcQuoteBubbleRegionEvent(const InputAtom* event)
{
    if (event->usEvent != LEFT_BUTTON_DOWN) {
        return 0;
    }
    TryFinishNpcVoicePlayback(1);
    return 1;
}
