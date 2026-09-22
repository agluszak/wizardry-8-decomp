/* Local Screens\MGSPortraitCombat.cpp: the portrait combat sub-menu. The
   demo's __FILE__ path sits in .data between "Local Screens\MGSKeyboard.cpp"
   and "Local Screens\MGSButtons.cpp", matching this unit's retail .text
   span between the proved MGSKeyboard hull and MGSButtons' anchored start
   at 0x005963E0. Its demo literal cluster carries the sub-menu icon paths
   (icons_submenu, icon_combat_toggle, attack_confirm, combat_stop,
   cont_start/toggle/pending, main_roof_buttons, main_layout_arrows,
   options_disk) and the "Failed submenu building" diagnostic, matching the
   const tables and path globals emitted below. */

#include "wiz8/local_screens/MGSPortraitCombat.h"

#include "wiz8/layouts/character.h"
#include "wiz8/character_skills.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/local_code/CharGeneration.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/local_code/Search.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/GameplayTime.h"
#include "wiz8/local_code/GameplayMods.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/local_code/party_encumbrance.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/UtilityFunctions.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/layouts/screen_state.h"
#include "wiz8/local_code/CombatRange.h"
#include "wiz8/local_code/Gameloop.h"
#include "wiz8/dialog_code/DialogButton.h"
#include "wiz8/fonts.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/layouts/item_tables.h"
#include "wiz8/cursor.h"
#include "wiz8/local_code/Controls.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_code/TextControl.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/NPCInteractionSubscreen.h"
#include "wiz8/local_screens/MGSFormation.h"
#include "wiz8/local_screens/MGSButtons.h"
#include "wiz8/local_screens/MGSSpellCasting.h"
#include "wiz8/local_screens/MGSUseItemSelect.h"
#include "wiz8/local_screens/Screens.h"
#include "wiz8/npc_interaction.h"
#include "wiz8/regions.h"
#include "wiz8/sr_api.h"
#include "wiz8/local_code/Targeting.h"
#include "wiz8/utility.h"
#include "wiz8/video_object_catalog.h"
#include "wiz8/xstatus.h"

