#pragma once

struct NpcDialogueSemanticResult {
    unsigned char state_ready;
    unsigned char prepended_quote_ran_first;
    unsigned char spacer_consumed_inertly;
    unsigned char close_flag_set;
    unsigned char notice_dispatched;
    unsigned char notice_requeued_quote;
    unsigned char pending_order;
    unsigned char queue_drained;
    unsigned char layout_retired;
    unsigned char layout_installed;
};

bool RunNpcDialogueSemanticTest(NpcDialogueSemanticResult* result);
void PrintNpcDialogueSemanticResults(const NpcDialogueSemanticResult* result);
