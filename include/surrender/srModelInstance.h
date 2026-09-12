#pragma once

#include "srModel.h"
#include "srNode.h"

#include <math.h>

class SR_DLL_IMPORT srModelInstance : public srClassSupport<srModelInstance, srNode, 0, 0x1100>,
                                      public srModel::Client {
public:
    srModelInstance(srNode* parent);
    srModelInstance(const srModelInstance& other);
    srModelInstance& operator=(const srModelInstance& other);

    static const char* sGetClassName()
    {
        return "srModelInstance";
    }

    virtual void dump(std::ostream& stream) override;
    virtual srClass* vInstance() override;
    virtual void traverse(TraverseInfo& info) override;
    virtual void process(const ProcessInfo& info, e_processType type) override;
    virtual void getLocalBounds(BoundInfo& bounds) override;
    virtual void updateClient(srModel::Client::e_update update) override;

    void assignModel(srModel* model)
    {
        static_cast<srModel::Client*>(this)->setModel(model);
    }
    srModel* model() const
    {
        return static_cast<const srModel::Client*>(this)->getModel();
    }

    double getAlignAngle() const;
    srVector3T<float> getAlignAxis() const
    {
        return align_axis_14c;
    }
    unsigned long getExclusionMask() const;
    int isAligned() const
    {
        return (int)(alignment_flags_148 & 1);
    }
    void setAlignAngle(double angle);
    /* Header-visible in Wiz8: the EXE does not import these. SR.DLL still
       exports out-of-line copies. setAlignAxis stores the by-value axis,
       unitizes with the guarded reciprocal-sqrt form (z²+y²+x² order), then
       enables alignment. Prop 0x0044aee0 is setAlignment(1) plus this call,
       not a hand-written field expansion. */
    void setAlignAxis(srVector3T<float> axis)
    {
        float length_squared;
        float scale;

        align_axis_14c = axis;
        length_squared = align_axis_14c.z * align_axis_14c.z + align_axis_14c.y * align_axis_14c.y +
                         align_axis_14c.x * align_axis_14c.x;
        if ((double)length_squared != 0.0) {
            scale = (float)(1.0 / sqrt((double)length_squared));
            align_axis_14c *= scale;
        }
        alignment_flags_148 |= 1;
    }
    void setAlignment(int enabled)
    {
        if (enabled != 0) {
            alignment_flags_148 |= 1;
            return;
        }
        alignment_flags_148 &= ~1u;
    }
    void setExclusionMask(unsigned long mask);

protected:
    virtual ~srModelInstance() override;

    unsigned long alignment_flags_148;
    srVector3T<float> align_axis_14c;
    float align_angle_158;
    unsigned long exclusion_mask_15c;
};

static_assert((sizeof(srModelInstance) == 0x160), "srModelInstance_must_be_0x160");
