#ifndef WIZ8_ENGINE_STATE_006598A4_H
#define WIZ8_ENGINE_STATE_006598A4_H

#include "surrender/srMath.h"

class GDProp;

class W8Pathing00457CF0 {
public:
    unsigned int FindPathHandle(
        const unsigned char* path_name,
        unsigned short* path_bounds,
        float* path_range);              /* 0x00457CF0 */
    void LinkPropSurfaces(GDProp* prop);  /* 0x00460020 */
    void LinkPropVertices(GDProp* prop);  /* 0x004600B0 */
};

/* Address-qualified shared engine state reached through 0x006598A4. Only the
   pathing member and the monster-location notification method are recovered. */
struct W8EngineState006598A4 {
    unsigned char unknown_000[0x120];
    /* 0x120: the sector the world is currently resolving against, which the
       item settle path reads to decide whether a dropped item changed sector. */
    int current_sector;
    unsigned char unknown_124[0x5c];
    W8Pathing00457CF0* pathing_180;

    void Function42E620(
        unsigned short location_id,
        srVector3T<float>* position);     /* 0x0042E620 */
};

extern W8EngineState006598A4* g_engine_state_6598a4;

#endif
