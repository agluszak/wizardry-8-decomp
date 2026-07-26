#pragma once

/* The original names of GrCycle's two bases are not present in the available
   source-path, binary, or Cosmic Forge evidence. Their constructor addresses
   keep these types unambiguous without inventing semantic names. */
class W8GrCycleBase004B6900 {
public:
    W8GrCycleBase004B6900();             /* 0x004B6900 */
    virtual ~W8GrCycleBase004B6900();    /* 0x004B6B60 */

private:
    unsigned char unknown_004;           /* 0x04 */
    unsigned char unknown_005[3];
    int unknown_008;                     /* 0x08 */
    void* unknown_00c;                   /* 0x0c */
    void* unknown_010;                   /* 0x10 */
    unsigned char unknown_014[4];
};                                      /* 0x18 */

class W8GrCycleBase00451EC0 {
public:
    W8GrCycleBase00451EC0();             /* 0x00451EC0 */
    virtual ~W8GrCycleBase00451EC0();    /* 0x00452120 */
    virtual void secondary_vslot1();
    virtual void secondary_vslot2();
    virtual void secondary_vslot3();
    virtual void secondary_vslot4();

private:
    unsigned char unknown_004[0x18c];
};                                      /* 0x190 */

struct W8GrCycleTarget {
    unsigned char unknown_00[0x70];
    signed char m_bBehaviour;            /* 0x70 */
};

class W8GrCycle :
    public W8GrCycleBase004B6900,
    public W8GrCycleBase00451EC0 {
public:
    virtual ~W8GrCycle();
    virtual void vslot1();
    virtual void vslot2();
    virtual void vslot3();
    virtual void vslot4();
    virtual void vslot5() = 0;
    virtual void vslot6() = 0;
    virtual void vslot7() = 0;
    virtual void vslot8() = 0;
    virtual W8GrCycleTarget* vslot9() = 0;
    virtual void vslot10();
    virtual void vslot11();
    virtual void vslot12() = 0;
    virtual void vslot13() = 0;
    virtual void vslot14();
    virtual void vslot15() = 0;

    void SetBehaviour(signed char bBehaviour);

private:
    unsigned char unknown_1a8[0x30];
};                                      /* modeled minimum 0x1d8 */
