#pragma once

#pragma pack(push, 1)

/* One live item stack carried by a character, party pool, or world item. */
struct W8ItemInstance {
    int item_id;
    unsigned char stack_count;           /* 0x04: quantity-kind 1 */
    unsigned char uses_or_charges;       /* 0x05: quantity-kinds 2 through 4 */
    unsigned char identified;
    unsigned char unknown_07[3];
    /* 0x0a: the binding has already been announced for this instance, which is
       what stops the log line repeating. */
    unsigned char bind_announced;
    /* 0x0b: the instance is bound to its wearer. Raised when a binds-on-equip
       item is worn and read by the predicates that refuse to take it off. */
    unsigned char bound;
};                                      /* 0x0c */

#pragma pack(pop)

void NormalizeItemQuantityKind(W8ItemInstance* item);
int __cdecl CompareItemsForPool(const void* first, const void* second);
