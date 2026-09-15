#include "wiz8/local_screens/Screens.h"
#include "soundman.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/character_event_queue.h"
#include "wiz8/xstatus.h"
#include "wiz8/layouts/character.h"
#include "wiz8/character_skills.h"
#include "wiz8/local_code/CharGeneration.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/GameplayMods.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/local_code/party_encumbrance.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/UtilityFunctions.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/CombatRange.h"
#include "wiz8/layouts/item_tables.h"
#include "wiz8/float_constants.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatHostility.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/layouts/npc_state.h"
#include "wiz8/local_code/NPCManager.h"
#include "wiz8/local_code/NPCScripting.h"
#include "wiz8/npc_script_file.h"
#include "wiz8/layouts/item_instance.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/character_skills.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/local_code/SpellEffect.h"
#include "wiz8/sr_api.h"
#include "wiz8/local_code/Targeting.h"
#include "wiz8/utility.h"
#include "wiz8/sound_man.h"
#include "random.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/dialog_code/DialogInterface.h"
#include "wiz8/local_screens/MGSTextBox.h"

#include "wiz8/local_code/GameplayDatabase.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/music_playlist.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/string_database.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/layouts/screen_state.h"
#include "wiz8/local_code/Gameloop.h"
#include "wiz8/npc_interaction.h"
#include "wiz8/local_code/PC_Item.h"
#include "timer.h"
#include "wiz8/local_screens/MGSPortraits.h"
#include "wiz8/dialog_code/PortraitQuote.h"
#include "wiz8/local_screens/ReviewCharacterScreen.h"
#include "wiz8/notices.h"
#include "wiz8/regions.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/engine_code/Environment.h"
#include "wiz8/local_screens/mipe.h"
#include "wiz8/engine_code/Video2.h"
#include "sgp.h"
#undef S32
#undef U32
#include "bink.h"
#include "wiz8/bink_video.h"
#include "FileMan.h"

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include "wiz8/layouts/game_status.h"
#include "wiz8/local_screens/OptionsScreen.h"

/* Local Code\Health Stamina Mana.cpp, named by the assertions these bodies
   embed. The party sweeps in here all share one shape: walk the eight party
   slots, skip the empty ones, and hand each occupied slot to a per-character
   worker. */

#define HEALTH_STAMINA_MANA_CPP "C:\\Projects\\Wizardry 8\\Local Code\\Health Stamina Mana.cpp"

// FUNCTION: WIZ8 0x0052A890
unsigned int ApplyDamageToCharacter(int party_slot, unsigned int amount, char arg_3, char arg_4,
                                    char arg_5, W8SpellEffectResult* result_stats, char arg_7)
{
    W8Character* character = &g_status_685170.buffers.characters[party_slot];
    unsigned int absorbed;
    unsigned int applied;

    if (g_status_685170.buffers.party_rows[party_slot].occupied == 0) {
        srAssertFail("fCHAR_OCCUPIED(uiChar)", HEALTH_STAMINA_MANA_CPP, 403, 0);
    }
    if (character->hp_current == 0) {
        return 0;
    }
    if (g_status_685170.value_2390 != 0) {
        PostCharacterNotice(party_slot, gppStringList[0x94c / 4], amount);
        return 0;
    }

    if (character->enchantments[2].value_08 != 0) {
        absorbed = character->enchantments[2].value_06;
        if (amount <= absorbed) {
            PostCharacterNotice(party_slot, gppStringList[0x193 - (arg_3 != 0)], amount);
            character->enchantments[2].value_06 = static_cast<unsigned short>(absorbed - amount);
            if (result_stats != 0) {
                ++result_stats->count;
            }
            return 0;
        }

        PostCharacterNotice(party_slot, gppStringList[0x193 - (arg_3 != 0)], absorbed);
        amount -= absorbed;
        ClearCharacterEnchantmentSlot(party_slot, 2);
        PostCharacterNotice(party_slot, gppStringList[0x650 / 4]);
    }

    FatigueCharacter(party_slot, (amount * 2) / 3, 0, result_stats);
    if (result_stats != 0) {
        result_stats->amount += amount;
        ++result_stats->count;
    }

    if (arg_5 != 0) {
        if (arg_7 != 0) {
            PostCharacterNotice(party_slot, gppStringList[0x9a0 / 4], amount);
        } else if (arg_4 != 0) {
            WriteGameLogAmount(9, gppStringList[0x950 / 4], amount);
        } else {
            PostCharacterNotice(party_slot, gppStringList[0x954 / 4], amount,
                                arg_3 != 0 ? gppStringList[0x95c / 4] : &g_wchar_00689b34);
        }
    }

    applied = character->hp_current;
    if (applied <= amount) {
        if (CharacterHasTrait00547940(character, 2) != 0 &&
            character->condition_turns[W8_CONDITION_EXHAUSTED] < 7) {
            Function547A50(party_slot);
            RecordCharacterDamage(party_slot, amount);
            return applied;
        }
        if (applied < amount) {
            amount = applied;
        }
    }

    character->hp_current = applied - amount;
    RecordCharacterDamage(party_slot, amount);
    if (character->hp_current != 0) {
        if (gXStatus.fSurprisePossible == 0) {
            QueueDamageReactionEvents(character);
        }
    } else {
        if (result_stats != 0) {
            W8SpellDamageReport* report =
                static_cast<W8SpellDamageReport*>(malloc(sizeof(W8SpellDamageReport)));
            if (report != 0) {
                memset(report, 0, sizeof(W8SpellDamageReport));
                report->kind = 1;
                report->value = party_slot;
                result_stats->reports.Add(report);
            }
        }
        SetCharacterCondition(party_slot, W8_CONDITION_DEAD, W8_CONDITION_INDEFINITE, 0, 0,
                              result_stats == 0);
    }

    if (character->condition_turns[W8_CONDITION_ASLEEP] != 0 && arg_3 == 0 &&
        Random(100) < (character->attributes[6].effective >> 1) + 0x32) {
        RemoveCharacterCondition(party_slot, W8_CONDITION_ASLEEP, 1);
    }
    return amount;
}

/* Stamina and realm spell-point constants the encodings keep as addressable
   storage rather than immediates. */
// GLOBAL: WIZ8 0x005ed8b8
float g_float_005ed8b8 = 0.1f;
// GLOBAL: WIZ8 0x005ec3f8
float g_float_005ec3f8 = 125.0f;
// GLOBAL: WIZ8 0x005ecbb4
float g_float_005ecbb4 = 0.02f;

/* The eligibility window the party sweeps use, the same one GetRandomCharacter
   and AnyPartyMemberCanUseItem apply: highest_condition below death. */

/* A negative amount means "as much as they could possibly hold", which the
   restore computes by summing the whole spell-point ceiling. */
enum { W8_RESTORE_EVERYTHING = -1 };

/* Roll the dice once per eligible party member and apply the result to each of
   them. The roll is separate per character rather than shared. */
// FUNCTION: WIZ8 0x0052a820
void ApplyRolledHealthChangeToParty(const W8Dice* dice, W8SpellEffectResult* result, int arg_3)
{
    int party_slot;

    for (party_slot = 0; party_slot < 8; ++party_slot) {
        if (g_status_685170.buffers.party_rows[party_slot].occupied != 0 &&
            g_status_685170.buffers.characters[party_slot].highest_condition < W8_CONDITION_DEAD) {
            ApplyDamageToCharacter(party_slot, RollDice(dice), 0, arg_3, 0, result, 0);
        }
    }
}

/* Heal every occupied slot by its own roll, announced. The dice are assembled
   from the caller's own arguments rather than passed as a record, which is why
   the two count bytes arrive separately from the base. */
// FUNCTION: WIZ8 0x0052ad70
void HealPartyByDice(unsigned char count, unsigned char sides, short base)
{
    W8Dice dice;
    int party_slot;

    dice.base = base;
    dice.count = count;
    dice.sides = sides;
    for (party_slot = 0; party_slot < 8; ++party_slot) {
        if (g_status_685170.buffers.party_rows[party_slot].occupied != 0) {
            HealCharacter(party_slot, RollDice(&dice), 1);
        }
    }
}

/* The stamina form, which is the same sweep unannounced. */
// FUNCTION: WIZ8 0x0052b160
void RestorePartyStaminaByDice(unsigned char count, unsigned char sides, short base)
{
    W8Dice dice;
    int party_slot;

    dice.base = base;
    dice.count = count;
    dice.sides = sides;
    for (party_slot = 0; party_slot < 8; ++party_slot) {
        if (g_status_685170.buffers.party_rows[party_slot].occupied != 0) {
            RestoreCharacterStamina(party_slot, RollDice(&dice), 0);
        }
    }
}

/* Spend spell points from one realm. Spending more than is left is a caller
   error rather than something to clamp. */
// FUNCTION: WIZ8 0x0052b480
void SpendCharacterSpellPoints(int party_slot, int realm, int amount)
{
    W8Character* character = &g_status_685170.buffers.characters[party_slot];

    if (amount != 0) {
        if (character->sp_left[realm] < amount) {
            srAssertFail("pPC->iSPLeft[uiRealm] >= (INT32) uiSPs", HEALTH_STAMINA_MANA_CPP, 1067,
                         0);
        }
        character->sp_left[realm] -= amount;
        RequestPartySlotRedraw(party_slot);
    }
}

/* Give spell points back to one realm, never past its ceiling. */
// FUNCTION: WIZ8 0x0052b4f0
void RestoreCharacterRealmSpellPoints(int party_slot, int realm, int amount)
{
    W8Character* character = &g_status_685170.buffers.characters[party_slot];

    character->sp_left[realm] += amount;
    if (character->sp_max[realm] < character->sp_left[realm]) {
        character->sp_left[realm] = character->sp_max[realm];
    }
    RequestPartySlotRedraw(party_slot);
}

/* Drain spell points across the party. Unlike its neighbours this does not
   filter on the eligibility window - an unconscious character still loses
   points. */
// FUNCTION: WIZ8 0x0052b550
void DrainPartySpellPoints(int arg_1, int arg_2)
{
    int party_slot;

    for (party_slot = 0; party_slot < 8; ++party_slot) {
        if (g_status_685170.buffers.party_rows[party_slot].occupied != 0) {
            DrainCharacterSpellPoints(party_slot, arg_1, arg_2);
        }
    }
}

/* Restore spell points across the party. A negative amount means as much as
   the character could possibly hold, which is the sum of all six realm
   ceilings. */
// FUNCTION: WIZ8 0x0052ba00
void RestorePartySpellPoints(int amount)
{
    int party_slot;
    int realm;
    int granted;

    for (party_slot = 0; party_slot < 8; ++party_slot) {
        if (g_status_685170.buffers.party_rows[party_slot].occupied != 0 &&
            g_status_685170.buffers.characters[party_slot].highest_condition < W8_CONDITION_DEAD &&
            g_status_685170.buffers.characters[party_slot].hp_current != 0) {
            granted = amount;
            if (amount < 0) {
                granted = 0;
                for (realm = 0; realm < W8_SPELL_REALM_COUNT; ++realm) {
                    granted += g_status_685170.buffers.characters[party_slot].sp_max[realm];
                }
            }
            RestoreCharacterSpellPointsEvenly(party_slot, granted);
        }
    }
}

/* Suffix the damage notice carries when a poison tick is what hurt the
   monster. */
// GLOBAL: WIZ8 0x0061C964
const wchar_t g_poison_suffix_0061c964[] = L"POISON ";

/* Roll the dice once for every live monster inside the radius of a point and
   apply each roll as damage through the monster-side effect pass. */
// FUNCTION: WIZ8 0x0052BA80
void DamageMonstersInRadius(const srVector3T<float>& center, float radius, const W8Dice* dice,
                            W8TargetSource* source, W8SpellEffectResult* result)
{
    unsigned int index;
    int damage;
    W8MonsterInfo* monster_info;
    srVector3T<float> location;
    srVector3T<float> offset;

    for (index = 0; index < ILLength((W8IList*)gXStatus.plsMonsterList); ++index) {
        monster_info = MonsterGetScriptPartByLocationIndex(index);
        if (monster_info->fActive != 0) {
            MonsterGetLocation(monster_info->monster, &location);
            offset = srVector3T<float>(center.x - location.x, center.y - location.y,
                                       center.z - location.z);
            if (offset.Length() <= radius) {
                damage = RollDice(dice);
                ApplyDamageToMonster(monster_info, damage, source, 0, 1, 0, result, 0);
            }
        }
    }
}

/* Apply rolled damage to a monster: the slot-2 enchantment absorbs first,
   the result block collects the hit, combat and threat state decide whether
   the blow is announced and remembered, two thirds of it fatigue the monster,
   and the remainder comes off hit points - a lethal hit logs a kill record
   and starts the death sequence. Returns the amount actually applied. */
// FUNCTION: WIZ8 0x0052BB60
unsigned int ApplyDamageToMonster(W8MonsterInfo* monster_info, unsigned int amount,
                                  W8TargetSource* source, char quiet, unsigned char in_combat,
                                  char a, W8SpellEffectResult* result_stats, char c)
{
    unsigned int absorbed;
    unsigned int applied;
    unsigned int category;
    W8SpellDamageReport* report;

    if (monster_info->hp_current == 0) {
        return 0;
    }
    if (monster_info->enchantments[2].value_08 != 0) {
        absorbed = monster_info->enchantments[2].value_06;
        if (amount <= absorbed) {
            PostMonsterNotice(monster_info, gppStringList[0x193 - (quiet != 0)], amount);
            monster_info->enchantments[2].value_06 = static_cast<unsigned short>(absorbed - amount);
            if (result_stats != 0) {
                ++result_stats->count;
            }
            return 0;
        }
        PostMonsterNotice(monster_info, gppStringList[0x193 - (quiet != 0)], absorbed);
        amount -= absorbed;
        ClearMonsterEnchantmentSlot(monster_info->location_id, 2);
    }
    if (result_stats != 0) {
        result_stats->amount += amount;
        ++result_stats->count;
    }
    if (amount != 0) {
        if (gXStatus.fCombatMode != 0 || monster_info->party_threat.flag_25 != 0) {
            Function48F650(monster_info, 0, 1);
            category = 9;
            if (TargetSourceIsCharacter(source, 0) != 0 && source->iChar != -1) {
                category = 8;
            }
            if (in_combat != 0) {
                if (c != 0) {
                    WriteGameLogAmount(category,
                                       FormatWideString(g_format_s_space_s_00617584,
                                                        GetMonsterName(monster_info, 0, 0),
                                                        gppStringList[0x9a0 / 4], amount));
                } else if (a != 0) {
                    WriteGameLogAmount(category, gppStringList[0x950 / 4], amount);
                } else {
                    WriteGameLogAmount(category, gppStringList[0x958 / 4],
                                       GetMonsterName(monster_info, 0, 0), amount,
                                       quiet != 0 ? g_poison_suffix_0061c964 : &g_wchar_00689b34);
                }
            }
        }
        if (source->fBackfire == 0 && source->fReflection == 0 && source->unknown_1d[1] == 0 &&
            quiet == 0) {
            monster_info->condition_target_304 = *source;
            if (monster_info->fInCombat != 0 && TargetSourceIsCharacter(source, 0) != 0 &&
                source->iChar != -1) {
                monster_info->pCombat->character_hate[source->iChar] += amount;
            }
        }
        applied = monster_info->hp_current;
        if (amount < applied) {
            monster_info->hp_current = applied - amount;
            FatigueMonster(monster_info, (amount * 2) / 3, result_stats);
            MonsterReactsToBeingStruck(monster_info, source, quiet);
        } else {
            monster_info->hp_current = 0;
            if (result_stats != 0) {
                report = static_cast<W8SpellDamageReport*>(malloc(sizeof(W8SpellDamageReport)));
                if (report != 0) {
                    memset(report, 0, sizeof(W8SpellDamageReport));
                    report->kind = 3;
                    wcscpy(report->text, GetMonsterName(monster_info, 0, 0));
                    result_stats->reports.Add(report);
                }
            }
            MonsterStartsDying(monster_info, result_stats == 0);
        }
        ClearHighlightIfItIs(&monster_info->location_id);
        UpdateMonsterDamageAppearance(monster_info);
        if (monster_info->monster != 0) {
            Function4C6C30(monster_info->monster, amount);
        }
    }
    return amount;
}

