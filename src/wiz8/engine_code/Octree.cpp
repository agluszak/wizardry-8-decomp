#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"

/* Engine Code\Octree.cpp. The member names, and the UINT16/UINT32 split between
   the loaded counters and the totals, come from the canonical assertion
   expressions at lines 1157 and 1181; their messages name the class "Octree".
   Offsets come from the asserting bodies. Everything else stays opaque.
   This is the porting model of the reviewed Octree class in
   evidence/reviewed/wiz8/class-provenance.csv, which owns the class identity;
   the W8 prefix is the porting convention for reviewed class names. */
struct W8Octree {
    unsigned char unknown_000[0xc4];
    unsigned char m_fAccumulating;          /* 0x0c4: gates both accumulators */
    unsigned char unknown_0c5[0x33];
    unsigned int m_ulNumParticles;          /* 0x0f8 */
    unsigned char unknown_0fc[0x18];
    void** m_papProps;                      /* 0x114 */
    void** m_papParticles;                  /* 0x118 */
    unsigned short m_usNumPropsLoaded;      /* 0x11c */
    unsigned short m_usNumParticlesLoaded;  /* 0x11e */
    unsigned char unknown_120[0x68];
    unsigned int m_ulNumProps;              /* 0x188 */

    void AddLoadedProp(void* prop);
    void AddLoadedParticle(void* particle);
};

// FUNCTION: WIZ8 0x0042e440
void W8Octree::AddLoadedProp(void* prop)
{
    if (m_fAccumulating) {
        if (m_usNumPropsLoaded >= (unsigned short)m_ulNumProps) {
            srAssertFail(
                "m_usNumPropsLoaded<(UINT16)m_ulNumProps",
                "C:\\Projects\\Wizardry 8\\Engine Code\\Octree.cpp",
                0x485,
                "Too many props loaded for Octree");
        }
        m_papProps[m_usNumPropsLoaded] = prop;
        m_usNumPropsLoaded++;
        m_papProps[m_usNumPropsLoaded] = 0;
    }
}

// FUNCTION: WIZ8 0x0042e4c0
void W8Octree::AddLoadedParticle(void* particle)
{
    if (m_fAccumulating) {
        if (m_usNumParticlesLoaded >= (unsigned short)m_ulNumParticles) {
            srAssertFail(
                "m_usNumParticlesLoaded<(UINT16)m_ulNumParticles",
                "C:\\Projects\\Wizardry 8\\Engine Code\\Octree.cpp",
                0x49d,
                "Too many particles loaded for Octree");
        }
        m_papParticles[m_usNumParticlesLoaded] = particle;
        m_usNumParticlesLoaded++;
        m_papParticles[m_usNumParticlesLoaded] = 0;
    }
}

/* The visit marking a traversal uses to avoid walking the same node twice, and
   three thin wrappers over the shared traversal entry points. */

#include "wiz8/engine_code/BitArray.h"

/* The octree walker. Only the two fields the visit marking reaches are
   established: the running mark and the bit array it is checked against. */
class W8OctreeWalker {
public:
    unsigned char unknown_000[0x184];
    /* 0x184: the base this walk's marks are counted from. A negative offset
       resets it from the shared counter instead of advancing it. */
    int mark_base;
    unsigned char unknown_188[4];
    /* 0x18c: the visited set. With no set attached nothing is ever reported
       as already seen. */
    BitArray* visited;
};

/* The node whose 0x1c is non-null is the only kind worth attaching. */
extern int g_shared_mark_006598ac;

extern void OctreeTraverse(
    void* walker, void* arg_2, void* arg_3, int kind, unsigned int limit);   /* 0x0042F280 */
extern void OctreeVisitPoint(void* walker, const float* point);              /* 0x0042E540 */
extern void OctreeQueue(int kind, int id, const int* point);                 /* 0x00437000 */
extern float OctreeNextCoordinate(void);

/* Attach a visited set to the walker, but only one that has been built. */
// FUNCTION: WIZ8 0x0042e3e0
void W8OctreeWalker_SetVisitedSet(W8OctreeWalker* walker, BitArray* visited)
{
    if (visited->puiIndex != 0) {
        walker->visited = visited;
    }
}

/* Advance or reset the walk's mark. A negative offset takes the shared
   counter's value as this walk's base; anything else advances the shared
   counter and reports whether that mark has already been visited - which with
   no set attached is always no. */
// FUNCTION: WIZ8 0x0042e400
int W8OctreeWalker_MarkVisited(W8OctreeWalker* walker, int offset)
{
    if (offset < 0) {
        walker->mark_base = g_shared_mark_006598ac;
        return 0;
    }
    g_shared_mark_006598ac = walker->mark_base + offset;
    if (walker->visited != 0 && walker->visited->Test(g_shared_mark_006598ac)) {
        return 1;
    }
    return 0;
}

/* Visit a point handed over by address, copied to the stack first so the
   caller's copy is not the one the traversal holds. */
// FUNCTION: WIZ8 0x0042e620
void W8OctreeWalker_VisitPointCopy(void* walker, const float* point)
{
    float copy[3];

    copy[0] = point[0];
    copy[1] = point[1];
    copy[2] = point[2];
    OctreeVisitPoint(walker, copy);
}

/* Start a traversal of the twelfth kind. A limit of zero means no limit, which
   is what the -1 stands for. */
// FUNCTION: WIZ8 0x0042ef00
void OctreeTraverseKind12(void* walker, void* arg_2, void* arg_3, unsigned short limit)
{
    unsigned int bound = (unsigned int)-1;

    if (limit != 0) {
        bound = limit;
    }
    OctreeTraverse(walker, arg_2, arg_3, 0xc, bound);
}

/* Queue one node of the thirteenth kind, with its three coordinates converted
   from floating point - which is what puts three ftol calls in a row here. */
// FUNCTION: WIZ8 0x0042e810
void OctreeQueueKind13(int id)
{
    int point[3];
    int index;

    for (index = 0; index < 3; ++index) {
        point[index] = (int)OctreeNextCoordinate();
    }
    OctreeQueue(0xd, id + 1, point);
}

/* Engine Code\Trigger.cpp lives in the same interval. Its list of live
   triggers is torn down wholesale rather than one at a time. */

/* 0x006598FC and 0x00659904: the live triggers, count and data. */
extern int g_trigger_count;
extern void** g_trigger_data;
extern int g_trigger_flag_00659994;
extern int g_trigger_flag_006598e4;

/* Release every live trigger and forget the three pieces of state that go with
   the list, which is what groups them. */
// FUNCTION: WIZ8 0x004445b0
void ReleaseAllTriggers(void)
{
    int index;

    for (index = 0; index < g_trigger_count; ++index) {
        operator delete(g_trigger_data[index]);
    }
    g_trigger_flag_00659994 = 0;
    g_trigger_count = 0;
    g_trigger_flag_006598e4 = 0;
}
