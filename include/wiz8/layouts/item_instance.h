#ifndef WIZ8_LAYOUTS_ITEM_INSTANCE_H
#define WIZ8_LAYOUTS_ITEM_INSTANCE_H

#pragma pack(push, 1)

/* One live item stack carried by a character, party pool, or world item. */
struct W8ItemInstance {
    /* 0x00: the Items.dbs record id, spelled iItemNo by the
       pPC->EquippedItem[uiWeaponSlot].iItemNo assertion. */
    int iItemNo;
    unsigned char stack_count;     /* 0x04: quantity-kind 1 */
    unsigned char uses_or_charges; /* 0x05: quantity-kinds 2 through 4 */
    bool identified;
    /* 0x07: the spell hint displays even before full identification. */
    unsigned char spell_hint_07;
    unsigned char unknown_08;
    /* 0x09 bit0: the item's one-shot use effect has fired. */
    bool effect_used_09;
    /* 0x0a: the binding has already been announced for this instance, which is
       what stops the log line repeating. */
    unsigned char bind_announced;
    /* 0x0b: the instance is bound to its wearer. Raised when a binds-on-equip
       item is worn and read by the predicates that refuse to take it off. */
    bool bound;
}; /* 0x0c */

#pragma pack(pop)

static_assert(sizeof(W8ItemInstance) == 0x0c, "W8ItemInstance_must_be_0x0c");

#endif
