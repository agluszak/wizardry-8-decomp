#include "wiz8/engine_code/GameData.h"
#include "wiz8/local_code/Sight.h"
#include "wiz8/local_screens/MGSPartyMovement.h"
#include "wiz8/local_code/GameplayDatabase.h"
#include "wiz8/xstatus.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/notices.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/CombatPartyMovement.h"
#include "wiz8/local_code/FormationAndFacing.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/float_constants.h"
#include "wiz8/sr_api.h"
#include "wiz8/utility.h"
#include "wiz8/local_code/MonsterAI.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_screens/MGSSpellCasting.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/3d_code/PList.h"
#include "timer.h"

/*
 * Local Code\Combat Party Movement.cpp.
 *
 * How the party moves while a fight is on: which of the two combat movement
 * modes is running, how fast it goes, and which phase of the turn the party is
 * allowed to act in.
 */

#define COMBAT_MOVEMENT_CPP "C:\\Projects\\Wizardry 8\\Local Code\\Combat Party Movement.cpp"

/* PHASES_PER_ROUND, named by the assertion that bounds the phase. */
enum { W8_PHASES_PER_ROUND = 100 };

/* The finished party-action status hands movement back to the party; the
   assertion corpus names this value ACTION_STATUS_FINISHED. */
enum { W8_ACTION_STATUS_FINISHED = 3 };

/* Note that the party has started moving. */
// FUNCTION: WIZ8 0x004efbe0
void BeginPartyMovement(void)
{
    gXStatus.party_moving = true;
    MoveTimer(1);
}

/* Round one combatant's phase to the ten it belongs in and clamp it into the
   round. The assertion names the bound as PHASES_PER_ROUND. */
// FUNCTION: WIZ8 0x004f0480
void RoundPhaseToStep(unsigned int* phase, unsigned int base)
{
    if (g_combat_state->uiCurrentPartyAction != 0) {
        *phase = (*phase + 5) - (*phase + 5) % 10;
        if (base % 10 != 0) {
            base += 10 - base % 10;
        }
        ClampUnsignedInteger(phase, base, W8_PHASES_PER_ROUND);
    }
    if (*phase == 0 || *phase > W8_PHASES_PER_ROUND) {
        srAssertFail("( *puiPhase > 0 ) && ( *puiPhase <= PHASES_PER_ROUND )", COMBAT_MOVEMENT_CPP,
                     410, 0);
    }
}

/* Advance the movement gauge from real world motion. Continuous-combat mode
   supplies a bounded synthetic step while the world is stationary. Emptying
   the gauge completes the action and synchronizes every combatant/UI owner. */
// FUNCTION: WIZ8 0x004f01d0
void UpdateActivePartyMovement(void)
{
    float real_elapsed;
    float frame_elapsed;

    if (HandlePartyMovement(&real_elapsed, &frame_elapsed) != 0) {
        gXStatus.party_move_distance += real_elapsed + frame_elapsed;
    } else if (g_settings_6850c8.continuous_combat != 0 &&
               ClockIsTicking(g_combat_state->party_movement_clock) == 0) {
        unsigned int step_count = g_settings_6850c8.combat_delay_ms / 200 + 10;
        ClampUnsignedInteger(&step_count, 10, 60);
        gXStatus.party_move_distance += gXStatus.flPartyMoveDistLimit / step_count;
    } else {
        goto check_completion;
    }

    if (gXStatus.flPartyMoveDistLimit <= g_float_005ebb34) {
        srAssertFail("gXStatus.flPartyMoveDistLimit > 0.0f", COMBAT_MOVEMENT_CPP, 359, 0);
    }
    g_level_block->move_budget_2dc = static_cast<int>(
        100.0f - gXStatus.party_move_distance * 100.0f / gXStatus.flPartyMoveDistLimit);
    ClampInteger(&g_level_block->move_budget_2dc, 0, 100);
    if (g_level_block->move_budget_2dc != g_level_block->move_budget_2e0) {
        InvalidatePartyMovementPanel();
        g_level_block->move_budget_2e0 = g_level_block->move_budget_2dc;
    }
    if (g_settings_6850c8.continuous_combat != 0) {
        g_combat_state->party_movement_clock = SetCountdownClock(g_settings_6850c8.combat_delay_ms);
    }

check_completion:
    if (g_level_block->move_budget_2dc < 1) {
        BeginFreeTurnPhase();
    }
}

