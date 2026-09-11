#ifndef WIZ8_MAGIC_H
#define WIZ8_MAGIC_H

void ReleaseSpellDatabase(void);

#include "wiz8/layouts/gameplay_databases.h"
#include "surrender/srMath.h"

struct W8TargetSource;
struct W8CombatSlot;
class W8Missile;

extern W8SpellRuntimeRecord* g_spell_records;
extern unsigned int g_spell_database_version;
extern int g_effect_argument_005ed8c8;
extern int g_effect_argument_005ed914;
extern unsigned char g_detailed_combat_messages_0068510c;

int GetSpellTargetType(
    int spell_id, unsigned char normalize_single_target);
bool IsSpellInSingledOutSet(int spell_id);
int MinimumCasterLevelForSpellLevel(int spell_level);
int GetMinimumCasterLevelForSpell(int spell_id);
bool CanSpellBackfire(int spell_id);

class W8SpellVisual;
struct W8EffectSlot;
struct W8MonsterInfo;
/* Create one spell visual from the spell's own record resource. Engine
   Code\Spells.cpp's factory, whose result the queued effect owns. */
void Function5526F0(W8EffectSlot* slots, const int* args); /* 0x005526F0 */
void ClearEffectSlot(W8MonsterInfo* monster_info, W8EffectSlot* slot);
void ResetPartyEffectBlock(W8EffectSlot* slot);
W8SpellVisual* SpawnSpellEffect(
    const srVector3T<float>* position, const char* resource_name,
    int argument_3, int argument_4, int argument_5);      /* 0x004AD430 */

struct W8MonsterInfo;
struct W8EffectSlot;
struct W8Character;
struct W8ItemInstance;

char GetTargetNeededForSpellFriendly(int spell_id, unsigned char normalize, int context);
int GetTargetNeededForSpellHostile(int spell_id);
unsigned int MonsterCastsSpell(
    W8MonsterInfo* monster_info, int spell_id, unsigned int power_level);
int PointCastSpell(float x, float y, float z, int spell_id, unsigned int power_level);

bool CanCastFromItem(const W8Character* caster, const W8ItemInstance* item);

char CanCharacterLearnSpell(W8Character* character, int spell_id);
void LearnSpell(W8Character* character, int spell_id, char announce);

/* Zeroes the six per-realm learned-spell counters and recounts them from the
   spell_learned array. */
void RecountLearnedSpellsByRealm004F96A0(W8Character* character);


int CastSpellFromSource(
    int spell_id, W8TargetSource* source, W8CombatSlot* target,
    unsigned int power_level, int a, int b, int c, int d, int e, int f,
    int g);                                                     /* 0x004FB4C0 */

#endif
