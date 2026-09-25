#include "wiz8/layouts/character.h"
#include "wiz8/character_event_queue.h"
#include "wiz8/character_skills.h"
#include "wiz8/local_code/CharGeneration.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/GameplayMods.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/local_code/party_encumbrance.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/UtilityFunctions.h"
#include "wiz8/local_code/SpellEffect.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/local_code/GameplayTime.h"
#include "wiz8/engine_code/Environment.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/Navigator.h"
#include "wiz8/engine_code/OctPath.h"
#include "wiz8/engine_code/PolyPick.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/float_constants.h"
#include "wiz8/local_code/Sight.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_code/NPCManager.h"
#include "wiz8/monster_runtime.h"
#include "wiz8/monster_generators.h"
#include "wiz8/engine_code/Spells.h"
#include "wiz8/engine_code/Levels.h"
#include "wiz8/local_code/Targeting.h"
#include "wiz8/level_specific_code/MasterFunctionList.h"
#include "wiz8/xstatus.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/GameTimeAccumulator0043A910.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/dice.h"
#include "wiz8/fact_state.h"
#include "wiz8/utility.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/NPCInteractionSubscreen.h"
#include "wiz8/local_screens/MGSButtons.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/local_screens/Screens.h"
#include "wiz8/music_playlist.h"
#include "wiz8/regions.h"
#include "wiz8/sr_api.h"
#include "random.h"
#include "soundman.h"
#include "timer.h"

#define GAMEPLAYTIME_CPP "C:\\Projects\\Wizardry 8\\Local Code\\GameplayTime.cpp"

/*
 * Local Code\GameplayTime.cpp.
 *
 * The character's out-of-combat regeneration rates, derived from the pool
 * ceilings, and the per-monster aging cycle tick at 0x00503990 - moved here
 * from Sight.cpp: the retail body pushes this file's path string.
 */

/* The per-frame game-time driver: folds the elapsed milliseconds into the
   world clock, fires the 120-second aging tick, the hourly item recharge,
   the 2500ms camping-fatigue and 1400ms stamina ticks, and runs the surprise
   sequence entry/transition while the party is eligible. */
// FUNCTION: WIZ8 0x00502010
void UpdateGameClock00502010(int elapsed)
{
    g_status_685170.world_clock +=
        static_cast<int>((g_status_685170.world_clock_ms_18dc + elapsed) / 1000);
    g_status_685170.world_clock_ms_18dc = (g_status_685170.world_clock_ms_18dc + elapsed) % 1000;

    g_status_685170.aging_accumulator_238b += elapsed;
    unsigned int aging_ticks;
    if (gXStatus.item_drag_active == 0) {
        aging_ticks = g_status_685170.aging_accumulator_238b / 120000;
        if (aging_ticks != 0) {
            g_status_685170.aging_accumulator_238b %= 120000;
            AdvanceTimedEffects00502D00(aging_ticks);
        }
    }

    g_status_685170.item_recharge_ms_239d += elapsed;
    unsigned int hours = g_status_685170.item_recharge_ms_239d / 3600000;
    if (hours != 0) {
        g_status_685170.item_recharge_ms_239d %= 3600000;
        for (unsigned int slot = 0; slot < 8; ++slot) {
            char uses = static_cast<char>(hours);
            if (g_status_685170.buffers.XChar[slot].fOccupied == 0) {
                continue;
            }
            W8Character* character = &g_status_685170.buffers.Char[slot];
            int i;
            W8ItemInstance* item = character->EquippedItem;
            for (i = 0xc; i != 0; --i) {
                if (item->iItemNo == 0x266) {
                    AddItemUses(item, uses);
                }
                ++item;
            }
            item = character->backpack;
            for (i = 8; i != 0; --i) {
                if (item->iItemNo == 0x266) {
                    AddItemUses(item, uses);
                }
                ++item;
            }
        }
        for (unsigned int index = 0;
             index < static_cast<unsigned int>(g_status_685170.party_item_count_1791); ++index) {
            W8ItemInstance* item = &g_status_685170.party_item_pool_0021[index];
            if (item->iItemNo == 0x266) {
                AddItemUses(item, static_cast<char>(hours));
            }
        }
    }

    if ((gXStatus.fCombatMode == 0 || aging_ticks != 0) && g_camera_sway_active_652da4 != 0) {
        if (gXStatus.fCombatMode != 0) {
            if (aging_ticks == 0) {
                srAssertFail("uiTurnsElapsed > 0", GAMEPLAYTIME_CPP, 0x6d, 0);
            }
            g_status_685170.camp_tick_ms_2436 -= 4 * 0x9c4;
            UpdateCampFatigue005044D0(4);
        } else {
            g_status_685170.camp_tick_ms_2436 += elapsed;
            unsigned int camp_ticks = g_status_685170.camp_tick_ms_2436 / 0x9c4;
            if (camp_ticks != 0) {
                g_status_685170.camp_tick_ms_2436 -= camp_ticks * 0x9c4;
                UpdateCampFatigue005044D0(static_cast<int>(camp_ticks));
            }
        }
    } else {
        g_status_685170.camp_tick_ms_2436 = 0;
        g_status_685170.party_fatigued_2433 = 0;
        g_status_685170.camp_fatigue_count_2498 = 0;
    }

    if (gXStatus.fCombatMode != 0) {
        g_status_685170.stamina_tick_ms_2483 = 0;
    } else {
        g_status_685170.stamina_tick_ms_2483 += elapsed;
        unsigned int stamina_ticks = g_status_685170.stamina_tick_ms_2483 / 0x578;
        if (stamina_ticks != 0) {
            g_status_685170.stamina_tick_ms_2483 %= 0x578;
            UpdatePartyStamina00504670(static_cast<int>(stamina_ticks));
        }

        if (gXStatus.fSurprisePossible == 0 && AnyCharacterEngaged() == false &&
            AnyCharacterActive() && g_status_685170.party_fatigued_2433 == 0 &&
            HasLevelDataVector() == 0 && static_cast<char>(GetLevelDataFlag4()) != 0 &&
            static_cast<char>(IsScreenIdle()) != 0) {
            if (gXStatus.world_update_blocked != 0) {
                ResumeMainGameWorld();
            }
            if (AnyCharacterEngaged() == false) {
                gXStatus.surprise_unengaged = 1;
                ShowNotice(0xc, gppStringList[0x794], -1, 0xffffffff, 0);
            } else {
                gXStatus.surprise_unengaged = 0;
                ShowNoticef(0xc, gppStringList[0x790], 8);
            }
            SetNpcQuoteBubbleVisible(false, 0, 0, -1, 0xffffffff);
            gXStatus.character_event_queue->CompleteAllActiveEvents();
            for (unsigned int slot = 0; slot < 8; ++slot) {
                if (g_status_685170.buffers.XChar[slot].fOccupied != 0) {
                    SetPortraitTargetPose(&gXStatus.monster_manager_entries[slot], 2);
                }
            }
            DisableMenuButtonBanks00598C70();
            RequestRedraw(0xff);
            gXStatus.fSurprisePossible = 1;
            EnableRegionInput(0x137);
            ActivateDialogRegion(0x137);
            gXStatus.surprise_deadline_turns = 0;
            CreateSurpriseFade0056B4E0();
            gXStatus.surprise_phase = 0;
            StartMusicResource0048FC10("Camping.MPL", 0, 1);
            gXStatus.combat_countdown = 0;
        }
    }

    if (gXStatus.fSurprisePossible == 0) {
        return;
    }
    if (gXStatus.surprise_unengaged != 0 && AnyCharacterEngaged() && gXStatus.surprise_phase == 1) {
        SetViewDistance(12.0f);
        SetNavigatorLinkMode00452F50(0);
        g_game_time_accumulator_6598bc->ResetDurationScale();
        ResetMonsterGeneratorTimers0048CBE0();
        ReverseSurpriseFade0056B5F0();
        gXStatus.surprise_phase = 2;
        ReleaseMarkedNpcBindings0050DA00();
        StartLevelMusic(1, 1);
    }
}

