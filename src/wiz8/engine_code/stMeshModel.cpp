#include "wiz8/gameplay_boundaries.h"
#include "wiz8/mesh_model.h"
#include "surrender/srCore.h"
#include "surrender/srTypeRegistry.h"

/*
 * Engine Code\stMeshModel.cpp.
 *
 * A mesh model and the sibling chain it can be linked into.
 */

extern void Function4729F0(void* model);

// FUNCTION: WIZ8 0x004712d0
int stMeshModel::FindMappedIndex(short key)
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
// FUNCTION: WIZ8 0x00471d60
void stMeshModel::LinkTo(stMeshModel* other)
{
    next = other;
    if (other != 0) {
        other->previous = this;
    }
}

/* One vertex, bounds-checked against the model's own count and refused
   outright when there is no table at all. */
// FUNCTION: WIZ8 0x00471aa0
void* stMeshModel::GetVertex(unsigned int index)
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

/* The two SurRender registry slots. The literal is the class's own original
   name and the id sits in the Wizardry-registered range at 0x10000 and up,
   which is what separates this class from SurRender's own srMeshModel at
   0x2010. */
// FUNCTION: WIZ8 0x004741f0
const char* stMeshModel::getClassName() const
{
    return "stMeshModel";
}

// FUNCTION: WIZ8 0x004741e0
unsigned long stMeshModel::getClassID() const
{
    return 0x10003;
}

/* Three-level registry builder that names every level by literal - no static
   name getter appears at all, which is what makes it the shortest of the
   three-level forms. The chain is stMeshModel under srMeshModel under srModel
   under srClass. */
// FUNCTION: WIZ8 0x00474820
srRegistry::ClassNode* stMeshModel::getClassNode() const
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x10003);

    if (!node) {
        srRegistry* mesh_registry = srCore.getRegistry();
        srRegistry::ClassNode* mesh = mesh_registry->getClassNode(0x2010);

        if (!mesh) {
            srRegistry* model_registry = srCore.getRegistry();
            srRegistry::ClassNode* model = model_registry->getClassNode(0x2000);

            if (!model) {
                model = model_registry->registerClass(
                    "srModel", srClass::sGetClassNode(), 0x2000, 1);
            }
            mesh = mesh_registry->registerClass("srMeshModel", model, 0x2010, 0);
        }
        node = registry->registerClass("stMeshModel", mesh, 0x10003, 0);
    }
    return node;
}
