#pragma once

class srNode;

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

    void configureStartupRange(float range);
    void configureStartupDepth(float near_depth, float far_depth);

private:
    /* The secondary-base constructor treats this payload as 99 contiguous
       four-byte fields.  Keeping that observed granularity lets startup set
       the six proven range fields without claiming names for the rest. */
    unsigned int unknown_004[98];
    srNode* node_18c;                    /* 0x18c: constructed srNode */
};                                      /* 0x190 */

struct W8GrCycleTarget {
    unsigned char unknown_00[0x64];
    unsigned char m_subcycle;             /* 0x64 */
    unsigned char unknown_65[0xb];
    signed char m_bBehaviour;            /* 0x70 */
};

class W8VectorElement005ECED4;
class W8Vector005ECED4;
class W8Vector005EC294;
class stGroundShadow;

class W8GrCycle :
    public W8GrCycleBase004B6900,
    public W8GrCycleBase00451EC0 {
public:
    virtual ~W8GrCycle() override;
    virtual void vslot1();
    virtual void vslot2();
    virtual void vslot3();
    virtual void vslot4();
    virtual signed char vslot5() = 0;
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

    void SetSubCycle(unsigned char subcycle);
    void SetBehaviour(signed char bBehaviour);
    void SetLights(W8Vector005EC294* lights);
    void AddVectorElement005ECED4(W8VectorElement005ECED4* element);
    void CreateGroundShadow(int value_140, int value_13c);
    void SetGroundShadowVisible(char visible);

private:
    unsigned char unknown_1a8[0x4];
    W8Vector005EC294* m_plsLights;        /* 0x1ac: named by GrCycle.cpp:1656 */
    W8Vector005ECED4* m_vector_1b0;       /* 0x1b0 */
    unsigned char m_fDeleteLights;        /* 0x1b4: named by GrCycle.cpp:1656 */
    unsigned char unknown_1b5[0x1b];
    stGroundShadow* m_ground_shadow;       /* 0x1d0: typed runtime class stGroundShadow */
    unsigned char unknown_1d4[4];
};                                      /* modeled minimum 0x1d8 */
