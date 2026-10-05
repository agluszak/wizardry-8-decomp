#ifndef WIZ8_LAYOUTS_ITEM_TABLES_H
#define WIZ8_LAYOUTS_ITEM_TABLES_H

#include "wiz8/attack_modes.h"
#include "wiz8/dice.h"

#pragma pack(push, 1)

struct W8ItemTableEntry {
    short selector;      /* 0x00: zero disables the slot */
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

/* Equipment classes indexed by the retail Assay name table. Labels
   1087..1112 name classes 0..25; labels 1114..1119 name classes 26..31.
   The serialized field remains a byte. */
enum W8ItemEquipClass {
    W8_ITEM_EQUIP_CLASS_SHORT_WEAPON = 0,
    W8_ITEM_EQUIP_CLASS_EXTENDED_WEAPON = 1,
    W8_ITEM_EQUIP_CLASS_THROWN_WEAPON = 2,
    W8_ITEM_EQUIP_CLASS_RANGED_WEAPON = 3,
    W8_ITEM_EQUIP_CLASS_AMMUNITION = 4,
    W8_ITEM_EQUIP_CLASS_SHIELD = 5,
    W8_ITEM_EQUIP_CLASS_TORSO = 6,
    W8_ITEM_EQUIP_CLASS_LEGS = 7,
    W8_ITEM_EQUIP_CLASS_HEAD = 8,
    W8_ITEM_EQUIP_CLASS_HANDS = 9,
    W8_ITEM_EQUIP_CLASS_FEET = 10,
    W8_ITEM_EQUIP_CLASS_MISC = 11,
    W8_ITEM_EQUIP_CLASS_CLOAK = 12,
    W8_ITEM_EQUIP_CLASS_INSTRUMENT = 13,
    W8_ITEM_EQUIP_CLASS_GADGET = 14,
    W8_ITEM_EQUIP_CLASS_MISC_MAGIC = 15,
    W8_ITEM_EQUIP_CLASS_POTION = 16,
    W8_ITEM_EQUIP_CLASS_BOMB = 17,
    W8_ITEM_EQUIP_CLASS_POWDER = 18,
    W8_ITEM_EQUIP_CLASS_SPELLBOOK = 19,
    W8_ITEM_EQUIP_CLASS_SCROLL = 20,
    W8_ITEM_EQUIP_CLASS_FOOD = 21,
    W8_ITEM_EQUIP_CLASS_DRINK = 22,
    W8_ITEM_EQUIP_CLASS_KEY = 23,
    W8_ITEM_EQUIP_CLASS_WRITING = 24,
    W8_ITEM_EQUIP_CLASS_OTHER = 25,
    W8_ITEM_EQUIP_CLASS_DAGGER = 26,
    W8_ITEM_EQUIP_CLASS_LONG_SWORD = 27,
    W8_ITEM_EQUIP_CLASS_BIPENNIS = 28,
    W8_ITEM_EQUIP_CLASS_BATTLE_AXE = 29,
    W8_ITEM_EQUIP_CLASS_FLAIL = 30,
    W8_ITEM_EQUIP_CLASS_MACE = 31,
};

/* Item flag labels 1255..1262 in the retail string table name bits 0..7.
   The serialized flags field remains a byte. */
enum W8ItemFlag {
    W8_ITEM_FLAG_AUTO_IDENTIFY = 0x01,
    W8_ITEM_FLAG_NO_DISCARD = 0x02, /* Crucial Item */
    W8_ITEM_FLAG_TWO_HANDED = 0x04,
    W8_ITEM_FLAG_OFF_HAND_ALLOWED = 0x08, /* Secondary Weapon */
    W8_ITEM_FLAG_MERCHANT_CANNOT_SELL = 0x10,
    W8_ITEM_FLAG_CONTAINER = 0x20,
    W8_ITEM_FLAG_MUST_EQUIP_TO_USE = 0x40,
    W8_ITEM_FLAG_NEVER_DEPLETES = 0x80
};

/* Assay quantity labels 1264..1268. Value 5 has no label; the database
   domain is kept open rather than assigning it an inferred meaning. */
enum W8ItemQuantityKind {
    W8_ITEM_QUANTITY_NONE = 0,
    W8_ITEM_QUANTITY_STACK = 1,
    W8_ITEM_QUANTITY_CHARGES = 2,
    W8_ITEM_QUANTITY_USES = 3,
    W8_ITEM_QUANTITY_SHOTS = 4
};

/* Item categories the usability rules distinguish. Three is the spell source
   the magic code already names; six and eight both cast the record's spell but
   read a different profession level to decide whether the caster is strong
   enough for it. */
enum W8ItemCategory {
    W8_ITEM_CATEGORY_SPELL_SOURCE = 3,
    W8_ITEM_CATEGORY_CASTER_ITEM_6 = 6,
    W8_ITEM_CATEGORY_CASTER_ITEM_8 = 8
};

struct W8ItemDatabaseRecord {
    wchar_t display_name[30]; /* 0x000 */
    /* 0x03c: the item number the Wizardry 7 import matches imported item ids
       against (Party Import.cpp). */
    short legacy_item_number;
    unsigned char equip_class;              /* 0x03e: W8ItemEquipClass */
    unsigned short unidentified_name_index; /* 0x03f */
    unsigned char flags;                    /* 0x041: W8ItemFlag bits */
    unsigned char category;                 /* 0x042: W8ItemCategory */
    unsigned char unknown_043[3];
    signed char weapon_skill; /* 0x046: -1 when the item grants none */
    /* 0x047: the W8RangeCategory band the weapon attacks at. GetCharAttackRange
       returns it, the Assay caption names it, and paired weapons/ammunition
       must share it. */
    unsigned char range_category;
    signed char attack_damage_bonus; /* 0x048 */
    signed char attack_hit_bonus;    /* 0x049 */
    W8Dice damage_dice;              /* 0x04a */
    unsigned short attack_flags;
    /* 0x050..0x05f: the item's missile-attack modifier block; the missile
       resolver sums it byte-wise across the wielded and paired weapons into
       the fired effect definition's condition_chances. */
    unsigned char missile_values[W8_ATTACK_EFFECT_COUNT];
    /* 0x060: the item's missile bonus, summed across both weapons into the
       effect definition's value_1c. */
    unsigned char missile_magnitude;
    /* 0x061: the monster kind the weapon slays for an extra damage die,
       compared against W8MonsterRecord::kind by the character damage
       resolver; 0xff means the weapon slays nothing. */
    unsigned char slays_kind;
    signed char armor_class_bonus; /* 0x062 */
    unsigned char spell_id;        /* 0x063 */
    /* 0x064: the cast spell's power level - Assay prints "(Pwr %d)" and the
       use path consumes it as the casting power. */
    unsigned char spell_power;
    unsigned char unknown_065;
    unsigned char quantity_kind; /* 0x066: W8ItemQuantityKind */
    W8Dice initial_quantity;     /* 0x067 */
    /* The stack merge path clamps quantity-kind 1 items to this byte. */
    unsigned char maximum_quantity; /* 0x06b */
    /* 0x06c..0x06e: three per-item modifier bytes the equipment fold adds to
       the derived block's own unknowns; 0x06f..0x074 are the six resistance
       bonuses it sums and clamps. */
    signed char health_regen_bonus;
    signed char stamina_regen_bonus;
    signed char spell_regen_bonus;
    signed char resistance_bonus[6]; /* 0x06f .. 0x074 */
    unsigned char property;          /* assay special-property label index */
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
    int swings_bonus;
    /* 0x0b1/0x0b3: the item's (index, value) modifier pairs the equipment
       fold adds to the derived block's two byte tables. 0xff is no pair. */
    signed char modifier_0b1_index;
    signed char modifier_0b1_value;
    signed char modifier_0b3_index;
    signed char modifier_0b3_value;
    unsigned char unknown_0b5[4]; /* 0x0b5 .. 0x0b8 */
    int merge_component_a;        /* 0x0b9: first item kind MergeItems combines into this one */
    int merge_component_b;        /* 0x0bd: the other kind; either order is accepted */
    /* 0x0c1: the item's material index; combat sound reads it on the struck
       item to pick the impact table's material column (0..11). */
    int material;
    /* 0x0c5: the weapon's attack sound class; combat sound bounds it against
       the 38-entry swing table and the 28 impact rows. */
    int weapon_sound_class;
    signed char merge_skill;           /* 0x0c9: skill required to create this item, -1 for none */
    unsigned char merge_skill_level;   /* 0x0ca: level of merge_skill required */
    unsigned char editor_excluded; /* hidden from the MIPE item list */
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
