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
    enum e_oper {};

    srMaterial();
    srMaterial(const srMaterial& other);
    static const char* sGetClassName();

    virtual void dump(std::ostream& stream) override;
    virtual void verify(srRuntimeClass::e_verify mode) override;
    virtual srClass* vInstance() override;

    /* Slot 7. Unexported, and every subclass overrides it; stMaterial's copies
       through the instance slot 6 returns, so it is a clone. */
    virtual srMaterial* clone();
    virtual void getMaterialInfo(srVertexProcessor::MaterialInfo& info);
    virtual void preProcess(srVertexPipe& pipe);
    virtual void postProcess(srVertexPipe& pipe);

protected:
    virtual ~srMaterial() override;
    virtual void updateParms();
    virtual void reset();

public:
    srMaterial& operator=(const srMaterial& other);
    void disable(e_oper operation);
    void enable(e_oper operation);
    srVector4T<float> getAmbient() const;
    void getAmbient(srVector4T<float>& ambient) const;
    srVector4T<float> getDiffuse() const;
    void getDiffuse(srVector4T<float>& diffuse) const;
    srVector4T<float> getEmissive() const;
    void getEmissive(srVector4T<float>& emissive) const;
    srVertexProcessor* getMapper() const;
    float getOpacity() const;
    float getShininess() const;
    srVector4T<float> getSpecular() const;
    void getSpecular(srVector4T<float>& specular) const;
    float getTranslucency() const;
    int isEnabled(e_oper operation) const;
    void setAmbient(const srVector4T<float>& ambient);
    void setAmbientAndDiffuse(const srVector4T<float>& color);
    void setDiffuse(const srVector4T<float>& diffuse);
    void setEmissive(const srVector4T<float>& emissive);
    void setMapper(srVertexProcessor* mapper);
    void setOpacity(double opacity);
    void setShininess(double shininess);
    void setSpecular(const srVector4T<float>& specular);
    void setTranslucency(double translucency);

protected:
    void setVector(srVector4T<float>& destination,
                   const srVector4T<float>& source);

    srVertexProcessor::MaterialInfo parms_18; /* 0x18 */
    srFlags<e_oper> operations_6c;             /* 0x6c */
    srVertexProcessor* mapper_70;              /* 0x70 */
    int dirty_74;                              /* 0x74 */
};

static_assert((sizeof(srMaterial) == 0x78), "srMaterial_must_be_0x78");
