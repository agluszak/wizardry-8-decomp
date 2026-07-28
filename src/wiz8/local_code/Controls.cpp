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
    virtual void UnknownSlot0() = 0;
    virtual void Invalidate(unsigned char immediate);
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
    unsigned char unknown_35[7];
    int m_nControls;                        /* 0x3c */
    int m_nControlsAllocated;               /* 0x40 */
    W8WidgetBase005ED5BC** m_ppControls;    /* 0x44 */
    unsigned int m_uiRegionSetId;           /* 0x48 */

    void EnableRegionSet(unsigned char enable);
    void SetEnabled(unsigned char enable);
    void RemoveControl(W8WidgetBase005ED5BC* control);
    void DestroyAllControls();
    void Invalidate(const W8ControlsRect* rect);
    void InvalidateLayout();
    void Redraw();
    void SetBounds(int left, int top, int right, int bottom);
    void AcquireRegionSet(unsigned int* shared_region_set);

    /* The bounds-checked element read every walker above shares. Out of range
       it answers element zero rather than failing, which is what the canonical
       `p = m_ppControls; if (i < m_nControls) p += i;` compiles from and why
       the guard shows up once per use rather than once per loop. */
    __inline W8WidgetBase005ED5BC* ControlAt(int index)
    {
        if (index < m_nControls) {
            return m_ppControls[index];
        }
        return m_ppControls[0];
    }
};

/* 0x00562A50 takes the redraw-request mask the panel raises. */
extern void Function562A50(unsigned int mask);
extern void Function422D50(int left, int top, int right, int bottom, int mode);
extern void Function548F90(int operation, int target, int arg_1c, int arg_20,
                           int left, int top, int mode, int flags);
extern void Function5494F0(int target, int arg_1c, int arg_20,
                           int left, int top, int mode);
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
    void GetTextOrigin(int unused, int* px, int* py);
    void Invalidate(unsigned char immediate);
    virtual void SetVisible(unsigned char visible);
    void SetFlaggedRegionBounds(short left, short top, unsigned short right);
    virtual void AddLayoutFlags(unsigned int flags);
    virtual void SetAlternateTextEnabled(unsigned char enabled);
    void RemoveLayoutFlags(unsigned int flags);
    void EnableSecondaryState(unsigned char immediate);
    void DisableSecondaryState(unsigned char immediate);
    void Function4D30(int event);
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
    unsigned char unknown_50[8];
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
                m_pPanel->Invalidate((unsigned char)0);
            } else {
                m_pPanel->m_fLayoutDirty = 1;
                Function562A50(0x80000000);
            }
            Function562A50(0x80000000);
        }
        m_textBuffer.SetGeometryDirty();
    }
};

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
    void SetRegionHelp(const wchar_t* text);
    void UpdateRegionHelp(int event);
    virtual void InvokeFocusCallback();
    virtual void ActivateSecondary();
    virtual void InvokeBlurCallback();

protected:
    wchar_t m_regionHelp[200];            /* 0xb8 */
};

class W8RangeThumb : public W8WidgetBase005ED5BC {
public:
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
    unsigned char unknown_34[0x18];
    int m_pixelPosition;                 /* 0x4c */
    int m_field_50;
    int m_trackLength;                   /* 0x54 */
    int m_field_58;
    int m_field_5c;
    float m_minimumPosition;             /* 0x60 */
    float m_maximumPosition;             /* 0x64 */
    float m_position;                    /* 0x68 */
};

class W8RangeControl005ED74C;
class W8RangeListener {
public:
    virtual void OnRangeChanged(W8RangeControl005ED74C* control) = 0;
};

class W8RangeControl005ED74C {
public:
    void SetRange(int first, int second);
    void SetValue(int value);
    void Decrement();
    void Increment();
    void SetEnabled(unsigned char enabled);

protected:
    unsigned char unknown_00[0x4c];
    int m_minimum;                       /* 0x4c */
    int m_maximum;                       /* 0x50 */
    int m_value;                         /* 0x54 */
    W8WidgetBase005ED5BC* m_decrement;   /* 0x58 */
    W8WidgetBase005ED5BC* m_increment;   /* 0x5c */
    W8RangeThumb* m_thumb;               /* 0x60 */
    W8RangeListener* m_listener;         /* 0x64 */
    unsigned char m_enabled;             /* 0x68 */
};

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
    W8Control005ED66C(Controls* panel, unsigned int region, int left, int top,
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
W8Control005ED66C::W8Control005ED66C(Controls* panel, unsigned int region, int left, int top,
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

/* The teardown is the implicit one: it restores the class's own table, then
   runs the vector's destructor, which restores the template table and releases
   the array with no null test - the template's own destructor, not a private
   copy of it. Nothing else is destroyed, which is what fixes the class as
   owning exactly the one vector. */

class W8Control005ED5A4 {
public:
    W8Control005ED5A4();
    W8Control005ED5A4(int a2, int a3, int a4, int a5, int a6, int a7, int a8);

    virtual ~W8Control005ED5A4();

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

__forceinline W8ElementVector005ED5B0::~W8ElementVector005ED5B0()
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

// FUNCTION: WIZ8 0x004F2D30
W8Control005ED5A4::~W8Control005ED5A4()
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
    for (index = 0; index < m_nControls; ++index) {
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
    int count = m_nControls;
    int index = 0;

    if (count > 0) {
        W8WidgetBase005ED5BC** cursor = m_ppControls;

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
                    m_ppControls[index] = m_ppControls[index + 1];
                    ++index;
                } while (index < m_nControls - 1);
            }
            --m_nControls;
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
    int index = m_nControls;

    if (index > 0) {
        while (--index, index >= 0) {
            if (index < m_nControls && index >= 0) {
                W8WidgetBase005ED5BC* control = m_ppControls[index];
                int shift = index;

                if (index < m_nControls - 1) {
                    do {
                        m_ppControls[shift] = m_ppControls[shift + 1];
                        ++shift;
                    } while (shift < m_nControls - 1);
                }
                --m_nControls;
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
    for (index = 0; index < m_nControls; ++index) {
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
    for (index = 0; index < m_nControls; ++index) {
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
            m_pPanel->Invalidate((unsigned char)0);
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
