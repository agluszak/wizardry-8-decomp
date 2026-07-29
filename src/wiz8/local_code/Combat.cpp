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
/* The per-character combat rows begin at the combat state's own address and
   run 0xd4 bytes apart, so the state's leading fields are the first row's. */
extern unsigned char g_combat_log_enabled_0068d810;
extern const wchar_t g_combat_log_format_00617664;
extern int g_active_party_slot_0068518d;
extern void ClampUnsignedInteger(unsigned int* value, unsigned int base, unsigned int span);
extern void RoundPhaseToStep(unsigned int* actor, int round);
extern int GetCurrentTargetingContext(int party_slot);
extern void ClearTargetHighlights(int party_slot, const W8CombatSlot* target);
/* 0x0053AC30 */
extern void ResetCombatSlot(W8CombatSlot* slot);
extern void NotifySpellPointsChanged(int party_slot);
extern void RequestRedrawParty(void);
extern void Function4E8000(int party_slot, int action_kind, int action_detail, int a, int b);

/* Whether anybody in the party is engaged with something. */
// FUNCTION: WIZ8 0x004e7ca0
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
// FUNCTION: WIZ8 0x004ed2b0
int GetEngagementCount(void)
{
    if (g_combat_state->turn_phase != 0) {
        return g_combat_state->pending_move_kind;
    }
    return g_combat_state->movement_mode;
}

/* Whether the party is engaged at all. Being told so outright settles it;
   otherwise, with nothing derived, one conscious character whose combat row is
   flagged is enough. */
