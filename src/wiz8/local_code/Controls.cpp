#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"
#include "wiz8/vector.h"

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

   The image names neither the widget class nor most of its fields, so they
   carry address-qualified positional names. The member at +0x1c is the
   exception: the assertion at Controls.cpp:1849 reads m_pPanel != NULL, and
   the pointer it guards is the one the constructor stores there and registers
   into, so that member is named by the original source. It had been called
   m_owner here, which was a guess at the same thing. The three remaining W8Controls members
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
    W8Controls* m_pPanel;                /* 0x1c: named by Controls.cpp:1849 */
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
    m_pPanel = owner;
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

    holder = m_pPanel;
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
        holder = m_pPanel;
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

/* Measures a string, returning its extent through the last two arguments. Not
   recovered, and not first-party as far as this file can tell - declared only
   so the caller below can be. */
extern void Function549660(int a, int b, int c, short* width, short* height);

/*
 * A widget carrying text. Derived from the widget base rather than merely
 * shaped like it: it reads the rectangle at +0x08 and the panel at +0x1c
 * exactly as the base lays them out, and everything it owns starts past the
 * base's 0x34. The class is named for the function that establishes it,
 * because no vtable of its own is in evidence here.
 */
class W8TextWidget004F4850 : public W8WidgetBase005ED5BC {
public:
    void GetTextOrigin(int unused, int* px, int* py);

protected:
    int m_field_34;
    unsigned int m_flags_38;             /* 0x38: 0x80 skips alignment, 0x04 pins left */
    int m_field_3c;
    int m_text_40;                       /* 0x40: the four below are -1 when unset and */
    int m_text_44;                       /* 0x44: are what the measure call consumes */
    int m_text_48;                       /* 0x48 */
    int m_text_4c;                       /* 0x4c */
    unsigned char unknown_50[0xc];
    short m_measured_w;                  /* 0x5c: -1 until measured */
    short m_measured_h;                  /* 0x5e */
};

/*
 * Where the text should be drawn: the panel's origin, plus either the widget's
 * own corner or an alignment computed from the measured extent.
 *
 * The measure is cached in the two shorts at +0x5c and only recomputed when
 * either is -1. If the handles it would measure from are themselves unset the
 * cache is stamped -1 again and the plain corner is used, so an unmeasurable
 * widget re-tests those handles on every call rather than settling.
 *
 * The first argument is not read. It is kept because the calling convention
 * needs it, not because anything here wants it.
 */
// FUNCTION: WIZ8 0x004F4850
void W8TextWidget004F4850::GetTextOrigin(int unused, int* px, int* py)
{
    short* measured;
    short width;
    int handle;
    int x;

    if (m_pPanel == 0) {
        srAssertFail("m_pPanel != NULL",
                     "C:\\Projects\\Wizardry 8\\Local Code\\Controls.cpp",
                     0x739,
                     0);
    }
    *px = m_pPanel->origin_x;
    measured = &m_measured_w;
    *py = m_pPanel->origin_y;
    width = *measured;
    if (width == -1 || m_measured_h == -1) {
        if (m_text_40 == -1 || m_text_44 == -1) {
            *measured = -1;
            m_measured_h = -1;
            goto plain;
        }
        handle = m_text_48;
        if (handle == -1 && (handle = m_text_4c, handle == -1)) {
            *measured = -1;
            m_measured_h = -1;
            goto plain;
        }
        Function549660(m_text_40, m_text_44, handle, measured, &m_measured_h);
        if ((m_flags_38 & 0x80) != 0) {
            *px = *px + m_left;
            *py = *py + m_top;
            return;
        }
        if ((m_flags_38 & 4) != 0) {
            x = m_left;
            goto aligned;
        }
        width = *measured;
    } else {
        if ((m_flags_38 & 0x80) != 0) {
            *px = *px + m_left;
            *py = *py + m_top;
            return;
        }
        if ((m_flags_38 & 4) != 0) {
            x = m_left;
            goto aligned;
        }
    }
    x = m_right - (int)width;

aligned:
    *px = *px + x;
    *py = *py + ((m_bottom - (int)m_measured_h) - m_top) / 2 + m_top;
    return;

plain:
    *px = *px + m_left;
    *py = *py + m_top;
}

