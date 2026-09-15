/* In-process semantic scenario for the keyboard-menu availability refresh
   recovered in MGSKeyboard.cpp. The scenario runs on the driver thread once
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
#include "wiz8/local_code/TextControl.h"
#include "wiz8/local_screens/MGSButtons.h"
#include "wiz8/local_screens/MGSKeyboard.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/xstatus.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { KEYBOARD_MENU_ROW_COUNT = 12, TEST_SPELL_ID = 1, TEST_SPELL_COST = 4 };

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
    const int spell_id = TEST_SPELL_ID;
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
    saved_spell_id = g_status_685170.buffers.party_rows[0].spell_id;
    saved_power_level = g_status_685170.buffers.party_rows[0].spell_detail.spell.power_level;
    saved_spell_learned = g_status_685170.buffers.characters[0].spell_learned[spell_id];
    saved_sp_left = g_status_685170.buffers.characters[0].sp_left[realm];
    saved_usable_when = g_spell_records[spell_id].usable_when;
    saved_realm = g_spell_records[spell_id].realm;
    saved_spell_point_cost = g_spell_records[spell_id].spell_point_cost;

    gXStatus.fCombatMode = 1;
    g_value_64c1c8 = 0;
    g_status_685170.buffers.party_rows[0].spell_id = spell_id;
    g_status_685170.buffers.party_rows[0].spell_detail.spell.power_level = 1;
    g_status_685170.buffers.characters[0].spell_learned[spell_id] = 1;
    g_status_685170.buffers.characters[0].sp_left[realm] = TEST_SPELL_COST * 2;
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
        g_status_685170.buffers.party_rows[0].spell_id = saved_spell_id;
        g_status_685170.buffers.party_rows[0].spell_detail.spell.power_level = saved_power_level;
        g_status_685170.buffers.characters[0].spell_learned[spell_id] = saved_spell_learned;
        g_status_685170.buffers.characters[0].sp_left[realm] = saved_sp_left;
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
    g_status_685170.buffers.characters[0].sp_left[realm] = 0;
    RefreshKeyboardMenuRows();

    result->entry_disabled_after_transition = rows[0]->m_enabled == 0;
    result->sprites_untouched_after_transition =
        rows[0]->m_normalSprite == saved_normal_sprite &&
        rows[0]->m_alternateNormalSprite == saved_alternate_sprite;
    result->other_row_still_enabled = rows[1] != 0 && rows[1]->m_enabled != 0;

    for (index = 0; index < KEYBOARD_MENU_ROW_COUNT; ++index) {
        if (g_keyboard_menu_rows_69b820[index] != 0) {
            delete g_keyboard_menu_rows_69b820[index];
            g_keyboard_menu_rows_69b820[index] = 0;
        }
        g_keyboard_menu_pages_69b808[index] = 0;
        g_keyboard_menu_items_69b7ec[index] = 0;
    }

    g_status_685170.buffers.party_rows[0].spell_id = saved_spell_id;
    g_status_685170.buffers.party_rows[0].spell_detail.spell.power_level = saved_power_level;
    g_status_685170.buffers.characters[0].spell_learned[spell_id] = saved_spell_learned;
    g_status_685170.buffers.characters[0].sp_left[realm] = saved_sp_left;
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
           result->sprites_untouched_after_transition != 0 && result->other_row_still_enabled != 0;
}

void PrintKeyboardMenuSemanticResults(const KeyboardMenuSemanticResult* result)
{
    fprintf(stderr,
            "keyboard-menu semantic: spell_record=%u initially_enabled=%u "
            "disabled_after_transition=%u sprites_untouched=%u other_row_enabled=%u\n",
            result->spell_record, result->entry_initially_enabled,
            result->entry_disabled_after_transition, result->sprites_untouched_after_transition,
            result->other_row_still_enabled);
    fflush(stderr);
}
