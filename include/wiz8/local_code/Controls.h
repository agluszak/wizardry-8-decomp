#ifndef WIZ8_LOCAL_CODE_CONTROLS_H
#define WIZ8_LOCAL_CODE_CONTROLS_H

#include <wchar.h>

#include "wiz8/vector.h"

/*
 * Local Code\\Controls.cpp's shared surface.
 *
 * The text buffer lived in two places before this header existed - the full
 * model here, recovered from Controls.cpp's own bodies, and a size-and-vtable
 * sketch in the dialog code that derives from it. They are one class: the
 * sketch's 0x4c bytes of storage after the vptr are exactly the fields below,
 * so the two agree on the extent and the richer model subsumes the other.
 */

/* Note that this is the same four ints in the same order as W8ScreenRect, and
   the two are almost certainly one type in the original - but nothing recovered
   so far passes a control's rectangle to a screen-rect body or the other way
   round, so the match is recorded rather than acted on. */
struct W8ControlsRect {
    int left;                               /* 0x00 */
    int top;                                /* 0x04 */
    int right;                              /* 0x08 */
    int bottom;                             /* 0x0c */
};

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
    void Function4F39B0(int offset, unsigned char force, int target);
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

public:
    /* State-5's controller raises the alternate-renderer byte directly when
       it changes modes; retain that observed public storage access. */
    unsigned char m_geometryDirty;
    unsigned char m_alternateRenderer;
    unsigned char pad_42[2];
    int m_renderMode;                    /* 0x44: 4 initially */
    int m_fontStateIndex;                /* 0x48: -1 skips the state-table override */
    unsigned char m_flag_4c;
};

/* The widget the file's bodies act on, and the owner that holds them. Local
   Screens\\RCSCommon.cpp reaches the same objects. */
typedef void (*W8ControlCallback)();
struct Controls;
struct W8Region;
struct W8RegionEvent;

// VTABLE: WIZ8 0x005ed5bc
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
    // FUNCTION: WIZ8 0x005b1be0
    virtual void Redraw(int) {}
    virtual void SetBounds(int left, int top, int right, int bottom);
    virtual void SetBoundsFromRect(const W8ControlsRect* bounds);
    virtual void AddLayoutFlags(unsigned int) {}
    virtual void SetAlternateTextEnabled(unsigned char) {}
    virtual void Function4D30(int) {}
    virtual void Function4E00(int) {}
    virtual void FunctionSlot09(int) {}
    virtual void AdjustValue(int) {}
    virtual void Function4F70(int) {}
    virtual void InvokeFocusCallback(int) {}
    virtual void Function50C0(int) {}
    virtual void Function5290(int) {}
    virtual void InvokeBlurCallback(int) {}
    virtual void ActivatePrimary(int) {}
    virtual void ActivateSecondary(int) {}

    /* Read from outside the class by Local Screens\RCSCommon.cpp, which is what
       keeps the three flags reachable rather than protected. */
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
WIZ8_ASSERT_SIZE(W8WidgetBase005ED5BC, 0x34);

// VTABLE: WIZ8 0x005ed604
class W8TextControl005ED604 : public W8WidgetBase005ED5BC {
public:
    friend class W8Control005ED654;

    class Listener {
    public:
        virtual void OnPrimary(W8TextControl005ED604* control) = 0;
        virtual void OnSecondary(W8TextControl005ED604* control) = 0;
    };

    W8TextControl005ED604(Controls* panel, unsigned int region,
                          int left, int top, int right, int bottom,
                          int text_40, int text_44, int text_48, int text_4c,
                          int text_54, int text_50, int text_58);
    virtual ~W8TextControl005ED604() override;
    unsigned char MeasureText004F4800();
    void GetTextOrigin(int unused, int* px, int* py);
    void Invalidate(unsigned char immediate);
    virtual void SetVisible(unsigned char visible) override;
    virtual void Redraw(int full_redraw) override;
    void SetFlaggedRegionBounds(short left, short top, unsigned short right);
    virtual void AddLayoutFlags(unsigned int flags) override;
    virtual void SetAlternateTextEnabled(unsigned char enabled) override;
    void RemoveLayoutFlags(unsigned int flags);
    virtual void EnableSecondaryState(unsigned char immediate);
    virtual void DisableSecondaryState(unsigned char immediate);
    virtual void Function4D30(int event) override;
    virtual void Function4E00(int event) override;
    virtual void Function4F70(int event) override;
    virtual void InvokeFocusCallback(int event) override;
    virtual void Function50C0(int event) override;
    virtual void Function5290(int event) override;
    virtual void InvokeBlurCallback(int event) override;
    virtual void ActivatePrimary(int event) override;
    virtual void ActivateSecondary(int event) override;
    void UpdateTextBounds(int left, int top, int right, int bottom);
    virtual void SetBounds(int left, int top, int right, int bottom) override;
    virtual void SetBoundsFromRect(const W8ControlsRect* bounds) override;

public:
    /* The state-5 controller persists two option bits by directly masking the
       controls' state words.  This is observed storage access, not an accessor
       API inferred for convenience. */
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

public:
    W8TextBuffer005ED5B8 m_textBuffer;   /* 0x60: complete typed subobject */
    int m_field_b0;
    /* Several owning panels install their listener immediately after
       construction; the pointer is the shared callback attachment point. */
    Listener* m_listener;                /* 0xb4 */

protected:
    __forceinline void InvalidateCore(unsigned char immediate);
};
WIZ8_ASSERT_SIZE(W8TextControl005ED604, 0xb8);