/* The Camp key/button requests the surprise-possible camp state. Camping is
   refused outright in combat and while a level vector or a level flag blocks
   it; a blocking world-cursor node or a busy screen declines silently. */
// FUNCTION: WIZ8 0x00502460
void RequestCamp00502460(void)
{
    if (gXStatus.fSurprisePossible != 0) {
        return;
    }
    if (gXStatus.fCombatMode != 0) {
        ShowNotice(0xc, gppStringList[0x774], -1, 0xffffffff, 0);
        return;
    }
    if (HasLevelDataVector() == 0 && static_cast<char>(GetLevelDataFlag4()) != 0) {
        if (DispatchWorldCursorNodeCommand004D9080(0, 3) != 0) {
            return;
        }
        if (static_cast<char>(IsScreenIdle()) == 0) {
            return;
        }
        if (gXStatus.world_update_blocked != 0) {
            ResumeMainGameWorld();
        }
        if (AnyCharacterEngaged() == false) {
            gXStatus.surprise_unengaged = 1;
            ShowNotice(0xc, gppStringList[0x794], -1, 0xffffffff, 0);
        } else {
            gXStatus.surprise_unengaged = 0;
            ShowNoticef(0xc, gppStringList[0x790], 8);
        }
        SetNpcQuoteBubbleVisible(false, 0, 0, -1, 0xffffffff);
        gXStatus.character_event_queue->CompleteAllActiveEvents();
        for (unsigned int slot = 0; slot < 8; ++slot) {
            if (g_status_685170.buffers.XChar[slot].fOccupied != 0) {
                SetPortraitTargetPose(&gXStatus.monster_manager_entries[slot], 2);
            }
        }
        DisableMenuButtonBanks00598C70();
        RequestRedraw(0xff);
        gXStatus.fSurprisePossible = 1;
        EnableRegionInput(0x137);
        ActivateDialogRegion(0x137);
        gXStatus.surprise_deadline_turns = 0;
        CreateSurpriseFade0056B4E0();
        gXStatus.surprise_phase = 0;
        StartMusicResource0048FC10("Camping.MPL", 0, 1);
        gXStatus.combat_countdown = 0;
        return;
    }
    ShowNotice(0xc, gppStringList[0x796], -1, 0xffffffff, 0);
}

/* Complete the pending character events, reset the occupied slots' portrait
   poses to the startled frame, take the menu button banks down and request a
   full redraw. The scripted ambush triggers call this to open the surprise
   sequence. */
// FUNCTION: WIZ8 0x005025F0
void BeginSurprise005025F0(void)
{
    unsigned int i;

    gXStatus.character_event_queue->CompleteAllActiveEvents();
    for (i = 0; i < W8_PARTY_SLOT_COUNT; ++i) {
        if (g_status_685170.buffers.XChar[i].fOccupied != 0) {
            SetPortraitTargetPose(&gXStatus.monster_manager_entries[i], 2);
        }
    }
    DisableMenuButtonBanks00598C70();
    RequestRedraw(0xff);
}

/* Advance the surprise fade/hold/resolve sequence while the party is locked
   into fSurprisePossible. Phase 0 waits for the fade-in helper, widens sight,
   and scales time from the level's +0x54 float; phase 1 holds until the
   world clock reaches the deadline (unless surprise started unengaged);
   phase 2 waits for the fade-out helper, then ends surprise and forwards
   combat if needed. */
// FUNCTION: WIZ8 0x00502650
void UpdateSurpriseMode(void)
{
    float scale;
    float level_scale;

    if (gXStatus.fSurprisePossible == 0) {
        return;
    }
    switch (gXStatus.surprise_phase) {
    case 0:
        if (UpdateSurpriseFade0056B6F0() != 0) {
            gXStatus.surprise_deadline_turns = g_status_685170.world_clock + 0x7080;
            DestroyUngroupedMonsters();
            SetViewDistance(2880.0f);
            SetNavigatorLinkMode00452F50(1);
            level_scale = g_level_records[g_status_685170.current_level].gameplay_time_scale_054;
            scale = g_float_005ebb38 / level_scale;
            g_game_time_accumulator_6598bc->SetDurationScale(scale);
            SetMonsterGeneratorDurationScale(scale);
            gXStatus.surprise_phase = 1;
        }
        break;
    case 1:
        if (gXStatus.surprise_unengaged == 0 &&
            static_cast<unsigned int>(g_status_685170.world_clock) >=
                gXStatus.surprise_deadline_turns) {
            SetViewDistance(12.0f);
            SetNavigatorLinkMode00452F50(0);
            g_game_time_accumulator_6598bc->ResetDurationScale();
            ResetMonsterGeneratorTimers0048CBE0();
            UpdateEnvironmentLight004834B0();
            RefreshEnvironment00483560();
            ReverseSurpriseFade0056B5F0();
            gXStatus.surprise_phase = 2;
            ReleaseMarkedNpcBindings0050DA00();
            StartLevelMusic(1, 1);
        }
        break;
    case 2:
        if (UpdateSurpriseFade0056B6F0() != 0) {
            EndSurprise00502860();
            if (gXStatus.fCombatMode != 0) {
                MonsterForward453160();
            }
        }
        break;
    }
}

/* The cancel command while surprise runs: while the ambush is unengaged and
   the party is out of combat it reposts the surprise notice; once the fade
   has reached phase 1 it performs the same restore as UpdateSurpriseMode's
   deadline path - without the environment-light refresh - and moves the
   sequence to phase 2. */
