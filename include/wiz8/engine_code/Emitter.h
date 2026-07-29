#ifndef WIZ8_ENGINE_CODE_EMITTER_H
#define WIZ8_ENGINE_CODE_EMITTER_H

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
};

/* The copy path at 0x004B87C0 and clone slot at 0x0044EDF0 establish the
   0x64-byte polymorphic root below.  Its original name is not available. */
class W8AnimRepBase005EC1D8 {
public:
    virtual ~W8AnimRepBase005EC1D8();
    virtual W8AnimRepBase005EC1D8* Clone();

protected:
    unsigned char unknown_004[0x58];
    float value_05c;
    unsigned char flag_060;
    unsigned char flag_061;
    unsigned char unknown_062[2];
};                                       /* 0x64 */

/* AnimRep.cpp's constructor and copy constructor extend the root through
   0x98.  The address suffix preserves the unresolved original class name. */
class W8AnimRep005ED050 : public W8AnimRepBase005EC1D8 {
public:
    virtual ~W8AnimRep005ED050() override;

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
    unsigned int values_074[8];
    unsigned char counter_094;
    unsigned char counter_095;
    unsigned char unknown_096[2];
};                                       /* 0x98 */

/* The 0x005ED058 table adds three pure emitter operations to the two-slot
   AnimRep hierarchy.  Concrete missile and spell hosts supply those slots. */
class W8EmitterHost : public W8AnimRep005ED050 {
public:
    virtual ~W8EmitterHost() override;
    virtual void SendToEmitter(char emitter, int arg_2, int arg_3) = 0;
    virtual void ApplyEmitterSetting(char emitter) = 0;
    virtual void StopEmitter(char emitter) = 0;

    /* 0x6c: the host is live; the spell side checks it before starting. */
    /* 0x98: the setting handed to the renderer alongside the visual. */
    unsigned char setting_98;
    unsigned char unknown_099[3];
    unsigned int value_09c;
    unsigned int value_0a0;
    /* 0xa4: which emitter is in use. Signed, and read as a byte into a
       full-width index. */
    signed char emitter_index;
    unsigned char unknown_0a5[3];
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

static_assert(sizeof(W8AnimRepBase005EC1D8) == 0x64, "W8AnimRepBase005EC1D8_size_must_be_0x64");
static_assert(sizeof(W8AnimRep005ED050) == 0x98, "W8AnimRep005ED050_size_must_be_0x98");
static_assert(sizeof(W8EmitterHost) == 0xac, "W8EmitterHost_size_must_be_0xac");
static_assert(sizeof(W8EmitterTableHost) == 0xe0, "W8EmitterTableHost_size_must_be_0xe0");

#pragma pack(pop)

#endif
