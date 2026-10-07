#pragma once

#include "wiz8/character_skills.h"

#include "timer.h"
#include "input.h"

/* Local Screens\MGSButtons.cpp. The combat sub-menu's panel, its five text
   rows and the two scroll-arrow buttons: construction, teardown and the
   per-frame name/action caption draw. The shared W8SubMenu enums and the
   portrait-combat unit's const tables live in MGSPortraitCombat.h. */

#include "wiz8/local_screens/MGSPortraitCombat.h"

struct Controls;
struct W8Region;
class W8TextControl;
class W8DialogButton;

/* Owned globals. */
extern Controls* gpSubMenuPanel;
extern W8DialogButton* g_submenu_scroll_buttons[2];
extern W8DialogButton* g_submenu_panel_buttons[2];
extern W8DialogButton* g_layout_arrow_buttons[6];
extern W8DialogButton* g_combat_stance_buttons[5];
extern W8DialogButton* g_roof_buttons[3];
extern W8DialogButton* g_options_disk_button;
extern short g_submenu_entry_count;
extern TIMER g_submenu_clock;
extern bool g_submenu_close_pending;
/* The cancel row lands at [built-1] where built is the available-entry count
   plus one, so five rows only suffice because Berserk (fighter trait 0x14) and
   Pray (priest trait 0x0b) can never be available together. */
extern W8TextControl* g_submenu_rows[5];
extern W8DialogButton* g_submenu_buttons[9];
/* The action-kind message indexes the caption draw maps through. */
extern unsigned short g_action_kind_message_ids[12];
/* Which menu the open sub-menu panel serves (W8SubMenuPage value), each row's
   entry index and each row's W8SubMenuEntryState, stored as shorts. */
extern short g_submenu_menu;
extern short g_submenu_entries[6];
extern short g_submenu_entry_states[5];

/* Invalidate (when asked) then redraw the sub-menu panel. */
void RefreshSubMenuPanel(bool invalidate);
/* Close and rebuild the sub menu around the saved combat-end notification. */
void ResetSubMenuPanel(void);
/* Create and lay out the two scroll-arrow buttons. */
unsigned char CreateSubMenuScrollButtons(void);
/* Create the close/formation panel buttons. */
unsigned char CreateSubMenuPanelButtons(void);
/* Create the options-disk button. */
unsigned char CreateOptionsDiskButton(void);
/* Create the five combat-stance buttons. */
unsigned char CreateCombatStanceButtons(void);
/* Create the three roof/viewpoint buttons. */
unsigned char CreateRoofButtons(void);
/* Draw roof chrome, mark the three buttons dirty, sync state, and request the
   0x100000 redraw bit. */
void RedrawRoofButtons(void);
/* Sync roof-button visibility/enabled/pressed state and Draw each button. */
void UpdateRoofButtons(void);
/* Press the roof button that matches g_settings.main_ui_mode. */
void SyncRoofButtonPressedState(void);
/* Draw layout-arrow chrome and sync the six raise/lower arrows against the
   live action-panel / formation-board / radar-map visibility flags. */
void RedrawLayoutArrowButtons(void);
/* Create the six layout-arrow buttons. */
unsigned char CreateLayoutArrowButtons(void);
/* MainGameScreenEnter's interface-button bank: submenu, scroll, panel,
   options disk, combat stance, roof, and layout arrows. */
void CreateMainGameInterfaceButtons(void);
void DestroyMainGameInterfaceButtons(void);
/* Draw the selected character's name/profession line and the caption for
   their queued action. */
void DrawSubMenuCharacterAction(void);
/* Sync enable/press of the two sub-menu panel buttons, then Draw them. */
void UpdateSubMenuPanelButtons(void);
/* Redraw the bottom sub-menu chrome, bank buttons, panel buttons, character
   action caption, and scroll arrows (redraw bit 0x1000). */
void RedrawSubMenuButtons(void);
/* Redraw the options-disk button (redraw bit 0x40000). */
void RedrawOptionsDiskButton(void);
/* Mark combat-stance buttons dirty and refresh which stance is shown
   (redraw bit 0x80000). */