// GLOBAL: WIZ8 0x0064C238
const char g_submenu_icons_path_64c238[] = "Data\\Main Interface\\icons_standard.sti";
// GLOBAL: WIZ8 0x0064C260
const char g_submenu_combat_icons_path_64c260[] = "Data\\Main Interface\\icon_combat_toggle.sti";
/* The (x, y) of the nine bank buttons. */
// GLOBAL: WIZ8 0x0064C290
const int g_submenu_button_positions_64c290[9][2] = {
    {40, 452},  {70, 452},  {100, 452}, {130, 452}, {160, 452},
    {232, 452}, {262, 452}, {199, 452}, {200, 452},
};
/* The (x, y) of the two scroll arrows. */
// GLOBAL: WIZ8 0x0064C330
const int g_scroll_button_positions_64c330[2][2] = {{300, 456}, {323, 456}};
/* The (x, y) of the two panel buttons (close and formation). */
// GLOBAL: WIZ8 0x0064C378
const int g_submenu_panel_button_positions_64c378[2][2] = {{541, 452}, {571, 452}};
// GLOBAL: WIZ8 0x0064C388
const char g_options_disk_path_64c388[] = "Data\\Main Interface\\options_disk.sti";
// GLOBAL: WIZ8 0x0064C3B0
const int g_options_disk_position_64c3b0[2] = {0, 450};
// GLOBAL: WIZ8 0x0064C3B8
const char g_attack_confirm_path_64c3b8[] = "Data\\Main Interface\\attack_confirm.sti";
// GLOBAL: WIZ8 0x0064C3E0
const char g_combat_stop_path_64c3e0[] = "Data\\Main Interface\\combat_stop.sti";
// GLOBAL: WIZ8 0x0064C404
const char g_cont_start_path_64c404[] = "Data\\Main Interface\\cont_start.sti";
// GLOBAL: WIZ8 0x0064C428
const char g_cont_toggle_path_64c428[] = "Data\\Main Interface\\cont_toggle.sti";
// GLOBAL: WIZ8 0x0064C44C
const char g_cont_pending_path_64c44c[] = "Data\\Main Interface\\cont_pending.sti";
/* All five combat-stance buttons share the same screen origin. */
// GLOBAL: WIZ8 0x0064C478
const int g_combat_stance_positions_64c478[5][2] = {
    {610, 450}, {610, 450}, {610, 450}, {610, 450}, {610, 450},
};
// GLOBAL: WIZ8 0x0064C4A0
const char g_roof_buttons_path_64c4a0[] = "Data\\Main Interface\\main_roof_buttons.sti";
// GLOBAL: WIZ8 0x0064C4D0
const int g_roof_button_positions_64c4d0[3][2] = {{10, 1}, {39, 1}, {68, 1}};
// GLOBAL: WIZ8 0x0064C4E8
const char g_layout_arrows_path_64c4e8[] = "Data\\Main Interface\\main_layout_arrows.sti";
/* Left column then right column; both columns share the same x in retail. */
// GLOBAL: WIZ8 0x0064C518
const int g_layout_arrow_positions_64c518[6][2] = {
    {0, 371}, {0, 394}, {0, 417}, {0, 371}, {0, 394}, {0, 417},
};
/* The (menu, item) keyed message indexes both menus build rows from. */
// GLOBAL: WIZ8 0x0064C548
const short g_submenu_entry_message_ids_64c548[25] = {
    0,  63, 70,  77,  84, 91, 98, -1,  -1,  -1, 154, 161, 168,
    -1, -1, 105, 112, -1, -1, -1, 175, 182, -1, -1,  -1,
};
/* The matching help indexes. */
// GLOBAL: WIZ8 0x0064C57C
const int g_submenu_entry_help_ids_64c57c[25] = {
    82, 83, 84, 85, 86, 87, 88, -1, -1, -1, 91, 92, -1,
    -1, -1, 89, -1, -1, -1, -1, 95, 94, -1, -1, -1,
};

void SubMenuButtonPendingScreen(W8DialogButton* button);
void SubMenuButtonSurprise(W8DialogButton* button);
void SubMenuButtonToggleFlag(W8DialogButton* button);
void SubMenuButtonUseItem(W8DialogButton* button);
void SubMenuButtonSpellView(W8DialogButton* button);
void SubMenuButtonOpenMenu0(W8DialogButton* button);
void SubMenuButtonOpenMenu1(W8DialogButton* button);
void SubMenuButtonToggleCombat(W8DialogButton* button);