// FUNCTION: WIZ8 0x00502790
void AcknowledgeSurprise00502790(void)
{
    if (gXStatus.surprise_unengaged != 0 && gXStatus.fCombatMode == 0) {
        ShowNotice(0xc, gppStringList[0x794], -1, 0xffffffff, 0);
        return;
    }
    if (gXStatus.surprise_phase == 1) {
        SetViewDistance(12.0f);
        SetNavigatorLinkMode00452F50(0);
        g_game_time_accumulator_6598bc->ResetDurationScale();
        ResetMonsterGeneratorTimers0048CBE0();
        ReverseSurpriseFade0056B5F0();
        gXStatus.surprise_phase = 2;
        ReleaseMarkedNpcBindings0050DA00();
        StartLevelMusic(1, 1);
    }
}

/* While a surprise sequence is holding, end it for combat: restore the view,
   navigator and time-scale overrides, reverse the fade and restart the level
   music. */
// FUNCTION: WIZ8 0x00502810
void ResolveSurpriseHold00502810(void)
{
    if (gXStatus.surprise_phase == 1) {
        SetViewDistance(12.0f);
        SetNavigatorLinkMode00452F50(0);
        g_game_time_accumulator_6598bc->ResetDurationScale();
        ResetMonsterGeneratorTimers0048CBE0();
        ReverseSurpriseFade0056B5F0();
        gXStatus.surprise_phase = 2;
        ReleaseMarkedNpcBindings0050DA00();
        StartLevelMusic(1, 1);
    }
}

/* End the surprise sequence: post the outcome notice and, if the condition-13
   rest event armed condition13_clock_2487 more than a world-clock day ago, clear the
   condition and queue the rest-benefit event for that character. */
// FUNCTION: WIZ8 0x00502860
void EndSurprise00502860(void)
{
    gXStatus.fSurprisePossible = 0;
    ResolveSurpriseWake005029E0();
    ClearActiveRegionIfMatches(0x137);
    DisableRegionInput(0x137);

    const wchar_t* text;
    if (gXStatus.surprise_unengaged == 0) {
        if (gXStatus.surprise_deadline_turns == 0) {
            text = gppStringList[0x793];
        } else if (gXStatus.surprise_deadline_turns <
                   static_cast<unsigned int>(g_status_685170.world_clock)) {
            text = gppStringList[0x791];
        } else {
            text = gppStringList[0x792];
        }
    } else {
        gXStatus.surprise_unengaged = 0;
        text = AnyCharacterEngaged() ? gppStringList[0x795] : gppStringList[0x792];
    }
    ShowNotice(0xc, text, -1, 0xffffffff, 0);

    if (g_status_685170.condition13_clock_2487 != 0 &&
        0x15180 < static_cast<unsigned int>(g_status_685170.world_clock) -
                      g_status_685170.condition13_clock_2487) {
        g_status_685170.condition13_clock_2487 = 0;
        g_status_685170.skip_next_condition_reaction = 1;
        int party_slot = g_status_685170.pending_condition_party_slot_248f;
        RemoveCharacterCondition(party_slot, 0x13, 0);
        QueueCharacterEvent(&g_status_685170.buffers.Char[party_slot], g_effect_005ee658, 0,
                            g_effect_argument_005ed8c8, g_effect_argument_005ed914);
        SetFact(0xb6, 1, 0);
    }
}

/* Tear down the world-state overrides the surprise sequence installed:
   restore default view distance and navigator link mode, reset the time
   scale, restart the monster generator timers and release the fade overlay. */
// FUNCTION: WIZ8 0x005029a0
void RestoreSurpriseView005029A0(void)
{
    gXStatus.fSurprisePossible = 0;
    gXStatus.surprise_unengaged = 0;
    SetViewDistance(12.0f);
    SetNavigatorLinkMode00452F50(0);
    g_game_time_accumulator_6598bc->ResetDurationScale();
    ResetMonsterGeneratorTimers0048CBE0();
    DestroySurpriseFade0056B690();
}

/* The camp-wake resolution inside the surprise teardown: each surviving
   character rolls against their senses attribute - failure applies the
   groggy condition, success posts a notice. Then every occupied portrait is
   posed awake with a randomized idle clock and the button banks re-enable. */
// FUNCTION: WIZ8 0x005029e0
void ResolveSurpriseWake005029E0(void)
{
    if (g_combat_state != 0 && g_combat_state->party_surprised_a52 != 0) {
        for (unsigned int slot = 0; slot < 8; ++slot) {
            W8Character* character = &g_status_685170.buffers.Char[slot];
            if (g_status_685170.buffers.XChar[slot].fOccupied == 0 ||
                character->highest_condition >= 0x12) {
                continue;
            }
            int roll = static_cast<int>(Random(100)) - 0x14 -
                       static_cast<int>(character->attributes[6].effective * 0x46 / 100);
            if (roll < 1) {
                PostCharacterNotice(slot, gppStringList[0x243],
                                    gppStringList[g_condition_notices_0061E570[60]]);
            } else {
                SetCharacterCondition(slot, 0xf, roll / 0x1e + 1, 0, 0, 0);
            }
        }
    }

    for (unsigned int slot = 0; slot < 8; ++slot) {
        if (g_status_685170.buffers.XChar[slot].fOccupied == 0) {
            continue;
        }
        W8MonsterManagerEntry* entry = &gXStatus.monster_manager_entries[slot];
        SetPortraitTargetPose(entry, 1);
        if (Random(2) != 0) {
            entry->portrait_idle_clock = SetCountdownClock(Random(5000) + 5000);
        }
        entry->portrait_pose_dirty = 1;
        RequestRedraw(1 << slot);
    }
    EnableMenuButtonBanks00598C10();
}

/* 0x00502B50: the hit-point, stamina and per-realm spell regeneration rates
   are one tick of the pool ceiling's share, twenty points of base and a
   twelveth of a minute each; the modifier block's three regeneration-boost flags
   raise their rate by half. */
// FUNCTION: WIZ8 0x00502b50
void RebuildCharacterRegenRates00502B50(W8Character* character)
{
    float rate;
    int realm;

    rate = (character->uiHPMax * 0.4f + 20.0f) * 0.0041666669f;
    character->health_regen_rate_0b69 = rate;
    if (character->bonus_1770.boost_health_regen != 0) {
        character->health_regen_rate_0b69 = rate * 1.5f;
    }

    rate = (character->uiStaminaMax * 0.9f + 20.0f) * 0.0041666669f;
    character->stamina_regen_rate_0b71 = rate;
    if (character->bonus_1770.boost_stamina_regen != 0) {
        character->stamina_regen_rate_0b71 = rate * 1.5f;
    }

    for (realm = 0; realm < W8_SPELL_REALM_COUNT; ++realm) {
        if (character->sp_max[realm] == 0) {
            character->spell_regen_rates_0b79[realm * 2] = 0.0f;
            continue;
        }
        rate = ((float)character->sp_max[realm] * 0.65f + 20.0f) * 0.0041666669f;
        character->spell_regen_rates_0b79[realm * 2] = rate;
        if (character->bonus_1770.boost_spell_regen != 0) {
            character->spell_regen_rates_0b79[realm * 2] = rate * 1.5f;
        }
    }
}

