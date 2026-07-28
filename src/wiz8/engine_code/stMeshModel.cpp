#include "wiz8/gameplay_boundaries.h"
#include "wiz8/mesh_model.h"

/*
 * Engine Code\stMeshModel.cpp.
 *
 * A mesh model and the sibling chain it can be linked into.
 */

extern void Function4729F0(void* model);

// FUNCTION: WIZ8 0x004712D0
int W8MeshModel::FindMappedIndex(short key)
{
    if (key < 0) {
        return -1;
    }
    int index = mapped_keys.IndexOf(key);
    if (index != -1) {
        return *mapped_values.GetAt(index);
    }
    return -1;
}

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
