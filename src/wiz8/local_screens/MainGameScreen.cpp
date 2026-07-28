#include "wiz8/gameplay_boundaries.h"

/*
 * Local Screens\MainGameScreen.cpp.
 *
 * The screen the game is played on. Nothing here draws: everything that
 * changes what the screen shows ORs a bit into the level runtime block's
 * redraw word and lets the frame pick it up, which is why so much of the rest
 * of the game calls into this file.
 */

/* The screen state the main game screen owns. Every redraw request checks it
   first, so a request made from another screen is simply dropped. */
enum { W8_SCREEN_MAIN_GAME = 7 };

/* The region set the two enable/disable wrappers below own. */
enum { W8_REGION_SET_MAIN = 4 };

extern int g_screen_state_0068ec78;
extern W8LevelRuntimeBlock* g_level_block;
extern void* g_modal_owner_0068edd0;
extern int g_flag_0068ed14;
extern unsigned char g_flag_00683f94;
extern unsigned char g_flag_00683f95;
extern unsigned char g_flag_00683f96;
extern unsigned char g_flag_00683f97;
extern unsigned char g_flag_00683f98;
extern unsigned char g_flag_00683f99;

extern void Function55EC50(int arg_1);
extern void Function4F2040(int region);
extern void Function429770(void);
extern unsigned char Function577850(void);
extern void SetCombatSelection(int value);                              /* 0x00569F70 */
extern void SetCombatTarget(int value);                                 /* 0x0056A2D0 */
extern void SetCombatAction(int value);                                 /* 0x0056A480 */
extern int Function53A3D0(int arg_1);
extern void Function55EE70(int arg_1);

/* Ask for part of the screen to be redrawn. A request made while another
   screen is up, or before the level block exists, is dropped rather than
   queued - which is what makes the block the only place redraw state lives. */
// FUNCTION: WIZ8 0x00562A50
void RequestRedraw(unsigned int mask)
{
    if (g_screen_state_0068ec78 == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
        g_level_block->redraw_flags |= mask;
    }
}

/* Two callers of that with a fixed bit each, written out rather than
   forwarding - which is what shows the mask is a compile-time constant at
   every one of its callers. */
// FUNCTION: WIZ8 0x00565420
void RequestRedrawParty(void)
{
    if (g_screen_state_0068ec78 == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
        g_level_block->redraw_flags |= 0x20000;
    }
}

// FUNCTION: WIZ8 0x005699B0
void RequestRedrawCombatBar(void)
{
    if (g_screen_state_0068ec78 == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
        g_level_block->redraw_flags |= 0x100;
    }
}

/* Note that the party's state changed. The combat half is only asked for while
   a fight is on; the party half always. */
// FUNCTION: WIZ8 0x005653F0
void RequestRefreshPartyState(void)
{
    if (g_level_block == 0) {
        return;
    }
    if (g_flag_00683f94 != 0) {
        g_level_block->refresh_combat_panel = 1;
    }
    g_level_block->refresh_party_panel = 1;
}

/* Whether a modal owner has the screen. */
// FUNCTION: WIZ8 0x0056AA20
bool IsModalOpen(void)
{
    return g_modal_owner_0068edd0 != 0;
}

/* Take the screen for a modal owner and put its region up. */
// FUNCTION: WIZ8 0x005698A0
void OpenModal(void* owner)
{
    g_modal_owner_0068edd0 = owner;
    Function4F2040(0x138);
}

/* Put the main region set up, and take it down again with its mode reset -
   the two are not symmetric, which is what the extra call shows. */
// FUNCTION: WIZ8 0x00561FA0
void EnableMainRegionSet(void)
{
    RegionSetEnable(W8_REGION_SET_MAIN);
}

// FUNCTION: WIZ8 0x00561FB0
void DisableMainRegionSet(void)
{
    RegionSetDisable(W8_REGION_SET_MAIN);
    SetRegionSetMode4(W8_REGION_SET_MAIN);
}

/* Clear whatever the screen was waiting on and hand the tenth reason to the
   frame. */
// FUNCTION: WIZ8 0x00565970
void ClearScreenWait(void)
{
    g_flag_0068ed14 = 0;
    Function55EC50(10);
}

/* Forget the whole combat selection - what is picked, what it is aimed at, and
   what is going to be done - and then re-derive who is acting. */
// FUNCTION: WIZ8 0x0056A5A0
void ClearCombatSelection(void)
{
    SetCombatSelection(-1);
    SetCombatTarget(-1);
    SetCombatAction(-1);
    Function55EE70(Function53A3D0(0));
}

/* Drop the highlight when the thing being highlighted is the one going away. */
// FUNCTION: WIZ8 0x0056A2A0
void ClearHighlightIfItIs(const int* item)
{
    if (g_screen_state_0068ec78 == W8_SCREEN_MAIN_GAME && g_level_block != 0 &&
        g_level_block->highlighted_item != -1 &&
        *item == g_level_block->highlighted_item) {
        Function429770();
    }
}

/* Whether the screen is in one of the states that takes the player's input
   away. The first flag settles it outright; otherwise one state only counts
   while a further check disagrees, and three more count on their own. */
// FUNCTION: WIZ8 0x00562540
int IsScreenInputBlocked(void)
{
    if (g_flag_00683f95 != 0) {
        return 1;
    }
    if (g_flag_00683f97 != 0 && !Function577850()) {
        return 1;
    }
    if (g_flag_00683f98 == 0 && g_flag_00683f99 == 0 && g_flag_00683f96 == 0) {
        return 0;
    }
    return 1;
}
