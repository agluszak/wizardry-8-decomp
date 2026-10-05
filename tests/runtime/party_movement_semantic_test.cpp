/* In-process semantic test for the recovered combat party-movement lifecycle.
   It runs after normal product initialization, but substitutes isolated combat
   and level blocks so cancellation and completion cannot mutate a save. */

#include "party_movement_semantic_test.h"

#include "wiz8/layouts/combat_state.h"
#include "wiz8/layouts/main_game_screen.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/local_code/CombatPartyMovement.h"
#include "wiz8/local_screens/MGSPartyMovement.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/xstatus.h"

#include <stdio.h>
#include <string.h>

bool RunPartyMovementSemanticTest(PartyMovementSemanticResult* result)
{
    static W8CombatState combat;
    static W8LevelRuntimeBlock level;
    W8CombatState* saved_combat = g_combat_state;
    W8LevelRuntimeBlock* saved_level = g_level_block;
    unsigned char saved_combat_mode = gXStatus.fCombatMode;
    unsigned char saved_movement_mode = gXStatus.fPartyMovementMode;
    unsigned char saved_movement_ui = gXStatus.fPartyMovementUi;

    memset(result, 0, sizeof(*result));
    memset(&level, 0, sizeof(level));
    g_combat_state = &combat;
    g_level_block = &level;
    gXStatus.fCombatMode = true;
    gXStatus.fPartyMovementMode = true;
    gXStatus.fPartyMovementUi = false;

    combat.uiCurrentPartyAction = W8_PARTY_ACTION_WALK;
    combat.uiCurrentPartyActionStatus = W8_PARTY_ACTION_NOT_STARTED;
    CancelPartyMovement();
    result->cancel_clears_active_move = combat.uiCurrentPartyAction == W8_PARTY_ACTION_NONE;

    combat.uiCurrentPartyAction = W8_PARTY_ACTION_WALK;
    combat.uiCurrentPartyActionStatus = W8_PARTY_ACTION_NOT_STARTED;
    level.move_percent = 0;
    BeginFreeTurnPhase();
    result->completion_marks_finished =
        combat.uiCurrentPartyActionStatus == W8_PARTY_ACTION_FINISHED;
    result->completion_clears_mode = !gXStatus.fPartyMovementMode;
    result->completion_releases_ui = !gXStatus.fPartyMovementUi;

    g_combat_state = saved_combat;
    g_level_block = saved_level;
    gXStatus.fCombatMode = saved_combat_mode;
    gXStatus.fPartyMovementMode = saved_movement_mode;
    gXStatus.fPartyMovementUi = saved_movement_ui;

    return result->cancel_clears_active_move && result->completion_marks_finished &&
           result->completion_clears_mode && result->completion_releases_ui;
}

void PrintPartyMovementSemanticResults(const PartyMovementSemanticResult* result)
{
    fprintf(stderr,
            "party-movement semantic: cancel_cleared=%u completion_finished=%u "
            "mode_cleared=%u ui_released=%u\n",
            result->cancel_clears_active_move, result->completion_marks_finished,
            result->completion_clears_mode, result->completion_releases_ui);
    fflush(stderr);
}
