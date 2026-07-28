#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"

/* Local Code\Health Stamina Mana.cpp, named by the assertions these bodies
   embed. The party sweeps in here all share one shape: walk the eight party
   slots, skip the empty ones, and hand each occupied slot to a per-character
   worker. */

#define HEALTH_STAMINA_MANA_CPP "C:\\Projects\\Wizardry 8\\Local Code\\Health Stamina Mana.cpp"

/* The eligibility window the party sweeps use, the same one GetRandomCharacter
   and AnyPartyMemberCanUseItem apply. */
enum { W8_CHARACTER_ELIGIBLE_LIMIT = 0x12 };

/* A negative amount means "as much as they could possibly hold", which the
   restore computes by summing the whole spell-point ceiling. */
enum { W8_RESTORE_EVERYTHING = -1 };

extern void ApplyHealthChangeToCharacter(
    int party_slot, int amount, int arg_3, int arg_4, int arg_5, int arg_6, int arg_7);
/* 0x0052A890 */
extern void ApplyStaminaChangeToCharacter(int party_slot, int amount, int announce);
/* 0x0052ADD0 */
extern void ApplyFatigueChangeToCharacter(int party_slot, int amount, int announce);
/* 0x0052B1C0 */
extern void ApplySpellPointChangeToCharacter(int party_slot, int arg_2, int arg_3);
/* 0x0052B590 */
extern void RestoreCharacterSpellPoints(int party_slot, int amount);
/* 0x0052B910 */
extern void NotifySpellPointsChanged(int party_slot);      /* 0x0055EE30 */
extern void Function56A2A0(W8MonsterInfo* monster_info);
extern void WriteGameLog(int channel, const wchar_t* format, ...);
W8WideChar* GetMonsterName(W8MonsterInfo* monster_info, W8MonsterRecord* record,
                           unsigned char name_form);

/* Roll the dice once per eligible party member and apply the result to each of
   them. The roll is separate per character rather than shared. */
// FUNCTION: WIZ8 0x0052A820
void ApplyRolledHealthChangeToParty(const W8Dice* dice, int arg_2, int arg_3)
{
    int party_slot;

    for (party_slot = 0; party_slot < 8; ++party_slot) {
        if (g_party_slot_rows[party_slot].flag_00 != 0 &&
            g_party_characters[party_slot].unknown_0b01 < W8_CHARACTER_ELIGIBLE_LIMIT) {
            ApplyHealthChangeToCharacter(
                party_slot, RollDice(dice), 0, arg_3, 0, arg_2, 0);
        }
    }
}

/* The stamina form: every occupied slot, announced. The dice are assembled
   from the caller's own arguments rather than passed as a record, which is why
   the two count bytes arrive separately from the base. */
// FUNCTION: WIZ8 0x0052AD70
void ApplyRolledStaminaChangeToParty(unsigned char count, unsigned char sides, short base)
{
    W8Dice dice;
    int party_slot;

    dice.base = base;
    dice.count = count;
    dice.sides = sides;
    for (party_slot = 0; party_slot < 8; ++party_slot) {
        if (g_party_slot_rows[party_slot].flag_00 != 0) {
            ApplyStaminaChangeToCharacter(party_slot, RollDice(&dice), 1);
        }
    }
}

/* The fatigue form, which is the same sweep unannounced. */
// FUNCTION: WIZ8 0x0052B160
void ApplyRolledFatigueChangeToParty(unsigned char count, unsigned char sides, short base)
{
    W8Dice dice;
    int party_slot;

    dice.base = base;
    dice.count = count;
    dice.sides = sides;
    for (party_slot = 0; party_slot < 8; ++party_slot) {
        if (g_party_slot_rows[party_slot].flag_00 != 0) {
            ApplyFatigueChangeToCharacter(party_slot, RollDice(&dice), 0);
        }
    }
}

/* Spend spell points from one realm. Spending more than is left is a caller
   error rather than something to clamp. */
// FUNCTION: WIZ8 0x0052B480
void SpendCharacterSpellPoints(int party_slot, int realm, int amount)
{
    W8Character* character = &g_party_characters[party_slot];

    if (amount != 0) {
        if (character->sp_left[realm] < amount) {
            srAssertFail("pPC->iSPLeft[uiRealm] >= (INT32) uiSPs",
                         HEALTH_STAMINA_MANA_CPP, 1067, 0);
        }
        character->sp_left[realm] -= amount;
        NotifySpellPointsChanged(party_slot);
    }
}

