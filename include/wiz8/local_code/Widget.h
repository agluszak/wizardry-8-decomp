#pragma once

#include "wiz8/local_code/ControlsRect.h"

/* Shared declaration owner; implementation remains in Local Code\Controls.cpp. */

typedef void (*W8ControlCallback)();
struct Controls;
struct W8Region;

// VTABLE: WIZ8 0x005ed5bc
class W8Widget {
public:
    friend struct Controls;

    W8Widget()
        : m_enabled(1), m_active(0), m_dirty(0), m_left(0), m_top(0), m_right(0), m_bottom(0),
          m_region(-1), m_pPanel(0), m_primaryActivationCallback(0), m_leftButtonDownCallback(0),
          m_secondaryActivationCallback(0), m_rightButtonDownCallback(0),
          m_leftDoubleClickCallback(0)
    {
    }
    W8Widget(Controls* owner, unsigned int region, int left, int top, int right, int bottom);

    void SetPanel(Controls* panel);
    void SetRegion(unsigned int region);
    void Invalidate(bool immediate);
    void SetActive(bool active);
    void EnableRegionHelp(int help_text_id);
    void DisableRegionHelp();

    virtual ~W8Widget();

    virtual void SetEnabled(bool enabled);
    /* Default hooks; Redraw's flag is the panel's dirty state. */
    // FUNCTION: WIZ8 0x005b1be0
    virtual void Redraw(bool) {}
    virtual void SetBounds(int left, int top, int right, int bottom);
    virtual void SetBoundsFromRect(const W8ControlsRect* bounds);
    virtual void AddLayoutFlags(unsigned int) {}
    virtual void SetAlternateTextEnabled(bool) {}
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

protected:
    void UpdateRegionBounds(int left, int top, int right, int bottom);

public:
    /* Read from outside the class by Local Screens\RCSCommon.cpp, which is what
       keeps the three flags reachable rather than protected. */
    bool m_enabled; /* interaction and enabled appearance */
    bool m_active;  /* panel participation and region input */
    bool m_dirty;   /* pending widget redraw */
    unsigned char pad_007;
    /* The widget's rectangle, relative to the owner's origin. The
       constructor adds the origin to all four before handing them to the
       region, which is what makes right and bottom edges rather than a size. */
    int m_left;
    int m_top;
    int m_right;
    int m_bottom;
    int m_region; /* handed to DisableRegionInput unless -1 */
    Controls* m_pPanel;
    W8ControlCallback m_primaryActivationCallback; /* invoked by text-control activation */
    W8ControlCallback m_leftButtonDownCallback;
    W8ControlCallback m_secondaryActivationCallback;
    W8ControlCallback m_rightButtonDownCallback;
    W8ControlCallback m_leftDoubleClickCallback;
};
static_assert(sizeof(W8Widget) == 0x34, "W8Widget_size");
