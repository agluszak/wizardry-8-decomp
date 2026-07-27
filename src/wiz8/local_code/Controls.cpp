#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"

/* Local Code\Controls.cpp. m_uiRegionSetId is named by the canonical assertion
   at line 399, and REGSET_NULL is zero because the body's guard is a plain
   test against zero.

   The 18-slot widget base at vtable 0x005ED5BC lives here too. Ghidra
   attributes 0x004F3D90, 0x004F3DD0 and 0x004F4020 to this file by bounded
   interval, and the class the constructor registers into is this one: the
   region set it reads sits at +0x48, which is where the assertion puts
   m_uiRegionSetId. It had been recovered separately against an invented
   W8Controls of its own, so this merges the two rather than leaving one
   address described by two structures.

   The image names neither the widget class nor its fields, so both carry
   address-qualified positional names. The three remaining W8Controls members
   below are positional for the same reason - only m_uiRegionSetId is spoken
   for by the assertion - so they are spelled in the file's style without
   claiming that style is evidence. */
#define REGSET_NULL 0

class W8WidgetBase005ED5BC;

/* The region callback a widget without its own region is given. Ghidra has no
   function at 0x004F3140, only a label, so this is a declaration and not a
   claim on the address. */
extern void Function4F3140(const W8RegionEvent* event, struct W8Region* region);

struct W8Controls {
    unsigned char unknown_00[8];
    int origin_x;                           /* 0x08: widget rectangles are relative to this */
    int origin_y;                           /* 0x0c */
    unsigned char unknown_10[0x2c];
    int m_nControls;                        /* 0x3c */
    int m_nControlsAllocated;               /* 0x40 */
    W8WidgetBase005ED5BC** m_ppControls;    /* 0x44 */
    unsigned int m_uiRegionSetId;           /* 0x48 */

    void EnableRegionSet(unsigned char enable);
};

// FUNCTION: WIZ8 0x004F30F0
void W8Controls::EnableRegionSet(unsigned char enable)
{
    if (m_uiRegionSetId == REGSET_NULL) {
        srAssertFail(
            "m_uiRegionSetId != REGSET_NULL",
            "C:\\Projects\\Wizardry 8\\Local Code\\Controls.cpp",
            0x18f,
            0);
    }
    if (enable) {
        RegionSetEnable(m_uiRegionSetId);
    } else {
        RegionSetDisable(m_uiRegionSetId);
    }
}

class W8WidgetBase005ED5BC {
public:
    W8WidgetBase005ED5BC(W8Controls* owner, unsigned int region,
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
    W8Controls* m_owner;              /* 0x1c */
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
W8WidgetBase005ED5BC::W8WidgetBase005ED5BC(W8Controls* owner, unsigned int region,
                                           int left, int top, int right, int bottom)
{
    W8Controls* holder;
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
    wanted = holder->m_nControls + 1;
    if (holder->m_nControlsAllocated < wanted) {
        previous = holder->m_ppControls;
        holder->m_ppControls = (W8WidgetBase005ED5BC**)new void*[wanted];
        if (holder->m_ppControls == 0) {
            holder->m_ppControls = previous;
            index = -1;
            goto registered;
        }
        i = 0;
        holder->m_nControlsAllocated = wanted;
        if (0 < holder->m_nControls) {
            do {
                holder->m_ppControls[i] = previous[i];
                i = i + 1;
            } while (i < holder->m_nControls);
        }
        delete[] previous;
    }
    holder->m_ppControls[holder->m_nControls] = this;
    index = holder->m_nControls;
    holder->m_nControls = index + 1;

registered:
    if (holder->m_uiRegionSetId != 0 && m_region_18 == -1) {
        taken = AddRegionToSet(holder->m_uiRegionSetId);
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
    W8Controls* holder;
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
