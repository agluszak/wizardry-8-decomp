#include "wiz8/gameplay_boundaries.h"

/*
 * Engine Code\stMeshModel.cpp.
 *
 * A mesh model and the sibling chain it can be linked into.
 */

/* Only the members these bodies reach are established: the two ends of the
   sibling link, and the vertex table with its own count. */
class W8MeshModel {
public:
    void LinkTo(W8MeshModel* other);      /* 0x00471D60 */
    void* GetVertex(unsigned int index);  /* 0x00471AA0 */

    unsigned char unknown_000[0x398];
    W8MeshModel* next;                    /* 0x398 */
    W8MeshModel* previous;                /* 0x39c */
    unsigned char unknown_3a0[0x30];
    unsigned int vertex_count;            /* 0x3d0 */
    unsigned char unknown_3d4[0xc];
    void** vertices;                      /* 0x3e0 */
};

extern void Function4729F0(void* model);

/* Link one model onto another, setting both ends - so the two pointers are one
   link rather than two independent fields. Unlinking passes nothing. */
// FUNCTION: WIZ8 0x00471D60
void W8MeshModel::LinkTo(W8MeshModel* other)
{
    next = other;
    if (other != 0) {
        other->previous = this;
    }
}

/* One vertex, bounds-checked against the model's own count and refused
   outright when there is no table at all. */
// FUNCTION: WIZ8 0x00471AA0
void* W8MeshModel::GetVertex(unsigned int index)
{
    if (vertices != 0 && index < vertex_count) {
        return vertices[index];
    }
    return 0;
}

/* Thirteen-byte forwarder onto the model release path. */
// FUNCTION: WIZ8 0x00473180
void ReleaseMeshModel(void* model)
{
    Function4729F0(model);
}
