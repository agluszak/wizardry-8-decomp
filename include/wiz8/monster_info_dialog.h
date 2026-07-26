#pragma once

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
    virtual void vslot6();
    virtual void vslot7();
    virtual void vslot8();
    virtual void vslot9();
    virtual void vslot10();
    virtual void vslot11();
    virtual void ClearField41IfEnabled();
    virtual void vslot13();

protected:
    unsigned char unknown_004[0x3d];
    unsigned char m_field_41;            /* 0x41 */
    unsigned char unknown_042[0x0e];
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
    unsigned char unknown_004[0x44];
};                                      /* 0x48 */

class W8DialogMember005D14D0 {
public:
    W8DialogMember005D14D0();            /* 0x005D14D0 */
    ~W8DialogMember005D14D0();           /* 0x005D1590 */

private:
    unsigned char unknown_000[0x44];
};                                      /* modeled minimum 0x44 */

class W8MonsterInfoDialog : public W8DialogBase005DC7A0 {
public:
    virtual ~W8MonsterInfoDialog();
    virtual void vslot1();
    virtual void ResetSubobjectAndRefresh();
    virtual void vslot3();
    virtual void ClearField41IfEnabled();
    virtual void vslot13();

private:
    void* m_constructor_argument_54;      /* 0x54 */
    W8DialogMember005E0C40 m_member_58;  /* 0x58 */
    W8DialogMember005DB1B0 m_member_a4;  /* 0xa4 */
    W8DialogMember005D14D0 m_member_ec;  /* 0xec */
};                                      /* modeled minimum 0x130 */