/* Heal one monster. A monster that is already dead or already whole is left
   alone; healing it to full says so differently from healing it partway. */
// FUNCTION: WIZ8 0x0052bfd0
void HealMonster(W8MonsterInfo* monster_info, unsigned int amount, char announce)
{
    if (monster_info->hp_current == 0 ||
        monster_info->hp_current == static_cast<unsigned int>(monster_info->hp_max) ||
        amount == 0) {
        return;
    }

    monster_info->hp_current += amount;
    if (monster_info->hp_current > static_cast<unsigned int>(monster_info->hp_max)) {
        monster_info->hp_current = monster_info->hp_max;
    }
    ClearHighlightIfItIs(&monster_info->location_id);
    UpdateMonsterDamageAppearance(monster_info);

    if (announce) {
        if (monster_info->hp_current == static_cast<unsigned int>(monster_info->hp_max)) {
            WriteGameLog(9, gppStringList[0x964 / 4], GetMonsterName(monster_info, 0, 0));
        } else {
            WriteGameLog(9, gppStringList[0x96c / 4], GetMonsterName(monster_info, 0, 0), amount);
        }
    }
}

/* What one monster action costs in fatigue, before it is spent. Four of the
   ten actions are free; the plain attack costs markedly more when its detail
   is three. A monster that tires twice as fast pays double. */
