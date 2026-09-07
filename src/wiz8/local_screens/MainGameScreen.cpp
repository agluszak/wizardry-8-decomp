#include "wiz8/combat_state.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/render_state.h"
#include "wiz8/sgp_video.h"
#include "wiz8/local_code/LoadSaveGame.h"
#include "wiz8/dialog_code/DialogInterface.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/cursor.h"
#include "wiz8/engine_code/Octree.h"
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
#include "wiz8/local_code/GameplayDatabase.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/engine_code/Environment.h"
#include "wiz8/local_screens/MGSUseItemSelect.h"
#include "wiz8/xstatus.h"
#include "wiz8/wiz8_windows.h"

#include "font.h"
#include "FileMan.h"
#include "input.h"
#include "timer.h"
#include "mousesystem.h"
#include "surrender/srTypeRegistry.h"

#include <stdlib.h>
#include <string.h>

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

extern unsigned char ClearPrimarySurface(void);
extern unsigned char TakePendingSaveFlag(void);
extern unsigned char IsPartySlotEligible00524A10(int slot);
extern void ResetRegions(void);
extern void TurnPartyToImmediate(unsigned int facing, char update_saved);
extern void ResetTargetingState(void);
extern void Function568E10(void);
extern void Function598AB0(void);
extern void Function5AE9D0(void);
extern void Function59B940(void);
extern void Function59BDB0(void);
extern void Function55F2C0(void);
extern void ScrollTextBoxToCursor(void);
extern void Function413FD0(int, int, int, int, int);
extern void ResetTransientRenderScenes(void);
extern void Function482EA0(void);
extern void Function482990(unsigned char enabled);
extern void Function425570(int enabled);
extern void Function58AC00(int, const wchar_t*, int, int, int);
extern void Function58AAD0(int, const wchar_t*, const wchar_t*);
extern void Function55D3C0(void);
extern void Function55F160(int value);
extern void Function53A320(int value);
extern void Function587510(int value);
extern void Function58A470(int value);
extern void Function565740(int slot);
extern void Function59C930(int slot);
extern void Function42B770(int, int);
extern void Function4098F0(void);
extern void Function490AF0(void);
extern char Function4914C0(void);
extern void Function5187E0(void);
extern void Function56E800(int);
extern void Function57D740(void);
extern void Function5879A0(int);
extern void Function58A790(int);
extern void Function592E60(void);
extern void Function598AE0(void);
extern void Function59B270(void);
extern void Function59BAD0(void);
extern void Function59BF70(void);
extern void Function59C9C0(void);
extern void Function59F2B0(void);
extern void Function5A20E0(int);
extern void Function5A23E0(void);
extern void Function5AEB20(void);
extern void Function5B1C00(void);
extern void Function5B2200(void);
extern void Function563DD0(void);
extern void Function4257F0(int value);
extern void Function529510(void);
extern short Function5698C0(void);
extern void Function5618F0(unsigned short mode);
extern unsigned char SetFlag603C60(void);
extern unsigned char GetFlag68F105(void);
extern void DisableRegionSet1C(void);
extern void ReleaseLoadedVideoFrames(void);
extern void MSYS_Shutdown(void);
extern void NoOp(void);
extern void UpdateHeldItemCursor(void);
extern void Function42B3E0(void);
int IsScreenInputBlocked(void);
void DisableCombatRegions(void);

extern unsigned char g_flag_006840bc;
extern unsigned short g_value_006840be;
extern int g_held_item_source_006840c0;
extern unsigned char g_held_item_origin_006840c4;
extern unsigned short g_held_item_slot_006840c5;
extern unsigned char g_flag_00685070;
extern unsigned char g_flag_00685071;
extern int g_value_00685072;
extern unsigned char g_flag_00685076;
extern signed char g_value_00685077;
extern int g_value_006850d5;
extern unsigned char g_in_combat_00683f94;
extern unsigned char g_flag_00683f95;
extern unsigned char g_flag_00683f96;
extern unsigned char g_flag_00683f97;
extern unsigned char g_flag_00683f98;
extern unsigned char g_flag_00683f99;
extern unsigned char g_flag_00683f9a;
extern unsigned char g_flag_00683fcd;
extern unsigned char g_flag_006850ce;
extern unsigned char g_flag_0068edbc;
extern unsigned char g_flag_0068edc8;
extern unsigned char g_flag_0068edc9;
extern unsigned char g_flag_0068edd8;
extern int g_main_game_mode_0068eddc;

