#ifndef WIZ8_LOCAL_CODE_PARTYIMPORT_H
#define WIZ8_LOCAL_CODE_PARTYIMPORT_H
#include "wiz8/character_skills.h"

struct W8Character;

/* One imported Wizardry 7 item record. */
struct W8Wiz7Item {
    short item_number;
    short unknown_02[5];
};

/* The imported Wizardry 7 character record. */
struct W8Wiz7Character {
    char name[0x10]; /* ASCII name TitleCaseString reads */
    int kill_count;  /* stored verbatim to kill_count */
    unsigned char unknown_014[0x10];
    short level;  /* positive values import as level 1 */
    short deaths; /* stored to death_count minus one */
    unsigned char unknown_028[0x18];
    W8Wiz7Item items[2][10]; /* two ten-item lists, 0x78 each */
    unsigned char unknown_130[0x40];
    unsigned char attributes[8]; /* ConvertAttribute reads [0..5] */
    unsigned char skills[0x22];  /* ConvertSkill indexes it by the
                                          mapped skill id (highest 0x21) and
                                          reads the contiguous ranges inside */
    unsigned char unknown_19a[0x98];
    /* The shared save tag - every record in one import file carries the
       same byte; its high nibble picks the Wiz7 ending and its low nibble the
       difficulty. */
    unsigned char party_tag;
    unsigned char unknown_233[4];
    unsigned char race;
    unsigned char gender;
    unsigned char profession; /* Wiz7 class byte */
    unsigned char unknown_23a;
    unsigned char status; /* 2 or 3 imports as a dead member */
    unsigned char unknown_23c[0xc];
};

static_assert(sizeof(W8Wiz7Character) == 0x248, "W8Wiz7Character_must_be_0x248");

static_assert(sizeof(W8Wiz7Character) == 0x248, "W8Wiz7Character_must_be_0x248");

/* Local Code\Party Import.cpp: the Wizardry 7 character-import conversions. */
void ConvertAttribute(W8Character* character, const W8Wiz7Character* imported);
void GrantStartingSpells(W8Character* character, const W8Wiz7Character* imported);
void ImportEquipment(W8Character* character, const W8Wiz7Character* imported);
unsigned int ConvertSkill(W8Skill skill_id, W8Character* character,
                          const W8Wiz7Character* imported);
void ImportWizardry7Character(W8Character* character, W8Wiz7Character* imported);

/* Parse the Wizardry 7 save into the import globals - character
   count, records, ending/difficulty selectors and the 96 flag bits. */
unsigned char LoadWizardry7ImportFile(char* path);
/* Load the file, reset the run, seed 2500 gold and convert every
   imported record into a regular party member. 0 ok, 1 load/pool failure,
   2 when the ending selector carries the value three. */
unsigned char ImportWizardry7Party(char* path);

extern int g_import_character_count;
extern bool g_import_ending_record;
extern int g_wiz7_ending; /* Ending selector */
extern int g_import_difficulty;
/* The 96 file flags; index 5 doubles as the unsuppress byte and
   index 0xb as the loaded marker the fact seeder reads. */
extern unsigned char g_import_flags[0x60];
extern W8Wiz7Character g_imported_characters[6];

#endif