// FUNCTION: WIZ8 0x0052c240
int MonsterActionFatigueCost(const W8MonsterInfo* monster_info)
{
    int cost = 0;

    switch (monster_info->action_kind) {
    case 0:
        if (monster_info->action_detail == 3) {
            cost = Random(8) + 5;
        } else {
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

    if (monster_info->condition_turns[W8_CONDITION_FATIGUE_DOUBLED] == 0) {
        return cost;
    }
    return cost * 2;
}

/* The two fatigue-band thresholds and the four bands they cut the stamina
   fraction into. Both characters and monsters use the same ladder. */
enum {
    W8_FATIGUE_BAND_1 = 0x32,
    W8_FATIGUE_BAND_2 = 0x46,
    W8_FATIGUE_BAND_3 = 0x55,
    W8_FATIGUE_BAND_4 = 0x5e
};

/* How much stamina shakes exhaustion off again. */
enum { W8_STAMINA_TO_SHAKE_OFF_EXHAUSTION = 9 };

/* 0x00590950 */

// FUNCTION: WIZ8 0x0052a710
int GetCharacterRealmSpellPoints(const W8Character* character, int realm)
{
    int points = character->sp_left[realm];
    return points > 0 ? points : 0;
}

// FUNCTION: WIZ8 0x0052a730
int SumCharacterSpellPointsLeft(const W8Character* character)
{
    int total = 0;
    for (int realm = 0; realm < W8_SPELL_REALM_COUNT; ++realm) {
        total += character->sp_left[realm] > 0 ? character->sp_left[realm] : 0;
    }
    return total;
}

// FUNCTION: WIZ8 0x0052a7d0
unsigned int FatigueArmorPenalty(int fatigue_band)
{
    switch (fatigue_band) {
    case 0:
        return 0;
    case 1:
        return 5;
    case 2:
        return 10;
    case 3:
        return 20;
    case 4:
        return 40;
    default:
        return fatigue_band;
    }
}

/* Two effects the party is holding that a wounded character can no longer
   sustain, and the third that only the deeper threshold breaks. */
// GLOBAL: WIZ8 0x005ed904
unsigned int g_effect_threshold_005ed904 = 50;
// GLOBAL: WIZ8 0x005ed900
unsigned int g_effect_threshold_005ed900 = 70;
/* 0x0061E518: one notice index per spell realm, giving the realm's name. */
/* Turn a pool fraction into a band. The same ladder decides a character's
   fatigue band and a monster's, from the percentage of the pool that is
   missing rather than the part that is left. */
static int FatigueBandFromMissing(int missing_percent)
{
    if (missing_percent < W8_FATIGUE_BAND_1) {
        return 0;
    }
    if (missing_percent < W8_FATIGUE_BAND_2) {
        return 1;
    }
    if (missing_percent < W8_FATIGUE_BAND_3) {
        return 2;
    }
    return (missing_percent > W8_FATIGUE_BAND_4) + 3;
}

/* Heal one character, never past their maximum. Recovering enough of their
   hit points breaks the effects that only held while they were badly hurt -
   the deeper threshold breaks two more than the shallower one. */
// FUNCTION: WIZ8 0x0052add0
void HealCharacter(int party_slot, int amount, char announce)
{
    W8Character* character = &g_status_685170.buffers.characters[party_slot];
    unsigned int hp_max;
    unsigned int fraction;

    if (g_status_685170.buffers.party_rows[party_slot].occupied == 0) {
        srAssertFail("fCHAR_OCCUPIED(uiChar)", HEALTH_STAMINA_MANA_CPP, 661, 0);
    }

    if (character->hp_current == 0) {
        return;
    }
    hp_max = character->hp_max;
    if (character->hp_current == hp_max) {
        return;
    }

    character->hp_current += amount;
    if (character->hp_current > hp_max) {
        character->hp_current = hp_max;
    }
    if (announce) {
        if (character->hp_current == hp_max) {
            PostCharacterNotice(party_slot, gppStringList[0x960 / 4]);
        } else {
            PostCharacterNotice(party_slot, gppStringList[0x968 / 4], amount);
        }
    }

    fraction = (character->hp_current * 100) / (unsigned int)character->hp_max;
    if (fraction >= g_effect_threshold_005ed904) {
        if (gXStatus.character_event_queue->HasEventCharacter(g_effect_005ee594, party_slot)) {
            gXStatus.character_event_queue->SetEventCharacterMask(g_effect_005ee594, party_slot, 0);
        }
        if (fraction >= g_effect_threshold_005ed900) {
            if (gXStatus.character_event_queue->HasEventCharacter(g_effect_005ee590, party_slot)) {
                gXStatus.character_event_queue->SetEventCharacterMask(g_effect_005ee590, party_slot,
                                                                      0);
            }
            if (gXStatus.character_event_queue->HasEventCharacter(g_effect_005ee5f8, party_slot)) {
                gXStatus.character_event_queue->SetEventCharacterMask(g_effect_005ee5f8, party_slot,
                                                                      0);
            }
        }
    }
}

/* Give one character stamina back, never past their maximum. Any move re-bands
   their fatigue, and a change of band re-runs the armour class pass because
   fatigue feeds it. Enough stamina also shakes off exhaustion. */
// FUNCTION: WIZ8 0x0052b1c0
void RestoreCharacterStamina(int party_slot, int amount, char announce)
{
    W8Character* character = &g_status_685170.buffers.characters[party_slot];
    int stamina_max;
    int previous_band;
    int band;

    if (character->highest_condition >= W8_CONDITION_DEAD || character->hp_current == 0) {
        return;
    }
    stamina_max = character->stamina_max;
    if (character->stamina == stamina_max) {
        return;
    }

    character->stamina += amount;
    if (character->stamina > stamina_max) {
        character->stamina = stamina_max;
    }
    if (announce) {
        if (character->stamina == stamina_max) {
            PostCharacterNotice(party_slot, gppStringList[0x970 / 4]);
        } else {
            PostCharacterNotice(party_slot, gppStringList[0x978 / 4], amount);
        }
    }

    previous_band = character->fatigue_band;
    band = FatigueBandFromMissing(
        100 - (int)((character->stamina * 100) / (unsigned int)character->stamina_max));
    character->fatigue_band = band;
    if (band != previous_band) {
        CalcArmorClasses(character);
    }
    if (character->condition_turns[W8_CONDITION_EXHAUSTED] == W8_CONDITION_INDEFINITE &&
        character->stamina > W8_STAMINA_TO_SHAKE_OFF_EXHAUSTION) {
        RemoveCharacterCondition(party_slot, W8_CONDITION_EXHAUSTED, 1);
    }
}

/* Drain spell points from one character, taking them from randomly chosen
   realms until the whole amount is gone or fifty attempts have been spent.
   Each realm only gives up what it has, and each withdrawal is announced with
   that realm's name. */
// FUNCTION: WIZ8 0x0052b590
void DrainCharacterSpellPoints(int party_slot, unsigned int amount, char announce)
{
    W8Character* character = &g_status_685170.buffers.characters[party_slot];
    unsigned int remaining = amount;
    unsigned int taken;
    int attempts;
    int realm;

    if (character->hp_current == 0) {
        return;
    }
    if (g_status_685170.value_2390 != 0) {
        PostCharacterNotice(party_slot, gppStringList[0x980 / 4], amount);
        return;
    }

    for (attempts = 0x32; remaining != 0 && attempts != 0; --attempts) {
        realm = Random(W8_SPELL_REALM_COUNT);
        if (character->sp_left[realm] > 0) {
            taken = remaining;
            if ((unsigned int)character->sp_left[realm] <= remaining) {
                taken = character->sp_left[realm];
            }
            SpendCharacterSpellPoints(party_slot, realm, taken);
            if (announce) {
                WriteGameLog(8, gppStringList[0x98c / 4], amount,
                             gppStringList[g_realm_message_offsets[realm]]);
            }
            remaining = amount - taken;
            amount = remaining;
        }
    }
}

/* Spread spell points across a character's six realms, always topping up the
   realm that is furthest from full. The realms are sorted by how far short
   they are, then handed a point each in turn; ties are given a point together
   so the deficits stay level. */
// FUNCTION: WIZ8 0x0052b910
void RestoreCharacterSpellPointsEvenly(int party_slot, int amount)
{
    W8Character* character = &g_status_685170.buffers.characters[party_slot];
    struct {
        unsigned int realm;
        unsigned int deficit;
    } order[W8_SPELL_REALM_COUNT];
    unsigned int index;
    int granted = 0;
    bool tied;

    for (index = 0; index < W8_SPELL_REALM_COUNT; ++index) {
        order[index].realm = index;
        order[index].deficit = character->sp_max[index] - character->sp_left[index];
    }
    qsort(order, W8_SPELL_REALM_COUNT, sizeof(order[0]), CompareSpellPointDeficits);

    for (;;) {
        if (amount == 0) {
            break;
        }
        for (index = 0; index < W8_SPELL_REALM_COUNT; ++index) {
            if (order[index].deficit == 0 || (index != W8_SPELL_REALM_COUNT - 1 &&
                                              order[index].deficit < order[index + 1].deficit)) {
                if (index == W8_SPELL_REALM_COUNT - 1) {
                    PostCharacterNotice(party_slot, gppStringList[0x68c / 4], granted);
                    return;
                }
                continue;
            }
            tied = index < W8_SPELL_REALM_COUNT && order[index].deficit == order[index + 1].deficit;
            ++character->sp_left[order[index].realm];
            --amount;
            ++granted;
            --order[index].deficit;
            if (!tied) {
                break;
            }
            if (amount == 0) {
                PostCharacterNotice(party_slot, gppStringList[0x68c / 4], granted);
                return;
            }
        }
    }
    PostCharacterNotice(party_slot, gppStringList[0x68c / 4], granted);
}

/* Wound one character. Two thirds of the damage also tires them, the damage
   itself is booked against the hit-point adjustment and the pools rebuilt from
   it, and a character with no protection against it is put under condition
   one. */
// FUNCTION: WIZ8 0x0052b7e0
void DamageCharacter(int party_slot, int damage, char announce)
{
    W8Character* character = &g_status_685170.buffers.characters[party_slot];

    if (g_status_685170.buffers.party_rows[party_slot].occupied == 0) {
        srAssertFail("fCHAR_OCCUPIED(uiChar)", HEALTH_STAMINA_MANA_CPP, 1186, 0);
    }

    if (character->hp_max != 0 && character->hp_current != 0) {
        FatigueCharacter(party_slot, (damage * 2) / 3, 0, 0);
        if (announce) {
            WriteGameLog(8, gppStringList[0x710 / 4], damage);
        }
        character->hp_adjustment -= damage;
        RecalculateCharacterHitPoints(character);
        if (character->condition_turns[1] == 0) {
            SetCharacterCondition(party_slot, 1, W8_CONDITION_INDEFINITE, 0, 0, 0);
        }
        if (character->hp_current != 0) {
            QueueDamageReactionEvents(character);
        }
    }
}

/* Tire one monster. Running its pool down to nothing puts it under the
   exhausted condition indefinitely, and tells whoever asked for the fatigue
   that it landed. */
// FUNCTION: WIZ8 0x0052c070
void FatigueMonster(W8MonsterInfo* monster_info, unsigned int amount,
                    W8SpellEffectResult* report_to)
{
    W8TargetSource target_block;

    if (monster_info->hp_current == 0) {
        return;
    }
    if (amount != 0) {
        GetMonsterDataForInfo(monster_info);
        if (amount < static_cast<unsigned int>(monster_info->stamina)) {
            monster_info->stamina -= amount;
        } else {
            monster_info->stamina = 0;
        }
    }

    monster_info->fatigue_band = FatigueBandFromMissing(
        100 - static_cast<int>((monster_info->stamina * 100) /
                               static_cast<unsigned int>(monster_info->stamina_max)));

    if (monster_info->stamina == 0 &&
        monster_info->condition_turns[W8_CONDITION_EXHAUSTED] < W8_CONDITION_INDEFINITE) {
        ResetTargetSource(&target_block);
        SetMonsterCondition(monster_info->location_id, W8_CONDITION_EXHAUSTED,
                            W8_CONDITION_INDEFINITE, 0, &target_block, report_to == 0);
        if (report_to != 0) {
            ++report_to->condition_counts[W8_CONDITION_EXHAUSTED];
        }
    }
}

/* A completed spell costs its database level plus the cast result. Result
   eight is the one outcome that carries no stamina charge. MonsterCastsSpell
   deliberately returns this value to its caller. */
// FUNCTION: WIZ8 0x0052c320
unsigned int SpellCastFatigueCost(int spell_id, int result)
{
    if (result == 8) {
        return 0;
    }
    return g_spell_records[spell_id].spell_level + result;
}

/* Give one monster stamina back, the mirror of the character form down to the
   band ladder and the exhaustion it shakes off. */
// FUNCTION: WIZ8 0x0052c140
void RestoreMonsterStamina(W8MonsterInfo* monster_info, int amount, char announce)
{
    unsigned int stamina_max;

    if (monster_info->highest_condition >= W8_CONDITION_DEAD || monster_info->hp_current == 0) {
        return;
    }
    stamina_max = monster_info->stamina_max;
    if (static_cast<unsigned int>(monster_info->stamina) == stamina_max) {
        return;
    }

    monster_info->stamina += amount;
    if (static_cast<unsigned int>(monster_info->stamina) > stamina_max) {
        monster_info->stamina = stamina_max;
    }
    if (announce) {
        if (static_cast<unsigned int>(monster_info->stamina) == stamina_max) {
            WriteGameLog(9, gppStringList[0x974 / 4], GetMonsterName(monster_info, 0, 0));
        } else {
            WriteGameLog(9, gppStringList[0x97c / 4], GetMonsterName(monster_info, 0, 0), amount);
        }
    }

    monster_info->fatigue_band = FatigueBandFromMissing(
        100 - static_cast<int>((monster_info->stamina * 100) /
                               static_cast<unsigned int>(monster_info->stamina_max)));

    if (monster_info->condition_turns[W8_CONDITION_EXHAUSTED] == W8_CONDITION_INDEFINITE &&
        static_cast<unsigned int>(monster_info->stamina) > W8_STAMINA_TO_SHAKE_OFF_EXHAUSTION) {
        ClearMonsterCondition(monster_info->location_id, W8_CONDITION_EXHAUSTED);
    }
}

/* How a monster answers being struck. It plays the struck cycle; a monster
   with the right make-up may be knocked into another condition, its chance
   riding on half its fifth converted attribute; a monster the party had under
   control is handed back to itself; and if the attacker is not already an
   enemy, the disposition check decides whether being hit makes them one. */
// FUNCTION: WIZ8 0x0052beb0
void MonsterReactsToBeingStruck(W8MonsterInfo* monster_info, W8TargetSource* attacker, char quiet)
{
    StartMonsterCycle(monster_info, 0x14, 1);

    if (monster_info->condition_turns[15] != 0 && quiet == 0 &&
        Random(100) < static_cast<unsigned int>((monster_info->attributes[4] >> 1) + 0x32)) {
        ClearMonsterCondition(monster_info->location_id, W8_CONDITION_ASLEEP);
    }
    if (monster_info->control_state == 1) {
        SetMonsterControlState(monster_info, 0);
    }

    if (!TargetSourceIsCharacter(attacker, 0) && !TargetSourceIsMonster(attacker, 0)) {
        return;
    }
    if (attacker->fBackfire == 0 && attacker->fReflection == 0 && attacker->unknown_1d[1] == 0 &&
        quiet == 0 && monster_info->condition_turns[W8_CONDITION_HOSTILE] != 0) {
        if (TargetSourceIsCharacter(attacker, 0)) {
            if (MonsterVsCharDisposition(attacker->iChar, monster_info) == 2) {
                ApplyMonsterCondition(monster_info->location_id, 0xd, 1);
            }
        } else if (MonsterHostility00546F80(
                       MonsterInfoFromID(1570, HEALTH_STAMINA_MANA_CPP, attacker->iMonsterID, 1),
                       monster_info) == 2) {
            ApplyMonsterCondition(monster_info->location_id, 0xd, 1);
        }
    }
}

/* The five load categories and what each costs on top of an action's base
   fatigue, as a percentage. */
enum { W8_LOAD_CATEGORY_COUNT = 5 };
static const int kLoadFatiguePercent[W8_LOAD_CATEGORY_COUNT] = {0, 0x19, 0x32, 100, 200};

/* The band past which deep fatigue takes hold, and the band it lets go at. */
enum { W8_FATIGUE_BAND_DEEP = 2, W8_FATIGUE_BAND_RECOVERED = 2 };

/* Tire one character. The load they are carrying scales the cost - eased or
   worsened by the two load modifiers - and the result is taken out of their
   stamina without going below nothing. Running out puts them under the
   exhausted condition; merely dropping into the deep band applies the
   deep-fatigue effect once. */
// FUNCTION: WIZ8 0x0052af50
void FatigueCharacter(int party_slot, int amount, char scale_by_load,
                      W8SpellEffectResult* report_to)
{
    W8Character* character = &g_status_685170.buffers.characters[party_slot];
    int previous_band;
    int band;
    int load_percent;

    if (character->stamina <= 0 || character->hp_current == 0 || amount == 0) {
        return;
    }

    if (scale_by_load) {
        if ((unsigned int)character->load_category >= W8_LOAD_CATEGORY_COUNT) {
            srAssertFail("FALSE", HEALTH_STAMINA_MANA_CPP, 813,
                         "FatigueCharacter: ERROR - Invalid load category");
        }
        load_percent = kLoadFatiguePercent[character->load_category];
        if (character->condition_turns[W8_CONDITION_LOAD_EASED] == 0) {
            if (character->enchantments[5].value_08 != 0) {
                load_percent += 0x19;
            }
        } else {
            load_percent -= 0x19;
        }
        amount += (load_percent * amount) / 100;
    }

    if (amount < 0) {
        amount = 0;
    } else if (amount > character->stamina) {
        amount = character->stamina;
    }
    character->stamina -= amount;

    previous_band = character->fatigue_band;
    band = FatigueBandFromMissing(
        100 - (int)((character->stamina * 100) / (unsigned int)character->stamina_max));
    character->fatigue_band = band;
    if (band != previous_band) {
        CalcArmorClasses(character);
    }

    if (character->stamina < 1) {
        if (character->condition_turns[W8_CONDITION_EXHAUSTED] < W8_CONDITION_INDEFINITE) {
            SetCharacterCondition(party_slot, W8_CONDITION_EXHAUSTED, W8_CONDITION_INDEFINITE, 0, 0,
                                  report_to == 0);
            if (report_to != 0) {
                ++report_to->condition_counts[W8_CONDITION_EXHAUSTED];
            }
        }
    } else if (band != previous_band && band > W8_FATIGUE_BAND_DEEP) {
        if (character->deep_fatigue_applied == 0) {
            ApplyCharacterEffect(character, g_effect_005ee598, 0, g_effect_argument_005ed8c8,
                                 g_effect_argument_005ed914);
            character->deep_fatigue_applied = 1;
        }
        if ((unsigned int)character->fatigue_band < W8_FATIGUE_BAND_RECOVERED) {
            character->deep_fatigue_applied = 0;
        }
    }
}

/* What one character action costs in fatigue before it is spent. An attack
   costs by the weight of what is swung - unarmed and the two special attack
   modes are flat rolls instead - a run costs a fifth of the character's whole
   stamina with a floor, and the rest are free. A character who tires twice as
   fast pays double. */
// FUNCTION: WIZ8 0x0052b2f0
unsigned int CharacterActionFatigueCost(int party_slot, int action_kind)
{
    unsigned int cost = 0;
    W8CombatCharacterRow* combat_row = &g_combat_state->characters[party_slot];
    int attack_mode;
    int item_id;
    int weight_bands;

    switch (action_kind) {
    case W8_ACTION_ATTACK:
    case W8_ACTION_BERSERK:
        attack_mode =
            g_status_685170.buffers.party_rows[party_slot].attack_mode[combat_row->current_hand];
        if (attack_mode == 5) {
            cost = Random(3) + 2;
        } else if (attack_mode == 6) {
            cost = Random(3) + 3;
        } else {
            item_id = g_status_685170.buffers.characters[party_slot]
                          .equipment[combat_row->current_equip_slot]
                          .item_id;
            if (item_id == -1) {
                cost = Random(4) + 3;
            } else {
                weight_bands = g_item_records[item_id].weight / 0x28 + 1;
                cost = Random(weight_bands) + 1 + weight_bands;
            }
        }
        if (action_kind == W8_ACTION_BERSERK) {
            cost *= 2;
        }
        break;
    case W8_ACTION_BREATHE:
    case W8_ACTION_DEFEND:
    case W8_ACTION_PROTECT:
    case W8_ACTION_CAST_SPELL:
        break;
    case W8_ACTION_TURN_UNDEAD:
    case W8_ACTION_PRAY:
        cost = g_status_685170.buffers.characters[party_slot].stamina_max / 5;
        if (cost < 0x14) {
            cost = 0x14;
        }
        break;
    case W8_ACTION_USE_ITEM:
        cost = Random(2) + 1;
        break;
    case W8_ACTION_EQUIP:
        cost = Random(4) + 3;
        break;
    default:
        cost = 0;
    }

    if (g_status_685170.buffers.characters[party_slot]
            .condition_turns[W8_CONDITION_FATIGUE_DOUBLED] != 0) {
        cost *= 2;
    }
    return cost;
}

/* Drain spell points from one named realm, taking no more than it holds.
   Announced with the realm's own name. */
// FUNCTION: WIZ8 0x0052b6d0
void DrainCharacterRealmSpellPoints(int party_slot, int realm, unsigned int amount, char announce)
{
    W8Character* character = &g_status_685170.buffers.characters[party_slot];
    unsigned int available;

    if (character->hp_current == 0) {
        return;
    }
    available = character->sp_left[realm];
    if (available == 0) {
        return;
    }

    if (g_status_685170.value_2390 != 0) {
        PostCharacterNotice(party_slot, gppStringList[0x988 / 4], amount,
                            gppStringList[g_realm_message_offsets[realm]]);
        return;
    }

    if ((int)available < 0) {
        return;
    }
    if (amount >= available) {
        amount = available;
    }
    if (amount == 0) {
        return;
    }
    SpendCharacterSpellPoints(party_slot, realm, amount);
    if (announce) {
        WriteGameLog(8, gppStringList[0x98c / 4], amount,
                     gppStringList[g_realm_message_offsets[realm]]);
    }
}

/* One character dies. Every condition but the tenth is lifted, both pools are
   emptied, the two targeting blocks on their slot row are cleared, combat
   forgets them as a participant, and whatever they were animating is told to
   stop. */
// FUNCTION: WIZ8 0x0052abf0
void CharacterDies(int party_slot)
{
    W8Character* character = &g_status_685170.buffers.characters[party_slot];
    W8PartySlotRow* row = &g_status_685170.buffers.party_rows[party_slot];
    unsigned int condition;
    int animation;

    if (row->occupied == 0) {
        srAssertFail("fCHAR_OCCUPIED(uiChar)", HEALTH_STAMINA_MANA_CPP, 561, 0);
    }

    ++character->death_count_09fd;
    for (condition = 0; condition < W8_CONDITION_CLEARABLE_COUNT; ++condition) {
        if (condition != 10 && character->condition_turns[condition] != 0) {
            RemoveCharacterCondition(party_slot, condition, 0);
        }
    }
    character->hp_current = 0;
    character->stamina = 0;

    ResetCombatSlot(&row->target_out_of_combat);
    ResetCombatSlot(&row->target_in_combat);
    if (gXStatus.fCombatMode != 0) {
        RecordCharacterDeath(party_slot);
    }
    QueuePartyDeathReaction(party_slot);
    SoundPlay("Data\\Sound\\Misc\\CharacterDead.wav", 0);

    if (gXStatus.fCombatMode != 0) {
        if (g_combat_state->iActionChar == party_slot) {
            g_combat_state->eCombatActionStatus = 0;
            g_combat_state->iActionChar = -1;
        }
        row->pending_action = -1;
        g_combat_state->characters[party_slot].phase = 0;
        g_combat_state->characters[party_slot].flag_34 = 1;
        DropCharacterFromRound(party_slot);
    }

    animation = row->animation_0fa;
    if (animation != -1) {
        W8NpcState* npc = GetNpcState(animation);
        if (npc != 0) {
            npc->unknown_04 = 1;
        }
    }
}

/* Rebuilds the hit-point ceiling from scratch every time it is called: each
   profession the character has levels in contributes its own per-level factor,
   scaled by a figure derived from the fourth attribute record's effective
   value, and the profession the character started in counts one level more
   than it has taken. The running total lives in the x87 stack across the whole
   loop, which is why the zero it starts from is loaded before the profession
   guard and discarded by an `fstp` on the early return.
   The level is unsigned - the emitted test is `jbe`, not `jle` - and the
   attribute is widened through a zeroed high dword, which is the unsigned
   conversion rather than the signed one.
   Losing the last hit point applies condition 0x12 with the ceiling duration
   the party notice uses, which is the one place this writes anything beyond
   the two pools. */
// FUNCTION: WIZ8 0x0052A2F0
void RecalculateCharacterHitPoints(W8Character* character)
{
    double total = 0.0;
    int profession;
    int hit_points;
    int remaining;

    if (character->current_profession == W8_PROFESSION_NONE) {
        return;
    }

    for (profession = 0; profession < W8_PROFESSION_COUNT; profession++) {
        unsigned int levels = character->profession_levels[profession];
        if (profession == character->original_profession) {
            levels++;
        }
        if (levels > 0) {
            double vitality = character->attributes[3].effective * 0.4;
            total += (vitality * 0.02 + 0.6) * g_profession_hit_point_factors[profession] * levels;
        }
    }

    hit_points = (int)(total + 0.5) + character->hp_adjustment;
    if (hit_points < 1) {
        hit_points = 1;
    }
    if (hit_points != character->hp_max) {
        remaining = (hit_points - character->hp_max) + (int)character->hp_current;
        if (remaining < 0) {
            remaining = 0;
        }
        character->hp_max = hit_points;
        character->hp_current = remaining;
        if (remaining == 0) {
            SetCharacterCondition(CharacterPointerToPartySlot(character), W8_CONDITION_DEAD,
                                  W8_CONDITION_INDEFINITE, 0, 0, 1);
        }
    }
}

/* Recompute the stamina ceiling from the three physical attributes and the
   level, subtract any outstanding penalty, carry the difference into the
   current pool, and derive the fatigue band from what is left.

   Retail compares the penalty with JNC and divides the scaled pool with DIV,
   so the ceiling and its delta are unsigned values even though the stored
   fields are ints. The divisor is unguarded in retail; a zero ceiling would
   trap there too. */
// FUNCTION: WIZ8 0x0052a3e0
void RecalculateCharacterStamina(W8Character* character)
{
    unsigned int previous = character->stamina_max;
    unsigned int value =
        (unsigned int)(((character->attributes[0].effective + character->attributes[2].effective +
                         character->attributes[3].effective) *
                        (1.0f / 3.0f)) *
                           (character->level * g_float_005ed8b8 +
                            g_environment_near_scale_005ec0b0) +
                       g_double_005ebe80);
    character->stamina_max = value;
    if (character->fatigue_penalty_0b21 < value) {
        character->stamina_max = value - character->fatigue_penalty_0b21;
    } else {
        character->stamina_max = 0;
    }
    value = character->stamina_max;
    if (value != previous) {
        character->stamina += value - previous;
    }
    int fatigue = 100 - (int)((unsigned int)character->stamina * 100 / value);
    if (fatigue < 0x32) {
        character->fatigue_band = 0;
        return;
    }
    if (fatigue < 0x46) {
        character->fatigue_band = 1;
        return;
    }
    if (fatigue < 0x55) {
        character->fatigue_band = 2;
        return;
    }
    character->fatigue_band = (fatigue > 0x5e) + 3;
}

/* The resistance bonus skill (36) is derived only for the professions whose
   bodies can learn spells; a few fixed professions keep it at zero. */
// FUNCTION: WIZ8 0x0052a500
void RecalculateRealmSpellPoints(W8Character* character)
{
    int profession = character->current_profession;
    if (profession != 0 && (profession < 7 || profession > 9)) {
        character->skill_unlocks[0x24] = RebuildRealmSpellPointCeilings0052A540(character);
        return;
    }
    character->skill_unlocks[0x24] = 0;
}

/* Rebuild the six realm spell-point ceilings from the learned spells, the
   realm skill levels and the attributes. The weighted best four realm skills
   are capped at 125 and added to every realm's own skill and school
   attribute; the sum scales with the realm's learned-spell count, level and
   one. */
// FUNCTION: WIZ8 0x0052a540
int RebuildRealmSpellPointCeilings0052A540(W8Character* character)
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-compare"
    /* Retail compiled this comparison with VC6's mixed-sign operands; the
   signedness is part of the recovered body and changing it would change
   the compare and branch. Suppress only this diagnostic here. */
    int max_spell_levels[6];
    int realm_skills[4];
    int index;
    int best = 0;

    RecountLearnedSpellsByRealm004F96A0(character);
    for (index = 0; index < 6; ++index) {
        max_spell_levels[index] = 0;
    }
    for (index = 0; index < 0x72; ++index) {
        if (character->spell_learned[index] == 1 || character->spell_learned[index] == 2) {
            int realm = g_spell_records[index].realm;
            int cost = g_spell_records[index].spell_point_cost;
            if (max_spell_levels[realm] < cost) {
                max_spell_levels[realm] = cost;
            }
        }
    }
    for (index = 0; index < 4; ++index) {
        realm_skills[index] = character->skills[0x18 + index].level;
    }
    qsort(realm_skills, 4, 4, CompareUnsignedDescending);

    float weighted = (float)(realm_skills[0] + (realm_skills[1] >> 1) + (realm_skills[2] >> 2) +
                             (realm_skills[3] >> 3));
    if (weighted > 125.0f) {
        weighted = 125.0f;
    }

    for (index = 0; index < 6; ++index) {
        unsigned int old = character->sp_max[index];
        unsigned int learned = character->skill_unlocks[0x1c + index];
        int computed = (int)(((weighted + character->skills[0x1c + index].level * 3 +
                               character->attributes[2].effective) *
                              g_float_005ecbb4) *
                                 (learned + character->level + 1) +
                             g_double_005ebe80);
        if (best < computed) {
            best = computed;
        }
        if (learned == 0) {
            computed = 0;
        }
        character->sp_max[index] = computed;
        if (computed < max_spell_levels[index]) {
            character->sp_max[index] = max_spell_levels[index];
        }
        if (character->sp_max[index] != old) {
            character->sp_left[index] += character->sp_max[index] - old;
        }
    }
    return best;
#pragma clang diagnostic pop
}

// FUNCTION: WIZ8 0x0052a760
int SumCharacterSpellPoints(const W8Character* character)
{
    int total = 0;
    for (int realm = 0; realm < W8_SPELL_REALM_COUNT; ++realm) {
        total += character->sp_max[realm];
    }
    return total;
}

/* Occupied slots below the dead condition compete on hit-point percent; the
   weakest one wins. */
// FUNCTION: WIZ8 0x0052C350
unsigned int FindPartySlotWithLowestHitPoints(void)
{
    unsigned int best_slot = 0;
    unsigned int best_percent = 100;

    for (unsigned int slot = 0; slot < 8; ++slot) {
        W8Character* character = &g_status_685170.buffers.characters[slot];
        if (g_status_685170.buffers.party_rows[slot].occupied != 0 &&
            character->highest_condition < W8_CONDITION_DEAD) {
            unsigned int percent = (character->hp_current * 100) / (unsigned int)character->hp_max;
            if (percent < best_percent) {
                best_slot = slot;
                best_percent = percent;
            }
        }
    }
    return best_slot;
}

/* Same sweep over the spell-point pools: the numerator counts only realms
   with points still remaining, so the member with the lowest remaining
   percentage wins. */
// FUNCTION: WIZ8 0x0052C3B0
unsigned int FindPartySlotWithLowestSpellPoints(void)
{
    unsigned int best_slot = 0;
    unsigned int best_percent = 100;

    for (unsigned int slot = 0; slot < 8; ++slot) {
        W8Character* character = &g_status_685170.buffers.characters[slot];
        if (g_status_685170.buffers.party_rows[slot].occupied != 0 &&
            character->highest_condition < W8_CONDITION_DEAD) {
            unsigned int pool_max = 0;
            for (int realm = 0; realm < W8_SPELL_REALM_COUNT; ++realm) {
                pool_max += character->sp_max[realm];
            }
            if (pool_max > 0) {
                int pool_left = 0;
                for (int realm = 0; realm < W8_SPELL_REALM_COUNT; ++realm) {
                    int left = character->sp_left[realm];
                    pool_left += left & ((left <= 0) - 1);
                }
                unsigned int percent = (unsigned int)(pool_left * 100) / pool_max;
                if (percent < best_percent) {
                    best_percent = percent;
                    best_slot = slot;
                }
            }
        }
    }
    return best_slot;
}

/* Applies one queued fatigue op: kind 1 fatigues party slot op[1], kind 3
   fatigues the monster spawned from location id op[2]. */
// FUNCTION: WIZ8 0x0052C500
void ApplyQueuedFatigue(int* op, unsigned int amount)
{
    if (op[0] == 1) {
        FatigueCharacter(op[1], amount, 0, 0);
        return;
    }
    if (op[0] == 3) {
        unsigned int location_index =
            MonsterGetIndexByLocationID(0x771, HEALTH_STAMINA_MANA_CPP, op[2], 1);
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(location_index);
        FatigueMonster(monster_info, amount, 0);
    }
}

// FUNCTION: WIZ8 0x0052C480
W8Character* FindPartyMemberWithLowestResistance4(void)
{
    unsigned int lowest = 999;
    int selected = 0;
    for (int party_slot = 0; party_slot < 8; ++party_slot) {
        W8Character* character = &g_status_685170.buffers.characters[party_slot];
        if (g_status_685170.buffers.party_rows[party_slot].occupied != 0 &&
            character->highest_condition < W8_CONDITION_DEAD &&
            character->resistances[4].total < lowest) {
            selected = party_slot;
            lowest = character->resistances[4].total;
        }
    }
    if (lowest == 999)
        return 0;
    return &g_status_685170.buffers.characters[selected];
}

/* Re-blit each active portrait quote bubble onto the game surface. Runs when
   the screen comes back from a modal view; slots whose character is dead or
   too far gone to be speaking keep their bubble down. */
// FUNCTION: WIZ8 0x0052FE00
void RedrawPortraitQuoteBubbles(void)
{
    unsigned int party_slot;
    W8MonsterManagerEntry* slot;

    IsModalOpen();
    for (party_slot = 0; party_slot < 8; ++party_slot) {
        slot = &gXStatus.monster_manager_entries[party_slot];
        if (g_status_685170.buffers.party_rows[party_slot].occupied == 0 ||
            g_status_685170.buffers.characters[party_slot].hp_current <= 0 ||
            g_status_685170.buffers.characters[party_slot].highest_condition >= 0xf ||
            slot->portrait_event_active == 0) {
            continue;
        }
        DrawPortraitQuoteBubble(slot->quote.quote_handle, slot->quote.x, slot->quote.y, -0xe);
    }
}

// FUNCTION: WIZ8 0x0052fe80
void StartBreathCycle(int party_slot, char force)
{
    if ((gXStatus.fSpellCastMode == 0 && gXStatus.fItemSelectMode == 0) || force != 0) {
        QueueCharacterEvent(&g_status_685170.buffers.characters[party_slot],
                            g_special_event_0068c50c, 0, g_effect_argument_005ed8c8,
                            g_effect_argument_005ed914);
    }
}

/* Collect every occupied living slot other than `excluded_slot` whose
   character can actually speak `event_type`, then pick one at random. */
// FUNCTION: WIZ8 0x0052FEE0
int PickRandomPartySpeaker(unsigned int event_type, int excluded_slot)
{
    int eligible[8];
    unsigned int count = 0;

    for (int slot = 0; slot < 8; ++slot) {
        W8Character* character = &g_status_685170.buffers.characters[slot];
        if (g_status_685170.buffers.party_rows[slot].occupied != 0 && slot != excluded_slot &&
            character->hp_current != 0 && character->highest_condition < 0xf &&
            FormatCharacterQuoteText(character, event_type, 0) != 0) {
            eligible[count++] = slot;
        }
    }
    if (count != 0) {
        return eligible[Random(count)];
    }
    return -1;
}

/* Character-event subsystem: the event descriptor table, event object, and the
   five-vector dispatch queue. No assertion path survives in this span, but
   binary order places it inside this translation unit: 0x52C500 asserts this
   file's name immediately before it, and StartBreathCycle at 0x52FE80
   (assertion-anchored to this file) follows it; Strings.cpp begins at
   0x52FF80. */

// GLOBAL: WIZ8 0x0068C578
int g_special_event_0068c578;
// GLOBAL: WIZ8 0x0068c57c
unsigned int g_value_0068c57c;
/* 0x0068C580: the shared wide buffer formatted character text lands in. The
   message reader admits at most 0x7D0 code units, so the buffer holds exactly
   the two thousand characters that reach the next global at 0x0068D520. */
// GLOBAL: WIZ8 0x0068C580
wchar_t g_character_text_0068c580[2000];
/* 0x005ED91C: the quote file-name stem per personality, a twenty-byte fixed
   buffer each. The nine personas end exactly at the next global; the quote
   lookup composes Data\Quotes\PCs\<m|f>_<stem><1|2>0.MSG from them. */
// GLOBAL: WIZ8 0x005ed91c
const char g_quote_personality_names_005ed91c[9][0x14] = {
    "aggr", "intell", "burly", "chaos", "cun", "ecc", "kind", "laid", "loner",
};
// GLOBAL: WIZ8 0x0068c554
unsigned int g_value_0068c554;
struct W8PortraitTables {
    unsigned short quote_x[8];
    unsigned short quote_y[8];
    int pose_transition[30];
};
// GLOBAL: WIZ8 0x0061cb3c
W8PortraitTables g_portrait_tables_0061cb3c = {
    {0x0080, 0x0138, 0x0080, 0x0138, 0x0080, 0x0138, 0x0080, 0x0138},
    {0x0013, 0x0067, 0x00bc, 0x0111, 0x0013, 0x0067, 0x00bc, 0x0111},
    {1, 3, 3, 4, 5, 3, 2, 3, 3, 3, 1, 2, 3, 4, 1, 1, 3, 3, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
};
// GLOBAL: WIZ8 0x005ED8C8
int g_effect_argument_005ed8c8 = 0;
// GLOBAL: WIZ8 0x005ED8E0
unsigned int g_event_flag_005ed8e0 = 8;
// GLOBAL: WIZ8 0x005ED8E4
unsigned char g_character_event_flags_mask_005ed8e4 = 16;
// GLOBAL: WIZ8 0x005ED8EC
unsigned int g_event_flag_005ed8ec = 0x40;
// GLOBAL: WIZ8 0x005ED8F8
unsigned int g_flee_hp_fraction_005ed8f8 = 50;
// GLOBAL: WIZ8 0x005ED8FC
unsigned int g_value_005ed8fc = 20;
// GLOBAL: WIZ8 0x005ED8D0
int g_effect_argument_005ed8d0 = 2;
// GLOBAL: WIZ8 0x005ED8D4
int g_effect_argument_005ed8d4 = 1;
// GLOBAL: WIZ8 0x005ED914
int g_effect_argument_005ed914 = 127;
// GLOBAL: WIZ8 0x005EE590
int g_effect_005ee590 = 2;
// GLOBAL: WIZ8 0x005EE594
int g_effect_005ee594 = 3;
// GLOBAL: WIZ8 0x005ee598
int g_effect_005ee598 = 4;
// GLOBAL: WIZ8 0x005ee588
int g_effect_005ee588 = 0;
// GLOBAL: WIZ8 0x005EE58C
int g_effect_005ee58c = 1;
// GLOBAL: WIZ8 0x005EE5A4
int g_effect_005ee5a4 = 7;
// GLOBAL: WIZ8 0x005EE5AC
int g_effect_005ee5ac = 9;
// GLOBAL: WIZ8 0x005EE5B4
int g_effect_005ee5b4 = 11;
// GLOBAL: WIZ8 0x005EE5B8
int g_effect_005ee5b8 = 12;
// GLOBAL: WIZ8 0x005EE5DC
int g_effect_005ee5dc = 0x15;
// GLOBAL: WIZ8 0x005EE5E0
int g_effect_005ee5e0 = 0x16;
// GLOBAL: WIZ8 0x005EE5F8
int g_effect_005ee5f8 = 28;
// GLOBAL: WIZ8 0x005ee610
int g_effect_005ee610 = 34;
// GLOBAL: WIZ8 0x005EE628
int g_effect_005ee628 = 40;
// GLOBAL: WIZ8 0x005ee640
int g_item_message_005ee640 = 46;
// GLOBAL: WIZ8 0x005ee644
int g_item_message_005ee644 = 47;
// GLOBAL: WIZ8 0x005ee648
int g_item_message_005ee648 = 48;
// GLOBAL: WIZ8 0x005ee64c
int g_item_message_005ee64c = 49;
// GLOBAL: WIZ8 0x005EE654
int g_effect_005ee654 = 51;
// GLOBAL: WIZ8 0x005ee664
int g_item_message_005ee664 = 55;
// GLOBAL: WIZ8 0x005ee68c
int g_item_message_005ee68c = 65;
// GLOBAL: WIZ8 0x005ee690
int g_item_message_005ee690 = 66;
// GLOBAL: WIZ8 0x005EE6D8
int g_effect_005ee6d8 = 0x54;
// GLOBAL: WIZ8 0x005EE6DC
int g_effect_005ee6dc = 0x55;
// GLOBAL: WIZ8 0x005ee6f0
const int g_value_005ee6f0 = 129;
// GLOBAL: WIZ8 0x005ee6fc
int g_item_message_005ee6fc = 132;
// GLOBAL: WIZ8 0x005EE70C
unsigned int g_normal_event_count_005ee70c = 146; /* ordinary-event count */
// GLOBAL: WIZ8 0x005EE710
unsigned int g_remapped_event_count_005ee710 = 31;
// GLOBAL: WIZ8 0x005EE718
unsigned int g_first_remapped_event_005ee718 = 500;

/* 0x005EE000: eight-byte dispatch records indexed by remapped event type.
   The table ends at 0x005EE588 where g_effect_005ee588 begins. TryAdjustQueuedEvent
   reads coalesce_duplicates at +4; ProcessDeferredCharacterEvents reads
   defer_outside_main_game at +5. */
struct W8CharacterEventDescriptor {
    int portrait_pose_category;
    /* 0x04: a queued copy may be rewritten to the shared follow-up event (10)
       when another character already has the same event active. */
    unsigned char coalesce_duplicates;
    unsigned char defer_outside_main_game;
    /* 0x06: when the portrait quote closes, its text is posted to the notice
       log instead of being dropped silently. */
    unsigned char log_quote_on_finish;
    unsigned char unknown_07;
};
static_assert(sizeof(W8CharacterEventDescriptor) == 8, "W8CharacterEventDescriptor_must_be_8");
// GLOBAL: WIZ8 0x005EE000
W8CharacterEventDescriptor g_character_event_descriptors_005ee000[0xb1] = {
    {0x00000001, 0x00, 0x00, 0x00, 0x00}, {0x00000005, 0x00, 0x01, 0x00, 0x00},
    {0x00000004, 0x00, 0x01, 0x00, 0x00}, {0x00000002, 0x00, 0x01, 0x00, 0x00},
    {0x00000003, 0x01, 0x01, 0x00, 0x00}, {0x00000003, 0x01, 0x01, 0x00, 0x00},
    {0x00000003, 0x01, 0x01, 0x00, 0x00}, {0x00000003, 0x01, 0x01, 0x00, 0x00},
    {0x00000001, 0x01, 0x00, 0x00, 0x00}, {0x00000001, 0x01, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000005, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000004, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000004, 0x00, 0x01, 0x00, 0x00}, {0x00000004, 0x00, 0x01, 0x00, 0x00},
    {0x00000004, 0x00, 0x01, 0x00, 0x00}, {0x00000004, 0x00, 0x01, 0x00, 0x00},
    {0x00000004, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000004, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000005, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000005, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000005, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000005, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x00, 0x00, 0x00},
    {0x00000004, 0x00, 0x01, 0x00, 0x00}, {0x00000005, 0x00, 0x00, 0x00, 0x00},
    {0x00000005, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000005, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x00, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000004, 0x00, 0x01, 0x00, 0x00},
    {0x00000005, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00},
};

// GLOBAL: WIZ8 0x005EE5D0
int g_effect_005ee5d0;
// GLOBAL: WIZ8 0x005EE5D4
int g_effect_005ee5d4;
// GLOBAL: WIZ8 0x005EE5D8
int g_effect_005ee5d8;
// GLOBAL: WIZ8 0x0068C504
int g_special_event_0068c504;
// GLOBAL: WIZ8 0x0068C508
int g_special_event_0068c508;
// GLOBAL: WIZ8 0x0068C50C
int g_special_event_0068c50c;
// GLOBAL: WIZ8 0x0068C514
int g_special_event_0068c514;
// GLOBAL: WIZ8 0x0068C51C
int g_special_event_0068c51c;
// GLOBAL: WIZ8 0x0068C52C
int g_special_event_0068c52c;
// GLOBAL: WIZ8 0x0068C538
int g_special_event_0068c538;
// GLOBAL: WIZ8 0x0068C544
int g_special_event_0068c544;
// GLOBAL: WIZ8 0x0068C540
int g_special_event_0068c540;
// GLOBAL: WIZ8 0x0068C550
int g_special_event_0068c550;
// GLOBAL: WIZ8 0x0068C558
int g_special_event_0068c558;
// GLOBAL: WIZ8 0x0068C55C
int g_special_event_0068c55c;
// GLOBAL: WIZ8 0x0068C564
int g_special_event_0068c564;
// GLOBAL: WIZ8 0x0068C568
int g_special_event_0068c568;

static bool MapEventTypeToDescriptorIndex(unsigned int event_type, unsigned int* descriptor_index)
{
    if (event_type < g_normal_event_count_005ee70c) {
        *descriptor_index = event_type;
        return true;
    }
    if (event_type >= g_first_remapped_event_005ee718 &&
        event_type < g_first_remapped_event_005ee718 + g_remapped_event_count_005ee710) {
        *descriptor_index =
            g_normal_event_count_005ee70c + event_type - g_first_remapped_event_005ee718;
        return true;
    }
    return false;
}

/* Character-event queue and portrait/voice updates. The original
   translation-unit name is unknown; the existing unit is retained intact. */

// FUNCTION: WIZ8 0x0052E360
bool IsVoiceMuted(void)
{
    return g_settings_6850c8.muted_voice_volume != 0xff;
}

// FUNCTION: WIZ8 0x0052E370
void SetVoiceMuted(unsigned char muted)
{
    if (muted != 0) {
        if (g_settings_6850c8.muted_voice_volume == 0xff) {
            g_settings_6850c8.muted_voice_volume = g_settings_6850c8.voice_volume;
            g_settings_6850c8.voice_volume = 0;
        }
    } else if (g_settings_6850c8.muted_voice_volume != 0xff) {
        g_settings_6850c8.voice_volume = g_settings_6850c8.muted_voice_volume;
        g_settings_6850c8.muted_voice_volume = 0xff;
    }
}

static void CharacterEventSoundEndCallback(void* callback_data)
{
    W8CharacterEvent* entry = static_cast<W8CharacterEvent*>(callback_data);
    W8CharacterEventQueue* queue = gXStatus.character_event_queue;

    if (entry->sound_end_handled != 0 || queue == 0) {
        return;
    }
    int index = queue->active_events.IndexOf(entry);
    if (index >= 0) {
        queue->active_events.RemoveAt(index);
    }
    if ((queue->follow_up_flags & 1) != 0 && entry->event_type >= 14 && entry->event_type < 16) {
        queue->follow_up_clock = SetCountdownClock(
            (queue->follow_up_flags & 2) != 0 ? Random(6000) + 2000 : Random(60000) + 300000);
    }
    entry->Complete();
    delete entry;
}

// FUNCTION: WIZ8 0x0052C810
W8CharacterEvent::W8CharacterEvent(W8Character* character, unsigned int event_type, int value_0c,
                                   unsigned int flags, int volume)
    : sound_end_handled(0), character(character), event_type(event_type), value_0c(value_0c),
      flags(flags), volume(volume), dispatch_delay_ms(0)
{
    item.item_id = -1;
    switch (event_type) {
    case 2:
    case 3:
        value_18 = character->hp_current;
        break;
    case 5:
    case 6:
    case 9:
    case 0x54:
        value_18 = character->highest_condition;
        break;
    case 7:
        value_18 = 12;
        break;
    case 0x38:
    case 0x55:
        value_18 = character->hp_current;
        value_1c = character->highest_condition;
        break;
    }
}

/* 0x0052D0B0: format one character quote for the given event type into the
   shared wide text buffer. A party member on any screen but the character
   screen takes the text from the NPC bound to its slot; otherwise the type
   selects an entry of the sex/personality/voice quote file, which is then
   wrapped in quotes. Clears the buffer and answers zero when no quote exists. */
// FUNCTION: WIZ8 0x0052D0B0
unsigned char FormatCharacterQuoteText(W8Character* character, unsigned int event_type,
                                       unsigned int* metadata)
{
    char path[80];
    wchar_t text[500];
    int npc_index;
    unsigned char has_npc;

    if (event_type >= 0x92) {
        return 0;
    }
    if (metadata != 0) {
        *metadata = 0xffffffff;
    }
    npc_index = -1;
    has_npc = 0;
    if (character->in_party != 0 && g_current_screen_state.id != W8_SCREEN_CHARACTER) {
        unsigned int slot = CharacterPointerToPartySlot(character);
        npc_index = g_status_685170.buffers.party_rows[slot].animation_0fa;
        has_npc = npc_index != -1;
    }
    if (!has_npc) {
        char gender_code = static_cast<char>(((character->gender != 0) - 1U & 7) + 0x66);
        sprintf(path, "Data\\Quotes\\PCs\\%c_%s%d0.MSG", gender_code,
                g_quote_personality_names_005ed91c[character->personality_0081],
                (character->voice_0085 != 0) + 1);
        if (!FileExists(path)) {
            g_character_text_0068c580[0] = 0;
            return 0;
        }
        GetStringFromStringDatabase(path, event_type, g_character_text_0068c580, 0, metadata);
        g_character_text_0068c580[wcslen(g_character_text_0068c580) - 1] = 0;
    } else {
        W8NpcState* npc = GetNpcState(npc_index);
        if (GetNpcQuoteText(npc, event_type, g_character_text_0068c580) == 0) {
            g_character_text_0068c580[0] = 0;
            return 0;
        }
    }

    if (wcslen(g_character_text_0068c580) == 0) {
        return 0;
    }
    swprintf(text, L"\"%s\"", g_character_text_0068c580);
    wcscpy(g_character_text_0068c580, text);
    return 1;
}

/* The quote text builder fills the shared wide buffer; the final character
   page's description area displays it. */
// FUNCTION: WIZ8 0x0052D240
wchar_t* W8CharacterEvent::GetQuoteText()
{
    FormatCharacterQuoteText(character, event_type, 0);
    return g_character_text_0068c580;
}

/* 0x0052D460 proves four equal derived growable-vector instantiations followed
   by a fifth instantiation with a distinct vtable and the tail state below. */

// FUNCTION: WIZ8 0x0052d460
W8CharacterEventQueue::W8CharacterEventQueue()
    : active_event_type(-1), active_party_slot(-1), follow_up_flags(0), follow_up_speaker_slot(-1)
{
    event_character_masks = new unsigned char[0xb1];
    memset(event_character_masks, 0, 0xb1);
}

// FUNCTION: WIZ8 0x0052d5b0
W8CharacterEventQueue::~W8CharacterEventQueue()
{
    delete[] event_character_masks;
}

// FUNCTION: WIZ8 0x0052db80
void W8CharacterEventQueue::DestroyAllEvents()
{
    while (active_events.count > 0) {
        active_events.RemoveAt(0)->Complete();
    }
    while (npc_deferred_events.count > 0) {
        delete npc_deferred_events.RemoveAt(0);
    }
    while (pending_events.count > 0) {
        delete pending_events.RemoveAt(0);
    }
    while (vector_00.count > 0) {
        delete vector_00.RemoveAt(0);
    }
}

// FUNCTION: WIZ8 0x0052D970
void W8CharacterEventQueue::RemoveCharacterEvents(W8Character* character)
{
    int index;
    W8CharacterEvent* entry;

    for (index = 0; index < active_events.count; ++index) {
        entry = *active_events.GetAt(index);
        if (entry->character == character) {
            active_events.RemoveAt(index);
            --index;
            entry->Complete();
        }
    }
    for (index = 0; index < vector_20.count; ++index) {
        entry = *vector_20.GetAt(index);
        if (entry->character == character) {
            vector_20.RemoveAt(index);
            --index;
            delete entry;
        }
    }
    for (index = 0; index < pending_events.count; ++index) {
        entry = *pending_events.GetAt(index);
        if (entry->character == character) {
            pending_events.RemoveAt(index);
            --index;
            delete entry;
        }
    }
    for (index = 0; index < vector_00.count; ++index) {
        entry = *vector_00.GetAt(index);
        if (entry->character == character) {
            vector_00.RemoveAt(index);
            --index;
            delete entry;
        }
    }
    for (index = 0; index < npc_deferred_events.count; ++index) {
        entry = *npc_deferred_events.GetAt(index);
        if (entry->character == character) {
            npc_deferred_events.RemoveAt(index);
            --index;
            delete entry;
        }
    }
}

// FUNCTION: WIZ8 0x0052DB30
void W8CharacterEventQueue::CompleteAllActiveEvents()
{
    while (active_events.count > 0) {
        active_events.RemoveAt(0)->Complete();
    }
}

// FUNCTION: WIZ8 0x0052e3b0
void W8CharacterEventQueue::CompleteFirstActiveEvent()
{
    W8CharacterEvent* entry;

    if (active_events.count > 0) {
        entry = *active_events.GetAt(0);
        active_events.RemoveAt(active_events.IndexOf(entry));
        if ((follow_up_flags & 1) != 0 && entry->event_type >= 14 && entry->event_type < 16) {
            if ((follow_up_flags & 2) != 0) {
                follow_up_clock = SetCountdownClock(Random(6000) + 2000);
            } else {
                follow_up_clock = SetCountdownClock(Random(60000) + 300000);
            }
        }
        entry->Complete();
        delete entry;
    }
}

// FUNCTION: WIZ8 0x0052ced0
void W8CharacterEvent::Complete()
{
    W8MonsterManagerEntry* slot;
    int party_slot;
    unsigned char sound_was_active;

    party_slot = CharacterPointerToPartySlot(character);
    slot = &gXStatus.monster_manager_entries[party_slot];
    sound_was_active = slot->portrait_event_active;
    slot->active_character_event = 0;
    if (sound_was_active != 0) {
        if (IsSoundPlaying(slot->voice_sound_handle) != 0) {
            sound_end_handled = 1;
            StopSound(slot->voice_sound_handle);
        }
        SetPartyPortraitEventState(party_slot, 0, -1, 0, 1);
    }
    if (event_type == 23 || event_type == 24) {
        if ((flags & W8_EVENT_NPC_SCRIPT) == 0) {
            if (item.item_id == -1) {
                PostCharacterMessage(party_slot, gppStringList[0x1dc4 / 4]);
            } else {
                PostCharacterMessage(party_slot, gppStringList[0x1dc8 / 4],
                                     GetItemDisplayName(&item));
            }
        }
    } else if (event_type == 51) {
        QueueGameplayEvent(30, party_slot);
    }
}

// FUNCTION: WIZ8 0x0052C910
unsigned char W8CharacterEvent::IsConditionMet(unsigned int event_type)
{
    do {
        switch (event_type) {
        case 2:
        case 3:
            return static_cast<unsigned int>(value_18) > character->hp_current;
        case 5:
        case 6:
        case 7:
        case 9:
            return character->highest_condition == static_cast<unsigned int>(value_18);
        case 10:
            event_type = original_event_type;
            break;
        case 14:
        case 15:
        case 35:
            return gXStatus.fCombatMode == 0;
        case 43:
        case 44:
        case 45:
            return character->gender != W8_GENDER_FEMALE;
        case 56:
            return character->hp_current >= static_cast<unsigned int>(value_18) &&
                   character->highest_condition >= static_cast<unsigned int>(value_1c);
        case 84:
            return static_cast<unsigned int>(value_18) > character->highest_condition;
        case 85:
            if (character->hp_current < static_cast<unsigned int>(value_18) ||
                character->highest_condition != 0) {
                QueueCharacterEvent(character, 84, 0, 1, 0x7f);
                return 0;
            }
            return 1;
        default:
            return 1;
        }
    } while (1);
}

// FUNCTION: WIZ8 0x0052CFB0
static unsigned char CanDispatchCharacterEvent(unsigned int party_slot, unsigned int event_type,
                                               unsigned int flags)
{
    W8Character* character;
    unsigned int mapped_event_type;
    unsigned char slot_mask;

    (void)flags;
    if (party_slot >= 8) {
        return 0;
    }
    if (event_type >= g_normal_event_count_005ee70c) {
        if (event_type < g_first_remapped_event_005ee718 ||
            event_type >= g_remapped_event_count_005ee710 + g_first_remapped_event_005ee718) {
            return 0;
        }
    }
    if (g_status_685170.buffers.party_rows[party_slot].occupied == 0) {
        return 0;
    }
    character = &g_status_685170.buffers.characters[party_slot];
    if (character->highest_condition > 14) {
        if (event_type == static_cast<unsigned int>(g_special_event_0068c538) ||
            event_type == static_cast<unsigned int>(g_special_event_0068c540) ||
            g_special_event_0068c564 != 0) {
            if (character->condition_turns[17] != 0 || character->condition_turns[19] != 0) {
                return 0;
            }
        } else {
            if (character->highest_condition != 0x11 && character->highest_condition != 0xf) {
                return 0;
            }
            if (event_type != static_cast<unsigned int>(g_special_event_0068c544) &&
                event_type != static_cast<unsigned int>(g_special_event_0068c550) &&
                event_type != static_cast<unsigned int>(g_special_event_0068c51c)) {
                return 0;
            }
        }
    }
    if (character->hp_current == 0 &&
        event_type != static_cast<unsigned int>(g_special_event_0068c538)) {
        return 0;
    }
    mapped_event_type = event_type;
    if (mapped_event_type >= g_first_remapped_event_005ee718) {
        mapped_event_type += g_normal_event_count_005ee70c - g_first_remapped_event_005ee718;
    }
    slot_mask = static_cast<unsigned char>(1 << (party_slot & 31));
    return (gXStatus.character_event_queue->event_character_masks[mapped_event_type] & slot_mask) ==
           0;
}

// FUNCTION: WIZ8 0x0052D260
unsigned char W8CharacterEvent::PlayEventSound()
{
    unsigned int party_slot = CharacterPointerToPartySlot(character);
    unsigned int sound_event = event_type;
    int npc_index = g_status_685170.buffers.party_rows[party_slot].animation_0fa;
    char voice_stem[20];
    char sound_path[80];
    char npc_sound_name[128];
    SOUNDPARMS sound_parms;
    unsigned int sound_handle;
    unsigned int total_ms;
    unsigned int current_ms;
    W8MonsterManagerEntry* record;
    W8NpcState* npc;

    if (character->condition_turns[8] != 0) {
        sound_event = g_special_event_0068c508;
    }
    if (npc_index == -1 || g_status_685170.game_started == 0 ||
        g_current_screen_state.id == W8_SCREEN_CHARACTER) {
        char gender_code = static_cast<char>(((character->gender != 0) - 1U & 7) + 0x66);
        sprintf(voice_stem, "%c_%s%d0", gender_code,
                g_quote_personality_names_005ed91c[character->personality_0081],
                (character->voice_0085 != 0) + 1);
        sprintf(sound_path, "Data\\Sound\\PCs\\%s\\%s_%03d.wav", voice_stem, voice_stem,
                sound_event);
    } else {
        npc = GetNpcState(npc_index);
        if (npc != 0) {
            FormatNpcVoiceSoundPath(npc, npc_sound_name);
            sprintf(sound_path, "Data\\Sound\\PCs\\%s\\%s_%03d.wav", npc_sound_name, npc_sound_name,
                    sound_event);
        }
    }
    memset(&sound_parms, 0xff, sizeof(sound_parms));
    sound_parms.uiVolume = (volume * (g_settings_6850c8.voice_volume & 0xff)) / 0x7f;
    sound_parms.EOSCallback = CharacterEventSoundEndCallback;
    sound_parms.pCallbackData = this;
    sound_handle = SoundPlay(sound_path, &sound_parms);
    record = &gXStatus.monster_manager_entries[party_slot];
    record->voice_sound_handle = sound_handle;
    if (sound_handle == 0xffffffff) {
        if (event_type > 0x91) {
            static const wchar_t kFallbackVoiceText[] = L"Ouch play this sound";
            record->voice_time_remaining_ms =
                ComputePortraitMessageDuration(const_cast<wchar_t*>(kFallbackVoiceText));
        } else {
            record->voice_time_remaining_ms =
                ComputePortraitMessageDuration(g_character_text_0068c580);
        }
        return 1;
    }
    SoundGetMilliSecondPosition(sound_handle, &total_ms, &current_ms);
    record->voice_time_remaining_ms = total_ms;
    Function5E2D10(sound_path, &record->mouth_gap);
    return 1;
}

// FUNCTION: WIZ8 0x0052CA60
unsigned char W8CharacterEvent::Dispatch()
{
    unsigned int party_slot;
    unsigned int metadata;
    unsigned int descriptor_index;
    W8MonsterManagerEntry* slot;
    W8PartySlotRow* row;
    int npc_index;
    W8NpcState* npc;
    unsigned char has_quote;

    metadata = 0xffffffff;
    if (character == 0) {
        return 0;
    }
    party_slot = CharacterPointerToPartySlot(character);
    if (!MapEventTypeToDescriptorIndex(event_type, &descriptor_index)) {
        return 0;
    }
    metadata = 0xffffffff;
    if ((flags & W8_EVENT_BYPASS_CHECKS) == 0) {
        if (CanDispatchCharacterEvent(party_slot, event_type, flags) == 0 ||
            IsConditionMet(event_type) == 0) {
            goto finish_without_dispatch;
        }
        if (event_type != (unsigned int)g_special_event_0068c578 &&
            event_type != (unsigned int)g_special_event_0068c508) {
            if (character->condition_turns[W8_CONDITION_SPELLCASTING_BLOCKED] != 0) {
                QueueCharacterEvent(character, g_special_event_0068c508, 0, 1, 0x7f);
                return 0;
            }
            if (character->condition_turns[11] != 0) {
                QueueCharacterEvent(character, g_special_event_0068c578, 0, 1, 0x7f);
                return 0;
            }
        }
    }
    slot = &gXStatus.monster_manager_entries[party_slot];
    if (slot->portrait_event_active == 0) {
        if (event_type == 0x33) {
            SetNpcDialoguePanelVisible(0);
        }
        row = &g_status_685170.buffers.party_rows[party_slot];
        npc_index = row->animation_0fa;
        if (npc_index != -1 && event_type < 0x93) {
            npc = GetNpcState(npc_index);
            row->pending_event_type_ff = event_type;
            if (npc == 0) {
                return 1;
            }
            if (npc->name_style == 0x18 && event_type > 0x8b && event_type < 0x92 &&
                g_status_685170.current_level != 0) {
                return 0;
            }
            if ((flags & W8_EVENT_NPC_SCRIPT) != 0) {
                SetFlag68C500(1);
                ReleaseNpcScriptFile0055A0A0(npc->script_file);
                ReloadNpcScriptResources(npc);
            }
            BeginNpcScriptDialogue(npc, 1);
            RunNpcScriptLine(event_type, (flags & W8_EVENT_NPC_SCRIPT) != 0);
            if ((flags & W8_EVENT_NPC_SCRIPT) != 0) {
                SetFlag68C500(0);
                ReleaseNpcScriptFile0055A0A0(npc->script_file);
                ReloadNpcScriptResources(npc);
            }
            slot->active_character_event = this;
            if (event_type > 1 && (event_type < 4 || event_type == 0x1c)) {
                gXStatus.character_event_queue->SetEventCharacterMask(event_type, party_slot, 1);
            }
            if (event_type != 10) {
                gXStatus.character_event_queue->active_event_type = event_type;
                gXStatus.character_event_queue->active_party_slot = party_slot;
            }
            gXStatus.character_event_queue->recent_event_clock = SetCountdownClock(5000);
            return 1;
        }
        has_quote = FormatCharacterQuoteText(character, event_type, &metadata);
        if (PlayEventSound() != 0) {
            if (event_type > 1 && (event_type < 4 || event_type == 0x1c)) {
                gXStatus.character_event_queue->SetEventCharacterMask(event_type, party_slot, 1);
            }
            if (event_type != 10) {
                gXStatus.character_event_queue->active_event_type = event_type;
                gXStatus.character_event_queue->active_party_slot = party_slot;
            }
            gXStatus.character_event_queue->recent_event_clock = SetCountdownClock(5000);
            if (event_type < 0x92) {
                SetPartyPortraitEventState(
                    party_slot, 1, event_type, g_character_text_0068c580,
                    1 - ((flags & g_character_event_flags_mask_005ed8e4) != 0));
                slot->active_character_event = this;
                row->pending_event_type_ff = event_type;
                slot->pending_event_type_114 = event_type;
                return 1;
            }
            SetPartyPortraitEventState(party_slot, 1, event_type, 0, 1);
            slot->pending_event_type_114 = event_type;
            return 1;
        }
        Complete();
        if (has_quote == 0) {
            return 0;
        }
        if (event_type > 1 && (event_type < 4 || event_type == 0x1c)) {
            gXStatus.character_event_queue->SetEventCharacterMask(event_type, party_slot, 1);
        }
        if (event_type != 10) {
            gXStatus.character_event_queue->active_event_type = event_type;
            gXStatus.character_event_queue->active_party_slot = party_slot;
        }
        gXStatus.character_event_queue->recent_event_clock = SetCountdownClock(5000);
        if ((gXStatus.character_event_queue->follow_up_flags & 1) == 0) {
            return 0;
        }
        if (event_type < 14) {
            return 0;
        }
        if (event_type >= 16) {
            return 0;
        }
        if ((gXStatus.character_event_queue->follow_up_flags & 2) != 0) {
            gXStatus.character_event_queue->follow_up_clock =
                SetCountdownClock(Random(6000) + 2000);
            return 0;
        }
        gXStatus.character_event_queue->follow_up_clock = SetCountdownClock(Random(60000) + 300000);
        return 0;
    }
finish_without_dispatch:
    Complete();
    return 0;
}

/* 0x0052F890: turn a party-slot portrait/voice event on or off, optionally
   laying out the quote bubble and posting subtitle notices when one ends. */
// FUNCTION: WIZ8 0x0052F890
void SetPartyPortraitEventState(unsigned int party_slot, unsigned char active,
                                unsigned int event_type, const wchar_t* quote_text, int show_quote)
{
    // clang-format off
    W8MonsterManagerEntry* record = &gXStatus.monster_manager_entries[party_slot];
    W8PortraitQuoteState* quote = &record->quote;

    if (record->portrait_event_active == active) {
        return;
    }
    if (active == 0) {
        record->portrait_event_active = 0;
        if (gfCapturingVideo != 0) {
            return;
        }
        record->previous_portrait_frame = record->portrait_frame;
        record->portrait_frame = 6;
        record->portrait_frame_dirty = 1;
        int pc_slot = RPCPtrToPCSlot(record);
        record->portrait_pose_animation_active = 0;
        unsigned int highest_condition = g_status_685170.buffers.characters[pc_slot].highest_condition;
        if (highest_condition < 0xf && gXStatus.fSurprisePossible == 0) {
            if (record->target_portrait_pose != 1) {
                record->target_portrait_pose = 1;
            }
        } else {
            record->target_portrait_pose = 2;
        }
        if (quote->quote_handle == -1) {
            record->portrait_event_active = active;
            return;
        }
        unsigned int stored_event = record->pending_event_type_114;
        unsigned int mapped_event = stored_event;
        if (static_cast<int>(g_normal_event_count_005ee70c) < static_cast<int>(mapped_event)) {
        show_deactivate_quote:
            wchar_t formatted[100];
            const wchar_t* character_name = g_status_685170.buffers.characters[party_slot].name;
            swprintf(formatted, L"%s", character_name);
            int scroll_range = GetTextBoxScrollRange();
            ShowNotice(1, formatted, 3, scroll_range, 0);
            const wchar_t* suffix =
                GetPortraitQuoteText(quote->quote_handle);
            ShowNotice(0xf, suffix);
        } else {
            if (g_first_remapped_event_005ee718 <= mapped_event) {
                mapped_event =
                    g_normal_event_count_005ee70c - g_first_remapped_event_005ee718 + mapped_event;
            }
            if (g_character_event_descriptors_005ee000[mapped_event].log_quote_on_finish != 0) {
                goto show_deactivate_quote;
            }
        }
        ReleasePortraitQuoteBubble(quote->quote_handle);
        if (g_current_screen_state.id == W8_SCREEN_CAMP ||
            (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block->flag_327 == 0)) {
            int left = quote->x;
            int top = quote->y;
            ClearSurfaceRect(left, top, quote->width + left, quote->height + top);
            InvalidateRegion(left, top, quote->width + left, quote->height + top, 0);
        }
        RegionSetDisable(party_slot + 0x1d);
        DisableRegionSetInput(party_slot + 0x1d);
        if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME) {
            RequestRedraw(0x8000);
            RequestRedrawParty();
            record->portrait_event_active = 0;
            return;
        }
        if (g_current_screen_state.id == W8_SCREEN_CAMP) {
            g_camp_screen_0069c0f4->redraw_flags |= 0x0fffffff;
        }
        record->portrait_event_active = active;
        return;
    }

    record->portrait_event_active = 1;
    if (gfCapturingVideo != 0) {
        return;
    }
    record->previous_portrait_frame = record->portrait_frame;
    record->portrait_frame = 7;
    record->portrait_frame_dirty = 1;
    record->portrait_frame_clock = SetCountdownClock(0x78);
    int pose_category;
    if (static_cast<int>(g_normal_event_count_005ee70c) < static_cast<int>(event_type)) {
        pose_category = 1;
    } else {
        unsigned int mapped_event = event_type;
        if (g_first_remapped_event_005ee718 <= event_type) {
            mapped_event =
                g_normal_event_count_005ee70c - g_first_remapped_event_005ee718 + event_type;
        }
        pose_category =
            g_character_event_descriptors_005ee000[mapped_event].portrait_pose_category;
    }
    int pc_slot = RPCPtrToPCSlot(record);
    record->portrait_pose_animation_active = 0;
    unsigned int highest_condition = g_status_685170.buffers.characters[pc_slot].highest_condition;
    if (highest_condition < 0xf && gXStatus.fSurprisePossible == 0) {
        if (record->target_portrait_pose != pose_category) {
            record->target_portrait_pose = pose_category;
        }
    } else {
        record->target_portrait_pose = 2;
    }
    if (quote_text == 0 || show_quote == 0) {
        quote->quote_handle = -1;
    } else {
        unsigned char layout_quote = 0;
        unsigned char use_modal_gate = 0;
        if (static_cast<int>(g_normal_event_count_005ee70c) < static_cast<int>(event_type)) {
            use_modal_gate = 1;
        } else {
            unsigned int mapped_event = event_type;
            if (g_first_remapped_event_005ee718 <= event_type) {
                mapped_event =
                    g_normal_event_count_005ee70c - g_first_remapped_event_005ee718 + event_type;
            }
            if (g_character_event_descriptors_005ee000[mapped_event].defer_outside_main_game != 0) {
                use_modal_gate = 1;
            } else if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME) {
                use_modal_gate = 1;
            } else {
                layout_quote = 1;
            }
        }
        if (use_modal_gate != 0 && g_current_screen_state.id == W8_SCREEN_MAIN_GAME &&
            IsModalOpen() == 0) {
            layout_quote = 1;
        }
        if (layout_quote == 0 || g_settings_6850c8.pc_subtitles == 0) {
            quote->quote_handle = -1;
        } else {
            unsigned short width;
            unsigned short height;
            quote->quote_handle = LayoutPortraitQuoteBubble(-1, 0, 0, quote_text, 200, 0, 0, 0,
                                                             &width, &height, 0xffffffff);
            quote->width = width;
            quote->height = height;
            if (g_current_screen_state.id == W8_SCREEN_CAMP) {
                if (static_cast<int>(party_slot) == giReviewCharSlot) {
                    quote->x = 10;
                    quote->y = 8;
                } else {
                    quote->x = static_cast<unsigned short>(((party_slot & 1) * 0x30) + 0x36);
                    quote->y = static_cast<unsigned short>((party_slot >> 1) * 0x27 + 5);
                }
                g_camp_screen_0069c0f4->redraw_flags |= 0x0fffffff;
            } else {
                unsigned short base_x = g_portrait_tables_0061cb3c.quote_x[party_slot];
                quote->x = base_x;
                quote->y = g_portrait_tables_0061cb3c.quote_y[party_slot];
                if ((party_slot & 1) == 1) {
                    quote->x = static_cast<unsigned short>(base_x - quote->width + 200);
                }
                if (quote->y + quote->height > 0x165) {
                    quote->y = static_cast<unsigned short>(0x165 - quote->height);
                }
            }
            unsigned short x = quote->x;
            unsigned short y = quote->y;
            SetRegionBounds(party_slot + 0x12e, x, y, x + quote->width, quote->height + y);
            RegionSetEnable(party_slot + 0x1d);
            EnableRegionSetInput(party_slot + 0x1d);
        }
    }
    if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_settings_6850c8.field_006 != 0 &&
        g_level_block->portrait_refresh_pending[party_slot] == 0 &&
        event_type != static_cast<unsigned int>(g_special_event_0068c568)) {
        RefreshSelectedPartyPortrait(party_slot);
        record->field_0cf = 1;
        record->portrait_event_active = active;
        return;
    }
    record->portrait_event_active = active;
    // clang-format on
}

