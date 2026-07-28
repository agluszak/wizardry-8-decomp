#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"
#include "wiz8/vector.h"

#include <wchar.h>

/* Local Code\Controls.cpp. m_uiRegionSetId is named by the canonical assertion
   at line 399, and REGSET_NULL is zero because the body's guard is a plain
   test against zero.

   The 18-slot widget base at vtable 0x005ED5BC lives here too. Ghidra
   attributes 0x004F3D90, 0x004F3DD0 and 0x004F4020 to this file by bounded
   interval, and the class the constructor registers into is this one: the
   region set it reads sits at +0x48, which is where the assertion puts
   m_uiRegionSetId. It had been recovered separately against an invented
   Controls of its own, so this merges the two rather than leaving one
   address described by two structures.

   The image names neither the widget class nor most of its fields, so they
   carry address-qualified positional names. The member at +0x1c is the
   exception: the assertion at Controls.cpp:1849 reads m_pPanel != NULL, and
   the pointer it guards is the one the constructor stores there and registers
   into, so that member is named by the original source. It had been called
   m_owner here, which was a guess at the same thing. The three remaining Controls members
   below are positional for the same reason - only m_uiRegionSetId is spoken
   for by the assertion - so they are spelled in the file's style without
   claiming that style is evidence. */
#define REGSET_NULL 0

class W8WidgetBase005ED5BC;
struct W8RangeControlConstruction005ED74C {};

class W8ControlsVector005ED5B0
    : public W8GrowableVector<W8WidgetBase005ED5BC*> {
public:
    W8ControlsVector005ED5B0();
    virtual ~W8ControlsVector005ED5B0();
};

/* The region callback a widget without its own region is given. Ghidra has no
   function at 0x004F3140, only a label, so this is a declaration and not a
   claim on the address. */
extern void Function4F3140(const W8RegionEvent* event, struct W8Region* region);

/* The accumulated redraw rectangle a panel hands the compositor. An empty
   rectangle is spelled with left at -1, which is what 0x004F2E50 tests before
   it starts unioning rather than intersecting. */
struct W8ControlsRect {
    int left;                               /* 0x00 */
    int top;                                /* 0x04 */
    int right;                              /* 0x08 */
    int bottom;                             /* 0x0c */
};

struct Controls {
    Controls();
    Controls(int left, int top, int right, int bottom,
             int render_target, int render_arg_1c, int render_arg_20);
    __forceinline Controls(W8RangeControlConstruction005ED74C,
                           int left, int top, int right, int bottom,
                           int render_target, int render_arg_1c,
                           int render_arg_20);
    __forceinline ~Controls();

    virtual void SetEnabled(unsigned char enable);
    virtual void Invalidate(const W8ControlsRect* rect);
    virtual void Redraw();
    /* 0x04 and 0x05 travel together: SetEnabled writes the panel's own state to
       the first and mirrors it into every child's m_flag_5, and the redraw
       requests raise the second. 0x06 is raised on its own by 0x004F2F00. */
    unsigned char m_fEnabled;               /* 0x04 */
    unsigned char m_fDirty;                 /* 0x05 */
    unsigned char m_fLayoutDirty;           /* 0x06 */
    unsigned char pad_07;
    int origin_x;                           /* 0x08: widget rectangles are relative to this */
    int origin_y;                           /* 0x0c */
    int right;                              /* 0x10: panel bounds propagated to children */
    int bottom;                             /* 0x14 */
    int m_renderTarget;                     /* 0x18: -1 skips target-backed drawing */
    int m_renderArg_1c;                     /* 0x1c: forwarded with the target */
    int m_renderArg_20;                     /* 0x20: forwarded with the target */
    W8ControlsRect m_dirtyRect;             /* 0x24 */
    unsigned char m_fWholeAreaDirty;        /* 0x34: set when a caller passes no rectangle */
    unsigned char unknown_35[3];
    W8ControlsVector005ED5B0 m_controls;    /* 0x38 */
    unsigned int m_uiRegionSetId;           /* 0x48 */

    void EnableRegionSet(unsigned char enable);
    void RemoveControl(W8WidgetBase005ED5BC* control);
    void DestroyAllControls();
    void InvalidateLayout();
    void SetBounds(int left, int top, int right, int bottom);
    void AcquireRegionSet(unsigned int* shared_region_set);

    /* The bounds-checked element read every walker above shares. Out of range
       it answers element zero rather than failing, which is what the canonical
       `p = m_ppControls; if (i < m_nControls) p += i;` compiles from and why
       the guard shows up once per use rather than once per loop. */
    __inline W8WidgetBase005ED5BC* ControlAt(int index)
    {
        if (index < m_controls.count) {
            return m_controls.data[index];
        }
        return m_controls.data[0];
    }
};

__forceinline W8ControlsVector005ED5B0::W8ControlsVector005ED5B0()
{
}

__forceinline W8ControlsVector005ED5B0::~W8ControlsVector005ED5B0()
{
}

/* The class at vtable 0x005ED5A4 is Controls itself. Its embedded vector at
   +0x38 stores the widget pointers walked by every panel method; the former
   W8Control005ED5A4 declaration duplicated this same layout under a candidate
   name and hid that proven element type. */

/* The default constructor. Everything the seven-argument one takes from its
   caller, this one zeroes or sets to -1. */
// FUNCTION: WIZ8 0x004F2C30
Controls::Controls()
{
    m_renderTarget = -1;
    m_renderArg_1c = -1;
    m_renderArg_20 = -1;
    m_fEnabled = 0;
    m_fDirty = 0;
    m_fLayoutDirty = 0;
    origin_x = 0;
    origin_y = 0;
    right = 0;
    bottom = 0;
    m_dirtyRect.left = -1;
    m_fWholeAreaDirty = 1;
    m_uiRegionSetId = 0;
}

// FUNCTION: WIZ8 0x004F2CA0
Controls::Controls(int left, int top, int right_bound, int bottom_bound,
                   int render_target, int render_arg_1c, int render_arg_20)
{
    origin_x = left;
    right = right_bound;
    m_fEnabled = 0;
    m_fDirty = 0;
    m_fLayoutDirty = 0;
    m_renderTarget = render_target;
    m_renderArg_1c = render_arg_1c;
    m_renderArg_20 = render_arg_20;
    origin_y = top;
    bottom = bottom_bound;
    m_dirtyRect.left = -1;
    m_fWholeAreaDirty = 1;
    m_uiRegionSetId = 0;
}

__forceinline Controls::Controls(
    W8RangeControlConstruction005ED74C,
    int left, int top, int right_bound, int bottom_bound,
    int render_target, int render_arg_1c, int render_arg_20)
{
    origin_x = left;
    right = right_bound;
    m_fEnabled = 0;
    m_fDirty = 0;
    m_fLayoutDirty = 0;
    m_renderTarget = render_target;
    m_renderArg_1c = render_arg_1c;
    m_renderArg_20 = render_arg_20;
    origin_y = top;
    bottom = bottom_bound;
    m_dirtyRect.left = -1;
    m_fWholeAreaDirty = 1;
    m_uiRegionSetId = 0;
}

// FUNCTION: WIZ8 0x004F2D30
__forceinline Controls::~Controls()
{
}

/* 0x00562A50 takes the redraw-request mask the panel raises. */
extern void Function562A50(unsigned int mask);
extern void Function422D50(int left, int top, int right, int bottom, int mode);
extern void Function548F90(int operation, int target, int arg_1c, int arg_20,
                           int left, int top, int mode, int flags);
extern void Function5494F0(int target, int arg_1c, int arg_20,
                           int left, int top, int mode);
extern void Function549600(int operation, int target, int arg_1c, int arg_20,
                           int left, int top, int mode, int flags);
extern unsigned short Function4071F0(int font);             /* font line height */
extern short Function407010(const wchar_t* text, int font);
extern void Function407210(int font);
extern int Function406DF0(int font);
extern void Function4068E0(int font_context, int render_mode);
extern int Function406DE0(int font);
extern void Function406DC0(int font, int state);
extern void Function407090();
extern void Function407140();
extern void Function407220(int target, int left, int top, int right, int bottom,
                           int flags);
extern void Function407A10(int a, int b, int font, int x, int y,
                           const wchar_t* format, const wchar_t* text);
extern void Function407B80(int a, int b, int font, int x, int y);
extern unsigned char FillSurfaceRect(int surface_id, int left, int top, int right,
                                     int bottom, int colour);
extern const wchar_t g_W8EmptyText0060CC74[];
extern const wchar_t g_W8EmptyHelpText00689B34[];
extern const wchar_t g_W8TextBreakCharacters00617C88[];
extern const wchar_t g_W8LineBreakCharacters00617C90[];
extern const wchar_t g_W8TextFormat006068E4[];
extern int g_W8TextClipTarget005FF5F4;
extern int g_W8TextClipFlags00650E38;
extern int g_W8FontStateTable0068EE1C[];
extern float g_W8RangeStart005EBB34;
extern float g_W8RangeEnd005EBB38;
extern float g_W8RangeHalfStep005EBC7C;
extern unsigned int g_W8TextControlMask005ED56C;
extern unsigned int g_W8TextControlMask005ED570;
extern void Function558720(int sound_id);
extern void Function5587C0(int a, int b);
extern void Function4284F0(int* coordinates);
extern void Function4F2040(unsigned int region);

typedef void (*W8ControlCallback)();