/* One or ten, depending on whether the party is moving under combat rules -
   the step the phase counter advances by. */
// FUNCTION: WIZ8 0x004f0500
int GetPhaseStep(void)
{
    return g_combat_state->uiCurrentPartyAction != 0 ? 10 : 1;
}

/* Note what kind of move is pending. Only one value is singled out; everything
   else counts as the other kind. */
// FUNCTION: WIZ8 0x004f0520
void SetPendingMoveKind(int kind)
{
    if (gXStatus.fCombatMode == 0) {
        srAssertFail("gXStatus.fCombatMode", COMBAT_MOVEMENT_CPP, 435, 0);
    }
    g_combat_state->uiNextPartyAction = (kind != 10) + 1;
}

/* Drop a queued party move and rebuild every other switchable character's
   combat target. The excluded slot is the character currently being edited;
   -1 refreshes the whole party. */
// FUNCTION: WIZ8 0x004f0560
void ClearPendingPartyMovement(int excluded_party_slot)
{
    if (gXStatus.fCombatMode == 0) {
        srAssertFail("gXStatus.fCombatMode", COMBAT_MOVEMENT_CPP, 470, 0);
    }
    g_combat_state->uiNextPartyAction = 0;
    for (int party_slot = 0; party_slot < W8_PARTY_SLOT_COUNT; ++party_slot) {
        if (party_slot != excluded_party_slot &&
            CharacterCanSwitchTo(party_slot, W8_TARGETING_CONTEXT_IN_COMBAT, 0, 0) != 0) {
            RefreshCombatTargetHighlights(
                party_slot, &g_status_685170.buffers.XChar[party_slot].target_in_combat);
        }
    }
    UpdatePartyMovementControl();
}

/* Hand movement back to the party, or take it away and fill both budgets. The
   party has its own movement only out of combat mode, or in the free phase,
   and only with nothing pending. */
// FUNCTION: WIZ8 0x004f0aa0
void UpdatePartyMovementControl(void)
{
    if ((g_combat_state->uiCurrentPartyAction == 0 ||
         g_combat_state->uiCurrentPartyActionStatus == W8_ACTION_STATUS_FINISHED) &&
        g_combat_state->uiNextPartyAction == 0) {
        ReleasePartyMovement();
        return;
    }
    g_level_block->move_budget_2dc = 100;
    g_level_block->move_budget_2e0 = 100;
    InvalidatePartyMovementPanel();
}

/* Whether the party may move at all right now. Out of combat mode, or in the
   free phase, it comes down to whether a move is already pending; in the
   opening phase it is always allowed. */
// FUNCTION: WIZ8 0x004f0800
bool CanPartyMove(void)
{
    unsigned int status;

    if (gXStatus.fCombatMode == 0) {
        srAssertFail("gXStatus.fCombatMode", COMBAT_MOVEMENT_CPP, 613, 0);
    }
    status = g_combat_state->uiCurrentPartyActionStatus;
    if (g_combat_state->uiCurrentPartyAction == 0 || status == W8_ACTION_STATUS_FINISHED) {
        if (g_combat_state->uiNextPartyAction != 0) {
            return true;
        }
        return false;
    }
    if (status == 0) {
        return true;
    }
    return false;
}

/* Average the haste bonus over the party: every occupied slot must either be
   free of the sixth condition or carry the sixth enchantment, else there is no
   bonus at all. Each contributing slot adds ten steps per point of power,
   adjusted by the enchantment's percentage. */
