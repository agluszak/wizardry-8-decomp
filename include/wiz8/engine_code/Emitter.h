#ifndef WIZ8_ENGINE_CODE_EMITTER_H
#define WIZ8_ENGINE_CODE_EMITTER_H

struct W8Position;
class srModelInstance;
template<class T> class srMatrix3T;
template<class T> class srVector3T;

/*
 * The emitter record Engine Code\Missile.cpp and Engine Code\Spells.cpp both
 * hang their visuals off. The two files carry the pointer at different
 * offsets - a missile at 0x1dc, a spell at 0x1e0 - but reach identical fields
 * through it, and the four accessors on each side are the same bodies one
 * offset apart, which is what makes it one record rather than two.
 */

#pragma pack(push, 1)

/* One emitter. Only the field the reach-through accessors read is
   established. */
class W8Emitter {
public:
    unsigned char unknown_00[8];
    float value_08;                      /* 0x08 */
    unsigned char unknown_0c[0x0c];
    void* values_18[4];                  /* 0x18 */
};

/* Copy constructors establish these aggregate boundaries, but no independent
   witness yet establishes their original semantic types.  Keep their names
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

struct W8AnimRepValue9 {
    unsigned int value_00;
    unsigned int value_04;
    unsigned int value_08;
    unsigned int value_0c;
    unsigned int value_10;
    unsigned int value_14;
    unsigned int value_18;
    unsigned int value_1c;
    unsigned int value_20;
};

/* The copy path at 0x004B87C0 and clone slot at 0x0044EDF0 establish the
   0x64-byte polymorphic root below.  Its original name is not available. */
class W8AnimRepBase005EC1D8 {
public:
    W8AnimRepBase005EC1D8();
    W8AnimRepBase005EC1D8(const W8AnimRepBase005EC1D8& other);
    virtual ~W8AnimRepBase005EC1D8();
    virtual W8AnimRepBase005EC1D8* Clone();

    void SetLocation004B8850(const W8Position* location);
    void GetLocation004B8890(srVector3T<float>* location);
    void GetRotation004B88F0(srMatrix3T<float>* rotation);

public:
    W8AnimRepValue3 value_004;
    W8AnimRepValue3 value_010;
    W8AnimRepValue3 value_01c;
    W8AnimRepValue9 value_028;
    W8AnimRepValue4 value_04c;
    float value_05c;
    unsigned char flag_060;
    unsigned char flag_061;
    unsigned char unknown_062[2];
};                                       /* 0x64 */

/* AnimRep.cpp's constructor and copy constructor extend the root through
   0x98.  The address suffix preserves the unresolved original class name. */
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
    unsigned char active;               /* 0x6c */
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
};                                       /* 0x98 */

/* The 0x005ED058 table adds three pure emitter operations to the two-slot
   AnimRep hierarchy.  Concrete missile and spell hosts supply those slots. */
class W8EmitterHost : public W8AnimRep005ED050 {
public:
    W8EmitterHost();
    W8EmitterHost(const W8EmitterHost& other);
    virtual ~W8EmitterHost() override;
    virtual srModelInstance* SetCycleFrameLod(
        signed char cycle, int frame, int lod) = 0;
    virtual unsigned int ApplyEmitterSetting(char emitter) = 0;
    virtual void StopEmitter(char emitter) = 0;

    /* 0x6c: the host is live; the spell side checks it before starting. */
    /* 0x98: the setting handed to the renderer alongside the visual. */
    unsigned char setting_98;
    unsigned char unknown_099[3];
    unsigned int value_09c;
    unsigned int value_0a0;
    /* The shared emitter host treats this as an emitter/sub-entry quartet.
       MonsterRep gives the same four bytes the cycle meanings witnessed by
       Monster.cpp.  Keeping both views here models the real inherited storage
       instead of inventing a second runtime object at Monster +0x18. */
    union {
        struct {
            signed char emitter_index;       /* 0xa4 */
            unsigned char emitter_subindex;  /* 0xa5 */
            unsigned char emitter_value_a6;  /* 0xa6 */
            signed char emitter_value_a7;    /* 0xa7 */
        } emitter;
        struct {
            signed char current_cycle;       /* 0xa4 */
            signed char current_subcycle;    /* 0xa5 */
            unsigned char runtime_value_a6;  /* 0xa6 */
            signed char pending_cycle;       /* 0xa7 */
        } monster;
    } selection;
    unsigned int value_0a8;
};                                       /* 0xac */

/* Both recovered consumers directly address the first two emitters at 0xd8;
   the concrete host families may extend the table beyond those two entries. */
class W8EmitterTableHost : public W8EmitterHost {
public:
    unsigned char unknown_0ac[0x2c];
    /* 0xd8: the emitters. The count is derived rather than stored - both
       counters test exactly these two for null, which is what bounds it. */
    W8Emitter* emitters[2];              /* 0xd8 */
};                                       /* 0xe0 */

static_assert(sizeof(W8AnimRepValue3) == 0x0c, "W8AnimRepValue3_size_must_be_0x0c");
static_assert(sizeof(W8AnimRepValue4) == 0x10, "W8AnimRepValue4_size_must_be_0x10");
static_assert(sizeof(W8AnimRepValue9) == 0x24, "W8AnimRepValue9_size_must_be_0x24");
static_assert(sizeof(W8AnimRepBase005EC1D8) == 0x64, "W8AnimRepBase005EC1D8_size_must_be_0x64");
static_assert(sizeof(W8AnimRep005ED050) == 0x98, "W8AnimRep005ED050_size_must_be_0x98");
static_assert(sizeof(W8EmitterHost) == 0xac, "W8EmitterHost_size_must_be_0xac");
static_assert(sizeof(W8EmitterTableHost) == 0xe0, "W8EmitterTableHost_size_must_be_0xe0");

#pragma pack(pop)

#endif
