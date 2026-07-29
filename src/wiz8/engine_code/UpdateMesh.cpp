#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"

#include "surrender/srMeshModel.h"

/* The world owns a pointer to an object with an embedded polymorphic provider
   at +0x138. Only the fourth virtual slot is used by this body. */
class W8UpdateMeshProvider {
public:
    virtual void Method0();
    virtual void Method1();
    virtual void Method2();
    virtual srMeshModel* GetMesh004BAF92();
};

struct W8UpdateMeshSource {
    unsigned char unknown_000[0x138];
    W8UpdateMeshProvider provider_138;
};

static_assert(offsetof(W8UpdateMeshSource, provider_138) == 0x138,
              "W8UpdateMeshSource_provider_must_be_at_0x138");

/* Rebuild the active-polygon table for the world's mesh and raise the control
   bit consumed by SurRender. The original redundantly computes and stores the
   same raised bit twice; the optimizer collapses the equivalent C++ here. */
// FUNCTION: WIZ8 0x004baf60
void UpdateWorldMesh004BAF60(W8World* world)
{
    W8UpdateMeshSource* source;
    srMeshModel* mesh;
    unsigned long* table;
    long polygon_count;
    unsigned long index;

    if (world == 0) {
        srAssertFail("pWorld", "C:\\Projects\\Wizardry 8\\Engine Code\\UpdateMesh.cpp", 0x2a2, 0);
    }
    source = world->update_mesh_source_70;
    if (source != 0) {
        W8UpdateMeshProvider* provider = &source->provider_138;
        mesh = provider->GetMesh004BAF92();
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