/*
 * The class at vtable 0x005ED5B8. It owns a wide-string buffer at +0x34, which
 * is the one field the encodings name for themselves: the destructor frees it
 * and 0x004F33A0 fills it with wcscpy. Everything else the constructor touches
 * is positional.
 *
 * The destructor is written inside the class body because that is what folds it
 * into the deleting destructor, the same shape the widget base above needed,
 * and the constructor is what emits the vtable so the fold has something to
 * hang on.
 */
class W8TextBuffer005ED5B8 {
public:
    W8TextBuffer005ED5B8();

    // FUNCTION: WIZ8 0x004F3370
    virtual ~W8TextBuffer005ED5B8()
    {
        delete[] m_buffer;
    }

protected:
    int m_field_04;
    int m_field_08;
    int m_field_0c;
    int m_field_10;
    int m_field_14;
    int m_field_18;
    int m_field_1c;
    int m_field_20;
    int m_field_24;                      /* 0x24: the constructor steps over this one */
    int m_field_28;
    int m_field_2c;
    int m_field_30;
    wchar_t* m_buffer;                   /* 0x34: freed on teardown */
    int m_field_38;                      /* 0x38: 10 */
    int m_field_3c;
    unsigned char m_flag_40;
    unsigned char m_flag_41;
    unsigned char pad_42[2];
    int m_field_44;                      /* 0x44: 4 */
    int m_field_48;                      /* 0x48: the -1 sentinel */
    unsigned char m_flag_4c;
};

// FUNCTION: WIZ8 0x004F3310
W8TextBuffer005ED5B8::W8TextBuffer005ED5B8()
{
    m_buffer = 0;
    m_field_28 = 0;
    m_field_2c = 0;
    m_flag_40 = 0;
    m_field_38 = 10;
    m_field_3c = 0;
    m_field_30 = 0;
    m_field_44 = 4;
    m_flag_41 = 0;
    m_field_48 = -1;
    m_flag_4c = 0;
    m_field_04 = 0;
    m_field_08 = 0;
    m_field_0c = 0;
    m_field_10 = 0;
    m_field_14 = 0;
    m_field_18 = 0;
    m_field_1c = 0;
    m_field_20 = 0;
}

/*
 * A widget at vtable 0x005ED66C that sizes itself to its text. It hands the
 * base a degenerate rectangle - left and top from its arguments, right and
 * bottom zero - then measures and writes the real right and bottom back over
 * them before rebinding the region to the corrected bounds.
 *
 * It carries two handle triples, at +0x34 and +0x40. The first is what the
 * initial measure sizes the widget from; the second measure mixes them, taking
 * the first two of one and the first of the other, and what that produces is
 * kept at +0x50 with the leftover width at +0x4c. Nothing here says what the
 * triples are, only that the measure consumes three at a time.
 */
class W8Control005ED66C : public W8WidgetBase005ED5BC {
public:
    W8Control005ED66C(W8Controls* panel, unsigned int region, int left, int top,
                      int a0, int a1, int a2, int b0, int b1, int b2);

protected:
    int m_a0;                            /* 0x34 */
    int m_a1;                            /* 0x38 */
    int m_a2;                            /* 0x3c */
    int m_b0;                            /* 0x40 */
    int m_b1;                            /* 0x44 */
    int m_b2;                            /* 0x48 */
    int m_slack_4c;                      /* 0x4c: width left over after the second measure */
    int m_measured_50;                   /* 0x50 */
    int m_field_54;
    unsigned char unknown_58[4];
    unsigned char m_flag_5c;
    unsigned char m_flag_5d;
    unsigned char pad_5e[2];
    int m_field_60;
    float m_scale_64;                    /* 0x64: 1.0f */
    int m_field_68;
    int m_field_6c;
};

