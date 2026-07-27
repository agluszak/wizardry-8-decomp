#include "surrender/srMaterial.h"

/* Engine Code\materials.cpp. Its canonical assertions name the pointer
   ppstMaterial, and the constructor at 0x004925B0 registers the class with
   SurRender's registry under the literal "stMaterial" and the class id
   0x10002, whose parent chain the same body spells out: srMaterialIFace
   0x2200, then srMaterial 0x2210, then this. So the name is the original's
   own, not a descriptive one.

   The vtable at 0x005ECB6C lines up with srMaterial's slot for slot: slots 3,
   4, 6 and 8 through 12 are import thunks into SR.DLL, and slots 0, 1, 2, 5
   and 7 are the five srMaterial does not export, all overridden here. The
   object is 0x7C bytes, which is what the constructor's callers allocate
   through srHeap, and the only field past srMaterial's extent is at 0x78. */
class stMaterial : public srMaterial {
public:
    virtual const char* vslot0();
    virtual unsigned long vslot1();
    virtual srMaterial* vslot7();

    int m_field_78;                          /* 0x78 */
};

typedef char stMaterial_must_be_0x7c[(sizeof(stMaterial) == 0x7C) ? 1 : -1];

extern "C" char g_stMaterialClassName[];

// Slot 0. The name the constructor also hands to srRegistry::registerClass.
// FUNCTION: WIZ8 0x00492950
const char* stMaterial::vslot0()
{
    return g_stMaterialClassName;
}

// Slot 1. The class id registered for stMaterial.
// FUNCTION: WIZ8 0x00492940
unsigned long stMaterial::vslot1()
{
    return 0x10002;
}

// Slot 7. Copies through the instance slot 6 returns, then carries the one
// field srMaterial's assignment operator cannot know about.
// FUNCTION: WIZ8 0x00492A00
srMaterial* stMaterial::vslot7()
{
    /* srClass is srMaterial's own base and sits at offset zero, so the
       original reuses the returned pointer without adjusting it. Declaring
       srClass fully here to make that a static_cast would claim a layout
       nothing has established. */
    stMaterial* instance = reinterpret_cast<stMaterial*>(vInstance());

    if (this != instance) {
        *static_cast<srMaterial*>(instance) = *this;
        instance->m_field_78 = m_field_78;
    }
    return instance;
}
