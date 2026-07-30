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
        srRegistry* registry = srCore.getRegistry();
        srRegistry::ClassNode* iface = registry->getClassNode(0x2200);
        if (iface == 0) {
            iface = registry->registerClass(
                srMaterialIFace::sGetClassName(),
                srClass::sGetClassNode(),
                0x2200,
                1);
        }
        registry->registerInstance(iface, this);

        srRegistry::ClassNode* material = registry->getClassNode(0x2210);
        if (material == 0) {
            material = registry->registerClass(
                srMaterial::sGetClassName(), iface, 0x2210, 0);
        }
        registry->registerInstance(material, this);
        field_68 = 0;
        field_6c = 0;
        reset();
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
    srMaterial* vslot7() override
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
        setVector(vector_54, value);
        value.x = value.y = value.z = value.w = 0.0f;
        setVector(vector_18, value);
        setVector(vector_38, value);
        vector_18.w = 1.0f;
        field_74 = 1;
    }

    void setParameterSource(const void* source)
    {
        field_70 = reinterpret_cast<unsigned long>(source);
    }
};

static_assert(sizeof(W8Material005EBDE0) == 0x78,
              "W8Material005EBDE0_must_be_0x78");
