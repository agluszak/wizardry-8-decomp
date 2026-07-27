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

/* The thing a widget is constructed into and registers itself with. Only the
   parts 0x004F3DD0 touches are named: an origin the widget's rectangle is
   relative to, a grow-by-one array of the widgets it holds, and a region set
   that decides whether a widget without its own region gets one. */
struct W8WidgetOwner {
    unsigned char unknown_00[8];
    int origin_x;                        /* 0x08 */
    int origin_y;                        /* 0x0c */
    unsigned char unknown_10[0x2c];
    int widget_count;                    /* 0x3c */
    int widget_capacity;                 /* 0x40 */
    class W8WidgetBase005ED5BC** widgets;/* 0x44 */
    unsigned int region_set;             /* 0x48 */
};

extern void Function4F3140(const W8RegionEvent* event, struct W8Region* region);

class W8WidgetBase005ED5BC {
public:
    W8WidgetBase005ED5BC(W8WidgetOwner* owner, unsigned int region,
                         int left, int top, int right, int bottom);

    void SetRegion(unsigned int region);

    // FUNCTION: WIZ8 0x004F3D90
    virtual ~W8WidgetBase005ED5BC()
    {
        m_flag_5 = 0;
        if (m_region_18 != -1) {
            SetRegionMode4(m_region_18);
        }
    }

protected:
    unsigned char m_flag_4;              /* 0x04: set to 1 on construction */
    unsigned char m_flag_5;              /* 0x05: cleared on teardown */
    unsigned char m_flag_6;              /* 0x06: cleared on construction */
    unsigned char pad_007;
    /* 0x08: the widget's rectangle, relative to the owner's origin. The
       constructor adds the origin to all four before handing them to the
       region, which is what makes right and bottom edges rather than a size. */
    int m_left;                          /* 0x08 */
    int m_top;                           /* 0x0c */
    int m_right;                         /* 0x10 */
    int m_bottom;                        /* 0x14 */
    int m_region_18;                     /* 0x18: handed to SetRegionMode4 unless -1 */
    W8WidgetOwner* m_owner;              /* 0x1c */
    int m_field_20;                      /* 0x20: the five below are zeroed and not */
    int m_field_24;                      /* 0x24: otherwise touched by the recovered */
    int m_field_28;                      /* 0x28: bodies */
    int m_field_2c;                      /* 0x2c */
    int m_field_30;                      /* 0x30 */
};                                       /* 0x34 established */


/*
 * The full constructor. Places the widget in its owner, gives the region the
 * owner-relative rectangle translated by the owner's origin, appends the
 * widget to the owner's array growing it by exactly one, and - only if the
 * widget has no region of its own - takes one from the owner's region set and
 * points it back here.
 *
 * The array grows by one element per insertion, so filling an owner is
 * quadratic. That is what the image does.
 *
 * If the allocation fails the old array is put back and the index is left -1,
 * which then skips the append but still falls into the region-set branch. The
 * widget is not in the owner's array at that point and the callback id is the
 * -1 truncated to a word, so the failure path registers a region against an
 * index that does not exist. Preserved as found.
 */
// FUNCTION: WIZ8 0x004F3DD0
W8WidgetBase005ED5BC::W8WidgetBase005ED5BC(W8WidgetOwner* owner, unsigned int region,
                                           int left, int top, int right, int bottom)
{
    W8WidgetOwner* holder;
    W8WidgetBase005ED5BC** previous;
    unsigned int taken;
    int wanted;
    int index;
    int i;
    int origin_x;
    int origin_y;

    m_right = right;
    m_owner = owner;
    m_flag_4 = 1;
    m_flag_5 = 0;
    m_flag_6 = 0;
    m_region_18 = region;
    m_field_20 = 0;
    m_field_24 = 0;
    m_field_28 = 0;
    m_field_2c = 0;
    m_field_30 = 0;
    m_left = left;
    m_top = top;
    m_bottom = bottom;
    if (region != 0xffffffff) {
        origin_y = owner->origin_y;
        origin_x = owner->origin_x;
        SetRegionBounds(region,
                        (unsigned short)((short)origin_x + (short)left),
                        (unsigned short)((short)origin_y + (short)top),
                        (unsigned short)((short)right + (short)origin_x),
                        (unsigned short)((short)bottom + (short)origin_y));
        SetRegionMode4(m_region_18);
    }

    holder = m_owner;
    wanted = holder->widget_count + 1;
    if (holder->widget_capacity < wanted) {
        previous = holder->widgets;
        holder->widgets = (W8WidgetBase005ED5BC**)new void*[wanted];
        if (holder->widgets == 0) {
            holder->widgets = previous;
            index = -1;
            goto registered;
        }
        i = 0;
        holder->widget_capacity = wanted;
        if (0 < holder->widget_count) {
            do {
                holder->widgets[i] = previous[i];
                i = i + 1;
            } while (i < holder->widget_count);
        }
        delete[] previous;
    }
    holder->widgets[holder->widget_count] = this;
    index = holder->widget_count;
    holder->widget_count = index + 1;

registered:
    if (holder->region_set != 0 && m_region_18 == -1) {
        taken = AddRegionToSet(holder->region_set);
        SetRegion(taken);
        SetRegionCallback(taken, Function4F3140, (unsigned short)index);
        SetRegionOwner(taken, holder);
    }
}

/*
 * Rebinds the widget to a region: stores it, gives it the widget's rectangle
 * translated by the owner's origin, and then puts it in one of two modes
 * depending on the flag at +0x05 that the teardown clears.
 *
 * The bounds are only pushed when the widget has an owner, but the mode is set
 * regardless, and the second test re-reads the field rather than reusing the
 * argument - so passing -1 leaves the widget with no region and skips both.
 * The rectangle is read a word at a time here while the constructor stores it
 * a dword at a time, which is what fixes the fields as ints whose low halves
 * are all the region is given.
 */
// FUNCTION: WIZ8 0x004F4020
void W8WidgetBase005ED5BC::SetRegion(unsigned int region)
{
    W8WidgetOwner* holder;
    unsigned int bound;
    int origin_x;
    int origin_y;

    m_region_18 = region;
    if (region != 0xffffffff) {
        holder = m_owner;
        if (holder != 0) {
            origin_y = holder->origin_y;
            origin_x = holder->origin_x;
            SetRegionBounds(region,
                            (unsigned short)((short)m_left + (short)origin_x),
                            (unsigned short)((short)m_top + (short)origin_y),
                            (unsigned short)((short)m_right + (short)origin_x),
                            (unsigned short)((short)m_bottom + (short)origin_y));
        }
    }
    bound = m_region_18;
    if (bound != 0xffffffff) {
        if (m_flag_5 != 0) {
            ClearRegionModeBits(bound);
            return;
        }
        SetRegionMode4(bound);
    }
}
