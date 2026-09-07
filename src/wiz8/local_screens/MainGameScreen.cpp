#include "wiz8/combat_state.h"
#include "wiz8/local_screens/MainGameScreen.h"
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
#include "mousesystem.h"
#include "surrender/srTypeRegistry.h"

#include <stdlib.h>
#include <string.h>

/*
 * Local Screens\MainGameScreen.cpp.
 *
 * The screen the game is played on. Nothing here draws: everything that
 * changes what the screen shows ORs a bit into the level runtime block's
 * redraw word and lets the frame pick it up, which is why so much of the rest
 * of the game calls into this file.
 */

/* Every redraw request checks the screen state first, so a request made from
   another screen is simply dropped. */

/* The region set the two enable/disable wrappers below own. */
enum { W8_REGION_SET_MAIN = 4 };
extern void* g_modal_owner_0068edd0;
extern int g_flag_0068ed14;
unsigned char g_flag_006840bd;
W8LevelRuntimeBlock* g_level_block;

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
extern void Function422F10(void);
extern float Function420B40(int value);
extern void Function482EA0(void);
extern void Function482990(unsigned char enabled);
extern void Function425570(int enabled);
extern void Function58AC00(int, const wchar_t*, int, int, int);
extern void Function58AAD0(int, const wchar_t*, const wchar_t*);
extern void Function55D3C0(void);
extern "C" void ClearHeldItemDisplay(void);
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

extern unsigned char g_flag_0065970d;
extern unsigned char g_flag_0065970c;
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
extern int g_value_006f04ec;

struct W8MainGameResourceSlot {
    srClass* object;
    unsigned char positional_04[0x10];
};
extern W8MainGameResourceSlot g_main_game_resource_slots_64827c[17];

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
unsigned char MainGameScreenEnter0055F8C0(void)
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
    g_flag_0065970d = 1;
    g_flag_0065970c = 1;
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
    Function422F10();
    Function420B40(4);
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

/* Leave the live screen. A temporary transition keeps the allocation and the
   resource strip alive; a full leave additionally unloads the level and owns
   the final release of the per-screen block. */
// FUNCTION: WIZ8 0x00560660
unsigned char MainGameScreenLeave00560660(int leaving)
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
        g_value_006f04ec = 0;
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
    Function420B40(1);
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
        if (g_screen_state_0068ec78.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
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
        if (g_screen_state_0068ec78.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
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
        if (g_screen_state_0068ec78.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
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
    g_flag_0065970d = 0;
    g_flag_0065970c = 0;
    DisableSky();
    Function598AE0();
    Function5AEB20();
    return 1;
}

extern void SetPendingScreenState(int state);
extern unsigned char Function577850(void);
extern void SetCombatSelection(int value);                              /* 0x00569F70 */
extern void SetCombatTarget(int value);                                 /* 0x0056A2D0 */
extern void SetCombatAction(int value);                                 /* 0x0056A480 */
extern int Function53A3D0(int arg_1);
extern void Function55EE70(int arg_1);

/* Ask for part of the screen to be redrawn. A request made while another
   screen is up, or before the level block exists, is dropped rather than
   queued - which is what makes the block the only place redraw state lives. */
// FUNCTION: WIZ8 0x00562a50
void RequestRedraw(unsigned int mask)
{
    if (g_screen_state_0068ec78.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
        g_level_block->redraw_flags |= mask;
    }
}

/* Two callers of that with a fixed bit each, written out rather than
   forwarding - which is what shows the mask is a compile-time constant at
   every one of its callers. */
// FUNCTION: WIZ8 0x00565420
void RequestRedrawParty(void)
{
    if (g_screen_state_0068ec78.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
        g_level_block->redraw_flags |= 0x20000;
    }
}

// FUNCTION: WIZ8 0x005699b0
void RequestRedrawCombatBar(void)
{
    if (g_screen_state_0068ec78.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
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
void OpenModal(void* owner)
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
    g_flag_0068ed14 = 0;
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
    if (g_screen_state_0068ec78.id == W8_SCREEN_MAIN_GAME && g_level_block != 0 &&
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
