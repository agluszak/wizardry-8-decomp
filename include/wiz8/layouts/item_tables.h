#ifndef WIZ8_LAYOUTS_ITEM_TABLES_H
#define WIZ8_LAYOUTS_ITEM_TABLES_H

#include "wiz8/dice.h"

#pragma pack(push, 1)

struct W8ItemTableEntry {
    short selector_00;      /* 0x00: zero disables the slot */
    unsigned short item_id; /* 0x02: index into Items.dbs */
    unsigned char weight;   /* 0x04 */
}; /* 0x05 */

struct W8ItemTableRecord {
    char name[256];               /* 0x000 */
    unsigned int category_id;     /* 0x100 */
    W8ItemTableEntry entries[40]; /* 0x104 */
    unsigned char level_scaled;   /* 0x1cc */
    W8Dice item_count_dice;       /* 0x1cd */
    unsigned char unknown_1d1[4];
    W8Dice gold_dice; /* 0x1d5 */
    unsigned char unknown_1d9[0x18];
}; /* 0x1f1 */

/* One "you need this much of that" entry. CanCharacterUseItem walks two of
   these for attributes and two for skills, stopping at an id of 0xff. */
struct W8ItemRequirement {
    unsigned char stat_id; /* 0xff when the entry is unused */
    unsigned char minimum;
}; /* 0x02 */

/* Named subset of W8ItemDatabaseRecord::equip_class. Values 0..3 are weapon
   classes whose finer distinction is not recovered here. Classes 4..12 are
   fixed by GetItemEquipSlotMask: class 4 is the non-shield off-hand class
   (ammunition), class 5 is the shield class used by the Shield AC component,
   and classes 6..12 map directly to the manual's six worn armor/accessory
   locations.

   That slot-mask coverage does not bound the database field. The Assay display
   table g_equip_class_name_ids has 32 entries, and recovered Assay /
   item paths also use higher values (including 0x0d, 0x0e, 0x11, 0x12, and
   0x13). Keep equip_class a byte: this enum names the proven subset and is not
   a completeness claim. */
enum W8ItemEquipClass {
    W8_ITEM_EQUIP_CLASS_WEAPON_0 = 0,
    W8_ITEM_EQUIP_CLASS_WEAPON_1 = 1,
    W8_ITEM_EQUIP_CLASS_WEAPON_2 = 2,
    W8_ITEM_EQUIP_CLASS_WEAPON_3 = 3,
    W8_ITEM_EQUIP_CLASS_AMMUNITION = 4,
    W8_ITEM_EQUIP_CLASS_SHIELD = 5,
    W8_ITEM_EQUIP_CLASS_TORSO = 6,
    W8_ITEM_EQUIP_CLASS_LEGS = 7,
    W8_ITEM_EQUIP_CLASS_HEAD = 8,
    W8_ITEM_EQUIP_CLASS_FEET = 9,
    W8_ITEM_EQUIP_CLASS_HANDS = 10,
    W8_ITEM_EQUIP_CLASS_MISC = 11,
    W8_ITEM_EQUIP_CLASS_CLOAK = 12,
};