// FUNCTION: WIZ8 0x004F30F0
void Controls::EnableRegionSet(unsigned char enable)
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
    friend struct Controls;

    W8WidgetBase005ED5BC(Controls* owner, unsigned int region,
                         int left, int top, int right, int bottom);

    void SetRegion(unsigned int region);
    void Invalidate(unsigned char immediate);
    void SetEnabled(unsigned char enabled);
    void EnableRegionHelp(int help_text_id);
    void DisableRegionHelp();

    virtual ~W8WidgetBase005ED5BC();

    virtual void SetVisible(unsigned char visible);
    virtual void Redraw(int full_redraw) = 0;
    virtual void SetBounds(int left, int top, int right, int bottom);
    virtual void SetBoundsFromRect(const W8ControlsRect* bounds);

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
    Controls* m_pPanel;                  /* 0x1c: named by Controls.cpp:1849 */
    W8ControlCallback m_primaryCallback; /* 0x20: invoked by text-control activation */
    int m_field_24;                      /* 0x24: otherwise touched by the recovered */
    W8ControlCallback m_secondaryCallback;/* 0x28 */
    W8ControlCallback m_focusCallback;   /* 0x2c */
    W8ControlCallback m_blurCallback;    /* 0x30 */
};                                       /* 0x34 established */

// FUNCTION: WIZ8 0x004F3F10
W8WidgetBase005ED5BC::~W8WidgetBase005ED5BC()
{
    m_flag_5 = 0;
    if (m_region_18 != -1) {
        SetRegionMode4(m_region_18);
    }
}


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
W8WidgetBase005ED5BC::W8WidgetBase005ED5BC(Controls* owner, unsigned int region,
                                           int left, int top, int right, int bottom)
{
    Controls* holder;
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
    m_primaryCallback = 0;
    m_field_24 = 0;
    m_secondaryCallback = 0;
    m_focusCallback = 0;
    m_blurCallback = 0;
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
    wanted = holder->m_controls.count + 1;
    if (holder->m_controls.capacity < wanted) {
        previous = holder->m_controls.data;
        holder->m_controls.data = (W8WidgetBase005ED5BC**)new void*[wanted];
        if (holder->m_controls.data == 0) {
            holder->m_controls.data = previous;
            index = -1;
            goto registered;
        }
        i = 0;
        holder->m_controls.capacity = wanted;
        if (0 < holder->m_controls.count) {
            do {
                holder->m_controls.data[i] = previous[i];
                i = i + 1;
            } while (i < holder->m_controls.count);
        }
        delete[] previous;
    }
    holder->m_controls.data[holder->m_controls.count] = this;
    index = holder->m_controls.count;
    holder->m_controls.count = index + 1;

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
    Controls* holder;
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
    W8TextBuffer005ED5B8(const W8ControlsRect* bounds, const wchar_t* text,
                         int font, unsigned int layout_mode, int render_mode);
    void CopyTextTo(wchar_t* destination);
    unsigned int GetLineHeight();
    int GetHorizontalPosition(int width);
    int GetVerticalPosition();
    void SetLineHeight(unsigned int height);
    void FillBounds(int colour);
    void RenderText(int a, int b, int x_offset, int y_offset,
                    unsigned char force);
    void UpdateLayout();                  /* 0x004F35B0 */
    void SetLayoutMode(unsigned int layout_mode);
    void SetText(const wchar_t* text, int font);
    void SetLayoutBounds(const W8ControlsRect* bounds,
                         unsigned char copy_pending,
                         unsigned char update_layout);

    __forceinline void SetLayoutBounds(int left, int top, int right, int bottom)
    {
        m_layoutBounds.left = left;
        m_layoutBounds.top = top;
        m_layoutBounds.right = right;
        m_layoutBounds.bottom = bottom;
        m_pendingBounds = m_layoutBounds;
    }

    __forceinline int HasBuffer() const { return m_buffer != 0; }
    __forceinline void SetGeometryDirty() { m_geometryDirty = 1; }
    __forceinline void SetRenderMode(int mode) { m_renderMode = mode; }
    __forceinline void MarkGeometryDirty(int mode)
    {
        m_geometryDirty = 1;
        m_layoutMode = mode;
    }

    virtual ~W8TextBuffer005ED5B8();

protected:
    W8ControlsRect m_layoutBounds;        /* 0x04: current absolute layout bounds */
    W8ControlsRect m_pendingBounds;       /* 0x14: mirrored pending bounds */
    int m_field_24;                      /* 0x24: the constructor steps over this one */
    int m_font;                          /* 0x28: font used for uncached line height */
    int m_lineCount;
    unsigned int m_lineHeight;           /* 0x30: cached height, zero means query font */
    wchar_t* m_buffer;                   /* 0x34: freed on teardown */
    int m_layoutMode;                    /* 0x38: 10 initially */
    unsigned int m_maxLineWidth;
    unsigned char m_geometryDirty;
    unsigned char m_alternateRenderer;
    unsigned char pad_42[2];
    int m_renderMode;                    /* 0x44: 4 initially */
    int m_fontStateIndex;                /* 0x48: -1 skips the state-table override */
    unsigned char m_flag_4c;
};

// FUNCTION: WIZ8 0x004F3480
W8TextBuffer005ED5B8::~W8TextBuffer005ED5B8()
{
    delete[] m_buffer;
}

// FUNCTION: WIZ8 0x004F3310
W8TextBuffer005ED5B8::W8TextBuffer005ED5B8()
{
    m_buffer = 0;
    m_font = 0;
    m_lineCount = 0;
    m_geometryDirty = 0;
    m_layoutMode = 10;
    m_maxLineWidth = 0;
    m_lineHeight = 0;
    m_renderMode = 4;
    m_alternateRenderer = 0;
    m_fontStateIndex = -1;
    m_flag_4c = 0;
    m_layoutBounds.left = 0;
    m_layoutBounds.top = 0;
    m_layoutBounds.right = 0;
    m_layoutBounds.bottom = 0;
    m_pendingBounds.left = 0;
    m_pendingBounds.top = 0;
    m_pendingBounds.right = 0;
    m_pendingBounds.bottom = 0;
}

// FUNCTION: WIZ8 0x004F33A0
W8TextBuffer005ED5B8::W8TextBuffer005ED5B8(const W8ControlsRect* bounds,
                                           const wchar_t* text,
                                           int font,
                                           unsigned int layout_mode,
                                           int render_mode)
{
    m_buffer = 0;
    m_font = 0;
    m_lineCount = 0;
    m_maxLineWidth = 0;
    m_geometryDirty = 0;
    m_lineHeight = 0;
    m_alternateRenderer = 0;
    m_layoutMode = layout_mode;
    if ((m_layoutMode & 7) == 0) {
        m_layoutMode |= 2;
    }
    if ((m_layoutMode & 0x38) == 0) {
        m_layoutMode |= 8;
    }
    m_renderMode = render_mode;
    m_fontStateIndex = -1;
    m_flag_4c = 0;
    m_layoutBounds = *bounds;
    m_pendingBounds = *bounds;
    m_lineCount = 0;
    m_font = font;
    if (text != 0) {
        m_buffer = new wchar_t[wcslen(text) + 1];
        wcscpy(m_buffer, text);
        UpdateLayout();
        m_geometryDirty = 1;
        return;
    }
    m_buffer = 0;
    m_geometryDirty = 1;
}

// FUNCTION: WIZ8 0x004F34A0
void W8TextBuffer005ED5B8::SetLayoutMode(unsigned int layout_mode)
{
    m_layoutMode = layout_mode;
    if ((m_layoutMode & 7) == 0) {
        m_layoutMode |= 2;
    }
    if ((m_layoutMode & 0x38) == 0) {
        m_layoutMode |= 8;
    }
}

// FUNCTION: WIZ8 0x004F34D0
void W8TextBuffer005ED5B8::SetText(const wchar_t* text, int font)
{
    m_font = font;
    m_lineCount = 0;
    delete[] m_buffer;
    if (text != 0) {
        m_buffer = new wchar_t[wcslen(text) + 1];
        wcscpy(m_buffer, text);
        UpdateLayout();
        m_geometryDirty = 1;
        return;
    }
    m_buffer = 0;
    m_geometryDirty = 1;
}

// FUNCTION: WIZ8 0x004F3540
void W8TextBuffer005ED5B8::SetLayoutBounds(const W8ControlsRect* bounds,
                                           unsigned char copy_pending,
                                           unsigned char update_layout)
{
    m_layoutBounds = *bounds;
    if (copy_pending) {
        m_pendingBounds = *bounds;
    }
    if (update_layout && m_buffer != 0) {
        UpdateLayout();
    }
    m_geometryDirty = 1;
}

// FUNCTION: WIZ8 0x004F35B0
void W8TextBuffer005ED5B8::UpdateLayout()
{
    unsigned int available_width = m_layoutBounds.right - m_layoutBounds.left;
    wchar_t* line = m_buffer;
    unsigned int accumulated_width = 0;
    wchar_t* previous_break = 0;
    short separator_width = Function407010(g_W8EmptyText0060CC74, m_font);

    m_lineCount = 1;
    if ((m_layoutMode & 0x40) == 0) {
        m_maxLineWidth = 0;
        size_t span = wcscspn(line, g_W8TextBreakCharacters00617C88);
        wchar_t* break_at = line + span;
        while (*break_at != L'\0') {
            *break_at = L'\0';
            short word_width = Function407010(line, m_font);
            if ((unsigned int)((int)word_width + accumulated_width) < available_width) {
                accumulated_width += (int)separator_width + (int)word_width;
                previous_break = break_at;
            } else {
                if (previous_break != 0) {
                    *previous_break = L'\n';
                }
                unsigned int completed_width = accumulated_width - (int)separator_width;
                if (m_maxLineWidth < completed_width) {
                    m_maxLineWidth = completed_width;
                }
                accumulated_width = (int)separator_width + (int)word_width;
                previous_break = 0;
                ++m_lineCount;
            }
            line += span + 1;
            *break_at = L' ';
            span = wcscspn(line, g_W8TextBreakCharacters00617C88);
            break_at = line + span;
        }
        short final_width = Function407010(line, m_font);
        unsigned int total_width = (int)final_width + accumulated_width;
        if (available_width <= total_width) {
            if (previous_break != 0) {
                *previous_break = L'\n';
            }
            if (m_maxLineWidth < accumulated_width) {
                m_maxLineWidth = accumulated_width;
            }
            total_width = (int)separator_width + (int)final_width;
            ++m_lineCount;
        }
        if (m_maxLineWidth < total_width) {
            m_maxLineWidth = total_width;
        }
        return;
    }
    m_maxLineWidth = wcslen(m_buffer);
}

