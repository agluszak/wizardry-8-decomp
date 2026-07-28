#ifndef WIZ8_ITEM_TABLES_H
#define WIZ8_ITEM_TABLES_H

#include "wiz8/gameplay_databases.h"
#include "wiz8/vector.h"

struct W8WorldItem;

#pragma pack(push, 1)

struct W8ItemTableEntry {
    short selector_00;                    /* 0x00: zero disables the slot */
    unsigned short item_id;               /* 0x02: index into Items.dbs */
    unsigned char weight;                 /* 0x04 */
};                                       /* 0x05 */

struct W8ItemTableRecord {
    char name[256];                       /* 0x000 */
    unsigned int category_id;             /* 0x100 */
    W8ItemTableEntry entries[40];         /* 0x104 */
    unsigned char level_scaled;           /* 0x1cc */
    unsigned char unknown_1cd[0x24];      /* 0x1cd */
};                                       /* 0x1f1 */

/* One "you need this much of that" entry. CanCharacterUseItem walks two of
   these for attributes and two for skills, stopping at an id of 0xff. */
struct W8ItemRequirement {
    unsigned char stat_id;                /* 0xff when the entry is unused */
    unsigned char minimum;
};                                       /* 0x02 */

struct W8ItemDatabaseRecord {
    /* 0x000: the item's own name, which is what the display path returns
       directly for an identified item - the record address doubles as the name
       address because the name leads the record. */
    W8WideChar display_name[30];
    unsigned char unknown_03c[2];
    /* 0x03e: the equipment class, zero through twelve. GetItemDefaultEquipSlot
       is a thirteen-way switch on it and is the only body that enumerates the
       whole domain; class four additionally prices and stacks by the bundle. */
    unsigned char equip_class;
    /* 0x03f: groups items that share one generic name while unidentified.
       GetItemDisplayRecord builds the shared name lazily, one cached string per
       index, and two items with equal indices read as the same thing. */
    unsigned short unidentified_name_index;
    /* 0x041: bit one blocks discarding the item, bit two makes it two-handed,
       and bit three lets it be held in the off hand. Those are the three bits
       any recovered body reads. */
    unsigned char flags_041;
    /* 0x042: the item's kind. Three selects the spell-source items the magic
       code accepts; no other value is established. */
    unsigned char category;
    unsigned char unknown_043[3];
    /* 0x046: the skill a weapon is used with. GetItemEquipSlotMask refuses to
       place a weapon whose value is 0xff and says so - "ERROR - Item %ls is a
       weapon without a skill specified -> Charles". */
    unsigned char weapon_skill;
    /* 0x047: two weapons may only be wielded together when this agrees. Its
       domain is not established, so it is named for that one use. */
    unsigned char wield_group;
    unsigned char unknown_048[0x1b];
    /* 0x063: the spell the item casts, for the categories that cast one. The
       assertions in CanCharacterUseItem call it uiSpell and refuse zero, which
       is SPELL_NONE. */
    unsigned char spell_id;
    unsigned char unknown_064[2];
    /* 0x066: zero none, one stack, two through four uses or charges. */
    unsigned char quantity_kind;
    unsigned char unknown_067[0xf];
    /* 0x076: one bit per profession id; the character's current profession has
       to be among them. */
    unsigned short profession_mask;
    unsigned int race_mask;               /* 0x078: one bit per race id */
    /* 0x07c: one bit per faction id, except that the value three admits every
       faction rather than only the two it would name. */
    unsigned char faction_mask;
    /* 0x07d: up to two attribute floors. An id of 0xff ends the pair; the
       minimum is compared against the character's effective attribute. */
    W8ItemRequirement attribute_requirements[2];
    /* 0x081: up to two skill floors, the same shape, compared against the
       character's skill level. */
    W8ItemRequirement skill_requirements[2];
    unsigned char unknown_085;
    /* 0x086: the gold value of one item, except for equip class four, which is
       priced by the bundle of twenty-five. GenerateItemsFromTable filters a
       level-scaled table on the same field. */
    unsigned int value;
    unsigned short weight;                /* 0x08a: the weight of one item */
    /* 0x08c: the item binds itself to whoever equips it. Equipping one raises
       the instance's `bound` flag and posts a log line, the two hand slots of
       the alternate weapon set are checked for it before a weapon swap is
       allowed, and nothing else reads it. */
    unsigned char binds_on_equip;
    unsigned char unknown_08d[0x80];
};                                       /* 0x10d */

#pragma pack(pop)

unsigned int GetAveragePartyLevel(void); /* 0x004EF420 */
int FindItemTableByName(const char* name);
int GenerateItemsFromTable(
    W8GrowableVector<W8WorldItem*>* output_items,
    unsigned int table_id,
    unsigned int maximum_items);          /* 0x004F88F0 */

#endif
