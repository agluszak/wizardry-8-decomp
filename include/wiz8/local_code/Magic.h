#ifndef WIZ8_LOCAL_CODE_MAGIC_H
#define WIZ8_LOCAL_CODE_MAGIC_H

#include "wiz8/layouts/targeting.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "surrender/srMath.h"
#include "wiz8/vector.h"

struct W8TargetSource;
struct W8CombatSlot;
struct W8PartySlotRow;
struct W8Character;
struct W8ItemInstance;
struct W8MonsterInfo;
class W8Missile;

void TickSpellEffects(void); /* 0x00500E90 */

extern int g_learn_sound_0068c510;

/* 0x005001E0: whether the spell may be cast in the current situation. Every
   retail caller pushes only these two arguments. */
bool SpellUsableNow(int spell_id, unsigned char allow_out_of_combat);

int GetTargetNeededForSpellFriendly(int spell_id, unsigned char normalize,
                                    W8TargetingContext context);
int GetTargetNeededForSpellHostile(int spell_id);
/* 0x004FB1D0: whether the monster's spellcasting-blocked condition stops it
   casting this spell - everything except alchemy, and alchemy only for the
   monster kinds that keep it. */
bool IsSpellBlockedForMonster(W8MonsterInfo* monster_info, int spell_id);
/* 0x004FB0A0: everything that has to hold before a monster may start
   casting. The retail caller passes the chosen power level as a third
   argument the body never reads. */
bool MonsterOKToCastSpell(W8MonsterInfo* monster_info, int spell_id, int power_level);
unsigned int MonsterCastsSpell(W8MonsterInfo* monster_info, int spell_id, unsigned int power_level);
/* 0x00500330: the power level the monster can afford for the spell, out of
   its database base plus its runtime bonus. */
unsigned int ChooseMonsterSpellPowerLevel(W8MonsterInfo* monster_info, W8MonsterRecord* record,
                                          int spell_id);
/* 0x004FA4D0: one physical callable for queued and repeated casts; returns
   0/1/2 and writes the per-step point cost. */
int ExecuteCharacterSpellCast(int party_slot, int spell_id, unsigned int power_level,
                              int* out_points, char continue_cast);
/* 0x004FE740: a backfiring spell swaps roles - the intended target becomes
   the source and the original source the target. */
void RedirectBackfiredSpellTarget004FE740(W8TargetSource* source, W8CombatSlot* target);
/* 0x004FEDC0: pick one random in-combat participant other than the source
   and write it as the backfired spell's new target. */
int PickBackfireTarget004FEDC0(int spell_id, W8TargetSource* source, W8CombatSlot* target);
/* 0x004FEF90: scatter a backfired point target to a random reachable spot
   inside the spell's range around its source. */
void ScatterSpellPointTarget004FEF90(int spell_id, W8TargetSource* source, W8CombatSlot* target);
/* 0x004FF220: the once-per-cast backfire roll for single-target spells -
   condition 0x0c gates it, a trait save can avoid it. */
void CheckSpellBackfire004FF220(int spell_id, W8TargetSource* source, W8CombatSlot* target);
void SetPartySlotSpell(int party_slot, int spell_id, int power_level, const W8CombatSlot* target);
int PointCastSpell(srVector3T<float> position, int spell_id, unsigned int power_level);

bool CanCastFromItem(const W8Character* caster, const W8ItemInstance* item);

char CanCharacterLearnSpell(W8Character* character, int spell_id);
void LearnSpell(W8Character* character, int spell_id, char announce);
/* 0x00500060: learns the spell a spell-source item holds and empties the
   item. Retail callers push two arguments; the earlier three-parameter decl
   added a phantom slot. */
void LearnSpellFromItem(W8Character* character, W8ItemInstance* item);

/* Zeroes the six per-realm learned-spell counters and recounts them from the
   spell_learned array. */
void RecountLearnedSpellsByRealm004F96A0(W8Character* character);
bool CanCharacterCastSpell(W8Character* character, int spell_id);
unsigned int GetBestSpellbookSkillForSpell(W8Character* character, int spell_id, char pricing,
                                           char prefer_unlocked, unsigned int power_level);

int CastSpellFromSource(int spell_id, W8TargetSource* source, W8CombatSlot* target,
                        unsigned int power_level, int a, int b, int c, int* d, int e,
                        W8GrowableVector<int>* party_targets,
                        W8GrowableVector<int>* monster_targets); /* 0x004FB4C0 */
/* 0x004FEA50: assert and route the source/target pair a cast is about to
   use; some target kinds have their own placement pass. */