/* Give spell points back to one realm, never past its ceiling. */
// FUNCTION: WIZ8 0x0052B4F0
void RestoreCharacterRealmSpellPoints(int party_slot, int realm, int amount)
{
    W8Character* character = &g_party_characters[party_slot];

    character->sp_left[realm] += amount;
    if (character->sp_max[realm] < character->sp_left[realm]) {
        character->sp_left[realm] = character->sp_max[realm];
    }
    NotifySpellPointsChanged(party_slot);
}

/* The party-wide spell-point change, which does not filter on the eligibility
   window - an unconscious character's pool still moves. */
// FUNCTION: WIZ8 0x0052B550
void ApplySpellPointChangeToParty(int arg_1, int arg_2)
{
    int party_slot;

    for (party_slot = 0; party_slot < 8; ++party_slot) {
        if (g_party_slot_rows[party_slot].flag_00 != 0) {
            ApplySpellPointChangeToCharacter(party_slot, arg_1, arg_2);
        }
    }
}

/* Restore spell points across the party. A negative amount means as much as
   the character could possibly hold, which is the sum of all six realm
   ceilings. */
// FUNCTION: WIZ8 0x0052BA00
void RestorePartySpellPoints(int amount)
{
    int party_slot;
    int realm;
    int granted;

    for (party_slot = 0; party_slot < 8; ++party_slot) {
        if (g_party_slot_rows[party_slot].flag_00 != 0 &&
            g_party_characters[party_slot].unknown_0b01 < W8_CHARACTER_ELIGIBLE_LIMIT &&
            g_party_characters[party_slot].unknown_0b11 != 0) {
            granted = amount;
            if (amount < 0) {
                granted = 0;
                for (realm = 0; realm < W8_SPELL_REALM_COUNT; ++realm) {
                    granted += g_party_characters[party_slot].sp_max[realm];
                }
            }
            RestoreCharacterSpellPoints(party_slot, granted);
        }
    }
}

/* Heal one monster. A monster that is already dead or already whole is left
   alone; healing it to full says so differently from healing it partway. */
// FUNCTION: WIZ8 0x0052BFD0
void HealMonster(W8MonsterInfo* monster_info, int amount, char announce)
{
    if (monster_info->hp_current == 0 || monster_info->hp_current == monster_info->hp_max ||
        amount == 0) {
        return;
    }

    monster_info->hp_current += amount;
    if (monster_info->hp_current > monster_info->hp_max) {
        monster_info->hp_current = monster_info->hp_max;
    }
    Function56A2A0(monster_info);
    UpdateMonsterDamageAppearance(monster_info);

    if (announce) {
        if (monster_info->hp_current == monster_info->hp_max) {
            WriteGameLog(9, (const wchar_t*)g_notices[0x964 / 4],
                         GetMonsterName(monster_info, 0, 0));
        }
        else {
            WriteGameLog(9, (const wchar_t*)g_notices[0x96c / 4],
                         GetMonsterName(monster_info, 0, 0), amount);
        }
    }
}

/* What one monster action costs in fatigue, before it is spent. Four of the
   ten actions are free; the plain attack costs markedly more when its detail
   is three. A monster that tires twice as fast pays double. */
// FUNCTION: WIZ8 0x0052C240
int MonsterActionFatigueCost(const W8MonsterInfo* monster_info)
{
    int cost = 0;

    switch (monster_info->action_kind) {
    case 0:
        if (monster_info->action_detail == 3) {
            cost = Random(8) + 5;
        }
        else {
            cost = Random(3) + 2;
        }
        break;
    case 1:
    case 9:
        cost = Random(2) + 1;
        break;
    case 4:
    case 7:
        cost = Random(5) + 5;
        break;
    case 5:
    case 6:
        cost = Random(3) + 3;
        break;
    case -1:
    case 2:
    case 3:
    case 8:
        break;
    default:
        srAssertFail("FALSE", HEALTH_STAMINA_MANA_CPP, 1774,
                     FormatString("MonsterActionFatigueCost: ERROR - Invalid action %d",
                                  monster_info->action_kind));
    }

    if (monster_info->fatigue_doubled_05f == 0) {
        return cost;
    }
    return cost * 2;
}
