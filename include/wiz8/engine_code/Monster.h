#ifndef WIZ8_ENGINE_CODE_MONSTER_H
#define WIZ8_ENGINE_CODE_MONSTER_H

#include "surrender/srMath.h"
#include "wiz8/geometry.h"
#include "wiz8/local_code/MonsterGroup.h"

struct W8AnimObj;

typedef struct W8Monster W8Monster;

enum { W8_MONSTER_CYCLE_COUNT = 27 };

/* Partial layout of the engine object referenced by cycle 18. The Monster
   wrappers at 0x004C5780..0x004C5AA0 establish the timestamp, animation state,
   pending cycle, and scale fields below. */
/* Sixteen bytes the cycle runtime record carries at 0x04c, written as one block
   by the setter at 0x004C5AD0. That setter takes the block by value and VC6
   copies it with the interleaved two-register rotation it uses for a struct
   assignment, rather than the sequential load/store pairs four separate scalar
   parameters would emit - which is what makes this one object and not four.
   Nothing observed so far types its contents. */
typedef struct W8MonsterRuntimeBlock4C {
    unsigned int values[4];
} W8MonsterRuntimeBlock4C;                  /* 0x10 */

struct W8MonsterCycleRuntime {
    unsigned char unknown_000[0x4c];
    W8MonsterRuntimeBlock4C block_04c;      /* 0x04c */
    unsigned char unknown_05c[0xa];
    unsigned short value_066;               /* 0x066: cleared when a cycle starts */
    unsigned int animation_timestamp;       /* 0x068 */
    unsigned char unknown_06c;
    unsigned char animating;                /* 0x06d */
    unsigned char unknown_06e[3];
    signed char behaviour;                   /* 0x071: asserted BEHAVIOUR_FIRST..LAST */
    unsigned char unknown_072[0x32];
    /* 0x0a4 is copied straight into pending_cycle when nothing is pending, so
       it is a cycle number and carries pending_cycle's own signedness - the
       fallback the setter at 0x004C6C00 applies. */
    signed char value_0a4;                   /* 0x0a4 */
    unsigned char unknown_0a5;
    unsigned char value_0a6;                 /* 0x0a6: written by that same setter */
    signed char pending_cycle;               /* 0x0a7 */
    unsigned char unknown_0a8[0x514];
    unsigned char flag_5bc;                 /* 0x5bc */
    unsigned char unknown_5bd[0x33];
    float scale;                             /* 0x5f0 */
    float minimum_scale;                     /* 0x5f4 */
    float maximum_scale;                     /* 0x5f8 */
};

struct W8MonsterCycle {
    union {
        unsigned int flags_00;              /* 0x00: cycle 19 bit 7 set by 0x004e67a0 */
        struct {
            unsigned char flag_00;
            unsigned char flag_01;
            unsigned char state_02;         /* 0x02: cycle 17 state saved across activation */
            unsigned char flag_03;
        } bytes;
    };
    union {
        unsigned int num_subs_04;           /* 0x04: IsCycleSupported tests the complete field */
        struct {
            unsigned char ubNumSubs;        /* 0x04: GetNumSubsPerCycle returns the low byte */
            unsigned char unknown_05[3];
        } count;
    };
    union {
        unsigned int value_08;              /* 0x08: copied by 0x004c5870 */
        unsigned int location_id_08;        /* 0x08: consumed as a location id by 0x004c6240 */
        struct {
            unsigned char unknown_08;
            unsigned char unknown_09;       /* 0x09: cleared for cycle 22 by 0x004e6130 */
            unsigned char unknown_0a[2];
        } bytes_08;
    };
    union {
        W8MonsterCycleRuntime* runtime;      /* 0x0c: cycle 18's shared engine state */
        W8AnimObj** animation_objects;       /* 0x0c: per-subcycle AnimObj table */
    };
};                                          /* 0x10 */

struct W8MonsterPolymorphicSubobject18 {
    /* 0x00, which is Monster +0x18: the additional vftable at 0x005ED218,
       five slots. The root-relative placement is proven; whether this
       polymorphic subobject is a base or an embedded member is not. */
    void* vptr;
    unsigned char unknown_04[8];
    unsigned int flags_0c;                  /* 0x0c: Monster +0x24 */
    unsigned char unknown_10[0x4c];
    int value_5c;                           /* 0x5c: Monster +0x74 */
    unsigned char unknown_60[0x20];
    signed char animation_index_80;          /* 0x80: Monster +0x98 */
    unsigned char unknown_81[3];
    /* 0x84, which is Monster +0x9c: the monster's own extent. Every range test
       subtracts it from the centre-to-centre distance, so it is a radius
       rather than a diameter or a bounding box. */
    float radius_84;
    unsigned char state_a0;                 /* 0x88: Monster +0xa0 */
    unsigned char unknown_89[3];
    signed char m_bCurrentCycle;             /* 0x8c: Monster +0xa4 */
    signed char current_subcycle_8d;         /* 0x8d: Monster +0xa5 */
    unsigned char unknown_8e[6];