void RedrawCombatStanceButtons(void);
/* Show the active continuous/manual combat-stance button for the live mode. */
void UpdateCombatStanceButtons(void);
/* Per-frame button refresh from DrawMainGameScreen: bank buttons, scroll
   arrows, panel/options/stance/roof buttons and the layout-arrow pair. */
void UpdateMainGameButtons(void);
/* While the combat-end submenu is up and the cursor has left its row band,
   run a 500 ms countdown, then tear the panel and rows down. */
void UpdateSubMenuAutoClose(void);

/* The scroll-arrow callbacks are one-argument thunks over the shared handler:
   up steps toward lower slots, down toward higher, both wrapping and skipping
   ineligible slots. */
void SubMenuScrollArrowUp(W8DialogButton* button);
void SubMenuScrollArrowDown(W8DialogButton* button);
/* Move selected_character to the next eligible party slot (0 steps toward
   slot 0, 1 toward slot 7), wrapping, and raise the portrait redraw masks. */
void ScrollSubMenuCharacter(char direction);

/* The per-row select callbacks AssignSubMenuCallback installs. */
void SubMenuSelectAttack(void);
void SubMenuSelectBerserk(void);
void SubMenuSelectBreathe(void);
void SubMenuSelectTurnUndead(void);
void SubMenuSelectPray(void);
void SubMenuSelectDefend(void);
void SubMenuSelectProtect(void);
void SubMenuSelectEquip(void);
void SubMenuOpenUseItemView(void);
void SubMenuUseRecordedItem(void);
void SubMenuOpenSpellView(void);
void SubMenuCastRecordedSpell(void);
void SubMenuSelectRun(void);
void SubMenuSelectWalk(void);

/* Applies SetTooltipEnabled to all nine bank buttons, both scroll arrows and
   both panel buttons; the submenu rebuild paths call it around teardown. */
void SetSubMenuButtonTooltips(int enabled);
/* Region callback for the sub-menu background: right-up tears the panel down. */
/* Enable/disable the scroll, roof and layout button
   banks around the surprise/camp sequence. */
void EnableMenuButtonBanks(void);
void DisableMenuButtonBanks(void);
unsigned char SubMenuBackgroundRegionEvent(const InputAtom* event, W8Region* region);
/* Region callback the five sub-menu rows share. */
unsigned char SubMenuRowRegionEvent(const InputAtom* event, W8Region* region);

/* Enable the panel region set and one input region per live row. */
void EnableSubMenuRegions(void);
/* Descriptive name for the complete submenu opening operation expanded in
   the button callbacks and reopen/reset paths. */
void OpenSubMenuPanel(short notification);
/* Build the panel and one row per available entry of the notification's
   menu. */
unsigned char BuildSubMenuPanel(short notification);
/* Install the row's primary callback for its (W8SubMenuPage, entry) pair. */
void AssignSubMenuCallback(W8TextControl* row, short menu, short item);
/* The row availability states the refresh maps icon frames through. */
W8SubMenuEntryState GetSubMenuEntryState(short menu, short item, int party_slot);
/* Store the (W8SubMenuPage, entry) pair's pending command in the level block. */
void MapSubMenuSelection(short menu, short item);
/* Whether the slot may perform the pending command, in entry-state terms:
   USABLE/UNUSABLE or the _SELECTED variant when it is already queued. */
W8SubMenuEntryState CheckSubMenuActionUsable(int party_slot);

inline int GetAttackMenuWeaponOffset(W8Skill skill)
{
    int base;
    switch (skill) {
    case W8_SKILL_MACE_FLAIL:
        base = 3;
        break;
    case W8_SKILL_AXE:
        base = 4;
        break;
    case W8_SKILL_POLEARM:
        base = 7;
        break;
    case W8_SKILL_STAFF_WAND:
        base = 6;
        break;
    case W8_SKILL_BOW:
        base = 1;
        break;
    case W8_SKILL_THROWING_SLING:
        base = 2;
        break;
    case W8_SKILL_MODERN_WEAPONS:
        base = 8;
        break;
    case W8_SKILL_MARTIAL_ARTS:
        base = 5;
        break;
    default:
        base = 0;
        break;
    }
    return base;
}
