#pragma once

struct SplitStackSemanticResult {
    unsigned char stackable_item_found;
    unsigned char ctor_split_in_range;
    unsigned char ctor_counts_sum_to_stack;
    unsigned char explicit_count_applied;
    unsigned char cancel_leaves_stack;
    unsigned char state_restored_after_failure;
};

bool RunSplitStackSemanticTest(SplitStackSemanticResult* result);
void PrintSplitStackSemanticResults(const SplitStackSemanticResult* result);
