#include "wiz8/local_screens/MGSButtons.h"
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
#include "wiz8/local_code/GameplayMods.h"
#include "wiz8/local_code/GameplayTime.h"
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
#include "wiz8/local_screens/MGSFormation.h"
#include "wiz8/local_screens/MGSPortraitCombat.h"
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

#include "Font.h"
#include "timer.h"

#include <stdio.h>

#define MGSBUTTONS_CPP "C:\\Projects\\Wizardry 8\\Local Screens\\MGSButtons.cpp"

// GLOBAL: WIZ8 0x0061e988
const unsigned short g_action_kind_message_ids_61e988[12] = {
    0x050f, 0x0510, 0x0511, 0x0512, 0x0513, 0x0514, 0x0515, 0x0516, 0x0517, 0x0518, 0x0519, 0x051a,
};

// GLOBAL: WIZ8 0x0069B854
short g_submenu_menu_69b854;
// GLOBAL: WIZ8 0x0069B858
W8DialogButton* g_submenu_scroll_buttons_69b858[2];
/* [0] closes the panel (callback 0x005978D0), [1] toggles the formation
   screen (callback 0x00597A10 -> OpenFormationPanel/CloseFormationPanel). */
// GLOBAL: WIZ8 0x0069B860
W8DialogButton* g_submenu_panel_buttons_69b860[2];
// GLOBAL: WIZ8 0x0069B868
short g_submenu_entries_69b868[6];
// GLOBAL: WIZ8 0x0069B874
short g_submenu_entry_states_69b874[5];
// GLOBAL: WIZ8 0x0069B87E
short g_submenu_entry_count_69b87e;
// GLOBAL: WIZ8 0x0069B880
TIMER g_submenu_clock_69b880;
/* Layout arrows: [0..2] left column (radar/action/formation), [3..5] right. */
// GLOBAL: WIZ8 0x0069B884
W8DialogButton* g_layout_arrow_buttons_69b884[6];
/* Combat stance: attack confirm, stop, continuous start/toggle/pending. */
// GLOBAL: WIZ8 0x0069B89C
W8DialogButton* g_combat_stance_buttons_69b89c[5];
// GLOBAL: WIZ8 0x0069B8B0
W8DialogButton* g_submenu_buttons_69b8b0[9];
// GLOBAL: WIZ8 0x0069B8D4
unsigned char g_submenu_flag_69b8d4;
/* Roof viewpoint buttons: modes 0, 1 and 2. */
// GLOBAL: WIZ8 0x0069B8D8
W8DialogButton* g_roof_buttons_69b8d8[3];
// GLOBAL: WIZ8 0x0069B8E4
W8DialogButton* g_options_disk_button_69b8e4;
/* The name is proven by this file's own assertion. */
// GLOBAL: WIZ8 0x0069B8E8
Controls* gpSubMenuPanel;
/* The five caption rows; BuildSubMenuPanel news 0xb8-byte objects
   through the W8TextControl constructor. Five is the retail bound: the next
   known global starts at 0x0069B900. Menu 0 considers five actions then adds
   a cancel row at index built - 1, which could look like a sixth write - but
   Berserk requires the fighter-only trait 0x14 while Pray requires the
   priest-only trait 0x0b and iProfession is a single index, so menu 0
   never has all five actions available and built stays <= 5. */
// GLOBAL: WIZ8 0x0069B8EC
W8TextControl* g_submenu_rows_69b8ec[5];

void SubMenuPanelCloseButton(W8DialogButton* button);
void SubMenuPanelFormationButton(W8DialogButton* button);
void MainGameOptionsDiskButton(W8DialogButton* button);
void MainGameCombatConfirmButton(W8DialogButton* button);
void MainGameCombatStanceSecondary(W8DialogButton* button);
void MainGameRoofButton0(W8DialogButton* button);
void MainGameRoofButton1(W8DialogButton* button);
void MainGameRoofButton2(W8DialogButton* button);
void MainGameLayoutRadarButton(W8DialogButton* button);
void MainGameLayoutActionPanelButton(W8DialogButton* button);
void MainGameLayoutFormationButton(W8DialogButton* button);

// FUNCTION: WIZ8 0x005963E0
void RefreshSubMenuPanel(char invalidate)
{
    if (gpSubMenuPanel == 0) {
        srAssertFail("gpSubMenuPanel", MGSBUTTONS_CPP, 0x64f, 0);
    }
    if (invalidate != 0) {
        gpSubMenuPanel->Invalidate(0);
    }
    gpSubMenuPanel->Redraw();
}

