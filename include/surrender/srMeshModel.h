#pragma once

#include "srMaterial.h"
#include "srMath.h"
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
        virtual ~Client();
        virtual srModel* getModel() const;
        virtual void setModel(srModel* model);
        virtual void updateClient(e_update update);

    private:
        unsigned char unknown_04_[0x24];
    };

    virtual void updateAllClients(Client::e_update update);
};

class SR_DLL_IMPORT srMeshModel : public srModel {
public:
    enum e_side {};

    srMeshModel(long polygons, long vertices);
    virtual void dump(std::ostream& stream) override;
    virtual void verify(srRuntimeClass::e_verify mode) override;
    virtual srClass* vInstance() override;
    virtual int getBoundingSphere(srVector3T<float>& center, float& radius);
    virtual int getBoundingBox(srVector3T<float>& minimum,
                               srVector3T<float>& maximum);
    virtual void render(class srGERD& renderer);
    virtual void reindexPolygons(const unsigned long* indices);
    virtual void reindexVertices(const unsigned long* indices);
    srTextureIFace* getTexture(long polygon, long layer) const;
    void setMaterial(srMaterialIFace* material, long polygon, e_side side);
    void setTexture(srTextureIFace* texture, long polygon, long layer);
    void setShader(srShader shader, long pass);
    void enableStartupControls() {
        control_state_394 |= 0x40;
        control_state_390 |= 8;
        control_state_394 |= 0x30;
    }

public:
    /* Emitted by Wiz8.exe at 0x00424A50, freeing through the renderer's heap. */
    srMeshModel* scalar_deleting_destructor(unsigned char flags);

protected:
    virtual ~srMeshModel() override;
    virtual void updateTriMesh();
    virtual void calculateBounds();
    virtual void calculatePolygonNormals();
    virtual void calculateVertexNormals();
    unsigned char unknown_04_[0x38c];
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

static_assert((sizeof(srModel::Client) == 0x28), "srModelClient_must_be_0x28");
static_assert((sizeof(srMeshModel) == 0x398), "srMeshModel_must_be_0x398");
static_assert((sizeof(srModeler) == 0x14), "srModeler_must_be_0x14");