// FUNCTION: WIZ8 0x004f0010
unsigned char GetPartyHasteSteps(unsigned int* out_steps)
{
    unsigned int total = 0;
    unsigned int count = 0;
    unsigned int party_slot;
    unsigned int steps;

    for (party_slot = 0; party_slot < 8; ++party_slot) {
        W8Character* character;

        if (g_status_685170.buffers.XChar[party_slot].fOccupied == 0) {
            continue;
        }
        character = &g_status_685170.buffers.Char[party_slot];
        if (character->uiCondition[19] != 0) {
            continue;
        }
        if (character->enchantments[5].turns_08 == 0) {
            return 0;
        }
        steps = static_cast<unsigned char>(character->enchantments[5].power_00 * 10);
        AdjustIntegerByPercent(&steps, character->enchantments[5].percent_04);
        total += steps;
        ++count;
    }
    if (out_steps != 0) {
        *out_steps = total / count;
    }
    return 1;
}

/* How fast the party moves. The second combat mode is half again as fast, and
   whatever the step count adds is scaled by the same world constant. */
// FUNCTION: WIZ8 0x004effa0
float GetPartyMovementSpeed(void)
{
    float speed = 1.0f;
    unsigned int steps;

    if (g_combat_state->uiCurrentPartyAction == 2) {
        speed = 1.5f;
    }
    if (GetPartyHasteSteps(&steps)) {
        return (steps * g_movement_speed_step_005ed490 + speed) * g_float_005ec0a8;
    }
    return speed * g_float_005ec0a8;
}

/* Reconcile character turns after the party finishes moving. Characters that
   pass the remaining-movement roll catch up to the combat clock; the rest are
   marked finished for this round and have their queued portrait event reset. */
// FUNCTION: WIZ8 0x004f06b0
void CompletePartyMovementTurns(void)
{
    if (g_level_block->move_budget_2dc < 0) {
        srAssertFail("gpMGSV->iPartyMovementPercent >= 0", COMBAT_MOVEMENT_CPP, 547, 0);
    }
    if (g_level_block->move_budget_2dc > 100) {
        srAssertFail("gpMGSV->iPartyMovementPercent <= 100", COMBAT_MOVEMENT_CPP, 548, 0);
    }

    unsigned int remaining = 100 - g_level_block->move_budget_2dc;
    if (g_combat_state->uiCurrentPartyAction == 2) {
        remaining = static_cast<unsigned int>(remaining * g_float_005ec3b8);
    }
    remaining = remaining < 100 ? 100 - remaining : 0;

    for (int party_slot = 0; party_slot < W8_PARTY_SLOT_COUNT; ++party_slot) {
        W8PartySlotRow* party_row = &g_status_685170.buffers.XChar[party_slot];
        W8Character* character = &g_status_685170.buffers.Char[party_slot];
        if (party_row->fOccupied == 0 || character->hp_current == 0 ||
            character->highest_condition >= 0xf) {
            continue;
        }
        W8CombatCharacterRow* combat_row = &g_combat_state->characters[party_slot];
        if (combat_row->dead_34 == 0 && Random(100) < remaining) {
            CatchUpCombatActor(combat_row);
            continue;
        }
        combat_row->dead_34 = 1;
        combat_row->phase = 0;
        party_row->pending_event_type_ff = static_cast<unsigned int>(-1);
    }
    RequestRedraw(0x1000ff);
}

/* Choose the party's phase from the best living character initiative, with
   the retail four-point random tie break. Party movement uses ten-step phase
   boundaries. */
// FUNCTION: WIZ8 0x004efe70
void InitializePartyMovementPhase(void)
{
    int minimum_initiative = 90;

    for (int party_slot = 0; party_slot < W8_PARTY_SLOT_COUNT; ++party_slot) {
        W8PartySlotRow* row = &g_status_685170.buffers.XChar[party_slot];
        W8Character* character = &g_status_685170.buffers.Char[party_slot];
        if (row->fOccupied != 0 && character->hp_current != 0) {
            int initiative = character->initiative + Random(4);
            if (initiative < minimum_initiative) {
                minimum_initiative = initiative;
            }
        }
    }
    ClampInteger(&minimum_initiative, -10, 89);
    g_combat_state->uiPartyActionPhase = 90 - minimum_initiative;
    if (g_combat_state->uiCurrentPartyAction == 1 || g_combat_state->uiCurrentPartyAction == 2) {
        RoundPhaseToStep(&g_combat_state->uiPartyActionPhase, 10);
    }
    if (g_combat_state->uiPartyActionPhase == 0 ||
        g_combat_state->uiPartyActionPhase > W8_PHASES_PER_ROUND) {
        srAssertFail("( gpCombat->uiPartyActionPhase > 0 ) && "
                     "( gpCombat->uiPartyActionPhase <= PHASES_PER_ROUND )",
                     COMBAT_MOVEMENT_CPP, 186, 0);
    }
}

