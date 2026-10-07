#pragma once

struct W8Character;
struct W8MonsterInfo;

void UpdateGameClock(int elapsed);
/* The 120-second aging tick - wait-state machine, per-character
   aging, combat/monster effect expiry and NPC-side aging. */
void AdvanceTimedEffects(unsigned int minutes);
/* Tear down the surprise sequence's world-state overrides. */
void RestoreSurpriseView(void);
void ResolveSurpriseWake(void);
void EndSurprise(void); /* End surprise and post the notice */
/* End a holding surprise sequence when combat starts. */
void ResolveSurpriseHold(void);
/* Per-character share of the aging tick - damage/heal/
   stamina/spell-point modifiers, disease progression, regen accumulators and
   the condition/enchantment countdowns. */
void GameTurnsPassedChar(int party_slot, unsigned int minutes);
/* Camping fatigue tick - rolls fatigue dice per character. */
void UpdateCampFatigue(int ticks);
/* Stamina driver - refreshes the wait state then ticks each
   eligible character. */
void UpdatePartyStamina(int ticks);
/* Per-character stamina regen/fatigue tick. */
void RegenCharacterStamina(int party_slot, unsigned int elapsed);
void AgeMonsterSight(W8MonsterInfo* monster_info, unsigned int minutes,
                     unsigned char full_health_regeneration);
/* Rebuild the per-realm stamina and spell regeneration rates from
   the pool ceilings and the three regeneration-boost flags. */
void RebuildCharacterRegenRates(W8Character* character);
/* The monster counterpart - hit points and stamina only, plus the
   modifier block's regen channels. */
void RebuildMonsterRegenRates(W8MonsterInfo* monster_info);
/* Advance the surprise fade/hold/resolve sequence while
   fSurprisePossible is set. */
void UpdateSurpriseMode(void);
/* The cancel command while surprise runs - reposts the surprise
   notice while the ambush is unengaged, otherwise performs the phase-1
   transition that restores the view, time scale and music. */
void AcknowledgeSurprise(void);
/* Scripted-surprise entry - completes the pending character
   events, resets the occupied slots' portrait poses, takes the menu button
   banks down and redraws. */
void BeginSurprise(void);
/* The Camp command's request to enter the surprise-possible camp state. */
void RequestCamp(void);
