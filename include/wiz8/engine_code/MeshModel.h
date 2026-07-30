#pragma once

#include "surrender/srMeshModel.h"

/* Zero-storage client subclass of srMeshModel. The local vtable below is real,
   but its registry and clone entries point at srClassSupport<srMeshModel,
   srModel,0,0x2010> emissions inherited from srMeshModel; this class defines
   none of them. Do not reintroduce getClassName, getClassID, getClassNode or
   clone overrides here. */
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