// FUNCTION: WIZ8 0x004F5620
W8Control005ED66C::W8Control005ED66C(W8Controls* panel, unsigned int region, int left, int top,
                                     int a0, int a1, int a2, int b0, int b1, int b2)
    : W8WidgetBase005ED5BC(panel, region, left, top, 0, 0)
{
    short width;
    short height;

    m_b0 = b0;
    m_b1 = b1;
    m_b2 = b2;
    m_a0 = a0;
    m_a1 = a1;
    m_a2 = a2;
    m_field_54 = 0;
    m_flag_5c = 0;
    m_flag_5d = 0;
    m_field_60 = 0;
    m_scale_64 = 1.0f;
    m_field_68 = 0;
    m_field_6c = 0;
    Function549660(a0, a1, a2, &width, &height);
    m_right = (unsigned short)width + m_left;
    m_bottom = m_top + (unsigned short)height;
    SetRegion(m_region_18);
    Function549660(m_a0, m_a1, m_b0, &width, &height);
    m_measured_50 = (unsigned short)width;
    m_slack_4c = (m_right - m_left) - (unsigned short)width;
}

/*
 * The class at vtable 0x005ED5A4, which embeds a growable vector at +0x38.
 * That vector is the shared template, not a private copy: it allocates 0x14
 * bytes and records a capacity of five, which is the template's own default of
 * five four-byte elements, and it installs the template table then its own -
 * the second-vtable shape wiz8/vector.h describes.
 *
 * Its element type is unproven, so it is named for the vtable the image gives
 * the vector.
 */
class W8VectorElement005ED5B0;

class W8ElementVector005ED5B0 : public W8GrowableVector<W8VectorElement005ED5B0*> {
public:
    W8ElementVector005ED5B0();
    virtual ~W8ElementVector005ED5B0();
};

class W8Control005ED5A4 {
public:
    W8Control005ED5A4();
    W8Control005ED5A4(int a2, int a3, int a4, int a5, int a6, int a7, int a8);

    virtual void vslot00();

protected:
    unsigned char m_flag_04;
    unsigned char m_flag_05;
    unsigned char m_flag_06;
    unsigned char pad_07;
    int m_field_08;
    int m_field_0c;
    int m_field_10;
    int m_field_14;
    int m_field_18;                      /* 0x18: the three below default to -1 */
    int m_field_1c;
    int m_field_20;
    int m_field_24;                      /* 0x24: always -1 */
    unsigned char unknown_28[0xc];
    unsigned char m_flag_34;             /* 0x34: set to 1 */
    unsigned char pad_35[3];
    W8ElementVector005ED5B0 m_items;     /* 0x38 */
    int m_field_48;
};                                       /* 0x4c, which is the allocation size */

__forceinline W8ElementVector005ED5B0::W8ElementVector005ED5B0()
{
}

/* The default constructor. Everything the seven-argument one takes from its
   caller, this one zeroes or sets to -1. */
// FUNCTION: WIZ8 0x004F2C30
W8Control005ED5A4::W8Control005ED5A4()
{
    m_field_18 = -1;
    m_field_1c = -1;
    m_field_20 = -1;
    m_flag_04 = 0;
    m_flag_05 = 0;
    m_flag_06 = 0;
    m_field_08 = 0;
    m_field_0c = 0;
    m_field_10 = 0;
    m_field_14 = 0;
    m_field_24 = -1;
    m_flag_34 = 1;
    m_field_48 = 0;
}

// FUNCTION: WIZ8 0x004F2CA0
W8Control005ED5A4::W8Control005ED5A4(int a2, int a3, int a4, int a5, int a6, int a7, int a8)
{
    m_field_18 = a6;
    m_field_20 = a8;
    m_field_1c = a7;
    m_field_0c = a3;
    m_field_08 = a2;
    m_field_14 = a5;
    m_flag_04 = 0;
    m_flag_05 = 0;
    m_flag_06 = 0;
    m_field_10 = a4;
    m_field_24 = -1;
    m_flag_34 = 1;
    m_field_48 = 0;
}