struct W8MainGameResourceSlot {
    srClass* object;
    unsigned int frame_count;
    int field_08;
    int field_0c;
    int image_id;
};
// GLOBAL: WIZ8 0x0064827c
W8MainGameResourceSlot g_main_game_resource_slots_64827c[17] = {
    {0, 1, 0, 0, 2}, {0, 5, 0, 0, 3}, {0, 1, 0, 0, 4},
    {0, 1, 0, 0, 5}, {0, 1, 0, 0, 6}, {0, 4, 0, 0, 7},
    {0, 4, 0, 0, 0}, {0, 0, 0, 0, 8}, {0, 1, 0, 0, 9},
    {0, 1, 0, 0, 10}, {0, 5, 0, 0, 11}, {0, 1, 0, 0, 12},
    {0, 1, 0, 0, 13}, {0, 4, 0, 0, 15}, {0, 1, 0, 0, 14},
    {0, 1, 0, 0, 16}, {0, 1, 0, 0, 0}
};

extern unsigned char g_build_level_links_0065bd2c;
extern unsigned char g_flag_689b32;
extern int g_next_link_level_0068ede8;
extern unsigned char g_flag_0068edd9;
extern unsigned char g_byte_00659a64;
extern unsigned char g_level_runtime_flag_0065ba70;
extern unsigned char g_debug_monster_cycle_0068f0fc;
extern W8IList* g_debug_monster_ids_0068f100;
extern unsigned char g_navigator_position_changed_659c11;
extern unsigned char g_flag_006840bb;

