#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_code/ControlsRect.h"
#include "wiz8/local_code/TextBuffer.h"
#include "wiz8/local_code/Widget.h"
#include "wiz8/local_code/TextControl.h"
#include "wiz8/local_code/RangeControl.h"
#include "wiz8/local_code/ControlSelection.h"
#include "wiz8/float_constants.h"
#include "wiz8/fonts.h"
#include "wiz8/cursor.h"
#include "wiz8/local_code/ButtonSound.h"
#include "wiz8/local_code/Controls.h"
#include "wiz8/regions.h"
#include "wiz8/utility.h"
#include "wiz8/sr_api.h"
#include "wiz8/vector.h"
#include "wiz8/video_object_catalog.h"
#include "Font.h"
#include "input.h"
#include "vsurface.h"

#include <wchar.h>

/* Original Local Code\Controls.cpp translation unit. Keep these implementations
   together and preserve their order. Retail assertions identify m_uiRegionSetId
   (line 399) and the widget's m_pPanel (line 1849); other role names are recovered. */
#define REGSET_NULL 0

// GLOBAL: WIZ8 0x00617C90
extern const wchar_t g_W8LineBreakCharacters00617C90[] = L"\n";

// GLOBAL: WIZ8 0x005ED548
extern const unsigned int g_W8TextBufferLayoutMask005ED548 = 0x01;
// GLOBAL: WIZ8 0x005ED54C
extern const unsigned int g_W8TextBufferLayoutMask005ED54C = 0x02;
// GLOBAL: WIZ8 0x005ED550
extern const unsigned int g_W8TextBufferLayoutMask005ED550 = 0x04;
// GLOBAL: WIZ8 0x005ED554
extern const unsigned int g_W8TextBufferLayoutMask005ED554 = 0x08;
// GLOBAL: WIZ8 0x005ED558
extern const unsigned int g_W8TextBufferLayoutMask005ED558 = 0x10;
// GLOBAL: WIZ8 0x005ED55C
extern const unsigned int g_W8TextBufferLayoutMask005ED55C = 0x20;
// GLOBAL: WIZ8 0x005ED560
extern const unsigned int g_W8TextBufferLayoutMask005ED560 = 0x40;
// GLOBAL: WIZ8 0x005ED56C
extern const unsigned int g_W8TextControlMask005ED56C = 0x01;
// GLOBAL: WIZ8 0x005ED570
extern const unsigned int g_W8TextControlMask005ED570 = 0x02;
// GLOBAL: WIZ8 0x005ED578
extern const unsigned int g_W8TextControlMask005ED578 = 0x01;
// GLOBAL: WIZ8 0x005ED588
extern const unsigned int g_W8TextControlMask005ED588 = 0x10;
// GLOBAL: WIZ8 0x005ed594
extern const unsigned int g_W8TextControlMask005ED594 = 0x80;

// SYNTHETIC: WIZ8 0x004f68a0
// W8GrowableVector<W8Widget*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x004f68c0
// W8GrowableVector<W8Widget*>::~W8GrowableVector<W8Widget*>

/* The default constructor. Everything the seven-argument one takes from its
   caller, this one zeroes or sets to -1. */
// FUNCTION: WIZ8 0x004f2c30
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

// FUNCTION: WIZ8 0x004f2ca0
Controls::Controls(int left, int top, int right_bound, int bottom_bound, int render_target,
                   int render_arg_1c, int render_arg_20)
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

// FUNCTION: WIZ8 0x004f2d30
__forceinline Controls::~Controls() {}

/* 0x00562A50 takes the redraw-request mask the panel raises. */
const wchar_t g_W8TextSeparator0060CC74[] = L" ";
const wchar_t g_W8TextBreakCharacters00617C88[] = L" \n";

// GLOBAL: WIZ8 0x005ff5f4
int g_W8TextClipTarget005FF5F4 = -15;

// GLOBAL: WIZ8 0x00650e38
int g_W8TextClipFlags00650E38;

// GLOBAL: WIZ8 0x005ebb38
float g_float_005ebb38 = 1.0f;

// GLOBAL: WIZ8 0x005ebc7c
float g_float_005ebc7c = 0.5f;

// FUNCTION: WIZ8 0x004f30f0
void Controls::EnableRegionSet(bool enable)
{
    if (m_uiRegionSetId == REGSET_NULL) {
        srAssertFail("m_uiRegionSetId != REGSET_NULL",
                     "C:\\Projects\\Wizardry 8\\Local Code\\Controls.cpp", 0x18f, 0);
    }
    if (enable) {
        RegionSetEnable(m_uiRegionSetId);
    } else {
        RegionSetDisable(m_uiRegionSetId);
    }
}

// SYNTHETIC: WIZ8 0x004f3d90
// W8Widget::`scalar deleting destructor'
// FUNCTION: WIZ8 0x004f3f10
W8Widget::~W8Widget()
{
    m_active = 0;
    if (m_region != -1) {
        DisableRegionInput(m_region);
    }
}

/* Move a widget between panels, preserving its existing region when it has
   one and allocating a region from the new panel otherwise. */
