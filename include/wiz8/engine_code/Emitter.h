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

/* The record the emitters hang from. */
class W8EmitterHost {
public:
    unsigned char unknown_00[0x6c];
    /* 0x6c: the host is live; the spell side checks it before starting. */
    unsigned char active;
    unsigned char unknown_6d[0x2b];
    /* 0x98: the setting handed to the renderer alongside the visual. */
    unsigned char setting_98;
    unsigned char unknown_99[0xb];
    /* 0xa4: which emitter is in use. Signed, and read as a byte into a
       full-width index. */
    signed char emitter_index;
    unsigned char unknown_a5[0x33];
    /* 0xd8: the emitters. The count is derived rather than stored - both
       counters test exactly these two for null, which is what bounds it. */
    W8Emitter* emitters[2];              /* 0xd8 */
};

#pragma pack(pop)

#endif
