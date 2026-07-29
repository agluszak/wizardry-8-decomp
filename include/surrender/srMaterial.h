#pragma once

#include "srMath.h"
#include "srTypeRegistry.h"
#include "srVertexProcessor.h"

class srVertexPipe;

/* srMaterial's exported vftable has thirteen slots, and the first seven are
   srClass's: evidence/snapshots/surrender-abi/vftable-slots.csv resolves slots
   3, 4 and 6 to dump, verify and vInstance, and Wizardry's stMaterial overrides
   slots 0, 1, 2 and 5 with a class-name getter, a class-id getter, a registry
   walk and a destructor - which is srClass's declaration order exactly.

   srMaterialIFace is the 0x2200 node the registry tree puts between srClass and
   srMaterial's 0x2210; nothing observed adds a slot there, so it carries none.

   Two parameter types are simplified and neither moves a slot:
   getMaterialInfo really takes srVertexProcessor::MaterialInfo&. */
class SR_DLL_IMPORT srMaterialIFace : public srClass {
public:
    static const char* sGetClassName();
};

class SR_DLL_IMPORT srMaterial : public srMaterialIFace {
public:
    static const char* sGetClassName();

    virtual void dump(std::ostream& stream) override;
    virtual void verify(srRuntimeClass::e_verify mode) override;
    virtual srClass* vInstance() override;

    /* Slot 7. Unexported, and every subclass overrides it; stMaterial's copies
       through the instance slot 6 returns, so it is a clone. */
    virtual srMaterial* vslot7();
    virtual void getMaterialInfo(srVertexProcessor::MaterialInfo& info);
    virtual void preProcess(srVertexPipe& pipe);
    virtual void postProcess(srVertexPipe& pipe);

protected:
    virtual ~srMaterial() override;
    virtual void updateParms();
    virtual void reset();

public:
    /* Wiz8.exe emits this class's deleting destructor itself, at 0x00423E50,
       and frees through the renderer's heap. A member for the same reason
       srNode's is: the destructor it runs is protected. */
    srMaterial* scalar_deleting_destructor(unsigned char flags);

    srMaterial& operator=(const srMaterial& other);

protected:
    void setVector(srVector4T<float>& destination,
                   const srVector4T<float>& source);

    unsigned char unknown_04_[0x14];
    srVector4T<float> vector_18;
    unsigned char unknown_28_[0x10];
    srVector4T<float> vector_38;
    unsigned char unknown_48_[0x0c];
    srVector4T<float> vector_54;
    unsigned long field_64;
    unsigned long field_68;
    unsigned long field_6c;
    unsigned long field_70;
    unsigned long field_74;
};

static_assert((sizeof(srMaterial) == 0x78), "srMaterial_must_be_0x78");
