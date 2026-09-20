/* In-process semantic scenario for the NPC message queue recovered in
   NPC Scripting.cpp. The scenario runs on the driver thread once the main
   menu is live.

   It fabricates a script NPC whose record flags (flag_2ea set, no monster, no
   script file) keep every dispatch on the shallow path, stages a pending
   script notice, then queues a spacer, a dialogue close and the notice's
   dispatch line, with a marked script quote prepended onto the head. Pumping
   ProcessMessageBoxQueue line by line verifies the queue semantics retail
   relies on:

   - prepend inserts ahead of everything already queued;
   - mark_pending records the quote index in pending_script_values;
   - the spacer kind is consumed with no case and no effect;
   - CLOSE_DIALOGUE sets g_flag_6109f0 while the dialogue stays closed;
   - DISPATCH_PENDING_NOTICE hands the staged record to
     BeginNpcDialogueInternal, which clears g_flag_68f0f9, opens dialogue mode
     and appends the notice's quote line;
   - an emptied queue flips g_message_queue_idle_68c501 back on.

   The fake NPC's flag_2ea also keeps BeginNpcDialogueInternal on the quote
   branch, and fCampMode/fCombatMode keep the speaker pick and world pause
   out of the scenario. */

#include "npc_dialogue_semantic_test.h"

#include "wiz8/layouts/game_status.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/layouts/main_game_screen.h"
#include "wiz8/layouts/npc_state.h"
#include "wiz8/layouts/screen_state.h"
#include "wiz8/level_specific_code/MasterFunctionList.h"
#include "wiz8/local_code/Gameloop.h"
#include "wiz8/local_code/NPCScripting.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/NPCInteractionSubscreen.h"
#include "wiz8/message_box.h"
#include "wiz8/xstatus.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { FIRST_QUOTE = 7, MARKED_QUOTE = 9, NOTICE_QUOTE = 0x21 };