/* Resolves one measured line against the horizontal alignment flags. */
// FUNCTION: WIZ8 0x004F3C00
int W8TextBuffer005ED5B8::GetHorizontalPosition(int width)
{
    if ((m_layoutMode & 2) != 0) {
        int inset = (m_layoutBounds.right - m_layoutBounds.left - width) / 2;
        if (inset < 0) {
            inset = 0;
        }
        return m_layoutBounds.left + inset;
    }
    if ((m_layoutMode & 4) == 0) {
        return m_layoutBounds.left;
    }
    int position = m_layoutBounds.right - width;
    if (position <= m_layoutBounds.left) {
        return m_layoutBounds.left;
    }
    return position;
}

/* Resolves the first baseline from the line count, line height and vertical
   alignment flags, including the font's own height inside a larger override. */
// FUNCTION: WIZ8 0x004F3C50
int W8TextBuffer005ED5B8::GetVerticalPosition()
{
    unsigned int line_height = m_lineHeight;
    unsigned int font_height = Function4071F0(m_font);
    unsigned int first_line_inset;
    if (line_height == 0) {
        line_height = font_height;
        first_line_inset = 0;
    } else {
        first_line_inset = (line_height - font_height + 1) >> 1;
    }

    if ((m_layoutMode & 0x10) != 0) {
        return m_layoutBounds.top + first_line_inset;
    }
    int free_height = (m_layoutBounds.bottom - m_layoutBounds.top) -
                      m_lineCount * line_height;
    if (free_height < 0) {
        free_height = 0;
    }
    if ((m_layoutMode & 0x20) != 0) {
        return m_layoutBounds.top + free_height + first_line_inset;
    }
    return m_layoutBounds.top + free_height / 2 + first_line_inset;
}

/* Rejects an override smaller than the active font's natural line height. */
// FUNCTION: WIZ8 0x004F3CF0
void W8TextBuffer005ED5B8::SetLineHeight(unsigned int height)
{
    if (height < Function4071F0(m_font)) {
        m_lineHeight = 0;
        return;
    }
    m_lineHeight = height;
}

/* Fills the current text rectangle on the standard UI surface and queues the
   same rectangle for composition. */
// FUNCTION: WIZ8 0x004F3D50
void W8TextBuffer005ED5B8::FillBounds(int colour)
{
    FillSurfaceRect(-14, m_layoutBounds.left, m_layoutBounds.top,
                    m_layoutBounds.right, m_layoutBounds.bottom, colour);
    Function422D50(m_layoutBounds.left, m_layoutBounds.top,
                   m_layoutBounds.right, m_layoutBounds.bottom, 0);
}

/* Draws each newline-delimited line through the active font context. The
   temporary terminators are restored before advancing to the next line. */
// FUNCTION: WIZ8 0x004F3710
void W8TextBuffer005ED5B8::RenderText(int a, int b, int x_offset, int y_offset,
                                     unsigned char force)
{
    wchar_t* line = m_buffer;
    if (line == 0 || (force == 0 && m_geometryDirty == 0)) {
        return;
    }

    Function407210(m_font);
    int font_context = Function406DF0(m_font);
    if (font_context == 0) {
        return;
    }
    Function4068E0(font_context, m_renderMode);
    int previous_state = Function406DE0(m_font);
    if (m_fontStateIndex != -1) {
        Function406DC0(m_font, g_W8FontStateTable0068EE1C[m_fontStateIndex]);
    }
    Function407090();
    Function407220(g_W8TextClipTarget005FF5F4,
                   m_pendingBounds.left, m_pendingBounds.top,
                   m_pendingBounds.right, m_pendingBounds.bottom,
                   g_W8TextClipFlags00650E38);

    int y = GetVerticalPosition();
    size_t span = wcscspn(line, g_W8LineBreakCharacters00617C90);
    while (line[span] != L'\0') {
        line[span] = L'\0';
        int x = GetHorizontalPosition(Function407010(line, m_font));
        if (m_alternateRenderer == 0) {
            Function407A10(a, b, m_font, x + x_offset, y + y_offset,
                           g_W8TextFormat006068E4, line);
        } else {
            Function407B80(a, b, m_font, x + x_offset, y + y_offset);
        }
        y += GetLineHeight();
        line[span] = L'\n';
        if (m_layoutBounds.bottom <= y) {
            goto done;
        }
        line += span + 1;
        span = wcscspn(line, g_W8LineBreakCharacters00617C90);
    }

    {
        int x = GetHorizontalPosition(Function407010(line, m_font));
        if (m_alternateRenderer == 0) {
            Function407A10(a, b, m_font, x + x_offset, y + y_offset,
                           g_W8TextFormat006068E4, line);
        } else {
            Function407B80(a, b, m_font, x + x_offset, y + y_offset);
        }
    }

done:
    Function406DC0(m_font, previous_state);
    Function407140();
    m_geometryDirty = 0;
}

/* Copies the owned text into caller storage. The caller supplies the capacity;
   the canonical method performs the same unbounded wide-string copy. */
// FUNCTION: WIZ8 0x004F3990
void W8TextBuffer005ED5B8::CopyTextTo(wchar_t* destination)
{
    wcscpy(destination, m_buffer);
}

/* Returns the cached line height, falling back to the active font's 16-bit
   height when the cache is zero. */
// FUNCTION: WIZ8 0x004F3D30
unsigned int W8TextBuffer005ED5B8::GetLineHeight()
{
    unsigned int height = m_lineHeight;
    if (height == 0) {
        height = Function4071F0(m_font);
    }
    return height;
}

/*
 * The text control at vtable 0x005ED604. The bounds and draw methods prove it
 * derives from the widget base and embeds a complete W8TextBuffer005ED5B8 at
 * +0x60: 0x004F44D0 writes both rectangle pairs inside that subobject and
 * passes its +0x60 address as this to W8TextBuffer005ED5B8::UpdateLayout.
 */
class W8TextControl005ED604 : public W8WidgetBase005ED5BC {
public:
    W8TextControl005ED604(Controls* panel, unsigned int region,
                          int left, int top, int right, int bottom,
                          int text_40, int text_44, int text_48, int text_4c,
                          int text_54, int text_50, int text_58);
    virtual ~W8TextControl005ED604();
    W8TextControl005ED604* scalar_deleting_destructor(unsigned char flags);
    void GetTextOrigin(int unused, int* px, int* py);
    void Invalidate(unsigned char immediate);
    virtual void SetVisible(unsigned char visible);
    virtual void Redraw(int full_redraw);
    void SetFlaggedRegionBounds(short left, short top, unsigned short right);
    virtual void AddLayoutFlags(unsigned int flags);
    virtual void SetAlternateTextEnabled(unsigned char enabled);
    void RemoveLayoutFlags(unsigned int flags);
    void EnableSecondaryState(unsigned char immediate);
    void DisableSecondaryState(unsigned char immediate);
    void Function4D30(int event);
    void Function4F70(int event);
    virtual void InvokeFocusCallback();
    virtual void ActivatePrimary();
    virtual void InvokeBlurCallback();
    virtual void ActivateSecondary();
    void UpdateTextBounds(int left, int top, int right, int bottom);

    virtual void SetBounds(int left, int top, int right, int bottom);
    virtual void SetBoundsFromRect(const W8ControlsRect* bounds);

protected:
    unsigned int m_stateFlags;           /* 0x34: paired state masks */
    unsigned int m_flags_38;             /* 0x38: 0x02 builds layout, 0x04 pins left */
    unsigned char m_alternateTextEnabled;/* 0x3c: alternate text-selection flag */
    unsigned char pad_3d[3];
    int m_text_40;
    int m_text_44;
    int m_text_48;
    int m_text_4c;
    int m_text_50;
    int m_text_54;
    int m_text_58;
    short m_measured_w;                  /* 0x5c: -1 until measured */
    short m_measured_h;                  /* 0x5e */
    W8TextBuffer005ED5B8 m_textBuffer;   /* 0x60: complete typed subobject */
    int m_field_b0;
    class Listener {
    public:
        virtual void OnPrimary(W8TextControl005ED604* control) = 0;
        virtual void OnSecondary(W8TextControl005ED604* control) = 0;
    } *m_listener;                       /* 0xb4 */

    __forceinline void InvalidateCore(unsigned char immediate)
    {
        if (m_pPanel != 0) {
            m_flag_6 = 1;
            if (immediate) {
                m_pPanel->Invalidate(0);
            } else {
                m_pPanel->m_fLayoutDirty = 1;
                Function562A50(0x80000000);
            }
            Function562A50(0x80000000);
        }
        m_textBuffer.SetGeometryDirty();
    }
};