struct W8ItemDatabaseRecord {
    wchar_t display_name[30]; /* 0x000 */
    /* 0x03c: the item number the Wizardry 7 import matches imported item ids
       against (Party Import.cpp). */
    short legacy_item_number_03c;
    unsigned char equip_class;              /* 0x03e: open byte domain; named W8ItemEquipClass
                                  values are the proven subset, not the bound */
    unsigned short unidentified_name_index; /* 0x03f */
    unsigned char flags_041;                /* 0x041 */
    unsigned char category;                 /* 0x042: three is a spell source */
    unsigned char unknown_043[3];
    signed char weapon_skill;        /* 0x046: -1 when the item grants none */
    unsigned char wield_group;       /* 0x047 */
    signed char attack_damage_bonus; /* 0x048 */
    signed char attack_hit_bonus;    /* 0x049 */
    W8Dice damage_dice;              /* 0x04a */
    unsigned short attack_flags_04e;
    /* 0x050..0x05f: the item's missile-attack modifier block; the missile
       resolver sums it byte-wise across the wielded and paired weapons into
       the fired effect definition's condition_chances. */
    unsigned char missile_values_050[0x10];
    /* 0x060: the item's missile bonus, summed across both weapons into the
       effect definition's value_1c. */
    unsigned char missile_magnitude_060;
    /* 0x061: the monster kind the weapon slays for an extra damage die,
       compared against W8MonsterRecord::kind_0cb by the character damage
       resolver; 0xff means the weapon slays nothing. */
    unsigned char slays_kind_061;
    signed char armor_class_bonus; /* 0x062 */
    unsigned char spell_id;        /* 0x063 */
    /* 0x064: the cast spell's power level - Assay prints "(Pwr %d)" and the
       use path consumes it as the casting power. */
    unsigned char spell_power_064;
    unsigned char unknown_065;
    unsigned char quantity_kind; /* 0x066 */
    W8Dice initial_quantity;     /* 0x067 */
    /* The stack merge path clamps quantity-kind 1 items to this byte. */
    unsigned char maximum_quantity; /* 0x06b */
    /* 0x06c..0x06e: three per-item modifier bytes the equipment fold adds to
       the derived block's own unknowns; 0x06f..0x074 are the six resistance
       bonuses it sums and clamps. */
    signed char modifier_06c;
    signed char modifier_06d;
    signed char modifier_06e;
    signed char resistance_bonus_06f[6]; /* 0x06f .. 0x074 */
    unsigned char property_075;          /* assay special-property label index */
    unsigned short profession_mask;      /* 0x076 */
    unsigned int race_mask;              /* 0x078 */
    /* 0x07c: one bit per sex; three admits either, and
       CanCharacterUseItem indexes it with the character's own field. */
    unsigned char gender_mask;
    W8ItemRequirement attribute_requirements[2]; /* 0x07d */
    W8ItemRequirement skill_requirements[2];     /* 0x081 */
    unsigned char identify_difficulty;           /* 0x085 */
    unsigned int value;                          /* 0x086 */
    unsigned short weight;                       /* 0x08a */
    unsigned char binds_on_equip;                /* 0x08c */
    char internal_name[0x20];                    /* 0x08d .. 0x0ac */
    /* 0x0ad: extra swings the weapon grants when the wielder starts an
       attack; StartCharacterAttack adds it to the rolled uiSwingsRemaining.
       It sits inside the retail name region's tail dword, so the name buffer
       is really 0x20 characters. */
    int swings_bonus_0ad;
    /* 0x0b1/0x0b3: the item's (index, value) modifier pairs the equipment
       fold adds to the derived block's two byte tables. 0xff is no pair. */
    signed char modifier_0b1_index;
    signed char modifier_0b1_value;
    signed char modifier_0b3_index;
    signed char modifier_0b3_value;
    unsigned char unknown_0b5[4]; /* 0x0b5 .. 0x0b8 */
    int merge_kind_0b9;           /* first database kind accepted by MergeItems */
    int merge_kind_0bd;           /* second database kind accepted by MergeItems */
    /* 0x0c1: the item's material index; combat sound reads it on the struck
       item to pick the impact table's material column (0..11). */
    int material_0c1;
    /* 0x0c5: the weapon's attack sound class; combat sound bounds it against
       the 38-entry swing table and the 28 impact rows. */
    int weapon_sound_class_0c5;
    signed char merge_skill_0c9; /* skill required to create this item */
    unsigned char merge_skill_level_0ca;
    unsigned char editor_excluded_0cb; /* hidden from the MIPE item list */
    /* 0x0cc: the missile table entry the item fires; the missile resolver
       bounds it against g_missile_table_count. */
    signed char missile_type;
    /* GetOrCreateVideoObject treats this fixed buffer as the item image name. */
    char video_object_name[0x40]; /* 0x0cd */
}; /* 0x10d */

static_assert(sizeof(W8ItemDatabaseRecord) == 0x10d, "W8ItemDatabaseRecord_size_must_be_0x10d");
static_assert(sizeof(W8ItemTableRecord) == 0x1f1, "W8ItemTableRecord_size_must_be_0x1f1");

#pragma pack(pop)

/* GameplayDatabase.cpp owns the runtime Items.dbs and ItemTables.dbs roots.
   Keeping their declarations beside the record layouts gives item consumers a
   direct owner beside the file-format types they index. */
extern W8ItemDatabaseRecord* g_item_records;
extern W8ItemTableRecord** g_item_tables;
extern char** g_item_table_category_names;

#endif
