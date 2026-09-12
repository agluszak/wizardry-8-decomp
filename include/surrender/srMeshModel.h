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
    /* Bit indices into control_state_394. renderTriMesh tests bits 0/1 as
       front/back sides. updateTriMesh skips auto box when bit 4 is set and
       auto sphere when bit 5 is set. enableStartupControls ORs bits 4–6. */
    enum e_control {
        CONTROL_FRONT = 0,
        CONTROL_BACK = 1,
        CONTROL_SKIP_AUTO_BOX = 4,
        CONTROL_SKIP_AUTO_SPHERE = 5,
        CONTROL_STARTUP = 6
    };
    /* Detached 0x154-byte value at srMeshModel+0x23c. updateTriMesh fills it
       from the live tables; getTriMesh copies or returns it; renderTriMesh
       feeds srTriMeshPipeline from these slots. */
    struct TriMesh {
        TriMesh()
        {
            for (int pass = 0; pass != 4; ++pass) {
                shaders_b0[pass].value = 0x0100241b;
            }
        }

        long vertex_count_00;
        long polygon_count_04;
        long pass_count_08;
        unsigned long control_flags_0c;
        srVector3i* poly_vertices_10;
        srVector4T<float>* poly_equations_14;
        srVector2T<float>* texcoords_18[4][2];
        srVector3T<float>* positions_38;
        srVector3T<float>* normals_3c;
        srVector3T<float>* dig_40[4];
        srVector4T<float>* dcg_50[4];
        srVector4T<float>* scg_60[4];
        srMaterial* materials_70[4][2];
        srTextureIFace* textures_90[4][2];
        srShader shaders_b0[4];
        srPtr<srMaterialIFace>* vertex_materials_c0[4][2];
        srPtr<srTextureIFace>* poly_textures_e0[4][2];
        srShader* poly_shaders_100[4];
        srVector3i* poly_uv_110[4];
        srVector3T<float> bounds_minimum_120;
        srVector3T<float> bounds_maximum_12c;
        srVector3T<float> bounds_center_138;
        float bounds_radius_144;
        float sort_bias_148;
        unsigned long* active_polygons_14c;
        unsigned long active_polygon_count_150;
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
    /* setMaterial indexes [pass][side]; ctor default-constructs eight slots. */
    srMaterialIFace* materials_1c[4][2];
    srTextureIFace* textures_3c[4][2];
    srShader shaders_5c[4];
    unsigned char unknown_6c_[0x1bc];
    long pass_count_228;
    /* GrCycle.cpp's 0x004A7E50 clamps a vertex index against this before
       indexing the location array, which is what makes it that array's
       length rather than one more opaque dword. */
    long vertex_location_count_22c;
    long polygon_count_230;
    long uv_count_234;
    float sort_bias_238;
    TriMesh tri_mesh_23c;
    unsigned long control_state_390;
    unsigned long control_state_394;
};

static_assert((sizeof(srMeshModel::TriMesh) == 0x154), "srMeshModel_TriMesh_must_be_0x154");
static_assert((sizeof(srMeshModel) == 0x398), "srMeshModel_must_be_0x398");
