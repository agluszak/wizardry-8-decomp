#ifndef WIZ8_CHARACTER_H
#define WIZ8_CHARACTER_H

/* The eighteen conditions a character can be under, indexed directly into
   W8Character::condition_turns. Only the ones a recovered body names are
   spelled out; the rest keep their numbers. */
enum {
    /* Twenty entries: the per-character copier walks all twenty, while the
       sweep that lifts everything stops at eighteen because the last two are
       not the kind a rest clears. */
    W8_CONDITION_COUNT = 20,
    W8_CONDITION_CLEARABLE_COUNT = 18,
    W8_CONDITION_FATIGUE_DOUBLED = 2,
    W8_CONDITION_LOAD_EASED = 5,
    /* Seven is the one condition that carries a second value alongside its
       duration, which both copiers special-case. */
    W8_CONDITION_WITH_ARGUMENT = 7,
    W8_CONDITION_SPELLCASTING_BLOCKED = 8,
    W8_CONDITION_HOSTILE = 0xd,
    W8_CONDITION_EXHAUSTED = 0x11,
    W8_CONDITION_EQUIPMENT_UNLOCKED = 18,
    /* The duration that means "until lifted". */
    W8_CONDITION_INDEFINITE = 9999
};

/* One enchantment slot. Both a character and a monster carry eight of them,
   and both clear a slot by zeroing all three dwords at once. */
typedef struct W8Enchantment {
    int value_00;
    int value_04;
    /* 0x08: the field the topmost-slot scan reads and the one the fatigue path
       consults on slot five. */
    int value_08;
} W8Enchantment;                          /* 0x0c */

#endif
