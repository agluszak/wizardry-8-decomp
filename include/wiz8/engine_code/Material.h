#pragma once

#include "surrender/srCore.h"
#include "surrender/srMaterial.h"

/* Zero-storage client subclass used where Wiz8 needs an instantiable
   srMaterial; its runtime identity stays SurRender's srMaterial (0x2210). The
   local vtable the source name records is real, but its registry and clone
   entries point at srClassSupport<srMaterial,srMaterialIFace,0,0x2210>
   emissions inherited from srMaterial; this class defines none of them. Do not
   reintroduce getClassName, getClassID, getClassNode or clone overrides here. */
// VTABLE: WIZ8 0x005ebde0
class W8Material005EBDE0 : public srMaterial {
public:
    W8Material005EBDE0()
    {
    }

    void initializeBlitRect()
    {
        srVector4T<float> value;
        value.x = 1.0f;
        value.y = 1.0f;
        value.z = 1.0f;
        value.w = 1.0f;
        setVector(parms_18.emissive, value);
        value.x = value.y = value.z = value.w = 0.0f;
        setVector(parms_18.diffuse, value);
        setVector(parms_18.specular, value);
        parms_18.diffuse.w = 1.0f;
        dirty_74 = 1;
    }

    void setParameterSource(const void* source)
    {
        mapper_70 = reinterpret_cast<srVertexProcessor*>(
            const_cast<void*>(source));
    }
};

static_assert(sizeof(W8Material005EBDE0) == 0x78,
              "W8Material005EBDE0_must_be_0x78");