/* The 182-caller text-control constructor. The first six arguments construct
   the reviewed widget base, while the implicit W8TextBuffer constructor owns
   the second EH state. The remaining positional values and the two measured
   shorts are fixed by the constructor's direct stores and Function549660 call;
   their descriptive identities remain unknown. */
// FUNCTION: WIZ8 0x004F4250
W8TextControl005ED604::W8TextControl005ED604(
    Controls* panel, unsigned int region,
    int left, int top, int right, int bottom,
    int text_40, int text_44, int text_48, int text_4c,
    int text_54, int text_50, int text_58)
    : W8WidgetBase005ED5BC(panel, region, left, top, right, bottom)
{
    m_text_4c = text_4c;
    m_text_50 = text_50;
    m_text_54 = text_54;
    m_stateFlags = 0;
    m_alternateTextEnabled = 0;
    m_text_40 = text_40;
    m_text_44 = text_44;
    m_text_48 = text_48;
    m_text_58 = text_58;
    m_flags_38 = 0;
    m_listener = 0;
    m_field_b0 = 1;

    int measured_text = text_48;
    if (text_40 == -1 || text_44 == -1 ||
        (measured_text == -1 && (measured_text = text_4c) == -1)) {
        m_measured_w = -1;
        m_measured_h = -1;
    } else {
        Function549660(text_40, text_44, measured_text,
                       &m_measured_w, &m_measured_h);
    }

    if (right == 0) {
        right = left + m_measured_w;
    }
    if (bottom == 0) {
        bottom = top + m_measured_h;
    }
    SetBounds(left, top, right, bottom);

    m_textBuffer.SetLayoutBounds(panel->origin_x + left,
                                 panel->origin_y + top,
                                 panel->origin_x + right,
                                 panel->origin_y + bottom);
    if (m_textBuffer.HasBuffer()) {
        m_textBuffer.UpdateLayout();
    }
    m_textBuffer.MarkGeometryDirty(10);
}

/* Where the text should be drawn: the panel origin plus either the widget's
   corner or an alignment computed from its cached measured extent. */
// FUNCTION: WIZ8 0x004F4850
void W8TextControl005ED604::GetTextOrigin(int unused, int* px, int* py)
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

// FUNCTION: WIZ8 0x004F4460
void W8TextControl005ED604::SetBoundsFromRect(const W8ControlsRect* bounds)
{
    SetBounds(bounds->left, bounds->top, bounds->right, bounds->bottom);
    if (m_region_18 != -1 && m_pPanel != 0) {
        SetRegionBounds(m_region_18,
                        (unsigned short)((short)bounds->left + (short)m_pPanel->origin_x),
                        (unsigned short)((short)bounds->top + (short)m_pPanel->origin_y),
                        (unsigned short)((short)bounds->right + (short)m_pPanel->origin_x),
                        (unsigned short)((short)bounds->bottom + (short)m_pPanel->origin_y));
    }
}

// FUNCTION: WIZ8 0x004F44D0
void W8TextControl005ED604::SetBounds(int left, int top, int right, int bottom)
{
    short measured_width;
    short measured_height;

    m_left = left;
    m_top = top;
    m_right = right;
    m_bottom = bottom;
    if (m_pPanel != 0) {
        if (m_region_18 != -1) {
            SetRegionBounds(m_region_18,
                            (unsigned short)((short)left + (short)m_pPanel->origin_x),
                            (unsigned short)((short)top + (short)m_pPanel->origin_y),
                            (unsigned short)((short)right + (short)m_pPanel->origin_x),
                            (unsigned short)((short)bottom + (short)m_pPanel->origin_y));
        }
        if ((m_flags_38 & 2) != 0) {
            int absolute_left = m_pPanel->origin_x + left;
            int absolute_top = m_pPanel->origin_y + top;
            int absolute_right = m_pPanel->origin_x + right;
            int absolute_bottom = m_pPanel->origin_y + bottom;
            if (m_text_40 != -1 && m_text_44 != -1) {
                Function549660(m_text_40, m_text_44, m_text_48,
                               &measured_width, &measured_height);
                if ((m_flags_38 & 4) != 0) {
                    absolute_left += 2 + (unsigned short)measured_width;
                } else {
                    absolute_right -= 2 + (unsigned short)measured_width;
                }
            }
            m_textBuffer.SetLayoutBounds(absolute_left, absolute_top,
                                         absolute_right, absolute_bottom);
            if (m_textBuffer.HasBuffer()) {
                m_textBuffer.UpdateLayout();
            }
            m_textBuffer.MarkGeometryDirty(9);
        }
    }
}

// FUNCTION: WIZ8 0x004F4650
void W8TextControl005ED604::Invalidate(unsigned char immediate)
{
    InvalidateCore(immediate);
}

// FUNCTION: WIZ8 0x004F4600
void W8TextControl005ED604::SetFlaggedRegionBounds(short left, short top,
                                                   unsigned short right)
{
    if (m_region_18 != -1 && m_pPanel != 0 && RegionHasFlags(m_region_18, 2)) {
        SetRegionBounds(m_region_18,
                        (unsigned short)((short)m_pPanel->origin_x + left),
                        (unsigned short)((short)m_pPanel->origin_y + top),
                        right,
                        0);
    }
}

// FUNCTION: WIZ8 0x004F46A0
void W8TextControl005ED604::AddLayoutFlags(unsigned int flags)
{
    short measured_width;
    short measured_height;

    m_flags_38 |= flags;
    if (m_pPanel != 0 && (m_flags_38 & 2) != 0) {
        int absolute_left = m_pPanel->origin_x + m_left;
        int absolute_top = m_pPanel->origin_y + m_top;
        int absolute_right = m_pPanel->origin_x + m_right;
        int absolute_bottom = m_pPanel->origin_y + m_bottom;
        if (m_text_40 != -1 && m_text_44 != -1) {
            Function549660(m_text_40, m_text_44, m_text_48,
                           &measured_width, &measured_height);
            if ((m_flags_38 & 4) != 0) {
                absolute_left += 2 + (unsigned short)measured_width;
            } else {
                absolute_right -= 2 + (unsigned short)measured_width;
            }
        }
        m_textBuffer.SetLayoutBounds(absolute_left, absolute_top,
                                     absolute_right, absolute_bottom);
        if (m_textBuffer.HasBuffer()) {
            m_textBuffer.UpdateLayout();
        }
        m_textBuffer.MarkGeometryDirty(9);
    }
}

// FUNCTION: WIZ8 0x004F4780
void W8TextControl005ED604::RemoveLayoutFlags(unsigned int flags)
{
    if ((flags & 2) != 0 && m_pPanel != 0) {
        m_textBuffer.SetLayoutBounds(m_pPanel->origin_x + m_left,
                                     m_pPanel->origin_y + m_top,
                                     m_pPanel->origin_x + m_right,
                                     m_pPanel->origin_y + m_bottom);
        if (m_textBuffer.HasBuffer()) {
            m_textBuffer.UpdateLayout();
        }
        m_textBuffer.MarkGeometryDirty(10);
    }
    m_flags_38 &= ~flags;
}

// FUNCTION: WIZ8 0x004F4C40
void W8TextControl005ED604::EnableSecondaryState(unsigned char immediate)
{
    if ((m_flags_38 & 1) != 0 && (m_stateFlags & 2) == 0) {
        m_stateFlags |= g_W8TextControlMask005ED56C;
        m_stateFlags |= g_W8TextControlMask005ED570;
        InvalidateCore(immediate);
    }
}

// FUNCTION: WIZ8 0x004F4CB0
void W8TextControl005ED604::DisableSecondaryState(unsigned char immediate)
{
    if ((m_flags_38 & 1) != 0 && (m_stateFlags & 2) != 0) {
        m_stateFlags &= ~g_W8TextControlMask005ED56C;
        m_stateFlags &= ~g_W8TextControlMask005ED570;
        InvalidateCore(immediate);
    }
}

// FUNCTION: WIZ8 0x004F5070
void W8TextControl005ED604::InvokeFocusCallback()
{
    if ((m_flag_5 != 0 && m_flag_4 != 0)) {
        if ((m_flags_38 & 0x20) != 0) {
            Function5587C0(0, 1);
        }
        if (m_focusCallback != 0) {
            m_focusCallback();
        }
        return;
    }
    if (m_flag_5 == 0 && m_flag_4 != 0) {
        return;
    }
    Function5587C0(0, 1);
}

// FUNCTION: WIZ8 0x004F5230
void W8TextControl005ED604::ActivatePrimary()
{
    if ((m_flags_38 & 0x100) != 0 && m_flag_5 != 0 && m_flag_4 != 0 &&
        (m_stateFlags & 1) != 0) {
        m_stateFlags |= 4;
        if ((m_flags_38 & 0x20) == 0) {
            Function558720(3);
        }
        if (m_listener != 0) {
            m_listener->OnPrimary(this);
        }
        if (m_primaryCallback != 0) {
            m_primaryCallback();
        }
    }
}

// FUNCTION: WIZ8 0x004F5310
void W8TextControl005ED604::InvokeBlurCallback()
{
    if (m_flag_5 != 0 && m_flag_4 != 0) {
        if ((m_flags_38 & 0x20) != 0) {
            Function5587C0(0, 1);
        }
        if ((m_flags_38 & 1) != 0 && (m_stateFlags & 2) == 0) {
            return;
        }
        if (m_blurCallback != 0) {
            m_blurCallback();
        }
        return;
    }
    if (m_flag_5 == 0 && m_flag_4 != 0) {
        return;
    }
    Function5587C0(0, 1);
}

