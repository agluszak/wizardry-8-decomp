/* In-process semantic scenario for the split-stack dialog recovered in
   SplitItemDialog.cpp and consumed by RCSItemsPage.cpp. The scenario runs on
   the driver thread once the main menu is live so the item database is real.

   The destroy callback's accept path is not driven: its tail calls
   (RebuildEquipmentAndDerivedStatsForSlot, RebuildCampItemList005A4A00,
   SetCampItemActionMode005B59B0) are recovered, but they assume the camp
   state and the items-page action controls are live, which only holds while
   the review screen is open. The cancel path is exercised because it returns
   before touching them. */

#include "split_stack_semantic_test.h"

#include "wiz8/dialog_code/DialogFactoryDialogs.h"
#include "wiz8/game_status.h"
#include "wiz8/item_instance.h"
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

bool RunSplitStackSemanticTest(SplitStackSemanticResult* result)
{
    W8ItemInstance source;
    W8ItemInstance saved_hand;
    W8SplitItemDialog* dialog;
    W8SplitItemDialog* cancel_dialog;
    unsigned char saved_cursor;
    int item_id;
    int expected_split;

    memset(result, 0, sizeof(*result));
    item_id = FindStackableItemId();
    if (item_id < 0) {
        return false;
    }
    result->stackable_item_found = 1;

    memset(&source, 0, sizeof(source));
    source.item_id = item_id;
    source.stack_count = 6;
    saved_cursor = g_status_685170.item_in_cursor;
    saved_hand = g_status_685170.item_in_hand_235b;
    g_status_685170.item_in_cursor = 0;
    g_status_685170.item_in_hand_235b.item_id = -1;

    /* The constructor picks the split count: half the stack for stacks whose
       record allows more than ten, one for small-capacity stacks. */
    expected_split = g_item_records[item_id].maximum_quantity <= 0xa ? 1 : 3;
    dialog = new W8SplitItemDialog(0, &source, -1);
    if (dialog == 0) {
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
    g_split_item_source_0069c424 = &source;
    cancel_dialog = new W8SplitItemDialog(0, &source, -1);
    if (cancel_dialog != 0) {
        cancel_dialog->split_result_0c8 = g_split_result_kind_005efb44 + 1;
        SplitStackDialogResult005BAA80(cancel_dialog);
        result->cancel_leaves_stack =
            source.stack_count == 6 && g_status_685170.item_in_hand_235b.item_id == -1;
        delete cancel_dialog;
    }

    g_status_685170.item_in_hand_235b = saved_hand;
    g_status_685170.item_in_cursor = saved_cursor;
    g_split_item_source_0069c424 = 0;
    return result->ctor_split_in_range && result->ctor_counts_sum_to_stack &&
           result->explicit_count_applied && result->cancel_leaves_stack;
}

void PrintSplitStackSemanticResults(const SplitStackSemanticResult* result)
{
    fprintf(stderr,
            "split-stack semantic: item_found=%u ctor_in_range=%u "
            "ctor_counts_sum=%u explicit_count=%u cancel_untouched=%u\n",
            result->stackable_item_found, result->ctor_split_in_range,
            result->ctor_counts_sum_to_stack, result->explicit_count_applied,
            result->cancel_leaves_stack);
    fflush(stderr);
}
