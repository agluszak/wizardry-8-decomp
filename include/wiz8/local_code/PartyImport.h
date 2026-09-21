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
    unsigned char unknown_19a[0x98];
    /* 0x232: the shared save tag - every record in one import file carries the
       same byte; its high nibble picks the Wiz7 ending and its low nibble the
       difficulty. */
    unsigned char party_tag_232;
    unsigned char unknown_233[4];
    unsigned char race_237;       /* 0x237 */
    unsigned char gender_238;     /* 0x238 */
    unsigned char profession_239; /* 0x239: Wiz7 class byte */
    unsigned char unknown_23a;
    unsigned char status_23b; /* 0x23b: 2 or 3 imports as a dead member */
    unsigned char unknown_23c[0xc];
}; /* 0x248 */

static_assert(sizeof(W8Wiz7Character) == 0x248, "W8Wiz7Character_must_be_0x248");

static_assert(sizeof(W8Wiz7Character) == 0x248, "W8Wiz7Character_must_be_0x248");

/* Local Code\Party Import.cpp: the Wizardry 7 character-import conversions. */
void ConvertAttribute(W8Character* character, const W8Wiz7Character* imported); /* 0x005592D0 */
void GrantStartingSpells005595D0(W8Character* character, const W8Wiz7Character* imported);
void ImportEquipment00559650(W8Character* character, const W8Wiz7Character* imported);
unsigned int ConvertSkill(unsigned int skill_id, W8Character* character,
                          const W8Wiz7Character* imported); /* 0x00559BC0 */
void ImportWizardry7Character005590B0(W8Character* character, W8Wiz7Character* imported);

/* 0x00558D00: parse the Wizardry 7 save into the import globals - character
   count, records, ending/difficulty selectors and the 96 flag bits. */
unsigned char LoadWizardry7ImportFile00558D00(char* path);
/* 0x00558C40: load the file, reset the run, seed 2500 gold and convert every
   imported record into a regular party member. 0 ok, 1 load/pool failure,
   2 when the ending selector carries the value three. */
unsigned char ImportWizardry7Party00558C40(char* path);

extern int g_import_character_count_0068de48;         /* 0x0068DE48 */
extern unsigned char g_import_ending_record_0068de4c; /* 0x0068DE4C */
extern int g_value_68de50;                            /* 0x0068DE50: ending selector */
extern int g_import_difficulty_0068de54;              /* 0x0068DE54 */
/* 0x0068DE58: the 96 file flags; index 5 doubles as the unsuppress byte and
   index 0xb as the loaded marker the fact seeder reads. */
extern unsigned char g_import_flags_0068de58[0x60];
extern W8Wiz7Character g_imported_characters_0068deb8[6]; /* 0x0068DEB8 */

#endif