// FUNCTION: WIZ8 0x004F5360
void W8TextControl005ED604::ActivateSecondary()
{
    if ((m_flags_38 & 0x100) != 0 && m_flag_5 != 0 && m_flag_4 != 0) {
        m_stateFlags |= 4;
        if ((m_flags_38 & 0x20) == 0) {
            Function558720(3);
        }
        if (m_listener != 0) {
            m_listener->OnSecondary(this);
        }
        if (m_secondaryCallback != 0) {
            m_secondaryCallback();
        }
    }
}

// FUNCTION: WIZ8 0x004F53B0
void W8TextControl005ED604::UpdateTextBounds(int left, int top, int right, int bottom)
{
    W8ControlsRect absolute = {
        m_pPanel->origin_x + left,
        m_pPanel->origin_y + top,
        m_pPanel->origin_x + right,
        m_pPanel->origin_y + bottom
    };
    m_textBuffer.SetLayoutBounds(&absolute, 1, 0);
    if (m_textBuffer.HasBuffer()) {
        m_textBuffer.UpdateLayout();
    }
    m_textBuffer.SetGeometryDirty();
}

// FUNCTION: WIZ8 0x004F5410
void W8TextControl005ED604::SetVisible(unsigned char visible)
{
    m_flag_4 = visible;
    if (visible == 0) {
        if (m_text_58 != -1) {
            m_textBuffer.SetRenderMode(7);
        }
    } else if (m_text_58 != -1) {
        m_textBuffer.SetRenderMode(4);
    }
}

// FUNCTION: WIZ8 0x004F69A0
void W8TextControl005ED604::SetAlternateTextEnabled(unsigned char enabled)
{
    m_alternateTextEnabled = enabled;
}

/* The 0x005ed758 text-control variant owns a fixed 200-character region-help
   buffer immediately after the reviewed W8TextControl base. */
class W8HelpTextControl005ED758 : public W8TextControl005ED604 {
public:
    W8HelpTextControl005ED758(Controls* panel, unsigned int region,
                              int left, int top, int right, int bottom);
    void SetRegionHelp(const wchar_t* text);
    void UpdateRegionHelp(int event);
    virtual void InvokeFocusCallback();
    virtual void ActivateSecondary();
    virtual void InvokeBlurCallback();

protected:
    wchar_t m_regionHelp[200];            /* 0xb8 */
};

// FUNCTION: WIZ8 0x004F65E0
W8HelpTextControl005ED758::W8HelpTextControl005ED758(
    Controls* panel, unsigned int region,
    int left, int top, int right, int bottom)
    : W8TextControl005ED604(panel, region, left, top, right, bottom,
                           -1, -1, -1, -1, -1, -1, -1)
{
    wcscpy(m_regionHelp, g_W8EmptyHelpText00689B34);
}

class W8RangeControl005ED74C;

class W8VerticalRangeThumb005ED6B4 : public W8WidgetBase005ED5BC {
public:
    W8VerticalRangeThumb005ED6B4(W8RangeControl005ED74C* range,
                                 int left, int top, int right, int bottom,
                                 int render_arg, int normal_sprite,
                                 int hovered_sprite, int disabled_sprite);
    virtual void Redraw(int full_redraw);
    void AdjustValue(int steps);
    void BeginDrag(int event);
    void EndDrag(int event);
    void UpdateDrag(int event);
    __forceinline void SetRangePosition(float position)
    {
        m_position = position;
        if (m_position < m_minimumPosition) {
            m_position = m_minimumPosition;
        }
        if (m_maximumPosition < m_position) {
            m_position = m_maximumPosition;
        }
        m_pixelPosition = (int)(((m_position - m_minimumPosition) /
                                 (m_maximumPosition - m_minimumPosition)) *
                                m_trackLength);
        if (m_pPanel != 0) {
            m_flag_6 = 1;
            m_pPanel->m_fLayoutDirty = 1;
            Function562A50(0x80000000);
            Function562A50(0x80000000);
        }
    }

protected:
    int m_renderArg;
    int m_renderArg38;
    int m_normalSprite;
    int m_hoveredSprite;
    int m_disabledSprite;
    int m_drawOffsetX;
    int m_pixelPosition;                 /* 0x4c */
    int m_thumbHeight;                   /* 0x50 */
    int m_trackLength;                   /* 0x54 */
    int m_dragCoordinate;                /* 0x58 */
    unsigned char m_hovered;             /* 0x5c */
    unsigned char m_dragging;            /* 0x5d */
    unsigned char pad_5e[2];
    float m_minimumPosition;             /* 0x60 */
    float m_maximumPosition;             /* 0x64 */
    float m_position;                    /* 0x68 */
    W8RangeControl005ED74C* m_range;     /* 0x6c */

    void ClampPositionAndInvalidate();
    void SynchronizeRangeValue();
};

class W8RangeListener {
public:
    virtual void OnRangeChanged(W8RangeControl005ED74C* control) = 0;
};

class W8RangeControl005ED74C : public Controls {
public:
    friend class W8VerticalRangeThumb005ED6B4;

    W8RangeControl005ED74C(int left, int top, int right, int bottom,
                           unsigned int* shared_region_set);
    ~W8RangeControl005ED74C();

    void SetRange(int first, int second);
    void SetValue(int value);
    void Decrement();
    void Increment();
    void SetEnabled(unsigned char enabled);

protected:
    int m_minimum;                       /* 0x4c */
    int m_maximum;                       /* 0x50 */
    int m_value;                         /* 0x54 */
    W8WidgetBase005ED5BC* m_decrement;   /* 0x58 */
    W8WidgetBase005ED5BC* m_increment;   /* 0x5c */
    W8VerticalRangeThumb005ED6B4* m_thumb; /* 0x60 */
    W8RangeListener* m_listener;         /* 0x64 */
    unsigned char m_enabled;             /* 0x68 */
};

class W8RangeButton005ED6FC : public W8TextControl005ED604 {
public:
    W8RangeButton005ED6FC(Controls* panel, unsigned int region,
                          int left, int top, int right, int bottom,
                          int text_40, int text_44, int text_48, int text_4c,
                          int text_54, int text_50, int text_58,
                          short direction, W8RangeControl005ED74C* range);
    void HandlePress(int event);
    virtual void ActivatePrimary();
    void AdjustValue(int steps);

protected:
    short m_direction;                   /* 0xb8: zero decrements */
    unsigned short pad_ba;
    W8RangeControl005ED74C* m_range;     /* 0xbc */
};

__forceinline W8RangeButton005ED6FC::W8RangeButton005ED6FC(
    Controls* panel, unsigned int region,
    int left, int top, int right, int bottom,
    int text_40, int text_44, int text_48, int text_4c,
    int text_54, int text_50, int text_58,
    short direction, W8RangeControl005ED74C* range)
    : W8TextControl005ED604(panel, region, left, top, right, bottom,
                           text_40, text_44, text_48, text_4c,
                           text_54, text_50, text_58),
      m_direction(direction), m_range(range)
{
}

/* The range panel owns two text-derived step buttons and one vertical thumb.
   Ghidra's constructor packet proves the six stack arguments, all three
   allocation sizes, the ordered child construction, and the four EH cleanup
   states. */
// FUNCTION: WIZ8 0x004F61F0
W8RangeControl005ED74C::W8RangeControl005ED74C(
    int left, int top, int right, int bottom,
    unsigned int* shared_region_set)
    : Controls(W8RangeControlConstruction005ED74C(),
               left, top, right, bottom, -1, -1, -1),
      m_minimum(0), m_maximum(1), m_value(0),
      m_listener(0)
{
    int height = bottom - top;

    if (*shared_region_set == 0) {
        *shared_region_set = CreateRegionSet();
    }
    m_uiRegionSetId = *shared_region_set;
    ResetRegionSet(m_uiRegionSetId);

    m_decrement = new W8RangeButton005ED6FC(
        this, 0xffffffff, 0, 0, 0x10, 0x10,
        0x86, 0, 0, 2, 1, 2, 3, 0, this);
    m_increment = new W8RangeButton005ED6FC(
        this, 0xffffffff, 0, height - 0x10, 0x10, height,
        0x86, 0, 8, 10, 9, 10, 0xb, 1, this);
    m_thumb = new W8VerticalRangeThumb005ED6B4(
        this, 0, 0x10, 0x10, height - 0x10, 0x86, 4, 5, -1);
    m_enabled = 0;
}

/* Child deletion order follows the three null-tested scalar-deleting virtual
   calls in the retail body; the Controls destructor then releases the typed
   embedded vector. */
// FUNCTION: WIZ8 0x004F63C0
W8RangeControl005ED74C::~W8RangeControl005ED74C()
{
    delete m_decrement;
    delete m_increment;
    delete m_thumb;
}

// FUNCTION: WIZ8 0x004F6440
void W8RangeControl005ED74C::SetRange(int first, int second)
{
    if (first < second) {
        m_minimum = first;
        m_maximum = second;
    } else {
        m_maximum = first;
        m_minimum = second;
    }
    SetValue(m_value);
}

// FUNCTION: WIZ8 0x004F6470
void W8RangeControl005ED74C::SetValue(int value)
{
    m_value = value;
    if (value < m_minimum) {
        m_value = m_minimum;
    } else if (m_maximum < value) {
        m_value = m_maximum;
    }

    float position;
    if (m_value == m_minimum) {
        position = g_W8RangeStart005EBB34;
    } else if (m_value == m_maximum) {
        position = g_W8RangeEnd005EBB38;
    } else {
        position = ((float)(m_value - m_minimum) + g_W8RangeHalfStep005EBC7C) /
                   (float)((m_maximum - m_minimum) + 1);
    }
    m_thumb->SetRangePosition(position);
}