// VTABLE: WIZ8 0x005ed758
class W8HelpTextControl005ED758 : public W8TextControl005ED604 {
public:
    W8HelpTextControl005ED758(Controls* panel, unsigned int region,
                              int left, int top, int right, int bottom);
    void SetRegionHelp(const wchar_t* text);
    virtual void Function4D30(int event) override;
    virtual void Function4F70(int event) override;
    virtual void InvokeFocusCallback(int event) override;
    virtual void Function50C0(int event) override;
    virtual void Function5290(int event) override;
    virtual void InvokeBlurCallback(int event) override;

protected:
    wchar_t m_regionHelp[200];            /* 0xb8 */
};
WIZ8_ASSERT_SIZE(W8HelpTextControl005ED758, 0x248);

class W8RangeControl005ED74C;

// VTABLE: WIZ8 0x005ed6fc
class W8RangeButton005ED6FC : public W8TextControl005ED604 {
public:
    W8RangeButton005ED6FC(Controls* panel, unsigned int region,
                          int left, int top, int right, int bottom,
                          int text_40, int text_44, int text_48, int text_4c,
                          int text_54, int text_50, int text_58,
                          short direction, W8RangeControl005ED74C* range);
    virtual void Function4F70(int event) override;
    virtual void ActivatePrimary(int event) override;
    virtual void AdjustValue(int steps) override;

protected:
    short m_direction;                   /* 0xb8: zero decrements */
    unsigned short pad_ba;
    W8RangeControl005ED74C* m_range;     /* 0xbc */
};
WIZ8_ASSERT_SIZE(W8RangeButton005ED6FC, 0xc0);

class W8WidgetBase005ED5BC;

// VTABLE: WIZ8 0x005ed5b0
// class W8GrowableVector<W8WidgetBase005ED5BC*>

/* The region callback a widget without its own region is given. Ghidra has no
   function at 0x004F3140, only a label, so this is a declaration and not a
   claim on the address. */
extern void Function4F3140(const W8RegionEvent* event, struct W8Region* region);

/* The accumulated redraw rectangle a panel hands the compositor. An empty
   rectangle is spelled with left at -1, which is what 0x004F2E50 tests before
   it starts unioning rather than intersecting. */
struct Controls {
    Controls();
    Controls(int left, int top, int right, int bottom,
             int render_target, int render_arg_1c, int render_arg_20);
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
    W8GrowableVector<W8WidgetBase005ED5BC*> m_controls; /* 0x38 */
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

class W8VerticalRangeThumb005ED6B4;

class W8RangeListener {
public:
    virtual void OnRangeChanged(W8RangeControl005ED74C* control) = 0;
};

/* The range panel is shared by Controls.cpp and the state-5 party-selection
   controls. Its listener, value and child ownership are therefore part of the
   common Controls surface rather than a translation-unit-local sketch. */
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
    void SetEnabled(unsigned char enabled) override;

    int m_minimum;                       /* 0x4c */
    int m_maximum;                       /* 0x50 */
    int m_value;                         /* 0x54 */
    W8WidgetBase005ED5BC* m_decrement;   /* 0x58 */
    W8WidgetBase005ED5BC* m_increment;   /* 0x5c */
    W8VerticalRangeThumb005ED6B4* m_thumb; /* 0x60 */
    W8RangeListener* m_listener;         /* 0x64 */
    unsigned char m_enabled;             /* 0x68 */
};
WIZ8_ASSERT_SIZE(W8RangeControl005ED74C, 0x6c);

class W8Control005ED654;

class W8ControlSelectionListener {
public:
    virtual void vslot00(W8Control005ED654* control, int selected) = 0;
};

// VTABLE: WIZ8 0x005ed65c
// class W8GrowableVector<W8TextControl005ED604*>

/* W8Control is itself the ordinary two-slot text-control listener. The same
   W8TextControl pointer is laid out, registered in the vector and dispatched
   for selected/deselected state; there is no parallel vector-element object. */
class W8Control005ED654 : public W8TextControl005ED604::Listener {
public:
    W8Control005ED654();
    int AddEntry(W8TextControl005ED604* entry);
    void SetSelected(int iSelected);
    virtual void OnPrimary(W8TextControl005ED604* entry) override;
    virtual void OnSecondary(W8TextControl005ED604*) override {}

public:
    /* State-5's option owner reads the selected index from its embedded
       W8Control directly.  Keep the recovered storage public rather than
       manufacturing an accessor that is absent from the retail call site. */
    int m_value_4;
    int m_value_8;
    int m_index_c;
    W8GrowableVector<W8TextControl005ED604*> m_lsButtons;
    W8ControlSelectionListener* m_value_20;
};
WIZ8_ASSERT_SIZE(W8Control005ED654, 0x24);

#endif
