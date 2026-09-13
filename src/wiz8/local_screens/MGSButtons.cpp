#include "wiz8/local_screens/MGSButtons.h"
#include "wiz8/character.h"
#include "wiz8/combat_state.h"
#include "wiz8/dialog_code/DialogButton.h"
#include "wiz8/fonts.h"
#include "wiz8/game_status.h"
#include "wiz8/layouts/item_tables.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/Controls.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_code/TextControl.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/magic.h"
#include "wiz8/regions.h"
#include "wiz8/sr_api.h"
#include "wiz8/targeting.h"
#include "wiz8/video_object_catalog.h"
#include "wiz8/xstatus.h"

#include "Font.h"
#include "timer.h"

#include <stdio.h>

#define MGSBUTTONS_CPP "C:\\Projects\\Wizardry 8\\Local Screens\\MGSButtons.cpp"

/* The (x, y) of the two scroll arrows. */
// GLOBAL: WIZ8 0x0064C330
const int g_scroll_button_positions_64c330[2][2] = {{300, 456}, {323, 456}};

// GLOBAL: WIZ8 0x0069B858
W8DialogButton* g_submenu_scroll_buttons_69b858[2];
// GLOBAL: WIZ8 0x0069B87E
short g_submenu_entry_count_69b87e;
// GLOBAL: WIZ8 0x0069B880
TIMER g_submenu_clock_69b880;
// GLOBAL: WIZ8 0x0069B8D4
unsigned char g_submenu_flag_69b8d4;
/* The name is proven by this file's own assertion. */
// GLOBAL: WIZ8 0x0069B8E8
Controls* gpSubMenuPanel;
/* The five caption rows; the gap builder at 0x00595850 news 0xb8-byte objects
   through the W8TextControl constructor. */
// GLOBAL: WIZ8 0x0069B8EC
W8TextControl* g_submenu_rows_69b8ec[5];

/* Drop the combat-end notification and tear down the panel and its rows.
   The reset body compiles this three times - before the rebuild, again while
   the notification is still live, and once more when the rebuild fails. */
static __forceinline void DestroySubMenuControls(void)
{
    int i;

    Function5990F0(1);
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

// FUNCTION: WIZ8 0x005963E0
void RefreshSubMenuPanel005963E0(char invalidate)
{
    if (gpSubMenuPanel == 0) {
        srAssertFail("gpSubMenuPanel", MGSBUTTONS_CPP, 0x64f, 0);
    }
    if (invalidate != 0) {
        gpSubMenuPanel->Invalidate(0);
    }
    gpSubMenuPanel->Redraw();
}

// FUNCTION: WIZ8 0x00596CF0
void ResetSubMenuPanel00596CF0(void)
{
    short saved_notification;
    char rebuilt;

    saved_notification = g_level_block->combat_end_notification;
    DestroySubMenuControls();
    if (g_level_block->combat_end_notification != -1) {
        DestroySubMenuControls();
    }
    UpdateScreenOverlays(0);
    rebuilt = Function595850(saved_notification);
    if (rebuilt == 0) {
        DestroySubMenuControls();
    }
    Function5990F0(0);
    g_submenu_clock_69b880 = SetCountdownClock(0);
    g_submenu_flag_69b8d4 = 0;
    RequestRedraw(0x200);
}

// FUNCTION: WIZ8 0x00596EC0
unsigned char CreateSubMenuScrollButtons00596EC0(void)
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
                                                  0, 1, 2, 2, SubMenuScrollArrowUp00597550, 0, 0,
                                                  0x7f, 0x4b, 0, 0);
    g_submenu_scroll_buttons_69b858[1]->Configure("Data\\Main Interface\\main_scroll_arrows.sti", 7,
                                                  4, 5, 6, 6, SubMenuScrollArrowDown00597560, 0, 0,
                                                  0x7f, 0x4c, 0, 0);
    for (i = 0; i < 2; ++i) {
        g_submenu_scroll_buttons_69b858[i]->SetPosition(g_scroll_button_positions_64c330[i][0],
                                                        g_scroll_button_positions_64c330[i][1]);
        g_submenu_scroll_buttons_69b858[i]->m_owner_040 = 0;
    }
    return 1;
}

// FUNCTION: WIZ8 0x00596FE0
void DrawSubMenuCharacterAction00596FE0(void)
{
    unsigned short slot;
    W8Character* character;
    W8PartySlotRow* row;
    int action;
    unsigned int monster_index;
    W8MonsterInfo* monster_info;
    W8WideChar* name;
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
    row = &g_status_685170.buffers.party_rows[slot];
    SetFontObjectPalette16BPP(g_smfnt_font_683694,
                              g_font_state_palettes_68ee1c[row->party_order_0f1]);
    DrawCatalogImageAndInvalidate(-0xe, 0x7e, 0, 6, 0x157, 0x1c2, 2, 0);
    character = &g_status_685170.buffers.characters[slot];
    swprintf(text, L"%s - %s", character->name,
             gppStringList[g_profession_name_message_ids_61e3f0[character->current_profession]]);
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
            if (character->hand_attacks[0].in_play != 0) {
                if (character->equipment[6].item_id == -1) {
                    wcscat(text, gppStringList[0x16e0 / 4]);
                } else {
                    wcscat(text,
                           gppStringList[g_generic_item_name_notice[GetItemUnidentifiedNameIndex(
                               &character->equipment[6])]]);
                }
            }
            if (character->hand_attacks[1].in_play == 0) {
                if (character->hand_attacks[0].in_play == 0) {
                    wcscat(text, gppStringList[0x16e0 / 4]);
                }
            } else {
                if (character->equipment[7].item_id == -1) {
                    swprintf(second, L"%s", gppStringList[0x16e0 / 4]);
                } else {
                    swprintf(second, L"%s",
                             gppStringList[g_generic_item_name_notice[GetItemUnidentifiedNameIndex(
                                 &character->equipment[7])]]);
                }
                if (character->hand_attacks[0].in_play == 0) {
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
                         g_status_685170.buffers.characters[row->target_in_combat.iChar].name);
            } else if (row->target_in_combat.iType == W8_TARGET_KIND_MONSTER) {
                monster_index = MonsterGetIndexByLocationID(0x7ed, MGSBUTTONS_CPP,
                                                            row->target_in_combat.iMonsterID, 1);
                monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
                name = GetMonsterName(monster_info, 0, 0);
                swprintf(text, L"%s - %s", gppStringList[g_action_kind_message_ids_61e988[5]],
                         name);
            } else {
                swprintf(text, L"%s", gppStringList[g_action_kind_message_ids_61e988[5]]);
            }
            break;
        case 7:
            swprintf(text, L"%s - %s (%d)", gppStringList[g_action_kind_message_ids_61e988[7]],
                     g_spell_records[row->action_detail_041].display_name,
                     row->action_detail_045.spell.power_level);
            break;
        case 8:
            swprintf(text, L"%s - %s", gppStringList[g_action_kind_message_ids_61e988[8]],
                     g_spell_records[g_item_records[row->action_detail_045.item_use.item->item_id]
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
