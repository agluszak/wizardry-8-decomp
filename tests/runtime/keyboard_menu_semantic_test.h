#pragma once

struct KeyboardMenuSemanticResult {
    unsigned char spell_record;
    unsigned char entry_initially_enabled;
    unsigned char entry_disabled_after_transition;
    unsigned char sprites_untouched_after_transition;
    unsigned char other_row_still_enabled;
};

bool RunKeyboardMenuSemanticTest(KeyboardMenuSemanticResult* result);
void PrintKeyboardMenuSemanticResults(const KeyboardMenuSemanticResult* result);