/* Restarts the follow-up clock for entries of the middle event band while the
   state flag selects it. */
// FUNCTION: WIZ8 0x0052E160
void W8CharacterEventQueue::RestartFollowUpClock(W8CharacterEvent* entry)
{
    int flags = follow_up_flags;
    unsigned int type = entry->event_type;
    int duration;

    if ((flags & 1) == 0 || type < 14 || type >= 16) {
        return;
    }
    if ((flags & 2) == 0) {
        duration = Random(60000) + 300000;
    } else {
        duration = Random(6000) + 2000;
    }
    follow_up_clock = SetCountdownClock(duration);
}

/* Advances the ambient follow-up exchange once the environment allows it.
   Bit 0 of follow_up_flags marks the sequence armed; bit 1 marks that a
   speaker was already picked and a response event is pending. Each queued
   middle-band event re-arms follow_up_clock when it dispatches; the -1 here
   disarms the clock until that happens. */
// FUNCTION: WIZ8 0x0052E1C0
void W8CharacterEventQueue::ProcessFollowUpEvents()
{
    if (GetFlag68F105() != 0 || GetEnvironmentFlag0060A394() == 0) {
        return;
    }
    if ((follow_up_flags & 1) == 0) {
        follow_up_flags |= 1;
        follow_up_speaker_slot = GetRandomCharacter(0, 0, -1, -1);
        if (follow_up_speaker_slot != -1) {
            follow_up_flags |= 2;
            QueueCharacterEvent(&g_status_685170.buffers.characters[follow_up_speaker_slot],
                                Random(2) + 0xe, 1, 0, 0x7f);
            follow_up_clock = -1;
        }
        return;
    }
    if ((follow_up_flags & 2) == 0) {
        if (ClockIsTicking(follow_up_clock) != 0) {
            return;
        }
        follow_up_flags |= 1;
        follow_up_speaker_slot = GetRandomCharacter(0, 0, -1, -1);
        if (follow_up_speaker_slot != -1) {
            follow_up_flags |= 2;
            QueueCharacterEvent(&g_status_685170.buffers.characters[follow_up_speaker_slot],
                                Random(2) + 0xe, 1, 0, 0x7f);
            follow_up_clock = -1;
        }
        return;
    }
    if (ClockIsTicking(follow_up_clock) != 0) {
        return;
    }
    if (follow_up_speaker_slot == -1) {
        follow_up_flags &= ~2;
        return;
    }
    int next_slot = GetRandomCharacter(0, 0, follow_up_speaker_slot, -1);
    if (next_slot != -1) {
        QueueCharacterEvent(&g_status_685170.buffers.characters[next_slot], Random(2) + 0xe, 1, 0,
                            0x7f);
        follow_up_clock = -1;
        follow_up_flags &= ~2;
    }
}

