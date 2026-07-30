#pragma once

#include "srMeshModel.h"
#include "srNode.h"

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
protected:
    virtual ~srModelInstance() override;
};

static_assert((sizeof(srModelInstance) == 0x160), "srModelInstance_must_be_0x160");
