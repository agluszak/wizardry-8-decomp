#include "wiz8/gameplay_boundaries.h"

/* The 18-slot widget base at vtable 0x005ED5BC. Its teardown is one function:
   VC6 folded the complete destructor into the scalar deleting destructor at
   0x004F3D90, which vtable slot 0 points at.

   Folding only happens when the destructor is defined inside the class body,
   and writing it that way used to stop the deleting destructor being emitted
   at all, because an out-of-line virtual was the only thing pulling the vtable
   into this translation unit. 0x004F3F10 supplies what was missing: it stores
   the vtable, so by MSVC's rules it is a constructor, and having it here emits
   the vtable and lets the destructor sit inline where VC6 will fold it.

   The image names neither the class nor its fields, so both carry
   address-qualified positional names. What the bodies establish is that the
   class owns a region registration at +0x18 which both construction and
   teardown hand to the same function, 0x004F23D0, unless it is the -1 sentinel
   the RegionManager uses for "none".

   That function is SetRegionMode4, an accepted identity already recovered in
   RegionManager.cpp. This file previously declared it locally as
   ReleaseRegion, which is a second name for one address and made the teardown
   read as a release when it is not one.

   Ghidra attributes 0x004F3D90, 0x004F3DD0 and 0x004F3F10 to Local Code\
   Controls.cpp by bounded interval. That is a reviewed-identity question this
   file does not settle, so the address-derived filename stands for now. */

class W8WidgetBase005ED5BC {
public:
    W8WidgetBase005ED5BC();

    // FUNCTION: WIZ8 0x004F3D90
    virtual ~W8WidgetBase005ED5BC()
    {
        m_flag_5 = 0;
        if (m_region_18 != -1) {
            SetRegionMode4(m_region_18);
        }
    }

protected:
    unsigned char unknown_004;
    unsigned char m_flag_5;              /* 0x05: cleared on teardown */
    unsigned char unknown_006[0x12];
    int m_region_18;                     /* 0x18: released unless it is -1 */
};                                       /* 0x1c established */

/* Reads m_region_18 without writing it, which a constructor would normally not
   do. That is what the image does, and the vtable store is what makes it a
   constructor rather than an ordinary method, so it stands as found. */
// FUNCTION: WIZ8 0x004F3F10
W8WidgetBase005ED5BC::W8WidgetBase005ED5BC()
{
    m_flag_5 = 0;
    if (m_region_18 != -1) {
        SetRegionMode4(m_region_18);
    }
}
