#ifndef WIZ8_GAMEPLAY_MODIFIERS_H
#define WIZ8_GAMEPLAY_MODIFIERS_H

/*
 * The two packed records the gameplay-modifier passes accumulate through.
 *
 * A W8EffectSlot is one running effect or condition: the party keeps twelve in
 * the status block, a monster keeps twelve more, and combat keeps nine records
 * at +0x7c1. The six 0x11-byte records at combat +0x85a share storage with
 * later independently typed fields; their originating array bound is
 * unresolved. A W8GameplayModifierBlock is the 0x67-byte
 * accumulator those slots and the worn equipment fold into, one block per
 * character plus the party-wide block in the status record.
 */

#pragma pack(push, 1)

/* One 0x11-byte effect slot. The id at +0x01 is both the condition id the
   HasCondition predicates compare and the index into the effect-visual table
   the clear path drops. Function50EDC0 scales the amount at +0x05 by the
   percentage at +0x09 before accumulating it. */
struct W8EffectSlot {
    unsigned char active;                 /* 0x00 */
    int effect_id;                        /* 0x01 */
    signed char amount;                   /* 0x05 */
    unsigned char unknown_06[3];
    unsigned int percent;                 /* 0x09 */
    float duration_0d;                    /* 0x0d */
};                                        /* 0x11 */

static_assert(sizeof(W8EffectSlot) == 0x11, "W8EffectSlot_must_be_0x11");

/* The 0x67-byte modifier accumulator. Its byte runs are fixed by the fold at
   0x0050F090: bytes 0x00..0x0b, 0x0c..0x12, 0x13..0x3b and 0x3c..0x41 are
   added, bytes 0x42..0x46 and 0x4a are boolean-OR'd, and bytes 0x47..0x49
   keep the larger value. Only the offsets another recovered pass names are
   labelled; the rest stay runs. */
struct W8GameplayModifierBlock {
    signed char value_00;                 /* 0x00: added to initiative and to displayed hand damage */
    signed char value_01;                 /* 0x01: added to the hand attack hit bonus */
    signed char value_02;                 /* 0x02 */
    signed char value_03;                 /* 0x03: added to the hand attack damage bonus */
    signed char armor_bonus_04;           /* 0x04 */
    signed char armor_bonus_05;           /* 0x05 */
    unsigned char value_06;               /* 0x06: added to damage reduction */
    signed char resistance_bonus_all;     /* 0x07: added to every resistance */
    unsigned char unknown_08[4];          /* 0x08 .. 0x0b */
    unsigned char unknown_0c[7];          /* 0x0c .. 0x12 */
    unsigned char unknown_13[0x29];       /* 0x13 .. 0x3b */
    signed char resistance_bonus[6];      /* 0x3c .. 0x41 */
    unsigned char flag_42;                /* 0x42 .. 0x44: doubled from the trait pass */
    unsigned char flag_43;
    unsigned char flag_44;
    unsigned char out_of_formation;       /* 0x45 */
    unsigned char flag_46;                /* 0x46: set by effect id 0x11 */
    unsigned char light_47;               /* 0x47: the doubled light value the sky node reads */
    unsigned char value_48;               /* 0x48: max-combined, effect id 0x21 */
    unsigned char value_49;               /* 0x49: max-combined, effect id 0x1a */
    unsigned char flag_4a;                /* 0x4a: set by effect id 0x2d, the sight light gate */
    unsigned char value_4b;               /* 0x4b: added to armor-class component 8 */
    unsigned char unknown_4c[0x1b];       /* 0x4c .. 0x66 */
};                                        /* 0x67 */

static_assert(sizeof(W8GameplayModifierBlock) == 0x67,
              "W8GameplayModifierBlock_must_be_0x67");

#pragma pack(pop)

#endif
