#pragma once

#include "srMaterial.h"
#include "srMath.h"
#include "srModel.h"
#include "srPtr.h"
#include "srShader.h"
#include "srTexture.h"
#include "srTypeRegistry.h"

class SR_DLL_IMPORT srMeshModel : public srClassSupport<srMeshModel, srModel, 0, 0x2010> {
public:
    enum e_side {};
    /* SR.DLL's getTriMesh copies 0x154 bytes into this value. Wiz8's 2D
       model path independently proves the material at +0x70 and the four
       pass shaders beginning at +0xb0; the rest remains renderer-owned. */
    struct TriMesh {
        TriMesh()
        {
            for (int pass = 0; pass != 4; ++pass) {
                shaders_0b0[pass].value = 0x0100241b;
            }
        }

        unsigned char unknown_000[0x70];
        srMaterial* material_070;
        unsigned char unknown_074[0x3c];
        srShader shaders_0b0[4];
        unsigned char unknown_0c0[0x94];
    };

    srMeshModel(long polygons, long vertices);
    srMeshModel& operator=(const srMeshModel& other);

    static const char* sGetClassName()
    {
        return "srMeshModel";
    }

    virtual void dump(std::ostream& stream) override;
    virtual void verify(srRuntimeClass::e_verify mode) override;
    virtual srClass* vInstance() override;
    virtual int getBoundingSphere(srVector3T<float>& center, float& radius) override;
    virtual int getBoundingBox(srVector3T<float>& minimum, srVector3T<float>& maximum) override;
    virtual void render(class srGERD& renderer) override;
    virtual void reindexPolygons(const unsigned long* indices);
    virtual void reindexVertices(const unsigned long* indices);
    virtual void getTriMesh(TriMesh& mesh);
    virtual const TriMesh& getTriMesh();
    virtual void renderTriMesh(class srGERD& renderer, const TriMesh& mesh);
    srPtr<srTextureIFace>* getPolyTexture(long polygon, long layer, int table);
    srVector3i* getPolyVertex();
    srVector3i* getPolyUVIndex(long layer, int table);
    srVector2T<float>* getVertexTexCoords(long vertex, long layer, int table);
    srPtr<srMaterialIFace>* getVertexMaterial(long vertex, e_side side, int table);
    unsigned long* getVertexShadeIndex(int table);
    srVector3T<float>* getVertexNormal();
    srVector4T<float>* getPolyEq();
    srVector3T<float>* getVertexDIG(long vertex, int table);
    srMaterialIFace* getMaterial(long polygon, e_side side) const;
    srTextureIFace* getTexture(long polygon, long layer) const;
    void setMaterial(srMaterialIFace* material, long polygon, e_side side);
    void setTexture(srTextureIFace* texture, long polygon, long layer);
    srShader* getPolyShader(long polygon, int layer);
    srShader getShader(long polygon) const;
    void setShader(srShader shader, long pass);
    void setUVCount(long count);
    void setActivePolygonCount(long count);
    unsigned long* getActivePolygonTable(int table);
    srVector3T<float>* getVertexLoc();
    void enableStartupControls()
    {
        control_state_394 |= 0x40;
        control_state_390 |= 8;
        control_state_394 |= 0x30;
    }
    /* Raise one 0x394 control bit and mark the 0x390 changed bit when it is
       clear. The original stores the changed bit twice; VC6 emits that pair
       at every expansion site, so the body keeps both stores. */
    void setControlMask(unsigned long mask)
    {
        control_state_394 |= mask;
        if ((control_state_390 & 8) == 0) {
            unsigned long state = control_state_390;
            control_state_390 = state | 8;
            control_state_390 = state | 8;
        }
    }

protected:
    virtual ~srMeshModel() override;
    virtual void updateTriMesh();
    virtual void calculateBounds();
    virtual void calculatePolygonNormals();
    virtual void calculateVertexNormals();

public:
    unsigned char unknown_1c_[0x210];
    /* GrCycle.cpp's 0x004A7E50 clamps a vertex index against this before
       indexing the location array, which is what makes it that array's
       length rather than one more opaque dword. */
    long vertex_location_count_22c;
    long polygon_count_230;
    unsigned char unknown_234_[0x15c];
    unsigned long control_state_390;
    unsigned long control_state_394;
};

static_assert((sizeof(srMeshModel::TriMesh) == 0x154), "srMeshModel_TriMesh_must_be_0x154");
static_assert((sizeof(srMeshModel) == 0x398), "srMeshModel_must_be_0x398");
