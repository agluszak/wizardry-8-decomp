#include "surrender/srIlluminator.h"

// FUNCTION: SURRENDER 0x1004CAE0
const char* srIlluminator::sGetClassName()
{
    return "srIlluminator";
}

// FUNCTION: SURRENDER 0x1004CB00
unsigned long srIlluminator::getGroupMask() const
{
    return group_mask_13c;
}

// FUNCTION: SURRENDER 0x1004CAF0
void srIlluminator::setGroupMask(unsigned long mask)
{
    group_mask_13c = mask;
}
