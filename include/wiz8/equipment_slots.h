#pragma once

/* W8Character::equipment index domain. The five body-location slots are fixed
   by CalcArmorClasses' {0,4,10,5,11} location table and the per-location hit
   weights; GetItemDefaultEquipSlot independently fixes the torso/legs/head/
   hands/feet classes. The inventory paper-doll regions identify the two
   stacked accessory cells and the cloak cell, while the four weapon cells are
   already established by the hand-pairing code. */
enum W8EquipSlot {
    W8_EQUIP_SLOT_NONE = -1,
    W8_EQUIP_SLOT_HEAD = 0,
    W8_EQUIP_SLOT_MISC_1 = 1,
    W8_EQUIP_SLOT_MISC_2 = 2,
    W8_EQUIP_SLOT_CLOAK = 3,
    W8_EQUIP_SLOT_TORSO = 4,
    W8_EQUIP_SLOT_HANDS = 5,
    W8_EQUIP_SLOT_PRIMARY_WEAPON = 6,
    W8_EQUIP_SLOT_SECONDARY_WEAPON = 7,
    W8_EQUIP_SLOT_ALTERNATE_PRIMARY_WEAPON = 8,
    W8_EQUIP_SLOT_ALTERNATE_SECONDARY_WEAPON = 9,
    W8_EQUIP_SLOT_LEGS = 10,
    W8_EQUIP_SLOT_FEET = 11,
    W8_EQUIP_SLOT_COUNT = 12
};