/* Bring every not-yet-active combatant onto the ten-step schedule used by a
   party movement action. */
// FUNCTION: WIZ8 0x004f0c80
static void AlignCombatantsToPartyMovementPhase(void)
{
    RoundPhaseToStep(&g_combat_state->uiPartyActionPhase, g_combat_state->round_counter);
    for (int party_slot = 0; party_slot < W8_PARTY_SLOT_COUNT; ++party_slot) {
        W8CombatCharacterRow* row = &g_combat_state->characters[party_slot];
        if (row->phase >= g_combat_state->round_counter && row->dead_34 == 0) {
            RoundPhaseToStep(&row->phase, g_combat_state->round_counter);
        }
    }
    unsigned int monster_count = PLLength(gXStatus.plsMonsterList);
    for (unsigned int monster_index = 0; monster_index < monster_count; ++monster_index) {
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
        if (monster_info->fInCombat != 0 &&
            monster_info->pCombat->phase >= g_combat_state->round_counter &&
            monster_info->pCombat->active == 0) {
            RoundPhaseToStep(&monster_info->pCombat->phase, g_combat_state->round_counter);
        }
        monster_count = PLLength(gXStatus.plsMonsterList);
    }
}

/* Start one of the two party movement actions and synchronize the world,
   party and monster combat clocks around its movement limit. */
// FUNCTION: WIZ8 0x004f0af0
void StartPartyMovementAction(int move_kind)
{
    if (move_kind == 0) {
        srAssertFail("uiPartyAction != PARTY_ACTION_NONE", COMBAT_MOVEMENT_CPP, 759, 0);
    }
    if (g_combat_state->uiCurrentPartyAction == static_cast<unsigned int>(move_kind)) {
        return;
    }

    g_combat_state->uiCurrentPartyAction = move_kind;
    g_combat_state->uiCurrentPartyActionStatus = 0;
    gXStatus.flPartyMoveDistLimit = GetPartyMovementSpeed();
    ResetLevelMovement0041EEE0(gXStatus.flPartyMoveDistLimit, 0, move_kind == 2);
    InitializePartyMovementPhase();
    g_combat_state->uiPartyActionPhase += g_combat_state->round_counter;
    if (g_combat_state->uiPartyActionPhase > W8_PHASES_PER_ROUND) {
        g_combat_state->uiPartyActionPhase = W8_PHASES_PER_ROUND;
    }
    AlignCombatantsToPartyMovementPhase();
    RoundPhaseToStep(&g_combat_state->round_counter, g_combat_state->round_counter);
    RequestRedraw(0x100000);
}

/* End the party's movement phase. Outside the two combat modes there is
   nothing to unwind; inside them the notice is posted unless the level says
   otherwise, and everything that watched the party move is told. */
// FUNCTION: WIZ8 0x004efd30
void EndPartyMovementPhase(void)
{
    if (g_combat_state->uiCurrentPartyAction != 1 && g_combat_state->uiCurrentPartyAction != 2) {
        g_combat_state->uiCurrentPartyActionStatus = 2;
        return;
    }
    if (!GetLevelDataFlag6()) {
        ShowNotice(8, gppStringList[0x870 / 4], -1, -1, 0);
    }
    ResetLevelDataVectors();
    DisableFreeTurnButton();
    InvalidatePartyMovementPanel();
    RefreshOutwardSightForAllMonsters();
    g_combat_state->uiCurrentPartyActionStatus = 2;
}

