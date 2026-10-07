#pragma once

#include "surrender/srMath.h"

/* The portrait combat sub-menu's bank buttons and the shared (menu, entry)
   state tables. */

class W8TextControl;
class W8DialogButton;

/* The five sub-menu pages the bank buttons open; the entry message and help
   tables are indexed page * 5 + entry, and the keyboard menu keys its pages
   the same way. Names follow the entry help captions. */
enum W8SubMenuPage {
    W8_SUBMENU_ATTACK = 0, /* Attack, Berserk, Breathe, Turn Undead, Pray */
    W8_SUBMENU_DEFEND = 1, /* Defend, Protect */
    W8_SUBMENU_ITEMS = 2,  /* Equip, Use Item, Use Last Item */
    W8_SUBMENU_SPELLS = 3, /* Cast Spell, Cast Last Spell */
    W8_SUBMENU_MOVE = 4    /* Walk, Run */
};

/* The row states GetSubMenuEntryState returns. _SELECTED marks the entry that
   is already the slot's unsettled action; UNUSABLE means the action exists but
   fails the current targeting/usability check, and UNAVAILABLE means the slot
   lacks the capability entirely, so BuildSubMenuPanel creates no row for it. */
enum W8SubMenuEntryState {
    W8_SUBMENU_ENTRY_USABLE = 0,
    W8_SUBMENU_ENTRY_USABLE_SELECTED = 1,
    W8_SUBMENU_ENTRY_UNUSABLE = 2,
    W8_SUBMENU_ENTRY_UNUSABLE_SELECTED = 3,
    W8_SUBMENU_ENTRY_UNAVAILABLE = 4
};

/* The icon paths, button positions and (menu, item)-keyed entry message/help
   tables. */
extern char g_submenu_icons_path[];
extern char g_submenu_combat_icons_path[];
extern char g_options_disk_path[];
extern char g_attack_confirm_path[];
extern char g_combat_stop_path[];
extern char g_cont_start_path[];
extern char g_cont_toggle_path[];
extern char g_cont_pending_path[];
extern char g_roof_buttons_path[];
extern char g_layout_arrows_path[];
extern srVector2i g_submenu_button_positions[9];
extern srVector2i g_scroll_button_positions[2];
extern srVector2i g_submenu_panel_button_positions[2];
extern srVector2i g_options_disk_position;
extern srVector2i g_combat_stance_positions[5];
extern srVector2i g_roof_button_positions[3];
extern srVector2i g_layout_arrow_positions[6];
/* The (menu, item) keyed entry message/help indexes both menus build rows
   from; the keyboard menu shares them. */
extern short g_submenu_entry_message_ids[25];
extern int g_submenu_entry_help_ids[25];

/* Create the nine-button bank and keep its state flag clear. */
unsigned char CreateSubMenuButtons(void);
/* Restate one bank button against the live mode flags. */
void UpdateSubMenuButton(int index);
/* Drop the combat-end notification, rebuild the panel for the saved one. */
void ReopenSubMenuPanel(void);
/* Descriptive name for panel/row deletion shared with failed construction. */
void DestroySubMenuPanel();
/* Drop the combat-end notification and tear down the panel and its rows. */
void DestroySubMenuControls(void);