void Function4314C0(int save);
void Function5615F0(int level, int entry, int flag);
void Function568C40(void);
void Function569CC0(void);
void Function5A6970(void);
unsigned char Function5A6790(void);
void Function5A68C0(void);
void Function50B3B0(int value);
void Function577220(void);
unsigned char Function554540(void);
void Function5542E0(void);
void Function4EDD20(void);
void Function59A3A0(void);
void MonsterForward453160(void);
void Function41F0D0(void);
void Function575C50(void);
void Function577560(void);
void Function52DDD0(void);
int Function52E750(void);
void Function59B1A0(void);
void Function59B4C0(void);
void Function59B390(void);
void Function502650(void);
void Function562A80(void);
void Function515B00(void);
void Function41F1F0(void);
unsigned char Function525DF0(unsigned char require_group_entry);
void Function4916C0(void);
unsigned char Function5684E0(void);
void Function561330(unsigned char value);
unsigned char GetFlag69DA6C(void);
unsigned char GetFlag68F104(void);
void Function5929D0(void);
unsigned char Function57E490(void);
void Function592A10(void);
void Function57E0E0(int event, const W8ScreenPoint* point);
void Function44FC20(W8World* world, unsigned int flags);
void Function450210(W8World* world, unsigned int flags);
void Function59B2D0(void);
void Function55F080(void);
void Function5A1EB0(W8ScreenPoint* point, unsigned int* value);
void Function5171C0(void);
void Function4E8EA0(void);
unsigned char IsSightRangeOverridden(void);
void StartCombat(int surprise);
void Function530110(void);
void Function530150(int value);
void Function5398D0(void);
void Function53B310(void);
void Function53B1D0(void);
void UpdateAllMonsterHighlights(int character, int item);
void Function4F7480(void);
void Function5A0BC0(void);
void Function59D180(void);
void Function56E510(void);
void Function587960(void);
void Function58A750(void);
unsigned char Function445140(W8World* world);
unsigned char Function53A1D0(void);
unsigned char Function4F8650(void);
unsigned char Function57E3C0(void);
unsigned char Function48EFC0(void);
void Function427830(int enabled);
void Function4EF1F0(void);

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
        gXStatus.field_01d = 0;
        gXStatus.field_01f = 0;
        gXStatus.fItemSelectMode = 0;
        gXStatus.field_020 = 0;
        gXStatus.field_021 = 0;
        gXStatus.field_022 = 0;
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
    int& saved_display_mode =
        *reinterpret_cast<int*>(&g_status_685170.status_header_block_1904[0xb43]);

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
    Function55F2C0();
    ScrollTextBoxToCursor();
    Function413FD0(0x500, 0, 0, 0x280, 0x1e0);
    SetFontDestBuffer(-14, 0, 0, 0x280, 0x1e0, 0);
    g_flag_65970d = 1;
    g_monster_shadow_updates_enabled_0065970c = 1;
    ClearPrimarySurface();
    if (IsFogEnabled()) {
        Function482EA0();
    }
    else {
        DisableSky();
    }
    if (TakePendingSaveFlag()) {
        Function58AC00(0xc, gppStringList[0x1e08 / 4], -1, -1, 0);
    }
    if (g_value_006850d5 != saved_display_mode) {
        g_value_006850d5 = saved_display_mode;
        switch (saved_display_mode) {
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
        Function58AAD0(0xc, gppStringList[0x1e30 / 4],
                       gppStringList[display_mode]);
    }
    ResetTransientRenderScenes();
    MoveTimer(4);
    if (!g_flag_006840bc && !g_in_combat_00683f94) {
        Function482990(1);
    }
    {
        W8GameTimer* timer = g_gameplay_timer_685067;
        if ((timer->m_flags & 8) != 0 ||
            (g_shared_timer_paused && (timer->m_flags & 1) == 0) ||
            g_shared_timer_flag_d1) {
            timer->m_flags &= ~8;
            timer->m_start = timer->Method00439A60() - timer->m_start;
            timer->SetDuration(-1.0f);
        }
    }
    Function55D3C0();
    if (g_status_685170.item_in_hand_235b.item_id != -1) {
        Function55F160(0);
    }
    else {
        ClearHeldItemDisplay();
    }
    Function53A320(0);
    Function425570(1);
    if (gXStatus.field_024) {
        Function587510(0);
    }
    if (gXStatus.field_025) {
        Function58A470(0);
    }
    if (g_flag_00685071) {
        if (IsPartySlotEligible00524A10(g_value_00685077)) {
            Function565740(g_value_00685077);
            Function59C930(g_value_00685077);
            SelectCurrentUseItemLine0059E0E0();
        }
        else {
            g_flag_00685071 = 0;
            g_value_00685072 = 0;
            g_flag_00685076 = 0xff;
            g_value_00685077 = -1;
        }
    }
    if (!g_in_combat_00683f94) {
        Function42B770(1, 1);
    }
    return 1;
}

