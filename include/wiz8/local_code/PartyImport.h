#ifndef WIZ8_LOCAL_CODE_PARTYIMPORT_H
#define WIZ8_LOCAL_CODE_PARTYIMPORT_H

struct W8Character;

/* One imported Wizardry 7 item record: the item number plus five shorts the
   four recovered bodies never read individually (quality/charges/etc. stay
   unknown). */
struct W8Wiz7Item {
    short item_number;
    short unknown_02[5];
}; /* 0x0c */

/* The imported Wizardry 7 character record. Only the members a recovered
   body reads are named; the gaps and the total size are unknown (no
   size assertion). */
struct W8Wiz7Character {
    unsigned char unknown_000[0x40];
    W8Wiz7Item items[2][10]; /* 0x040: two ten-item lists, 0x78 each */
    unsigned char unknown_130[0x40];
    unsigned char attributes[8]; /* 0x170: ConvertAttribute reads [0..5] */
    unsigned char skills[0x22];  /* 0x178: ConvertSkill indexes it by the
                                          mapped skill id (highest 0x21) and
                                          reads the contiguous ranges inside */
    unsigned char unknown_19a[0x9f];
    unsigned char profession_239; /* 0x239: Wiz7 class byte */
};

/* Local Code\Party Import.cpp: the four Wizardry 7 character-import
   conversions. ConvertAttribute/ConvertSkill are named by their assertion
   strings; the other two are behaviour names. The driver 0x005590B0 that
   calls all four stays in the gap (it is not called by any recovered body,
   so it is not declared here). */
void ConvertAttribute(W8Character* character, const W8Wiz7Character* imported); /* 0x005592D0 */
void GrantStartingSpells005595D0(W8Character* character);
void ImportEquipment00559650(W8Character* character, const W8Wiz7Character* imported);
unsigned int ConvertSkill(unsigned int skill_id, W8Character* character,
                          const W8Wiz7Character* same_record, const W8Wiz7Character* imported,
                          unsigned int base_value); /* 0x00559BC0 */

/* Unresolved gap, declared for the call site: rebuilds the character's
   derived state through a 0x3dc-byte scratch record after the spell grant. */
void Function4F9600(void* scratch, W8Character* character); /* 0x004F9600 */

#endif
