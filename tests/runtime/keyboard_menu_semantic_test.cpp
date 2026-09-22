/* In-process semantic scenario for the keyboard-menu availability refresh
   recovered in MGSKeyboard.cpp. The scenario runs on the game thread once
   the main menu is live.

   It builds the twelve-row refresh set the way BuildKeyboardMenu leaves it -
   page/item pairs in g_keyboard_menu_pages_69b808 /
   g_keyboard_menu_items_69b7ec and one W8TextControl per slot in
   g_keyboard_menu_rows_69b820 - then drives RefreshKeyboardMenuRows across an
   availability transition on the spells page's recorded-spell entry: the
   slot's recorded spell goes from castable to unaffordable, which flips
   GetSubMenuEntryState from W8_SUBMENU_ENTRY_USABLE to
   W8_SUBMENU_ENTRY_UNAVAILABLE. The unavailable path must SetEnabled(0) the
   row; the regression this guards left the row enabled and copied an
   uninitialized icon index into its sprites.

   The remaining rows use the move page, whose entries are unconditionally
   usable, so no combat state is needed. The default-constructed controls have
   no panel or region, which makes Invalidate/SetActive/SetEnabled inert
   except for the observable flags and sprite fields.

   At main-menu time the spell database only holds the placeholder record, so
   the scenario fabricates the fields the recorded-spell predicates read on a
   fixed record and restores them afterwards. */

#include "keyboard_menu_semantic_test.h"

#include "wiz8/layouts/character.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/layouts/item_tables.h"
#include "wiz8/layouts/targeting.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/TextControl.h"
#include "wiz8/local_screens/MGSButtons.h"
#include "wiz8/local_screens/MGSKeyboard.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/xstatus.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { KEYBOARD_MENU_ROW_COUNT = 12, TEST_SPELL_ID = 1, TEST_SPELL_COST = 4, TEST_ITEM_ID = 1 };

