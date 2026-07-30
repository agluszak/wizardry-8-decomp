#pragma once

#include "srMeshModel.h"
#include "srNode.h"

class SR_DLL_IMPORT srModelInstance
    : public srClassSupport<srModelInstance, srNode, 0, 0x1100>,
      public srModel::Client {
public:
    srModelInstance(srNode* parent);
    srModelInstance(const srModelInstance& other);
    srModelInstance& operator=(const srModelInstance& other);

    static const char* sGetClassName();

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

    double getAlignAngle() const;
    srVector3T<float> getAlignAxis() const;
    unsigned long getExclusionMask() const;
    int isAligned() const;
    void setAlignAngle(double angle);
    void setAlignAxis(srVector3T<float> axis);
    void setAlignment(int enabled);
    void setExclusionMask(unsigned long mask);

protected:
    virtual ~srModelInstance() override;

    unsigned long alignment_flags_148;
    srVector3T<float> align_axis_14c;
    float align_angle_158;
    unsigned long exclusion_mask_15c;
};

static_assert((sizeof(srModelInstance) == 0x160), "srModelInstance_must_be_0x160");
