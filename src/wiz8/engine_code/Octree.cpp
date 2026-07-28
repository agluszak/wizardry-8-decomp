#include "wiz8/engine_code/BitArray.h"

/*
 * Engine Code\Octree.cpp.
 *
 * The visit marking a traversal uses to avoid walking the same node twice, and
 * three thin wrappers over the shared traversal entry points.
 */

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
// FUNCTION: WIZ8 0x0042E3E0
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
// FUNCTION: WIZ8 0x0042E400
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
// FUNCTION: WIZ8 0x0042E620
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
// FUNCTION: WIZ8 0x0042EF00
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
// FUNCTION: WIZ8 0x0042E810
void OctreeQueueKind13(int id)
{
    int point[3];
    int index;

    for (index = 0; index < 3; ++index) {
        point[index] = (int)OctreeNextCoordinate();
    }
    OctreeQueue(0xd, id + 1, point);
}
