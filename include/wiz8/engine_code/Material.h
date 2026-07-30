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

    const char* getClassName() const override
    {
        return srMaterial::sGetClassName();
    }
    unsigned long getClassID() const override; /* 0x00429CC0 */
    srRegistry::ClassNode* getClassNode() const override
    {
        srRegistry* registry = srCore.getRegistry();
        srRegistry::ClassNode* node = registry->getClassNode(0x2210);
        if (node == 0) {
            srRegistry::ClassNode* iface = registry->getClassNode(0x2200);
            if (iface == 0) {
                iface = registry->registerClass(
                    srMaterialIFace::sGetClassName(),
                    srClass::sGetClassNode(),
                    0x2200,
                    1);
            }
            node = registry->registerClass(
                srMaterial::sGetClassName(), iface, 0x2210, 0);
        }
        return node;
    }
    srMaterial* clone() override
    {
        srMaterial* copy = static_cast<srMaterial*>(vInstance());
        *copy = *this;
        return copy;
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