// FUNCTION: WIZ8 0x004F6540
void W8RangeControl005ED74C::Decrement()
{
    if (m_enabled != 0) {
        SetValue(m_value - 1);
        if (m_listener != 0) {
            m_listener->OnRangeChanged(this);
        }
    }
}

// FUNCTION: WIZ8 0x004F6570
void W8RangeControl005ED74C::Increment()
{
    if (m_enabled != 0) {
        SetValue(m_value + 1);
        if (m_listener != 0) {
            m_listener->OnRangeChanged(this);
        }
    }
}

// FUNCTION: WIZ8 0x004F65A0
void W8RangeControl005ED74C::SetEnabled(unsigned char enabled)
{
    m_enabled = enabled;
    m_decrement->SetVisible(enabled);
    m_increment->SetVisible(enabled & m_enabled);
    m_thumb->SetVisible(enabled);
}

__forceinline void W8VerticalRangeThumb005ED6B4::ClampPositionAndInvalidate()
{
    if (m_position < m_minimumPosition) {
        m_position = m_minimumPosition;
    }
    if (m_maximumPosition < m_position) {
        m_position = m_maximumPosition;
    }
    m_pixelPosition = (int)(((m_position - m_minimumPosition) /
                             (m_maximumPosition - m_minimumPosition)) *
                            m_trackLength);
    if (m_pPanel != 0) {
        m_flag_6 = 1;
        m_pPanel->m_fLayoutDirty = 1;
        Function562A50(0x80000000);
        Function562A50(0x80000000);
    }
}

__forceinline void W8VerticalRangeThumb005ED6B4::SynchronizeRangeValue()
{
    int value = (int)(m_range->m_thumb->m_position *
                      (float)(m_range->m_maximum - m_range->m_minimum + 1)) +
                m_range->m_minimum;
    if (m_range->m_maximum < value) {
        value = m_range->m_maximum;
    }
    if (value != m_range->m_value) {
        W8RangeListener* listener = m_range->m_listener;
        m_range->m_value = value;
        if (listener != 0) {
            listener->OnRangeChanged(m_range);
        }
    }
}

// FUNCTION: WIZ8 0x004F5B20
W8VerticalRangeThumb005ED6B4::W8VerticalRangeThumb005ED6B4(
    W8RangeControl005ED74C* range, int left, int top, int right, int bottom,
    int render_arg, int normal_sprite, int hovered_sprite, int disabled_sprite)
    : W8WidgetBase005ED5BC(range, 0xffffffff, left, top, 0, bottom)
{
    short width;
    short height;

    m_hoveredSprite = hovered_sprite;
    m_disabledSprite = disabled_sprite;
    m_renderArg = render_arg;
    m_renderArg38 = 0;
    m_normalSprite = normal_sprite;
    m_hovered = 0;
    m_dragging = 0;
    m_minimumPosition = 0.0f;
    m_maximumPosition = 1.0f;
    m_position = 0.0f;
    Function549660(render_arg, 0, normal_sprite, &width, &height);
    if (right - left < (unsigned short)width) {
        m_right = m_left + (unsigned short)width;
        m_drawOffsetX = 0;
    } else {
        m_right = right;
        m_drawOffsetX = right - (unsigned short)width - left;
    }
    SetRegion(m_region_18);
    m_thumbHeight = (unsigned short)height;
    m_trackLength = bottom - (unsigned short)height - top;
    m_range = range;
}

// FUNCTION: WIZ8 0x004F5C00
void W8VerticalRangeThumb005ED6B4::BeginDrag(int event)
{
    Function5587C0(0, 1);
    if (m_flag_4 != 0) {
        int cursor[2];
        Function4284F0(cursor);
        int y = cursor[1] - m_pPanel->origin_y - m_top;
        if (m_hovered == 0) {
            m_hovered = 1;
            m_position = ((float)(y - m_thumbHeight / 2) / (float)m_trackLength) *
                         (m_maximumPosition - m_minimumPosition) + m_minimumPosition;
            ClampPositionAndInvalidate();
            SynchronizeRangeValue();
        }
        m_dragCoordinate = y;
        m_dragging = 1;
        Function4F2040(m_region_18);
    }
}

// FUNCTION: WIZ8 0x004F5D30
void W8VerticalRangeThumb005ED6B4::EndDrag(int event)
{
    Function5587C0(0, 1);
    if (m_flag_4 != 0 && m_dragging != 0) {
        m_dragging = 0;
        ClearActiveRegionIfMatches(m_region_18);
    }
}

// FUNCTION: WIZ8 0x004F5D70
void W8VerticalRangeThumb005ED6B4::UpdateDrag(int event)
{
    if (m_flag_4 == 0) {
        return;
    }

    int cursor[2];
    Function4284F0(cursor);
    int y = cursor[1] - m_pPanel->origin_y - m_top;
    if (m_dragging != 0) {
        int half_height = m_thumbHeight / 2;
        if (y <= half_height) {
            m_position = 0.0f;
        } else if (m_trackLength + half_height <= y) {
            m_position = 1.0f;
        } else {
            int delta = y - m_dragCoordinate;
            m_dragCoordinate = y;
            m_position += ((float)delta / (float)m_trackLength) *
                          (m_maximumPosition - m_minimumPosition);
        }
        ClampPositionAndInvalidate();
        SynchronizeRangeValue();
        return;
    }

    unsigned char hovered =
        (m_pixelPosition <= y && y <= m_pixelPosition + m_thumbHeight);
    if (hovered != m_hovered && m_pPanel != 0) {
        m_flag_6 = 1;
        m_pPanel->m_fLayoutDirty = 1;
        Function562A50(0x80000000);
        Function562A50(0x80000000);
    }
    m_hovered = hovered;
}

// FUNCTION: WIZ8 0x004F5EF0
void W8VerticalRangeThumb005ED6B4::AdjustValue(int steps)
{
    if (steps > 0) {
        do {
            m_range->Decrement();
            --steps;
        } while (steps != 0);
    } else if (steps < 0) {
        steps = -steps;
        do {
            m_range->Increment();
            --steps;
        } while (steps != 0);
    }
}

// FUNCTION: WIZ8 0x004F5F60
void W8VerticalRangeThumb005ED6B4::Redraw(int full_redraw)
{
    if (m_flag_5 == 0 || (full_redraw == 0 && m_flag_6 == 0)) {
        return;
    }
    int left = m_pPanel->origin_x + m_left;
    int top = m_pPanel->origin_y + m_top;
    int right = m_pPanel->origin_x + m_right;
    int bottom = m_pPanel->origin_y + m_bottom;
    Function422D50(left, top, right, bottom, 0);
    FillSurfaceRect(-14, left, top, right, bottom, 0x8000);

    int sprite;
    if (m_flag_4 == 0) {
        sprite = m_disabledSprite;
        if (sprite == -1) {
            return;
        }
    } else if (m_hovered == 0 || (sprite = m_hoveredSprite) == -1) {
        sprite = m_normalSprite;
    }
    Function548F90(-14, m_renderArg, m_renderArg38, sprite,
                   m_pPanel->origin_x + m_drawOffsetX + m_left,
                   m_pPanel->origin_y + m_pixelPosition + m_top, 2, 0);
}

// FUNCTION: WIZ8 0x004F6050
void W8RangeButton005ED6FC::HandlePress(int event)
{
    Function4F70(event);
    if (m_flag_5 != 0 && m_flag_4 != 0) {
        if (m_direction == 0) {
            m_range->Decrement();
        } else {
            m_range->Increment();
        }
    }
}

// FUNCTION: WIZ8 0x004F60C0
void W8RangeButton005ED6FC::ActivatePrimary()
{
    W8TextControl005ED604::ActivatePrimary();
    if (m_flag_5 != 0 && m_flag_4 != 0) {
        if (m_direction == 0) {
            m_range->Decrement();
        } else {
            m_range->Increment();
        }
    }
}

// FUNCTION: WIZ8 0x004F6180
void W8RangeButton005ED6FC::AdjustValue(int steps)
{
    if (steps > 0) {
        do {
            m_range->Decrement();
            --steps;
        } while (steps != 0);
    } else if (steps < 0) {
        steps = -steps;
        do {
            m_range->Increment();
            --steps;
        } while (steps != 0);
    }
}

// FUNCTION: WIZ8 0x004F6680
void W8HelpTextControl005ED758::SetRegionHelp(const wchar_t* text)
{
    if (wcslen(text) < 200) {
        wcscpy(m_regionHelp, text);
    }
}

// FUNCTION: WIZ8 0x004F66B0
void W8HelpTextControl005ED758::UpdateRegionHelp(int event)
{
    Function5587C0(0, 1);
    Function4D30(event);
    if (wcslen(m_regionHelp) > 1 && m_region_18 != -1) {
        ::SetRegionHelpText(m_regionHelp);
        ::EnableRegionHelp(m_region_18);
        return;
    }
    if (m_region_18 != -1) {
        ::DisableRegionHelp(m_region_18);
    }
}

// FUNCTION: WIZ8 0x004F6720
void W8HelpTextControl005ED758::InvokeFocusCallback()
{
    if (m_secondaryCallback == 0) {
        Function5587C0(0, 1);
    }
    W8TextControl005ED604::InvokeFocusCallback();
}

