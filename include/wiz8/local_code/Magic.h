#ifndef WIZ8_LOCAL_CODE_MAGIC_H
#define WIZ8_LOCAL_CODE_MAGIC_H

#include "wiz8/layouts/targeting.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "surrender/srMath.h"
#include "wiz8/vector.h"

struct W8TargetSource;
struct W8CombatSlot;
struct W8Character;
struct W8ItemInstance;
struct W8MonsterInfo;
class W8Missile;

extern int g_learn_sound_0068c510;

/* 0x005001E0: whether the spell may be cast in the current situation. Every
   retail caller pushes only these two arguments. */
unsigned char SpellUsableNow(int spell_id, int allow_out_of_combat);

char GetTargetNeededForSpellFriendly(int spell_id, unsigned char normalize,
                                     W8TargetingContext context);
int GetTargetNeededForSpellHostile(int spell_id);
/* 0x004FB1D0: whether the monster's spellcasting-blocked condition stops it
   casting this spell - everything except alchemy, and alchemy only for the
   monster kinds that keep it. */
bool IsSpellBlockedForMonster(W8MonsterInfo* monster_info, int spell_id);
/* 0x004FB0A0: everything that has to hold before a monster may start
   casting. */
bool MonsterOKToCastSpell(W8MonsterInfo* monster_info, int spell_id);
unsigned int MonsterCastsSpell(W8MonsterInfo* monster_info, int spell_id, unsigned int power_level);
/* 0x00500330: the power level the monster can afford for the spell, out of
   its database base plus its runtime bonus. */
unsigned int ChooseMonsterSpellPowerLevel(W8MonsterInfo* monster_info, int unused, int spell_id);
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
char CanCharacterCastSpell(W8Character* character, int spell_id);
unsigned int GetBestSpellbookSkillForSpell(W8Character* character, int spell_id, char pricing,
                                           char prefer_unlocked, unsigned int power_level,
                                           int level_bonus);

int CastSpellFromSource(int spell_id, W8TargetSource* source, W8CombatSlot* target,
                        unsigned int power_level, int a, int b, int c, int d, int e, int f,
                        int g); /* 0x004FB4C0 */

/* Resolve valid spell targets for one cast and append location ids to the
   caller's vectors. The party and monster vectors are separate outputs; the
   final flag enables radius highlighting rather than plain line-of-sight
   collection. */
void PopulateSpellTargetMarkers(int spell_id, int power_level, W8TargetSource* source,
                                W8CombatSlot* target, W8GrowableVector<int>* monster_markers,
                                W8GrowableVector<int>* party_markers,
                                int highlighting); /* 0x004FD030 */
int GetProfessionCasterLevel(W8Character* character, int profession_id);
int GetSpellbookForSpell(const W8Character* character, int spell_id, int a, int b, int c);
/* 0x00501A60: the spell a missile type carries, or W8_SPELL_NONE. */
int MissileSpellId(int missile_type);
/* Whether the party as a whole is under one particular condition. */
bool PartyHasCondition(int condition_id); /* 0x005012B0 */
extern const unsigned short g_realm_message_offsets[W8_SPELL_REALM_COUNT];

void DetachMissileReferences005019A0(W8Missile* missile);

#endif
