#pragma once

#include "surrender/srMeshModel.h"

/* Wizardry's instantiable wrapper presents as SurRender's canonical
   srMeshModel class and supplies the first-party registry and clone slots. */
// VTABLE: WIZ8 0x005ebe98
class W8MeshModel005EBE98 : public srMeshModel {
public:
    W8MeshModel005EBE98(long polygons, long vertices)
        : srMeshModel(polygons, vertices) {}

protected:
    virtual ~W8MeshModel005EBE98() override {}
};

static_assert(sizeof(W8MeshModel005EBE98) == 0x398,
              "W8MeshModel005EBE98_must_be_0x398");
