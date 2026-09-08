#pragma once

#include "wiz8/local_code/ControlsRect.h"
#include "wiz8/vector.h"

/* Panel declaration. Widgets and text/range helpers have separate headers;
   their implementation remains in the original Controls.cpp translation unit.
   Controls is a recovered role name, not a proven original class spelling. */

struct W8Region;
struct W8RegionEvent;

class W8Widget;

// VTABLE: WIZ8 0x005ed5b0
// class W8GrowableVector<W8Widget*>

/* The region callback a widget without its own region is given. It answers
   whether the event was consumed; the screen-input dispatcher returns that
   byte to its caller. */
extern unsigned char DispatchControlRegionEvent(
    const W8RegionEvent* event, struct W8Region* region); /* 0x004F3140 */

/* Panel/container, not a W8Widget base. Widgets register their pointers in
   m_controls and retain a back-pointer in m_pPanel. DestroyAllControls performs
   explicit child deletion; the vector itself only owns its pointer storage.
   The accumulated redraw rectangle uses left == -1 for an empty rectangle. */
struct Controls {
    Controls();
    Controls(int left, int top, int right, int bottom,
             int render_target, int render_arg_1c, int render_arg_20);
    __forceinline ~Controls();

    virtual void SetEnabled(unsigned char enable);
    virtual void Invalidate(const W8ControlsRect* rect);
    virtual void Redraw();
    /* 0x04 and 0x05 travel together: SetEnabled writes the panel's own state to
       the first and mirrors it into every child's m_active, and the redraw
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
    W8GrowableVector<W8Widget*> m_controls; /* 0x38 */
    unsigned int m_uiRegionSetId;           /* 0x48 */

    void EnableRegionSet(unsigned char enable);
    void RemoveControl(W8Widget* control);
    void DestroyAllControls();
    void InvalidateLayout();
    void SetBounds(int left, int top, int right, int bottom);
    void AcquireRegionSet(unsigned int* shared_region_set);

    /* The bounds-checked element read every walker above shares. Out of range
       it answers element zero rather than failing, which is what the canonical
       `p = m_ppControls; if (i < m_nControls) p += i;` compiles from and why
       the guard shows up once per use rather than once per loop. */
    __inline W8Widget* ControlAt(int index)
    {
        return *m_controls.GetAt(index);
    }
};

extern int g_W8TextClipTarget005FF5F4;
extern int g_W8TextClipFlags00650E38;
extern float g_W8RangeEnd005EBB38;
extern float g_W8RangeHalfStep005EBC7C;
