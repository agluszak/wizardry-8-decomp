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
#include "wiz8/local_screens/MGSPortraitCombat.h"
#include "wiz8/local_screens/NPCInteractionSubscreen.h"
#include "wiz8/local_screens/MGSRadarMap.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/layouts/npc_state.h"
#include "wiz8/local_code/NPCManager.h"
#include "wiz8/npc_items.h"
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
#include "surrender/srScene.h"
#include "surrender/srColorSurface.h"
#include "surrender/srMaterial.h"
#include "surrender/srMeshModel.h"
#include "surrender/srShader.h"
#include "wiz8/surface2d.h"
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

void ServiceNpcDialogue0056E510(void);
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
        SelectTextBox(0);
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
        SelectTextBox(0);
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
        SelectTextBox(0);
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
        SelectTextBox(0);
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
    SelectTextBox(0);
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
    SelectTextBox(0);
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
                            EndNpcDialogueSession0056E800(0);
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
            gXStatus.hostile_monster_count) {
            StartCombat(0);
        }
        if (!ClockIsTicking(g_level_block->character_update_timer)) {
            UpdateMonsterSight();
            g_level_block->character_update_timer = SetCountdownClock(500);
        }
        if (!g_flag_006840bc) {
            UpdateMonsterGroups(1);
            if (!gXStatus.fCombatMode && AnyCharacterActive() && gXStatus.hostile_monster_count &&
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
            ServiceNpcDialogue0056E510();
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
            EndNpcDialogueSession0056E800(0);
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
        EndNpcDialogueSession0056E800(0);
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
            if (gXStatus.monster_manager_entries[party_slot].portrait_stats_dirty != 0) {
                RedrawPartyPortraitBars(party_slot,
                                        g_level_block->portrait_refresh_pending[party_slot] == 0);
            }
            if (gXStatus.monster_manager_entries[party_slot].keyboard_menu_open != 0) {
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
            if (gXStatus.monster_manager_entries[party_slot].keyboard_menu_open != 0) {
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
        if (HasNpcDialogueDirtyPanels0056ED80() != 0 || GetOpenDialogueFlag() != 0) {
            ActivateNpcDialoguePanels0056ECF0(0);
        }
    } else {
        ActivateNpcDialoguePanels0056ECF0(1);
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
        SyncNpcServiceButtons0056EE20(g_status_685170.selected_character);
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
        gXStatus.monster_manager_entries[party_slot].effect_icon_active == 0 &&
        gXStatus.monster_manager_entries[party_slot].damage_splat_active == 0) {
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
            EndNpcDialogueSession0056E800(0);
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
    if (gXStatus.monster_manager_entries[party_slot].keyboard_menu_open == 0) {
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
            EndNpcDialogueSession0056E800(0);
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
            if (entry->damage_splat_active != 0 || entry->effect_icon_active != 0 ||
                (entry->portrait_event_active != 0 && gfCapturingVideo == 0)) {
                RefreshSelectedPartyPortrait(party_slot);
                entry->auto_portrait_refresh = 1;
            }
        } else if (g_status_685170.buffers.party_rows[party_slot].occupied == 0) {
            ClearPortraitRefreshSlot(static_cast<int>(party_slot));
            entry->auto_portrait_refresh = 0;
            DisableRegionInput(party_slot + 0x5a);
        } else if (entry->portrait_refresh_pinned == 0 && entry->damage_splat_active == 0 &&
                   entry->effect_icon_active == 0 && entry->portrait_event_active == 0) {
            if (IsPartyPortraitUnderCursor00561980(party_slot) == 0 ||
                entry->auto_portrait_refresh != 0) {
                ClearPortraitRefreshSlot(static_cast<int>(party_slot));
                entry->auto_portrait_refresh = 0;
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
        gXStatus.monster_manager_entries[slot].portrait_refresh_pinned = 0;
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
            if (gXStatus.monster_manager_entries[slot].keyboard_menu_open == 0) {
                EnableRegionSetInput(region_set);
            } else {
                DisableRegionSetInput(region_set);
            }
        } else if (static_cast<unsigned int>(g_settings_6850c8.main_ui_mode) <=
                   static_cast<unsigned int>(W8_MAIN_UI_MODE_RADAR)) {
            if (gXStatus.monster_manager_entries[slot].keyboard_menu_open == 0) {
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
                    TryNpcDialoguePickpocket0056EFF0(slot);
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
                        EndNpcDialogueSession0056E800(0);
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
                        EndNpcDialogueSession0056E800(0);
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
                        EndNpcDialogueSession0056E800(0);
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
            gXStatus.monster_manager_entries[slot].combat_portrait_dirty = 1;
            DisableRegionHelpFlag004F27E0(region);
            return 0;
        }
        if ((region->flags & W8_REGION_MOUSE_ENTER) == 0) {
            return 0;
        }
        if (g_combat_state->flag_001 != 0) {
            g_level_block->party_slots_170[4] = slot;
            gXStatus.monster_manager_entries[slot].combat_portrait_dirty = 1;
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
                    EndNpcDialogueSession0056E800(0);
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
            EndNpcDialogueSession0056E800(0);
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
            EndNpcDialogueSession0056E800(0);
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
        AcknowledgeSurprise00502790();
    }
    if (g_flag_006840bd == 0) {
        return 1;
    }
    ResumeMainGameWorld();
    return 1;
}

/* Note that the party's state changed. The combat half is only asked for while
   a fight is on; the party half always. */

// GLOBAL: WIZ8 0x0068edb8
unsigned long g_surprise_fade_tick_base_0068edb8;
// GLOBAL: WIZ8 0x005ee9a8
const float g_fade_resume_scale_005ee9a8 = -500.0f;
// GLOBAL: WIZ8 0x0068edca
unsigned char g_surprise_fade_in_0068edca;
// GLOBAL: WIZ8 0x0068edf0
srColorSurfaceIFace* g_surprise_snapshot_surface_0068edf0;
// GLOBAL: WIZ8 0x0068edf4
stSurface2D* g_surprise_snapshot_overlay_0068edf4;
// GLOBAL: WIZ8 0x0068edf8
stModelInstance2D* g_surprise_fade_node_0068edf8;

// FUNCTION: WIZ8 0x00560C60
void ResetMainGameMode00560C60(void)
{
    switch (g_main_game_mode_0068eddc) {
    case 3:
        if (gXStatus.fNpcDialogueMode != 0) {
            EndNpcDialogueSession0056E800(0);
        }
        break;
    case 5:
        Function5187E0();
        break;
    case 6:
        if (g_level_block->highlight_graphic != 0) {
            ReleaseObject004257F0(g_level_block->highlight_graphic);
            g_level_block->highlight_graphic = 0;
            if (g_main_game_mode_0068eddc != 6) {
                goto mode_reset;
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
        if (0x166 < g_level_block->dialogue_height_228 + g_level_block->dialogue_y_224 &&
            g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
            g_level_block->redraw_flags |= 0x800;
        }
        break;
    }
mode_reset:
    g_main_game_mode_0068eddc = 0;
    if (IsMessageBoxActive()) {
        Function5187E0();
    }
    if (gXStatus.fCombatMode != 0) {
        EndCombat004EA310(1);
    } else if (AnyCharacterActive() && g_party_moving_006850b5 == 0) {
        AutoSaveIfAllowed(1);
    }
    if (gXStatus.fSurprisePossible != 0) {
        RestoreSurpriseView005029A0();
    }
    g_status_685170.game_started = 0;
    ClearHeldItemDisplay();
    RequestScreenTransition();
    SetPrimarySurfaceTextureHint2Enabled(0);
}

// FUNCTION: WIZ8 0x0056b4e0
void CreateSurpriseFade0056B4E0(void)
{
    srShader shader;
    srVector4T<float> color;

    SetFlag603C4C(0);
    color.x = 0.0f;
    color.y = 0.0f;
    color.z = 0.0f;
    color.w = 0.0f;
    g_surprise_fade_node_0068edf8 = CreateColoredPolygonSprite(0x280, 0x1e0, &color, 1);
    PositionToolTipNode(g_surprise_fade_node_0068edf8, 0, 0, 0);
    shader = static_cast<srMeshModel*>(g_surprise_fade_node_0068edf8->model())->getShader(0);
    shader.value = (shader.value & ~0x6040) | 0xa0;
    static_cast<srMeshModel*>(g_surprise_fade_node_0068edf8->model())->setShader(shader, 0);
    static_cast<srMaterial*>(static_cast<srMeshModel*>(g_surprise_fade_node_0068edf8->model())
                                 ->getMaterial(0, static_cast<srMeshModel::e_side>(0)))
        ->setOpacity(0.0);
    g_surprise_fade_tick_base_0068edb8 = GetTickCount();
    g_surprise_fade_in_0068edca = 1;
}

// FUNCTION: WIZ8 0x0056b5f0
void ReverseSurpriseFade0056B5F0(void)
{
    if (gXStatus.surprise_phase == 0) {
        srMaterial* material = static_cast<srMaterial*>(
            static_cast<srMeshModel*>(g_surprise_fade_node_0068edf8->model())
                ->getMaterial(0, static_cast<srMeshModel::e_side>(0)));
        float opacity = material->parms_18.diffuse.w;
        unsigned long now = GetTickCount();
        g_surprise_fade_in_0068edca = g_surprise_fade_in_0068edca == 0;
        if (g_surprise_fade_in_0068edca != 0) {
            g_surprise_fade_tick_base_0068edb8 =
                now + static_cast<int>((g_float_005ebb38 - opacity) * g_fade_resume_scale_005ee9a8);
        } else {
            g_surprise_fade_tick_base_0068edb8 =
                now + static_cast<int>(opacity * g_fade_resume_scale_005ee9a8);
        }
    } else {
        g_surprise_fade_node_0068edf8->clearFlag(srNode::FLAG_DISABLE);
        g_surprise_fade_tick_base_0068edb8 = GetTickCount();
        g_surprise_fade_in_0068edca = 1;
    }
}

// FUNCTION: WIZ8 0x0056b690
void DestroySurpriseFade0056B690(void)
{
    if (g_surprise_snapshot_overlay_0068edf4 != 0) {
        g_surprise_snapshot_overlay_0068edf4->release();
        g_surprise_snapshot_overlay_0068edf4 = 0;
    }
    if (g_surprise_snapshot_surface_0068edf0 != 0) {
        g_surprise_snapshot_surface_0068edf0->release();
        g_surprise_snapshot_surface_0068edf0 = 0;
    }
    if (g_surprise_fade_node_0068edf8 != 0) {
        g_surprise_fade_node_0068edf8->release();
        g_surprise_fade_node_0068edf8 = 0;
    }
    SetFlag603C4C(1);
    g_flag_65970d = 1;
}

// FUNCTION: WIZ8 0x0056b6f0
unsigned char UpdateSurpriseFade0056B6F0(void)
{
    bool done = false;
    bool boundary = false;
    float opacity;
    unsigned long now = GetTickCount();

    if (g_surprise_fade_tick_base_0068edb8 + 500 < now) {
        if (g_surprise_fade_in_0068edca == 0) {
            opacity = 0.0f;
            done = true;
        } else {
            boundary = true;
            opacity = 1.0f;
            g_surprise_fade_in_0068edca = 0;
            g_surprise_fade_tick_base_0068edb8 = now;
        }
    } else {
        opacity = (now - g_surprise_fade_tick_base_0068edb8) * g_float_005ebc60;
        if (g_surprise_fade_in_0068edca == 0) {
            opacity = g_float_005ebb38 - opacity;
        }
    }

    static_cast<srMaterial*>(static_cast<srMeshModel*>(g_surprise_fade_node_0068edf8->model())
                                 ->getMaterial(0, static_cast<srMeshModel::e_side>(0)))
        ->setOpacity(opacity);

    if (boundary) {
        if (gXStatus.surprise_phase == 0) {
            long pitch;
            void* pixels = Function5498A0(0x1e0, 0, &pitch);
            srColorSurface* surface = SR_NEW(srColorSurface)(srPixelConvert::SURFACE_ARGB1555,
                                                             pixels, 0x280, 0x1e0, pitch);
            g_surprise_snapshot_surface_0068edf0 = surface;
            g_surprise_snapshot_overlay_0068edf4 =
                SR_NEW(stSurface2D)(g_surprise_snapshot_surface_0068edf0, 0x280, 0x1e0,
                                    g_scene_fullscreen_659644, 0x80);
            g_surprise_snapshot_overlay_0068edf4->updateRectangle(g_gerd_659634, pixels, pitch, 0,
                                                                  0, 0x280, 0x1e0);
            Function549950(0x1e0, 0);
            g_flag_65970d = 0;
            g_surprise_fade_tick_base_0068edb8 = GetTickCount();
        } else {
            g_surprise_snapshot_overlay_0068edf4->release();
            g_surprise_snapshot_overlay_0068edf4 = 0;
            g_surprise_snapshot_surface_0068edf0->release();
            g_surprise_snapshot_surface_0068edf0 = 0;
            g_flag_65970d = 1;
        }
        return done;
    }
    if (done != 0) {
        if (gXStatus.surprise_phase == 0) {
            g_surprise_fade_node_0068edf8->setFlag(srNode::FLAG_DISABLE);
            return done;
        }
        g_surprise_fade_node_0068edf8->release();
        g_surprise_fade_node_0068edf8 = 0;
        SetFlag603C4C(1);
    }
    return done;
}
