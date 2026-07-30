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

class SR_DLL_IMPORT srModel : public srClass {
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

    static srRegistry::ClassNode* sGetClassNode()
    {
        srRegistry* registry = srCore.getRegistry();
        srRegistry::ClassNode* node = registry->getClassNode(0x2000);

        if (node == 0) {
            node = registry->registerClass(
                sGetClassName(), srClass::sGetClassNode(), 0x2000, 1);
        }
        return node;
    }

    virtual void dump(std::ostream& stream) override;

protected:
    virtual ~srModel() override;

public:
    virtual srModel* clone();
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
    struct TriMesh;

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
    virtual const TriMesh& getTriMesh();
    virtual void getTriMesh(TriMesh& mesh);
    virtual void renderTriMesh(class srGERD& renderer, const TriMesh& mesh);
    srPtr<srTextureIFace>* getPolyTexture(long polygon, long layer, int table);
    srVector3i* getPolyVertex();
    srMaterialIFace* getMaterial(long polygon, e_side side) const;
    srTextureIFace* getTexture(long polygon, long layer) const;
    void setMaterial(srMaterialIFace* material, long polygon, e_side side);
    void setTexture(srTextureIFace* texture, long polygon, long layer);
    void setShader(srShader shader, long pass);
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
    unsigned char unknown_1c_[0x214];
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
static_assert((sizeof(srModel) == 0x1c), "srModel_must_be_0x1c");
static_assert((sizeof(srMeshModel) == 0x398), "srMeshModel_must_be_0x398");
static_assert((sizeof(srModeler) == 0x14), "srModeler_must_be_0x14");
