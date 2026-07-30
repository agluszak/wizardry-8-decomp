#pragma once

#include "surrender/srCore.h"
#include "surrender/srMaterial.h"

/* Concrete wrapper used when Wiz8 needs an instantiable srMaterial. Its
   runtime identity remains SurRender's srMaterial (0x2210); the address in the
   source name records the first-party vtable that supplies the otherwise
   abstract registry and clone slots. */
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
