#include "wiz8/character.h"
#include "wiz8/local_code/GameplayTime.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/Navigator.h"
#include "wiz8/engine_code/OctPath.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/float_constants.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_code/Sight.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/magic.h"
#include "wiz8/targeting.h"
#include "wiz8/xstatus.h"

#define GAMEPLAYTIME_CPP "C:\\Projects\\Wizardry 8\\Local Code\\GameplayTime.cpp"

/*
 * Local Code\GameplayTime.cpp.
 *
 * The character's out-of-combat regeneration rates, derived from the pool
 * ceilings, and the per-monster aging cycle tick at 0x00503990 - moved here
 * from Sight.cpp: the retail body pushes this file's path string.
 */

/* 0x00502B50: the hit-point, stamina and per-realm spell regeneration rates
   are one tick of the pool ceiling's share, twenty points of base and a
   twelveth of a minute each; the modifier block's three doubled-cost flags
   raise their rate by half. */
// FUNCTION: WIZ8 0x00502b50
void RebuildCharacterRegenRates00502B50(W8Character* character)
{
    float rate;
    int realm;

    rate = ((float)character->hp_max * 0.4f + 20.0f) * 0.0041666669f;
    character->health_regen_rate_0b69 = rate;
    if (character->bonus_1770.flag_42 != 0) {
        character->health_regen_rate_0b69 = rate * 1.5f;
    }

    rate = ((float)character->stamina_max * 0.9f + 20.0f) * 0.0041666669f;
    character->stamina_regen_rate_0b71 = rate;
    if (character->bonus_1770.flag_43 != 0) {
        character->stamina_regen_rate_0b71 = rate * 1.5f;
    }

    for (realm = 0; realm < W8_SPELL_REALM_COUNT; ++realm) {
        if (character->sp_max[realm] == 0) {
            character->spell_regen_rates_0b79[realm * 2] = 0.0f;
            continue;
        }
        rate = ((float)character->sp_max[realm] * 0.65f + 20.0f) * 0.0041666669f;
        character->spell_regen_rates_0b79[realm * 2] = rate;
        if (character->bonus_1770.flag_44 != 0) {
            character->spell_regen_rates_0b79[realm * 2] = rate * 1.5f;
        }
    }
}

/* Advance one monster's whole aging cycle by the elapsed minutes: the
   look-around timers, the sight bookkeeping around its last-seen position,
   the per-turn regeneration and fatigue bookkeeping, condition, enchantment
   and effect countdowns, and finally the combat effect slots. */
