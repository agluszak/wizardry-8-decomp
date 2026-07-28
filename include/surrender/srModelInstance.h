#pragma once

#include "srMeshModel.h"
#include "srNode.h"

class SR_DLL_IMPORT srModelInstance : public srNode, public srModel::Client {
public:
    srModelInstance(srNode* parent);
    virtual void dump(std::ostream& stream);
    virtual srClass* vInstance();
    virtual void traverse(TraverseInfo& info);
    virtual void process(const ProcessInfo& info, e_processType type);
    virtual void getLocalBounds(BoundInfo& bounds);
    virtual void updateClient(srModel::Client::e_update update);

    void assignModel(srModel* model) {
        static_cast<srModel::Client*>(this)->setModel(model);
    }
    srModel* model() const {
        return static_cast<const srModel::Client*>(this)->getModel();
    }
    void configure2D(short width, short height) {
        state_160 = 0;
        render_depth_164 = 2000;
        left_168 = width;
        top_16a = height;
        right_16c = 0;
        bottom_16e = 0;
        state_170 = 0;
        state_171 = 0;
        state_174 = 0;
        state_178 = 0;
        state_17c = 0;
    }
    void setRenderDepth(unsigned long depth) { render_depth_164 = depth; }

protected:
    virtual ~srModelInstance();
    unsigned long state_160;
    unsigned long render_depth_164;
    short left_168;
    short top_16a;
    short right_16c;
    short bottom_16e;
    unsigned char state_170;
    unsigned char state_171;
    unsigned char padding_172[2];
    unsigned long state_174;
    unsigned long state_178;
    unsigned long state_17c;
};

typedef char srModelInstance_must_be_0x180[
    (sizeof(srModelInstance) == 0x180) ? 1 : -1];