// FUNCTION: WIZ8 0x00594AF0
unsigned char CreateSubMenuButtons(void)
{
    int index;

    for (index = 0; index < 9; ++index) {
        g_submenu_buttons_69b8b0[index] = new W8DialogButton;
        if (g_submenu_buttons_69b8b0[index] == 0) {
            for (index = 0; index < 9; ++index) {
                if (g_submenu_buttons_69b8b0[index] != 0) {
                    delete g_submenu_buttons_69b8b0[index];
                    g_submenu_buttons_69b8b0[index] = 0;
                }
            }
            return 0;
        }
    }
    g_submenu_buttons_69b8b0[0]->Configure(g_submenu_icons_path_64c238, 0x2b, 0x28, 0x29, 0x2a,
                                           0x2c, SubMenuButtonPendingScreen, 0, 0, 0x7f, 0x42, 0,
                                           0);
    g_submenu_buttons_69b8b0[1]->Configure(g_submenu_icons_path_64c238, 0x3, 0x0, 0x1, 0x2, 0x4,
                                           SubMenuButtonSurprise, 0, 0, 0x7f, 0x43, 0, 0);
    g_submenu_buttons_69b8b0[2]->Configure(g_submenu_icons_path_64c238, 0x8, 0x5, 0x6, 0x7, 0x9,
                                           SubMenuButtonToggleFlag, 0, 1, 0x7f, 0x44, 0, 0);
    g_submenu_buttons_69b8b0[3]->Configure(g_submenu_icons_path_64c238, 0x17, 0x14, 0x15, 0x16,
                                           0x18, SubMenuButtonUseItem, 0, 1, 0x7f, 0x45, 0, 0);
    g_submenu_buttons_69b8b0[4]->Configure(g_submenu_icons_path_64c238, 0x1c, 0x19, 0x1a, 0x1b,
                                           0x1d, SubMenuButtonSpellView, 0, 1, 0x7f, 0x46, 0, 0);
    g_submenu_buttons_69b8b0[5]->Configure(g_submenu_icons_path_64c238, 0x12, 0xf, 0x10, 0x11, 0x13,
                                           SubMenuButtonOpenMenu1, 0, 0, 0x7f, 0x4a, 0, 0);
    g_submenu_buttons_69b8b0[6]->Configure(g_submenu_icons_path_64c238, 0xd, 0xa, 0xb, 0xc, 0xe,
                                           SubMenuButtonOpenMenu0, 0, 0, 0x7f, 0x49, 0, 0);
    g_submenu_buttons_69b8b0[7]->Configure(g_submenu_combat_icons_path_64c260, 0x3, 0x0, 0x1, 0x2,
                                           0x2, SubMenuButtonToggleCombat, 0, 0, 0x7f, 0x47, 0, 0);
    g_submenu_buttons_69b8b0[8]->Configure(g_submenu_combat_icons_path_64c260, 0x7, 0x4, 0x5, 0x6,
                                           0x6, SubMenuButtonToggleCombat, 0, 0, 0x7f, 0x48, 0, 0);
    for (index = 0; index < 9; ++index) {
        g_submenu_buttons_69b8b0[index]->SetPosition(g_submenu_button_positions_64c290[index][0],
                                                     g_submenu_button_positions_64c290[index][1]);
        g_submenu_buttons_69b8b0[index]->m_owner_040 = 0;
    }
    return 1;
}