/* The monster counterpart of the character regen rebuild: the same pool-share
   rates plus the modifier block's two regen channels, raised by half while the corresponding
   regeneration-boost flags are set. */
// FUNCTION: WIZ8 0x00502c50
void RebuildMonsterRegenRates00502C50(W8MonsterInfo* monster_info)
{
    float rate;

    rate = (static_cast<unsigned int>(monster_info->uiHPMax) * g_navigator_mode3_scale_005ebca4 +
            g_monster_record_float_scale) *
               0.0041666669f +
           monster_info->modifiers_1db.health_regen_adjustment;
    monster_info->hp_regen_rate_47 = rate;
    if (monster_info->modifiers_1db.boost_health_regen != 0) {
        monster_info->hp_regen_rate_47 = rate * g_float_005ec3b8;
    }

    rate = (static_cast<float>(static_cast<unsigned int>(monster_info->stamina_max)) *
                g_float_005ec390 +
            g_monster_record_float_scale) *
               0.0041666669f +
           monster_info->modifiers_1db.stamina_regen_adjustment;
    monster_info->stamina_regen_rate_4f = rate;
    if (monster_info->modifiers_1db.boost_stamina_regen != 0) {
        monster_info->stamina_regen_rate_4f = rate * g_float_005ec3b8;
    }
}

/* The 120-second aging tick: advances the wait-state machine while the
   real/frame elapsed accumulators hold at zero, ages each living character
   and monster, expires party and combat effect slots with expiry notices,
   ticks queued spell effects once per minute and runs the NPC-side passes. */
// FUNCTION: WIZ8 0x00502d00
void AdvanceTimedEffects00502D00(unsigned int minutes)
{
    if (g_status_685170.real_elapsed_2391 + g_status_685170.frame_elapsed_2395 ==
        g_float_005ebb34) {
        if (g_status_685170.wait_state_2399 == 1 || g_status_685170.wait_state_2399 == 0) {
            g_status_685170.wait_state_2399 = 2;
            for (unsigned int slot = 0; slot < 8; ++slot) {
                g_status_685170.buffers.XChar[slot].movement_fatigue = 0;
            }
        } else if (g_status_685170.wait_state_2399 == 2) {
            g_status_685170.wait_state_2399 = 3;
        }
    } else {
        g_status_685170.wait_state_2399 =
            g_status_685170.real_elapsed_2391 != g_float_005ebb34 ? 1 : 0;
        g_status_685170.real_elapsed_2391 = g_float_005ebb34;
        g_status_685170.frame_elapsed_2395 = g_float_005ebb34;
    }

    for (unsigned int slot = 0; slot < 8; ++slot) {
        W8Character* character = &g_status_685170.buffers.Char[slot];
        if (g_status_685170.buffers.XChar[slot].fOccupied != 0 &&
            (character->highest_condition < 0x12 ||
             (character->uiCondition[0x12] == 0 && GetConditionRecordFlag(slot, 1) != 0))) {
            GameTurnsPassedChar00503100(slot, minutes);
        }
    }

    if (gXStatus.fCombatMode != 0) {
        W8CombatSlot target;
        ResetCombatSlot(&target);
        target.iType = W8_TARGET_KIND_PARTY;
        TickCombatEffectSlots(g_combat_state->effect_slots, &target);
        TickRadiusBlastEffectSlots(g_combat_state->effect_slots_85a);
    }

    for (unsigned int index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(index);
        if (monster_info->highest_condition < 0x12) {
            AgeMonsterSight(monster_info, minutes, 0);
        }
    }

    bool party_changed = false;
    bool combat_changed = false;
    unsigned int i;
    for (i = 0; i < 12; ++i) {
        W8EffectSlot* slot = &g_status_685170.effect_slots_17af[i];
        if (slot->active == 0) {
            continue;
        }
        if (minutes < slot->duration_0d) {
            slot->duration_0d -= minutes;
            continue;
        }
        ShowNoticef(0xc, gppStringList[0x1b4], g_spell_records[slot->effect_id].display_name);
        if (GetViewDistance() != g_encounter_culling_rate) {
            SoundPlay("Data\\Sound\\Misc\\Spell Expiry.wav", 0);
        }
        ResetPartyEffectBlock(slot);
        party_changed = true;
    }

    if (gXStatus.fCombatMode != 0) {
        for (i = 0; i < 9; ++i) {
            W8EffectSlot* slot = &g_combat_state->effect_slots[i];
            if (slot->active == 0) {
                continue;
            }
            if (minutes < slot->duration_0d) {
                slot->duration_0d -= minutes;
                continue;
            }
            ShowNoticef(0xc, gppStringList[0x1b4], g_spell_records[slot->effect_id].display_name);
            if (GetViewDistance() != g_encounter_culling_rate) {
                SoundPlay("Data\\Sound\\Misc\\Spell Expiry.wav", 0);
            }
            ResetPartyEffectBlock(slot);
            combat_changed = true;
        }
        for (i = 0; i < 6; ++i) {
            W8EffectSlot* slot = &g_combat_state->effect_slots_85a[i];
            if (slot->active == 0) {
                continue;
            }
            if (minutes < slot->duration_0d) {
                slot->duration_0d -= minutes;
                continue;
            }
            ShowNoticef(0xc, gppStringList[0x1b4], g_spell_records[slot->effect_id].display_name);
            if (GetViewDistance() != g_encounter_culling_rate) {
                SoundPlay("Data\\Sound\\Misc\\Spell Expiry.wav", 0);
            }
            ResetPartyEffectBlock(slot);
            combat_changed = true;
        }
    }

    if (party_changed) {
        RequestRedraw(0x100);
    }
    if (combat_changed) {
        RequestRedraw(0x8000);
    }
    for (unsigned int remaining = minutes; remaining != 0; --remaining) {
        TickSpellEffects();
    }
    W8SpellEffectEntry* control = FindMonsterControlSpellEffect();
    if (control != 0 && control->turns_remaining != 0) {
        ApplyMonsterControlToNearbyMonsters(control);
    }
    if (party_changed || combat_changed) {
        SoundPlay("Data\\Sound\\Misc\\GeneralMagic.wav", 0);
    }
    if (gXStatus.fSurprisePossible == 0) {
        DetectMonsterGroups004E4AB0();
    }
    AdvanceNpcTimers0050C7D0(minutes * 10);
    ProcessNpcPendingEvents0050CA80();
}

/* Advance one character's whole aging cycle by the elapsed minutes: the timed
   damage/heal modifier bytes, the disease deterioration rolls, the
   item-0x243/0x239 madness pair, the fractional health/stamina/spell-point
   regeneration accumulators, and the finite condition and enchantment
   countdowns. */
