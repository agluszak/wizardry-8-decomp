#include "wiz8/combat_state.h"
#include "wiz8/game_state.h"
#include "wiz8/regions.h"
#include "wiz8/screen_state.h"
#include "wiz8/xstatus.h"
#include "wiz8/wiz8_windows.h"

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

extern void SetPendingScreenState(int state);
extern void Function4F2040(int region);
extern void ReleaseScreenTransitionObjects(void);
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
    if (g_in_combat_00683f94 != 0) {
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
    Function4F2040(0x138);
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
    SetRegionSetMode4(W8_REGION_SET_MAIN);
}

/* Clear whatever the screen was waiting on and hand the tenth reason to the
   frame. */
// FUNCTION: WIZ8 0x00565970
void ClearScreenWait(void)
{
    g_flag_0068ed14 = 0;
    SetPendingScreenState(10);
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

/* The screen object at 0x0068F2D4. Only the members these bodies reach are
   established: two panels, the object the region events go to, and the
   pending target the Knock Knock cast records. */
class W8MainGameScreenPanel {
public:
    virtual ~W8MainGameScreenPanel();
    virtual void Redraw(int full_redraw);

    unsigned char unknown_004[0x70];
    /* 0x074 and 0x078: the key handler and the selection it moves. */
    W8MainGameScreenPanel* key_target;
    int selection;
    unsigned char unknown_07c[0xc4];
    unsigned char flag_140;              /* 0x140 */
};

extern unsigned char g_map_loading_00659757;
extern void Function55EE70(int reason);
extern void UpdateHeldItemCursor(void);
extern void Function42B3E0(void);
extern unsigned char Function42ACE0(const char* path);
extern void SetRegionMode4(unsigned int region);
extern void Function5879A0(int arg_1);
extern void Function58A790(int arg_1);
extern void Function59F2B0(void);
extern void Function59CAC0(void);
extern void Function5B2200(void);
extern unsigned int DispatchScreenInput004F1910(const void* event);
extern void Function558810(void);
extern void Function558720(int arg_1);
extern void ShowNotice(int channel, const void* text, int a, int b, int c);

/* Whether the screen is idle - none of the six overlays is up. The same six
   flags the input block reads, but all of them and unconditionally. */
// FUNCTION: WIZ8 0x00561440
int IsScreenIdle(void)
{
    if (g_in_combat_00683f94 == 0 && g_flag_00683f95 == 0 && g_flag_00683f96 == 0 &&
        g_flag_00683f97 == 0 && g_flag_00683f98 == 0 && g_flag_00683f99 == 0) {
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

    if (g_current_level != -1) {
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
    SetRegionMode4(0x52);
    SetRegionMode4(0x53);
    SetRegionMode4(0x54);
    SetRegionMode4(0x55);
    SetRegionMode4(0x56);
    SetRegionMode4(0x57);
    SetRegionMode4(0x58);
    if (g_level_block->flag_155 == 0) {
        SetRegionMode4(0x59);
        RegionSetDisable(0x14);
    }
}

/* Hand one frame to whichever overlays are up. Each is independent, so more
   than one can take the same frame. */
// FUNCTION: WIZ8 0x0056af20
void UpdateScreenOverlays(int frame)
{
    if (g_flag_00683f98 != 0) {
        Function5879A0(frame);
    }
    if (g_flag_00683f99 != 0) {
        Function58A790(frame);
    }
    if (g_flag_00683f95 != 0) {
        Function59F2B0();
    }
    if (g_flag_00683f96 != 0) {
        Function59CAC0();
    }
    if (g_flag_00683f9a != 0) {
        Function5B2200();
    }
}

/* Which party portrait the pointer is over, if any. The slots are walked
   against two runs of region numbers at once - one starting at 0x24 six apart
   and one at 0x5a one apart - and only two event kinds are answered. */
// FUNCTION: WIZ8 0x00569c00
unsigned int HitTestPartyPortrait(const void* event)
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
        return DispatchScreenInput004F1910(event);
    }
    return 0;
}