// FUNCTION: WIZ8 0x00594D20
void UpdateSubMenuButton(int index)
{
    switch (index) {
    case 5:
    case 6:
    case 8:
        g_submenu_buttons_69b8b0[index]->SetVisible(gXStatus.fCombatMode != 0);
        break;
    case 7:
        g_submenu_buttons_69b8b0[index]->SetVisible(gXStatus.fCombatMode == 0);
        break;
    }
    if (gXStatus.fNpcDialogueMode != 0 &&
        (index != 0 || IsNpcDialogueCursorActive() != 0 || CanOpenNpcDialogue() != 0)) {
        g_submenu_buttons_69b8b0[index]->SetEnabled(0);
        return;
    }
    if (gXStatus.fLockInteractMode != 0 || gXStatus.fTrapInteractMode != 0 ||
        gXStatus.fReviewCharacterMode != 0) {
        g_submenu_buttons_69b8b0[index]->SetEnabled(0);
        return;
    }
    if (gXStatus.fItemSelectMode != 0 && index != 3) {
        g_submenu_buttons_69b8b0[index]->SetEnabled(0);
        return;
    }
    if (gXStatus.fSpellCastMode != 0 && index != 4) {
        g_submenu_buttons_69b8b0[index]->SetEnabled(0);
        return;
    }
    if (gXStatus.fSurprisePossible != 0 || gXStatus.fCampMode != 0) {
        g_submenu_buttons_69b8b0[index]->SetEnabled(0);
        return;
    }
    if (gXStatus.fCombatMode != 0 && g_combat_state->round_active_001 == 0) {
        g_submenu_buttons_69b8b0[index]->SetEnabled(0);
        return;
    }
    if (g_level_block->combat_end_notification != -1) {
        g_submenu_buttons_69b8b0[index]->SetEnabled(0);
        return;
    }
    switch (index) {
    case 0:
        g_submenu_buttons_69b8b0[0]->SetEnabled(gXStatus.fCombatMode == 0);
        break;
    case 1:
        g_submenu_buttons_69b8b0[1]->SetEnabled(gXStatus.fCombatMode == 0);
        break;
    case 2:
        g_submenu_buttons_69b8b0[2]->SetEnabled(gXStatus.fCombatMode == 0);
        if (g_submenu_buttons_69b8b0[2]->IsPressed() != (g_status_685170.search_mode != 0)) {
            g_submenu_buttons_69b8b0[2]->SetPressed(g_status_685170.search_mode != 0);
        }
        break;
    case 3:
        g_submenu_buttons_69b8b0[3]->SetEnabled(
            IsPartySlotEligible00524A10(g_status_685170.selected_character) != 0);
        if (g_submenu_buttons_69b8b0[3]->IsPressed() != (gXStatus.fItemSelectMode != 0)) {
            g_submenu_buttons_69b8b0[3]->SetPressed(gXStatus.fItemSelectMode != 0);
        }
        break;
    case 4:
        g_submenu_buttons_69b8b0[4]->SetEnabled(
            IsPartySlotEligible00524A10(g_status_685170.selected_character) != 0 &&
            CharacterHasCastableSpell(
                &g_status_685170.buffers.Char[g_status_685170.selected_character]) != 0);
        if (g_submenu_buttons_69b8b0[4]->IsPressed() != (gXStatus.fSpellCastMode != 0)) {
            g_submenu_buttons_69b8b0[4]->SetPressed(gXStatus.fSpellCastMode != 0);
        }
        break;
    case 5:
    case 6:
        g_submenu_buttons_69b8b0[index]->SetEnabled(gXStatus.fCombatMode != 0);
        if (IsPartySlotEligible00524A10(g_status_685170.selected_character) == 0) {
            g_submenu_buttons_69b8b0[index]->SetEnabled(0);
        }
        break;
    case 7:
        g_submenu_buttons_69b8b0[7]->SetEnabled(gXStatus.fCombatMode == 0);
        break;
    case 8:
        g_submenu_buttons_69b8b0[8]->SetEnabled(gXStatus.fCombatMode != 0);
        break;
    }
}

// FUNCTION: WIZ8 0x00595090
void SubMenuButtonPendingScreen(W8DialogButton* button)
{
    button->SetPressed(0);
    button->m_dirty = 1;
    if (gXStatus.fNpcDialogueMode != 0) {
        gXStatus.fCampMode = 1;
    }
    SetPendingScreenState(W8_SCREEN_JOURNAL);
}

// FUNCTION: WIZ8 0x005950C0
void SubMenuButtonSurprise(W8DialogButton* button)
{
    RequestCamp00502460();
    RequestRedraw(0x200);
    DrawSubMenuCharacterAction();
}

// FUNCTION: WIZ8 0x005950E0
void SubMenuButtonToggleFlag(W8DialogButton* button)
{
    ToggleSearchMode();
    button->SetPressed(g_status_685170.search_mode ? 1 : 0);
    RequestRedraw(0x200);
    DrawSubMenuCharacterAction();
}