// FUNCTION: WIZ8 0x00503100
void GameTurnsPassedChar00503100(int party_slot, unsigned int minutes)
{
    W8Character* character = &g_status_685170.buffers.Char[party_slot];
    bool diseased = false;
    unsigned int realm;

    unsigned int damage = character->bonus_1770.damage_per_minute;
    if (damage != 0) {
        damage *= minutes;
        if (g_status_685170.wait_state_2399 != 3 || gXStatus.fCombatMode != 0) {
            damage += damage >> 1;
        }
        ApplyDamageToCharacter(party_slot, damage, 1, 1, 0, static_cast<W8SpellEffectResult*>(0),
                               0);
    }

    if (character->uiCondition[W8_CONDITION_DISEASED] != 0) {
        diseased = true;
        if (Random(character->attributes[W8_ATTRIBUTE_VITALITY].effective * 10) < minutes) {
            switch (Random(4)) {
            case 0: {
                unsigned int condition;
                switch (Random(5)) {
                case 0:
                    condition = W8_CONDITION_IRRITATED;
                    break;
                case 1:
                    condition = W8_CONDITION_NAUSEATED;
                    break;
                case 2:
                    condition = W8_CONDITION_SLOWED;
                    break;
                case 3:
                    condition = W8_CONDITION_BLIND;
                    break;
                case 4:
                    condition = W8_CONDITION_INSANE;
                    break;
                case 5:
                    condition = W8_CONDITION_PARALYZED;
                    break;
                default:
                    srAssertFail("FALSE", GAMEPLAYTIME_CPP, 0x333,
                                 "GameTurnsPassedChar: ERROR - Invalid condition");
                    condition = minutes;
                    break;
                }
                SetCharacterCondition(party_slot, condition, W8_CONDITION_INDEFINITE, 0, 0, 1);
                break;
            }
            case 1: {
                unsigned int attribute = Random(7);
                if (1 < character->attributes[attribute].value) {
                    character->attributes[attribute].value =
                        character->attributes[attribute].value - 1;
                    ++character->attributes[attribute].change_counter_08;
                    ApplyAttributeChange(character, attribute);
                    PostCharacterNotice(
                        party_slot, gppStringList[0x270],
                        gppStringList[g_character_description_first_ids_61e3a4[attribute]]);
                }
                break;
            }
            case 2:
                if (static_cast<unsigned int>(character->uiHPMax) < 5) {
                    break;
                }
                {
                    unsigned int loss = Random(4) + 1;
                    character->uiHPMax -= loss;
                    character->hp_adjustment -= loss;
                    if (loss == 1) {
                        PostCharacterNotice(party_slot, gppStringList[0x26e]);
                    } else {
                        PostCharacterNotice(party_slot, gppStringList[0x26f], loss);
                    }
                    unsigned int current = character->hp_current;
                    if (1 < current) {
                        if (loss >= current - 1) {
                            loss = current - 1;
                        }
                        character->hp_current = current - loss;
                    }
                    RequestPartySlotRedraw(party_slot);
                }
                break;
            case 3:
                if (static_cast<unsigned int>(character->uiStaminaMax) < 5) {
                    break;
                }
                {
                    unsigned int loss = Random(4) + 1;
                    character->uiStaminaMax -= loss;
                    character->fatigue_penalty_0b21 += loss;
                    PostCharacterNotice(party_slot, gppStringList[0x271], loss);
                    int stamina = character->stamina;
                    if (1 < stamina) {
                        if (loss >= stamina - 1U) {
                            loss = stamina - 1U;
                        }
                        character->stamina = stamina - loss;
                    }
                    RequestPartySlotRedraw(party_slot);
                }
                break;
            }
        }
    }

    if (character->uiCondition[W8_CONDITION_INFATUATED] != 0) {
        if (GetLevelBand(g_status_685170.current_level) == 9 ||
            GetLevelBand(g_status_685170.current_level) == 0xa) {
            if (character->uiCondition[W8_CONDITION_HEXED] == W8_CONDITION_INDEFINITE) {
                RemoveCharacterCondition(party_slot, W8_CONDITION_HEXED, 1);
            }
        } else {
            unsigned int hits = 0;
            for (unsigned int roll = minutes; roll != 0; --roll) {
                if (Random(4) == 0) {
                    ++hits;
                }
            }
            if (hits != 0) {
                ApplyDamageToCharacter(party_slot, hits, 0, 1, 0,
                                       static_cast<W8SpellEffectResult*>(0), 0);
            }
            if (character->uiCondition[W8_CONDITION_HEXED] == 0) {
                SetCharacterCondition(party_slot, W8_CONDITION_HEXED, W8_CONDITION_INDEFINITE, 0, 0,
                                      1);
            }
        }
    }

    W8ItemInstance* found;
    W8Character* holder;
    if (FindItemOnParty(0x243, &found, &holder, 2, static_cast<W8ItemInstance*>(0)) != 0 &&
        found != &g_status_685170.item_in_hand_235b) {
        if (holder == static_cast<W8Character*>(0) ||
            FindItemOnCharacter(holder, 0x239, static_cast<W8ItemInstance**>(0), 0,
                                static_cast<W8ItemInstance*>(0)) == 0) {
            if (character->uiCondition[W8_CONDITION_INSANE] < W8_CONDITION_INDEFINITE) {
                SetCharacterCondition(party_slot, W8_CONDITION_INSANE, W8_CONDITION_INDEFINITE, 0,
                                      0, 1);
            }
        } else if (character->uiCondition[W8_CONDITION_INSANE] == W8_CONDITION_INDEFINITE) {
            RemoveCharacterCondition(party_slot, W8_CONDITION_INSANE, 1);
        }
    }

    signed char health_mod = character->bonus_1770.health_regen_adjustment;
    if (health_mod > 0) {
        if (character->hp_current < static_cast<unsigned int>(character->uiHPMax)) {
            HealCharacter(party_slot, static_cast<int>(health_mod) * static_cast<int>(minutes), 0);
        }
    } else if (health_mod < 0) {
        ApplyDamageToCharacter(party_slot,
                               -static_cast<int>(health_mod) * static_cast<int>(minutes), 0, 1, 0,
                               static_cast<W8SpellEffectResult*>(0), 0);
    }

    signed char stamina_mod = character->bonus_1770.stamina_regen_adjustment;
    if (stamina_mod > 0) {
        if (character->stamina < character->uiStaminaMax) {
            RestoreCharacterStamina(party_slot, stamina_mod * static_cast<int>(minutes), 0);
        }
    } else if (stamina_mod < 0) {
        FatigueCharacter(party_slot, -static_cast<int>(stamina_mod * minutes), 0,
                         static_cast<W8SpellEffectResult*>(0));
    }

    for (realm = 0; realm < W8_SPELL_REALM_COUNT; ++realm) {
        signed char spell_mod = character->bonus_1770.spell_regen_adjustment;
        if (spell_mod > 0) {
            if (character->iSPLeft[realm] < character->sp_max[realm]) {
                RestoreCharacterRealmSpellPoints(
                    party_slot, realm, static_cast<int>(spell_mod) * static_cast<int>(minutes));
            }
        } else if (spell_mod < 0) {
            DrainCharacterRealmSpellPoints(party_slot, realm, -static_cast<int>(spell_mod), 1);
        }
    }

    float health_scale;
    if (gXStatus.fSurprisePossible != 0) {
        health_scale = g_float_005ebb38;
    } else if (g_status_685170.wait_state_2399 == 3 && gXStatus.fCombatMode == 0) {
        health_scale = g_float_005ebc7c;
    } else {
        health_scale = g_float_005ebb34;
    }
    if (diseased) {
        health_scale *= g_navigator_vertical_phase_step_005ebcc8;
    }
    float spell_scale = health_scale;
    if (health_scale < g_float_005ebc7c) {
        spell_scale = 0.5f;
    }
    float stamina_scale = 0.0f;

    if (CharacterHasTrait00547940(character, W8_TRAIT_HEALTH_REGENERATION) != 0) {
        if (health_scale == g_float_005ebb34) {
            health_scale = ScaleValueByProfessionLevel005479B0(
                               character, W8_TRAIT_HEALTH_REGENERATION, 16.67f) *
                           g_float_005ebc7c;
        } else {
            health_scale = ScaleValueByProfessionLevel005479B0(
                               character, W8_TRAIT_HEALTH_REGENERATION, 16.67f) *
                           health_scale;
        }
    }
    if (CharacterHasTrait00547940(character, W8_TRAIT_STAMINA_REGENERATION) != 0 &&
        gXStatus.fCombatMode != 0) {
        stamina_scale =
            ScaleValueByProfessionLevel005479B0(character, W8_TRAIT_STAMINA_REGENERATION, 3.3f);
    }
    if (CharacterHasTrait00547940(character, 0x1a) != 0 && spell_scale > g_float_005ebb34) {
        spell_scale *= g_float_005ec340;
    }
    if (CharacterHasTrait00547940(character, W8_TRAIT_LIZARDMAN_SLOW_MAGIC_RECOVERY) != 0 &&
        spell_scale > g_float_005ebb34) {
        spell_scale *= g_float_005ebccc;
    }

    if (health_scale > g_float_005ebb34 &&
        character->hp_current < static_cast<unsigned int>(character->uiHPMax)) {
        character->health_regen_accumulator_0b6d =
            minutes * character->health_regen_rate_0b69 * health_scale +
            character->health_regen_accumulator_0b6d;
        HealCharacter(party_slot, static_cast<int>(character->health_regen_accumulator_0b6d), 0);
        character->health_regen_accumulator_0b6d =
            character->health_regen_accumulator_0b6d -
            static_cast<unsigned int>(character->health_regen_accumulator_0b6d);
    }
    if (stamina_scale > g_float_005ebb34 && character->stamina < character->uiStaminaMax) {
        character->stamina_regen_accumulator_0b75 =
            minutes * character->stamina_regen_rate_0b71 * stamina_scale +
            character->stamina_regen_accumulator_0b75;
        RestoreCharacterStamina(party_slot,
                                static_cast<int>(character->stamina_regen_accumulator_0b75), 0);
        character->stamina_regen_accumulator_0b75 =
            character->stamina_regen_accumulator_0b75 -
            static_cast<unsigned int>(character->stamina_regen_accumulator_0b75);
    }
    if (spell_scale > g_float_005ebb34) {
        for (realm = 0; realm < W8_SPELL_REALM_COUNT; ++realm) {
            if (character->iSPLeft[realm] < character->sp_max[realm]) {
                character->spell_regen_rates_0b79[realm * 2 + 1] =
                    minutes * character->spell_regen_rates_0b79[realm * 2] * spell_scale +
                    character->spell_regen_rates_0b79[realm * 2 + 1];
                RestoreCharacterRealmSpellPoints(
                    party_slot, realm,
                    static_cast<int>(character->spell_regen_rates_0b79[realm * 2 + 1]));
                character->spell_regen_rates_0b79[realm * 2 + 1] =
                    character->spell_regen_rates_0b79[realm * 2 + 1] -
                    static_cast<unsigned int>(character->spell_regen_rates_0b79[realm * 2 + 1]);
            }
        }
    }

    if (character->hp_current >= static_cast<unsigned int>(character->uiHPMax)) {
        character->health_regen_accumulator_0b6d = 0.0f;
    }
    if (character->stamina >= character->uiStaminaMax) {
        character->stamina_regen_accumulator_0b75 = 0.0f;
    }
    for (realm = 0; realm < W8_SPELL_REALM_COUNT; ++realm) {
        if (character->iSPLeft[realm] >= character->sp_max[realm]) {
            character->spell_regen_rates_0b79[realm * 2 + 1] = 0.0f;
        }
    }

    for (unsigned int condition = 0; condition < W8_CONDITION_COUNT; ++condition) {
        if (character->uiCondition[condition] != 0 &&
            character->uiCondition[condition] < W8_CONDITION_INDEFINITE) {
            TickCharacterCondition(party_slot, condition, minutes);
        }
    }
    for (unsigned int slot = 0; slot < 8; ++slot) {
        unsigned int turns = character->enchantments[slot].turns_08;
        if (turns != 0 && turns < W8_CONDITION_INDEFINITE) {
            if (turns > minutes) {
                character->enchantments[slot].turns_08 = turns - minutes;
            } else {
                ClearCharacterEnchantmentSlot(party_slot, slot);
            }
        }
    }

    if (CharacterHasTrait00547940(character, W8_TRAIT_MAKE_POTIONS) != 0) {
        if (gXStatus.fSurprisePossible != 0) {
            if (character->potion_brew_cooldown_0b65 == 0) {
                BrewAlchemistPotion00548E60(character);
            }
        } else if (character->potion_brew_cooldown_0b65 != 0) {
            if (minutes >= character->potion_brew_cooldown_0b65) {
                character->potion_brew_cooldown_0b65 = 0;
            } else {
                character->potion_brew_cooldown_0b65 =
                    character->potion_brew_cooldown_0b65 - minutes;
            }
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
        W8CombatSlot target;
        target.iType = static_cast<W8TargetKind>(3);
        target.iChar = monster_info->location_id;
        TickCombatEffectSlots(monster_info->pCombat->effect_slots_3e, &target);
        goto after_early;
    }
    monster = monster_info->p3D;
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

                MoveMonsterGroupToPosition(group, &notify_position, monster->GetYaw(), 0, 0, 0, 0);
                for (int index = 0; index < 4; ++index) {
                    if (group->allied_group_ids[index] != 0) {
                        group_index = GetMonsterGroupIndexByID(0x4d9, GAMEPLAYTIME_CPP,
                                                               group->allied_group_ids[index], 1);
                        group = GetMonsterGroupByListIndex(group_index);
                        MoveMonsterGroupToPosition(group, &notify_position, monster->GetYaw(), 0, 0,
                                                   0, 0);
                    }
                }
            }
        }
        goto after_early;
    }
    if (GetViewDistance() != g_sight_default_005ec254) {
        goto after_early;
    }
    if ((monster->linked_navigator_05c == 0 && monster->halted_025 == 0) &&
        (static_cast<signed char>(monster_info->movement_stall_ticks_254) > 1 ||
         monster->movement_stopped_024 == 0)) {
        srVector3T<float> location;
        srVector3T<float> previous;
        srVector3T<float> delta;
        unsigned char cycle;

        MonsterGetLocation(monster, &location);
        previous.x = static_cast<float>(monster_info->movement_watch_position[0]);
        previous.y = static_cast<float>(monster_info->movement_watch_position[1]);
        previous.z = static_cast<float>(monster_info->movement_watch_position[2]);
        monster_info->position_17.y = location.y;
        cycle = monster_info->movement_stall_ticks_254;
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
                        monster_info->party_threat.visible_to_player_25 == 0) {
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

                                MoveMonsterGroupToPosition(group, &notify_position,
                                                           monster->GetYaw(), 0, 0, 0, 0);
                                for (int index = 0; index < 4; ++index) {
                                    if (group->allied_group_ids[index] != 0) {
                                        group_index = GetMonsterGroupIndexByID(
                                            0x516, GAMEPLAYTIME_CPP, group->allied_group_ids[index],
                                            1);
                                        group = GetMonsterGroupByListIndex(group_index);
                                        MoveMonsterGroupToPosition(group, &notify_position,
                                                                   monster->GetYaw(), 0, 0, 0, 0);
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

                            MoveMonsterGroupToPosition(group, &notify_position, monster->GetYaw(),
                                                       0, 0, 0, 0);
                            for (int index = 0; index < 4; ++index) {
                                if (group->allied_group_ids[index] != 0) {
                                    group_index = GetMonsterGroupIndexByID(
                                        0x542, GAMEPLAYTIME_CPP, group->allied_group_ids[index], 1);
                                    group = GetMonsterGroupByListIndex(group_index);
                                    MoveMonsterGroupToPosition(group, &notify_position,
                                                               monster->GetYaw(), 0, 0, 0, 0);
                                }
                            }
                        }
                        cycle_cleared = true;
                        flags_cleared = true;
                    }
                }
            }
            if (cycle_cleared) {
                monster_info->movement_stall_ticks_254 = 0;
            }
            if (flags_cleared) {
                monster_info->ai_mode_255 = 0;
                monster_info->pathing_cooldown_246 = 0;
            }
        }
        if (static_cast<signed char>(monster_info->movement_stall_ticks_254) > 0) {
            ++monster_info->movement_stall_ticks_254;
        }
        monster_info->movement_watch_position[0] = static_cast<int>(monster_info->position_17.x);
        monster_info->movement_watch_position[1] = static_cast<int>(monster_info->position_17.y);
        monster_info->movement_watch_position[2] = static_cast<int>(monster_info->position_17.z);
    }

