#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/sr_api.h"

#include "surrender/srMeshModel.h"
#include "surrender/srModelInstance.h"

// FUNCTION: WIZ8 0x004BAF50
void UpdateWorldOctree004BAF50(W8World* world)
{
    world->octree->Function0042F7E0();
}

/* Rebuild the active-polygon table for the world's mesh and raise the control
   bit consumed by SurRender. The original redundantly computes and stores the
   same raised bit twice; the optimizer collapses the equivalent C++ here. */
// FUNCTION: WIZ8 0x004baf60
void UpdateWorldMesh004BAF60(W8World* world)
{
    srModelInstance* source;
    srMeshModel* mesh;
    unsigned long* table;
    long polygon_count;
    unsigned long index;

    if (world == 0) {
        srAssertFail("pWorld", "C:\\Projects\\Wizardry 8\\Engine Code\\UpdateMesh.cpp", 0x2a2, 0);
    }
    source = world->update_mesh_source;
    if (source != 0) {
        mesh = static_cast<srMeshModel*>(source->model());
        polygon_count = mesh->polygon_count_230;
        mesh->setActivePolygonCount(polygon_count);
        table = mesh->getActivePolygonTable(1);
        for (index = 0; static_cast<long>(index) < polygon_count; ++index) {
            table[index] = index;
        }
        if ((mesh->control_state_390 & 8) == 0) {
            unsigned long state = mesh->control_state_390;
            mesh->control_state_390 = state | 8;
            mesh->control_state_390 = state | 8;
        }
    }
}