// FUNCTION: WIZ8 0x00503990
void AgeMonsterSight(W8MonsterInfo* monster_info, unsigned int minutes, int arg_3)
{
    W8MonsterRecord* record;
    W8Monster* monster;
    bool frost_condition = false;

    record = GetMonsterDataForInfo(monster_info);
    if (monster_info->fInCombat != 0) {
        int args[2];

        args[0] = 3;
        args[1] = monster_info->location_id;
        Function5526F0(monster_info->pCombat->entries_3e, args);
        goto after_early;
    }
    monster = monster_info->monster;
    if (monster->stay_home_291 != 0) {
        srVector3T<float> location;
        srVector3T<float> last_seen;
        srVector3T<float> delta;

        MonsterGetLocation(monster, &location);
        if (monster->formation.x == g_float_005ebb34 && monster->formation.y == g_float_005ebb34 &&
            monster->formation.z == g_float_005ebb34) {
            monster->formation = monster->GetPosition();
        }
        last_seen = monster->formation;
        delta = last_seen - location;
        if (delta.Length() > 500.0f) {
            srVector3T<float> probe = last_seen;
            srVector3T<float> camera;

            probe.y += g_float_005ebc64;
            GetCameraPosition(&camera);
            if (ProjectPointThroughCamera004BE940(&probe) == 0 &&
                g_octree_6598a4->HasLineOfSight(&camera, &probe, 1) == 0) {
                srVector3T<float> notify_position = last_seen;
                unsigned int group_index = GetMonsterGroupIndexByID(
                    0x4d2, GAMEPLAYTIME_CPP, monster_info->monster_group_id, 1);
                W8MonsterGroup* group = GetMonsterGroupByListIndex(group_index);

                Function510CC0(group, &notify_position, monster->GetYaw(), 0, 0, 0, 0);
                for (int index = 0; index < 4; ++index) {
                    if (group->allied_group_ids[index] != 0) {
                        group_index = GetMonsterGroupIndexByID(0x4d9, GAMEPLAYTIME_CPP,
                                                               group->allied_group_ids[index], 1);
                        group = GetMonsterGroupByListIndex(group_index);
                        Function510CC0(group, &notify_position, monster->GetYaw(), 0, 0, 0, 0);
                    }
                }
            }
        }
        goto after_early;
    }
    if (GetViewDistance() != g_sight_default_005ec254) {
        goto after_early;
    }
    if ((monster->linked_navigator_05c == 0 && monster->flag_025 == 0) &&
        ((signed char)monster_info->unknown_254 > 1 || monster->flag_024 == 0)) {
        srVector3T<float> location;
        srVector3T<float> previous;
        srVector3T<float> delta;
        unsigned char cycle;

        MonsterGetLocation(monster, &location);
        previous.x = static_cast<float>(monster_info->runtime_values_338[0]);
        previous.y = static_cast<float>(monster_info->runtime_values_338[1]);
        previous.z = static_cast<float>(monster_info->runtime_values_338[2]);
        monster_info->position_17.y = location.y;
        cycle = monster_info->unknown_254;
        delta = location - previous;
        monster_info->position_17.x = location.x;
        monster_info->position_17.z = location.z;
        if ((signed char)cycle > 1) {
            bool cycle_cleared = false;
            bool flags_cleared = false;

            if ((signed char)cycle < 4) {
                bool cleared = false;

                if (delta.Length() < 500.0f) {
                    srVector3T<float> navigator_position = monster->GetPosition();

                    if (g_pathing_00659c60->SnapWaypointPosition00462E60(&navigator_position, 0) ==
                            0 &&
                        monster_info->party_threat.flag_25 == 0) {
                        srVector3T<float> next_position;

                        monster->movement_0c0.attachment_0ac->GetNextPosition00456660(
                            &next_position);
                        if (next_position.Length() == static_cast<float>(g_zero_005ebb40)) {
                            next_position = monster->GetPosition();
                        }
                        {
                            srVector3T<float> camera;
                            srVector3T<float> probe = next_position;

                            GetCameraPosition(&camera);
                            if (ProjectPointThroughCamera004BE940(&probe) == 0 &&
                                g_octree_6598a4->HasLineOfSight(&camera, &probe, 1) == 0) {
                                srVector3T<float> notify_position = next_position;
                                unsigned int group_index = GetMonsterGroupIndexByID(
                                    0x50f, GAMEPLAYTIME_CPP, monster_info->monster_group_id, 1);
                                W8MonsterGroup* group = GetMonsterGroupByListIndex(group_index);

                                Function510CC0(group, &notify_position, monster->GetYaw(), 0, 0, 0,
                                               0);
                                for (int index = 0; index < 4; ++index) {
                                    if (group->allied_group_ids[index] != 0) {
                                        group_index = GetMonsterGroupIndexByID(
                                            0x516, GAMEPLAYTIME_CPP, group->allied_group_ids[index],
                                            1);
                                        group = GetMonsterGroupByListIndex(group_index);
                                        Function510CC0(group, &notify_position, monster->GetYaw(),
                                                       0, 0, 0, 0);
                                    }
                                }
                                cleared = true;
                            }
                        }
                    }
                }
                cycle_cleared = cleared;
                flags_cleared = true;
            } else {
                srVector3T<float> camera;
                srVector3T<float> probe;
                srVector3T<float> location2;

                GetCameraPosition(&camera);
                location2 = monster->GetPosition();
                probe = location2;
                probe.y += g_float_005ebc64;
                if (ProjectPointThroughCamera004BE940(&location2) == 0 &&
                    g_octree_6598a4->HasLineOfSight(&camera, &probe, 1) == 0) {
                    if (monster->formation.x == g_float_005ebb34 &&
                        monster->formation.y == g_float_005ebb34 &&
                        monster->formation.z == g_float_005ebb34) {
                        monster->formation = monster->GetPosition();
                    }
                    probe = monster->formation;
                    probe.y += g_float_005ebc64;
                    if (ProjectPointThroughCamera004BE940(&location2) == 0 &&
                        g_octree_6598a4->HasLineOfSight(&camera, &probe, 1) == 0) {
                        if (monster->formation.x == g_float_005ebb34 &&
                            monster->formation.y == g_float_005ebb34 &&
                            monster->formation.z == g_float_005ebb34) {
                            monster->formation = monster->GetPosition();
                        }
                        {
                            srVector3T<float> notify_position = monster->formation;
                            unsigned int group_index = GetMonsterGroupIndexByID(
                                0x53b, GAMEPLAYTIME_CPP, monster_info->monster_group_id, 1);
                            W8MonsterGroup* group = GetMonsterGroupByListIndex(group_index);

                            Function510CC0(group, &notify_position, monster->GetYaw(), 0, 0, 0, 0);
                            for (int index = 0; index < 4; ++index) {
                                if (group->allied_group_ids[index] != 0) {
                                    group_index = GetMonsterGroupIndexByID(
                                        0x542, GAMEPLAYTIME_CPP, group->allied_group_ids[index], 1);
                                    group = GetMonsterGroupByListIndex(group_index);
                                    Function510CC0(group, &notify_position, monster->GetYaw(), 0, 0,
                                                   0, 0);
                                }
                            }
                        }
                        cycle_cleared = true;
                        flags_cleared = true;
                    }
                }
            }
            if (cycle_cleared) {
                monster_info->unknown_254 = 0;
            }
            if (flags_cleared) {
                monster_info->flag_255 = 0;
                monster_info->unknown_246 = 0;
            }
        }
        if ((signed char)monster_info->unknown_254 > 0) {
            ++monster_info->unknown_254;
        }
        monster_info->runtime_values_338[0] = static_cast<int>(monster_info->position_17.x);
        monster_info->runtime_values_338[1] = static_cast<int>(monster_info->position_17.y);
        monster_info->runtime_values_338[2] = static_cast<int>(monster_info->position_17.z);
    }

after_early: {
    unsigned int amount = monster_info->modifiers_1db.unknown_08[0];

    if (amount != 0) {
        W8TargetSource source;

        amount *= minutes;
        if (monster_info->fInCombat != 0) {
            amount += amount >> 1;
        }
        ResetTargetSource(&source);
        Function52BB60(monster_info, amount, &source, 1, gXStatus.fCombatMode, 0, 0, 0);
    }
}
    if (monster_info->condition_turns[2] != 0) {
        frost_condition = true;
    }
    {
        W8MonsterRecord* data = GetMonsterDataForInfo(monster_info);
        int amount = (static_cast<int>(static_cast<signed char>(data->unknown_150[0x2c])) +
                      static_cast<int>(
                          static_cast<signed char>(monster_info->modifiers_1db.unknown_08[1]))) *
                     static_cast<int>(minutes);

        if (amount < 1) {
            if (amount < 0) {
                W8TargetSource source;

                ResetTargetSource(&source);
                Function52BB60(monster_info, -amount, &source, 0, 0, 0, 0, 0);
            }
        } else if (monster_info->hp_current < monster_info->hp_max) {
            HealMonster(monster_info, amount, 0);
        }
    }
    {
        int amount =
            (static_cast<int>(static_cast<signed char>(monster_info->modifiers_1db.unknown_08[2])) +
             static_cast<int>(static_cast<signed char>(record->unknown_0cd[1]))) *
            static_cast<int>(minutes);

        if (amount < 1) {
            if (amount < 0) {
                FatigueMonster(monster_info, -amount, 0);
            }
        } else if (monster_info->runtime_stat_current_33 < monster_info->runtime_stat_max_2f) {
            RestoreMonsterStamina(monster_info, amount, 0);
        }
    }
    {
        float heal_scale;

        if (gXStatus.fSurprisePossible == 0 && arg_3 == 0) {
            if (monster_info->fInCombat == 0) {
                heal_scale = 0.5f;
            } else {
                heal_scale = 0.0f;
            }
        } else {
            heal_scale = 1.0f;
        }
        if (frost_condition) {
            heal_scale *= g_navigator_vertical_phase_step_005ebcc8;
        }
        if (heal_scale > g_float_005ebb34) {
            if (monster_info->hp_current < monster_info->hp_max) {
                monster_info->hp_regen_accumulator_4b =
                    static_cast<float>(minutes) * monster_info->hp_regen_rate_47 * heal_scale +
                    monster_info->hp_regen_accumulator_4b;
                int healed = static_cast<int>(monster_info->hp_regen_accumulator_4b);

                HealMonster(monster_info, healed, 0);
                monster_info->hp_regen_accumulator_4b -= static_cast<float>(healed);
            }
            if (monster_info->runtime_stat_current_33 < monster_info->runtime_stat_max_2f) {
                monster_info->stamina_regen_accumulator_53 =
                    static_cast<float>(minutes) * monster_info->stamina_regen_rate_4f * heal_scale +
                    monster_info->stamina_regen_accumulator_53;
                int restored = static_cast<int>(monster_info->stamina_regen_accumulator_53);

                RestoreMonsterStamina(monster_info, restored, 0);
                monster_info->stamina_regen_accumulator_53 -= static_cast<float>(restored);
            }
        }
    }
    for (int condition = 0; condition < 0x14; ++condition) {
        if (monster_info->condition_turns[condition] != 0 &&
            static_cast<unsigned int>(monster_info->condition_turns[condition]) < 9999) {
            Function524110(monster_info->location_id, condition, minutes);
        }
    }
    for (int enchant = 0; enchant < 8; ++enchant) {
        float remaining = static_cast<float>(monster_info->enchantments[enchant].value_08);

        if (remaining != 0.0f && static_cast<unsigned int>(remaining) < 9999) {
            if (minutes < static_cast<unsigned int>(remaining)) {
                monster_info->enchantments[enchant].value_08 =
                    static_cast<int>(remaining) - static_cast<int>(minutes);
            } else {
                ClearMonsterEnchantmentSlot(monster_info->location_id, enchant);
            }
        }
    }
    if (monster_info->hp_max <= monster_info->hp_current) {
        monster_info->hp_regen_accumulator_4b = 0.0f;
    }
    if (monster_info->runtime_stat_max_2f <= monster_info->runtime_stat_current_33) {
        monster_info->stamina_regen_accumulator_53 = 0.0f;
    }
    {
        W8EffectSlot* slot = monster_info->effect_slots_10f;

        for (int owned_index = 0; owned_index < 0xc; ++owned_index) {
            if (slot->active != 0) {
                if (minutes < static_cast<unsigned int>(slot->duration_0d)) {
                    slot->duration_0d =
                        static_cast<int>(slot->duration_0d) - static_cast<int>(minutes);
                } else if (monster_info == 0) {
                    ResetPartyEffectBlock(slot);
                } else {
                    ClearEffectSlot(monster_info, slot);
                }
            }
            ++slot;
        }
    }
    if (monster_info->fInCombat != 0) {
        for (int combat_index = 0; combat_index < 9; ++combat_index) {
            W8EffectSlot* slot = &monster_info->pCombat->entries_3e[combat_index];

            if (slot->active != 0) {
                if (minutes < static_cast<unsigned int>(slot->duration_0d)) {
                    slot->duration_0d =
                        static_cast<int>(slot->duration_0d) - static_cast<int>(minutes);
                } else if (monster_info == 0) {
                    ResetPartyEffectBlock(slot);
                } else {
                    ClearEffectSlot(monster_info, slot);
                }
            }
        }
        for (int d7_index = 0; d7_index < 6; ++d7_index) {
            W8EffectSlot* slot = &monster_info->pCombat->entries_d7[d7_index];

            if (slot->active != 0) {
                if (minutes < static_cast<unsigned int>(slot->duration_0d)) {
                    slot->duration_0d =
                        static_cast<int>(slot->duration_0d) - static_cast<int>(minutes);
                } else if (monster_info == 0) {
                    ResetPartyEffectBlock(slot);
                } else {
                    ClearEffectSlot(monster_info, slot);
                }
            }
        }
    }
}
