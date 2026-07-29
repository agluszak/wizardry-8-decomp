#include "surrender/srCore.h"
#include "surrender/srMaterial.h"

/* Engine Code\materials.cpp. Its canonical assertions name the pointer
   ppstMaterial, and the constructor at 0x004925B0 registers the class with
   SurRender's registry under the literal "stMaterial" and the class id
   0x10002, whose parent chain the same body spells out: srMaterialIFace
   0x2200, then srMaterial 0x2210, then this. So the name is the original's
   own, not a descriptive one.

   The vtable at 0x005ECB6C lines up with srMaterial's slot for slot: slots 3,
   4, 6 and 8 through 12 are import thunks into SR.DLL, and the four overridden
   here plus the destructor are the five SurRender does not export. The object
   is 0x7C bytes, which is what the constructor's callers allocate through
   srHeap, and the only field past srMaterial's extent is at 0x78. */
class stMaterial : public srMaterial {
public:
    virtual const char* getClassName() const override;
    virtual unsigned long getClassID() const override;
    virtual srRegistry::ClassNode* getClassNode() const override;
    virtual ~stMaterial() override;
    virtual srMaterial* vslot7() override;

    int m_field_78;                          /* 0x78 */
};

typedef char stMaterial_must_be_0x7c[(sizeof(stMaterial) == 0x7C) ? 1 : -1];

extern "C" char g_stMaterialClassName[];

// Slot 0. The name the constructor also hands to srRegistry::registerClass.
// FUNCTION: WIZ8 0x00492950
const char* stMaterial::getClassName() const
{
    return g_stMaterialClassName;
}

// Slot 1. The class id registered for stMaterial.
// FUNCTION: WIZ8 0x00492940
unsigned long stMaterial::getClassID() const
{
    return 0x10002;
}

/* Slot 2. Ensures the class tree this instance registers against exists,
   walking down from stMaterial's own id to whichever ancestor is already
   registered and building back up. The three registry reads are three separate
   loads of srCore, not one cached pointer. */
// FUNCTION: WIZ8 0x00492960
srRegistry::ClassNode* stMaterial::getClassNode() const
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x10002);

    if (node == 0) {
        srRegistry* material_registry = srCore.getRegistry();

        node = material_registry->getClassNode(0x2210);
        if (node == 0) {
            srRegistry* interface_registry = srCore.getRegistry();

            node = interface_registry->getClassNode(0x2200);
            if (node == 0) {
                node = interface_registry->registerClass(
                    srMaterialIFace::sGetClassName(),
                    srClass::sGetClassNode(),
                    0x2200,
                    1);
            }
            node = material_registry->registerClass(
                srMaterial::sGetClassName(), node, 0x2210, 0);
        }
        node = registry->registerClass(g_stMaterialClassName, node, 0x10002, 0);
    }
    return node;
}

/* Slot 5. The complete destructor at 0x00492A30 is not recovered: it is 425
   bytes that tear the instance out of the registry at three levels, and the two
   it unwinds through are levels this class model does not yet have. Only the
   compiler-generated deleting destructor above it is claimed, which is what
   proves the srHeap routing on srClass::operator delete. */
stMaterial::~stMaterial()
{
}

// Slot 7. Copies through the instance slot 6 returns, then carries the one
// field srMaterial's assignment operator cannot know about.
// FUNCTION: WIZ8 0x00492A00
srMaterial* stMaterial::vslot7()
{
    /* srClass is srMaterial's base at offset zero, so the original reuses the
       returned pointer without adjusting it. */
    stMaterial* instance = static_cast<stMaterial*>(vInstance());

    if (this != instance) {
        *static_cast<srMaterial*>(instance) = *this;
        instance->m_field_78 = m_field_78;
    }
    return instance;
}
