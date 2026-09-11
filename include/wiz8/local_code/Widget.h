#pragma once

#include "wiz8/local_code/ControlsRect.h"

/* Shared declaration owner; implementation remains in Local Code\Controls.cpp. */

/* W8Widget is a recovered role name, not a claim about the original spelling.
   Its 0x34-byte layout and region/panel ownership are established by retail. */
typedef void (*W8ControlCallback)();
struct Controls;
struct W8Region;
struct W8RegionEvent;

// VTABLE: WIZ8 0x005ed5bc
class W8Widget {
public:
    friend struct Controls;

    W8Widget()
        : m_enabled(1), m_active(0), m_dirty(0),
          m_left(0), m_top(0), m_right(0), m_bottom(0), m_region(-1),
          m_pPanel(0), m_primaryActivationCallback(0), m_leftButtonDownCallback(0),
          m_secondaryActivationCallback(0), m_rightButtonDownCallback(0), m_leftDoubleClickCallback(0)
    {
    }
    W8Widget(Controls* owner, unsigned int region,
                         int left, int top, int right, int bottom);

    void SetPanel(Controls* panel);
    void SetRegion(unsigned int region);
    void Invalidate(unsigned char immediate);
    void SetActive(bool active);
    void EnableRegionHelp(int help_text_id);
    void DisableRegionHelp();

    virtual ~W8Widget();

    virtual void SetEnabled(bool enabled);
    /* Slots 2 and 5..17 share the retail ret-4 no-op at 0x005B1BE0.
       These are default hooks, not missing implementations. */
    // FUNCTION: WIZ8 0x005b1be0
    virtual void Redraw(int) {}
    virtual void SetBounds(int left, int top, int right, int bottom);
    virtual void SetBoundsFromRect(const W8ControlsRect* bounds);
    virtual void AddLayoutFlags(unsigned int) {}
    virtual void SetAlternateTextEnabled(unsigned char) {}
    virtual void OnMouseEnter(int) {}
    virtual void OnMouseLeave(int) {}
    virtual void OnMouseMove(int) {}
    virtual void AdjustValue(int) {}
    virtual void OnLeftButtonDown(int) {}
    virtual void OnRightButtonDown(int) {}
    virtual void OnLeftButtonUp(int) {}
    virtual void OnRightButtonUp(int) {}
    virtual void OnLeftButtonDoubleClick(int) {}
    virtual void ActivatePrimary(int) {}
    virtual void ActivateSecondary(int) {}

    /* Read from outside the class by Local Screens\RCSCommon.cpp, which is what
       keeps the three flags reachable rather than protected. */
    bool m_enabled;            /* 0x04: interaction and enabled appearance */
    bool m_active;             /* 0x05: panel participation and region input */
    bool m_dirty;              /* 0x06: pending widget redraw */
    unsigned char pad_007;
    /* 0x08: the widget's rectangle, relative to the owner's origin. The
       constructor adds the origin to all four before handing them to the
       region, which is what makes right and bottom edges rather than a size. */
    int m_left;                          /* 0x08 */
    int m_top;                           /* 0x0c */
    int m_right;                         /* 0x10 */
    int m_bottom;                        /* 0x14 */
    int m_region;                     /* 0x18: handed to DisableRegionInput unless -1 */
    Controls* m_pPanel;                  /* 0x1c: named by Controls.cpp:1849 */
    W8ControlCallback m_primaryActivationCallback; /* 0x20: invoked by text-control activation */
    W8ControlCallback m_leftButtonDownCallback;    /* 0x24 */
    W8ControlCallback m_secondaryActivationCallback;/* 0x28 */
    W8ControlCallback m_rightButtonDownCallback;   /* 0x2c */
    W8ControlCallback m_leftDoubleClickCallback;    /* 0x30 */
};                                       /* 0x34 established */
static_assert(sizeof(W8Widget) == 0x34, "W8Widget_size");
