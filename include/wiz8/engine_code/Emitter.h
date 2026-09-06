#ifndef WIZ8_ENGINE_CODE_EMITTER_H
#define WIZ8_ENGINE_CODE_EMITTER_H

#include "wiz8/engine_code/AnimRep.h"

struct W8AniMesh;
struct W8AnimObj;
class srModelInstance;

/*
 * The emitter record Engine Code\Missile.cpp and Engine Code\Spells.cpp both
 * hang their visuals off. The two files carry the pointer at different
 * offsets - a missile at 0x1dc, a spell at 0x1e0 - but reach identical fields
 * through it, and the four accessors on each side are the same bodies one
 * offset apart, which is what makes it one record rather than two.
 */

#pragma pack(push, 1)

/* The 0x005ED058 table adds three pure emitter operations to the two-slot
   AnimRep hierarchy.  Concrete missile and spell hosts supply those slots. */
class W8EmitterHost : public W8AnimRep005ED050 {
public:
    W8EmitterHost();
    W8EmitterHost(const W8EmitterHost& other);
    virtual ~W8EmitterHost() override;
    /* All three are chars: neither override widens them, and both
       0x004A8360 and 0x004A7470 push the containing dword unextended. */
    virtual srModelInstance* SetCycleFrameLod(
        signed char cycle, signed char frame, signed char lod) = 0;
    virtual unsigned int ApplyEmitterSetting(char emitter) = 0;
    /* Not a stop: both overrides tail-return AnimObjEntry004A1660's result,
       and GrCycle's 0x004A7470 hands that result straight to
       AniMeshSetFlag10004B6860, which types it. */
    virtual W8AniMesh* GetEmitterAniMesh(char emitter) = 0;

    /* 0x6c: the host is live; the spell side checks it before starting. */
    /* 0x98: the level of detail, named by GrCycle.cpp's own
       "bLOD >= 0 && bLOD < NUM_LODS" assertion and written 0, 1 or 2 by the
       selector at 0x004A7BE0. It doubles as the AnimObj list index, which is
       what makes one animation list per LOD. */
    signed char m_bLOD;
    unsigned char unknown_099[3];
    /* Two LOD switch distances, scaled by the detail slider before they
       are compared. 0x004A7BE0 reads both with fmul, which types them. */
    float lod_range_09c;
    float lod_range_0a0;
    /* W8GrCycle itself reads these inherited bytes as the common selected
       cycle/subcycle and pending cycle. Missile and Spell use current_cycle
       for the same role; there is no separate Monster view. Only +0xa6 stays
       positional until behavior outside Monster establishes its meaning. */
    signed char current_cycle;          /* 0xa4 */
    signed char current_subcycle;       /* 0xa5 */
    signed char selection_value_0a6;    /* 0xa6 */
    signed char pending_cycle;          /* 0xa7 */
    float value_0a8;
};                                       /* 0xac */

static_assert(sizeof(W8EmitterHost) == 0xac, "W8EmitterHost_size_must_be_0xac");

#pragma pack(pop)

#endif