/* The active screen's frame, including modal input and pending transitions. */
// FUNCTION: WIZ8 0x0055fb30
void MainGameScreenFrame(void)
{
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
                    Function5615F0(level, -1, 0);
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
    }
    else if (Function5A6790()) {
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
    if (!Function554540()) {
        Function5542E0();
    }
    if (!g_level_block->transition_active && !gXStatus.fCombatMode && gXStatus.field_028) {
        Function4EDD20();
    }
    Function59A3A0();
    if (g_modal_owner_0068edd0) {
        if (!g_flag_006840bc) {
            g_flag_006840bc = 1;
            if (gXStatus.field_055) {
                DisableRegionSet1C();
            }
            if (!gXStatus.fCombatMode) {
                if (g_flag_006840bd) {
                    MoveTimer(1);
                    EnableRegionInput(0x137);
                    ActivateDialogRegion(0x137);
                }
                Function482990(0);
                MonsterForward453160();
                Function41F0D0();
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
            Function56AAB0();
        }
    }
    Function575C50();
    NoOp();
    Function577560();
    Function52DDD0();
    Function52E750();
    Function59B1A0();
    if (gXStatus.fCombatMode) {
        Function59B4C0();
    }
    g_status_685170.unknown_238b[5] = 0;
    if (g_level_block->flag_314 || g_level_block->combat_slot != -1) {
        Function59B390();
    }
    Function502650();
    if (gXStatus.fCombatMode) {
        for (int slot = 0; slot < 8; ++slot) {
            if (!g_party_slot_rows[slot].occupied ||
                g_party_characters[slot].unknown_0b01 > 0x11 ||
                (g_level_block->flag_314 && g_level_block->combat_slot == slot)) {
                DisableRegionInput(slot + 10);
            }
            else {
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
    Function41F1F0();
    g_byte_00659a64 = 0;
    WorldUpdateProps(GetWorld());
    if (GetWorld659AB8()) {
        WorldUpdateProps(GetWorld659AB8());
    }
    W8ScreenPoint point;
    W8ScreenPoint current;
    unsigned int value;
    GetScreenPoint004284F0(&point);
    if (!Function4914C0()) {
        if (!g_modal_owner_0068edd0) {
            if ((!Function525DF0(1) || !gXStatus.field_01f) &&
                !g_status_685170.unknown_2429[12]) {
                g_level_block->hover_region = UpdateRegionMousePosition(point.x, point.y);
            }
            else {
                g_level_block->hover_region = FindRegionAtPoint(
                    static_cast<unsigned short>(point.x), static_cast<unsigned short>(point.y));
            }
        }
        else {
            g_level_block->hover_region = FindRegionAtPoint(
                static_cast<unsigned short>(point.x), static_cast<unsigned short>(point.y));
            for (int portrait = 0; portrait < 8; ++portrait) {
                if (g_party_slot_rows[portrait].occupied &&
                    (g_level_block->hover_region == portrait * 6 + 0x24U ||
                     g_level_block->hover_region == portrait + 0x5aU)) {
                    g_level_block->hover_region = UpdateRegionMousePosition(point.x, point.y);
                    break;
                }
            }
        }
    }
    else {
        Function4916C0();
    }
    if (!g_level_block->flag_314 && g_level_block->hover_combat_slot != -1 &&
        g_level_block->hover_region != g_level_block->hover_combat_slot + 10U) {
        g_level_block->hover_combat_slot = -1;
    }
    Function561330(Function5684E0());
    if (!GetFlag69DA6C()) {
        if (!GetFlag68F105() || GetFlag68F104()) {
            Function5929D0();
        }
        else if (Function57E490()) {
            Function592A10();
        }
    }
    if (!g_level_runtime_flag_0065ba70) {
        if (g_flag_0068edd8) {
            if (g_flag_0068edd9) {
                if (!gfKeyState[0x10]) {
                    g_level_block->world_render_flags |= 4;
                }
                else {
                    g_level_block->world_render_flags |= 0x84;
                }
            }
            if (g_flag_0068edd8) {
                goto render_world;
            }
        }
        if (gfLeftButtonState && !g_modal_owner_0068edd0 && GetFlag68F105()) {
            GetScreenPoint004284F0(&current);
            Function57E0E0(0x400, &current);
        }
    }
render_world:
    if (!IsScreenTransitionPending()) {
        if (Function57E490()) {
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
            g_main_game_resource_slots_64827c[gXStatus.iCurrentCursor].frame_count > 1 &&
            !ClockIsTicking(gXStatus.current_cursor_time) &&
            !Function4914C0() && !g_flag_0068edd8) {
            ++gXStatus.current_cursor_frame;
            if (gXStatus.current_cursor_frame ==
                g_main_game_resource_slots_64827c[gXStatus.iCurrentCursor].frame_count) {
                gXStatus.current_cursor_frame = 0;
            }
            Function55F080();
        }
        ProcessMonsterManagerFrame();
        if (g_debug_monster_cycle_0068f0fc) {
            W8Monster* monster = GetMonsterByLocationID(IListGetAt(g_debug_monster_ids_0068f100, 0));
            if (monster) {
                ClearSurfaceRect(0x122, 0x159, 0x226, 0x168);
                SetFont(g_font_683660);
                unsigned char frame = monster->m_pRep->flag_064;
                const char* cycle = g_cycle_names[monster->Query(6)].name;
                unsigned char subcycles = static_cast<unsigned char>(monster->GetNumSubCycles());
                mprintf(0x122, 0x159, (UINT16*)L"%2d/%2d %hs",
                        frame, subcycles, cycle);
            }
        }
        if (!gXStatus.fCombatMode) {
            Function5A1EB0(&current, &value);
            Function5171C0();
        }
        else if (!g_level_block->transition_active && !gXStatus.field_01d &&
                 !gXStatus.field_01f && !gXStatus.fItemSelectMode) {
            Function4E8EA0();
        }
        if (IsSightRangeOverridden() && !gXStatus.fCombatMode &&
            AnyCharacterActive() && gXStatus.field_02d) {
            StartCombat(0);
        }
        if (!ClockIsTicking(g_level_block->character_update_timer)) {
            Function530110();
            g_level_block->character_update_timer = SetCountdownClock(500);
        }
        if (!g_flag_006840bc) {
            Function530150(1);
            if (!gXStatus.fCombatMode && AnyCharacterActive() &&
                gXStatus.field_02d && !gXStatus.field_01f) {
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
        if (gXStatus.field_06f == 4) {
            Function53B310();
        }
        else if (gXStatus.field_06f == 3 && Function4914C0()) {
            Function53B1D0();
        }
        else if (gXStatus.field_06f != 5 && g_level_block->refresh_party_panel) {
            UpdateAllMonsterHighlights(g_status_685170.selected_character,
                                       g_level_block->highlighted_item);
            g_level_block->refresh_party_panel = 0;
        }
        if (!ClockIsTicking(g_level_block->world_update_timer)) {
            Function4F7480();
            DetachAllWorldItems();
            g_level_block->world_update_timer = SetCountdownClock(50);
        }
        if (gXStatus.field_01d) Function5A0BC0();
        if (gXStatus.fItemSelectMode) Function59D180();
        if (gXStatus.field_01f) Function56E510();
        if (gXStatus.field_020) Function587960();
        if (gXStatus.field_021) Function58A750();
        int active;
        if (!Function445140(g_world) && !Function53A1D0() && !Function4F8650() &&
            !Function57E3C0() && !Function48EFC0()) {
            active = 0;
        }
        else {
            active = 1;
        }
        Function427830(active);
        if (gXStatus.unknown_026[1] && !gXStatus.field_01f && !gXStatus.fCombatMode &&
            !g_level_block->transition_active) {
            Function4EF1F0();
        }
        if (g_status_685170.selected_character == -1) {
            int next = GetNextCharacter(1, 1, -1);
            if (next == -1) {
                srAssertFail("iNextChar != BAD_INDEX",
                             "C:\\Projects\\Wizardry 8\\Local Screens\\MainGameScreen.cpp",
                             0x1047, 0);
            }
            Function565740(next);
        }
    }
    Function562A80();
}

/* Suspension retains the allocation and resource strip. A full leave also
   unloads the level and releases the per-screen block. */
// FUNCTION: WIZ8 0x00560660
unsigned char MainGameScreenLeave(int leaving)
{
    int index;

    if (g_main_game_mode_0068eddc == 3) {
        if (g_flag_00683f97) {
            Function56E800(0);
        }
    }
    else if (g_main_game_mode_0068eddc == 5) {
        Function5187E0();
    }
    else if (g_main_game_mode_0068eddc == 6) {
        if (g_level_block->dialogue_owner != 0) {
            Function4257F0(reinterpret_cast<int>(g_level_block->dialogue_owner));
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
    if (Function4914C0()) {
        Function490AF0();
    }
    Function59B270();
    if (g_flag_00683f98) Function5879A0(0);
    if (g_flag_00683f99) Function58A790(0);
    if (g_flag_00683f95) Function59F2B0();
    if (g_flag_00683f96) Function59C9C0();
    if (g_flag_00683f9a) Function5B2200();
    if (g_flag_00683f97) Function56E800(0);
    if (g_level_block->flag_314) Function592E60();
    Function59BAD0();
    Function59BF70();
    if (g_flag_00683fcd) DisableRegionSet1C();
    Function529510();
    if (GetFlag68F105()) Function57D740();
    MoveTimer(1);
    Function482990(0);

    if ((g_gameplay_timer_685067->m_flags & 8) == 0) {
        g_gameplay_timer_685067->m_flags |= 8;
        g_gameplay_timer_685067->m_start =
            g_gameplay_timer_685067->Method00439A60() -
            g_gameplay_timer_685067->m_start;
    }

    if (static_cast<unsigned char>(leaving)) {
        for (index = 0; index < 17; ++index) {
            if (g_main_game_resource_slots_64827c[index].object != 0) {
                g_main_game_resource_slots_64827c[index].object->release();
                g_main_game_resource_slots_64827c[index].object = 0;
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
        Function5B1C00();
        if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
            g_level_block->redraw_flags |= 0x8200;
        }
        if (g_flag_0068edc9) {
            unsigned short mode;
            if (!IsScreenInputBlocked() && !g_level_block->flag_155 &&
                g_level_block->flag_156 && g_level_block->flag_157 &&
                g_flag_006850ce == 0) {
                mode = 4;
            }
            else if (!IsScreenInputBlocked() &&
                     (!g_level_block->flag_156 || !g_level_block->flag_157 ||
                      !g_level_block->flag_155)) {
                mode = 0;
            }
            else if (g_flag_006850ce == 1) {
                mode = 1;
            }
            else if (g_flag_006850ce == 2) {
                mode = 0;
            }
            else {
                mode = 2;
            }
            Function5618F0(mode);
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
            Function5618F0(Function5698C0());
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
            Function5618F0(Function5698C0());
        }
        g_flag_0068edc8 = 0;
    }

    if (static_cast<unsigned char>(leaving)) {
        if (g_status_685170.current_level != -1) {
            Function42B3E0();
            if (!UnloadLevel("")) {
                return 0;
            }
        }
        Function4098F0();
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

extern unsigned char Function577850(void);
extern void SetCombatSelection(int value);                              /* 0x00569F70 */
extern void SetCombatTarget(int value);                                 /* 0x0056A2D0 */
extern void SetCombatAction(int value);                                 /* 0x0056A480 */
extern int Function53A3D0(int arg_1);
extern void Function55EE70(int arg_1);

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
    Function55EE70(Function53A3D0(0));
}

/* Drop the highlight when the thing being highlighted is the one going away. */
// FUNCTION: WIZ8 0x0056a2a0
void ClearHighlightIfItIs(const int* item)
{
    if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0 &&
        g_level_block->highlighted_item != -1 &&
        *item == g_level_block->highlighted_item) {
        ReleaseScreenTransitionObjects();
    }
}

/* Whether the screen is in one of the states that takes the player's input
   away. The first flag settles it outright; otherwise one state only counts
   while a further check disagrees, and three more count on their own. */
// FUNCTION: WIZ8 0x00562540
int IsScreenInputBlocked(void)
{
    if (gXStatus.field_01d != 0) {
        return 1;
    }
    if (gXStatus.field_01f != 0 && !Function577850()) {
        return 1;
    }
    if (gXStatus.field_020 == 0 && gXStatus.field_021 == 0 && gXStatus.fItemSelectMode == 0) {
        return 0;
    }
    return 1;
}

extern unsigned char g_map_loading_00659757;
extern void Function55EE70(int reason);
extern void UpdateHeldItemCursor(void);
extern void Function42B3E0(void);
extern unsigned char Function42ACE0(const char* path);
extern void Function5879A0(int arg_1);
extern void Function58A790(int arg_1);
extern void Function59F2B0(void);
extern void Function59CAC0(void);
extern void Function5B2200(void);

/* Whether the screen is idle - none of the six overlays is up. The same six
   flags the input block reads, but all of them and unconditionally. */
// FUNCTION: WIZ8 0x00561440
int IsScreenIdle(void)
{
    if (gXStatus.fCombatMode == 0 && gXStatus.field_01d == 0 && gXStatus.fItemSelectMode == 0 &&
        gXStatus.field_01f == 0 && gXStatus.field_020 == 0 && gXStatus.field_021 == 0) {
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
        Function55EE70(9);
        g_map_loading_00659757 = 1;
        Function42B3E0();
        loaded = Function42ACE0("MAP") != 0;
        g_map_loading_00659757 = 0;
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
    if (gXStatus.field_020 != 0) {
        Function5879A0(frame);
    }
    if (gXStatus.field_021 != 0) {
        Function58A790(frame);
    }
    if (gXStatus.field_01d != 0) {
        Function59F2B0();
    }
    if (gXStatus.fItemSelectMode != 0) {
        Function59CAC0();
    }
    if (gXStatus.field_022 != 0) {
        Function5B2200();
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

    while (g_party_slot_rows[slot].occupied == 0 ||
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