void PrepareSpellTarget004FEA50(int spell_id, W8TargetSource* source, W8CombatSlot* target);
/* Resolve valid spell targets for one cast and append location ids to the
   caller's vectors. The party and monster vectors are separate outputs; the
   final flag enables radius highlighting rather than plain line-of-sight
   collection. */
void PopulateSpellTargetMarkers(int spell_id, int power_level, W8TargetSource* source,
                                W8CombatSlot* target, W8GrowableVector<int>* monster_markers,
                                W8GrowableVector<int>* party_markers,
                                int highlighting); /* 0x004FD030 */
/* 0x00501B70: drop every marker that no longer names a live, targetable
   monster; a few spell ids prune on extra monster-record rules. */
void PruneSpellTargetMarkers00501B70(int spell_id, W8GrowableVector<int>* monster_markers);
int GetProfessionCasterLevel(W8Character* character, int profession_id);
/* 0x00501D60: the highest power level this slot can afford to cast the spell
   at for its current target; zero when none is castable. */
unsigned int ChooseSpellPowerLevelForTarget(int party_slot, int spell_id, int identify_context);
extern unsigned char g_profession_spellbooks[W8_PROFESSION_COUNT];
int GetSpellbookForSpell(const W8Character* character, int spell_id, int a, int b, int c);
/* 0x00501A60: the spell a missile type carries, or W8_SPELL_NONE. */
int MissileSpellId(int missile_type);
/* Whether the party as a whole is under one particular condition. */
bool PartyHasCondition(int condition_id); /* 0x005012B0 */
/* Whether the combat party-effect slots already carry one condition. */
bool CombatHasCondition(int condition_id);                                   /* 0x00501250 */
int GetSpellDifficulty(unsigned int caster_figure, int spell_id, int bonus); /* 0x004FF790 */

/* 0x004FAE70: whether a spellcasting-blocked condition stops this character
   casting this spell. */
bool IsSpellBlockedForCharacter(const W8Character* character, int spell_id);
/* 0x004F9A20: records the character's chosen spell and power level for the
   pending action. */
void SetCharacterSpell(const W8Character* character, int spell_id, int power_level);
/* 0x00501400: the power level the party slot's chosen spell can actually be
   cast at; zero means the cast cannot happen at all. */
int GetAffordableSpellPowerLevel(int party_slot);
/* Whether the party slot's recorded spell is still castable. */
bool CanPartySlotCastRecordedSpell(int party_slot); /* 0x005012E0 */
/* Whether the party slot's recorded item is still usable. */
bool CanPartySlotUseRecordedItem(int party_slot); /* 0x00501660 */
/* Queue the slot's recorded spell cast; zero keeps the recorded power. */
void StartCharacterSpellCast(int party_slot, int power_level); /* 0x00501590 */
/* Queue the slot's recorded item use. */
void StartCharacterItemUse(int party_slot); /* 0x00501790 */
/* 0x00501860: one-line forwarder narrowing CanCharReBreathe to a flag. */
bool CanPartySlotReBreathe(int party_slot);
/* 0x00501880: start one character's breath attack. */
void StartCharacterBreathAttack(int party_slot);
/* 0x004FF4B0: the failure chance for one cast at a power level. Retail call
   sites push three arguments. */
unsigned int GetSpellFailureChanceForCast(W8Character* character, int spell_id,
                                          unsigned int power_level);
/* 0x004FF410: the same chance for a bare skill figure rather than a caster,
   which is what an item-use attempt has. */
unsigned int GetSpellFailureChance(unsigned int skill, int spell_id, int factor);

/* 0x004FF790: how hard this caster figure finds one spell at a power level, and
   0x00501910: the combat-pace scale an item-use attempt applies to its own
   difficulty. Both are defined in this unit. */
unsigned int ScaleByCombatPace(int party_slot, unsigned int* value);

/* Validate the available target set and cursor state for a spell or item cast. */
bool ValidateSpellTarget004FAC40(int party_slot, int spell_id, unsigned int power, bool item_cast,
                                 bool skip_world_cursor);
unsigned char SpellAffectedTarget004F9AE0(W8Character* character, int spell_id, W8CombatSlot* aim,
                                          unsigned int power);
void TrackItemSpellSource00501D20(W8Character* character, int spell_id);
bool IsTeleportCastMissingAnchor00501D00(W8Character* character, int spell_id); /* 0x00501D00 */
extern const unsigned short g_realm_message_offsets[W8_SPELL_REALM_COUNT];

void DetachMissileReferences005019A0(W8Missile* missile);
/* Whether every queued effect still has time left on it. */
bool AllSpellEffectsStillRunning(void); /* 0x00500E50 */
bool CombatHasCondition(int condition_id);

#endif
