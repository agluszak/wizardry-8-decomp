#pragma once

struct W8Character;
struct W8MonsterInfo;

void UpdateGameClock00502010(int elapsed); /* 0x00502010 */
/* 0x00502D00: the 120-second aging tick - wait-state machine, per-character
   aging, combat/monster effect expiry and NPC-side aging. */
void AdvanceTimedEffects00502D00(unsigned int minutes);
/* 0x005029A0: tear down the surprise sequence's world-state overrides. */
void RestoreSurpriseView005029A0(void); /* 0x005029A0 */
void ResolveSurpriseWake005029E0(void); /* 0x005029E0 */
void EndSurprise00502860(void);         /* 0x00502860: end surprise and post the notice */
/* 0x00502810: end a holding surprise sequence when combat starts. */
void ResolveSurpriseHold00502810(void);
/* 0x00503100: per-character share of the 0x00502D00 aging tick - damage/heal/
   stamina/spell-point modifiers, disease progression, regen accumulators and
   the condition/enchantment countdowns. */
void GameTurnsPassedChar00503100(int party_slot, unsigned int minutes);
/* 0x005044D0: camping fatigue tick - rolls fatigue dice per character. */
void UpdateCampFatigue005044D0(int ticks);
/* 0x00504670: stamina driver - refreshes the wait state then ticks each
   eligible character through 0x00504730. */
void UpdatePartyStamina00504670(int ticks);
/* 0x00504730: per-character stamina regen/fatigue tick. */
void RegenCharacterStamina00504730(int party_slot, unsigned int elapsed);
void AgeMonsterSight(W8MonsterInfo* monster_info, unsigned int minutes, int arg_3);
/* 0x00502B50: rebuild the per-realm stamina and spell regeneration rates from
   the pool ceilings and the three conditioned-rate flags. */
void RebuildCharacterRegenRates00502B50(W8Character* character);
/* 0x00502C50: the monster counterpart - hit points and stamina only, plus the
   modifier block's regen channels. */
void RebuildMonsterRegenRates00502C50(W8MonsterInfo* monster_info);
/* 0x00502650: advance the surprise fade/hold/resolve sequence while
   fSurprisePossible is set. */
void UpdateSurpriseMode(void);
/* 0x00502790: the cancel command while surprise runs - reposts the surprise
   notice while the ambush is unengaged, otherwise performs the phase-1
   transition that restores the view, time scale and music. */
void AcknowledgeSurprise00502790(void);
/* 0x005025F0: scripted-surprise entry - completes the pending character
   events, resets the occupied slots' portrait poses, takes the menu button
   banks down and redraws. */
void BeginSurprise005025F0(void);
/* The Camp command's request to enter the surprise-possible camp state. */
void RequestCamp00502460(void); /* 0x00502460 */
