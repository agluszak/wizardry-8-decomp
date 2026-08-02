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
class SR_DLL_IMPORT srMaterialIFace
    : public srClassSupport<srMaterialIFace, srClass, true, 0x2200> {
public:
    static const char* sGetClassName();

};

class SR_DLL_IMPORT srMaterial
    : public srClassSupport<srMaterial, srMaterialIFace, 0, 0x2210> {
public:
    enum e_oper {};

    inline srMaterial()
    {
        reset();
    }
    srMaterial(const srMaterial& other);
    static const char* sGetClassName();

    virtual void dump(std::ostream& stream) override;
    virtual void verify(srRuntimeClass::e_verify mode) override;
    virtual srClass* vInstance() override;

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
    inline void setDiffuse(const srVector4T<float>& diffuse)
    {
        setVector(parms_18.diffuse, diffuse);
    }
    inline void setEmissive(const srVector4T<float>& emissive)
    {
        setVector(parms_18.emissive, emissive);
    }
    inline void setMapper(srVertexProcessor* mapper)
    {
        mapper_70 = mapper;
    }
    inline void setOpacity(double opacity)
    {
        parms_18.diffuse.w = static_cast<float>(opacity);
        dirty_74 = 1;
    }
    void setShininess(double shininess);
    inline void setSpecular(const srVector4T<float>& specular)
    {
        setVector(parms_18.specular, specular);
    }
    void setTranslucency(double translucency);

protected:
    void setVector(srVector4T<float>& destination,
                   const srVector4T<float>& source);

public:
    /* ReadLevel.cpp directly edits cloned material parameters before setting
       dirty_74. The original SurRender declaration therefore exposed this
       state to clients; keeping it protected would force a fabricated wrapper. */
    srVertexProcessor::MaterialInfo parms_18; /* 0x18 */
    srFlags<e_oper> operations_6c;             /* 0x6c */
    srVertexProcessor* mapper_70;              /* 0x70 */
    int dirty_74;                              /* 0x74 */
};

static_assert((sizeof(srMaterial) == 0x78), "srMaterial_must_be_0x78");