// FUNCTION: WIZ8 0x004F6780
void W8HelpTextControl005ED758::ActivateSecondary()
{
    if (m_secondaryCallback == 0) {
        Function5587C0(0, 1);
    }
    if (m_flag_5 != 0 && m_flag_4 != 0) {
        if ((m_flags_38 & 0x20) != 0) {
            Function5587C0(0, 1);
        }
        if ((m_flags_38 & 0x100) != 0 && (m_stateFlags & 4) != 0) {
            m_stateFlags &= ~4u;
            Function5587C0(0, 1);
            return;
        }
        if (m_listener != 0) {
            m_listener->OnSecondary(this);
        }
        if (m_secondaryCallback != 0) {
            m_secondaryCallback();
        }
        return;
    }
    if (m_flag_5 == 0 && m_flag_4 != 0) {
        return;
    }
    Function5587C0(0, 1);
}

// FUNCTION: WIZ8 0x004F6810
void W8HelpTextControl005ED758::InvokeBlurCallback()
{
    Function5587C0(0, 1);
    W8TextControl005ED604::InvokeBlurCallback();
}

// FUNCTION: WIZ8 0x004F6030
W8TextControl005ED604*
W8TextControl005ED604::scalar_deleting_destructor(unsigned char flags)
{
    this->~W8TextControl005ED604();
    if ((flags & 1) != 0) {
        ::operator delete(this);
    }
    return this;
}

/* The vtable at 0x005ED66C is the horizontal draggable range thumb. Its
   constructor measures the normal sprite for the widget bounds, then measures
   the movable thumb sprite and retains the remaining horizontal travel at
   +0x4c. The interaction methods independently prove that geometry: cursor X
   is converted through +0x4c into the normalized float range +0x60..+0x68. */
class W8HorizontalRangeThumb005ED66C;

class W8HorizontalRangeThumbListener {
public:
    virtual void OnDrag(W8HorizontalRangeThumb005ED66C* thumb) = 0;
    virtual void OnDragEnd(W8HorizontalRangeThumb005ED66C* thumb) = 0;
};

class W8HorizontalRangeThumb005ED66C : public W8WidgetBase005ED5BC {
public:
    virtual ~W8HorizontalRangeThumb005ED66C();
    W8HorizontalRangeThumb005ED66C* scalar_deleting_destructor(unsigned char flags);
    W8HorizontalRangeThumb005ED66C(Controls* panel, unsigned int region, int left, int top,
                                   int render_arg_0, int render_arg_1, int background_sprite,
                                   int normal_thumb_sprite, int hovered_thumb_sprite,
                                   int disabled_thumb_sprite);
    virtual void Redraw(int full_redraw);
    void UpdatePixelPosition();
    void BeginDrag(int event);
    void EndDrag(int event);
    void ClearHover(unsigned char immediate);
    void UpdateDrag(int event);

protected:
    int m_renderArg0;                    /* 0x34 */
    int m_renderArg1;                    /* 0x38 */
    int m_backgroundSprite;              /* 0x3c */
    int m_normalThumbSprite;             /* 0x40 */
    int m_hoveredThumbSprite;            /* 0x44 */
    int m_disabledThumbSprite;           /* 0x48 */
    int m_trackLength;                   /* 0x4c: horizontal travel */
    int m_thumbWidth;                    /* 0x50 */
    int m_pixelPosition;                 /* 0x54 */
    int m_dragCoordinate;                /* 0x58 */
    unsigned char m_hovered;             /* 0x5c */
    unsigned char m_dragging;            /* 0x5d */
    unsigned char pad_5e[2];
    float m_minimumPosition;             /* 0x60 */
    float m_maximumPosition;             /* 0x64 */
    float m_position;                    /* 0x68 */
    W8HorizontalRangeThumbListener* m_listener; /* 0x6c */

    __forceinline void InvalidateThumb()
    {
        if (m_pPanel != 0) {
            m_flag_6 = 1;
            m_pPanel->m_fLayoutDirty = 1;
            Function562A50(0x80000000);
            Function562A50(0x80000000);
        }
    }

    __forceinline void ClampPositionAndInvalidate()
    {
        if (m_position < m_minimumPosition) {
            m_position = m_minimumPosition;
        }
        if (m_maximumPosition < m_position) {
            m_position = m_maximumPosition;
        }
        m_pixelPosition = (int)(((m_position - m_minimumPosition) /
                                 (m_maximumPosition - m_minimumPosition)) *
                                m_trackLength);
        InvalidateThumb();
    }
};

// FUNCTION: WIZ8 0x004F5620
W8HorizontalRangeThumb005ED66C::W8HorizontalRangeThumb005ED66C(
    Controls* panel, unsigned int region, int left, int top,
    int render_arg_0, int render_arg_1, int background_sprite,
    int normal_thumb_sprite, int hovered_thumb_sprite, int disabled_thumb_sprite)
    : W8WidgetBase005ED5BC(panel, region, left, top, 0, 0)
{
    short width;
    short height;

    m_normalThumbSprite = normal_thumb_sprite;
    m_hoveredThumbSprite = hovered_thumb_sprite;
    m_disabledThumbSprite = disabled_thumb_sprite;
    m_renderArg0 = render_arg_0;
    m_renderArg1 = render_arg_1;
    m_backgroundSprite = background_sprite;
    m_pixelPosition = 0;
    m_hovered = 0;
    m_dragging = 0;
    m_minimumPosition = 0.0f;
    m_maximumPosition = 1.0f;
    m_position = 0.0f;
    m_listener = 0;
    Function549660(render_arg_0, render_arg_1, background_sprite, &width, &height);
    m_right = (unsigned short)width + m_left;
    m_bottom = m_top + (unsigned short)height;
    SetRegion(m_region_18);
    Function549660(m_renderArg0, m_renderArg1, m_normalThumbSprite, &width, &height);
    m_thumbWidth = (unsigned short)width;
    m_trackLength = (m_right - m_left) - (unsigned short)width;
}

// FUNCTION: WIZ8 0x004F5710
void W8HorizontalRangeThumb005ED66C::UpdatePixelPosition()
{
    ClampPositionAndInvalidate();
}

// FUNCTION: WIZ8 0x004F5780
void W8HorizontalRangeThumb005ED66C::BeginDrag(int event)
{
    Function5587C0(0, 1);
    if (m_flag_4 != 0) {
        int cursor[2];
        Function4284F0(cursor);
        int x = cursor[0] - m_pPanel->origin_x - m_left;
        if (m_hovered == 0) {
            m_hovered = 1;
            m_position = ((float)(x - m_thumbWidth / 2) / (float)m_trackLength) *
                         (m_maximumPosition - m_minimumPosition) + m_minimumPosition;
            ClampPositionAndInvalidate();
            if (m_listener != 0) {
                m_listener->OnDrag(this);
            }
        }
        m_dragCoordinate = x;
        m_dragging = 1;
        Function4F2040(m_region_18);
    }
}

// FUNCTION: WIZ8 0x004F5880
void W8HorizontalRangeThumb005ED66C::EndDrag(int event)
{
    Function5587C0(0, 1);
    if (m_flag_4 != 0 && m_dragging != 0) {
        m_dragging = 0;
        ClearActiveRegionIfMatches(m_region_18);
        if (m_listener != 0) {
            m_listener->OnDragEnd(this);
        }
    }
}

// FUNCTION: WIZ8 0x004F58D0
void W8HorizontalRangeThumb005ED66C::ClearHover(unsigned char immediate)
{
    Function5587C0(0, 1);
    if (m_hovered != 0 && m_dragging == 0) {
        m_hovered = 0;
        if (m_pPanel != 0) {
            m_flag_6 = 1;
            if (immediate != 0) {
                m_pPanel->Invalidate(0);
                Function562A50(0x80000000);
                return;
            }
            m_pPanel->m_fLayoutDirty = 1;
            Function562A50(0x80000000);
            Function562A50(0x80000000);
        }
    }
}

// FUNCTION: WIZ8 0x004F5940
void W8HorizontalRangeThumb005ED66C::UpdateDrag(int event)
{
    if (m_flag_4 == 0) {
        return;
    }

    int cursor[2];
    Function4284F0(cursor);
    int x = cursor[0] - m_pPanel->origin_x - m_left;
    if (m_dragging != 0) {
        if (x < 0 || m_trackLength + m_thumbWidth / 2 < x) {
            return;
        }
        int delta = x - m_dragCoordinate;
        m_dragCoordinate = x;
        m_position += ((float)delta / (float)m_trackLength) *
                      (m_maximumPosition - m_minimumPosition);
        ClampPositionAndInvalidate();
        if (m_listener != 0) {
            m_listener->OnDrag(this);
        }
        return;
    }

    unsigned char hovered =
        (m_pixelPosition <= x && x <= m_pixelPosition + m_thumbWidth);
    if (m_hovered != hovered && m_pPanel != 0) {
        InvalidateThumb();
    }
    m_hovered = hovered;
}

// FUNCTION: WIZ8 0x004F5A80
void W8HorizontalRangeThumb005ED66C::Redraw(int full_redraw)
{
    if (m_flag_5 == 0 || ((unsigned char)full_redraw == 0 && m_flag_6 == 0)) {
        return;
    }

    int x = m_pPanel->origin_x + m_left;
    int y = m_pPanel->origin_y + m_top;
    Function549600(-14, m_renderArg0, m_renderArg1, m_backgroundSprite,
                   x, y, 2, 0);

    int sprite;
    if (m_flag_4 == 0 && m_disabledThumbSprite != -1) {
        sprite = m_disabledThumbSprite;
    } else if (m_hovered != 0 && m_hoveredThumbSprite != -1) {
        sprite = m_hoveredThumbSprite;
    } else {
        sprite = m_normalThumbSprite;
    }
    Function548F90(-14, m_renderArg0, m_renderArg1, sprite,
                   x + m_pixelPosition, y, 2, 0);
}

