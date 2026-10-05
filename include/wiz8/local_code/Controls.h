#pragma once

#include "wiz8/local_code/ControlsRect.h"
#include "wiz8/local_code/Widget.h"
#include "wiz8/regions.h"
#include "wiz8/vector.h"
#include "input.h"

/* Panel declaration. Widgets and text/range helpers have separate headers;
   their implementation remains in the original Controls.cpp translation unit.
   Controls is a recovered role name, not a proven original class spelling. */

/* The single-space separator wrapped notice lines are re-joined with. */
extern wchar_t g_W8TextSeparator[]; /* 0x0060CC74 */

// VTABLE: WIZ8 0x005ed5b0
// class W8Vector<W8Widget*>

/* The region callback a widget without its own region is given. It answers
   whether the event was consumed; the screen-input dispatcher returns that
   byte to its caller. */
extern unsigned char DispatchControlRegionEvent(const InputAtom* event,
                                                struct W8Region* region); /* 0x004F3140 */

/* The simple button-region policy shared by camp tabs, scrolling and
   movement controls. Repeat presses again; release is unconditional. The
   panel-owned dispatcher above has a different repeat/release policy. */
inline unsigned char DispatchButtonRegionEvent(const InputAtom* event, W8Region* region,
                                               W8Widget* button)
{
    switch (event->usEvent) {
    case LEFT_BUTTON_DOWN:
    case LEFT_BUTTON_REPEAT:
        button->OnLeftButtonDown(0);
        region->flags |= W8_REGION_LEFT_BUTTON_HELD;
        return 1;
    case LEFT_BUTTON_UP:
        button->OnLeftButtonUp(0);
        if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) != 0) {
            region->flags &= ~W8_REGION_LEFT_BUTTON_HELD;
        }
        return 1;
    case MOUSE_POS:
        if ((region->flags & W8_REGION_MOUSE_LEAVE) != 0) {
            button->OnMouseLeave(0);
            return 1;
        }
        if ((region->flags & W8_REGION_MOUSE_ENTER) != 0) {
            button->OnMouseEnter(0);
            return 1;
        }
        break;
    }
    return 0;
}

/* Panel/container, not a W8Widget base. Widgets register their pointers in
   m_controls and retain a back-pointer in m_pPanel. DestroyAllControls performs
   explicit child deletion; the vector itself only owns its pointer storage.
   The accumulated redraw rectangle uses left == -1 for an empty rectangle. */
// VTABLE: WIZ8 0x005ed5a4
struct Controls {
    Controls();
    Controls(int left, int top, int right, int bottom, int catalog_object, int catalog_frame,
             int catalog_image);
    ~Controls();

    virtual void SetEnabled(bool enable);
    virtual void Invalidate(const W8ControlsRect* rect);
    virtual void Redraw();
    /* 0x04 and 0x05 travel together: SetEnabled writes the panel's own state to
       the first and mirrors it into every child's m_active, and the redraw
       requests raise the second. 0x06 is raised on its own by 0x004F2F00. */
    bool m_fEnabled;     /* 0x04 */
    bool m_fDirty;       /* 0x05 */
    bool m_fLayoutDirty; /* 0x06 */
    unsigned char pad_07;
    W8ControlsRect m_bounds;        /* 0x08: widget rectangles are relative to its origin */
    int m_catalogObject;            /* 0x18: -1 skips catalog-image drawing */
    int m_catalogFrame;             /* 0x1c: frame within the catalog object */
    int m_catalogImage;             /* 0x20: subimage within the frame */
    W8ControlsRect m_dirtyRect;     /* 0x24 */
    bool m_fWholeAreaDirty;         /* 0x34: set when a caller passes no rectangle */
    W8Vector<W8Widget*> m_controls; /* 0x38 */
    unsigned int m_uiRegionSetId;   /* 0x48 */

    void EnableRegionSet(bool enable);
    void RemoveControl(W8Widget* control);
    void DestroyAllControls();
    void InvalidateLayout();
    void SetBounds(int left, int top, int right, int bottom);
    void AcquireRegionSet(unsigned int* shared_region_set);

    /* The bounds-checked element read every walker above shares. Out of range
       it answers element zero rather than failing, which is what the canonical
       `p = m_ppControls; if (i < m_nControls) p += i;` compiles from and why
       the guard shows up once per use rather than once per loop. */
    inline W8Widget* ControlAt(int index)
    {
        return *m_controls.GetAt(index);
    }

protected:
    void RedrawChildren(bool full_redraw)
    {
        for (int index = 0; index < m_controls.GetCount(); ++index) {
            if (ControlAt(index)->m_active) {
                ControlAt(index)->Redraw(full_redraw);
            }
        }
        m_fLayoutDirty = false;
    }
    void RedrawControls(bool full_redraw);
};

inline void DestroyControlPanel(Controls*& panel)
{
    if (panel != 0) {
        delete panel;
        panel = 0;
    }
}
