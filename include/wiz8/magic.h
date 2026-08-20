#ifndef WIZ8_MAGIC_H
#define WIZ8_MAGIC_H

#include "wiz8/layouts/gameplay_databases.h"

extern "C" {

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

}

#endif
