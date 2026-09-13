#ifndef WIZ8_LOCAL_CODE_PARTYIMPORT_H
#define WIZ8_LOCAL_CODE_PARTYIMPORT_H

struct W8Character;

/* Local Code\Party Import.cpp: the four Wizardry 7 character-import
   conversions. ConvertAttribute/ConvertSkill are named by their assertion
   strings; the other two are behaviour names. The driver 0x005590B0 that
   calls all four stays in the gap (it is not called by any recovered body,
   so it is not declared here). */
void ConvertAttribute(W8Character* character, const char* import_record); /* 0x005592D0 */
void GrantStartingSpells005595D0(W8Character* character);
void ImportEquipment00559650(W8Character* character, const char* import_record);
unsigned int ConvertSkill(unsigned int skill_id, W8Character* character, int unused,
                          const char* import_record, unsigned int base_value); /* 0x00559BC0 */

/* Unresolved gap, declared for the call site: rebuilds the character's
   derived state through a 0x3dc-byte scratch record after the spell grant. */
void Function4F9600(void* scratch, W8Character* character); /* 0x004F9600 */

#endif
