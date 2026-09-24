#pragma once

#include "srMaterialIFace.h"
#include "srMath.h"
#include "srTypeRegistry.h"
#include "srVertexPipe.h"

/* srMaterial's exported vftable has thirteen slots, and the first seven are
   srClass's: evidence/snapshots/surrender-abi/vftable-slots.csv resolves slots
   3, 4 and 6 to dump, verify and vInstance, and Wizardry's stMaterial overrides
   slots 0, 1, 2 and 5 with a class-name getter, a class-id getter, a registry
   walk and a destructor - which is srClass's declaration order exactly.

   Two parameter types are simplified and neither moves a slot:
   getMaterialInfo really takes srVertexProcessor::MaterialInfo&. */
// VTABLE: SURRENDER 0x10075538 srMaterial
class SR_DLL_IMPORT srMaterial : public srClassSupport<srMaterial, srMaterialIFace, 0, 0x2210> {
public:
    enum e_oper {};

    // FUNCTION: SURRENDER 0x10034700 SYMBOL
    // ??0srMaterial@@QAE@XZ
    inline srMaterial()
    {
        reset();
    }
    srMaterial(const srMaterial& other);
    static const char* sGetClassName();

    virtual void dump(std::ostream& stream) override;
    virtual void verify(srRuntimeClass::e_verify mode) override;
    virtual srClass* vInstance() override;

    virtual void getMaterialInfo(srVertexProcessor::MaterialInfo& info) override;
    virtual void preProcess(srVertexPipe& pipe) override;
    virtual void postProcess(srVertexPipe& pipe) override;

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
    // FUNCTION: SURRENDER 0x10034930 SYMBOL
    // ?setAmbient@srMaterial@@QAEXABV?$srVector4T@M@@@Z
    void setAmbient(const srVector4T<float>& ambient)
    {
        setVector(parms.ambient, ambient);
    }
    void setAmbientAndDiffuse(const srVector4T<float>& color);
    // FUNCTION: SURRENDER 0x10034960 SYMBOL
    // ?setDiffuse@srMaterial@@QAEXABV?$srVector4T@M@@@Z
    inline void setDiffuse(const srVector4T<float>& diffuse)
    {
        setVector(parms.diffuse, diffuse);
    }
    // FUNCTION: SURRENDER 0x10034990 SYMBOL
    // ?setEmissive@srMaterial@@QAEXABV?$srVector4T@M@@@Z
    inline void setEmissive(const srVector4T<float>& emissive)
    {
        setVector(parms.emissive, emissive);
    }
    // FUNCTION: SURRENDER 0x10034B40 SYMBOL
    // ?setMapper@srMaterial@@QAEXPAVsrVertexProcessor@@@Z
    inline void setMapper(srVertexProcessor* mapper)
    {
        mapper_70 = mapper;
    }
    // FUNCTION: SURRENDER 0x10034A80 SYMBOL
    // ?setOpacity@srMaterial@@QAEXN@Z
    inline void setOpacity(double opacity)
    {
        parms.diffuse.w = static_cast<float>(opacity);
        dirty_74 = 1;
    }
    void setShininess(double shininess);
    // FUNCTION: SURRENDER 0x100349C0 SYMBOL
    // ?setSpecular@srMaterial@@QAEXABV?$srVector4T@M@@@Z
    inline void setSpecular(const srVector4T<float>& specular)
    {
        setVector(parms.specular, specular);
    }
    void setTranslucency(double translucency);

protected:
    void setVector(srVector4T<float>& destination, const srVector4T<float>& source);

public:
    /* ReadLevel.cpp directly edits cloned material parameters before setting
       dirty_74. The original SurRender declaration therefore exposed this
       state to clients; keeping it protected would force a fabricated wrapper. */
    srVertexProcessor::MaterialInfo parms; /* 0x18 */
    srFlags<e_oper> operations_6c;         /* 0x6c */
    srVertexProcessor* mapper_70;          /* 0x70 */
    int dirty_74;                          /* 0x74 */
};

static_assert((sizeof(srMaterial) == 0x78), "srMaterial_must_be_0x78");