// FUNCTION: WIZ8 0x0052D610
int W8CharacterEventQueue::QueueEntry(W8CharacterEvent* entry)
{
    unsigned int party_slot;

    party_slot = CharacterPointerToPartySlot(entry->character);
    if (HasEventCharacter(entry->event_type, party_slot)) {
        delete entry;
        return 0;
    }
    if (g_status_685170.flag_2497 != 0) {
        delete entry;
        return 0;
    }
    if (entry->event_type > 0x91 && (entry->flags & W8_EVENT_NO_PREEMPT) == 0) {
        W8MonsterManagerEntry* slot = &gXStatus.monster_manager_entries[party_slot];
        if (slot->active_character_event != 0) {
            active_events.Remove(slot->active_character_event);
            RestartFollowUpClock(slot->active_character_event);
            slot->active_character_event->Complete();
            delete slot->active_character_event;
        }
        entry->Dispatch();
        return 1;
    }
    if (entry->event_type == 0x21) {
        int index;
        if (active_events.count > 0 && active_events.data[0]->event_type == 0x21) {
            delete entry;
            return 0;
        }
        for (index = 0; index < pending_events.count; ++index) {
            if (pending_events.data[index]->event_type == 0x21) {
                delete entry;
                return 0;
            }
        }
    }
    if ((ShouldDeferCharacterEventForNpcScript(0) != 0 || IsNpcScriptSessionActive() != 0) &&
        (entry->flags & W8_EVENT_NO_NPC_DEFER) == 0) {
        npc_deferred_events.Add(entry);
        return 1;
    }
    pending_events.Add(entry);
    return 1;
}

