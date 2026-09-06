#ifndef WIZ8_ENGINE_CODE_ANIM_REP_H
#define WIZ8_ENGINE_CODE_ANIM_REP_H

#include "surrender/srMath.h"

#pragma pack(push, 1)

/* Copy constructors establish these aggregate boundaries, but no independent
   witness yet establishes their original semantic types. Keep their names
   positional until consumers prove whether they are vectors, matrices, or
   another animation representation. */
struct W8AnimRepValue3 {
    unsigned int value_00;
    unsigned int value_04;
    unsigned int value_08;
};

struct W8AnimRepValue4 {
    unsigned int value_00;
    unsigned int value_04;
    unsigned int value_08;
    unsigned int value_0c;
};

/* The copy path at 0x004B87C0 and clone slot at 0x0044EDF0 establish the
   0x64-byte polymorphic root below. Its original name is not available. */
class W8AnimRepBase005EC1D8 {
public:
    W8AnimRepBase005EC1D8();
    W8AnimRepBase005EC1D8(const W8AnimRepBase005EC1D8& other);
    virtual ~W8AnimRepBase005EC1D8() {}
    virtual W8AnimRepBase005EC1D8* Clone();

    void SetLocation004B8850(const srVector3T<float>* location);
    void GetLocation004B8890(srVector3T<float>* location) const;
    void GetLocalLocation004B88B0(srVector3T<float>* location) const;
    void SetRotation004B88D0(const srMatrix3T<float>* rotation);
    void GetRotation004B88F0(srMatrix3T<float>* rotation);

public:
    srVector3T<float> location_004;
    srVector3T<float> local_location_010;
    srVector3T<float> parent_location_01c;
    srMatrix3T<float> rotation_028;
    W8AnimRepValue4 value_04c;
    float value_05c;
    unsigned char flag_060;
    unsigned char flag_061;
    unsigned char unknown_062[2];
};

/* AnimRep.cpp's constructor and copy constructor extend the root through
   0x98. The address suffix preserves the unresolved original class name. */
class W8AnimRep005ED050 : public W8AnimRepBase005EC1D8 {
public:
    W8AnimRep005ED050();
    W8AnimRep005ED050(const W8AnimRep005ED050& other);
    virtual ~W8AnimRep005ED050() override;
    void SetFrameMethod004B55C0(signed char method);

public:
    unsigned char flag_064;
    unsigned char unknown_065;
    unsigned short value_066;
    unsigned int timer_068;
    unsigned char active;
    unsigned char flag_06d;
    unsigned char flag_06e;
    unsigned char flag_06f;
    unsigned char flag_070;
    unsigned char behaviour_071;
    unsigned char unknown_072[2];
    W8AnimRepValue3 value_074;
    W8AnimRepValue3 value_080;
    unsigned int value_08c;
    unsigned int value_090;
    unsigned char counter_094;
    unsigned char counter_095;
    unsigned char unknown_096[2];
};

static_assert(sizeof(W8AnimRepValue3) == 0x0c,
              "W8AnimRepValue3_size_must_be_0x0c");
static_assert(sizeof(W8AnimRepValue4) == 0x10,
              "W8AnimRepValue4_size_must_be_0x10");
static_assert(sizeof(W8AnimRepBase005EC1D8) == 0x64,
              "W8AnimRepBase005EC1D8_size_must_be_0x64");
static_assert(sizeof(W8AnimRep005ED050) == 0x98,
              "W8AnimRep005ED050_size_must_be_0x98");

#pragma pack(pop)

#endif
