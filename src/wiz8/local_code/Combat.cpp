#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"

#include <stdarg.h>
#include <stdio.h>

/*
 * Local Code\Combat.cpp.
 *
 * The round bookkeeping: who is engaged, what each slot has chosen to do, and
 * the running list of characters who died this round.
 */

/* The action the round reset lifts, and the action the cached flag beside it
   watches for. */
enum { W8_ACTION_LIFTED_AT_ROUND_END = 9, W8_ACTION_KIND_ONE = 1 };

extern unsigned char CharacterIsEngaged(unsigned int party_slot);        /* 0x00524A10 */
extern unsigned char CharacterHasCondition(const W8Character* character, int condition);
/* 0x00547940 */
extern unsigned char g_in_combat_00683f94;
extern W8CombatState* g_combat_state;
/* The per-character combat rows begin at the combat state's own address and
   run 0xd4 bytes apart, so the state's leading fields are the first row's. */
extern W8CombatCharacterRow* g_combat_character_rows;
extern unsigned char g_combat_log_enabled_0068d810;
extern const wchar_t g_combat_log_format_00617664;
extern int g_active_character_0068518d;
extern void WriteGameLog(int channel, const wchar_t* format, ...);
extern void ClampUnsignedInteger(unsigned int* value, unsigned int base, unsigned int span);
extern void Function4F0480(unsigned int* actor, int round);
extern int Function53BC10(int party_slot);
extern void Function53AC30(int party_slot, void* combat_slot);
extern void ResetCombatSlot(W8CombatSlot* slot);
extern void NotifySpellPointsChanged(int party_slot);
extern void Function565420(void);
extern void Function4E8000(int party_slot, int action_kind, int action_detail, int a, int b);

/* Whether anybody in the party is engaged with something. */
// FUNCTION: WIZ8 0x004E7CA0
unsigned char AnyCharacterEngaged(void)
{
    unsigned int party_slot;

    for (party_slot = 0; party_slot < 8; ++party_slot) {
        if (CharacterIsEngaged(party_slot)) {
            return 1;
        }
    }
    return 0;
}

/* Which of the two engagement counts to report - the forced one when combat
   says so, the derived one otherwise. */
// FUNCTION: WIZ8 0x004ED2B0
int GetEngagementCount(void)
{
    if (g_combat_state->force_engaged_918 != 0) {
        return g_combat_state->value_90c;
    }
    return g_combat_state->value_910;
}

/* Whether the party is engaged at all. Being told so outright settles it;
   otherwise, with nothing derived, one conscious character whose combat row is
   flagged is enough. */
// FUNCTION: WIZ8 0x004E7E70
int IsPartyEngaged(void)
{
    unsigned int party_slot;

    if (g_combat_state->force_engaged_918 != 0) {
        return 1;
    }
    if (g_combat_state->value_910 == 0) {
        for (party_slot = 0; party_slot < 8; ++party_slot) {
            if (g_party_slot_rows[party_slot].flag_00 != 0 &&
                g_party_characters[party_slot].hp_current != 0 &&
                g_party_characters[party_slot].unknown_0b01 < 0xf &&
                g_combat_character_rows[party_slot].flag_4c != 0) {
                return 1;
            }
        }
    }
    return 0;
}

/* Note that one character died this round. */
// FUNCTION: WIZ8 0x004ECDD0
void RecordCharacterDeath(int party_slot)
{
    if (g_in_combat_00683f94 != 0) {
        g_combat_state->pending_deaths[g_combat_state->pending_death_count] = party_slot;
        ++g_combat_state->pending_death_count;
    }
}

/* Record what one slot has chosen to do, and cache whether it is the first
   kind beside it. */
// FUNCTION: WIZ8 0x004E8290
void SetSlotAction(int party_slot, int action_kind, int action_detail)
{
    W8PartySlotRow* row = &g_party_slot_rows[party_slot];

    row->action_kind = action_kind;
    row->action_detail = action_detail;
    row->action_is_kind_one = action_kind == W8_ACTION_KIND_ONE;
}

/* Post one line to the combat log, if the log is on. The whole line is
   formatted whether or not it will be shown. */
// FUNCTION: WIZ8 0x004ED260
void CombatLog(const char* format, ...)
{
    char line[200];
    va_list arguments;

    va_start(arguments, format);
    vsprintf(line, format, arguments);
    if (g_combat_log_enabled_0068d810 != 0) {
        WriteGameLog(7, &g_combat_log_format_00617664, line);
    }
}

/* Start a fresh round: lift the one action that does not survive it and clear
   the two round flags. */
// FUNCTION: WIZ8 0x004ECF00
void BeginCombatRound(void)
{
    int party_slot;

    if (g_in_combat_00683f94 == 0) {
        return;
    }
    for (party_slot = 0; party_slot < 8; ++party_slot) {
        if (g_party_slot_rows[party_slot].pending_action == W8_ACTION_LIFTED_AT_ROUND_END) {
            g_party_slot_rows[party_slot].pending_action = -1;
        }
    }
    g_combat_state->flag_a50 = 0;
    g_combat_state->flag_a51 = 0;
}

/* Bring one combatant's clock up to the current round: advance it by however
   many rounds have passed since it last caught up, clamp it, and remember
   where it got to. */
// FUNCTION: WIZ8 0x004ECEB0
void CatchUpCombatActor(unsigned int* actor)
{
    actor[0] += g_combat_state->round_counter - actor[0x27];
    ClampUnsignedInteger(actor, g_combat_state->round_counter, 100);
    Function4F0480(actor, g_combat_state->round_counter);
    actor[0x27] = g_combat_state->round_counter;
}

/* Whether one character can still take the action that costs stamina: they
   have to be free of the condition that forbids it and still hold a fifth of
   their stamina. */
// FUNCTION: WIZ8 0x004EBC80
unsigned char CanCharacterActAtCost(int party_slot)
{
    W8Character* character = &g_party_characters[party_slot];

    if (!CharacterHasCondition(character, 0x1c)) {
        return 0;
    }
    return (int)(character->stamina_max / 5) <= character->stamina;
}

/* Take one character out of the round: hand back whatever they were aiming at,
   clear the slot, and re-choose an action of the same kind. */
// FUNCTION: WIZ8 0x004E82E0
void DropCharacterFromRound(int party_slot)
{
    W8PartySlotRow* row = &g_party_slot_rows[party_slot];

    if (Function53BC10(party_slot) != 2) {
        Function53AC30(party_slot, &row->combat_slot_04d);
    }
    ResetCombatSlot((W8CombatSlot*)&row->combat_slot_04d);
    NotifySpellPointsChanged(party_slot);
    if (party_slot == g_active_character_0068518d) {
        Function565420();
    }
    Function4E8000(party_slot, row->action_kind, row->action_detail, 0, 0);
}
