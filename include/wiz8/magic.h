#ifndef WIZ8_MAGIC_H
#define WIZ8_MAGIC_H

void ReleaseSpellDatabase(void);

#include "wiz8/layouts/gameplay_databases.h"

extern W8SpellRuntimeRecord* g_spell_records;
extern unsigned int g_spell_database_version;
extern int g_effect_argument_005ed8c8;
extern int g_effect_argument_005ed914;
extern unsigned char g_detailed_combat_messages_0068510c;

int GetSpellTargetType(
    int spell_id, unsigned char normalize_single_target);
int MinimumCasterLevelForSpellLevel(int spell_level);
int GetMinimumCasterLevelForSpell(int spell_id);
bool CanSpellBackfire(int spell_id);

struct W8MonsterInfo;
struct W8Character;
struct W8ItemInstance;

char GetTargetNeededForSpellFriendly(int spell_id, unsigned char normalize, int context);
int GetTargetNeededForSpellHostile(int spell_id);
unsigned int MonsterCastsSpell(
    W8MonsterInfo* monster_info, int spell_id, unsigned int power_level);
int PointCastSpell(float x, float y, float z, int spell_id, unsigned int power_level);

bool CanCastFromItem(const W8Character* caster, const W8ItemInstance* item);

#endif
