#pragma once

#include "srMaterial.h"
#include "srMath.h"
#include "srPtr.h"
#include "srTexture.h"
#include "srTypeRegistry.h"

class srShader {
public:
    unsigned long value;
};

class SR_DLL_IMPORT srModel
    : public srClassSupport<srModel, srClass, true, 0x2000> {
public:
    class Client {
    public:
        enum e_update {};

        Client();
        Client(const Client& other);
        virtual ~Client();
        Client& operator=(const Client& other);
        virtual void setModel(srModel* model);
        virtual void updateClient(e_update update);
        virtual srModel* getModel() const;
        Client* getNextClient() const;
        Client* getPrevClient() const;

    private:
        srPtr<srModel> model_04;
        Client* previous_08;
        Client* next_0c;
    };

    srModel();
    srModel(const srModel& other);

    static const char* sGetClassName()
    {
        return "srModel";
    }


    virtual void dump(std::ostream& stream) override;

protected:
    virtual ~srModel() override;

public:
    virtual int getBoundingSphere(
        srVector3T<float>& center, float& radius) = 0;
    virtual int getBoundingBox(
        srVector3T<float>& minimum, srVector3T<float>& maximum) = 0;
    virtual void render(class srGERD& renderer) = 0;
    virtual void updateAllClients(Client::e_update update);

    Client* getFirstClient() const;

protected:
    Client* first_client_18;
};

class SR_DLL_IMPORT srMeshModel
    : public srClassSupport<srMeshModel, srModel, 0, 0x2010> {
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
    virtual int getBoundingSphere(
        srVector3T<float>& center, float& radius) override;
    virtual int getBoundingBox(srVector3T<float>& minimum,
                               srVector3T<float>& maximum) override;
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
    srPtr<srMaterialIFace>* getVertexMaterial(
        long vertex, e_side side, int table);
    unsigned long* getVertexShadeIndex(int table);
    srVector3T<float>* getVertexNormal();
    srVector4T<float>* getPolyEq();
    srVector3T<float>* getVertexDIG(long vertex, int table);
    srMaterialIFace* getMaterial(long polygon, e_side side) const;
    srTextureIFace* getTexture(long polygon, long layer) const;
    void setMaterial(srMaterialIFace* material, long polygon, e_side side);
    void setTexture(srTextureIFace* texture, long polygon, long layer);
    void setShader(srShader shader, long pass);
    void setUVCount(long count);
    void setActivePolygonCount(long count);
    unsigned long* getActivePolygonTable(int table);
    srVector3T<float>* getVertexLoc();
    void enableStartupControls() {
        control_state_394 |= 0x40;
        control_state_390 |= 8;
        control_state_394 |= 0x30;
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

class SR_DLL_IMPORT srModeler {
public:
    struct MappingInfo {
        unsigned long unknown_00;
        unsigned long unknown_04;
        unsigned long unknown_08;
        unsigned long unknown_0c;
        unsigned long unknown_10;
        unsigned long unknown_14;
    };

    srModeler();
    virtual ~srModeler();
    void createGrid(long columns, long rows);
    void planarMap(long polygon, long layer, const MappingInfo& mapping);
    void scale(const srVector3T<float>& scale);
    void convert(srMeshModel& model, int preserve);
    void discard();

private:
    unsigned char unknown_04_[0x10];
};

static_assert((sizeof(srModel::Client) == 0x10), "srModelClient_must_be_0x10");
static_assert((sizeof(srMeshModel::TriMesh) == 0x154),
              "srMeshModel_TriMesh_must_be_0x154");
static_assert((sizeof(srModel) == 0x1c), "srModel_must_be_0x1c");
static_assert((sizeof(srMeshModel) == 0x398), "srMeshModel_must_be_0x398");
static_assert((sizeof(srModeler) == 0x14), "srModeler_must_be_0x14");
