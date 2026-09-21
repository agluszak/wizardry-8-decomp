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
    char name_000[0x10]; /* 0x000: ASCII name TitleCaseString reads */
    int unknown_010;     /* 0x010: stored verbatim to value_09f9 */
    unsigned char unknown_014[0x10];
    short level_024;  /* 0x024: positive values import as level 1 */
    short deaths_026; /* 0x026: stored to death_count_09fd minus one */
    unsigned char unknown_028[0x18];
    W8Wiz7Item items[2][10]; /* 0x040: two ten-item lists, 0x78 each */
    unsigned char unknown_130[0x40];
    unsigned char attributes[8]; /* 0x170: ConvertAttribute reads [0..5] */
    unsigned char skills[0x22];  /* 0x178: ConvertSkill indexes it by the
                                          mapped skill id (highest 0x21) and
                                          reads the contiguous ranges inside */
    unsigned char unknown_19a[0x9d];
    unsigned char race_237;       /* 0x237 */
    unsigned char gender_238;     /* 0x238 */
    unsigned char profession_239; /* 0x239: Wiz7 class byte */
    unsigned char unknown_23a;
    unsigned char status_23b; /* 0x23b: 2 or 3 imports as a dead member */
    /* The on-disk record and the DAT_0068DEB8 import buffer are read and
       walked at a 0x248 stride; the tail beyond status_23b is unread. */
    unsigned char unknown_23c[0x0c];
};
static_assert(sizeof(W8Wiz7Character) == 0x248, "W8Wiz7Character_size");

/* Local Code\Party Import.cpp: the Wizardry 7 character-import conversions. */
void ConvertAttribute(W8Character* character, const W8Wiz7Character* imported); /* 0x005592D0 */
void GrantStartingSpells005595D0(W8Character* character, const W8Wiz7Character* imported);
void ImportEquipment00559650(W8Character* character, const W8Wiz7Character* imported);
unsigned int ConvertSkill(unsigned int skill_id, W8Character* character,
                          const W8Wiz7Character* imported); /* 0x00559BC0 */
void ImportWizardry7Character005590B0(W8Character* character, W8Wiz7Character* imported);

#endif