// FUNCTION: WIZ8 0x00595110
void SubMenuButtonUseItem(W8DialogButton* button)
{
    int index;

    if (gXStatus.fItemSelectMode != 0) {
        CloseUseItemSelectView();
        return;
    }
    if (gXStatus.fCombatMode != 0) {
        if (g_level_block->combat_end_notification != -1) {
            SetSubMenuButtonTooltips(1);
            g_level_block->combat_end_notification = -1;
            g_submenu_entry_count_69b87e = 0;
            RegionSetDisable(0x27);
            DisableRegionSetInput(0x27);
            if (gpSubMenuPanel != 0) {
                delete gpSubMenuPanel;
                gpSubMenuPanel = 0;
            }
            for (index = 0; index < 5; ++index) {
                if (g_submenu_rows_69b8ec[index] != 0) {
                    delete g_submenu_rows_69b8ec[index];
                    g_submenu_rows_69b8ec[index] = 0;
                }
            }
            RequestRedraw(0x200);
        }
        UpdateScreenOverlays(0);
        if (BuildSubMenuPanel(3) == 0) {
            SetSubMenuButtonTooltips(1);
            g_level_block->combat_end_notification = -1;
            g_submenu_entry_count_69b87e = 0;
            RegionSetDisable(0x27);
            DisableRegionSetInput(0x27);
            if (gpSubMenuPanel != 0) {
                delete gpSubMenuPanel;
                gpSubMenuPanel = 0;
            }
            for (index = 0; index < 5; ++index) {
                if (g_submenu_rows_69b8ec[index] != 0) {
                    delete g_submenu_rows_69b8ec[index];
                    g_submenu_rows_69b8ec[index] = 0;
                }
            }
            RequestRedraw(0x200);
        }
        SetSubMenuButtonTooltips(0);
        g_submenu_clock_69b880 = SetCountdownClock(0);
        g_submenu_flag_69b8d4 = 0;
        RequestRedraw(0x200);
        ResetClickedMode();
        return;
    }
    OpenUseItemSelectView(g_status_685170.selected_character);
}

// FUNCTION: WIZ8 0x00595280
void SubMenuButtonSpellView(W8DialogButton* button)
{
    if (CharacterHasCastableSpell(
            &g_status_685170.buffers.Char[g_status_685170.selected_character]) != 0) {
        if (gXStatus.fSpellCastMode != 0) {
            CloseSpellCastingView();
        } else {
            OpenSpellCastingView(g_status_685170.selected_character);
        }
    }
}

// FUNCTION: WIZ8 0x005952D0
void SubMenuButtonOpenMenu0(W8DialogButton* button)
{
    int index;

    if (g_level_block->combat_end_notification != -1) {
        SetSubMenuButtonTooltips(1);
        g_level_block->combat_end_notification = -1;
        g_submenu_entry_count_69b87e = 0;
        RegionSetDisable(0x27);
        DisableRegionSetInput(0x27);
        if (gpSubMenuPanel != 0) {
            delete gpSubMenuPanel;
            gpSubMenuPanel = 0;
        }
        for (index = 0; index < 5; ++index) {
            if (g_submenu_rows_69b8ec[index] != 0) {
                delete g_submenu_rows_69b8ec[index];
                g_submenu_rows_69b8ec[index] = 0;
            }
        }
        RequestRedraw(0x200);
    }
    UpdateScreenOverlays(0);
    if (BuildSubMenuPanel(6) == 0) {
        SetSubMenuButtonTooltips(1);
        g_level_block->combat_end_notification = -1;
        g_submenu_entry_count_69b87e = 0;
        RegionSetDisable(0x27);
        DisableRegionSetInput(0x27);
        if (gpSubMenuPanel != 0) {
            delete gpSubMenuPanel;
            gpSubMenuPanel = 0;
        }
        for (index = 0; index < 5; ++index) {
            if (g_submenu_rows_69b8ec[index] != 0) {
                delete g_submenu_rows_69b8ec[index];
                g_submenu_rows_69b8ec[index] = 0;
            }
        }
        RequestRedraw(0x200);
    }
    SetSubMenuButtonTooltips(0);
    g_submenu_clock_69b880 = SetCountdownClock(0);
    g_submenu_flag_69b8d4 = 0;
    RequestRedraw(0x200);
}

