#include "surrender/srModelInstance.h"

// FUNCTION: SURRENDER 0x1004FFF0
void srModelInstance::setExclusionMask(unsigned long mask)
{
    exclusion_mask_15c = mask;
}

// FUNCTION: SURRENDER 0x1004F920
srModelInstance::srModelInstance(srNode* parent)
    : srClassSupport<srModelInstance, srNode, 0, 0x1100>(static_cast<srNode*>(0))
{
    alignment_flags_148.value = 0;
    align_angle_158 = 0.0f;
    align_axis_14c = srVector3T<float>(0.0f, 0.0f, 1.0f);
    exclusion_mask_15c = 0;
    if (parent != 0) {
        setParent(parent, 0);
    }
}

// FUNCTION: SURRENDER 0x10050010
srModelInstance::srModelInstance(const srModelInstance& other)
    : srClassSupport<srModelInstance, srNode, 0, 0x1100>(static_cast<srNode*>(0))
{
    *this = other;
}

// FUNCTION: SURRENDER 0x1004FA40
srModelInstance::~srModelInstance() {}

/* The Client model pointer is refreshed through setModel/getModel rather than
   copied, so the reference counts stay balanced. */
// FUNCTION: SURRENDER 0x1004F890
srModelInstance& srModelInstance::operator=(const srModelInstance& other)
{
    if (this != &other) {
        srNode::operator=(other);
        alignment_flags_148 = other.alignment_flags_148;
        setModel(other.getModel());
        align_angle_158 = other.align_angle_158;
        align_axis_14c = other.align_axis_14c;
        exclusion_mask_15c = other.exclusion_mask_15c;
    }
    return *this;
}