// FUNCTION: WIZ8 0x0052DD20
void W8CharacterEventQueue::SetEventCharacterMask(unsigned int event_type, unsigned int party_slot,
                                                  bool enabled)
{
    unsigned char mask = (unsigned char)(1 << (party_slot & 31));

    unsigned int mask_index;
    if (!MapEventTypeToDescriptorIndex(event_type, &mask_index)) {
        return;
    }
    if (!enabled) {
        event_character_masks[mask_index] &= (unsigned char)~mask;
    } else {
        event_character_masks[mask_index] |= mask;
    }
}

// FUNCTION: WIZ8 0x0052DD90
bool W8CharacterEventQueue::HasEventCharacter(unsigned int event_type, unsigned int party_slot)
{
    unsigned int mask_index;
    unsigned char mask = (unsigned char)(1 << (party_slot & 31));

    if (!MapEventTypeToDescriptorIndex(event_type, &mask_index)) {
        return false;
    }
    return (event_character_masks[mask_index] & mask) != 0;
}

// FUNCTION: WIZ8 0x0052DC80
unsigned char W8CharacterEventQueue::TryAdjustQueuedEvent(W8CharacterEvent* entry)
{
    if (entry == 0 || active_event_type == -1 ||
        entry->event_type != (unsigned int)active_event_type) {
        return 1;
    }

    unsigned int party_slot = CharacterPointerToPartySlot(entry->character);
    if (party_slot == (unsigned int)active_party_slot) {
        return 1;
    }

    if (ClockIsTicking(recent_event_clock) == 0) {
        active_event_type = -1;
        active_party_slot = -1;
        return 1;
    }

    unsigned int event_type = entry->event_type;
    unsigned int descriptor_index;
    if (!MapEventTypeToDescriptorIndex(event_type, &descriptor_index)) {
        return 1;
    }
    if (g_character_event_descriptors_005ee000[descriptor_index].coalesce_duplicates == 0) {
        return 1;
    }

    entry->original_event_type = event_type;
    if (event_type == 4) {
        return 0;
    }
    entry->event_type = 10;
    return 1;
}