/* Enter the free phase: everything that was waiting on the party is released,
   the monsters are told, and movement control is settled. */
// FUNCTION: WIZ8 0x004f0630
void BeginFreeTurnPhase(void)
{
    ResetLevelDataVectors();
    g_combat_state->uiCurrentPartyActionStatus = W8_ACTION_STATUS_FINISHED;
    gXStatus.fPartyMovementMode = false;
    CheckMonsterGroupsEnterCombat();
    CompletePartyMovementTurns();
    NotifyNearbyMonsters(0);
    /* The tail is UpdatePartyMovementControl written out again rather than
       called, which is why this body is twice the size of a forwarder. */
    if ((g_combat_state->uiCurrentPartyAction == 0 ||
         g_combat_state->uiCurrentPartyActionStatus == W8_ACTION_STATUS_FINISHED) &&
        g_combat_state->uiNextPartyAction == 0) {
        ReleasePartyMovement();
        return;
    }
    g_level_block->move_budget_2dc = 100;
    g_level_block->move_budget_2e0 = 100;
    InvalidatePartyMovementPanel();
}

/* Cancel the pending movement action. An action that is already entering its
   first phase is handed to the phase-settling path; otherwise the queued
   action is cleared and every eligible party target highlight is rebuilt. */
// FUNCTION: WIZ8 0x004f0860
void CancelPartyMovement(void)
{
    if (gXStatus.fCombatMode == 0) {
        srAssertFail("gXStatus.fCombatMode", COMBAT_MOVEMENT_CPP, 0x283, 0);
    }
    if (g_combat_state->uiCurrentPartyAction != 0 &&
        g_combat_state->uiCurrentPartyActionStatus != W8_ACTION_STATUS_FINISHED) {
        if (g_combat_state->uiCurrentPartyActionStatus == 0) {
            InterruptActivePartyMovement();
        }
        return;
    }
    if (g_combat_state->uiNextPartyAction == 0) {
        return;
    }
    if (gXStatus.fCombatMode == 0) {
        srAssertFail("gXStatus.fCombatMode", COMBAT_MOVEMENT_CPP, 0x1d6, 0);
    }
    g_combat_state->uiNextPartyAction = 0;
    for (int party_slot = 0; party_slot < W8_PARTY_SLOT_COUNT; ++party_slot) {
        if (CharacterCanSwitchTo(party_slot, W8_TARGETING_CONTEXT_CURRENT, 1, 0) != 0) {
            RefreshCombatTargetHighlights(
                party_slot, &g_status_685170.buffers.XChar[party_slot].target_in_combat);
        }
    }
    UpdatePartyMovementControl();
}

/* Interrupt a move that has entered its first phase. Bring every unfinished
   party member forward to the combat clock, then retire any phase that moved
   beyond this round before handing movement control back to the UI. */
// FUNCTION: WIZ8 0x004f0990
void InterruptActivePartyMovement(void)
{
    if (gXStatus.fCombatMode == 0) {
        srAssertFail("gXStatus.fCombatMode", COMBAT_MOVEMENT_CPP, 674, 0);
    }
    g_combat_state->uiCurrentPartyAction = 0;
    for (int party_slot = 0; party_slot < W8_PARTY_SLOT_COUNT; ++party_slot) {
        W8PartySlotRow* party_row = &g_status_685170.buffers.XChar[party_slot];
        if (party_row->fOccupied == 0) {
            continue;
        }
        W8CombatCharacterRow* row = &g_combat_state->characters[party_slot];
        if (row->phase < g_combat_state->round_counter && row->dead_34 == 0) {
            row->phase = g_combat_state->round_counter;
        }
        row->phase_clock_stamp = g_combat_state->round_counter;
        if (row->phase > W8_PHASES_PER_ROUND) {
            row->dead_34 = 1;
            row->phase = 0;
            party_row->pending_event_type_ff = static_cast<unsigned int>(-1);
            RequestRedraw((1 << party_slot) | 0x100000);
        }
    }
    UpdatePartyMovementControl();
}

