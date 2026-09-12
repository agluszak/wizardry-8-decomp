#ifndef WIZ8_LOCAL_CODE_MAGIC_H
#define WIZ8_LOCAL_CODE_MAGIC_H

#include "wiz8/layouts/targeting.h"
#include "surrender/srMath.h"
#include "wiz8/vector.h"

struct W8TargetSource;
struct W8CombatSlot;
struct W8Character;
struct W8ItemInstance;
struct W8MonsterInfo;

extern int g_effect_argument_005ed8c8;
extern int g_effect_argument_005ed914;

char GetTargetNeededForSpellFriendly(int spell_id, unsigned char normalize,
                                     W8TargetingContext context);
int GetTargetNeededForSpellHostile(int spell_id);
unsigned int MonsterCastsSpell(W8MonsterInfo* monster_info, int spell_id, unsigned int power_level);
int PointCastSpell(srVector3T<float> position, int spell_id, unsigned int power_level);

bool CanCastFromItem(const W8Character* caster, const W8ItemInstance* item);

char CanCharacterLearnSpell(W8Character* character, int spell_id);
void LearnSpell(W8Character* character, int spell_id, char announce);

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
void PopulateSpellTargetMarkers(int spell_id, int normalize_single_target, W8TargetSource* source,
                                W8CombatSlot* target, W8GrowableVector<int>* party_markers,
                                W8GrowableVector<int>* monster_markers,
                                int highlighting); /* 0x004FD030 */
int GetProfessionCasterLevel(W8Character* character, int profession_id);
int GetSpellbookForSpell(const W8Character* character, int spell_id, int a, int b, int c);

#endif
