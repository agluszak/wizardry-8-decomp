#pragma once

#include "wiz8/vector.h"

class srNode;
struct W8ItemRep;

/* The original names of GrCycle's two bases are not present in the available
   source-path, binary, or Cosmic Forge evidence. Their constructor addresses
   keep these types unambiguous without inventing semantic names. */
class W8GrCycleBase004B6900 {
public:
    W8GrCycleBase004B6900();             /* 0x004B6900 */
    virtual ~W8GrCycleBase004B6900();    /* 0x004B6B60 */

protected:
    unsigned char unknown_004;           /* 0x04 */
    unsigned char unknown_005[3];
    int unknown_008;                     /* 0x08 */
    void* unknown_00c;                   /* 0x0c */
    void* unknown_010;                   /* 0x10 */
public:
    W8ItemRep* m_pRep;                   /* 0x14: typed by Engine Code\Item.cpp */
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

class W8GrCycleTarget {
public:
    virtual void vslot0();
    virtual void vslot1();
    virtual void SetCycleFrameLod(signed char cycle, signed char frame, signed char lod) = 0;

    unsigned char unknown_004[0x60];
    unsigned char m_subcycle;             /* 0x64 */
    unsigned char unknown_65[3];
    unsigned int tick_68;                 /* 0x68 */
    unsigned char unknown_6c[2];
    unsigned char flag_6e;                /* 0x6e */
    unsigned char unknown_6f;
    signed char m_bBehaviour;            /* 0x70 */
};

class W8VectorElement005ECED4;
class W8VectorElement005EC294;
class W8GrCycleShakeEvent;
class stGroundShadow;

class W8GrCycle :
    public W8GrCycleBase004B6900,
    public W8GrCycleBase00451EC0 {
public:
    W8GrCycle();
    virtual ~W8GrCycle() override;
    virtual void vslot1();
    virtual void vslot2();
    virtual void vslot3();
    virtual void vslot4();
    virtual signed char vslot5() = 0;
    virtual unsigned char IsCycleSupported(signed char cycle) = 0;
    virtual void vslot7() = 0;
    virtual void vslot8() = 0;
    virtual W8GrCycleTarget* vslot9() = 0;
    virtual void vslot10();
    virtual void vslot11(void* value);
    virtual void vslot12() = 0;
    virtual void vslot13() = 0;
    virtual void vslot14();
    virtual void SubmitCurrentAnimEntry004C3F00() = 0;

    void SetSubCycle(unsigned char subcycle);
    void SetBehaviour(signed char bBehaviour);
    void SetLights(W8GrowableVector<W8VectorElement005EC294*>* lights);
    void AddVectorElement005ECED4(W8VectorElement005ECED4* element);
    void CreateGroundShadow(int value_140, int value_13c);
    void SetGroundShadowVisible(char visible);
    void ResetRepresentation004A7420();
    unsigned char ReturnTrue004A7140(int unused);
    void SelectCycleFrameLod004A8360(signed char cycle, signed char frame, signed char lod);
    unsigned char ReplacePath004A8400(void* path);
    void SubmitTargetValue004A84A0();

private:
    int unknown_1a8;
    W8GrowableVector<W8VectorElement005EC294*>* m_plsLights; /* 0x1ac */
    W8GrowableVector<W8VectorElement005ECED4*>* m_vector_1b0; /* 0x1b0 */
    unsigned char m_fDeleteLights;        /* 0x1b4: named by GrCycle.cpp:1656 */
    unsigned char unknown_1b5;
    unsigned char unknown_1b6[2];
    W8GrowableVector<W8GrCycleShakeEvent*>* m_shake_events; /* 0x1b8 */
    unsigned char unknown_1bc;
    unsigned char enabled_1bd;
    unsigned char unknown_1be;
    unsigned char unknown_1bf;
    unsigned char unknown_1c0[0xc];
    float scale_1cc;
    stGroundShadow* m_ground_shadow;       /* 0x1d0: typed runtime class stGroundShadow */
    int unknown_1d4;
};                                      /* 0x1d8 */

static_assert(sizeof(W8GrCycle) == 0x1d8, "W8GrCycle_size_must_be_0x1d8");

unsigned char __fastcall IsSoleGrCycleForName(W8GrCycle* cycle);
unsigned char UnregisterGrCycle(W8GrCycle* cycle);