// FUNCTION: WIZ8 0x00596430
void SubMenuSelectAttack(void)
{
    int index;

    ChooseAction(g_status_685170.selected_character, W8_ACTION_ATTACK, -1, 0, 0, 1);
    DrawSubMenuCharacterAction();
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

// FUNCTION: WIZ8 0x005964D0
void SubMenuSelectBerserk(void)
{
    int index;

    ChooseAction(g_status_685170.selected_character, W8_ACTION_BERSERK, -1, 0, 0, 1);
    DrawSubMenuCharacterAction();
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

// FUNCTION: WIZ8 0x00596570
void SubMenuSelectBreathe(void)
{
    int index;

    ChooseAction(g_status_685170.selected_character, W8_ACTION_BREATHE, -1, 0, 0, 1);
    DrawSubMenuCharacterAction();
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

// FUNCTION: WIZ8 0x00596610
void SubMenuSelectTurnUndead(void)
{
    int index;

    ChooseAction(g_status_685170.selected_character, W8_ACTION_TURN_UNDEAD, -1, 0, 0, 1);
    DrawSubMenuCharacterAction();
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

// FUNCTION: WIZ8 0x005966B0
void SubMenuSelectPray(void)
{
    int index;

    ChooseAction(g_status_685170.selected_character, W8_ACTION_PRAY, -1, 0, 0, 1);
    DrawSubMenuCharacterAction();
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

// FUNCTION: WIZ8 0x00596750
void SubMenuSelectDefend(void)
{
    int index;

    ChooseAction(g_status_685170.selected_character, W8_ACTION_DEFEND, -1, 0, 0, 1);
    DrawSubMenuCharacterAction();
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

// FUNCTION: WIZ8 0x00596800
void SubMenuSelectProtect(void)
{
    int index;

    ChooseAction(g_status_685170.selected_character, W8_ACTION_PROTECT, -1, 0, 0, 1);
    DrawSubMenuCharacterAction();
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

// FUNCTION: WIZ8 0x005968A0
void SubMenuSelectEquip(void)
{
    int index;

    ChooseAction(g_status_685170.selected_character, W8_ACTION_EQUIP, -1, 0, 0, 1);
    DrawSubMenuCharacterAction();
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

// FUNCTION: WIZ8 0x00596940
void SubMenuOpenUseItemView(void)
{
    int index;

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
    OpenUseItemSelectView(g_status_685170.selected_character);
}

// FUNCTION: WIZ8 0x005969D0
void SubMenuUseRecordedItem(void)
{
    int index;

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
    StartCharacterItemUse(g_status_685170.selected_character);
}

// FUNCTION: WIZ8 0x00596A60
void SubMenuOpenSpellView(void)
{
    int index;

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
    OpenSpellCastingView(g_status_685170.selected_character);
}

// FUNCTION: WIZ8 0x00596AF0
void SubMenuCastRecordedSpell(void)
{
    int index;

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
    StartCharacterSpellCast(g_status_685170.selected_character, 0);
}

// FUNCTION: WIZ8 0x00596B90
void SubMenuSelectRun(void)
{
    int index;

    if (AnyCharacterEngaged() == 0) {
        return;
    }
    ChooseAction(g_status_685170.selected_character, W8_ACTION_RUN, -1, 0, 0, 1);
    DrawSubMenuCharacterAction();
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

// FUNCTION: WIZ8 0x00596C40
void SubMenuSelectWalk(void)
{
    int index;

    if (AnyCharacterEngaged() == 0) {
        return;
    }
    ChooseAction(g_status_685170.selected_character, W8_ACTION_WALK, -1, 0, 0, 1);
    DrawSubMenuCharacterAction();
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

// FUNCTION: WIZ8 0x00596CF0
void ResetSubMenuPanel(void)
{
    short saved_notification;
    char rebuilt;

    saved_notification = g_level_block->combat_end_notification;
    DestroySubMenuControls();
    if (g_level_block->combat_end_notification != -1) {
        DestroySubMenuControls();
    }
    UpdateScreenOverlays(0);
    rebuilt = BuildSubMenuPanel(saved_notification);
    if (rebuilt == 0) {
        DestroySubMenuControls();
    }
    SetSubMenuButtonTooltips(0);
    g_submenu_clock_69b880 = SetCountdownClock(0);
    g_submenu_flag_69b8d4 = 0;
    RequestRedraw(0x200);
}

// FUNCTION: WIZ8 0x00596EC0
unsigned char CreateSubMenuScrollButtons(void)
{
    int i;

    for (i = 0; i < 2; ++i) {
        g_submenu_scroll_buttons_69b858[i] = new W8DialogButton;
        if (g_submenu_scroll_buttons_69b858[i] == 0) {
            for (i = 0; i < 2; ++i) {
                if (g_submenu_scroll_buttons_69b858[i] != 0) {
                    delete g_submenu_scroll_buttons_69b858[i];
                    g_submenu_scroll_buttons_69b858[i] = 0;
                }
            }
            return 0;
        }
    }
    g_submenu_scroll_buttons_69b858[0]->Configure("Data\\Main Interface\\main_scroll_arrows.sti", 3,
                                                  0, 1, 2, 2, SubMenuScrollArrowUp, 0, 0, 0x7f,
                                                  0x4b, 0, 0);
    g_submenu_scroll_buttons_69b858[1]->Configure("Data\\Main Interface\\main_scroll_arrows.sti", 7,
                                                  4, 5, 6, 6, SubMenuScrollArrowDown, 0, 0, 0x7f,
                                                  0x4c, 0, 0);
    for (i = 0; i < 2; ++i) {
        g_submenu_scroll_buttons_69b858[i]->SetPosition(g_scroll_button_positions_64c330[i][0],
                                                        g_scroll_button_positions_64c330[i][1]);
        g_submenu_scroll_buttons_69b858[i]->m_owner_040 = 0;
    }
    return 1;
}

// FUNCTION: WIZ8 0x005978D0
void SubMenuPanelCloseButton(W8DialogButton* button)
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
    if (BuildSubMenuPanel(9) == 0) {
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

// FUNCTION: WIZ8 0x00597A10
void SubMenuPanelFormationButton(W8DialogButton* button)
{
    if (gXStatus.fReviewCharacterMode == 0) {
        OpenFormationPanel();
    } else {
        CloseFormationPanel();
    }
}

// FUNCTION: WIZ8 0x00597670
unsigned char CreateSubMenuPanelButtons(void)
{
    int index;

    for (index = 0; index < 2; ++index) {
        g_submenu_panel_buttons_69b860[index] = new W8DialogButton;
        if (g_submenu_panel_buttons_69b860[index] == 0) {
            for (index = 0; index < 2; ++index) {
                if (g_submenu_panel_buttons_69b860[index] != 0) {
                    delete g_submenu_panel_buttons_69b860[index];
                    g_submenu_panel_buttons_69b860[index] = 0;
                }
            }
            return 0;
        }
    }
    g_submenu_panel_buttons_69b860[0]->Configure(g_submenu_icons_path_64c238, 0x21, 0x1e, 0x1f,
                                                 0x20, 0x22, SubMenuPanelCloseButton, 0, 0, 0x7f,
                                                 0x4d, 0, 0);
    g_submenu_panel_buttons_69b860[1]->Configure(g_submenu_icons_path_64c238, 0x26, 0x23, 0x24,
                                                 0x25, 0x27, SubMenuPanelFormationButton, 0, 1,
                                                 0x7f, 0x4e, 0, 0);
    for (index = 0; index < 2; ++index) {
        g_submenu_panel_buttons_69b860[index]->SetPosition(
            g_submenu_panel_button_positions_64c378[index][0],
            g_submenu_panel_button_positions_64c378[index][1]);
        g_submenu_panel_buttons_69b860[index]->m_owner_040 = 0;
    }
    return 1;
}

// FUNCTION: WIZ8 0x00597B70
void MainGameOptionsDiskButton(W8DialogButton* button)
{
    if (IsLevelDataFlag4EffectivelySet() != 0) {
        g_pending_screen_state.mode = 3;
        SetPendingScreenState(W8_SCREEN_OPTIONS);
    }
}

// FUNCTION: WIZ8 0x00597A30
unsigned char CreateOptionsDiskButton(void)
{
    g_options_disk_button_69b8e4 = new W8DialogButton;
    if (g_options_disk_button_69b8e4 == 0) {
        return 0;
    }
    g_options_disk_button_69b8e4->Configure(g_options_disk_path_64c388, 3, 0, 1, 2, 2,
                                            MainGameOptionsDiskButton, 0, 0, 0x7f, 0x41, 0, 0);
    g_options_disk_button_69b8e4->SetPosition(g_options_disk_position_64c3b0[0],
                                              g_options_disk_position_64c3b0[1]);
    g_options_disk_button_69b8e4->m_owner_040 = 0;
    return 1;
}

/* Redraw the options-disk chrome and button; disabled under dialogue / lock /
   trap / camp interact modes. */
// FUNCTION: WIZ8 0x00597AF0
void RedrawOptionsDiskButton(void)
{
    bool enabled;

    DrawCatalogImageAndInvalidate(-0xe, 0x7e, 0, 2, 0, 0x1c2, 2, 0);
    g_options_disk_button_69b8e4->m_dirty = 1;
    if (gXStatus.fNpcDialogueMode == 0 && gXStatus.fLockInteractMode == 0 &&
        gXStatus.fTrapInteractMode == 0 && gXStatus.fCampMode == 0 && gXStatus.fLockInteract == 0 &&
        gXStatus.fTrapInteract == 0) {
        enabled = 1;
    } else {
        enabled = 0;
    }
    g_options_disk_button_69b8e4->SetEnabled(enabled);
    g_options_disk_button_69b8e4->Draw();
}

/* Pick which of the five combat-stance buttons is visible for the live combat
   / continuous-combat state, then enable and Draw that one. */
// FUNCTION: WIZ8 0x00597D70
void UpdateCombatStanceButtons(void)
{
    unsigned int stance;
    unsigned char index;
    bool enabled;
    W8DialogButton** button;

    if (gXStatus.fCombatMode == 0) {
        for (button = g_combat_stance_buttons_69b89c; button < &g_combat_stance_buttons_69b89c[5];
             ++button) {
            (*button)->SetVisible(0);
        }
        return;
    }

    if (g_settings_6850c8.continuous_combat == 0) {
        stance = g_combat_state->combat_over_000 != 0 ? 3U : 0U;
    } else if ((ClockIsTicking(g_combat_state->combat_ui_timer_7a8) == 0 &&
                CombatMayAdvanceContinuously() != 0) ||
               g_combat_state->party_surprised_a52 != 0) {
        stance =
            (static_cast<unsigned int>(-(g_combat_state->round_active_001 != 0)) & 0xfffffffdU) + 4;
    } else {
        stance = 2;
    }

    for (index = 0; index < 5; ++index) {
        g_combat_stance_buttons_69b89c[index]->SetVisible(index == stance);
    }
    if (gXStatus.fSpellCastMode == 0 && gXStatus.fItemSelectMode == 0 &&
        gXStatus.fReviewCharacterMode == 0) {
        enabled = 1;
    } else {
        enabled = 0;
    }
    g_combat_stance_buttons_69b89c[stance]->SetEnabled(enabled);
    g_combat_stance_buttons_69b89c[stance]->Draw();
}

/* Dirties the five combat-stance buttons and refreshes which one is live. */
// FUNCTION: WIZ8 0x00597D30
void RedrawCombatStanceButtons(void)
{
    W8DialogButton** button;

    DrawCatalogImageAndInvalidate(-0xe, 0x7e, 0, 3, 0x262, 0x1c2, 2, 0);
    for (button = g_combat_stance_buttons_69b89c; button < &g_combat_stance_buttons_69b89c[5];
         ++button) {
        (*button)->m_dirty = 1;
    }
    UpdateCombatStanceButtons();
}

// FUNCTION: WIZ8 0x00597E70
void MainGameCombatConfirmButton(W8DialogButton* button)
{
    if (gXStatus.fCombatMode == 0) {
        return;
    }
    if (g_combat_state->combat_over_000 != 0) {
        TogglePartyCombatStance();
        return;
    }
    BeginCombatExecution004E8370();
    if (g_settings_6850c8.continuous_combat == 0) {
        return;
    }
    if (ClockIsTicking(g_combat_state->combat_ui_timer_7a8) == 0) {
        return;
    }
    g_combat_state->combat_ui_timer_7a8 = SetCountdownClock(0);
}

// FUNCTION: WIZ8 0x00597ED0
void MainGameCombatStanceSecondary(W8DialogButton* button)
{
    if (gXStatus.fCombatMode != 0) {
        TogglePartyCombatStance();
    }
}

// FUNCTION: WIZ8 0x00597B90
unsigned char CreateCombatStanceButtons(void)
{
    int index;

    for (index = 0; index < 5; ++index) {
        g_combat_stance_buttons_69b89c[index] = new W8DialogButton;
        if (g_combat_stance_buttons_69b89c[index] == 0) {
            for (index = 0; index < 5; ++index) {
                if (g_combat_stance_buttons_69b89c[index] != 0) {
                    delete g_combat_stance_buttons_69b89c[index];
                    g_combat_stance_buttons_69b89c[index] = 0;
                }
            }
            return 0;
        }
    }
    g_combat_stance_buttons_69b89c[0]->Configure(g_attack_confirm_path_64c3b8, 3, 0, 1, 2, 2,
                                                 MainGameCombatConfirmButton, 0, 0, 0x7f, 0x4f,
                                                 MainGameCombatStanceSecondary, 0);
    g_combat_stance_buttons_69b89c[1]->Configure(g_combat_stop_path_64c3e0, 3, 0, 1, 2, 2,
                                                 MainGameCombatConfirmButton, 0, 0, 0x7f, 0x50,
                                                 MainGameCombatStanceSecondary, 0);
    g_combat_stance_buttons_69b89c[2]->Configure(g_cont_start_path_64c404, 3, 0, 1, 2, 2,
                                                 MainGameCombatConfirmButton, 0, 0, 0x7f, 0x4f,
                                                 MainGameCombatStanceSecondary, 0);
    g_combat_stance_buttons_69b89c[3]->Configure(g_cont_toggle_path_64c428, 3, 0, 1, 2, 2,
                                                 MainGameCombatConfirmButton, 0, 0, 0x7f, 0x51,
                                                 MainGameCombatStanceSecondary, 0);
    g_combat_stance_buttons_69b89c[4]->Configure(g_cont_pending_path_64c44c, 3, 0, 1, 2, 2,
                                                 MainGameCombatConfirmButton, 0, 0, 0x7f, 0x50,
                                                 MainGameCombatStanceSecondary, 0);
    for (index = 0; index < 5; ++index) {
        g_combat_stance_buttons_69b89c[index]->SetPosition(
            g_combat_stance_positions_64c478[index][0], g_combat_stance_positions_64c478[index][1]);
        g_combat_stance_buttons_69b89c[index]->m_owner_040 = 0;
    }
    return 1;
}

// FUNCTION: WIZ8 0x00598270
void MainGameRoofButton0(W8DialogButton* button)
{
    ApplyMainGameModeFlag(W8_MAIN_UI_MODE_PORTRAITS, 1);
    RequestRedraw(0x300);
}

// FUNCTION: WIZ8 0x00598290
void MainGameRoofButton1(W8DialogButton* button)
{
    ApplyMainGameModeFlag(W8_MAIN_UI_MODE_FORMATION, 1);
    RequestRedraw(0x300);
}

// FUNCTION: WIZ8 0x005982B0
void MainGameRoofButton2(W8DialogButton* button)
{
    ApplyMainGameModeFlag(W8_MAIN_UI_MODE_RADAR, 1);
    RequestRedraw(0x300);
}

// FUNCTION: WIZ8 0x00597EE0
unsigned char CreateRoofButtons(void)
{
    int index;

    for (index = 0; index < 3; ++index) {
        g_roof_buttons_69b8d8[index] = new W8DialogButton;
        if (g_roof_buttons_69b8d8[index] == 0) {
            for (index = 0; index < 3; ++index) {
                if (g_roof_buttons_69b8d8[index] != 0) {
                    delete g_roof_buttons_69b8d8[index];
                    g_roof_buttons_69b8d8[index] = 0;
                }
            }
            return 0;
        }
    }
    g_roof_buttons_69b8d8[0]->Configure(g_roof_buttons_path_64c4a0, 9, 0, 6, 3, 6,
                                        MainGameRoofButton0, 0, 1, 0x7f, 0x38, 0, 0);
    g_roof_buttons_69b8d8[1]->Configure(g_roof_buttons_path_64c4a0, 10, 1, 7, 4, 7,
                                        MainGameRoofButton1, 0, 1, 0x7f, 0x39, 0, 0);
    g_roof_buttons_69b8d8[2]->Configure(g_roof_buttons_path_64c4a0, 0xb, 2, 8, 5, 8,
                                        MainGameRoofButton2, 0, 1, 0x7f, 0x3a, 0, 0);
    if (g_settings_6850c8.main_ui_mode == W8_MAIN_UI_MODE_PORTRAITS) {
        g_roof_buttons_69b8d8[0]->SetPressed(1);
    } else if (g_settings_6850c8.main_ui_mode == W8_MAIN_UI_MODE_FORMATION) {
        g_roof_buttons_69b8d8[1]->SetPressed(1);
    } else if (g_settings_6850c8.main_ui_mode == W8_MAIN_UI_MODE_RADAR) {
        g_roof_buttons_69b8d8[2]->SetPressed(1);
    }
    for (index = 0; index < 3; ++index) {
        g_roof_buttons_69b8d8[index]->SetPosition(g_roof_button_positions_64c4d0[index][0],
                                                  g_roof_button_positions_64c4d0[index][1]);
        g_roof_buttons_69b8d8[index]->m_owner_040 = 0;
    }
    return 1;
}

/* Draw the roof chrome strip, mark each roof button dirty, sync enable/press
   state, and raise redraw bit 0x100000 for the second pass. */
// FUNCTION: WIZ8 0x00598060
void RedrawRoofButtons(void)
{
    W8DialogButton** button;

    DrawCatalogImage(-0xe, 0x85, 0, 0, 0, 0, 2, 0);
    for (button = g_roof_buttons_69b8d8; button < &g_options_disk_button_69b8e4; ++button) {
        (*button)->m_dirty = 1;
    }
    UpdateRoofButtons();
    RequestRedraw(0x100000);
}

/* Visibility and enablement for the three roof buttons, then Draw. The third
   button (radar) hides under NPC/spell/item/lock/trap modes; the first two
   disable under NPC dialogue or camp. */
// FUNCTION: WIZ8 0x005980B0
void UpdateRoofButtons(void)
{
    bool visible;
    bool enabled;
    W8DialogButton** button;

    if (gXStatus.fNpcDialogueMode == 0 && gXStatus.fSpellCastMode == 0 &&
        gXStatus.fItemSelectMode == 0 && gXStatus.fLockInteractMode == 0 &&
        gXStatus.fTrapInteractMode == 0) {
        visible = 1;
    } else {
        visible = 0;
    }
    g_roof_buttons_69b8d8[2]->SetVisible(visible);
    if (gXStatus.fNpcDialogueMode == 0 && gXStatus.fCampMode == 0) {
        g_roof_buttons_69b8d8[0]->SetEnabled(1);
        enabled = 1;
    } else {
        g_roof_buttons_69b8d8[0]->SetEnabled(0);
        enabled = 0;
    }
    g_roof_buttons_69b8d8[1]->SetEnabled(enabled);
    SyncRoofButtonPressedState();
    for (button = g_roof_buttons_69b8d8; button < &g_options_disk_button_69b8e4; ++button) {
        (*button)->Draw();
    }
}

/* Mirror g_settings_6850c8.main_ui_mode onto the three roof buttons' pressed
   state (portraits / formation / radar). */
// FUNCTION: WIZ8 0x00598150
void SyncRoofButtonPressedState(void)
{
    if (g_settings_6850c8.main_ui_mode == 0) {
        if (g_roof_buttons_69b8d8[0]->IsPressed() == 0) {
            g_roof_buttons_69b8d8[0]->SetPressed(1);
        }
        if (g_roof_buttons_69b8d8[1]->IsPressed() != 0) {
            g_roof_buttons_69b8d8[1]->SetPressed(0);
        }
        if (g_roof_buttons_69b8d8[2]->IsPressed() != 0) {
            g_roof_buttons_69b8d8[2]->SetPressed(0);
        }
    } else if (g_settings_6850c8.main_ui_mode == 1) {
        if (g_roof_buttons_69b8d8[1]->IsPressed() == 0) {
            g_roof_buttons_69b8d8[1]->SetPressed(1);
        }
        if (g_roof_buttons_69b8d8[0]->IsPressed() != 0) {
            g_roof_buttons_69b8d8[0]->SetPressed(0);
        }
        if (g_roof_buttons_69b8d8[2]->IsPressed() != 0) {
            g_roof_buttons_69b8d8[2]->SetPressed(0);
        }
    } else if (g_settings_6850c8.main_ui_mode == 2) {
        if (g_roof_buttons_69b8d8[2]->IsPressed() == 0) {
            g_roof_buttons_69b8d8[2]->SetPressed(1);
        }
        if (g_roof_buttons_69b8d8[0]->IsPressed() != 0) {
            g_roof_buttons_69b8d8[0]->SetPressed(0);
        }
        if (g_roof_buttons_69b8d8[1]->IsPressed() != 0) {
            g_roof_buttons_69b8d8[1]->SetPressed(0);
        }
    }
}

// FUNCTION: WIZ8 0x00598670
void MainGameLayoutRadarButton(W8DialogButton* button)
{
    if (g_level_block->radar_map_visible != 0) {
        if (g_level_block->formation_board_visible == 0 &&
            g_level_block->action_panel_visible == 0) {
            ApplyMainGameModeFlag(W8_MAIN_UI_MODE_RADAR, 1);
            return;
        }
        g_settings_6850c8.formation_radar_map_preference = 0;
        SetRadarMapVisible(0);
        return;
    }
    g_settings_6850c8.formation_radar_map_preference = 1;
    if (g_settings_6850c8.main_ui_mode == W8_MAIN_UI_MODE_RADAR) {
        g_settings_6850c8.formation_action_panel_preference = 0;
        g_settings_6850c8.formation_board_preference = 0;
        ApplyMainGameModeFlag(W8_MAIN_UI_MODE_FORMATION, 1);
        return;
    }
    SetRadarMapVisible(1);
}

// FUNCTION: WIZ8 0x005986E0
void MainGameLayoutActionPanelButton(W8DialogButton* button)
{
    if (g_level_block->action_panel_visible != 0) {
        if (g_settings_6850c8.main_ui_mode == W8_MAIN_UI_MODE_FORMATION) {
            if (g_level_block->radar_map_visible == 0 &&
                g_level_block->formation_board_visible == 0) {
                ApplyMainGameModeFlag(W8_MAIN_UI_MODE_RADAR, 1);
            }
            g_settings_6850c8.formation_action_panel_preference = 0;
            SetActionPanelVisible(0);
            return;
        }
        if (g_settings_6850c8.main_ui_mode == W8_MAIN_UI_MODE_PORTRAITS) {
            g_settings_6850c8.portraits_action_panel_preference = 0;
        }
        SetActionPanelVisible(0);
        return;
    }
    if (g_settings_6850c8.main_ui_mode == W8_MAIN_UI_MODE_RADAR) {
        g_settings_6850c8.formation_radar_map_preference = 0;
        g_settings_6850c8.formation_action_panel_preference = 1;
        g_settings_6850c8.formation_board_preference = 0;
        ApplyMainGameModeFlag(W8_MAIN_UI_MODE_FORMATION, 1);
        return;
    }
    if (g_settings_6850c8.main_ui_mode == W8_MAIN_UI_MODE_PORTRAITS) {
        g_settings_6850c8.portraits_action_panel_preference = 1;
        SetActionPanelVisible(1);
        return;
    }
    g_settings_6850c8.formation_action_panel_preference = 1;
    SetActionPanelVisible(1);
}

// FUNCTION: WIZ8 0x005987A0
void MainGameLayoutFormationButton(W8DialogButton* button)
{
    if (g_level_block->formation_board_visible != 0) {
        if (g_level_block->radar_map_visible == 0 && g_level_block->action_panel_visible == 0) {
            ApplyMainGameModeFlag(W8_MAIN_UI_MODE_RADAR, 1);
            return;
        }
        g_settings_6850c8.formation_board_preference = 0;
        SetFormationBoardVisible(0);
        return;
    }
    g_settings_6850c8.formation_board_preference = 1;
    if (g_settings_6850c8.main_ui_mode == W8_MAIN_UI_MODE_RADAR) {
        g_settings_6850c8.formation_radar_map_preference = 0;
        g_settings_6850c8.formation_action_panel_preference = 0;
        ApplyMainGameModeFlag(W8_MAIN_UI_MODE_FORMATION, 1);
        return;
    }
    SetFormationBoardVisible(1);
}

/* Draw the left layout-arrow strip, mark each arrow dirty, then raise/lower
   the paired arrows from the live action-panel / formation / radar flags.
   Modal NPC/spell/item/lock/trap modes hide the whole bank. */
// FUNCTION: WIZ8 0x00598490
void RedrawLayoutArrowButtons(void)
{
    W8DialogButton** button;
    bool enabled;
    W8DialogButton* draw_button;

    DrawCatalogImageAndInvalidate(-0xe, 0x7e, 0, 0, 0, 0x166, 2, 0);
    for (button = g_layout_arrow_buttons_69b884; button < &g_layout_arrow_buttons_69b884[6];
         ++button) {
        (*button)->m_dirty = 1;
    }
    if (gXStatus.fNpcDialogueMode != 0 || gXStatus.fSpellCastMode != 0 ||
        gXStatus.fItemSelectMode != 0 || gXStatus.fLockInteractMode != 0 ||
        gXStatus.fTrapInteractMode != 0) {
        for (button = g_layout_arrow_buttons_69b884; button < &g_layout_arrow_buttons_69b884[6];
             ++button) {
            (*button)->SetVisible(0);
        }
        return;
    }

    enabled = g_settings_6850c8.main_ui_mode != 0;
    if (enabled) {
        g_layout_arrow_buttons_69b884[0]->SetEnabled(1);
    } else {
        g_layout_arrow_buttons_69b884[0]->SetEnabled(0);
    }
    g_layout_arrow_buttons_69b884[2]->SetEnabled(enabled);

    if (g_level_block->radar_map_visible == 0) {
        g_layout_arrow_buttons_69b884[3]->SetVisible(1);
        g_layout_arrow_buttons_69b884[0]->SetVisible(0);
        draw_button = g_layout_arrow_buttons_69b884[3];
    } else {
        g_layout_arrow_buttons_69b884[0]->SetVisible(1);
        g_layout_arrow_buttons_69b884[3]->SetVisible(0);
        draw_button = g_layout_arrow_buttons_69b884[0];
    }
    draw_button->Draw();

    if (g_level_block->action_panel_visible == 0) {
        g_layout_arrow_buttons_69b884[4]->SetVisible(1);
        g_layout_arrow_buttons_69b884[1]->SetVisible(0);
        draw_button = g_layout_arrow_buttons_69b884[4];
    } else {
        g_layout_arrow_buttons_69b884[1]->SetVisible(1);
        g_layout_arrow_buttons_69b884[4]->SetVisible(0);
        draw_button = g_layout_arrow_buttons_69b884[1];
    }
    draw_button->Draw();

    if (g_level_block->formation_board_visible != 0) {
        g_layout_arrow_buttons_69b884[2]->SetVisible(1);
        g_layout_arrow_buttons_69b884[5]->SetVisible(0);
        g_layout_arrow_buttons_69b884[2]->Draw();
        return;
    }
    g_layout_arrow_buttons_69b884[5]->SetVisible(1);
    g_layout_arrow_buttons_69b884[2]->SetVisible(0);
    g_layout_arrow_buttons_69b884[5]->Draw();
}

// FUNCTION: WIZ8 0x005982D0
unsigned char CreateLayoutArrowButtons(void)
{
    int index;

    for (index = 0; index < 6; ++index) {
        g_layout_arrow_buttons_69b884[index] = new W8DialogButton;
        if (g_layout_arrow_buttons_69b884[index] == 0) {
            for (index = 0; index < 6; ++index) {
                if (g_layout_arrow_buttons_69b884[index] != 0) {
                    delete g_layout_arrow_buttons_69b884[index];
                    g_layout_arrow_buttons_69b884[index] = 0;
                }
            }
            return 0;
        }
    }
    g_layout_arrow_buttons_69b884[0]->Configure(g_layout_arrows_path_64c4e8, 6, 0, 1, -1, 2,
                                                MainGameLayoutRadarButton, 0, 0, 0x7f, 0x3e, 0, 0);
    g_layout_arrow_buttons_69b884[1]->Configure(g_layout_arrows_path_64c4e8, 0xe, 8, 9, -1, 10,
                                                MainGameLayoutActionPanelButton, 0, 0, 0x7f, 0x3f,
                                                0, 0);
    g_layout_arrow_buttons_69b884[2]->Configure(g_layout_arrows_path_64c4e8, 0x16, 0x10, 0x11, -1,
                                                0x12, MainGameLayoutFormationButton, 0, 0, 0x7f,
                                                0x40, 0, 0);
    g_layout_arrow_buttons_69b884[3]->Configure(g_layout_arrows_path_64c4e8, 7, 3, 4, -1, 5,
                                                MainGameLayoutRadarButton, 0, 0, 0x7f, 0x3b, 0, 0);
    g_layout_arrow_buttons_69b884[4]->Configure(g_layout_arrows_path_64c4e8, 0xf, 0xb, 0xc, -1, 0xd,
                                                MainGameLayoutActionPanelButton, 0, 0, 0x7f, 0x3c,
                                                0, 0);
    g_layout_arrow_buttons_69b884[5]->Configure(g_layout_arrows_path_64c4e8, 0x17, 0x13, 0x14, -1,
                                                0x15, MainGameLayoutFormationButton, 0, 0, 0x7f,
                                                0x3d, 0, 0);
    for (index = 0; index < 6; ++index) {
        g_layout_arrow_buttons_69b884[index]->SetPosition(
            g_layout_arrow_positions_64c518[index][0], g_layout_arrow_positions_64c518[index][1]);
        g_layout_arrow_buttons_69b884[index]->m_owner_040 = 0;
    }
    return 1;
}

// FUNCTION: WIZ8 0x00598AB0
void CreateMainGameInterfaceButtons(void)
{
    CreateSubMenuButtons();
    CreateSubMenuScrollButtons();
    CreateSubMenuPanelButtons();
    CreateOptionsDiskButton();
    CreateCombatStanceButtons();
    CreateRoofButtons();
    CreateLayoutArrowButtons();
}

/* Free every button bank CreateMainGameInterfaceButtons built, plus the submenu
   panel and row controls created with it. Every slot is nulled as it is
   released so a second pass is harmless. */
// FUNCTION: WIZ8 0x00598AE0
void DestroyMainGameInterfaceButtons(void)
{
    int index;

    for (index = 0; index < 9; ++index) {
        if (g_submenu_buttons_69b8b0[index] != 0) {
            delete g_submenu_buttons_69b8b0[index];
            g_submenu_buttons_69b8b0[index] = 0;
        }
    }
    for (index = 0; index < 2; ++index) {
        if (g_submenu_scroll_buttons_69b858[index] != 0) {
            delete g_submenu_scroll_buttons_69b858[index];
            g_submenu_scroll_buttons_69b858[index] = 0;
        }
    }
    for (index = 0; index < 2; ++index) {
        if (g_submenu_panel_buttons_69b860[index] != 0) {
            delete g_submenu_panel_buttons_69b860[index];
            g_submenu_panel_buttons_69b860[index] = 0;
        }
    }
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
    if (g_options_disk_button_69b8e4 != 0) {
        delete g_options_disk_button_69b8e4;
        g_options_disk_button_69b8e4 = 0;
    }
    for (index = 0; index < 5; ++index) {
        if (g_combat_stance_buttons_69b89c[index] != 0) {
            delete g_combat_stance_buttons_69b89c[index];
            g_combat_stance_buttons_69b89c[index] = 0;
        }
    }
    for (index = 0; index < 3; ++index) {
        if (g_roof_buttons_69b8d8[index] != 0) {
            delete g_roof_buttons_69b8d8[index];
            g_roof_buttons_69b8d8[index] = 0;
        }
    }
    for (index = 0; index < 6; ++index) {
        if (g_layout_arrow_buttons_69b884[index] != 0) {
            delete g_layout_arrow_buttons_69b884[index];
            g_layout_arrow_buttons_69b884[index] = 0;
        }
    }
}

/* Re-enable the button banks the surprise sequence took down: submenu scroll
   arrows, the options disk, roof buttons and the layout arrows. */
// FUNCTION: WIZ8 0x00598c10
void EnableMenuButtonBanks00598C10(void)
{
    int i;
    for (i = 0; i < 2; ++i) {
        g_submenu_scroll_buttons_69b858[i]->SetEnabled(1);
    }
    g_options_disk_button_69b8e4->SetEnabled(1);
    for (i = 0; i < 3; ++i) {
        g_roof_buttons_69b8d8[i]->SetEnabled(1);
    }
    for (i = 0; i < 6; ++i) {
        g_layout_arrow_buttons_69b884[i]->SetEnabled(1);
    }
}

/* Disable the same button banks while the surprise sequence runs. */
// FUNCTION: WIZ8 0x00598c70
void DisableMenuButtonBanks00598C70(void)
{
    int i;
    for (i = 0; i < 2; ++i) {
        g_submenu_scroll_buttons_69b858[i]->SetEnabled(0);
    }
    g_options_disk_button_69b8e4->SetEnabled(0);
    for (i = 0; i < 3; ++i) {
        g_roof_buttons_69b8d8[i]->SetEnabled(0);
    }
    for (i = 0; i < 6; ++i) {
        g_layout_arrow_buttons_69b884[i]->SetEnabled(0);
    }
}

// FUNCTION: WIZ8 0x00598CD0
unsigned char SubMenuBackgroundRegionEvent(const InputAtom* event, W8Region* region)
{
    unsigned short us_event;
    W8TextControl** row;

    if (g_level_block->combat_end_notification == -1) {
        return 0;
    }
    us_event = event->usEvent;
    if (us_event == RIGHT_BUTTON_DOWN) {
        return 1;
    }
    if (us_event == RIGHT_BUTTON_UP) {
        SetSubMenuButtonTooltips(1);
        g_level_block->combat_end_notification = -1;
        g_submenu_entry_count_69b87e = 0;
        RegionSetDisable(0x27);
        DisableRegionSetInput(0x27);
        if (gpSubMenuPanel != 0) {
            delete gpSubMenuPanel;
            gpSubMenuPanel = 0;
        }
        row = g_submenu_rows_69b8ec;
        do {
            if (*row != 0) {
                delete *row;
                *row = 0;
            }
            ++row;
        } while (row < &g_submenu_rows_69b8ec[5]);
        RequestRedraw(0x200);
        return 1;
    }
    if (us_event != MOUSE_POS) {
        return 0;
    }
    if ((region->flags & W8_REGION_MOUSE_LEAVE) != 0) {
        return 1;
    }
    return (region->flags >> 4) & 1;
}

// FUNCTION: WIZ8 0x00598DB0
unsigned char SubMenuRowRegionEvent(const InputAtom* event, W8Region* region)
{
    W8PartySlotRow* party_row;
    W8ItemInstance* item;
    wchar_t* name;
    int slot;

    if (g_level_block->combat_end_notification == -1) {
        return 0;
    }
    RequestRedraw(0x80000000);
    switch (event->usEvent) {
    case RIGHT_BUTTON_DOWN:
        g_submenu_rows_69b8ec[region->callback_id]->OnRightButtonDown(0);
        region->flags |= W8_REGION_RIGHT_BUTTON_HELD;
        return 1;
    case LEFT_BUTTON_DOWN:
        g_submenu_rows_69b8ec[region->callback_id]->OnLeftButtonDown(0);
        region->flags |= W8_REGION_LEFT_BUTTON_HELD;
        return 1;
    case LEFT_BUTTON_UP:
        g_submenu_rows_69b8ec[region->callback_id]->OnLeftButtonUp(0);
        if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) != 0) {
            region->flags &= ~W8_REGION_LEFT_BUTTON_HELD;
        }
        return 1;
    case RIGHT_BUTTON_UP:
        g_submenu_rows_69b8ec[region->callback_id]->OnRightButtonUp(0);
        if ((region->flags & W8_REGION_RIGHT_BUTTON_HELD) != 0) {
            region->flags &= ~W8_REGION_RIGHT_BUTTON_HELD;
        }
        return 1;
    case MOUSE_POS:
        if ((region->flags & W8_REGION_MOUSE_LEAVE) != 0) {
            g_submenu_rows_69b8ec[region->callback_id]->OnMouseLeave(0);
            return 1;
        }
        if ((region->flags & W8_REGION_MOUSE_ENTER) == 0) {
            return 0;
        }
        if (g_level_block->combat_end_notification == 3 &&
            g_submenu_entries_69b868[region->callback_id] == 2) {
            SetRegionHelpForceEnabled004F27C0(1);
            slot = g_status_685170.selected_character;
            if (CanPartySlotUseRecordedItem(slot) == 0) {
                SetRegionHelpText(gppStringList[0x174 / 4]);
            } else {
                party_row = &g_status_685170.buffers.XChar[slot];
                item = FindCharacterItemAt(slot, party_row->item_origin, party_row->item_slot);
                name = FormatItemDisplayName(item, 0);
                SetRegionHelpText(
                    FormatWideString(g_format_s_colon_s_0061c3e0, gppStringList[0x174 / 4], name));
            }
        }
        g_submenu_rows_69b8ec[region->callback_id]->OnMouseEnter(0);
        return 1;
    }
    return 0;
}

// FUNCTION: WIZ8 0x00596FE0
void DrawSubMenuCharacterAction(void)
{
    unsigned short slot;
    W8Character* character;
    W8PartySlotRow* row;
    int action;
    unsigned int monster_index;
    W8MonsterInfo* monster_info;
    wchar_t* name;
    wchar_t text[126];
    wchar_t second[126];
    INT16 width;
    INT16 separator;
    INT16 trailing;

    slot = (unsigned short)g_status_685170.selected_character;
    if (slot > 7) {
        return;
    }
    SetFont(g_smfnt_font_683694);
    row = &g_status_685170.buffers.XChar[slot];
    SetFontObjectPalette16BPP(g_smfnt_font_683694,
                              g_font_state_palettes_68ee1c[row->party_order_index]);
    DrawCatalogImageAndInvalidate(-0xe, 0x7e, 0, 6, 0x157, 0x1c2, 2, 0);
    character = &g_status_685170.buffers.Char[slot];
    swprintf(text, L"%s - %s", character->name,
             gppStringList[g_profession_name_message_ids_61e3f0[character->iProfession]]);
    gprintf((0xb9 - StringPixLength((UINT16*)text, g_smfnt_font_683694)) / 2 + 0x157, 0x1c6,
            (UINT16*)g_format_s_006068e4, text);
    if (gXStatus.fCombatMode != 1) {
        if (character->highest_condition == 0) {
            return;
        }
        swprintf(text, L"%s",
                 gppStringList[g_condition_notices_0061E570[character->highest_condition * 4]]);
    } else {
        action = row->action_03d;
        switch (action) {
        case 0:
            swprintf(text, L"%s - ", gppStringList[g_action_kind_message_ids_61e988[0]]);
            if (character->Hand[0].in_play != 0) {
                if (character->EquippedItem[6].iItemNo == -1) {
                    wcscat(text, gppStringList[0x16e0 / 4]);
                } else {
                    wcscat(text,
                           gppStringList[g_generic_item_name_notice[GetItemUnidentifiedNameIndex(
                               &character->EquippedItem[6])]]);
                }
            }
            if (character->Hand[1].in_play == 0) {
                if (character->Hand[0].in_play == 0) {
                    wcscat(text, gppStringList[0x16e0 / 4]);
                }
            } else {
                if (character->EquippedItem[7].iItemNo == -1) {
                    swprintf(second, L"%s", gppStringList[0x16e0 / 4]);
                } else {
                    swprintf(second, L"%s",
                             gppStringList[g_generic_item_name_notice[GetItemUnidentifiedNameIndex(
                                 &character->EquippedItem[7])]]);
                }
                if (character->Hand[0].in_play == 0) {
                    wcscat(text, second);
                } else {
                    width = StringPixLength((UINT16*)text, g_smfnt_font_683694);
                    separator = StringPixLength((UINT16*)L"/)", g_smfnt_font_683694);
                    trailing = StringPixLength((UINT16*)second, g_smfnt_font_683694);
                    if ((unsigned int)(trailing + width + separator) < 0xb9) {
                        wcscat(text, L"/");
                        wcscat(text, second);
                    }
                }
            }
            break;
        case 5:
            if (row->target_in_combat.iType == W8_TARGET_KIND_CHARACTER) {
                swprintf(text, L"%s - %s", gppStringList[g_action_kind_message_ids_61e988[5]],
                         g_status_685170.buffers.Char[row->target_in_combat.iChar].name);
            } else if (row->target_in_combat.iType == W8_TARGET_KIND_MONSTER) {
                monster_index = MonsterGetIndexByLocationID(0x7ed, MGSBUTTONS_CPP,
                                                            row->target_in_combat.iMonsterID, 1);
                monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
                name = GetMonsterName(monster_info, 0, 0);
                swprintf(text, L"%s - %s", gppStringList[g_action_kind_message_ids_61e988[5]],
                         name);
            } else {
                swprintf(text, L"%s ", gppStringList[g_action_kind_message_ids_61e988[5]]);
            }
            break;
        case 7:
            swprintf(text, L"%s - %s (%d)", gppStringList[g_action_kind_message_ids_61e988[7]],
                     g_spell_records[row->action_detail_041].display_name,
                     row->action_detail_045.spell.power_level);
            break;
        case 8:
            swprintf(text, L"%s - %s", gppStringList[g_action_kind_message_ids_61e988[8]],
                     g_spell_records[g_item_records[row->action_detail_045.item_use.item->iItemNo]
                                         .spell_id]
                         .display_name);
            break;
        case -1:
            wcscpy(text, gppStringList[0x1f84 / 4]);
            break;
        default:
            swprintf(text, L"%s ", gppStringList[g_action_kind_message_ids_61e988[action]]);
            break;
        }
    }
    gprintf((0xb9 - StringPixLength((UINT16*)text, g_smfnt_font_683694)) / 2 + 0x157, 0x1d1,
            (UINT16*)text);
}

/* Enable the fight/review panel buttons from combat engagement and mode
   flags, then Draw both. */
// FUNCTION: WIZ8 0x00597790
void UpdateSubMenuPanelButtons(void)
{
    bool enabled;
    W8DialogButton** button;

    if (gXStatus.fCombatMode == 0 || g_combat_state->round_active_001 == 0 ||
        gXStatus.fSurprisePossible != 0 || gXStatus.fLockInteractMode != 0 ||
        gXStatus.fTrapInteractMode != 0 || gXStatus.fNpcDialogueMode != 0 ||
        gXStatus.fSpellCastMode != 0 || gXStatus.fItemSelectMode != 0 ||
        g_level_block->combat_end_notification != -1) {
        enabled = 0;
    } else {
        enabled = AnyCharacterEngaged() != 0;
    }
    g_submenu_panel_buttons_69b860[0]->SetEnabled(enabled);

    if (gXStatus.fSurprisePossible == 0 && gXStatus.fLockInteractMode == 0 &&
        gXStatus.fTrapInteractMode == 0 && gXStatus.fNpcDialogueMode == 0 &&
        gXStatus.fSpellCastMode == 0 && gXStatus.fItemSelectMode == 0 &&
        g_level_block->combat_end_notification == -1 && AnyCharacterEngaged() != 0) {
        g_submenu_panel_buttons_69b860[1]->SetEnabled(1);
        if (gXStatus.fReviewCharacterMode == 0) {
            if (g_submenu_panel_buttons_69b860[1]->IsPressed() != 0) {
                g_submenu_panel_buttons_69b860[1]->SetPressed(0);
            }
        } else if (g_submenu_panel_buttons_69b860[1]->IsPressed() == 0) {
            g_submenu_panel_buttons_69b860[1]->SetPressed(1);
        }
    } else {
        g_submenu_panel_buttons_69b860[1]->SetEnabled(0);
    }

    for (button = g_submenu_panel_buttons_69b860; button < &g_submenu_panel_buttons_69b860[2];
         ++button) {
        (*button)->Draw();
    }
}

/* Redraw the bottom sub-menu strip: bank chrome, the nine bank buttons, panel
   chrome/buttons, character-action caption, and the scroll arrows. */
// FUNCTION: WIZ8 0x00598810
void RedrawSubMenuButtons(void)
{
    W8DialogButton** button;
    int index;
    int frame;

    DrawCatalogImageAndInvalidate(-0xe, 0x7e, 0, 4, 0x1e, 0x1c2, 2, 0);
    frame = gXStatus.fCombatMode == 0 ? 8 : 9;
    DrawCatalogImageAndInvalidate(-0xe, 0x7e, 0, frame, 0xc4, 0x1c2, 2, 0);

    for (button = g_submenu_buttons_69b8b0; button < &g_submenu_buttons_69b8b0[9]; ++button) {
        (*button)->m_dirty = 1;
    }
    for (index = 0, button = g_submenu_buttons_69b8b0; button < &g_submenu_buttons_69b8b0[9];
         ++button, ++index) {
        UpdateSubMenuButton(index);
        (*button)->Draw();
    }

    DrawCatalogImageAndInvalidate(-0xe, 0x7e, 0, 7, 0x210, 0x1c2, 2, 0);
    for (button = g_submenu_panel_buttons_69b860; button < &g_submenu_panel_buttons_69b860[2];
         ++button) {
        (*button)->m_dirty = 1;
    }
    UpdateSubMenuPanelButtons();

    DrawCatalogImageAndInvalidate(-0xe, 0x7e, 0, 5, 0x124, 0x1c2, 2, 0);
    DrawCatalogImageAndInvalidate(-0xe, 0x7e, 0, 6, 0x157, 0x1c2, 2, 0);
    if (g_status_685170.buffers.XChar[g_status_685170.selected_character].fOccupied) {
        DrawSubMenuCharacterAction();
    }

    for (button = g_submenu_scroll_buttons_69b858; button < &g_submenu_scroll_buttons_69b858[2];
         ++button) {
        (*button)->m_dirty = 1;
    }
    if (g_level_block->combat_end_notification == -1 && gXStatus.fNpcDialogueMode == 0 &&
        gXStatus.fCampMode == 0) {
        for (button = g_submenu_scroll_buttons_69b858; button < &g_submenu_scroll_buttons_69b858[2];
             ++button) {
            (*button)->SetEnabled(1);
        }
    } else {
        for (button = g_submenu_scroll_buttons_69b858; button < &g_submenu_scroll_buttons_69b858[2];
             ++button) {
            (*button)->SetEnabled(0);
        }
    }
    for (button = g_submenu_scroll_buttons_69b858; button < &g_submenu_scroll_buttons_69b858[2];
         ++button) {
        (*button)->Draw();
    }
}

// FUNCTION: WIZ8 0x00597550
void SubMenuScrollArrowUp(W8DialogButton* button)
{
    ScrollSubMenuCharacter(0);
}

// FUNCTION: WIZ8 0x00597560
void SubMenuScrollArrowDown(W8DialogButton* button)
{
    ScrollSubMenuCharacter(1);
}

/* The eligible-slot scan runs in two halves per direction: from the current
   slot to the edge, then wrapping from the far edge back to the current slot.
   The `slot != current` check inside the first half is unreachable for valid
   slots, but retail tests it anyway. */
// FUNCTION: WIZ8 0x00597570
void ScrollSubMenuCharacter(char direction)
{
    short current;
    short selected;
    short slot;

    current = static_cast<short>(g_status_685170.selected_character);
    selected = current;
    switch (direction) {
    case 1:
        for (slot = current + 1; slot < 8; ++slot) {
            if (IsPartySlotEligible00524A10(slot) == 0) {
                continue;
            }
            selected = slot;
            if (slot != current) {
                goto done;
            }
            break;
        }
        for (slot = 0; slot <= current; ++slot) {
            if (IsPartySlotEligible00524A10(slot) != 0) {
                selected = slot;
                goto done;
            }
        }
        break;
    case 0:
        for (slot = current - 1; slot >= 0; --slot) {
            if (IsPartySlotEligible00524A10(slot) == 0) {
                continue;
            }
            selected = slot;
            if (slot != current) {
                goto done;
            }
            break;
        }
        for (slot = 7; slot >= current; --slot) {
            if (IsPartySlotEligible00524A10(slot) != 0) {
                selected = slot;
                goto done;
            }
        }
        break;
    }
done:
    SelectPartyCharacter(selected);
    RequestRedraw(1 << current);
    RequestRedraw(1 << g_status_685170.selected_character);
    RequestRedraw(0x200000);
}

// FUNCTION: WIZ8 0x005990F0
void SetSubMenuButtonTooltips(int enabled)
{
    g_submenu_buttons_69b8b0[0]->SetTooltipEnabled(enabled);
    g_submenu_buttons_69b8b0[1]->SetTooltipEnabled(enabled);
    g_submenu_buttons_69b8b0[2]->SetTooltipEnabled(enabled);
    g_submenu_buttons_69b8b0[3]->SetTooltipEnabled(enabled);
    g_submenu_buttons_69b8b0[4]->SetTooltipEnabled(enabled);
    g_submenu_buttons_69b8b0[5]->SetTooltipEnabled(enabled);
    g_submenu_buttons_69b8b0[6]->SetTooltipEnabled(enabled);
    g_submenu_buttons_69b8b0[7]->SetTooltipEnabled(enabled);
    g_submenu_buttons_69b8b0[8]->SetTooltipEnabled(enabled);
    g_submenu_scroll_buttons_69b858[0]->SetTooltipEnabled(enabled);
    g_submenu_scroll_buttons_69b858[1]->SetTooltipEnabled(enabled);
    g_submenu_panel_buttons_69b860[0]->SetTooltipEnabled(enabled);
    g_submenu_panel_buttons_69b860[1]->SetTooltipEnabled(enabled);
}

/* Per-frame button refresh from DrawMainGameScreen: restate and draw the nine
   bank buttons, gate the scroll arrows on an idle submenu, enable the
   options/disk button outside modal modes, then refresh the panel, stance,
   roof and layout-arrow sets. */
// FUNCTION: WIZ8 0x005989B0
void UpdateMainGameButtons005989B0(void)
{
    W8DialogButton** button;
    int index;
    bool enabled;

    for (index = 0, button = g_submenu_buttons_69b8b0; button < &g_submenu_buttons_69b8b0[9];
         ++button, ++index) {
        UpdateSubMenuButton(index);
        (*button)->Draw();
    }
    if (g_level_block->combat_end_notification == -1 && gXStatus.fNpcDialogueMode == 0 &&
        gXStatus.fCampMode == 0) {
        for (button = g_submenu_scroll_buttons_69b858; button < &g_submenu_scroll_buttons_69b858[2];
             ++button) {
            (*button)->SetEnabled(1);
        }
    } else {
        for (button = g_submenu_scroll_buttons_69b858; button < &g_submenu_scroll_buttons_69b858[2];
             ++button) {
            (*button)->SetEnabled(0);
        }
    }
    for (button = g_submenu_scroll_buttons_69b858; button < &g_submenu_scroll_buttons_69b858[2];
         ++button) {
        (*button)->Draw();
    }
    UpdateSubMenuPanelButtons();
    enabled = gXStatus.fNpcDialogueMode == 0 && gXStatus.fLockInteractMode == 0 &&
              gXStatus.fTrapInteractMode == 0 && gXStatus.fCampMode == 0 &&
              gXStatus.fLockInteract == 0 && gXStatus.fTrapInteract == 0;
    g_options_disk_button_69b8e4->SetEnabled(enabled);
    g_options_disk_button_69b8e4->Draw();
    UpdateCombatStanceButtons();
    UpdateRoofButtons();
    if (gXStatus.fNpcDialogueMode == 0 && gXStatus.fSpellCastMode == 0 &&
        gXStatus.fItemSelectMode == 0 && gXStatus.fLockInteractMode == 0 &&
        gXStatus.fTrapInteractMode == 0) {
        W8DialogButton* draw;
        bool arrow_enabled;

        arrow_enabled = g_settings_6850c8.main_ui_mode != W8_MAIN_UI_MODE_PORTRAITS;
        if (arrow_enabled) {
            g_layout_arrow_buttons_69b884[0]->SetEnabled(1);
        } else {
            g_layout_arrow_buttons_69b884[0]->SetEnabled(0);
        }
        g_layout_arrow_buttons_69b884[2]->SetEnabled(arrow_enabled);
        if (g_level_block->radar_map_visible == 0) {
            g_layout_arrow_buttons_69b884[3]->SetVisible(1);
            g_layout_arrow_buttons_69b884[0]->SetVisible(0);
            draw = g_layout_arrow_buttons_69b884[3];
        } else {
            g_layout_arrow_buttons_69b884[0]->SetVisible(1);
            g_layout_arrow_buttons_69b884[3]->SetVisible(0);
            draw = g_layout_arrow_buttons_69b884[0];
        }
        draw->Draw();
        if (g_level_block->action_panel_visible == 0) {
            g_layout_arrow_buttons_69b884[4]->SetVisible(1);
            g_layout_arrow_buttons_69b884[1]->SetVisible(0);
            draw = g_layout_arrow_buttons_69b884[4];
        } else {
            g_layout_arrow_buttons_69b884[1]->SetVisible(1);
            g_layout_arrow_buttons_69b884[4]->SetVisible(0);
            draw = g_layout_arrow_buttons_69b884[1];
        }
        draw->Draw();
        if (g_level_block->formation_board_visible == 0) {
            g_layout_arrow_buttons_69b884[5]->SetVisible(1);
            g_layout_arrow_buttons_69b884[2]->SetVisible(0);
            g_layout_arrow_buttons_69b884[5]->Draw();
            return;
        }
        g_layout_arrow_buttons_69b884[2]->SetVisible(1);
        g_layout_arrow_buttons_69b884[5]->SetVisible(0);
        g_layout_arrow_buttons_69b884[2]->Draw();
        return;
    }
    for (button = g_layout_arrow_buttons_69b884; button < &g_layout_arrow_buttons_69b884[6];
         ++button) {
        (*button)->SetVisible(0);
    }
}

/* While the combat-end submenu is up and the cursor has left its row band,
   run a 500 ms countdown; once it lapses, tear the panel and rows down like
   the background right-click path. */
// FUNCTION: WIZ8 0x00598FA0
void UpdateSubMenuAutoClose00598FA0(void)
{
    int left;
    int right;
    W8TextControl** row;

    left = gpSubMenuPanel->origin_x;
    if (g_submenu_entry_count_69b87e != 0 && g_submenu_entry_count_69b87e != 1) {
        switch (g_submenu_entry_count_69b87e) {
        case 2:
            right = left + 0x2f;
            break;
        case 3:
            right = left + 0x42;
            break;
        case 4:
            right = left + 0x55;
            break;
        case 5:
            right = left + 0x67;
        }
        if (IsCursorInRectangle(left, gpSubMenuPanel->origin_y, right,
                                gpSubMenuPanel->origin_y + 0x1c) != 0) {
            if (g_submenu_flag_69b8d4 != 0) {
                g_submenu_clock_69b880 = SetCountdownClock(0);
                g_submenu_flag_69b8d4 = 0;
            }
            return;
        }
    }

    if (g_submenu_flag_69b8d4 == 0) {
        g_submenu_clock_69b880 = SetCountdownClock(500);
        g_submenu_flag_69b8d4 = 1;
        return;
    }
    if (ClockIsTicking(g_submenu_clock_69b880) == 0) {
        SetSubMenuButtonTooltips(1);
        g_level_block->combat_end_notification = -1;
        g_submenu_entry_count_69b87e = 0;
        RegionSetDisable(0x27);
        DisableRegionSetInput(0x27);
        if (gpSubMenuPanel != 0) {
            delete gpSubMenuPanel;
            gpSubMenuPanel = 0;
        }
        row = g_submenu_rows_69b8ec;
        do {
            if (*row != 0) {
                delete *row;
                *row = 0;
            }
            ++row;
        } while (row < &g_submenu_rows_69b8ec[5]);
        RequestRedraw(0x200);
    }
}
