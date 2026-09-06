#pragma once

#include "wiz8/vector.h"

class W8DialogBase005DC7A0;
extern "C" void Function5CF580(W8DialogBase005DC7A0* dialog, int value);

/* The released binary proves this shared dialog base and the three embedded
   member identities, but exposes none of their original source names. Address-
   qualified positional names preserve that distinction. */
class W8DialogBase005DC7A0 {
public:
    W8DialogBase005DC7A0();              /* 0x005DC7A0 */
    virtual ~W8DialogBase005DC7A0();     /* 0x005DC860 */
    virtual void vslot1();
    virtual void ResetSubobjectAndRefresh();
    virtual void vslot3();
    virtual void vslot4();
    virtual void vslot5();
    /* Slots 6, 7 and 8 of the table at 0x005EF8B0 are 0x005DC9C0, 0x005DC9F0 and
       0x005DCA70 - the three setters a derived constructor calls directly
       because its dynamic type is fixed, and that a caller holding a base
       pointer reaches through the table. */
    virtual void SetExtent(int width, int height);   /* 0x005DC9C0 */
    virtual void SetOrigin(int x, int y);            /* 0x005DC9F0 */
    virtual void SetBackground(const char* path, int flags); /* 0x005DCA70 */
    virtual unsigned char Close();
    virtual void vslot10();
    virtual void vslot11();
    virtual void ClearField41IfEnabled();
    virtual void vslot13();

    friend void Function5CF580(W8DialogBase005DC7A0* dialog, int value);

protected:
    unsigned char unknown_004[0x3d];
    unsigned char m_field_41;            /* 0x41 */
    unsigned char unknown_042[2];
    int m_field_44;                      /* 0x44 */
    unsigned char unknown_048[8];
    unsigned char m_field_50;            /* 0x50 */
    unsigned char unknown_051[3];
};                                      /* 0x54 */

class W8DialogMember005E0C40 {
public:
    W8DialogMember005E0C40();            /* 0x005E0C40 */
    ~W8DialogMember005E0C40();
    void Reset();                        /* 0x005E0E00 */

private:
    unsigned char unknown_000;           /* 0x00 */
    unsigned char unknown_001;           /* 0x01 */
    unsigned char unknown_002;           /* 0x02 */
    unsigned char unknown_003;
    int unknown_004;                     /* 0x04 */
    int unknown_008;                     /* 0x08 */
    int unknown_00c;                     /* 0x0c */
    int unknown_010;                     /* 0x10 */
    int unknown_014[4];                  /* 0x14 */
    int unknown_024;                     /* 0x24 */
    int unknown_028;                     /* 0x28 */
    int unknown_02c;                     /* 0x2c */
    int unknown_030;                     /* 0x30 */
    int unknown_034;                     /* 0x34 */
    int unknown_038;                     /* 0x38 */
    int unknown_03c;                     /* 0x3c */
    int unknown_040;                     /* 0x40 */
    int unknown_044;                     /* 0x44 */
    int unknown_048;                     /* 0x48 */
};                                      /* 0x4c */

class W8DialogMember005DB1B0 {
public:
    W8DialogMember005DB1B0();            /* 0x005DB1B0 */
    virtual ~W8DialogMember005DB1B0();   /* 0x005DB260 */

private:
    int unknown_004;
    int unknown_008;
    int unknown_00c;
    int unknown_010;
    int unknown_014;
    int m_resource_018;
    int m_resource_01c;
    int unknown_020;
    int unknown_024;
    int unknown_028;
    int unknown_02c;
    int unknown_030;
    unsigned char unknown_034;
    unsigned char unknown_035;
    unsigned char unknown_036;
    unsigned char unknown_037;
    unsigned char unknown_038;
    unsigned char unknown_039;
    unsigned char unknown_03a;
    unsigned char unknown_03b;
    unsigned char unknown_03c;
    unsigned char unknown_03d[3];
    int unknown_040;
    int unknown_044;
};                                      /* 0x48 */

class W8DialogOwned005D14D0 {
public:
    virtual ~W8DialogOwned005D14D0();
};

/* Two instances of this pointer-vector specialization are embedded in
   W8DialogMember005D14D0. */
// VTABLE: WIZ8 0x005ef898
// class W8GrowableVector<W8DialogOwned005D14D0*>

class W8DialogMember005D14D0 {
public:
    W8DialogMember005D14D0();            /* 0x005D14D0 */
    ~W8DialogMember005D14D0();           /* 0x005D1590 */
    unsigned char Function5D1AE0(unsigned int command);
    unsigned char Function5D1C00(unsigned int command);

private:
    unsigned char unknown_000[0x10];
    int unknown_010;
    int unknown_014;
    int unknown_018;
    W8GrowableVector<W8DialogOwned005D14D0*> m_vector_01c;
    W8GrowableVector<W8DialogOwned005D14D0*> m_vector_02c;
    unsigned char unknown_03c;
    unsigned char unknown_03d;
    unsigned char unknown_03e;
    unsigned char unknown_03f;
    int unknown_040;
    int unknown_044;
    int unknown_048;
    int unknown_04c;
    int unknown_050;
    unsigned char unknown_054;
    signed char unknown_055;
    unsigned char unknown_056;
    unsigned char unknown_057;
};                                      /* modeled minimum 0x58 */