// FUNCTION: WIZ8 0x004F69B0
W8HorizontalRangeThumb005ED66C*
W8HorizontalRangeThumb005ED66C::scalar_deleting_destructor(unsigned char flags)
{
    this->~W8HorizontalRangeThumb005ED66C();
    if ((flags & 1) != 0) {
        ::operator delete(this);
    }
    return this;
}

// FUNCTION: WIZ8 0x004F69D0
W8HorizontalRangeThumb005ED66C::~W8HorizontalRangeThumb005ED66C()
{
}

// FUNCTION: WIZ8 0x004F6640
W8TextControl005ED604::~W8TextControl005ED604()
{
}

/* Enables or disables the whole panel: the panel's own flag, then every child's,
   and each child's region follows - mode 4 restores the disabled region and
   clearing the mode bits re-arms it. */
// FUNCTION: WIZ8 0x004F2D50
void Controls::SetEnabled(unsigned char enable)
{
    int index;

    m_fEnabled = enable;
    for (index = 0; index < m_controls.count; ++index) {
        W8WidgetBase005ED5BC* control = ControlAt(index);

        control->m_flag_5 = enable;
        if (control->m_region_18 != -1) {
            if (enable == 0) {
                SetRegionMode4(control->m_region_18);
            } else {
                ClearRegionModeBits(control->m_region_18);
            }
        }
    }
}

/* Detaches one control by identity. The search stops at the first match and the
   tail is shifted down over it; a control that is not present leaves the array
   untouched. The array itself never shrinks. */
// FUNCTION: WIZ8 0x004F2DA0
void Controls::RemoveControl(W8WidgetBase005ED5BC* control)
{
    int count = m_controls.count;
    int index = 0;

    if (count > 0) {
        W8WidgetBase005ED5BC** cursor = m_controls.data;

        while (*cursor != control) {
            ++index;
            ++cursor;
            if (index >= count) {
                return;
            }
        }
        if (index >= 0 && index < count) {
            if (index < count - 1) {
                do {
                    m_controls.data[index] = m_controls.data[index + 1];
                    ++index;
                } while (index < m_controls.count - 1);
            }
            --m_controls.count;
        }
    }
}

/* Tears the panel down back to front. Each control is detached first and
   deleted afterwards - `delete` on the polymorphic base, which is the vtable
   slot 0 call with the deleting flag the image makes - so a control's destructor
   can safely walk the array it has already left.
   The shift-down is the same one RemoveControl performs, inlined here because
   the index is already known. */
// FUNCTION: WIZ8 0x004F2DF0
void Controls::DestroyAllControls()
{
    int index = m_controls.count;

    if (index > 0) {
        while (--index, index >= 0) {
            if (index < m_controls.count && index >= 0) {
                W8WidgetBase005ED5BC* control = m_controls.data[index];
                int shift = index;

                if (index < m_controls.count - 1) {
                    do {
                        m_controls.data[shift] = m_controls.data[shift + 1];
                        ++shift;
                    } while (shift < m_controls.count - 1);
                }
                --m_controls.count;
                delete control;
            }
        }
    }
}

/* Adds a rectangle to the panel's pending redraw. A null rectangle means the
   whole panel, and the first rectangle after a flush - recognised by a left
   edge of -1 - is copied rather than unioned. */
// FUNCTION: WIZ8 0x004F2E50
void Controls::Invalidate(const W8ControlsRect* rect)
{
    int edge;

    m_fDirty = 1;
    if (rect == 0) {
        m_fWholeAreaDirty = 1;
        Function562A50(0x80000000);
        return;
    }
    edge = m_dirtyRect.left;
    m_fWholeAreaDirty = 0;
    if (edge == -1) {
        m_dirtyRect.left = rect->left;
        m_dirtyRect.top = rect->top;
        m_dirtyRect.right = rect->right;
        m_dirtyRect.bottom = rect->bottom;
        Function562A50(0x80000000);
        return;
    }
    if (rect->left <= edge) {
        edge = rect->left;
    }
    m_dirtyRect.left = edge;
    edge = m_dirtyRect.top;
    if (rect->top <= m_dirtyRect.top) {
        edge = rect->top;
    }
    m_dirtyRect.top = edge;
    edge = m_dirtyRect.right;
    if (m_dirtyRect.right <= rect->right) {
        edge = rect->right;
    }
    m_dirtyRect.right = edge;
    edge = rect->bottom;
    if (rect->bottom < m_dirtyRect.bottom) {
        edge = m_dirtyRect.bottom;
    }
    m_dirtyRect.bottom = edge;
    Function562A50(0x80000000);
}

/* Marks the panel's layout stale without touching the redraw rectangle. */
// FUNCTION: WIZ8 0x004F2F00
void Controls::InvalidateLayout()
{
    m_fLayoutDirty = 1;
    Function562A50(0x80000000);
}

/* Flushes pending panel drawing, then asks each enabled child to redraw. A
   full panel request uses the target-backed path when one exists; a bounded
   request uses the accumulated rectangle. */
// FUNCTION: WIZ8 0x004F2F10
void Controls::Redraw()
{
    int redrawn = 0;
    int index;

    if (m_fEnabled == 0) {
        return;
    }
    if (m_fDirty != 0) {
        if (m_renderTarget != -1) {
            Function548F90(-14, m_renderTarget, m_renderArg_1c, m_renderArg_20,
                           origin_x, origin_y, 2, 0);
        }
        if (m_fWholeAreaDirty != 0) {
            if (m_renderTarget != -1) {
                Function5494F0(m_renderTarget, m_renderArg_1c, m_renderArg_20,
                               origin_x, origin_y, 2);
            }
        } else {
            Function422D50(m_dirtyRect.left, m_dirtyRect.top,
                           m_dirtyRect.right, m_dirtyRect.bottom, 2);
        }
        m_fDirty = 0;
        m_dirtyRect.left = -1;
        redrawn = 1;
    } else if (m_fLayoutDirty == 0) {
        return;
    }
    for (index = 0; index < m_controls.count; ++index) {
        if (ControlAt(index)->m_flag_5 != 0) {
            ControlAt(index)->Redraw(redrawn);
        }
    }
    m_fLayoutDirty = 0;
}

/* Replaces the panel bounds and forwards each child's existing relative
   rectangle through virtual slot three so derived widgets can respond. */
// FUNCTION: WIZ8 0x004F3010
void Controls::SetBounds(int left, int top, int new_right, int new_bottom)
{
    int index;

    origin_x = left;
    origin_y = top;
    right = new_right;
    bottom = new_bottom;
    for (index = 0; index < m_controls.count; ++index) {
        ControlAt(index)->SetBounds(ControlAt(index)->m_left,
                                    ControlAt(index)->m_top,
                                    ControlAt(index)->m_right,
                                    ControlAt(index)->m_bottom);
    }
}

/* Takes the panel's region set from a shared slot, creating it on first use,
   and empties it so this panel can repopulate it. */
// FUNCTION: WIZ8 0x004F30C0
void Controls::AcquireRegionSet(unsigned int* shared_region_set)
{
    if (*shared_region_set == 0) {
        *shared_region_set = CreateRegionSet();
    }
    m_uiRegionSetId = *shared_region_set;
    ResetRegionSet(m_uiRegionSetId);
}

/* Enables timed help for this widget's region when it owns one. */
// FUNCTION: WIZ8 0x004F4120
void W8WidgetBase005ED5BC::EnableRegionHelp(int help_text_id)
{
    if (m_region_18 != -1) {
        SetRegionHelp(m_region_18, 1, help_text_id);
    }
}

/* Disables timed help and clears the text id for this widget's region. */
// FUNCTION: WIZ8 0x004F4140
void W8WidgetBase005ED5BC::DisableRegionHelp()
{
    if (m_region_18 != -1) {
        SetRegionHelp(m_region_18, 0, -1);
    }
}

/* Marks the widget dirty and either asks its panel to invalidate immediately
   or raises the panel's deferred-layout flag. */
// FUNCTION: WIZ8 0x004F40A0
void W8WidgetBase005ED5BC::Invalidate(unsigned char immediate)
{
    if (m_pPanel != 0) {
        m_flag_6 = 1;
        if (immediate) {
            m_pPanel->Invalidate(0);
            Function562A50(0x80000000);
            return;
        }
        m_pPanel->m_fLayoutDirty = 1;
        Function562A50(0x80000000);
        Function562A50(0x80000000);
    }
}

// FUNCTION: WIZ8 0x004F40F0
void W8WidgetBase005ED5BC::SetEnabled(unsigned char enabled)
{
    m_flag_5 = enabled;
    if (m_region_18 != -1) {
        if (enabled) {
            ClearRegionModeBits(m_region_18);
            return;
        }
        SetRegionMode4(m_region_18);
    }
}

// FUNCTION: WIZ8 0x004F6950
void W8WidgetBase005ED5BC::SetVisible(unsigned char visible)
{
    m_flag_4 = visible;
}

// FUNCTION: WIZ8 0x004F6980
void W8WidgetBase005ED5BC::SetBounds(int left, int top, int right, int bottom)
{
    m_left = left;
    m_top = top;
    m_right = right;
    m_bottom = bottom;
}

// FUNCTION: WIZ8 0x004F6960
void W8WidgetBase005ED5BC::SetBoundsFromRect(const W8ControlsRect* bounds)
{
    m_left = bounds->left;
    m_top = bounds->top;
    m_right = bounds->right;
    m_bottom = bounds->bottom;
}