bool RunKeyboardMenuSemanticTest(KeyboardMenuSemanticResult* result)
{
    W8TextControl* rows[KEYBOARD_MENU_ROW_COUNT];
    W8LevelRuntimeBlock* saved_level_block;
    W8LevelRuntimeBlock* test_level_block;
    int saved_menu_slot;
    unsigned char saved_combat_mode;
    int saved_spell_id;
    int saved_power_level;
    int saved_spell_learned;
    int saved_sp_left;
    int saved_normal_sprite;
    int saved_alternate_sprite;
    W8SpellUsage saved_usable_when;
    W8SpellRealm saved_realm;
    int saved_spell_point_cost;
    int saved_selected;
    int saved_rotation_mode;
    int saved_cursor;
    int saved_confirmations;
    unsigned char saved_occupied1;
    unsigned int saved_condition1;
    unsigned char saved_entry_flag;
    unsigned char saved_keyboard_open;
    unsigned char saved_pending0;
    unsigned char saved_flag_218;
    unsigned char saved_cursor_grace_31c;
    unsigned char saved_pick_changed;
    unsigned char saved_refresh_combat;
    unsigned char saved_refresh_party;
    unsigned char saved_settled;
    short saved_combat_notification;
    int saved_combat_slot;
    int saved_hover_slot;
    int saved_selection_kind;
    int saved_value_2f4;
    unsigned int saved_clock_214;
    W8ItemInstance saved_backpack_item;
    W8ItemInstance* saved_item_use_ptr;
    W8ItemDatabaseRecord saved_item_record;
    W8Profession saved_profession0;
    W8Gender saved_gender0;
    int saved_race0;
    int saved_item_id_0c9;
    int saved_item_target_char;
    W8SpellTargetType saved_target_type;
    unsigned char saved_item_origin;
    unsigned char saved_camp_mode;
    unsigned char saved_lock_interact;
    unsigned char saved_trap_interact;
    unsigned short saved_item_slot;
    unsigned int saved_condition8;
    const int spell_id = TEST_SPELL_ID;
    const int item_id = TEST_ITEM_ID;
    const int realm = 0;
    int index;

    memset(result, 0, sizeof(*result));
    memset(rows, 0, sizeof(rows));
    if (g_spell_records == 0) {
        return false;
    }
    result->spell_record = 1;

    test_level_block = 0;
    saved_level_block = g_level_block;
    if (g_level_block == 0) {
        test_level_block = static_cast<W8LevelRuntimeBlock*>(malloc(sizeof(W8LevelRuntimeBlock)));
        if (test_level_block == 0) {
            return false;
        }
        memset(test_level_block, 0, sizeof(*test_level_block));
        g_level_block = test_level_block;
    }

    /* Slot zero's recorded spell is made genuinely castable, then its realm
       pool is emptied. In combat both recorded-spell predicates stop at the
       cost comparison, so fCombatMode keeps the target-validity half out of
       the scenario. */
    saved_combat_mode = gXStatus.fCombatMode;
    saved_menu_slot = g_value_64c1c8;
    saved_spell_id = g_status_685170.buffers.XChar[0].spell_id;
    saved_power_level = g_status_685170.buffers.XChar[0].spell_detail.spell.power_level;
    saved_spell_learned = g_status_685170.buffers.Char[0].spell_learned[spell_id];
    saved_sp_left = g_status_685170.buffers.Char[0].iSPLeft[realm];
    saved_usable_when = g_spell_records[spell_id].usable_when;
    saved_realm = g_spell_records[spell_id].realm;
    saved_spell_point_cost = g_spell_records[spell_id].spell_point_cost;

    gXStatus.fCombatMode = 1;
    g_value_64c1c8 = 0;
    g_status_685170.buffers.XChar[0].spell_id = spell_id;
    g_status_685170.buffers.XChar[0].spell_detail.spell.power_level = 1;
    g_status_685170.buffers.Char[0].spell_learned[spell_id] = 1;
    g_status_685170.buffers.Char[0].iSPLeft[realm] = TEST_SPELL_COST * 2;
    g_spell_records[spell_id].usable_when = W8_SPELL_USABLE_IN_COMBAT;
    g_spell_records[spell_id].realm = static_cast<W8SpellRealm>(realm);
    g_spell_records[spell_id].spell_point_cost = TEST_SPELL_COST;

    for (index = 0; index < KEYBOARD_MENU_ROW_COUNT; ++index) {
        rows[index] = new W8TextControl();
        if (rows[index] == 0) {
            break;
        }
        g_keyboard_menu_rows_69b820[index] = rows[index];
        if (index == 0) {
            g_keyboard_menu_pages_69b808[index] = W8_SUBMENU_SPELLS;
            g_keyboard_menu_items_69b7ec[index] = 1;
        } else {
            g_keyboard_menu_pages_69b808[index] = W8_SUBMENU_MOVE;
            g_keyboard_menu_items_69b7ec[index] = (short)(index & 1);
        }
    }
    if (index != KEYBOARD_MENU_ROW_COUNT) {
        for (index = 0; index < KEYBOARD_MENU_ROW_COUNT; ++index) {
            if (g_keyboard_menu_rows_69b820[index] != 0) {
                delete g_keyboard_menu_rows_69b820[index];
                g_keyboard_menu_rows_69b820[index] = 0;
            }
        }
        g_status_685170.buffers.XChar[0].spell_id = saved_spell_id;
        g_status_685170.buffers.XChar[0].spell_detail.spell.power_level = saved_power_level;
        g_status_685170.buffers.Char[0].spell_learned[spell_id] = saved_spell_learned;
        g_status_685170.buffers.Char[0].iSPLeft[realm] = saved_sp_left;
        g_spell_records[spell_id].usable_when = saved_usable_when;
        g_spell_records[spell_id].realm = saved_realm;
        g_spell_records[spell_id].spell_point_cost = saved_spell_point_cost;
        g_value_64c1c8 = saved_menu_slot;
        gXStatus.fCombatMode = saved_combat_mode;
        if (test_level_block != 0) {
            g_level_block = saved_level_block;
            free(test_level_block);
        }
        return false;
    }

    RefreshKeyboardMenuRows();
    result->entry_initially_enabled =
        rows[0] != 0 && rows[0]->m_enabled != 0 && rows[0]->m_active != 0;
    saved_normal_sprite = rows[0]->m_normalSprite;
    saved_alternate_sprite = rows[0]->m_alternateNormalSprite;

    /* The transition: the realm pool no longer covers the recorded spell, so
       the entry's state becomes W8_SUBMENU_ENTRY_UNAVAILABLE while the menu
       stays up. */
    g_status_685170.buffers.Char[0].iSPLeft[realm] = 0;
    RefreshKeyboardMenuRows();

    result->entry_disabled_after_transition = rows[0]->m_enabled == 0;
    result->sprites_untouched_after_transition =
        rows[0]->m_normalSprite == saved_normal_sprite &&
        rows[0]->m_alternateNormalSprite == saved_alternate_sprite;
    result->other_row_still_enabled = rows[1] != 0 && rows[1]->m_enabled != 0;

    /* CloseKeyboardMenu: the open-menu state as its callers leave it - the
       slot's monster-entry flag raised, keyboard_menu_open set, combat and
       hover slots populated and the twelve rows live. portrait_refresh_pending
       stays raised so the surface-rect path, which needs a real panel, is
       skipped; the row deletes and the field clears are the observable part. */
    saved_entry_flag = gXStatus.monster_manager_entries[0].keyboard_menu_open;
    saved_keyboard_open = g_level_block->keyboard_menu_open;
    saved_combat_slot = g_level_block->combat_slot;
    saved_hover_slot = g_level_block->hover_combat_slot;
    saved_cursor_grace_31c = g_level_block->cursor_grace_31c;
    saved_pending0 = g_level_block->portrait_refresh_pending[0];

    gXStatus.monster_manager_entries[0].keyboard_menu_open = 1;
    g_level_block->keyboard_menu_open = 1;
    g_level_block->combat_slot = 3;
    g_level_block->cursor_grace_31c = 1;
    g_level_block->portrait_refresh_pending[0] = 1;
    CloseKeyboardMenu();
    result->close_cleared_open_flag = g_level_block->keyboard_menu_open == 0;
    result->close_cleared_slot_flag = gXStatus.monster_manager_entries[0].keyboard_menu_open == 0;
    result->close_deleted_rows =
        g_keyboard_menu_rows_69b820[0] == 0 && g_keyboard_menu_rows_69b820[11] == 0;
    result->close_reset_combat_slot =
        g_level_block->combat_slot == -1 && g_level_block->hover_combat_slot == 0;
    rows[0] = 0;
    rows[1] = 0;

    /* SelectPartyCharacter: slot one is made occupied and healthy while the
       camera is parked and every open-mode flag stays down, so the switch
       only moves the selection and raises the refresh bookkeeping. */
    saved_occupied1 = g_status_685170.buffers.XChar[1].fOccupied;
    saved_condition1 = g_status_685170.buffers.Char[1].highest_condition;
    saved_selected = g_status_685170.selected_character;
    saved_rotation_mode = g_settings_6850c8.camera_rotation_mode;
    saved_cursor = gXStatus.iCurrentCursor;
    saved_confirmations = g_settings_6850c8.pc_confirmations;
    saved_combat_notification = g_level_block->combat_end_notification;
    saved_flag_218 = g_level_block->flag_218;
    saved_refresh_combat = g_level_block->refresh_combat_panel;
    saved_refresh_party = g_level_block->refresh_party_panel;
    saved_pick_changed = g_level_block->pick_changed_154;
    saved_clock_214 = g_level_block->clock_214;

    g_status_685170.buffers.XChar[1].fOccupied = 1;
    g_status_685170.buffers.Char[1].highest_condition = 0;
    g_status_685170.selected_character = 0;
    g_settings_6850c8.camera_rotation_mode = 1;
    gXStatus.iCurrentCursor = -1;
    g_settings_6850c8.pc_confirmations = 0;
    g_level_block->combat_end_notification = -1;
    g_level_block->flag_218 = 0;
    g_level_block->refresh_combat_panel = 0;
    g_level_block->refresh_party_panel = 0;
    g_level_block->pick_changed_154 = 0;

    SelectPartyCharacter(1);
    result->select_moved_selection = g_status_685170.selected_character == 1;
    result->select_flagged_refresh =
        g_level_block->flag_218 != 0 && g_level_block->refresh_combat_panel != 0 &&
        g_level_block->refresh_party_panel != 0 && g_level_block->pick_changed_154 != 0;

    /* MapSubMenuSelection: an attack entry lands as the unsettled action
       while the recorded-spell entry settles it, both through the level
       block's selection fields. */
    saved_selection_kind = g_level_block->selection_kind;
    saved_value_2f4 = g_level_block->value_2f4;
    saved_settled = g_level_block->selection_settled;

    MapSubMenuSelection(W8_SUBMENU_ATTACK, 1);
    result->submenu_maps_action =
        g_level_block->selection_kind == W8_ACTION_BERSERK && g_level_block->selection_settled == 0;
    MapSubMenuSelection(W8_SUBMENU_SPELLS, 1);
    result->submenu_settles_recorded = g_level_block->selection_kind == W8_ACTION_CAST_SPELL &&
                                       g_level_block->selection_settled != 0;

    /* Recorded item: slot zero's backpack gains a usable consumable whose
       record is rewritten permissive - open masks, no requirements, a
       self-target spell - so the whole CanPartySlotUseRecordedItem chain
       answers yes. Then the instance empties and the entry must flip to
       unavailable. */
    saved_backpack_item = g_status_685170.buffers.Char[0].backpack[0];
    saved_item_record = g_item_records[item_id];
    saved_profession0 = g_status_685170.buffers.Char[0].iProfession;
    saved_gender0 = g_status_685170.buffers.Char[0].gender;
    saved_race0 = g_status_685170.buffers.Char[0].iRace;
    saved_condition8 =
        g_status_685170.buffers.Char[0].uiCondition[W8_CONDITION_SPELLCASTING_BLOCKED];
    saved_item_origin = g_status_685170.buffers.XChar[0].item_origin;
    saved_item_slot = g_status_685170.buffers.XChar[0].item_slot;
    saved_item_id_0c9 = g_status_685170.buffers.XChar[0].item_id_0c9;
    saved_item_target_char = g_status_685170.buffers.XChar[0].item_target.iChar;
    saved_item_use_ptr = g_status_685170.buffers.XChar[0].item_detail.item_use.item;
    saved_target_type = g_spell_records[spell_id].target_type;
    saved_camp_mode = gXStatus.fCampMode;
    saved_lock_interact = gXStatus.fLockInteract;
    saved_trap_interact = gXStatus.fTrapInteract;

    g_status_685170.buffers.Char[0].backpack[0].iItemNo = item_id;
    g_status_685170.buffers.Char[0].backpack[0].identified = 1;
    g_status_685170.buffers.Char[0].backpack[0].uses_or_charges = 1;
    g_status_685170.buffers.Char[0].iProfession = static_cast<W8Profession>(0);
    g_status_685170.buffers.Char[0].gender = static_cast<W8Gender>(0);
    g_status_685170.buffers.Char[0].iRace = 0;
    g_status_685170.buffers.Char[0].uiCondition[W8_CONDITION_SPELLCASTING_BLOCKED] = 0;
    g_status_685170.buffers.XChar[0].item_origin = W8_ITEM_ORIGIN_BACKPACK;
    g_status_685170.buffers.XChar[0].item_slot = 0;
    g_status_685170.buffers.XChar[0].item_id_0c9 = item_id;
    g_status_685170.buffers.XChar[0].item_target.iChar = 0;
    g_item_records[item_id].profession_mask = 0xffff;
    g_item_records[item_id].race_mask = 0xffffffff;
    g_item_records[item_id].gender_mask = 3;
    g_item_records[item_id].attribute_requirements[0].stat_id = 0xff;
    g_item_records[item_id].attribute_requirements[1].stat_id = 0xff;
    g_item_records[item_id].skill_requirements[0].stat_id = 0xff;
    g_item_records[item_id].skill_requirements[1].stat_id = 0xff;
    g_item_records[item_id].equip_class = 0x10;
    g_item_records[item_id].flags_041 = 0;
    g_item_records[item_id].category = 2;
    g_item_records[item_id].spell_id = spell_id;
    g_item_records[item_id].quantity_kind = 1;
    g_spell_records[spell_id].target_type = W8_TARGET_TYPE_CASTER;
    gXStatus.fCampMode = 0;
    gXStatus.fLockInteract = 0;
    gXStatus.fTrapInteract = 0;

    result->item_recorded_usable =
        GetSubMenuEntryState(W8_SUBMENU_ITEMS, 2, 0) == W8_SUBMENU_ENTRY_USABLE;

    g_status_685170.buffers.Char[0].backpack[0].iItemNo = -1;
    result->item_unavailable_after_loss =
        GetSubMenuEntryState(W8_SUBMENU_ITEMS, 2, 0) == W8_SUBMENU_ENTRY_UNAVAILABLE;

    g_status_685170.buffers.Char[0].backpack[0] = saved_backpack_item;
    g_item_records[item_id] = saved_item_record;
    g_status_685170.buffers.Char[0].iProfession = saved_profession0;
    g_status_685170.buffers.Char[0].gender = saved_gender0;
    g_status_685170.buffers.Char[0].iRace = saved_race0;
    g_status_685170.buffers.Char[0].uiCondition[W8_CONDITION_SPELLCASTING_BLOCKED] =
        saved_condition8;
    g_status_685170.buffers.XChar[0].item_origin = saved_item_origin;
    g_status_685170.buffers.XChar[0].item_slot = saved_item_slot;
    g_status_685170.buffers.XChar[0].item_id_0c9 = saved_item_id_0c9;
    g_status_685170.buffers.XChar[0].item_target.iChar = saved_item_target_char;
    g_status_685170.buffers.XChar[0].item_detail.item_use.item = saved_item_use_ptr;
    g_spell_records[spell_id].target_type = saved_target_type;
    gXStatus.fCampMode = saved_camp_mode;
    gXStatus.fLockInteract = saved_lock_interact;
    gXStatus.fTrapInteract = saved_trap_interact;

    g_level_block->selection_kind = saved_selection_kind;
    g_level_block->value_2f4 = saved_value_2f4;
    g_level_block->selection_settled = saved_settled;

    g_status_685170.buffers.XChar[1].fOccupied = saved_occupied1;
    g_status_685170.buffers.Char[1].highest_condition = saved_condition1;
    g_status_685170.selected_character = saved_selected;
    g_settings_6850c8.camera_rotation_mode = saved_rotation_mode;
    gXStatus.iCurrentCursor = saved_cursor;
    g_settings_6850c8.pc_confirmations = saved_confirmations;
    g_level_block->combat_end_notification = saved_combat_notification;
    g_level_block->flag_218 = saved_flag_218;
    g_level_block->refresh_combat_panel = saved_refresh_combat;
    g_level_block->refresh_party_panel = saved_refresh_party;
    g_level_block->pick_changed_154 = saved_pick_changed;
    g_level_block->clock_214 = saved_clock_214;

    gXStatus.monster_manager_entries[0].keyboard_menu_open = saved_entry_flag;
    g_level_block->keyboard_menu_open = saved_keyboard_open;
    g_level_block->combat_slot = saved_combat_slot;
    g_level_block->hover_combat_slot = saved_hover_slot;
    g_level_block->cursor_grace_31c = saved_cursor_grace_31c;
    g_level_block->portrait_refresh_pending[0] = saved_pending0;

    for (index = 0; index < KEYBOARD_MENU_ROW_COUNT; ++index) {
        if (g_keyboard_menu_rows_69b820[index] != 0) {
            delete g_keyboard_menu_rows_69b820[index];
            g_keyboard_menu_rows_69b820[index] = 0;
        }
        g_keyboard_menu_pages_69b808[index] = 0;
        g_keyboard_menu_items_69b7ec[index] = 0;
    }

    g_status_685170.buffers.XChar[0].spell_id = saved_spell_id;
    g_status_685170.buffers.XChar[0].spell_detail.spell.power_level = saved_power_level;
    g_status_685170.buffers.Char[0].spell_learned[spell_id] = saved_spell_learned;
    g_status_685170.buffers.Char[0].iSPLeft[realm] = saved_sp_left;
    g_spell_records[spell_id].usable_when = saved_usable_when;
    g_spell_records[spell_id].realm = saved_realm;
    g_spell_records[spell_id].spell_point_cost = saved_spell_point_cost;
    g_value_64c1c8 = saved_menu_slot;
    gXStatus.fCombatMode = saved_combat_mode;
    if (test_level_block != 0) {
        g_level_block = saved_level_block;
        free(test_level_block);
    }

    return result->entry_initially_enabled != 0 && result->entry_disabled_after_transition != 0 &&
           result->sprites_untouched_after_transition != 0 &&
           result->other_row_still_enabled != 0 && result->close_cleared_open_flag != 0 &&
           result->close_cleared_slot_flag != 0 && result->close_deleted_rows != 0 &&
           result->close_reset_combat_slot != 0 && result->select_moved_selection != 0 &&
           result->select_flagged_refresh != 0 && result->submenu_maps_action != 0 &&
           result->submenu_settles_recorded != 0 && result->item_recorded_usable != 0 &&
           result->item_unavailable_after_loss != 0;
}

void PrintKeyboardMenuSemanticResults(const KeyboardMenuSemanticResult* result)
{
    fprintf(stderr,
            "keyboard-menu semantic: spell_record=%u initially_enabled=%u "
            "disabled_after_transition=%u sprites_untouched=%u other_row_enabled=%u "
            "close_open=%u close_slot=%u close_rows=%u close_combat=%u "
            "select_moved=%u select_refresh=%u submenu_map=%u submenu_settle=%u "
            "item_usable=%u item_lost=%u\n",
            result->spell_record, result->entry_initially_enabled,
            result->entry_disabled_after_transition, result->sprites_untouched_after_transition,
            result->other_row_still_enabled, result->close_cleared_open_flag,
            result->close_cleared_slot_flag, result->close_deleted_rows,
            result->close_reset_combat_slot, result->select_moved_selection,
            result->select_flagged_refresh, result->submenu_maps_action,
            result->submenu_settles_recorded, result->item_recorded_usable,
            result->item_unavailable_after_loss);
    fflush(stderr);
}
