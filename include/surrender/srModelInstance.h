#pragma once

#include "srMeshModel.h"
#include "srNode.h"

class srMaterial;

class SR_DLL_IMPORT srModelInstance : public srNode, public srModel::Client {
public:
    srModelInstance(srNode* parent);
    srModelInstance& operator=(const srModelInstance& other);
    virtual void dump(std::ostream& stream) override;
    virtual srClass* vInstance() override;
    virtual void traverse(TraverseInfo& info) override;
    virtual void process(const ProcessInfo& info, e_processType type) override;
    virtual void getLocalBounds(BoundInfo& bounds) override;
    virtual void updateClient(srModel::Client::e_update update) override;

    void assignModel(srModel* model) {
        static_cast<srModel::Client*>(this)->setModel(model);
    }
    srModel* model() const {
        return static_cast<const srModel::Client*>(this)->getModel();
    }
    unsigned char displayState() const { return state_170; }
    void configure2D(short width, short height) {
        state_160 = 0;
        render_depth_164 = 2000;
        left_168 = width;
        top_16a = height;
        right_16c = 0;
        bottom_16e = 0;
        state_170 = 0;
        state_171 = 0;
        vector_174 = 0;
        vector_178 = 0;
        m_pGlowMaterial_17c = 0;
    }
    void setRenderDepth(unsigned long depth) { render_depth_164 = depth; }

protected:
    virtual ~srModelInstance() override;
    unsigned long state_160;
    unsigned long render_depth_164;
    union {
        unsigned long state_168;
        struct {
            short left_168;
            short top_16a;
        };
    };
    union {
        unsigned long state_16c;
        struct {
            short right_16c;
            short bottom_16e;
        };
    };
    union {
        unsigned long state_170_173;
        struct {
            unsigned char state_170;
            unsigned char state_171;
            unsigned char padding_172[2];
        };
    };
    union {
        unsigned long state_174;
        srVector4T<float>* vector_174;
    };
    union {
        unsigned long state_178;
        srVector4T<float>* vector_178;
    };
    union {
        unsigned long state_17c;
        srMaterial* m_pGlowMaterial_17c;
    };
};

static_assert((sizeof(srModelInstance) == 0x180), "srModelInstance_must_be_0x180");