// FUNCTION: WIZ8 0x0052E460
unsigned char W8CharacterEventQueue::HasActiveEvents()
{
    return active_events.count > 0;
}

// FUNCTION: WIZ8 0x0052E470
unsigned char W8CharacterEventQueue::IsMainQueueEmpty() const
{
    return pending_events.count < 1;
}

// FUNCTION: WIZ8 0x0052DDD0
void W8CharacterEventQueue::ProcessDeferredCharacterEvents()
{
    W8CharacterEvent* entry;
    W8CharacterEvent* baseline;
    int index;
    int scan;
    int conflict_count;
    int* conflict_indices;
    int remaining_conflicts;
    unsigned int event_type;
    unsigned int party_slot;

    if (gXStatus.fSurprisePossible != 0) {
        return;
    }

    if (npc_deferred_events.count > 0 && ShouldDeferCharacterEventForNpcScript(0) == 0 &&
        IsNpcScriptSessionActive() == 0) {
        for (index = 0; index < npc_deferred_events.count; ++index) {
            QueueEntry(npc_deferred_events.data[index]);
        }
        npc_deferred_events.Clear();
    }

    if (pending_events.count != 0) {
        conflict_count = 1;
        baseline = pending_events.data[0];
        conflict_indices = new int[pending_events.count];
        conflict_indices[0] = 0;
        for (index = 1; index < pending_events.count; ++index) {
            entry = *pending_events.GetAt(index);
            event_type = entry->event_type;
            if (event_type == baseline->event_type && entry->character != baseline->character &&
                event_type != 0x2a && event_type != 0x24) {
                conflict_indices[conflict_count] = index;
                ++conflict_count;
            }
        }

        if (conflict_count > 3) {
            remaining_conflicts = conflict_count;
            while (remaining_conflicts > 3) {
                int pick = Random(conflict_count);
                if (conflict_indices[pick] != -1) {
                    conflict_indices[pick] = -1;
                    --remaining_conflicts;
                }
            }
            for (index = conflict_count - 1; index >= 0; --index) {
                if (conflict_indices[index] != -1) {
                    delete pending_events.RemoveAt(conflict_indices[index]);
                }
            }
        }
        delete[] conflict_indices;
    }

    if (pending_events.count == 0) {
        UpdateNpcDialogueVoiceAndCursor();
        if (PartyPortraitEventsIdle() != 0) {
            ProcessNpcScriptingFrame();
        }
        return;
    }

    index = 0;
    while (index < pending_events.count) {
        entry = *pending_events.GetAt(index);
        event_type = entry->event_type;
        if (g_current_screen_state.id != W8_SCREEN_MAIN_GAME) {
            unsigned int descriptor_index;
            if (MapEventTypeToDescriptorIndex(event_type, &descriptor_index) &&
                g_character_event_descriptors_005ee000[descriptor_index].defer_outside_main_game !=
                    0) {
                ++index;
                continue;
            }
        }

        if (entry->character->in_party != 0) {
            party_slot = CharacterPointerToPartySlot(entry->character);
            if (!HasEventCharacter(event_type, party_slot)) {
                if (PartyPortraitEventsIdle() == 0) {
                    return;
                }
                if (entry->dispatch_delay_ms != 0 && GetTickCount() - entry->dispatch_delay_start <=
                                                         (unsigned int)entry->dispatch_delay_ms) {
                    return;
                }
                pending_events.RemoveAt(index);
                if (TryAdjustQueuedEvent(entry) == 0) {
                    delete entry;
                    return;
                }
                if (entry->Dispatch() == 0) {
                    return;
                }
                active_events.Add(entry);
                return;
            }
        }

        for (scan = 0; scan < pending_events.count; ++scan) {
            if (pending_events.data[scan] == entry) {
                pending_events.RemoveAt(scan);
                return;
            }
        }
        return;
    }
}

/* Queue `effect` on a random eligible party member. When the first pick cannot
   receive the event the draw retries up to fifty times; exhaustion returns
   zero without queueing anything. */
// FUNCTION: WIZ8 0x0052E5C0
W8CharacterEvent* ApplyItemEffectToRandomCharacter(unsigned int event_type, int excluded_slot,
                                                   int argument, unsigned int flags)
{
    int character_slot;
    int attempts;

    if (event_type >= g_normal_event_count_005ee70c &&
        (event_type < g_first_remapped_event_005ee718 ||
         event_type >= g_remapped_event_count_005ee710 + g_first_remapped_event_005ee718)) {
        return 0;
    }
    character_slot = GetRandomCharacter(0, 0, excluded_slot, -1);
    if (character_slot == -1) {
        return 0;
    }
    attempts = 0;
    while (CanDispatchCharacterEvent(character_slot, event_type, 0) == 0) {
        if (attempts >= 0x32) {
            return 0;
        }
        character_slot = GetRandomCharacter(0, 0, excluded_slot, -1);
        ++attempts;
    }
    return QueueCharacterEvent(&g_status_685170.buffers.characters[character_slot], event_type,
                               argument, flags, g_effect_argument_005ed914);
}

// FUNCTION: WIZ8 0x0052E690
W8CharacterEvent* QueueCharacterEvent(W8Character* character, int event_type, int argument,
                                      unsigned int flags, unsigned int volume)
{
    W8CharacterEvent* entry;

    if (g_settings_6850c8.pc_confirmations == 0 &&
        (event_type == g_special_event_0068c50c || event_type == g_special_event_0068c568)) {
        return 0;
    }
    if (event_type != g_special_event_0068c504 && event_type != g_special_event_0068c550 &&
        event_type != g_special_event_0068c51c && event_type != g_special_event_0068c538 &&
        event_type != g_special_event_0068c540 && event_type != g_special_event_0068c564) {
        volume = volume * 70 / 100;
    }
    entry = new W8CharacterEvent(character, event_type, argument, flags, volume);
    if (entry != 0 && gXStatus.character_event_queue->QueueEntry(entry) == 0) {
        return 0;
    }
    return entry;
}

/* Remove one queued character event from the owned vector before dispatching
   and deleting it. Event types 14 and 15 also restart the runtime state's
   follow-up clock; bit 1 selects the short interval. */
// FUNCTION: WIZ8 0x0052D8D0
void W8CharacterEventQueue::CompleteActiveEvent(W8CharacterEvent* entry)
{
    int index = active_events.IndexOf(entry);

    if (index >= 0) {
        active_events.RemoveAt(index);
    }
    if ((follow_up_flags & 1) != 0 && entry->event_type >= 14 && entry->event_type < 16) {
        if ((follow_up_flags & 2) == 0) {
            follow_up_clock = SetCountdownClock(Random(60000) + 300000);
        } else {
            follow_up_clock = SetCountdownClock(Random(6000) + 2000);
        }
    }
    entry->Complete();
    delete entry;
}

/* Set the pose a party-slot portrait is animating toward. The request drops
   any pose animation already in progress; a slot whose character is dead or
   in a severe condition - or a party caught surprised - is forced to the
   incapacitated pose instead. */
// FUNCTION: WIZ8 0x0052F000
void SetPortraitTargetPose(W8MonsterManagerEntry* slot, int pose)
{
    int party_slot = RPCPtrToPCSlot(slot);

    slot->portrait_pose_animation_active = 0;
    if (g_status_685170.buffers.characters[party_slot].highest_condition < 0xf &&
        gXStatus.fSurprisePossible == 0) {
        if (slot->target_portrait_pose != pose) {
            slot->target_portrait_pose = pose;
        }
        return;
    }
    slot->target_portrait_pose = 2;
}

/* A character at zero percent hit points may start one of the three recovered
   incapacitation events. Which pair is available is selected by the two data
   flags; successfully queueing the event clears the matching held effect. */
// FUNCTION: WIZ8 0x0052F060
void MaybeStartIncapacitationEvent(unsigned int party_slot)
{
    W8Character* character = &g_status_685170.buffers.characters[party_slot];
    int effect;

    if ((character->hp_current * 100) / (unsigned int)character->hp_max != 0) {
        return;
    }
    effect = g_effect_005ee594;
    if (g_value_005ed8fc == 0) {
        if (g_flee_hp_fraction_005ed8f8 == 0) {
            return;
        }
        effect = Random(2) == 0 ? g_effect_005ee590 : g_effect_005ee5f8;
    }
    if (effect != -1 && QueueCharacterEvent(character, effect, 0, g_effect_argument_005ed8c8,
                                            g_effect_argument_005ed914) != 0) {
        gXStatus.character_event_queue->SetEventCharacterMask(effect, party_slot, 1);
    }
}

/* After a death in `party_slot`, pick one other eligible party member and queue
   their reaction event - the slot-two pair share one effect, later slots pick
   by gender - with a three-second clock on the queued entry. */
// FUNCTION: WIZ8 0x0052F110
void QueuePartyDeathReaction(unsigned int party_slot)
{
    W8CharacterEvent* entry;
    unsigned int selected[1];
    unsigned int remaining;
    unsigned int index;
    unsigned char skip_first_two = 0;
    int effect;

    if (party_slot < 2) {
        skip_first_two = 1;
        effect = g_effect_005ee5d8;
    } else {
        effect = g_effect_005ee5d0;
        if (g_status_685170.buffers.characters[party_slot].gender != 0) {
            effect = g_effect_005ee5d4;
        }
    }
    remaining = GetRandomPartySlots(0, 0, party_slot, selected, 1, skip_first_two);
    for (index = 0; index < remaining; ++index) {
        entry = QueueCharacterEvent(&g_status_685170.buffers.characters[selected[index]], effect, 0,
                                    g_effect_argument_005ed8cc, g_effect_argument_005ed914);
        if (entry != 0) {
            entry->dispatch_delay_ms = 3000;
            entry->dispatch_delay_start = GetTickCount();
        }
    }
}