    srVector3T<float> GetPosition();

    /* Six methods the Monster.cpp forwarder family reaches on this subobject.
       Each forwarder derives the receiver with `lea ecx, [monster + 0x18]`,
       which is what proves they are this object's methods and not the
       monster's; only the offset is proved that way, so the names stay
       address-qualified wherever the body does not say what a member means.

       These bodies read members at 0x25, 0x68, 0xc4, 0xd0, 0xf4 and 0x120,
       past the 0x94 this model covers - and Monster's cycle array is placed at
       Monster +0xac, which is subobject +0x94. Either this object is larger
       than the extent below and the cycle array lies inside it, or the two
       placements do not both hold. 0x120 is not on a cycle boundary, so
       neither reading is settled here. The disagreement is recorded rather
       than resolved: declaring a method claims nothing about the extent, and
       the static_assert below still pins what is proved. */
    void SetValue120(float value);            /* 0x00453C50 */
    float GetValue120();                      /* 0x00453C60 */
    unsigned char Function452630(const W8Position* position);  /* 0x00452630 */
    void Function453690(void* argument);      /* 0x00453690 */
    /* Reaches the object at 0x68 and raises or clears its own flag at 0x38. */
    void SetObject68Flag38(char value);       /* 0x004537C0 */
    /* Two more that take a position and read the three-dword block at 0x100.
       Which one 0x004C6240 picks is the whole of what its flag argument
       decides; nothing in either body says how they differ. */
    void Function454040(const W8Position* position);  /* 0x00454040 */
    void Function453F30(const W8Position* position);  /* 0x00453F30 */
    /* Writes the byte at 0x25 and, when it is cleared, releases the object at
       0x68; when it is set, zeroes the member at 0xf4 instead. */
    void SetFlag25(char value);               /* 0x004531F0 */
};                                          /* 0x94: through the cycle array at Monster +0xac */

/* Partial source model of the Monster class (0x628 bytes, vtable 0x005ed200,
   constructor 0x004bea20). Only members proven by recovered consumers are
   modelled here; unresolved layout remains original-binary analysis. */
struct W8Monster {
    /* 0x000: the root vtable at 0x005ED200, six slots. Slot zero is the
       compiler-generated scalar-deleting wrapper at 0x004BEBA0, which calls
       the complete destructor at 0x004BEE50. The additional vptr at +0x18
       remains an unresolved polymorphic subobject. */
    virtual ~W8Monster();
    unsigned char unknown_004[0x0c];
    void* linked_objects_010;                /* 0x010: collection traversed by 0x004c5870 */
    unsigned char unknown_014[4];
    W8MonsterPolymorphicSubobject18 polymorphic_subobject_18; /* 0x018: proven placement */
    W8MonsterCycle m_cycles[W8_MONSTER_CYCLE_COUNT]; /* 0x0ac .. 0x25c */
    /* Two further runs of 27 adjacent 0x10-byte subobjects, the same shape as
       the array above. Nothing proves they are the same type, so they stay
       opaque rather than borrowing its name. */
    unsigned char subobject_array_25c[0x24];    /* 0x25c */
    W8MonsterFormation formation;               /* 0x280: from the owning group */
    unsigned char subobject_array_28c[0x180];   /* 0x28c */
    unsigned char subobject_array_40c[0x1b0];   /* 0x40c */
    unsigned char tail_fields_5bc[0x6c];        /* 0x5bc: fields seen through 0x624 */

    unsigned char GetNumSubsPerCycle(signed char bCycle);
    void SubmitCycleAnimValue004BF970(signed char cycle);
    /* 0x004C4660. A method, not the free function an earlier reading assumed:
       it takes its receiver in ECX and IsDying calls it without reloading ECX
       at all, relying on `this` already being there. The query selector is
       bounded at nine by the body's own `ja` against the jump table. */
    int Query(int query);
    void SetRuntimeValueA6(unsigned char value);   /* 0x004C6C00 */
    unsigned char IsDying();
    unsigned char Function4C2CF0(signed char cycle);
    void Function4C50F0();
    int Function4C6A50();
    void Function4C6990(int value);
};

/* The constructor at 0x004BEA20 initialises through 0x624 and its sole caller
   allocates this much, so the extent is proven even though most of it is not.
   Asserting it here is what stops a field edit from silently shortening the
   object. */
static_assert(sizeof(W8Monster) == 0x628, "W8Monster_size_must_be_0x628");

static_assert(sizeof(W8MonsterCycle) == 0x10, "W8MonsterCycle_size_must_be_0x10");
static_assert(sizeof(W8MonsterPolymorphicSubobject18) == 0x94, "W8MonsterPolymorphicSubobject18_size_must_be_0x94");

#endif
