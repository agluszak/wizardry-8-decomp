/* In-process semantic scenario for the split-stack dialog recovered in
   SplitItemDialog.cpp and consumed by RCSItemsPage.cpp. The scenario runs on
   the game thread once the main menu is live so the item database is real.

   The destroy callback's accept path is not driven: its tail calls
   (RebuildEquipmentAndDerivedStatsForSlot, RebuildCampItemList,
   SetCampItemActionMode) are recovered, but they assume the camp
   state and the items-page action controls are live, which only holds while
   the review screen is open. The cancel path is exercised because it returns
   before touching them. */

#include "split_stack_semantic_test.h"

#include "wiz8/dialog_code/DialogFactoryDialogs.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/layouts/item_instance.h"
#include "wiz8/layouts/item_tables.h"
#include "wiz8/local_screens/RCSItemsPage.h"
#include "wiz8/local_screens/ReviewCharacterScreen.h"
#include "wiz8/xstatus.h"

#include <stdio.h>
#include <string.h>

static int FindStackableItemId(void)
{
    unsigned int index;

    for (index = 0; index < gXStatus.uiItemsInDatabase; ++index) {
        W8ItemDatabaseRecord* record = g_item_records + index;
        if (record->quantity_kind == 1 && (record->flags_041 & 2) == 0 &&
            record->maximum_quantity >= 8) {
            return (int)index;
        }
    }
    return -1;
}

/* Saves and restores the cursor/hand/split-source globals the body drives,
   so early returns cannot leak test state back into the real UI. */
struct SplitStackStateScope {
    unsigned char cursor;
    W8ItemInstance hand;
    W8ItemInstance* source;

    SplitStackStateScope()
    {
        cursor = g_status.item_in_cursor;
        hand = g_status.item_in_hand_235b;
        source = g_split_item_source;
    }

    ~SplitStackStateScope()
    {
        g_status.item_in_cursor = cursor;
        g_status.item_in_hand_235b = hand;
        g_split_item_source = source;
    }
};

static bool RunSplitStackBody(SplitStackSemanticResult* result, bool fail_early)
{
    SplitStackStateScope scope;
    W8ItemInstance source;
    W8SplitItemDialog* dialog;
    W8SplitItemDialog* cancel_dialog;
    int item_id;
    int expected_split;

    item_id = FindStackableItemId();
    if (item_id < 0) {
        return false;
    }
    result->stackable_item_found = 1;

    memset(&source, 0, sizeof(source));
    source.iItemNo = item_id;
    source.stack_count = 6;
    g_status.item_in_cursor = 0;
    g_status.item_in_hand_235b.iItemNo = -1;

    /* The constructor picks the split count: half the stack for stacks whose
       record allows more than ten, one for small-capacity stacks. */
    expected_split = g_item_records[item_id].maximum_quantity <= 0xa ? 1 : 3;
    dialog = new W8SplitItemDialog(0, &source, -1);
    if (dialog == 0) {
        return false;
    }
    if (fail_early) {
        /* The same early exit a failed check would take: the scope must
           restore cursor, hand, and split source. */
        delete dialog;
        return false;
    }
    result->ctor_split_in_range = dialog->split_count_0c0 == expected_split;
    result->ctor_counts_sum_to_stack =
        dialog->split_count_0c0 + dialog->m_remaining_0bc == 6 && dialog->m_stack_total_0c4 == 6;
    delete dialog;

    /* An explicit count splits exactly that many off the stack. */
    dialog = new W8SplitItemDialog(0, &source, 2);
    if (dialog != 0) {
        result->explicit_count_applied =
            dialog->split_count_0c0 == 2 && dialog->m_remaining_0bc == 4;
        delete dialog;
    }

    /* A cancelled dialog must leave the stack untouched; the result callback
       is the real RCSItemsPage destroy callback. */
    g_split_item_source = &source;
    cancel_dialog = new W8SplitItemDialog(0, &source, -1);
    if (cancel_dialog != 0) {
        cancel_dialog->split_result_0c8 = g_split_result_kind + 1;
        SplitStackDialogResult(cancel_dialog);
        result->cancel_leaves_stack =
            source.stack_count == 6 && g_status.item_in_hand_235b.iItemNo == -1;
        delete cancel_dialog;
    }
    return true;
}

bool RunSplitStackSemanticTest(SplitStackSemanticResult* result)
{
    memset(result, 0, sizeof(*result));

    /* An early-exit body run inside sentinel state must restore all three
       globals the body touches. */
    {
        SplitStackStateScope sentinels;
        W8ItemInstance sentinel_source;

        memset(&sentinel_source, 0, sizeof(sentinel_source));
        g_split_item_source = &sentinel_source;
        g_status.item_in_cursor = 0x5a;
        g_status.item_in_hand_235b.iItemNo = 0x1234;
        RunSplitStackBody(result, true);
        result->state_restored_after_failure = g_split_item_source == &sentinel_source &&
                                               g_status.item_in_cursor == 0x5a &&
                                               g_status.item_in_hand_235b.iItemNo == 0x1234;
    }

    RunSplitStackBody(result, false);
    return result->stackable_item_found && result->ctor_split_in_range &&
           result->ctor_counts_sum_to_stack && result->explicit_count_applied &&
           result->cancel_leaves_stack && result->state_restored_after_failure;
}

void PrintSplitStackSemanticResults(const SplitStackSemanticResult* result)
{
    fprintf(stderr,
            "split-stack semantic: item_found=%u ctor_in_range=%u "
            "ctor_counts_sum=%u explicit_count=%u cancel_untouched=%u "
            "state_restored=%u\n",
            result->stackable_item_found, result->ctor_split_in_range,
            result->ctor_counts_sum_to_stack, result->explicit_count_applied,
            result->cancel_leaves_stack, result->state_restored_after_failure);
    fflush(stderr);
}