/* Queue a low-HP flee or incapacitation event when the character is still
   standing, then always roll one of three ambient follow-up events. */
// FUNCTION: WIZ8 0x0052F2C0
void QueueDamageReactionEvents(W8Character* character)
{
    unsigned int hp_percent;
    unsigned int party_slot;
    bool has_incapacitation_event;
    bool has_flee_event;
    W8CharacterEvent* entry;
    int effect;
    int follow_up_events[3];

    hp_percent = (character->hp_current * 100) / (unsigned int)character->hp_max;
    party_slot = CharacterPointerToPartySlot(character);
    has_incapacitation_event =
        gXStatus.character_event_queue->HasEventCharacter(g_effect_005ee594, party_slot);
    has_flee_event =
        gXStatus.character_event_queue->HasEventCharacter(g_effect_005ee590, party_slot);
    gXStatus.character_event_queue->HasEventCharacter(g_effect_005ee5f8, party_slot);
    if (character->highest_condition != 0xf && character->highest_condition != 0x11) {
        if (g_value_005ed8fc <= hp_percent || has_incapacitation_event) {
            if (g_flee_hp_fraction_005ed8f8 <= hp_percent) {
                goto queue_follow_up_event;
            }
            if (Random(2) != 0 || has_flee_event) {
                effect = g_effect_005ee5f8;
            } else {
                effect = g_effect_005ee590;
            }
            entry = QueueCharacterEvent(character, effect, g_effect_argument_005ed8d4,
                                        g_effect_argument_005ed8d0, g_effect_argument_005ed914);
        } else {
            entry = QueueCharacterEvent(character, g_effect_005ee594, g_effect_argument_005ed8d4,
                                        g_effect_argument_005ed8d0, g_effect_argument_005ed914);
        }
        if (entry != 0) {
            entry->dispatch_delay_ms = 0x5dc;
            entry->dispatch_delay_start = GetTickCount();
        }
    }
queue_follow_up_event:
    follow_up_events[0] = g_special_event_0068c544;
    follow_up_events[1] = g_special_event_0068c550;
    follow_up_events[2] = g_special_event_0068c51c;
    QueueCharacterEvent(character, follow_up_events[Random(3)], g_effect_argument_005ed8d4,
                        g_effect_argument_005ed8cc, g_effect_argument_005ed914);
}

/* Turn-begin path for several surviving party members: queue event 0x15 on
   one random eligible character. */
// FUNCTION: WIZ8 0x0052F1D0
void QueueTurnReactionEvent(void)
{
    unsigned int selected[1];
    unsigned int index;
    unsigned int remaining;

    remaining = GetRandomPartySlots(0, 0, -1, selected, 1, 0);
    for (index = 0; index < remaining; ++index) {
        QueueCharacterEvent(&g_status_685170.buffers.characters[selected[index]], g_effect_005ee5dc,
                            0, g_effect_argument_005ed8cc, g_effect_argument_005ed914);
    }
}

/* Turn-begin path when only one occupied party member is still standing:
   that character says event 0x16. */
// FUNCTION: WIZ8 0x0052F240
void QueueLastSurvivorEvent(void)
{
    int alive_count = 0;
    int last_alive = 0;

    for (int slot = 0; slot < 8; ++slot) {
        if (g_status_685170.buffers.party_rows[slot].occupied != 0 &&
            g_status_685170.buffers.characters[slot].condition_turns[W8_CONDITION_DEAD] == 0) {
            ++alive_count;
            last_alive = slot;
        }
    }
    if (alive_count != 0) {
        QueueCharacterEvent(&g_status_685170.buffers.characters[last_alive], g_effect_005ee5e0, 0,
                            g_effect_argument_005ed8cc, g_effect_argument_005ed914);
    }
}

/* Reacts to a freshly recomputed highest_condition: the armed one-shot flag
   swallows one change, otherwise the character queues the reaction event for
   the new condition. A dying character first loses every queued event, and a
   charmed character makes a different party member say the line instead. */
// FUNCTION: WIZ8 0x0052F430
void QueueConditionChangeReaction(W8Character* character)
{
    int events[3];
    int excluded_slot;
    int slot;
    int attempts;
    unsigned int reaction;

    if (GetFlag68C4FA() != 0) {
        return;
    }
    if (g_status_685170.skip_next_condition_reaction != 0) {
        g_status_685170.skip_next_condition_reaction = 0;
        return;
    }
    switch (character->highest_condition) {
    case 2:
    case 7:
    case 9:
    case 10:
        reaction = g_value_005ee59c;
        if (Random(2) == 0) {
            reaction = g_value_005ee5a0;
        }
        QueueCharacterEvent(character, reaction, 0, g_effect_argument_005ed8cc,
                            g_effect_argument_005ed914);
        return;
    case 3:
    case 4:
        QueueCharacterEvent(character, g_special_event_0068c52c, 0, g_effect_argument_005ed8cc,
                            g_effect_argument_005ed914);
        return;
    case 5:
        QueueCharacterEvent(character, g_special_event_0068c558, 0, g_effect_argument_005ed8cc,
                            g_effect_argument_005ed914);
        return;
    case 6:
        QueueCharacterEvent(character, g_special_event_0068c514, 0, g_effect_argument_005ed8cc,
                            g_effect_argument_005ed914);
        return;
    case 8:
        QueueCharacterEvent(character, g_special_event_0068c508, 0, g_effect_argument_005ed8cc,
                            g_effect_argument_005ed914);
        return;
    case 0xb:
        QueueCharacterEvent(character, g_special_event_0068c578, 0, g_effect_argument_005ed8cc,
                            g_effect_argument_005ed914);
        break;
    case 0xc:
        QueueCharacterEvent(character, g_effect_005ee5a4, 0, g_effect_argument_005ed8cc,
                            g_effect_argument_005ed914);
        return;
    case 0xe:
    case 0x10:
        QueueCharacterEvent(character, g_effect_005ee5ac, 0, g_effect_argument_005ed8cc,
                            g_effect_argument_005ed914);
        return;
    case 0x11:
        events[0] = g_special_event_0068c538;
        events[1] = g_special_event_0068c540;
        events[2] = g_special_event_0068c564;
        QueueCharacterEvent(character, events[Random(3)], 0, g_effect_argument_005ed8c8,
                            g_effect_argument_005ed914);
        return;
    case 0x12:
        gXStatus.character_event_queue->RemoveCharacterEvents(character);
        events[0] = g_special_event_0068c538;
        events[1] = g_special_event_0068c540;
        events[2] = g_special_event_0068c564;
        QueueCharacterEvent(character, events[Random(3)], g_effect_argument_005ed8d4,
                            g_effect_argument_005ed8cc, g_effect_argument_005ed914);
        return;
    case 0x13:
        excluded_slot = CharacterPointerToPartySlot(character);
        reaction = g_effect_005ee628;
        if ((reaction < g_normal_event_count_005ee70c ||
             (g_first_remapped_event_005ee718 <= reaction &&
              reaction < g_remapped_event_count_005ee710 + g_first_remapped_event_005ee718)) &&
            (slot = GetRandomCharacter(0, 0, excluded_slot, -1)) != -1) {
            attempts = 0;
            while (CanDispatchCharacterEvent(slot, reaction, 0) == 0) {
                if (attempts >= 0x32) {
                    return;
                }
                slot = GetRandomCharacter(0, 0, excluded_slot, -1);
                ++attempts;
            }
            QueueCharacterEvent(&g_status_685170.buffers.characters[slot], reaction, 0,
                                g_effect_argument_005ed8cc, g_effect_argument_005ed914);
            return;
        }
        break;
    }
}

/* Reacts to a condition being lifted: when nothing remains as the highest
   condition the character announces full recovery (0x55), otherwise the
   surviving-condition line (0x54); selected conditions map to fixed events. */
// FUNCTION: WIZ8 0x0052F790
void QueueConditionClearedReaction(W8Character* character, int condition)
{
    if (GetFlag68C4FA() != 0) {
        return;
    }
    if (g_status_685170.skip_next_condition_reaction != 0) {
        g_status_685170.skip_next_condition_reaction = 0;
        return;
    }
    switch (condition) {
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 0xb:
    case 0xc:
    case 0xe:
    case 0x10:
    case 0x11:
        if (character->highest_condition == 0) {
            QueueCharacterEvent(character, g_effect_005ee6dc, 0, g_effect_argument_005ed8cc,
                                g_effect_argument_005ed914);
            return;
        }
        QueueCharacterEvent(character, g_effect_005ee6d8, 0, g_effect_argument_005ed8cc,
                            g_effect_argument_005ed914);
        break;
    case 10:
    case 0x13:
        QueueCharacterEvent(character, g_effect_005ee5b4, 0, g_effect_argument_005ed8cc,
                            g_effect_argument_005ed914);
        return;
    case 0x12:
        QueueCharacterEvent(character, g_effect_005ee5b8, 0, g_effect_argument_005ed8cc,
                            g_effect_argument_005ed914);
        return;
    }
}

/* Requeue the stored portrait event of the selected character with a forced
   argument of 4; the selection and the stored type are cleared elsewhere. */
// FUNCTION: WIZ8 0x0052E480
void RequeueSelectedPortraitEvent(void)
{
    int slot = g_status_685170.selected_character;

    if (slot != -1) {
        unsigned int event_type = g_status_685170.buffers.party_rows[slot].pending_event_type_ff;
        if (event_type != 0) {
            QueueCharacterEvent(&g_status_685170.buffers.characters[slot], event_type, 4, 0, 0x7f);
        }
    }
}

/* Occupied slots with portrait_event_active set still have a portrait/voice record in
   flight; trap-trigger follow-up waits until none of those are active. */
// FUNCTION: WIZ8 0x0052E590
unsigned char PartyPortraitEventsIdle(void)
{
    W8PartySlotRow* row = g_status_685170.buffers.party_rows;
    const W8MonsterManagerEntry* current;

    for (current = gXStatus.monster_manager_entries; current < &gXStatus.monster_manager_entries[8];
         ++current, ++row) {
        if (row->occupied != 0 && current->portrait_event_active != 0) {
            return 0;
        }
    }
    return 1;
}

/* Advance the eight character portrait/voice records. This is the complete
   per-frame state machine: it drains finished owned events, starts the
   incapacitation path when no record is active, advances facing and pose
   clocks, and asks the current screen to redraw a changed slot. */
// FUNCTION: WIZ8 0x0052E750
int UpdateCharacterEventState(void)
{
    unsigned int party_slot;
    int any_active = 0;

    for (party_slot = 0; party_slot < 8; ++party_slot) {
        W8MonsterManagerEntry* record = &gXStatus.monster_manager_entries[party_slot];
        unsigned char sound_active = 0;

        if (g_status_685170.buffers.party_rows[party_slot].occupied == 0) {
            continue;
        }
        if (record->portrait_event_active != 0) {
            if (record->voice_sound_handle == -1) {
                if (record->voice_time_remaining_ms == 0) {
                    if (record->active_character_event == 0) {
                        SetPartyPortraitEventState(party_slot, 0, -1, 0, 1);
                    } else {
                        gXStatus.character_event_queue->CompleteActiveEvent(
                            record->active_character_event);
                    }
                }
            } else {
                Function5E2F40(record->voice_sound_handle, &record->mouth_gap);
                sound_active = record->mouth_gap.mouth_open;
            }
        }

        if (record->portrait_event_active == 0) {
            unsigned int scan;
            for (scan = 0; scan < 8; ++scan) {
                if (g_status_685170.buffers.party_rows[scan].occupied != 0 &&
                    gXStatus.monster_manager_entries[scan].portrait_event_active != 0) {
                    break;
                }
            }
            if (scan == 8) {
                MaybeStartIncapacitationEvent(party_slot);
            }
        } else {
            W8Character* character = &g_status_685170.buffers.characters[party_slot];
            if ((character->highest_condition > 14 || character->hp_current == 0) &&
                record->active_character_event != 0) {
                gXStatus.character_event_queue->CompleteActiveEvent(record->active_character_event);
            }
            if (record->portrait_event_active == 0) {
                unsigned int scan;
                for (scan = 0; scan < 8; ++scan) {
                    if (g_status_685170.buffers.party_rows[scan].occupied != 0 &&
                        gXStatus.monster_manager_entries[scan].portrait_event_active != 0) {
                        break;
                    }
                }
                if (scan == 8) {
                    MaybeStartIncapacitationEvent(party_slot);
                }
            } else {
                any_active = 1;
                if (sound_active == 0) {
                    if (ClockIsTicking(record->portrait_frame_clock) == 0) {
                        if (record->voice_time_remaining_ms < 120) {
                            record->previous_portrait_frame = record->portrait_frame;
                            record->portrait_frame = 6;
                            record->portrait_pose_dirty = 1;
                            record->voice_time_remaining_ms = 0;
                        } else {
                            int direction = ChooseDifferentMonsterDirection004C2E00(
                                                (short)record->portrait_frame - 6) +
                                            6;
                            if (g_value_0068c57c <= record->field_113 &&
                                record->field_113 <= g_value_0068c554) {
                                direction = 8;
                            }
                            record->previous_portrait_frame = record->portrait_frame;
                            record->portrait_frame = direction;
                            record->portrait_pose_dirty = 1;
                            record->portrait_frame_clock = SetCountdownClock(120);
                            record->voice_time_remaining_ms -= 120;
                        }
                    }
                } else {
                    record->previous_portrait_frame = record->portrait_frame;
                    record->portrait_frame = 6;
                    record->portrait_pose_dirty = 1;
                }
            }
        }

        if (gXStatus.fNpcDialogueMode != 0 && (party_slot & 1) != 0 &&
            Function56EC90(party_slot) != 0) {
            continue;
        }
        if (record->portrait_frame_dirty == 0 && record->field_0bd == 0 &&
            (g_current_screen_state.id != W8_SCREEN_CHARACTER ||
             record->portrait_event_active != 0)) {
            if (record->portrait_pose_animation_active == 0) {
                if (record->portrait_pose == record->target_portrait_pose) {
                    if (record->portrait_pose == 1 &&
                        ClockIsTicking(record->portrait_idle_clock) == 0) {
                        record->portrait_pose_animation_active = 1;
                        record->portrait_idle_clock = SetCountdownClock(Random(5000) + 5000);
                    }
                } else if (ClockIsTicking(record->portrait_pose_clock) == 0) {
                    int pose = record->portrait_pose;
                    record->previous_portrait_pose = pose;
                    record->portrait_pose =
                        g_portrait_tables_0061cb3c
                            .pose_transition[pose * 5 + record->target_portrait_pose];
                    record->portrait_pose_animation_active = 1;
                    record->portrait_pose_clock = SetCountdownClock(Random(50) + 50);
                }
            } else if (ClockIsTicking(record->portrait_pose_clock) == 0) {
                int pose = record->portrait_pose;
                if (pose != 2) {
                    record->previous_portrait_pose = pose;
                    record->portrait_pose =
                        g_portrait_tables_0061cb3c.pose_transition[pose * 5 + 2];
                    record->portrait_pose_animation_active = 1;
                    record->portrait_pose_clock = SetCountdownClock(Random(50) + 50);
                }
                if (record->portrait_pose == 2) {
                    record->portrait_pose_animation_active = 0;
                }
            }
            if ((record->portrait_pose_animation_active != 0 || record->portrait_pose_dirty != 0) &&
                record->field_0cf == 0) {
                RefreshPartySlotDisplay(party_slot);
            }
        }
    }
    return any_active;
}