bool RunNpcDialogueSemanticTest(NpcDialogueSemanticResult* result)
{
    W8NpcDatabaseRecord* fake_record;
    W8NpcState* fake_npc;
    W8MainScreenState* fake_screen;
    W8LevelRuntimeBlock* test_level_block;
    W8LevelRuntimeBlock* saved_level_block;
    W8NpcState* saved_script_npc;
    W8MainScreenState* saved_screen;
    W8PendingNotice saved_notice;
    W8MessageBoxLine* tail;
    unsigned char saved_flag_68f0f9;
    unsigned char saved_flag_6109f0;
    unsigned char saved_flag_006840bc;
    unsigned char saved_queue_idle;
    unsigned char saved_camp_mode;
    unsigned char saved_combat_mode;
    unsigned char saved_dialogue_mode;
    unsigned char saved_movement_ui;
    int saved_screen_id;
    int index;

    memset(result, 0, sizeof(*result));

    fake_record = static_cast<W8NpcDatabaseRecord*>(malloc(sizeof(W8NpcDatabaseRecord)));
    fake_npc = static_cast<W8NpcState*>(malloc(sizeof(W8NpcState)));
    fake_screen = static_cast<W8MainScreenState*>(malloc(sizeof(W8MainScreenState)));
    if (fake_record == 0 || fake_npc == 0 || fake_screen == 0) {
        free(fake_record);
        free(fake_npc);
        free(fake_screen);
        return false;
    }
    memset(fake_record, 0, sizeof(*fake_record));
    memset(fake_npc, 0, sizeof(*fake_npc));
    memset(fake_screen, 0, sizeof(*fake_screen));

    /* flag_2ea routes the dispatcher's quote handling past the pending-value
       rescan and routes BeginNpcDialogueInternal to its quote branch; the
       name style avoids the healer and Savant cases it checks first. */
    fake_record->flag_2ea = 1;
    fake_npc->record = fake_record;
    fake_npc->name_style = 0x99;
    fake_screen->dialogue_npc = fake_npc;

    saved_level_block = g_level_block;
    test_level_block = 0;
    if (g_level_block == 0) {
        test_level_block = static_cast<W8LevelRuntimeBlock*>(malloc(sizeof(W8LevelRuntimeBlock)));
        if (test_level_block == 0) {
            free(fake_record);
            free(fake_npc);
            free(fake_screen);
            return false;
        }
        memset(test_level_block, 0, sizeof(*test_level_block));
        g_level_block = test_level_block;
    }

    saved_script_npc = g_npc_scripting.npc;
    saved_screen = g_screen_state_00649f1c;
    saved_notice = g_pending_notice_68ee60;
    saved_flag_68f0f9 = g_flag_68f0f9;
    saved_flag_6109f0 = g_flag_6109f0;
    saved_flag_006840bc = g_flag_006840bc;
    saved_queue_idle = g_message_queue_idle_68c501;
    saved_camp_mode = gXStatus.fCampMode;
    saved_combat_mode = gXStatus.fCombatMode;
    saved_dialogue_mode = gXStatus.fNpcDialogueMode;
    saved_movement_ui = gXStatus.fPartyMovementUi;
    saved_screen_id = g_current_screen_state.id;

    g_npc_scripting.npc = fake_npc;
    g_screen_state_00649f1c = fake_screen;
    g_current_screen_state.id = W8_SCREEN_MAIN_GAME;
    gXStatus.fCampMode = 1;
    gXStatus.fCombatMode = 1;
    gXStatus.fNpcDialogueMode = 0;
    gXStatus.fPartyMovementUi = 0;
    g_flag_6109f0 = 0;
    g_flag_68f0f9 = 1;
    g_message_queue_idle_68c501 = 1;

    g_pending_notice_68ee60.npc = fake_npc;
    g_pending_notice_68ee60.item.item_id = -1;
    g_pending_notice_68ee60.line = NOTICE_QUOTE;
    g_pending_notice_68ee60.flag = 0;
    g_pending_notice_68ee60.force = 0;
    g_pending_notice_68ee60.unused_16[0] = 0;
    g_pending_notice_68ee60.unused_16[1] = 0;

    result->state_ready = 1;

    QueueNpcMessageLine(W8_NPC_MSG_SPACER, 0);
    QueueNpcMessageLine(W8_NPC_MSG_CLOSE_DIALOGUE, 0);
    QueueNpcMessageLine(W8_NPC_MSG_DISPATCH_PENDING_NOTICE, 0);
    QueueNpcScriptLine(MARKED_QUOTE, 1, 0, 1);
    QueueNpcScriptLine(FIRST_QUOTE, 1, 1, 1);

    ProcessMessageBoxQueue(); /* the prepended quote */
    result->prepended_quote_ran_first =
        g_npc_scripting.pending_script_values.GetCount() == 1 &&
        **g_npc_scripting.pending_script_values.GetAt(0) == FIRST_QUOTE;

    ProcessMessageBoxQueue(); /* the spacer */
    result->spacer_consumed_inertly = g_npc_scripting.message_lines.GetCount() == 3;

    ProcessMessageBoxQueue(); /* CLOSE_DIALOGUE */
    result->close_flag_set = g_flag_6109f0 != 0 && gXStatus.fNpcDialogueMode == 0;

    ProcessMessageBoxQueue(); /* DISPATCH_PENDING_NOTICE */
    result->notice_dispatched =
        g_flag_68f0f9 == 0 && gXStatus.fNpcDialogueMode != 0 && fake_screen->flag_252 != 0;
    tail = 0;
    if (g_npc_scripting.message_lines.GetCount() == 2) {
        tail = *g_npc_scripting.message_lines.GetAt(1);
    }
    result->notice_requeued_quote =
        tail != 0 && tail->type == W8_NPC_MSG_QUOTE && tail->quote_index == NOTICE_QUOTE;

    ProcessMessageBoxQueue(); /* the appended marked quote */
    result->pending_order = g_npc_scripting.pending_script_values.GetCount() == 2 &&
                            **g_npc_scripting.pending_script_values.GetAt(0) == FIRST_QUOTE &&
                            **g_npc_scripting.pending_script_values.GetAt(1) == MARKED_QUOTE;

    ProcessMessageBoxQueue(); /* the notice's requeued quote */
    ProcessMessageBoxQueue(); /* empties the queue and restores the idle flag */
    result->queue_drained =
        g_npc_scripting.message_lines.GetCount() == 0 && g_message_queue_idle_68c501 != 0;

    /* SetNpcDialogueLayoutMode: with the dialogue cursor down, a zero value
       retires the current layout into previous_dialogue_layout and parks value_fc on NONE,
       while a nonzero value installs directly. The fake screen is freed below,
       so the layout fields need no restore. */
    fake_screen->value_fc = W8_DIALOGUE_LAYOUT_TOPIC_MENU;
    SetNpcDialogueLayoutMode(0);
    result->layout_retired =
        fake_screen->previous_dialogue_layout == W8_DIALOGUE_LAYOUT_TOPIC_MENU &&
        fake_screen->value_fc == W8_DIALOGUE_LAYOUT_NONE;
    SetNpcDialogueLayoutMode(W8_DIALOGUE_LAYOUT_TRANSCRIPT);
    result->layout_installed = fake_screen->value_fc == W8_DIALOGUE_LAYOUT_TRANSCRIPT;

    while (g_npc_scripting.message_lines.GetCount() > 0) {
        delete g_npc_scripting.message_lines.RemoveAt(0);
    }
    for (index = 0; index < g_npc_scripting.pending_script_values.GetCount(); ++index) {
        delete *g_npc_scripting.pending_script_values.GetAt(index);
    }
    g_npc_scripting.pending_script_values.Clear();

    g_npc_scripting.npc = saved_script_npc;
    g_screen_state_00649f1c = saved_screen;
    g_pending_notice_68ee60 = saved_notice;
    g_flag_68f0f9 = saved_flag_68f0f9;
    g_flag_6109f0 = saved_flag_6109f0;
    g_flag_006840bc = saved_flag_006840bc;
    g_message_queue_idle_68c501 = saved_queue_idle;
    gXStatus.fCampMode = saved_camp_mode;
    gXStatus.fCombatMode = saved_combat_mode;
    gXStatus.fNpcDialogueMode = saved_dialogue_mode;
    gXStatus.fPartyMovementUi = saved_movement_ui;
    g_current_screen_state.id = saved_screen_id;
    if (test_level_block != 0) {
        g_level_block = saved_level_block;
        free(test_level_block);
    }
    free(fake_record);
    free(fake_npc);
    free(fake_screen);

    return result->state_ready != 0 && result->prepended_quote_ran_first != 0 &&
           result->spacer_consumed_inertly != 0 && result->close_flag_set != 0 &&
           result->notice_dispatched != 0 && result->notice_requeued_quote != 0 &&
           result->pending_order != 0 && result->queue_drained != 0 &&
           result->layout_retired != 0 && result->layout_installed != 0;
}

void PrintNpcDialogueSemanticResults(const NpcDialogueSemanticResult* result)
{
    fprintf(stderr,
            "npc-dialogue semantic: ready=%u prepended_first=%u spacer_inert=%u "
            "close_flag=%u notice_dispatched=%u notice_quote=%u pending_order=%u "
            "queue_drained=%u layout_retired=%u layout_installed=%u\n",
            result->state_ready, result->prepended_quote_ran_first, result->spacer_consumed_inertly,
            result->close_flag_set, result->notice_dispatched, result->notice_requeued_quote,
            result->pending_order, result->queue_drained, result->layout_retired,
            result->layout_installed);
    fflush(stderr);
}
