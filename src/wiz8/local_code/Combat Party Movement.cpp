#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"

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

/* The turn phase in which the party has its own movement back. */
enum { W8_TURN_PHASE_FREE = 3 };

extern unsigned char g_in_combat_00683f94;
extern unsigned char g_party_moving_006850b5;
extern unsigned char g_flag_00683fce;
extern W8CombatState* g_combat_state;
extern W8LevelRuntimeBlock* g_level_block;
extern float g_movement_speed_005ec0a8;
extern float g_movement_speed_step_005ed490;
extern void ClampUnsignedInteger(unsigned int* value, unsigned int base, unsigned int span);
extern void SetPartyMoving(int moving);                                 /* 0x00420B40 */
extern void ReleasePartyMovement(void);                                 /* 0x005A1890 */
extern void HoldPartyMovement(void);                                    /* 0x005A1DD0 */
extern void Function5A1E90(void);
extern void Function41F0D0(void);
extern void Function5354E0(void);
extern void Function4F06B0(void);
extern void ShowNotice(int channel, void* notice, int a, int b, int c);  /* 0x0058AC00 */
extern unsigned char GetLevelDataFlag6(void);                            /* 0x0041F140 */
extern void NotifyNearbyMonsters(int what);                              /* 0x004ECAA0 */
extern void RefreshOutwardSightForAllMonsters(void);                     /* 0x00505780 */
extern unsigned char Function4F0010(unsigned int* out_steps);

/* Note that the party has started moving. */
// FUNCTION: WIZ8 0x004EFBE0
void BeginPartyMovement(void)
{
    g_party_moving_006850b5 = 1;
    SetPartyMoving(1);
}

/* One or ten, depending on whether the party is moving under combat rules -
   the step the phase counter advances by. */
// FUNCTION: WIZ8 0x004F0500
char GetPhaseStep(void)
{
    return g_combat_state->movement_mode != 0 ? 10 : 1;
}

/* Note what kind of move is pending. Only one value is singled out; everything
   else counts as the other kind. */
// FUNCTION: WIZ8 0x004F0520
void SetPendingMoveKind(int kind)
{
    if (g_in_combat_00683f94 == 0) {
        srAssertFail("gXStatus.fCombatMode", COMBAT_MOVEMENT_CPP, 435, 0);
    }
    g_combat_state->pending_move_kind = (kind != 10) + 1;
}

/* Hand movement back to the party, or take it away and fill both budgets. The
   party has its own movement only out of combat mode, or in the free phase,
   and only with nothing pending. */
// FUNCTION: WIZ8 0x004F0AA0
void UpdatePartyMovementControl(void)
{
    if ((g_combat_state->movement_mode == 0 ||
         g_combat_state->turn_phase == W8_TURN_PHASE_FREE) &&
        g_combat_state->pending_move_kind == 0) {
        ReleasePartyMovement();
        return;
    }
    g_level_block->move_budget_2dc = 100;
    g_level_block->move_budget_2e0 = 100;
    HoldPartyMovement();
}

/* Whether the party may move at all right now. Out of combat mode, or in the
   free phase, it comes down to whether a move is already pending; in the
   opening phase it is always allowed. */
// FUNCTION: WIZ8 0x004F0800
unsigned char CanPartyMove(void)
{
    unsigned int phase;

    if (g_in_combat_00683f94 == 0) {
        srAssertFail("gXStatus.fCombatMode", COMBAT_MOVEMENT_CPP, 613, 0);
    }
    phase = g_combat_state->turn_phase;
    if (g_combat_state->movement_mode == 0 || phase == W8_TURN_PHASE_FREE) {
        if (g_combat_state->pending_move_kind != 0) {
            return 1;
        }
        return 0;
    }
    if (phase == 0) {
        return 1;
    }
    return 0;
}

/* How fast the party moves. The second combat mode is half again as fast, and
   whatever the step count adds is scaled by the same world constant. */
// FUNCTION: WIZ8 0x004EFFA0
float GetPartyMovementSpeed(void)
{
    float speed = 1.0f;
    unsigned int steps;

    if (g_combat_state->movement_mode == 2) {
        speed = 1.5f;
    }
    if (Function4F0010(&steps)) {
        return (steps * g_movement_speed_step_005ed490 + speed) * g_movement_speed_005ec0a8;
    }
    return speed * g_movement_speed_005ec0a8;
}

/* End the party's movement phase. Outside the two combat modes there is
   nothing to unwind; inside them the notice is posted unless the level says
   otherwise, and everything that watched the party move is told. */
// FUNCTION: WIZ8 0x004EFD30
void EndPartyMovementPhase(void)
{
    if (g_combat_state->movement_mode != 1 && g_combat_state->movement_mode != 2) {
        g_combat_state->turn_phase = 2;
        return;
    }
    if (!GetLevelDataFlag6()) {
        ShowNotice(8, g_notices[0x870 / 4], -1, -1, 0);
    }
    Function41F0D0();
    Function5A1E90();
    HoldPartyMovement();
    RefreshOutwardSightForAllMonsters();
    g_combat_state->turn_phase = 2;
}

/* Enter the free phase: everything that was waiting on the party is released,
   the monsters are told, and movement control is settled. */
// FUNCTION: WIZ8 0x004F0630
void BeginFreeTurnPhase(void)
{
    Function41F0D0();
    g_combat_state->turn_phase = W8_TURN_PHASE_FREE;
    g_flag_00683fce = 0;
    Function5354E0();
    Function4F06B0();
    NotifyNearbyMonsters(0);
    /* The tail is UpdatePartyMovementControl written out again rather than
       called, which is why this body is twice the size of a forwarder. */
    if ((g_combat_state->movement_mode == 0 ||
         g_combat_state->turn_phase == W8_TURN_PHASE_FREE) &&
        g_combat_state->pending_move_kind == 0) {
        ReleasePartyMovement();
        return;
    }
    g_level_block->move_budget_2dc = 100;
    g_level_block->move_budget_2e0 = 100;
    HoldPartyMovement();
}

/* Round one combatant's phase to the ten it belongs in and clamp it into the
   round. The assertion names the bound as PHASES_PER_ROUND. */
// FUNCTION: WIZ8 0x004F0480
void RoundPhaseToStep(unsigned int* phase, unsigned int base)
{
    if (g_combat_state->movement_mode != 0) {
        *phase = (*phase + 5) - (*phase + 5) % 10;
        if (base % 10 != 0) {
            base += 10 - base % 10;
        }
        ClampUnsignedInteger(phase, base, W8_PHASES_PER_ROUND);
    }
    if (*phase == 0 || *phase > W8_PHASES_PER_ROUND) {
        srAssertFail("( *puiPhase > 0 ) && ( *puiPhase <= PHASES_PER_ROUND )",
                     COMBAT_MOVEMENT_CPP, 410, 0);
    }
}
