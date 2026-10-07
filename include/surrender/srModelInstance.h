#pragma once

#include "srModel.h"
#include "srNode.h"

#include <math.h>

// VTABLE: SURRENDER 0x10077194
// class srClassSupport<srModelInstance, srNode, 0, 4352>

// VTABLE: SURRENDER 0x10077150 srModel::Client
// VTABLE: SURRENDER 0x10077160 srClassSupport<srModelInstance, srNode, 0, 4352>
// class srModelInstance
class SR_DLL_EXPORT srModelInstance : public srClassSupport<srModelInstance, srNode, 0, 0x1100>,
                                      public srModel::Client {
public:
    SR_DLL_IMPORT srModelInstance(srNode* parent = 0);

    SR_DLL_IMPORT srModelInstance& operator=(const srModelInstance& other);

    // FUNCTION: SURRENDER 0x1004FFE0
    static const char* sGetClassName()
    {
        return "srModelInstance";
    }

    SR_DLL_IMPORT virtual void dump(std::ostream& stream) override;
    SR_DLL_IMPORT virtual srClass* vInstance() override;
    SR_DLL_IMPORT virtual void traverse(TraverseInfo& info) override;
    SR_DLL_IMPORT virtual void process(const ProcessInfo& info, e_processType type) override;
    SR_DLL_IMPORT virtual void getLocalBounds(BoundInfo& bounds) override;
    SR_DLL_IMPORT virtual void updateClient(srModel::Client::e_update update) override;

    double getAlignAngle() const;
#if defined(SURRENDER_BUILD)
    srVector3T<float> getAlignAxis() const;
#else
    srVector3T<float> getAlignAxis() const
    {
        return align_axis;
    }
#endif
    unsigned long getExclusionMask() const;
#if defined(SURRENDER_BUILD)
    int isAligned() const;
#else
    int isAligned() const
    {
        return (int)(alignment_flags.value & 1);
    }
#endif
    void setAlignAngle(double angle);
#if defined(SURRENDER_BUILD)
    void setAlignAxis(srVector3T<float> axis);
    void setAlignment(int enabled);
#else
    void setAlignAxis(srVector3T<float> axis)
    {
        float length_squared;
        float scale;

        align_axis = axis;
        length_squared =
            align_axis.z * align_axis.z + align_axis.y * align_axis.y + align_axis.x * align_axis.x;
        if (length_squared != 0.0) {
            scale = static_cast<float>(1.0 / sqrt(length_squared));
            align_axis *= scale;
        }
        alignment_flags.value |= 1;
    }
    void setAlignment(int enabled)
    {
        if (enabled != 0) {
            alignment_flags.value |= 1;
            return;
        }
        alignment_flags.value &= ~1u;
    }
#endif
#if defined(SURRENDER_BUILD)
    void setExclusionMask(unsigned long mask);
#else
    SR_DLL_IMPORT void setExclusionMask(unsigned long mask)
    {
        exclusion_mask = mask;
    }
#endif

    srFlags<int> alignment_flags;

protected:
    SR_DLL_IMPORT virtual ~srModelInstance() override;

    srVector3T<float> align_axis;
    float align_angle;
    unsigned long exclusion_mask;
};

static_assert((sizeof(srModelInstance) == 0x160), "srModelInstance_must_be_0x160");
W8_ASSERT_BASE_END(srModelInstance, srModel::Client, alignment_flags, 0x138);