// FUNCTION: WIZ8 0x004efda0
void FinishPartyMovementAction(void)
{
    if (g_combat_state->uiCurrentPartyAction != 1 && g_combat_state->uiCurrentPartyAction != 2) {
        g_combat_state->uiCurrentPartyActionStatus = 1;
        return;
    }
    if (GetLevelDataFlag6() != 0 && g_combat_state->uiCurrentPartyActionStatus == 2) {
        ShowNotice(8, gppStringList[0x21d], -1, -1, 0);
    }
    SoundPlay("Data\\Sound\\Misc\\Movement_Bar_Pop_Up.wav", 0);
    ClearLevelDataFlag6();
    if (g_combat_state->uiCurrentPartyAction == 2) {
        SetLevelDataFlag8();
    }
    EnableFreeTurnButton();
    InvalidatePartyMovementPanel();
    if (g_settings_6850c8.continuous_combat != 0) {
        g_combat_state->party_movement_clock = SetCountdownClock(1000);
    }
    TurnPartyTo(g_status_685170.party_heading);
    g_combat_state->uiCurrentPartyActionStatus = 1;
}

// FUNCTION: WIZ8 0x004efc00
void StartPartyMovementAction004EFC00(void)
{
    if (g_combat_state->uiCurrentPartyAction == 1 || g_combat_state->uiCurrentPartyAction == 2) {
        gXStatus.fPartyMovementMode = true;
        ShowNotice(8, gppStringList[0x21b], -1, -1, 0);
        gXStatus.party_move_distance = 0.0f;
    }
    if (g_combat_state->uiCurrentPartyAction == 1 || g_combat_state->uiCurrentPartyAction == 2) {
        if (GetLevelDataFlag6() != 0 && g_combat_state->uiCurrentPartyActionStatus == 2) {
            ShowNotice(8, gppStringList[0x21d], -1, -1, 0);
        }
        SoundPlay("Data\\Sound\\Misc\\Movement_Bar_Pop_Up.wav", 0);
        ClearLevelDataFlag6();
        if (g_combat_state->uiCurrentPartyAction == 2) {
            SetLevelDataFlag8();
        }
        EnableFreeTurnButton();
        InvalidatePartyMovementPanel();
        if (g_settings_6850c8.continuous_combat != 0) {
            g_combat_state->party_movement_clock = SetCountdownClock(1000);
        }
        TurnPartyTo(g_status_685170.party_heading);
    }
    g_combat_state->uiCurrentPartyActionStatus = 1;
    g_combat_state->passive_round_a55 = 0;
}

// FUNCTION: WIZ8 0x004f00c0
char PartyMovementReachedPhaseLimit(void)
{
    if (g_combat_state->uiCurrentPartyActionStatus != 1) {
        srAssertFail("gpCombat->uiCurrentPartyActionStatus == ACTION_STATUS_IN_PROGRESS",
                     COMBAT_MOVEMENT_CPP, 0x112, 0);
    }
    if (gXStatus.fPartyMovementMode == 0) {
        srAssertFail("gXStatus.fPartyMovementMode", COMBAT_MOVEMENT_CPP, 0x113, 0);
    }
    if (gXStatus.flPartyMoveDistLimit <= 0.0f) {
        srAssertFail("gXStatus.flPartyMoveDistLimit > 0", COMBAT_MOVEMENT_CPP, 0x114, 0);
    }
    float moved_fraction = gXStatus.party_move_distance / gXStatus.flPartyMoveDistLimit;
    int remaining_phases = 101 - g_combat_state->uiPartyActionPhase;
    if (remaining_phases == 0) {
        srAssertFail("uiTotalMovementPhases > 0", COMBAT_MOVEMENT_CPP, 0x120, 0);
    }
    unsigned int elapsed_phases = g_combat_state->round_counter -
                                  g_combat_state->uiPartyActionPhase +
                                  (g_combat_state->uiCurrentPartyAction != 0 ? 9 : 0) + 2;
    return static_cast<float>(elapsed_phases) / remaining_phases <= moved_fraction;
}
