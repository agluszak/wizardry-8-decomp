#ifndef WIZ8_ENGINE_STATE_006598A4_H
#define WIZ8_ENGINE_STATE_006598A4_H

#include "surrender/srMath.h"

class GDProp;
struct W8Position;
struct W8NavigatorMovement004572C0;

class W8OctreeQueue00437000 {
public:
    void Queue00437000(int kind, int id, const int* point);
};

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
   world services exercised by current Octree, item, prop, and Navigator ports
   are exposed. */
struct W8EngineState006598A4 {
    unsigned char unknown_000[0x0c];
    srVector3T<float> octree_origin_00c;
    unsigned char unknown_018[0x58];
    float octree_cell_size_070;
    unsigned char unknown_074[0x48];
    W8OctreeQueue00437000* octree_queue_0bc;
    unsigned char unknown_0c0[0x60];
    /* 0x120: the sector the world is currently resolving against, which the
       item settle path reads to decide whether a dropped item changed sector. */
    int current_sector;
    unsigned char unknown_124[0x5c];
    W8Pathing00457CF0* pathing_180;

    void W8OctreeWalker_VisitPointCopy(
        unsigned short location_id,
        srVector3T<float>* position);     /* 0x0042E620 */
    void UpdateMonsterLocation0042E540(
        unsigned short location_id,
        const W8Position* position);      /* 0x0042E540 */
    bool HasLineOfSight00434B60(
        const W8Position* from,
        W8Position* to,
        char allow_fallback);             /* 0x00434B60 */
    short TraceLineOfSight00434F20(
        const W8Position* from,
        const W8Position* to,
        char trace_world,
        int from_location_id,
        int to_location_id,
        char visit_octree,
        int trace_mode);                  /* 0x00434F20 */
    void AdjustPortalDestination00434A30(
        W8Position* destination,
        const W8Position* source);        /* 0x00434A30 */
    unsigned int AdvanceNavigator00434620(
        W8NavigatorMovement004572C0* movement,
        float radius,
        float separation);                /* 0x00434620 */
    void QueueOctreeKind130042E810(
        int id,
        const srVector3T<float>* position); /* 0x0042E810 */
};

extern W8EngineState006598A4* g_engine_state_6598a4;

#endif