// FUNCTION: WIZ8 0x004f3f30
void W8Widget::SetPanel(Controls* panel)
{
    int index;

    if (m_pPanel != 0) {
        W8Widget** cursor = m_pPanel->m_controls.data;
        for (index = 0; index < m_pPanel->m_controls.count; ++index) {
            if (*cursor == this) {
                m_pPanel->m_controls.RemoveAt(index);
                break;
            }
            ++cursor;
        }
    }

    m_pPanel = panel;
    index = panel->m_controls.Add(this);
    if (panel->m_uiRegionSetId != 0 && m_region == -1) {
        unsigned int region = AddRegionToSet(panel->m_uiRegionSetId);
        SetRegion(region);
        SetRegionCallback(region, DispatchControlRegionEvent, static_cast<unsigned short>(index));
        SetRegionOwner(region, panel);
    }
    if (m_region != -1) {
        SetRegionBounds(m_region,
                        static_cast<unsigned short>(static_cast<short>(m_left) +
                                                    static_cast<short>(m_pPanel->origin_x)),
                        static_cast<unsigned short>(static_cast<short>(m_top) +
                                                    static_cast<short>(m_pPanel->origin_y)),
                        static_cast<unsigned short>(static_cast<short>(m_right) +
                                                    static_cast<short>(m_pPanel->origin_x)),
                        static_cast<unsigned short>(static_cast<short>(m_bottom) +
                                                    static_cast<short>(m_pPanel->origin_y)));
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
// FUNCTION: WIZ8 0x004f3dd0
W8Widget::W8Widget(Controls* owner, unsigned int region, int left, int top, int right, int bottom)
{
    Controls* holder;
    unsigned int taken;
    int index;
    int origin_x;
    int origin_y;

    m_right = right;
    m_pPanel = owner;
    m_enabled = 1;
    m_active = 0;
    m_dirty = 0;
    m_region = region;
    m_primaryActivationCallback = 0;
    m_leftButtonDownCallback = 0;
    m_secondaryActivationCallback = 0;
    m_rightButtonDownCallback = 0;
    m_leftDoubleClickCallback = 0;
    m_left = left;
    m_top = top;
    m_bottom = bottom;
    if (region != 0xffffffff) {
        origin_y = owner->origin_y;
        origin_x = owner->origin_x;
        SetRegionBounds(region, (unsigned short)((short)origin_x + (short)left),
                        (unsigned short)((short)origin_y + (short)top),
                        (unsigned short)((short)right + (short)origin_x),
                        (unsigned short)((short)bottom + (short)origin_y));
        DisableRegionInput(m_region);
    }

    holder = m_pPanel;
    index = holder->m_controls.Add(this);
    if (holder->m_uiRegionSetId != 0 && m_region == -1) {
        taken = AddRegionToSet(holder->m_uiRegionSetId);
        SetRegion(taken);
        SetRegionCallback(taken, DispatchControlRegionEvent, (unsigned short)index);
        SetRegionOwner(taken, holder);
    }
}

/* The shared widget-region callback registered above. It fetches the widget
   from the owner's array - an out-of-range index saturates to the first
   widget - and dispatches the region event through eleven widget virtual
   slots, answering whether the event was consumed. Button press and release
   arms and disarms the 0x40/0x80 region latches their repeat and release
   paths test. */
// FUNCTION: WIZ8 0x004F3140
unsigned char DispatchControlRegionEvent(const W8RegionEvent* event, W8Region* region)
{
    Controls* owner = (Controls*)region->owner;
    W8Widget* widget;
    unsigned short index = region->callback_id;
    unsigned short reason = event->reason;

    if (index < owner->m_controls.count) {
        widget = owner->m_controls.data[index];
    } else {
        widget = owner->m_controls.data[0];
    }
    if (widget == 0) {
        srAssertFail("pControl", "C:\\Projects\\Wizardry 8\\Local Code\\Controls.cpp", 0x1AA, 0);
    }
    switch (reason) {
    case LEFT_BUTTON_DOWN:
        widget->OnLeftButtonDown(0);
        region->flags |= W8_REGION_LEFT_BUTTON_HELD;
        return 1;
    case LEFT_BUTTON_UP:
        if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) == 0) {
            return 1;
        }
        widget->OnLeftButtonUp(0);
        region->flags &= ~W8_REGION_LEFT_BUTTON_HELD;
        return 1;
    case LEFT_BUTTON_DBL_CLK:
        widget->OnLeftButtonDoubleClick(0);
        return 1;
    case LEFT_BUTTON_REPEAT:
        if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) == 0) {
            return 1;
        }
        widget->ActivatePrimary(0);
        return 1;
    case RIGHT_BUTTON_DOWN:
        widget->OnRightButtonDown(0);
        region->flags |= W8_REGION_RIGHT_BUTTON_HELD;
        return 1;
    case RIGHT_BUTTON_UP:
        if ((region->flags & W8_REGION_RIGHT_BUTTON_HELD) == 0) {
            return 1;
        }
        widget->OnRightButtonUp(0);
        region->flags &= ~W8_REGION_RIGHT_BUTTON_HELD;
        return 1;
    case RIGHT_BUTTON_REPEAT:
        if ((region->flags & W8_REGION_RIGHT_BUTTON_HELD) == 0) {
            return 1;
        }
        widget->ActivateSecondary(0);
        return 1;
    case MOUSE_POS:
        if ((region->flags & W8_REGION_MOUSE_LEAVE) != 0) {
            widget->OnMouseLeave(0);
        } else if ((region->flags & W8_REGION_MOUSE_ENTER) != 0) {
            widget->OnMouseEnter(0);
        } else {
            widget->OnMouseMove(region->flags & W8_REGION_LEFT_BUTTON_HELD);
        }
        return 1;
    case MOUSE_WHEEL:
        widget->AdjustValue(
            GetMouseWheelDeltaValue(((const W8RegionMouseEvent*)event)->mouse_position));
        return 1;
    }
    return 0;
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
// FUNCTION: WIZ8 0x004f4020
void W8Widget::SetRegion(unsigned int region)
{
    Controls* holder;
    unsigned int bound;
    int origin_x;
    int origin_y;

    m_region = region;
    if (region != 0xffffffff) {
        holder = m_pPanel;
        if (holder != 0) {
            origin_y = holder->origin_y;
            origin_x = holder->origin_x;
            SetRegionBounds(region, (unsigned short)((short)m_left + (short)origin_x),
                            (unsigned short)((short)m_top + (short)origin_y),
                            (unsigned short)((short)m_right + (short)origin_x),
                            (unsigned short)((short)m_bottom + (short)origin_y));
        }
    }
    bound = m_region;
    if (bound != 0xffffffff) {
        if (m_active) {
            EnableRegionInput(bound);
            return;
        }
        DisableRegionInput(bound);
    }
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

// SYNTHETIC: WIZ8 0x004f3370
// W8TextBuffer::`scalar deleting destructor'

// FUNCTION: WIZ8 0x004f3480
W8TextBuffer::~W8TextBuffer()
{
    delete[] m_buffer;
}

// FUNCTION: WIZ8 0x004f3310
W8TextBuffer::W8TextBuffer()
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

// FUNCTION: WIZ8 0x004f33a0
W8TextBuffer::W8TextBuffer(const W8ControlsRect* bounds, const wchar_t* text, int font,
                           unsigned int layout_mode, int render_mode)
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

// FUNCTION: WIZ8 0x004f34a0
void W8TextBuffer::SetLayoutMode(unsigned int layout_mode)
{
    m_layoutMode = layout_mode;
    if ((m_layoutMode & 7) == 0) {
        m_layoutMode |= 2;
    }
    if ((m_layoutMode & 0x38) == 0) {
        m_layoutMode |= 8;
    }
}

// FUNCTION: WIZ8 0x004f34d0
void W8TextBuffer::SetText(const wchar_t* text, int font)
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

// FUNCTION: WIZ8 0x004f3540
void W8TextBuffer::SetLayoutBounds(const W8ControlsRect* bounds, unsigned char copy_pending,
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

// FUNCTION: WIZ8 0x004f35b0
void W8TextBuffer::UpdateLayout()
{
    unsigned int available_width = m_layoutBounds.right - m_layoutBounds.left;
    wchar_t* line = m_buffer;
    unsigned int accumulated_width = 0;
    wchar_t* previous_break = 0;
    short separator_width = StringPixLength((unsigned short*)g_W8TextSeparator0060CC74, m_font);

    m_lineCount = 1;
    if ((m_layoutMode & 0x40) == 0) {
        m_maxLineWidth = 0;
        size_t span = wcscspn(line, g_W8TextBreakCharacters00617C88);
        wchar_t* break_at = line + span;
        while (*break_at != L'\0') {
            *break_at = L'\0';
            short word_width = StringPixLength((unsigned short*)line, m_font);
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
        short final_width = StringPixLength((unsigned short*)line, m_font);
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
// FUNCTION: WIZ8 0x004f3c00
int W8TextBuffer::GetHorizontalPosition(int width)
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
// FUNCTION: WIZ8 0x004f3c50
int W8TextBuffer::GetVerticalPosition()
{
    unsigned int line_height = m_lineHeight;
    unsigned int font_height = GetFontHeight(m_font);
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
    int free_height = (m_layoutBounds.bottom - m_layoutBounds.top) - m_lineCount * line_height;
    if (free_height < 0) {
        free_height = 0;
    }
    if ((m_layoutMode & 0x20) != 0) {
        return m_layoutBounds.top + free_height + first_line_inset;
    }
    return m_layoutBounds.top + free_height / 2 + first_line_inset;
}

/* Rejects an override smaller than the active font's natural line height. */
// FUNCTION: WIZ8 0x004f3cf0
void W8TextBuffer::SetLineHeight(unsigned int height)
{
    if (height < GetFontHeight(m_font)) {
        m_lineHeight = 0;
        return;
    }
    m_lineHeight = height;
}

/* Fills the current text rectangle on the standard UI surface and queues the
   same rectangle for composition. */
// FUNCTION: WIZ8 0x004f3d50
void W8TextBuffer::FillBounds(int colour)
{
    ColorFillVideoSurfaceArea(-14, m_layoutBounds.left, m_layoutBounds.top, m_layoutBounds.right,
                              m_layoutBounds.bottom, colour);
    InvalidateRegion(m_layoutBounds.left, m_layoutBounds.top, m_layoutBounds.right,
                     m_layoutBounds.bottom, 0);
}

/* Draws each newline-delimited line through the active font context. The
   temporary terminators are restored before advancing to the next line. */
// FUNCTION: WIZ8 0x004f3710
void W8TextBuffer::RenderText(unsigned char* buffer, unsigned int pitch, int x_offset, int y_offset,
                              unsigned char force)
{
    wchar_t* line = m_buffer;
    if (line == 0 || (force == 0 && m_geometryDirty == 0)) {
        return;
    }

    SetFont(m_font);
    HVOBJECT font_object = GetFontObject(m_font);
    if (font_object == 0) {
        return;
    }
    SetObjectShade(font_object, m_renderMode);
    unsigned short* previous_state = GetFontObjectPalette16BPP(m_font);
    if (m_fontStateIndex != -1) {
        SetFontObjectPalette16BPP(m_font, g_font_state_palettes_68ee1c[m_fontStateIndex]);
    }
    SaveFontSettings();
    SetFontDestBuffer(g_W8TextClipTarget005FF5F4, m_pendingBounds.left, m_pendingBounds.top,
                      m_pendingBounds.right, m_pendingBounds.bottom, g_W8TextClipFlags00650E38);

    int y = GetVerticalPosition();
    size_t span = wcscspn(line, g_W8LineBreakCharacters00617C90);
    while (line[span] != L'\0') {
        line[span] = L'\0';
        int x = GetHorizontalPosition(StringPixLength((unsigned short*)line, m_font));
        if (m_alternateRenderer == 0) {
            gprintf_buffer(buffer, pitch, m_font, x + x_offset, y + y_offset,
                           (unsigned short*)L"%s", line);
        } else {
            mprintf_buffer(buffer, pitch, m_font, x + x_offset, y + y_offset,
                           (unsigned short*)L"%s", line);
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
        int x = GetHorizontalPosition(StringPixLength((unsigned short*)line, m_font));
        if (m_alternateRenderer == 0) {
            gprintf_buffer(buffer, pitch, m_font, x + x_offset, y + y_offset,
                           (unsigned short*)L"%s", line);
        } else {
            mprintf_buffer(buffer, pitch, m_font, x + x_offset, y + y_offset,
                           (unsigned short*)L"%s", line);
        }
    }

done:
    SetFontObjectPalette16BPP(m_font, previous_state);
    RestoreFontSettings();
    m_geometryDirty = 0;
}

/* The state-5 option and summary panels use the simpler surface renderer. It
   shares the buffer's layout and palette state but prints directly to the
   selected target, then restores the full-screen clip. */
// FUNCTION: WIZ8 0x004f39b0
void W8TextBuffer::RenderToTarget(int offset, unsigned char force, int target)
{
    wchar_t* line = m_buffer;
    if (line == 0 || (!force && !m_geometryDirty)) {
        return;
    }

    SetFont(m_font);
    HVOBJECT font_object = GetFontObject(m_font);
    if (font_object == 0) {
        return;
    }
    SetObjectShade(font_object, m_renderMode);
    SetFontDestBuffer(target, m_pendingBounds.left, m_pendingBounds.top, m_pendingBounds.right,
                      m_pendingBounds.bottom, 0);
    unsigned short* previous_state = GetFontObjectPalette16BPP(m_font);
    if (m_fontStateIndex != -1) {
        SetFontObjectPalette16BPP(m_font, g_font_state_palettes_68ee1c[m_fontStateIndex]);
    }
    if (m_flag_4c) {
        SetFontObjectPalette16BPP(m_font, g_font_state_palettes_68ee1c[1]);
    }

    int y = GetVerticalPosition();
    size_t span = wcscspn(line, g_W8LineBreakCharacters00617C90);
    while (line[span] != L'\0') {
        line[span] = L'\0';
        int x = GetHorizontalPosition(StringPixLength((unsigned short*)line, m_font));
        gprintf(x + offset, y + offset, (unsigned short*)L"%s", line);
        y += GetLineHeight();
        line[span] = L'\n';
        if (m_layoutBounds.bottom <= y) {
            goto done;
        }
        line += span + 1;
        span = wcscspn(line, g_W8LineBreakCharacters00617C90);
    }

    {
        int x = GetHorizontalPosition(StringPixLength((unsigned short*)line, m_font));
        gprintf(x + offset, y + offset, (unsigned short*)L"%s", line);
    }

done:
    SetFontObjectPalette16BPP(m_font, previous_state);
    InvalidateRegion(m_layoutBounds.left, m_layoutBounds.top, m_layoutBounds.right,
                     m_layoutBounds.bottom, 0);
    SetFontDestBuffer(-14, 0, 0, 640, 480, 0);
    m_geometryDirty = 0;
}

/* Copies the owned text into caller storage. The caller supplies the capacity;
   the canonical method performs the same unbounded wide-string copy. */
// FUNCTION: WIZ8 0x004f3990
void W8TextBuffer::CopyTextTo(wchar_t* destination)
{
    wcscpy(destination, m_buffer);
}

/* Returns the cached line height, falling back to the active font's 16-bit
   height when the cache is zero. */
// FUNCTION: WIZ8 0x004f3d30
unsigned int W8TextBuffer::GetLineHeight()
{
    unsigned int height = m_lineHeight;
    if (height == 0) {
        height = GetFontHeight(m_font);
    }
    return height;
}

/* The text-control declaration is shared in Controls.h so every consumer sees
   the same 20-slot hierarchy and its embedded W8TextBuffer at +0x60. */
__forceinline void W8TextControl::InvalidateCore(unsigned char immediate)
{
    if (m_pPanel != 0) {
        m_dirty = 1;
        if (immediate) {
            m_pPanel->Invalidate(0);
        } else {
            m_pPanel->m_fLayoutDirty = 1;
            RequestRedraw(0x80000000);
        }
        RequestRedraw(0x80000000);
    }
    m_textBuffer.SetGeometryDirty();
}

/* The empty text control used as a base by controls that finish their setup
   later. */
// FUNCTION: WIZ8 0x004f4160
W8TextControl::W8TextControl()
{
    m_stateFlags = 0;
    m_flags_38 = 0;
    m_alternateTextEnabled = 0;
    m_imageObject = -1;
    m_imageFrame = -1;
    m_normalSprite = -1;
    m_pressedSprite = -1;
    m_alternatePressedSprite = -1;
    m_alternateNormalSprite = -1;
    m_disabledSprite = -1;
    m_pressedTextOffset = 1;
    m_listener = 0;
    m_textBuffer.MarkGeometryDirty(10);
}

/* The 182-caller text-control constructor. The first six arguments construct
   the reviewed widget base, while the implicit W8TextBuffer constructor owns
   the second EH state. The remaining positional values and the two measured
   shorts are fixed by the constructor's direct stores and GetCatalogImageSize call;
   their descriptive identities remain unknown. */
// FUNCTION: WIZ8 0x004f4250
W8TextControl::W8TextControl(Controls* panel, unsigned int region, int left, int top, int right,
                             int bottom, int text_40, int text_44, int text_48, int text_4c,
                             int text_54, int text_50, int text_58)
    : W8Widget(panel, region, left, top, right, bottom)
{
    m_pressedSprite = text_4c;
    m_alternatePressedSprite = text_50;
    m_alternateNormalSprite = text_54;
    m_stateFlags = 0;
    m_alternateTextEnabled = 0;
    m_imageObject = text_40;
    m_imageFrame = text_44;
    m_normalSprite = text_48;
    m_disabledSprite = text_58;
    m_flags_38 = 0;
    m_listener = 0;
    m_pressedTextOffset = 1;

    int measured_text = text_48;
    if (text_40 == -1 || text_44 == -1 ||
        (measured_text == -1 && (measured_text = text_4c) == -1)) {
        m_measured_w = -1;
        m_measured_h = -1;
    } else {
        GetCatalogImageSize(text_40, text_44, measured_text, &m_measured_w, &m_measured_h);
    }

    if (right == 0) {
        right = left + m_measured_w;
    }
    if (bottom == 0) {
        bottom = top + m_measured_h;
    }
    SetBounds(left, top, right, bottom);

    m_textBuffer.SetLayoutBounds(panel->origin_x + left, panel->origin_y + top,
                                 panel->origin_x + right, panel->origin_y + bottom);
    if (m_textBuffer.HasBuffer()) {
        m_textBuffer.UpdateLayout();
    }
    m_textBuffer.MarkGeometryDirty(10);
}

/* Refresh the cached extent from the preferred text handle, falling back to
   the alternate handle. */
// FUNCTION: WIZ8 0x004F4800
unsigned char W8TextControl::MeasureText004F4800()
{
    int handle;

    if (m_imageObject != -1 && m_imageFrame != -1 &&
        ((handle = m_normalSprite) != -1 || (handle = m_pressedSprite) != -1)) {
        GetCatalogImageSize(m_imageObject, m_imageFrame, handle, &m_measured_w, &m_measured_h);
        return 1;
    }
    m_measured_w = -1;
    m_measured_h = -1;
    return 0;
}

/* Where the text should be drawn: the panel origin plus either the widget's
   corner or an alignment computed from its cached measured extent. */
// FUNCTION: WIZ8 0x004f4850
void W8TextControl::GetTextOrigin(int unused, int* px, int* py)
{
    short* measured;
    short width;
    int handle;
    int x;

    if (m_pPanel == 0) {
        srAssertFail("m_pPanel != NULL", "C:\\Projects\\Wizardry 8\\Local Code\\Controls.cpp",
                     0x739, 0);
    }
    *px = m_pPanel->origin_x;
    measured = &m_measured_w;
    *py = m_pPanel->origin_y;
    width = *measured;
    if (width == -1 || m_measured_h == -1) {
        if (m_imageObject == -1 || m_imageFrame == -1) {
            *measured = -1;
            m_measured_h = -1;
            goto plain;
        }
        handle = m_normalSprite;
        if (handle == -1 && (handle = m_pressedSprite, handle == -1)) {
            *measured = -1;
            m_measured_h = -1;
            goto plain;
        }
        GetCatalogImageSize(m_imageObject, m_imageFrame, handle, measured, &m_measured_h);
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

// FUNCTION: WIZ8 0x004f4990
void W8TextControl::Redraw(int full_redraw)
{
    if (!m_active || m_pPanel == 0) {
        return;
    }

    int text_state = 0;
    if ((m_stateFlags & 1) != 0 && (m_flags_38 & 2) == 0) {
        text_state = m_pressedTextOffset;
    }

    if (full_redraw == 0 && !m_dirty) {
        if (m_textBuffer.HasBuffer()) {
            m_textBuffer.RenderToTarget(text_state, 0, -14);
        }
        return;
    }

    if (m_imageObject == -1 || m_imageFrame == -1) {
        if (m_textBuffer.HasBuffer()) {
            m_textBuffer.RenderToTarget(text_state, static_cast<unsigned char>(full_redraw), -14);
        }
        return;
    }

    int sprite;
    if (!m_enabled) {
        sprite = m_disabledSprite;
        if (sprite == -1) {
            if (m_textBuffer.HasBuffer()) {
                m_textBuffer.RenderToTarget(text_state, static_cast<unsigned char>(full_redraw),
                                            -14);
            }
            ShadowVideoSurfaceRect(-14, m_pPanel->origin_x + m_left, m_pPanel->origin_y + m_top,
                                   m_pPanel->origin_x + m_right, m_pPanel->origin_y + m_bottom);
            m_dirty = 0;
            return;
        }
    } else if ((m_stateFlags & 1) == 0) {
        sprite = m_alternateTextEnabled != 0 && m_alternateNormalSprite != -1
                     ? m_alternateNormalSprite
                     : m_normalSprite;
    } else {
        sprite = m_alternateTextEnabled != 0 && m_alternatePressedSprite != -1
                     ? m_alternatePressedSprite
                     : m_pressedSprite;
    }

    int x;
    int y;
    GetTextOrigin(0, &x, &y);
    if (sprite != -1) {
        if (m_alternateTextEnabled != 0) {
            short width;
            short height;
            GetCatalogImageSize(m_imageObject, m_imageFrame, sprite, &width, &height);
            int x_inset = m_left - m_right + (unsigned short)width;
            int y_inset = m_top - m_bottom + (unsigned short)height;
            if (x_inset > 0) {
                x += x_inset / 2;
            }
            if (y_inset > 0) {
                y += y_inset / 2;
            }
        }
        DrawCatalogImageAndInvalidate(-14, m_imageObject, m_imageFrame, sprite, x, y, 2, 0);
    }

    if (m_textBuffer.HasBuffer()) {
        m_textBuffer.RenderToTarget(text_state, static_cast<unsigned char>(full_redraw), -14);
    }
    m_dirty = 0;
}

// FUNCTION: WIZ8 0x004f4460
void W8TextControl::SetBoundsFromRect(const W8ControlsRect* bounds)
{
    SetBounds(bounds->left, bounds->top, bounds->right, bounds->bottom);
    if (m_region != -1 && m_pPanel != 0) {
        SetRegionBounds(m_region, (unsigned short)((short)bounds->left + (short)m_pPanel->origin_x),
                        (unsigned short)((short)bounds->top + (short)m_pPanel->origin_y),
                        (unsigned short)((short)bounds->right + (short)m_pPanel->origin_x),
                        (unsigned short)((short)bounds->bottom + (short)m_pPanel->origin_y));
    }
}

// FUNCTION: WIZ8 0x004f44d0
void W8TextControl::SetBounds(int left, int top, int right, int bottom)
{
    short measured_width;
    short measured_height;

    m_left = left;
    m_top = top;
    m_right = right;
    m_bottom = bottom;
    if (m_pPanel != 0) {
        if (m_region != -1) {
            SetRegionBounds(m_region, (unsigned short)((short)left + (short)m_pPanel->origin_x),
                            (unsigned short)((short)top + (short)m_pPanel->origin_y),
                            (unsigned short)((short)right + (short)m_pPanel->origin_x),
                            (unsigned short)((short)bottom + (short)m_pPanel->origin_y));
        }
        if ((m_flags_38 & 2) != 0) {
            int absolute_left = m_pPanel->origin_x + left;
            int absolute_top = m_pPanel->origin_y + top;
            int absolute_right = m_pPanel->origin_x + right;
            int absolute_bottom = m_pPanel->origin_y + bottom;
            if (m_imageObject != -1 && m_imageFrame != -1) {
                GetCatalogImageSize(m_imageObject, m_imageFrame, m_normalSprite, &measured_width,
                                    &measured_height);
                if ((m_flags_38 & 4) != 0) {
                    absolute_left += 2 + (unsigned short)measured_width;
                } else {
                    absolute_right -= 2 + (unsigned short)measured_width;
                }
            }
            m_textBuffer.SetLayoutBounds(absolute_left, absolute_top, absolute_right,
                                         absolute_bottom);
            if (m_textBuffer.HasBuffer()) {
                m_textBuffer.UpdateLayout();
            }
            m_textBuffer.MarkGeometryDirty(9);
        }
    }
}

// FUNCTION: WIZ8 0x004f4650
void W8TextControl::Invalidate(unsigned char immediate)
{
    InvalidateCore(immediate);
}

// FUNCTION: WIZ8 0x004f4600
void W8TextControl::SetFlaggedRegionBounds(short left, short top, unsigned short right)
{
    if (m_region != -1 && m_pPanel != 0 && RegionHasFlags(m_region, 2)) {
        SetRegionBounds(m_region, (unsigned short)((short)m_pPanel->origin_x + left),
                        (unsigned short)((short)m_pPanel->origin_y + top), right, 0);
    }
}

// FUNCTION: WIZ8 0x004f46a0
void W8TextControl::AddLayoutFlags(unsigned int flags)
{
    short measured_width;
    short measured_height;

    m_flags_38 |= flags;
    if (m_pPanel != 0 && (m_flags_38 & 2) != 0) {
        int absolute_left = m_pPanel->origin_x + m_left;
        int absolute_top = m_pPanel->origin_y + m_top;
        int absolute_right = m_pPanel->origin_x + m_right;
        int absolute_bottom = m_pPanel->origin_y + m_bottom;
        if (m_imageObject != -1 && m_imageFrame != -1) {
            GetCatalogImageSize(m_imageObject, m_imageFrame, m_normalSprite, &measured_width,
                                &measured_height);
            if ((m_flags_38 & 4) != 0) {
                absolute_left += 2 + (unsigned short)measured_width;
            } else {
                absolute_right -= 2 + (unsigned short)measured_width;
            }
        }
        m_textBuffer.SetLayoutBounds(absolute_left, absolute_top, absolute_right, absolute_bottom);
        if (m_textBuffer.HasBuffer()) {
            m_textBuffer.UpdateLayout();
        }
        m_textBuffer.MarkGeometryDirty(9);
    }
}

// FUNCTION: WIZ8 0x004f4780
void W8TextControl::RemoveLayoutFlags(unsigned int flags)
{
    if ((flags & 2) != 0 && m_pPanel != 0) {
        m_textBuffer.SetLayoutBounds(m_pPanel->origin_x + m_left, m_pPanel->origin_y + m_top,
                                     m_pPanel->origin_x + m_right, m_pPanel->origin_y + m_bottom);
        if (m_textBuffer.HasBuffer()) {
            m_textBuffer.UpdateLayout();
        }
        m_textBuffer.MarkGeometryDirty(10);
    }
    m_flags_38 &= ~flags;
}

// FUNCTION: WIZ8 0x004f4c40
void W8TextControl::EnableSecondaryState(unsigned char immediate)
{
    if ((m_flags_38 & 1) != 0 && (m_stateFlags & 2) == 0) {
        m_stateFlags |= g_W8TextControlMask005ED56C;
        m_stateFlags |= g_W8TextControlMask005ED570;
        InvalidateCore(immediate);
    }
}

// FUNCTION: WIZ8 0x004f4cb0
void W8TextControl::DisableSecondaryState(unsigned char immediate)
{
    if ((m_flags_38 & 1) != 0 && (m_stateFlags & 2) != 0) {
        m_stateFlags &= ~g_W8TextControlMask005ED56C;
        m_stateFlags &= ~g_W8TextControlMask005ED570;
        InvalidateCore(immediate);
    }
}

// FUNCTION: WIZ8 0x004f4d30
void W8TextControl::OnMouseEnter(int event)
{
    if (!m_active) {
        return;
    }
    if (!m_enabled) {
        PushButtonSoundScheme005587C0(0, 1);
        SetAlternateTextEnabled(1);
        return;
    }
    if ((m_flags_38 & 0x60) != 0) {
        PushButtonSoundScheme005587C0(0, 1);
    }

    if ((m_stateFlags & 1) == 0) {
        if (m_alternateNormalSprite == -1) {
            return;
        }
    } else if (m_alternatePressedSprite == -1) {
        return;
    }

    SetAlternateTextEnabled(1);
    InvalidateCore((unsigned char)event);
}

// FUNCTION: WIZ8 0x004f4e00
void W8TextControl::OnMouseLeave(int event)
{
    if (!m_active) {
        return;
    }
    if (!m_enabled) {
        PushButtonSoundScheme005587C0(0, 1);
        if ((m_flags_38 & 1) == 0) {
            m_stateFlags &= ~g_W8TextControlMask005ED56C;
        }
        SetAlternateTextEnabled(0);
        return;
    }
    if ((m_flags_38 & 0x60) != 0) {
        PushButtonSoundScheme005587C0(0, 1);
    }
    if ((m_flags_38 & 0x100) != 0 && (m_stateFlags & 4) != 0) {
        PushButtonSoundScheme005587C0(0, 1);
        m_stateFlags &= ~4u;
    }

    if ((m_stateFlags & 1) == 0) {
        if (m_alternateNormalSprite == -1) {
            return;
        }
        SetAlternateTextEnabled(0);
        InvalidateCore((unsigned char)event);
        return;
    }

    if (m_alternatePressedSprite != -1 ||
        (m_alternateNormalSprite != -1 && (m_flags_38 & 0x10) != 0)) {
        SetAlternateTextEnabled(0);
        InvalidateCore((unsigned char)event);
    }
    if ((m_stateFlags & 2) != 0) {
        return;
    }
    m_stateFlags &= ~g_W8TextControlMask005ED56C;
    InvalidateCore((unsigned char)event);
}

// FUNCTION: WIZ8 0x004f4f70
void W8TextControl::OnLeftButtonDown(int event)
{
    if (!m_active) {
        if (m_enabled) {
            return;
        }
        PushButtonSoundScheme005587C0(0, 1);
        return;
    }
    if (!m_enabled) {
        PushButtonSoundScheme005587C0(0, 1);
        return;
    }
    if ((m_flags_38 & 0x20) != 0) {
        PushButtonSoundScheme005587C0(0, 1);
    }

    if ((m_flags_38 & 1) == 0) {
        m_stateFlags |= g_W8TextControlMask005ED56C;
        if (m_imageObject != -1 && m_imageFrame != -1) {
            InvalidateCore((unsigned char)event);
        }
    } else if ((m_stateFlags & 1) == 0) {
        m_stateFlags |= g_W8TextControlMask005ED56C;
        if ((m_flags_38 & 0x10) == 0) {
            InvalidateCore((unsigned char)event);
        }
    }

    m_textBuffer.SetGeometryDirty();
    if (m_leftButtonDownCallback != 0) {
        m_leftButtonDownCallback();
    }
}

// FUNCTION: WIZ8 0x004f5070
void W8TextControl::OnRightButtonDown(int)
{
    if ((m_active && m_enabled)) {
        if ((m_flags_38 & 0x20) != 0) {
            PushButtonSoundScheme005587C0(0, 1);
        }
        if (m_rightButtonDownCallback != 0) {
            m_rightButtonDownCallback();
        }
        return;
    }
    if (!m_active && m_enabled) {
        return;
    }
    PushButtonSoundScheme005587C0(0, 1);
}

// FUNCTION: WIZ8 0x004f50c0
void W8TextControl::OnLeftButtonUp(int event)
{
    if (!m_active) {
        return;
    }
    if (!m_enabled) {
        PushButtonSoundScheme005587C0(0, 1);
        if ((m_flags_38 & 1) == 0) {
            m_stateFlags &= ~g_W8TextControlMask005ED56C;
        }
        return;
    }
    if ((m_stateFlags & 1) == 0) {
        return;
    }
    if ((m_flags_38 & 0x20) != 0) {
        PushButtonSoundScheme005587C0(0, 1);
    }

    if ((m_flags_38 & 1) == 0) {
        m_stateFlags &= ~g_W8TextControlMask005ED56C;
        InvalidateCore((unsigned char)event);
    } else if ((m_stateFlags & 2) == 0) {
        m_stateFlags |= g_W8TextControlMask005ED56C;
        m_stateFlags |= g_W8TextControlMask005ED570;
        if ((m_flags_38 & 0x10) != 0) {
            InvalidateCore((unsigned char)event);
        }
    } else if ((m_flags_38 & 8) == 0) {
        m_stateFlags &= ~g_W8TextControlMask005ED56C;
        m_stateFlags &= ~g_W8TextControlMask005ED570;
        InvalidateCore((unsigned char)event);
    }

    m_textBuffer.SetGeometryDirty();
    if ((m_flags_38 & 0x100) != 0 && (m_stateFlags & 4) != 0) {
        m_stateFlags &= ~4u;
        PushButtonSoundScheme005587C0(0, 1);
        return;
    }
    if (m_listener != 0) {
        m_listener->OnPrimary(this);
    }
    if (m_primaryActivationCallback != 0) {
        m_primaryActivationCallback();
    }
}

// FUNCTION: WIZ8 0x004f5290
void W8TextControl::OnRightButtonUp(int)
{
    if (!m_active) {
        if (m_enabled) {
            return;
        }
        PushButtonSoundScheme005587C0(0, 1);
        return;
    }
    if (!m_enabled) {
        PushButtonSoundScheme005587C0(0, 1);
        return;
    }
    if ((m_flags_38 & 0x20) != 0) {
        PushButtonSoundScheme005587C0(0, 1);
    }
    if ((m_flags_38 & 0x100) != 0 && (m_stateFlags & 4) != 0) {
        m_stateFlags &= ~4u;
        PushButtonSoundScheme005587C0(0, 1);
        return;
    }
    if (m_listener != 0) {
        m_listener->OnSecondary(this);
    }
    if (m_secondaryActivationCallback != 0) {
        m_secondaryActivationCallback();
    }
}

// FUNCTION: WIZ8 0x004f5230
void W8TextControl::ActivatePrimary(int)
{
    if ((m_flags_38 & 0x100) != 0 && m_active && m_enabled && (m_stateFlags & 1) != 0) {
        m_stateFlags |= 4;
        if ((m_flags_38 & 0x20) == 0) {
            PlayButtonSound(3);
        }
        if (m_listener != 0) {
            m_listener->OnPrimary(this);
        }
        if (m_primaryActivationCallback != 0) {
            m_primaryActivationCallback();
        }
    }
}

// FUNCTION: WIZ8 0x004f5310
void W8TextControl::OnLeftButtonDoubleClick(int)
{
    if (m_active && m_enabled) {
        if ((m_flags_38 & 0x20) != 0) {
            PushButtonSoundScheme005587C0(0, 1);
        }
        if ((m_flags_38 & 1) != 0 && (m_stateFlags & 2) == 0) {
            return;
        }
        if (m_leftDoubleClickCallback != 0) {
            m_leftDoubleClickCallback();
        }
        return;
    }
    if (!m_active && m_enabled) {
        return;
    }
    PushButtonSoundScheme005587C0(0, 1);
}

// FUNCTION: WIZ8 0x004f5360
void W8TextControl::ActivateSecondary(int)
{
    if ((m_flags_38 & 0x100) != 0 && m_active && m_enabled) {
        m_stateFlags |= 4;
        if ((m_flags_38 & 0x20) == 0) {
            PlayButtonSound(3);
        }
        if (m_listener != 0) {
            m_listener->OnSecondary(this);
        }
        if (m_secondaryActivationCallback != 0) {
            m_secondaryActivationCallback();
        }
    }
}

// FUNCTION: WIZ8 0x004f53b0
void W8TextControl::UpdateTextBounds(int left, int top, int right, int bottom)
{
    W8ControlsRect absolute = {m_pPanel->origin_x + left, m_pPanel->origin_y + top,
                               m_pPanel->origin_x + right, m_pPanel->origin_y + bottom};
    m_textBuffer.SetLayoutBounds(&absolute, 1, 0);
    if (m_textBuffer.HasBuffer()) {
        m_textBuffer.UpdateLayout();
    }
    m_textBuffer.SetGeometryDirty();
}

// FUNCTION: WIZ8 0x004f5410
void W8TextControl::SetEnabled(bool enabled)
{
    m_enabled = enabled;
    if (!enabled) {
        if (m_disabledSprite != -1) {
            m_textBuffer.SetRenderMode(7);
        }
    } else if (m_disabledSprite != -1) {
        m_textBuffer.SetRenderMode(4);
    }
}

// FUNCTION: WIZ8 0x004f69a0
void W8TextControl::SetAlternateTextEnabled(unsigned char enabled)
{
    m_alternateTextEnabled = enabled;
}

// FUNCTION: WIZ8 0x004f65e0
W8HelpTextControl::W8HelpTextControl(Controls* panel, unsigned int region, int left, int top,
                                     int right, int bottom)
    : W8TextControl(panel, region, left, top, right, bottom, -1, -1, -1, -1, -1, -1, -1)
{
    wcscpy(m_regionHelp, L"");
}

class W8RangeControl;

// VTABLE: WIZ8 0x005ed6b4
class W8VerticalRangeThumb : public W8Widget {
public:
    W8VerticalRangeThumb(W8RangeControl* range, int left, int top, int right, int bottom,
                         int render_arg, int normal_sprite, int hovered_sprite,
                         int disabled_sprite);
    virtual void Redraw(int full_redraw) override;
    void AdjustValue(int steps) override;
    virtual void OnMouseEnter(int event) override;
    virtual void OnMouseLeave(int event) override;
    virtual void OnLeftButtonDown(int event) override;
    virtual void OnLeftButtonUp(int event) override;
    virtual void OnMouseMove(int event) override;
    __forceinline void SetRangePosition(float position)
    {
        m_position = position;
        if (m_position < m_minimumPosition) {
            m_position = m_minimumPosition;
        }
        if (m_maximumPosition < m_position) {
            m_position = m_maximumPosition;
        }
        m_pixelPosition =
            (int)(((m_position - m_minimumPosition) / (m_maximumPosition - m_minimumPosition)) *
                  m_trackLength);
        if (m_pPanel != 0) {
            m_dirty = 1;
            m_pPanel->m_fLayoutDirty = 1;
            RequestRedraw(0x80000000);
            RequestRedraw(0x80000000);
        }
    }

protected:
    int m_renderArg;
    int m_renderArg38;
    int m_normalSprite;
    int m_hoveredSprite;
    int m_disabledSprite;
    int m_drawOffsetX;
    int m_pixelPosition;  /* 0x4c */
    int m_thumbHeight;    /* 0x50 */
    int m_trackLength;    /* 0x54 */
    int m_dragCoordinate; /* 0x58 */
    bool m_hovered;       /* 0x5c: cursor over the thumb */
    bool m_dragging;      /* 0x5d: thumb drag in progress */
    unsigned char pad_5e[2];
    float m_minimumPosition; /* 0x60 */
    float m_maximumPosition; /* 0x64 */
    float m_position;        /* 0x68 */
    W8RangeControl* m_range; /* 0x6c */

    void ClampPositionAndInvalidate();
    void SynchronizeRangeValue();
};

__forceinline W8RangeButton::W8RangeButton(Controls* panel, unsigned int region, int left, int top,
                                           int right, int bottom, int text_40, int text_44,
                                           int text_48, int text_4c, int text_54, int text_50,
                                           int text_58, short direction, W8RangeControl* range)
    : W8TextControl(panel, region, left, top, right, bottom, text_40, text_44, text_48, text_4c,
                    text_54, text_50, text_58),
      m_direction(direction), m_range(range)
{
}

/* The range panel owns two text-derived step buttons and one vertical thumb.
   Retail construction passes five stack arguments and proves all three
   allocation sizes, the ordered child construction, and the four EH cleanup
   states. */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow-field"
/* Controls stores panel bounds as `right`/`bottom`; this constructor forwards
   those same names to the base. That is original constructor style, not a
   second inherited field. */
// FUNCTION: WIZ8 0x004f61f0
W8RangeControl::W8RangeControl(int left, int top, int right, int bottom,
                               unsigned int* shared_region_set)
    : Controls(left, top, right, bottom, -1, -1, -1), m_minimum(0), m_maximum(1), m_value(0),
      m_listener(0)
{
    int height = bottom - top;

    if (*shared_region_set == 0) {
        *shared_region_set = CreateRegionSet();
    }
    m_uiRegionSetId = *shared_region_set;
    ResetRegionSet(m_uiRegionSetId);

    m_decrement =
        new W8RangeButton(this, 0xffffffff, 0, 0, 0x10, 0x10, 0x86, 0, 0, 2, 1, 2, 3, 0, this);
    m_increment = new W8RangeButton(this, 0xffffffff, 0, height - 0x10, 0x10, height, 0x86, 0, 8,
                                    10, 9, 10, 0xb, 1, this);
    m_thumb = new W8VerticalRangeThumb(this, 0, 0x10, 0x10, height - 0x10, 0x86, 4, 5, -1);
    m_enabled = 0;
}
#pragma clang diagnostic pop

/* Child deletion order follows the three null-tested scalar-deleting virtual
   calls in the retail body; the Controls destructor then releases the typed
   embedded vector. */
// FUNCTION: WIZ8 0x004f63c0
W8RangeControl::~W8RangeControl()
{
    delete m_decrement;
    delete m_increment;
    delete m_thumb;
}

// FUNCTION: WIZ8 0x004f6440
void W8RangeControl::SetRange(int first, int second)
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

// FUNCTION: WIZ8 0x004f6470
void W8RangeControl::SetValue(int value)
{
    m_value = value;
    if (value < m_minimum) {
        m_value = m_minimum;
    } else if (m_maximum < value) {
        m_value = m_maximum;
    }

    float position;
    if (m_value == m_minimum) {
        position = g_float_005ebb34;
    } else if (m_value == m_maximum) {
        position = g_float_005ebb38;
    } else {
        position = ((float)(m_value - m_minimum) + g_float_005ebc7c) /
                   (float)((m_maximum - m_minimum) + 1);
    }
    m_thumb->SetRangePosition(position);
}

// FUNCTION: WIZ8 0x004f6540
void W8RangeControl::Decrement()
{
    if (m_enabled) {
        SetValue(m_value - 1);
        if (m_listener != 0) {
            m_listener->OnRangeChanged(this);
        }
    }
}

// FUNCTION: WIZ8 0x004f6570
void W8RangeControl::Increment()
{
    if (m_enabled) {
        SetValue(m_value + 1);
        if (m_listener != 0) {
            m_listener->OnRangeChanged(this);
        }
    }
}

// FUNCTION: WIZ8 0x004f65a0
void W8RangeControl::SetRangeEnabled(bool enabled)
{
    m_enabled = enabled;
    m_decrement->SetEnabled(enabled);
    m_increment->SetEnabled(enabled & m_enabled);
    m_thumb->SetEnabled(enabled);
}

__forceinline void W8VerticalRangeThumb::ClampPositionAndInvalidate()
{
    if (m_position < m_minimumPosition) {
        m_position = m_minimumPosition;
    }
    if (m_maximumPosition < m_position) {
        m_position = m_maximumPosition;
    }
    m_pixelPosition =
        (int)(((m_position - m_minimumPosition) / (m_maximumPosition - m_minimumPosition)) *
              m_trackLength);
    if (m_pPanel != 0) {
        m_dirty = 1;
        m_pPanel->m_fLayoutDirty = 1;
        RequestRedraw(0x80000000);
        RequestRedraw(0x80000000);
    }
}

__forceinline void W8VerticalRangeThumb::SynchronizeRangeValue()
{
    int value =
        (int)(m_range->m_thumb->m_position * (float)(m_range->m_maximum - m_range->m_minimum + 1)) +
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

// FUNCTION: WIZ8 0x004f5b20
W8VerticalRangeThumb::W8VerticalRangeThumb(W8RangeControl* range, int left, int top, int right,
                                           int bottom, int render_arg, int normal_sprite,
                                           int hovered_sprite, int disabled_sprite)
    : W8Widget(range, 0xffffffff, left, top, 0, bottom)
{
    short width;
    short height;

    m_hoveredSprite = hovered_sprite;
    m_disabledSprite = disabled_sprite;
    m_renderArg = render_arg;
    m_renderArg38 = 0;
    m_normalSprite = normal_sprite;
    m_hovered = false;
    m_dragging = false;
    m_minimumPosition = 0.0f;
    m_maximumPosition = 1.0f;
    m_position = 0.0f;
    GetCatalogImageSize(render_arg, 0, normal_sprite, &width, &height);
    if (right - left < (unsigned short)width) {
        m_right = m_left + (unsigned short)width;
        m_drawOffsetX = 0;
    } else {
        m_right = right;
        m_drawOffsetX = right - (unsigned short)width - left;
    }
    SetRegion(m_region);
    m_thumbHeight = (unsigned short)height;
    m_trackLength = bottom - (unsigned short)height - top;
    m_range = range;
}

// FUNCTION: WIZ8 0x004f5c00
void W8VerticalRangeThumb::OnLeftButtonDown(int event)
{
    PushButtonSoundScheme005587C0(0, 1);
    if (m_enabled) {
        POINT cursor;
        SGPMouseGetPos(&cursor);
        int y = cursor.y - m_pPanel->origin_y - m_top;
        if (!m_hovered) {
            m_hovered = true;
            m_position = ((float)(y - m_thumbHeight / 2) / (float)m_trackLength) *
                             (m_maximumPosition - m_minimumPosition) +
                         m_minimumPosition;
            ClampPositionAndInvalidate();
            SynchronizeRangeValue();
        }
        m_dragCoordinate = y;
        m_dragging = true;
        ActivateDialogRegion(m_region);
    }
}

// FUNCTION: WIZ8 0x004f5d30
void W8VerticalRangeThumb::OnLeftButtonUp(int event)
{
    PushButtonSoundScheme005587C0(0, 1);
    if (m_enabled && m_dragging) {
        m_dragging = false;
        ClearActiveRegionIfMatches(m_region);
    }
}

// FUNCTION: WIZ8 0x004f5d70
void W8VerticalRangeThumb::OnMouseMove(int event)
{
    if (!m_enabled) {
        return;
    }

    POINT cursor;
    SGPMouseGetPos(&cursor);
    int y = cursor.y - m_pPanel->origin_y - m_top;
    if (m_dragging) {
        int half_height = m_thumbHeight / 2;
        if (y <= half_height) {
            m_position = 0.0f;
        } else if (m_trackLength + half_height <= y) {
            m_position = 1.0f;
        } else {
            int delta = y - m_dragCoordinate;
            m_dragCoordinate = y;
            m_position +=
                ((float)delta / (float)m_trackLength) * (m_maximumPosition - m_minimumPosition);
        }
        ClampPositionAndInvalidate();
        SynchronizeRangeValue();
        return;
    }

    bool hovered = (m_pixelPosition <= y && y <= m_pixelPosition + m_thumbHeight);
    if (hovered != m_hovered && m_pPanel != 0) {
        m_dirty = 1;
        m_pPanel->m_fLayoutDirty = 1;
        RequestRedraw(0x80000000);
        RequestRedraw(0x80000000);
    }
    m_hovered = hovered;
}

// FUNCTION: WIZ8 0x004f5ef0
void W8VerticalRangeThumb::AdjustValue(int steps)
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

// FUNCTION: WIZ8 0x004f5f60
void W8VerticalRangeThumb::Redraw(int full_redraw)
{
    if (!m_active || (static_cast<unsigned char>(full_redraw) == 0 && !m_dirty)) {
        return;
    }
    int left = m_pPanel->origin_x + m_left;
    int top = m_pPanel->origin_y + m_top;
    int right = m_pPanel->origin_x + m_right;
    int bottom = m_pPanel->origin_y + m_bottom;
    InvalidateRegion(left, top, right, bottom, 0);
    ColorFillVideoSurfaceArea(-14, left, top, right, bottom, 0x8000);

    int sprite;
    if (!m_enabled) {
        sprite = m_disabledSprite;
        if (sprite == -1) {
            return;
        }
    } else if (!m_hovered || (sprite = m_hoveredSprite) == -1) {
        sprite = m_normalSprite;
    }
    DrawCatalogImage(-14, m_renderArg, m_renderArg38, sprite,
                     m_pPanel->origin_x + m_drawOffsetX + m_left,
                     m_pPanel->origin_y + m_pixelPosition + m_top, 2, 0);
}

// FUNCTION: WIZ8 0x004f6050
void W8RangeButton::OnLeftButtonDown(int event)
{
    W8TextControl::OnLeftButtonDown(event);
    if (m_active && m_enabled) {
        if (m_direction == 0) {
            m_range->Decrement();
        } else {
            m_range->Increment();
        }
    }
}

// FUNCTION: WIZ8 0x004f60c0
void W8RangeButton::ActivatePrimary(int event)
{
    W8TextControl::ActivatePrimary(event);
    if (m_active && m_enabled) {
        if (m_direction == 0) {
            m_range->Decrement();
        } else {
            m_range->Increment();
        }
    }
}

// FUNCTION: WIZ8 0x004f6180
void W8RangeButton::AdjustValue(int steps)
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

// FUNCTION: WIZ8 0x004f6680
void W8HelpTextControl::SetRegionHelp(const wchar_t* text)
{
    if (wcslen(text) < 200) {
        wcscpy(m_regionHelp, text);
    }
}

// FUNCTION: WIZ8 0x004f66b0
void W8HelpTextControl::OnMouseEnter(int event)
{
    PushButtonSoundScheme005587C0(0, 1);
    W8TextControl::OnMouseEnter(event);
    if (wcslen(m_regionHelp) > 1 && m_region != -1) {
        ::SetRegionHelpText(m_regionHelp);
        ::EnableRegionHelp(m_region);
        return;
    }
    if (m_region != -1) {
        ::DisableRegionHelp(m_region);
    }
}

// FUNCTION: WIZ8 0x005b7cb0
void W8HelpTextControl::OnLeftButtonDown(int event)
{
    PushButtonSoundScheme005587C0(0, 1);
    W8TextControl::OnLeftButtonDown(event);
}

// FUNCTION: WIZ8 0x004f6720
void W8HelpTextControl::OnRightButtonDown(int event)
{
    if (m_secondaryActivationCallback == 0) {
        PushButtonSoundScheme005587C0(0, 1);
    }
    W8TextControl::OnRightButtonDown(event);
}

// FUNCTION: WIZ8 0x005b7cd0
void W8HelpTextControl::OnLeftButtonUp(int event)
{
    PushButtonSoundScheme005587C0(0, 1);
    W8TextControl::OnLeftButtonUp(event);
}

// FUNCTION: WIZ8 0x004f6780
void W8HelpTextControl::OnRightButtonUp(int)
{
    if (m_secondaryActivationCallback == 0) {
        PushButtonSoundScheme005587C0(0, 1);
    }
    if (m_active && m_enabled) {
        if ((m_flags_38 & 0x20) != 0) {
            PushButtonSoundScheme005587C0(0, 1);
        }
        if ((m_flags_38 & 0x100) != 0 && (m_stateFlags & 4) != 0) {
            m_stateFlags &= ~4u;
            PushButtonSoundScheme005587C0(0, 1);
            return;
        }
        if (m_listener != 0) {
            m_listener->OnSecondary(this);
        }
        if (m_secondaryActivationCallback != 0) {
            m_secondaryActivationCallback();
        }
        return;
    }
    if (!m_active && m_enabled) {
        return;
    }
    PushButtonSoundScheme005587C0(0, 1);
}

// FUNCTION: WIZ8 0x004f6810
void W8HelpTextControl::OnLeftButtonDoubleClick(int event)
{
    PushButtonSoundScheme005587C0(0, 1);
    W8TextControl::OnLeftButtonDoubleClick(event);
}

/* The vtable at 0x005ED66C is the horizontal draggable range thumb. Its
   constructor measures the normal sprite for the widget bounds, then measures
   the movable thumb sprite and retains the remaining horizontal travel at
   +0x4c. The interaction methods independently prove that geometry: cursor X
   is converted through +0x4c into the normalized float range +0x60..+0x68. */
__forceinline void W8HorizontalRangeThumb::InvalidateThumb()
{
    if (m_pPanel != 0) {
        m_dirty = 1;
        m_pPanel->m_fLayoutDirty = 1;
        RequestRedraw(0x80000000);
        RequestRedraw(0x80000000);
    }
}

__forceinline void W8HorizontalRangeThumb::ClampPositionAndInvalidate()
{
    if (m_position < m_minimumPosition) {
        m_position = m_minimumPosition;
    }
    if (m_maximumPosition < m_position) {
        m_position = m_maximumPosition;
    }
    m_pixelPosition =
        (int)(((m_position - m_minimumPosition) / (m_maximumPosition - m_minimumPosition)) *
              m_trackLength);
    InvalidateThumb();
}

// FUNCTION: WIZ8 0x004f5620
W8HorizontalRangeThumb::W8HorizontalRangeThumb(Controls* panel, unsigned int region, int left,
                                               int top, int render_arg_0, int render_arg_1,
                                               int background_sprite, int normal_thumb_sprite,
                                               int hovered_thumb_sprite, int disabled_thumb_sprite)
    : W8Widget(panel, region, left, top, 0, 0)
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
    m_hovered = false;
    m_dragging = false;
    m_minimumPosition = 0.0f;
    m_maximumPosition = 1.0f;
    m_position = 0.0f;
    m_listener = 0;
    GetCatalogImageSize(render_arg_0, render_arg_1, background_sprite, &width, &height);
    m_right = (unsigned short)width + m_left;
    m_bottom = m_top + (unsigned short)height;
    SetRegion(m_region);
    GetCatalogImageSize(m_renderArg0, m_renderArg1, m_normalThumbSprite, &width, &height);
    m_thumbWidth = (unsigned short)width;
    m_trackLength = (m_right - m_left) - (unsigned short)width;
}

// FUNCTION: WIZ8 0x004f5710
void W8HorizontalRangeThumb::UpdatePixelPosition()
{
    ClampPositionAndInvalidate();
}

// FUNCTION: WIZ8 0x004f5780
void W8HorizontalRangeThumb::OnLeftButtonDown(int event)
{
    PushButtonSoundScheme005587C0(0, 1);
    if (m_enabled) {
        POINT cursor;
        SGPMouseGetPos(&cursor);
        int x = cursor.x - m_pPanel->origin_x - m_left;
        if (!m_hovered) {
            m_hovered = true;
            m_position = ((float)(x - m_thumbWidth / 2) / (float)m_trackLength) *
                             (m_maximumPosition - m_minimumPosition) +
                         m_minimumPosition;
            ClampPositionAndInvalidate();
            if (m_listener != 0) {
                m_listener->OnDrag(this);
            }
        }
        m_dragCoordinate = x;
        m_dragging = true;
        ActivateDialogRegion(m_region);
    }
}

// FUNCTION: WIZ8 0x004f5880
void W8HorizontalRangeThumb::OnLeftButtonUp(int event)
{
    PushButtonSoundScheme005587C0(0, 1);
    if (m_enabled && m_dragging) {
        m_dragging = false;
        ClearActiveRegionIfMatches(m_region);
        if (m_listener != 0) {
            m_listener->OnDragEnd(this);
        }
    }
}

// FUNCTION: WIZ8 0x004f58c0
void W8HorizontalRangeThumb::OnMouseEnter(int)
{
    PushButtonSoundScheme005587C0(0, 1);
}

/* Retail folds these two hover hooks with the horizontal thumb's methods. */
void W8VerticalRangeThumb::OnMouseEnter(int)
{
    PushButtonSoundScheme005587C0(0, 1);
}

void W8VerticalRangeThumb::OnMouseLeave(int event)
{
    PushButtonSoundScheme005587C0(0, 1);
    if (m_hovered && !m_dragging) {
        m_hovered = false;
        if (m_pPanel != 0) {
            m_dirty = 1;
            if (static_cast<unsigned char>(event) != 0) {
                m_pPanel->Invalidate(0);
                RequestRedraw(0x80000000);
                return;
            }
            m_pPanel->m_fLayoutDirty = 1;
            RequestRedraw(0x80000000);
            RequestRedraw(0x80000000);
        }
    }
}

// FUNCTION: WIZ8 0x004f58d0
void W8HorizontalRangeThumb::OnMouseLeave(int event)
{
    PushButtonSoundScheme005587C0(0, 1);
    if (m_hovered && !m_dragging) {
        m_hovered = false;
        if (m_pPanel != 0) {
            m_dirty = 1;
            if (static_cast<unsigned char>(event) != 0) {
                m_pPanel->Invalidate(0);
                RequestRedraw(0x80000000);
                return;
            }
            m_pPanel->m_fLayoutDirty = 1;
            RequestRedraw(0x80000000);
            RequestRedraw(0x80000000);
        }
    }
}

// FUNCTION: WIZ8 0x004f5940
void W8HorizontalRangeThumb::OnMouseMove(int event)
{
    if (!m_enabled) {
        return;
    }

    POINT cursor;
    SGPMouseGetPos(&cursor);
    int x = cursor.x - m_pPanel->origin_x - m_left;
    if (m_dragging) {
        if (x < 0 || m_trackLength + m_thumbWidth / 2 < x) {
            return;
        }
        int delta = x - m_dragCoordinate;
        m_dragCoordinate = x;
        m_position +=
            ((float)delta / (float)m_trackLength) * (m_maximumPosition - m_minimumPosition);
        ClampPositionAndInvalidate();
        if (m_listener != 0) {
            m_listener->OnDrag(this);
        }
        return;
    }

    bool hovered = (m_pixelPosition <= x && x <= m_pixelPosition + m_thumbWidth);
    if (m_hovered != hovered && m_pPanel != 0) {
        InvalidateThumb();
    }
    m_hovered = hovered;
}

// FUNCTION: WIZ8 0x004f5a80
void W8HorizontalRangeThumb::Redraw(int full_redraw)
{
    if (!m_active || ((unsigned char)full_redraw == 0 && !m_dirty)) {
        return;
    }

    int x = m_pPanel->origin_x + m_left;
    int y = m_pPanel->origin_y + m_top;
    DrawCatalogImageAndInvalidate(-14, m_renderArg0, m_renderArg1, m_backgroundSprite, x, y, 2, 0);

    int sprite;
    if (!m_enabled && m_disabledThumbSprite != -1) {
        sprite = m_disabledThumbSprite;
    } else if (m_hovered && m_hoveredThumbSprite != -1) {
        sprite = m_hoveredThumbSprite;
    } else {
        sprite = m_normalThumbSprite;
    }
    DrawCatalogImage(-14, m_renderArg0, m_renderArg1, sprite, x + m_pixelPosition, y, 2, 0);
}

// SYNTHETIC: WIZ8 0x004f69b0
// W8HorizontalRangeThumb::`scalar deleting destructor'
// FUNCTION: WIZ8 0x004f69d0
W8HorizontalRangeThumb::~W8HorizontalRangeThumb() {}

// SYNTHETIC: WIZ8 0x004f6030
// W8TextControl::`scalar deleting destructor'
// FUNCTION: WIZ8 0x004f6640
W8TextControl::~W8TextControl() {}

/* Enables or disables the whole panel: the panel's own flag, then every child's,
   and each child's region follows - mode 4 restores the disabled region and
   clearing the mode bits re-arms it. */
// FUNCTION: WIZ8 0x004f2d50
void Controls::SetEnabled(bool enable)
{
    int index;

    m_fEnabled = enable;
    for (index = 0; index < m_controls.count; ++index) {
        W8Widget* control = ControlAt(index);

        control->m_active = enable;
        if (control->m_region != -1) {
            if (enable == 0) {
                DisableRegionInput(control->m_region);
            } else {
                EnableRegionInput(control->m_region);
            }
        }
    }
}

/* Detaches the first matching control through the panel's ordinary vector. */
// FUNCTION: WIZ8 0x004f2da0
void Controls::RemoveControl(W8Widget* control)
{
    int index = m_controls.IndexOf(control);

    if (index != -1) {
        m_controls.RemoveAt(index);
    }
}

/* Tears the panel down back to front, unlinking each control before deleting it. */
// FUNCTION: WIZ8 0x004f2df0
void Controls::DestroyAllControls()
{
    int index = m_controls.count;

    if (index > 0) {
        while (--index, index >= 0) {
            delete m_controls.RemoveAt(index);
        }
    }
}

/* Adds a rectangle to the panel's pending redraw. A null rectangle means the
   whole panel, and the first rectangle after a flush - recognised by a left
   edge of -1 - is copied rather than unioned. */
// FUNCTION: WIZ8 0x004f2e50
void Controls::Invalidate(const W8ControlsRect* rect)
{
    int edge;

    m_fDirty = 1;
    if (rect == 0) {
        m_fWholeAreaDirty = 1;
        RequestRedraw(0x80000000);
        return;
    }
    edge = m_dirtyRect.left;
    m_fWholeAreaDirty = 0;
    if (edge == -1) {
        m_dirtyRect.left = rect->left;
        m_dirtyRect.top = rect->top;
        m_dirtyRect.right = rect->right;
        m_dirtyRect.bottom = rect->bottom;
        RequestRedraw(0x80000000);
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
    RequestRedraw(0x80000000);
}

/* Marks the panel's layout stale without touching the redraw rectangle. */
// FUNCTION: WIZ8 0x004f2f00
void Controls::InvalidateLayout()
{
    m_fLayoutDirty = 1;
    RequestRedraw(0x80000000);
}

/* Flushes pending panel drawing, then asks each enabled child to redraw. A
   full panel request uses the target-backed path when one exists; a bounded
   request uses the accumulated rectangle. */
// FUNCTION: WIZ8 0x004f2f10
void Controls::Redraw()
{
    int redrawn = 0;
    int index;

    if (!m_fEnabled) {
        return;
    }
    if (m_fDirty) {
        if (m_renderTarget != -1) {
            DrawCatalogImage(-14, m_renderTarget, m_renderArg_1c, m_renderArg_20, origin_x,
                             origin_y, 2, 0);
        }
        if (m_fWholeAreaDirty) {
            if (m_renderTarget != -1) {
                InvalidateCatalogImageRect(m_renderTarget, m_renderArg_1c, m_renderArg_20, origin_x,
                                           origin_y, 2);
            }
        } else {
            InvalidateRegion(m_dirtyRect.left, m_dirtyRect.top, m_dirtyRect.right,
                             m_dirtyRect.bottom, 2);
        }
        m_fDirty = 0;
        m_dirtyRect.left = -1;
        redrawn = 1;
    } else if (!m_fLayoutDirty) {
        return;
    }
    for (index = 0; index < m_controls.count; ++index) {
        if (ControlAt(index)->m_active) {
            ControlAt(index)->Redraw(redrawn);
        }
    }
    m_fLayoutDirty = 0;
}

/* Replaces the panel bounds and forwards each child's existing relative
   rectangle through virtual slot three so derived widgets can respond. */
// FUNCTION: WIZ8 0x004f3010
void Controls::SetBounds(int left, int top, int new_right, int new_bottom)
{
    int index;

    origin_x = left;
    origin_y = top;
    right = new_right;
    bottom = new_bottom;
    for (index = 0; index < m_controls.count; ++index) {
        ControlAt(index)->SetBounds(ControlAt(index)->m_left, ControlAt(index)->m_top,
                                    ControlAt(index)->m_right, ControlAt(index)->m_bottom);
    }
}

/* Takes the panel's region set from a shared slot, creating it on first use,
   and empties it so this panel can repopulate it. */
// FUNCTION: WIZ8 0x004f30c0
void Controls::AcquireRegionSet(unsigned int* shared_region_set)
{
    if (*shared_region_set == 0) {
        *shared_region_set = CreateRegionSet();
    }
    m_uiRegionSetId = *shared_region_set;
    ResetRegionSet(m_uiRegionSetId);
}

/* Enables timed help for this widget's region when it owns one. */
// FUNCTION: WIZ8 0x004f4120
void W8Widget::EnableRegionHelp(int help_text_id)
{
    if (m_region != -1) {
        SetRegionHelp(m_region, 1, help_text_id);
    }
}

/* Disables timed help and clears the text id for this widget's region. */
// FUNCTION: WIZ8 0x004f4140
void W8Widget::DisableRegionHelp()
{
    if (m_region != -1) {
        SetRegionHelp(m_region, 0, -1);
    }
}

/* Marks the widget dirty and either asks its panel to invalidate immediately
   or raises the panel's deferred-layout flag. */
// FUNCTION: WIZ8 0x004f40a0
void W8Widget::Invalidate(unsigned char immediate)
{
    if (m_pPanel != 0) {
        m_dirty = 1;
        if (immediate) {
            m_pPanel->Invalidate(0);
            RequestRedraw(0x80000000);
            return;
        }
        m_pPanel->m_fLayoutDirty = 1;
        RequestRedraw(0x80000000);
        RequestRedraw(0x80000000);
    }
}

// FUNCTION: WIZ8 0x004f40f0
void W8Widget::SetActive(bool active)
{
    m_active = active;
    if (m_region != -1) {
        if (active) {
            EnableRegionInput(m_region);
            return;
        }
        DisableRegionInput(m_region);
    }
}

// FUNCTION: WIZ8 0x004f6950
void W8Widget::SetEnabled(bool enabled)
{
    m_enabled = enabled;
}

// FUNCTION: WIZ8 0x004f6980
void W8Widget::SetBounds(int left, int top, int right, int bottom)
{
    m_left = left;
    m_top = top;
    m_right = right;
    m_bottom = bottom;
}

// FUNCTION: WIZ8 0x004f6960
void W8Widget::SetBoundsFromRect(const W8ControlsRect* bounds)
{
    m_left = bounds->left;
    m_top = bounds->top;
    m_right = bounds->right;
    m_bottom = bounds->bottom;
}

/* Selection controller: the listener occupies +0, the text-control pointer
   vector +0x10. Controls.cpp:2679 names m_lsButtons and checks iSelected against
   its count. The selected index is stored at +0x0c. */

/* No destructor is declared here on purpose. Under /GX a user-declared base
   destructor makes the derived constructor carry an unwind frame, because the
   vector's operator new can throw after the base is built; the canonical body
   has no frame, so the original base's destructor is implicit. */
// SYNTHETIC: WIZ8 0x004f6910
// W8GrowableVector<W8TextControl*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x004f6930
// W8GrowableVector<W8TextControl*>::~W8GrowableVector<W8TextControl*>

// FUNCTION: WIZ8 0x004f5450
W8ControlSelection::W8ControlSelection()
{
    m_value_4 = 0;
    m_value_8 = 0;
    m_selectedIndex = -1;
    m_selectionListener = 0;
}

// FUNCTION: WIZ8 0x004f54b0
int W8ControlSelection::AddEntry(W8TextControl* entry)
{
    entry->AddLayoutFlags(0x19);
    entry->m_listener = this;
    return m_lsButtons.Add(entry);
}

/*
 * Moves the selection. Tells the outgoing entry it lost the selection and the
 * incoming one that it gained it, then reports the change.
 *
 * Both lookups go through the vector's bounds-checked GetAt, which returns the
 * base pointer rather than an offset one when the index is out of range - so an
 * index past the end quietly addresses element zero instead of failing. The
 * assert at the top is the only thing that would have caught it, and it does
 * not return.
 */
// FUNCTION: WIZ8 0x004f5540
void W8ControlSelection::SetSelected(int iSelected)
{
    W8TextControl** entry;
    int previous;

    if (iSelected >= m_lsButtons.GetCount()) {
        srAssertFail("iSelected < m_lsButtons.Length()",
                     "C:\\Projects\\Wizardry 8\\Local Code\\Controls.cpp", 0xa77, 0);
    }
    previous = m_selectedIndex;
    if (previous == iSelected) {
        return;
    }
    if (previous != -1) {
        entry = m_lsButtons.GetAt(previous);
        (*entry)->DisableSecondaryState(0);
    }
    m_selectedIndex = iSelected;
    if (iSelected != -1) {
        entry = m_lsButtons.GetAt(iSelected);
        (*entry)->EnableSecondaryState(0);
    }
    if (m_selectionListener != 0) {
        m_selectionListener->OnSelectionChanged(this, m_selectedIndex);
    }
}

// FUNCTION: WIZ8 0x004f55c0
void W8ControlSelection::OnPrimary(W8TextControl* entry)
{
    int iIndex;

    iIndex = m_lsButtons.IndexOf(entry);
    if (iIndex == -1) {
        srAssertFail("iIndex != -1", "C:\\Projects\\Wizardry 8\\Local Code\\Controls.cpp", 0xa9e,
                     0);
    }
    SetSelected(iIndex);
}