after_early: {
    unsigned int amount = monster_info->modifiers_1db.damage_per_minute;

    if (amount != 0) {
        W8TargetSource source;

        amount *= minutes;
        if (monster_info->fInCombat != 0) {
            amount += amount >> 1;
        }
        ResetTargetSource(&source);
        ApplyDamageToMonster(monster_info, amount, &source, 1, gXStatus.fCombatMode, 0, 0, 0);
    }
}
    if (monster_info->uiCondition[2] != 0) {
        frost_condition = true;
    }
    {
        W8MonsterRecord* data = GetMonsterDataForInfo(monster_info);
        int amount = (static_cast<int>(data->hp_regeneration_17c) +
                      monster_info->modifiers_1db.health_regen_adjustment) *
                     static_cast<int>(minutes);

        if (amount < 1) {
            if (amount < 0) {
                W8TargetSource source;

                ResetTargetSource(&source);
                ApplyDamageToMonster(monster_info, -amount, &source, 0, 0, 0, 0, 0);
            }
        } else if (monster_info->hp_current < static_cast<unsigned int>(monster_info->uiHPMax)) {
            HealMonster(monster_info, amount, 0);
        }
    }
    {
        int amount = (monster_info->modifiers_1db.stamina_regen_adjustment +
                      static_cast<int>(record->stamina_regeneration_0ce)) *
                     static_cast<int>(minutes);

        if (amount < 1) {
            if (amount < 0) {
                FatigueMonster(monster_info, -amount, 0);
            }
        } else if (monster_info->stamina < monster_info->stamina_max) {
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
            if (monster_info->hp_current < static_cast<unsigned int>(monster_info->uiHPMax)) {
                monster_info->hp_regen_accumulator_4b =
                    minutes * monster_info->hp_regen_rate_47 * heal_scale +
                    monster_info->hp_regen_accumulator_4b;
                int healed = static_cast<int>(monster_info->hp_regen_accumulator_4b);

                HealMonster(monster_info, healed, 0);
                monster_info->hp_regen_accumulator_4b -= static_cast<float>(healed);
            }
            if (monster_info->stamina < monster_info->stamina_max) {
                monster_info->stamina_regen_accumulator_53 =
                    minutes * monster_info->stamina_regen_rate_4f * heal_scale +
                    monster_info->stamina_regen_accumulator_53;
                int restored = static_cast<int>(monster_info->stamina_regen_accumulator_53);

                RestoreMonsterStamina(monster_info, restored, 0);
                monster_info->stamina_regen_accumulator_53 -= static_cast<float>(restored);
            }
        }
    }
    for (int condition = 0; condition < 0x14; ++condition) {
        if (monster_info->uiCondition[condition] != 0 &&
            monster_info->uiCondition[condition] < 9999) {
            TickMonsterCondition(monster_info->location_id, condition, minutes);
        }
    }
    for (int enchant = 0; enchant < 8; ++enchant) {
        float remaining = static_cast<float>(monster_info->enchantments[enchant].turns_08);

        if (remaining != 0.0f && static_cast<unsigned int>(remaining) < 9999) {
            if (minutes < static_cast<unsigned int>(remaining)) {
                monster_info->enchantments[enchant].turns_08 =
                    static_cast<int>(remaining) - static_cast<int>(minutes);
            } else {
                ClearMonsterEnchantmentSlot(monster_info->location_id, enchant);
            }
        }
    }
    if (static_cast<unsigned int>(monster_info->uiHPMax) <= monster_info->hp_current) {
        monster_info->hp_regen_accumulator_4b = 0.0f;
    }
    if (monster_info->stamina_max <= monster_info->stamina) {
        monster_info->stamina_regen_accumulator_53 = 0.0f;
    }
    {
        W8EffectSlot* slot = monster_info->effect_slots_10f;

        for (int owned_index = 0; owned_index < 0xc; ++owned_index) {
            if (slot->active != 0) {
                if (minutes < slot->duration_0d) {
                    slot->duration_0d -= minutes;
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
            W8EffectSlot* slot = &monster_info->pCombat->effect_slots_3e[combat_index];

            if (slot->active != 0) {
                if (minutes < slot->duration_0d) {
                    slot->duration_0d -= minutes;
                } else if (monster_info == 0) {
                    ResetPartyEffectBlock(slot);
                } else {
                    ClearEffectSlot(monster_info, slot);
                }
            }
        }
        for (int d7_index = 0; d7_index < 6; ++d7_index) {
            W8EffectSlot* slot = &monster_info->pCombat->effect_slots_d7[d7_index];

            if (slot->active != 0) {
                if (minutes < slot->duration_0d) {
                    slot->duration_0d -= minutes;
                } else if (monster_info == 0) {
                    ResetPartyEffectBlock(slot);
                } else {
                    ClearEffectSlot(monster_info, slot);
                }
            }
        }
    }
}

/* The camping fatigue tick: while a camp is active each character without
   the rest item rolls fatigue dice whose sides grow with camp_fatigue_count;
   stamina absorbs the roll first, overflow becomes damage. The first roll
   raises the fatigued flag and posts the notice once. */
// FUNCTION: WIZ8 0x005044d0
void UpdateCampFatigue005044D0(int ticks)
{
    if (g_status_685170.world_suspended_2390 != 0) {
        return;
    }
    bool any_rolled = false;
    for (unsigned int slot = 0; slot < 8; ++slot) {
        W8Character* character = &g_status_685170.buffers.Char[slot];
        if (g_status_685170.buffers.XChar[slot].fOccupied != 0 && character->hp_current != 0 &&
            character->highest_condition < 0x12 && character->iRace != 0xf &&
            FindItemOnCharacter(character, 0x1e5, static_cast<W8ItemInstance**>(0), 0,
                                static_cast<W8ItemInstance*>(0)) == 0) {
            W8Dice dice;
            dice.count = static_cast<unsigned char>(ticks);
            dice.sides =
                static_cast<unsigned char>(g_status_685170.camp_fatigue_count_2498 / 6) + 2;
            any_rolled = true;
            dice.base = 0;
            unsigned int amount = static_cast<unsigned int>(RollDice(&dice));
            if (amount != 0) {
                int stamina = character->stamina;
                if (stamina < static_cast<int>(amount)) {
                    if (stamina > 0) {
                        amount -= stamina;
                        FatigueCharacter(slot, stamina, 0, static_cast<W8SpellEffectResult*>(0));
                    }
                    char announce =
                        gXStatus.fCombatMode == 0 || g_settings_6850c8.verbose_combat_messages == 0
                            ? 0
                            : 1;
                    ApplyDamageToCharacter(slot, amount, 0, announce, 0,
                                           static_cast<W8SpellEffectResult*>(0), 0);
                } else {
                    FatigueCharacter(slot, static_cast<int>(amount), 0,
                                     static_cast<W8SpellEffectResult*>(0));
                }
            }
            if (g_status_685170.party_fatigued_2433 == 0) {
                g_status_685170.party_fatigued_2433 = 1;
                ShowNotice(8, gppStringList[0x1da], -1, 0xffffffff, 0);
            }
        }
    }
    if (any_rolled) {
        g_status_685170.camp_fatigue_count_2498 += ticks;
        return;
    }
    g_status_685170.camp_fatigue_count_2498 = 0;
    g_status_685170.party_fatigued_2433 = 0;
    g_status_685170.camp_tick_ms_2436 = 0;
}

/* The stamina-tick driver: refreshes the wait state from the level's
   rest flags, then ticks each eligible character. While the party is
   fatigued, characters lacking the rest item are skipped. */
// FUNCTION: WIZ8 0x00504670
void UpdatePartyStamina00504670(int ticks)
{
    if (static_cast<char>(GetLevelDataFlag8()) != 0) {
        g_status_685170.wait_state_2399 = 1;
    } else {
        g_status_685170.wait_state_2399 = static_cast<char>(GetLevelDataFlag9()) != 0 ? 0 : 3;
    }

    for (unsigned int slot = 0; slot < 8; ++slot) {
        if (g_status_685170.buffers.XChar[slot].fOccupied == 0) {
            continue;
        }
        W8Character* character = &g_status_685170.buffers.Char[slot];
        if (character->highest_condition >= 0x12 &&
            (character->uiCondition[0x12] != 0 || GetConditionRecordFlag(slot, 1) == 0)) {
            continue;
        }
        if (g_status_685170.party_fatigued_2433 != 0 &&
            FindItemOnCharacter(character, 0x1e5, static_cast<W8ItemInstance**>(0), 0,
                                static_cast<W8ItemInstance*>(0)) == 0) {
            continue;
        }
        RegenCharacterStamina00504730(slot, static_cast<unsigned int>(ticks));
    }
}

/* One character's stamina tick: the bonus block's signed regen modifier is
   applied directly, then the fractional regen accumulator is scaled by the
   wait state (camp halved, resting states reduced, surprise full), frost
   quarters it and the trait rerolls it through the profession level. */
// FUNCTION: WIZ8 0x00504730
void RegenCharacterStamina00504730(int party_slot, unsigned int elapsed)
{
    W8Character* character = &g_status_685170.buffers.Char[party_slot];
    unsigned int frost = character->uiCondition[2];
    signed char stamina_mod = character->bonus_1770.stamina_regen_adjustment;
    if (stamina_mod < 1) {
        if (stamina_mod < 0) {
            FatigueCharacter(party_slot, -static_cast<int>(stamina_mod * elapsed), 0,
                             static_cast<W8SpellEffectResult*>(0));
        }
    } else if (character->stamina < character->uiStaminaMax) {
        RestoreCharacterStamina(party_slot, stamina_mod * static_cast<int>(elapsed), 0);
    }

    float scale = g_float_005ebb38;
    if (gXStatus.fSurprisePossible == 0) {
        scale = g_float_005ebc7c;
        if (g_status_685170.wait_state_2399 != 3) {
            scale = g_float_005ebc3c;
            if (g_status_685170.wait_state_2399 != 0 && g_status_685170.wait_state_2399 != 2) {
                scale = g_float_005ebb34;
            }
        }
    }
    if (frost != 0) {
        scale *= g_navigator_vertical_phase_step_005ebcc8;
    }
    if (CharacterHasTrait00547940(character, 0)) {
        if (scale == g_float_005ebb34) {
            if (gXStatus.fCombatMode != 0) {
                scale = ScaleValueByProfessionLevel005479B0(character, 0, 3.3f);
            }
        } else {
            scale *= g_float_005ec3b8;
        }
    }
    if (scale > g_float_005ebb34 && character->stamina < character->uiStaminaMax) {
        character->stamina_regen_accumulator_0b75 +=
            elapsed * character->stamina_regen_rate_0b71 * scale;
        int amount = static_cast<int>(character->stamina_regen_accumulator_0b75);
        RestoreCharacterStamina(party_slot, amount, 0);
        character->stamina_regen_accumulator_0b75 -=
            static_cast<float>(static_cast<int>(character->stamina_regen_accumulator_0b75));
    }
    if (character->stamina >= character->uiStaminaMax) {
        character->stamina_regen_accumulator_0b75 = g_float_005ebb34;
    }
}
