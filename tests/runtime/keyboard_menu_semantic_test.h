#pragma once

struct KeyboardMenuSemanticResult {
    unsigned char spell_record;
    unsigned char entry_initially_enabled;
    unsigned char entry_disabled_after_transition;
    unsigned char sprites_untouched_after_transition;
    unsigned char other_row_still_enabled;
    unsigned char close_cleared_open_flag;
    unsigned char close_cleared_slot_flag;
    unsigned char close_deleted_rows;
    unsigned char close_reset_combat_slot;
    unsigned char select_moved_selection;
    unsigned char select_flagged_refresh;
    unsigned char submenu_maps_action;
    unsigned char submenu_settles_recorded;
    unsigned char item_recorded_usable;
    unsigned char item_unavailable_after_loss;
};

bool RunKeyboardMenuSemanticTest(KeyboardMenuSemanticResult* result);
void PrintKeyboardMenuSemanticResults(const KeyboardMenuSemanticResult* result);