// FUNCTION: WIZ8 0x004e7e70
int IsPartyEngaged(void)
{
    unsigned int party_slot;

    if (g_combat_state->turn_phase != 0) {
        return 1;
    }
    if (g_combat_state->movement_mode == 0) {
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
// FUNCTION: WIZ8 0x004ecdd0
void RecordCharacterDeath(int party_slot)
{
    if (g_in_combat_00683f94 != 0) {
        g_combat_state->pending_deaths[g_combat_state->pending_death_count] = party_slot;
        ++g_combat_state->pending_death_count;
    }
}

/* Record what one slot has chosen to do, and cache whether it is the first
   kind beside it. */
// FUNCTION: WIZ8 0x004e8290
void SetSlotAction(int party_slot, int action_kind, int action_detail)
{
    W8PartySlotRow* row = &g_party_slot_rows[party_slot];

    row->action_kind = action_kind;
    row->action_detail = action_detail;
    row->action_is_kind_one = action_kind == W8_ACTION_KIND_ONE;
}

/* Post one line to the combat log, if the log is on. The whole line is
   formatted whether or not it will be shown. */
// FUNCTION: WIZ8 0x004ed260
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
// FUNCTION: WIZ8 0x004ecf00
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
// FUNCTION: WIZ8 0x004eceb0
void CatchUpCombatActor(unsigned int* actor)
{
    actor[0] += g_combat_state->round_counter - actor[0x27];
    ClampUnsignedInteger(actor, g_combat_state->round_counter, 100);
    RoundPhaseToStep(actor, g_combat_state->round_counter);
    actor[0x27] = g_combat_state->round_counter;
}

/* Whether one character can breathe again. The name is the Magic.cpp:5320
   assertion's own - assert(CanCharReBreathe(uiChar)) - and the rule is that
   they have to be free of the condition that forbids it and still hold a fifth
   of their stamina, the same fifth a run costs. */
// FUNCTION: WIZ8 0x004ebc80
unsigned char CanCharReBreathe(int party_slot)
{
    W8Character* character = &g_party_characters[party_slot];

    if (!CharacterHasCondition(character, 0x1c)) {
        return 0;
    }
    return (int)(character->stamina_max / 5) <= character->stamina;
}

/* Take one character out of the round: hand back whatever they were aiming at,
   clear the slot, and re-choose an action of the same kind. */
// FUNCTION: WIZ8 0x004e82e0
void DropCharacterFromRound(int party_slot)
{
    W8PartySlotRow* row = &g_party_slot_rows[party_slot];

    if (GetCurrentTargetingContext(party_slot) != 2) {
        ClearTargetHighlights(party_slot, &row->target_in_combat);
    }
    ResetCombatSlot(&row->target_in_combat);
    NotifySpellPointsChanged(party_slot);
    if (party_slot == g_active_party_slot_0068518d) {
        RequestRedrawParty();
    }
    Function4E8000(party_slot, row->action_kind, row->action_detail, 0, 0);
}

extern float MonsterDistanceToParty(W8MonsterInfo* monster_info);        /* 0x004C7CB0 */
extern float CalcRangeDistance(int range_category);                      /* 0x0051A9A0 */
extern void NotifyMonsterOfSound(W8Monster* monster, int arg_2);         /* 0x004C6240 */
extern unsigned char g_surprise_possible_00683fc5;

/* Tell every monster within short range about something. A monster has to be
   in combat, alive, not on its way out, free of whatever 0x087 records, and
   in the engaged state before it is told. */
// FUNCTION: WIZ8 0x004ecaa0
void NotifyNearbyMonsters(int what)
{
    unsigned int index;
    W8MonsterInfo* monster_info;

    for (index = 0; index < PListGetCount(g_active_monster_list_00683fad); ++index) {
        monster_info = MonsterGetScriptPartByLocationIndex(index);
        if (monster_info->fInCombat != 0 && monster_info->hp_current != 0 &&
            (unsigned int)monster_info->value_107 < 0xe &&
            monster_info->condition_turns[12] == 0 && monster_info->flag_16 == 1) {
            if (MonsterDistanceToParty(monster_info) <= CalcRangeDistance(1)) {
                NotifyMonsterOfSound(monster_info->monster, what);
            }
        }
    }
}

/* Whether the party notices what is coming. The first character in shape to
   act rolls against half their sixth attribute; nobody in shape at all, or the
   global being clear, and the answer is yes by default. */
// FUNCTION: WIZ8 0x004ed1c0
int PartyAvoidsSurprise(void)
{
    unsigned int party_slot = 0;

    if (g_surprise_possible_00683fc5 == 0) {
        return 0;
    }
    while (g_party_slot_rows[party_slot].flag_00 == 0 ||
           g_party_characters[party_slot].hp_current == 0 ||
           g_party_characters[party_slot].unknown_0b01 > 10) {
        ++party_slot;
        if (party_slot > 7) {
            return 1;
        }
    }
    Random(1);
    if (g_party_characters[party_slot].attributes[6].effective >> 1 <= Random(100)) {
        return 1;
    }
    return 0;
}

extern int GetHandAttackValue(int party_slot, unsigned int hand);        /* 0x0053D7F0 */
extern void ChooseCombatAction(
    int party_slot, int is_monster_turn, int* out_kind, int a, int b, int c); /* 0x004E77B0 */
extern void ApplyCharacterEffect(
    W8Character* character, void* effect, int arg_3, int arg_4, int arg_5);
extern unsigned char CharacterCanSwitchTo(int party_slot, int a, int b, int c);
/* 0x004E79A0 */
extern void SwitchCharacterTo(int party_slot, int action);               /* 0x004ED390 */
extern void MonsterChooseTarget(W8MonsterInfo* monster_info, int* out, int arg_3);
/* 0x0051AC30 */
extern void NotifyMonsterIdle(W8Monster* monster, int arg_2);            /* 0x004C6240 */
extern void NotifyMonsterFacing(W8Monster* monster, W8Monster* target, int arg_3);
/* 0x004C62C0 */
extern void Function4C6200(W8Monster* monster, int arg_2);
extern unsigned char Function5323F0(W8MonsterInfo* monster_info, int a, int b, int c);
extern void SetMonsterTurnSpeed(float speed);                            /* 0x00453C70 */
extern int MonsterActionFatigueCost(const W8MonsterInfo* monster_info);
extern void FatigueMonster(W8MonsterInfo* monster_info, unsigned int amount, int report_to);
extern void RoundPhaseToStep(unsigned int* phase, unsigned int base);
extern void RequestRedraw(unsigned int mask);
extern float GetMonsterRecordScaledFloat1BA(W8MonsterInfo* monster_info);
extern W8Monster* GetMonsterByLocationID(int location_id);
extern void* g_effect_005ee610;
extern unsigned int g_flee_hp_fraction_005ed8f8;
extern unsigned int g_flee_chance_005ed908;
extern float g_movement_speed_step_005ed490;
/* 0x00683FE7-adjacent: the per-character per-hand attack values combat saved
   when the round began, 0x35 dwords per character. */
extern int g_saved_attack_values[];

/* What one character's whole turn is worth. A character whose turn combat has
   already set up uses the values it saved; anyone else is asked afresh. A
   phase of exactly a hundred is worth one whatever the hands say. */
// FUNCTION: WIZ8 0x004ec860
int GetCharacterTurnValue(int party_slot)
{
    W8CombatCharacterRow* row = &g_combat_character_rows[party_slot];
    int chosen;
    int total = 0;
    unsigned int hand;
    int value;

    ChooseCombatAction(party_slot, row->flag_4c == 0, &chosen, 0, 0, 0);
    if (chosen != 0 && chosen != 1) {
        return 1;
    }

    for (hand = 0; hand < 2; ++hand) {
        if (row->flag_4c == 0) {
            value = GetHandAttackValue(party_slot, hand);
        }
        else {
            value = g_saved_attack_values[party_slot * 0x35 + hand];
        }
        if (row->value_18 == 100) {
            value = 1;
        }
        total += value;
    }
    return total;
}

/* Whether a wounded character panics. Only a character target counts, they
   have to be below the fraction of their hit points that triggers it, and then
   it is a roll - so the same wound does not always panic. */
// FUNCTION: WIZ8 0x004ece00
unsigned char TryPanicWoundedCharacter(const W8CombatSlot* target)
{
    W8Character* character;

    if (target->iType != W8_TARGET_KIND_CHARACTER) {
        return 0;
    }
    character = &g_party_characters[target->iChar];
    if ((character->hp_current * 100) / (unsigned int)character->hp_max >=
        g_flee_hp_fraction_005ed8f8) {
        return 0;
    }
    if (Random(100) >= g_flee_chance_005ed908) {
        return 0;
    }
    ApplyCharacterEffect(character, g_effect_005ee610, 0, g_effect_argument_005ed8c8,
                         g_effect_argument_005ed914);
    return 1;
}

/* End one monster's turn: forget what it was doing, mark its combat state
   inactive, and - if it is still in the fight - either stand it down or turn
   it to face whoever it settled on. */
// FUNCTION: WIZ8 0x004e76f0
void EndMonsterTurn(W8MonsterInfo* monster_info)
{
    int chosen[2];
    int target_location;

    monster_info->action_kind = -1;
    monster_info->action_detail = 0;
    monster_info->pCombat->phase = 0;
    monster_info->pCombat->active = 1;
    monster_info->pCombat->value_14c = 0;
    RequestRedraw(0x100000);

    if (monster_info->hp_current != 0 && (unsigned int)monster_info->value_107 < 0xe &&
        monster_info->condition_turns[12] == 0) {
        MonsterChooseTarget(monster_info, chosen, 3);
        if (chosen[0] == 2) {
            NotifyMonsterIdle(monster_info->monster, 0);
            monster_info->flag_253 = 0;
            return;
        }
        if (chosen[0] == 3) {
            target_location = chosen[1];
            NotifyMonsterFacing(monster_info->monster,
                                GetMonsterByLocationID(target_location), 0);
        }
    }
    monster_info->flag_253 = 0;
}

/* Set one monster's turn up, once. How fast it moves through the turn depends
   on what it chose - fleeing is half speed and one action is half again - and
   an enchanted monster is quickened by ten per point instead of slowed by the
   condition it is under. */
// FUNCTION: WIZ8 0x004eb8c0
void SetUpMonsterTurn(W8MonsterInfo* monster_info)
{
    unsigned int speed = 100;
    float scale;

    if (monster_info->pCombat->turn_started != 0) {
        monster_info->monster->polymorphic_subobject_18.unknown_10[0x16] = 0;
        return;
    }

    scale = GetMonsterRecordScaledFloat1BA(monster_info);
    if (monster_info->action_kind == 9) {
        speed = 0x32;
    }
    else {
        if (monster_info->action_kind == 7) {
            speed = 0x96;
        }
        if (monster_info->enchantments[5].value_08 == 0) {
            if (monster_info->condition_turns[5] != 0) {
                speed -= 0x32;
            }
        }
        else {
            speed += monster_info->enchantments[5].value_00 * 10;
        }
    }

    SetMonsterTurnSpeed(speed * scale * g_movement_speed_step_005ed490);
    /* Four bytes inside cycle eight's block, cleared together. */
    monster_info->monster->m_cycles[8].bytes_08.unknown_08 = 0;
    monster_info->monster->m_cycles[8].bytes_08.unknown_09 = 0;
    monster_info->monster->m_cycles[8].bytes_08.unknown_0a[0] = 0;
    monster_info->monster->m_cycles[8].bytes_08.unknown_0a[1] = 0;
    monster_info->pCombat->turn_started = 1;
    monster_info->monster->polymorphic_subobject_18.unknown_10[0x16] = 0;
}

/* Finish one monster's attack: charge it the fatigue, drop the party's
   selection, and give it its next phase if it still has attacks left - which
   divides whatever is left of the round between them. */
// FUNCTION: WIZ8 0x004eb7f0
void EndMonsterAttack(W8MonsterInfo* monster_info)
{
    W8MonsterCombatState* combat = monster_info->pCombat;
    unsigned int next;

    GetMonsterDataForInfo(monster_info);
    FatigueMonster(monster_info, MonsterActionFatigueCost(monster_info), 0);
    Function4C6200(monster_info->monster, 1);
    g_combat_state->selected_slot = 0;
    g_combat_state->selected_monster = 0;

    if (combat->active == 0) {
        return;
    }
    if (monster_info->action_kind == 0 && combat->attacks_per_round != 0) {
        if (Function5323F0(monster_info, 1, 0, 0)) {
            next = combat->phase +
                   (100 - g_combat_state->round_counter) / (combat->attacks_per_round + 1);
            combat->phase = next;
            if (next < 0x65) {
                RoundPhaseToStep(&combat->phase, g_combat_state->round_counter);
                return;
            }
        }
    }
    combat->phase = 0;
}

/* Whether one character may take an action, and take it if asked. A character
   whose turn combat has set up already only agrees to the action they are
   already on; anyone else may switch, except into the fourth action while
   something else forbids it. */
// FUNCTION: WIZ8 0x004ed2d0
unsigned char TryCharacterAction(int party_slot, int action, char commit)
{
    W8Character* character = &g_party_characters[party_slot];

    if (character->hp_current == 0 || character->unknown_0b01 >= 0xf) {
        return 0;
    }
    if (g_combat_character_rows[party_slot].flag_4c != 0) {
        return g_party_slot_rows[party_slot].pending_action == action;
    }
    if (*(int*)((char*)&g_party_slot_rows[party_slot] + 0x3d) != action) {
        if (action != 4) {
            return 0;
        }
        if (CharacterCanSwitchTo(party_slot, 1, 0, 0)) {
            return 0;
        }
    }
    if (commit) {
        SwitchCharacterTo(party_slot, action);
    }
    return 1;
}