// FUNCTION: WIZ8 0x00595410
void SubMenuButtonOpenMenu1(W8DialogButton* button)
{
    int index;

    if (g_level_block->combat_end_notification != -1) {
        SetSubMenuButtonTooltips(1);
        g_level_block->combat_end_notification = -1;
        g_submenu_entry_count_69b87e = 0;
        RegionSetDisable(0x27);
        DisableRegionSetInput(0x27);
        if (gpSubMenuPanel != 0) {
            delete gpSubMenuPanel;
            gpSubMenuPanel = 0;
        }
        for (index = 0; index < 5; ++index) {
            if (g_submenu_rows_69b8ec[index] != 0) {
                delete g_submenu_rows_69b8ec[index];
                g_submenu_rows_69b8ec[index] = 0;
            }
        }
        RequestRedraw(0x200);
    }
    UpdateScreenOverlays(0);
    if (BuildSubMenuPanel(5) == 0) {
        SetSubMenuButtonTooltips(1);
        g_level_block->combat_end_notification = -1;
        g_submenu_entry_count_69b87e = 0;
        RegionSetDisable(0x27);
        DisableRegionSetInput(0x27);
        if (gpSubMenuPanel != 0) {
            delete gpSubMenuPanel;
            gpSubMenuPanel = 0;
        }
        for (index = 0; index < 5; ++index) {
            if (g_submenu_rows_69b8ec[index] != 0) {
                delete g_submenu_rows_69b8ec[index];
                g_submenu_rows_69b8ec[index] = 0;
            }
        }
        RequestRedraw(0x200);
    }
    SetSubMenuButtonTooltips(0);
    g_submenu_clock_69b880 = SetCountdownClock(0);
    g_submenu_flag_69b8d4 = 0;
    RequestRedraw(0x200);
}

// FUNCTION: WIZ8 0x00595550
void SubMenuButtonToggleCombat(W8DialogButton* button)
{
    ToggleCombatMode();
    RequestRedraw(0x200);
    DrawSubMenuCharacterAction();
}

/* Drop the combat-end notification and tear down the panel and its rows.
   The reset body compiles this three times - before the rebuild, again while
   the notification is still live, and once more when the rebuild fails. */
// FUNCTION: WIZ8 0x00595570
void DestroySubMenuControls(void)
{
    int i;

    SetSubMenuButtonTooltips(1);
    g_level_block->combat_end_notification = -1;
    g_submenu_entry_count_69b87e = 0;
    RegionSetDisable(0x27);
    DisableRegionSetInput(0x27);
    if (gpSubMenuPanel != 0) {
        delete gpSubMenuPanel;
        gpSubMenuPanel = 0;
    }
    for (i = 0; i < 5; ++i) {
        if (g_submenu_rows_69b8ec[i] != 0) {
            delete g_submenu_rows_69b8ec[i];
            g_submenu_rows_69b8ec[i] = 0;
        }
    }
    RequestRedraw(0x200);
}

// FUNCTION: WIZ8 0x00595600
void ReopenSubMenuPanel(void)
{
    int index;
    short notification = g_level_block->combat_end_notification;

    if (notification == -1) {
        return;
    }
    SetSubMenuButtonTooltips(1);
    g_level_block->combat_end_notification = -1;
    g_submenu_entry_count_69b87e = 0;
    RegionSetDisable(0x27);
    DisableRegionSetInput(0x27);
    if (gpSubMenuPanel != 0) {
        delete gpSubMenuPanel;
        gpSubMenuPanel = 0;
    }
    for (index = 0; index < 5; ++index) {
        if (g_submenu_rows_69b8ec[index] != 0) {
            delete g_submenu_rows_69b8ec[index];
            g_submenu_rows_69b8ec[index] = 0;
        }
    }
    RequestRedraw(0x200);
    UpdateScreenOverlays(0);
    if (BuildSubMenuPanel(notification) == 0) {
        SetSubMenuButtonTooltips(1);
        g_level_block->combat_end_notification = -1;
        g_submenu_entry_count_69b87e = 0;
        RegionSetDisable(0x27);
        DisableRegionSetInput(0x27);
        if (gpSubMenuPanel != 0) {
            delete gpSubMenuPanel;
            gpSubMenuPanel = 0;
        }
        for (index = 0; index < 5; ++index) {
            if (g_submenu_rows_69b8ec[index] != 0) {
                delete g_submenu_rows_69b8ec[index];
                g_submenu_rows_69b8ec[index] = 0;
            }
        }
        RequestRedraw(0x200);
    }
    SetSubMenuButtonTooltips(0);
    g_submenu_clock_69b880 = SetCountdownClock(0);
    g_submenu_flag_69b8d4 = 0;
    RequestRedraw(0x200);
}
