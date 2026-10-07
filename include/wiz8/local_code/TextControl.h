#pragma once

#include "wiz8/local_code/Widget.h"
#include "wiz8/local_code/TextBuffer.h"

/* Shared declaration owner; implementation remains in Local Code\Controls.cpp. */

extern const unsigned int g_W8TextControlStatePressed;
extern const unsigned int g_W8TextControlStateSecondary;
extern const unsigned int g_W8TextControlLayoutToggle;
extern const unsigned int g_W8TextControlLayoutTextBesideImage;
extern const unsigned int g_W8TextControlLayoutImageLeft;
extern const unsigned int g_W8TextControlLayoutStayLatched;
extern const unsigned int g_W8TextControlLayoutLatchedImage;
extern const unsigned int g_W8TextControlLayoutImageAtOrigin;
extern const unsigned int g_W8TextControlSilentHover;

enum W8TextControlInputFlag {
    W8_TEXT_CONTROL_SILENT = 0x20u,
    W8_TEXT_CONTROL_ACTIVATE_WHILE_HELD = 0x100u
};
enum { W8_TEXT_CONTROL_ACTIVATED_WHILE_HELD = 0x04u };

// VTABLE: WIZ8 0x005ed604
class W8TextControl : public W8Widget {
public:
    friend class W8ControlSelection;

    // VTABLE: WIZ8 0x005ed664
    class Listener {
    public:
        virtual void OnPrimary(W8TextControl* control) = 0;
        virtual void OnSecondary(W8TextControl*) {}
    };

    W8TextControl();
    W8TextControl(Controls* panel, unsigned int region, int left, int top, int right, int bottom,
                  int image_object, int image_frame, int normal_sprite, int pressed_sprite,
                  int alternate_normal_sprite, int alternate_pressed_sprite, int disabled_sprite);

    void SetImage(int object, int frame = 0)
    {
        m_imageObject = object;
        m_measured_w = -1;
        m_measured_h = -1;
        m_imageFrame = frame;
        m_normalSprite = 0;
        m_pressedSprite = 0;
    }
    void ClearImage()
    {
        m_imageObject = -1;
        m_measured_w = -1;
        m_measured_h = -1;
        m_imageFrame = -1;
        m_normalSprite = -1;
        m_pressedSprite = -1;
    }

    bool MeasureText();
    void GetTextOrigin(int* px, int* py);
    void Invalidate(bool immediate);
    virtual void SetEnabled(bool enabled) override;
    virtual void Redraw(bool full_redraw) override;
    void SetFlaggedRegionBounds(int left, int top, int right);
    virtual void AddLayoutFlags(unsigned int flags) override;
    virtual void SetAlternateTextEnabled(bool enabled) override;
    void RemoveLayoutFlags(unsigned int flags);
    virtual void EnableSecondaryState(bool immediate);
    virtual void DisableSecondaryState(bool immediate);
    virtual void OnMouseEnter(int event) override;
    virtual void OnMouseLeave(int event) override;
    virtual void OnLeftButtonDown(int event) override;
    virtual void OnRightButtonDown(int event) override;
    virtual void OnLeftButtonUp(int event) override;
    virtual void OnRightButtonUp(int event) override;
    virtual void OnLeftButtonDoubleClick(int event) override;
    virtual void ActivatePrimary(int event) override;
    virtual void ActivateSecondary(int event) override;
    void UpdateTextBounds(int left, int top, int right, int bottom);
    virtual void SetBounds(int left, int top, int right, int bottom) override;
    virtual void SetBoundsFromRect(const W8ControlsRect* bounds) override;

public:
    /* The state-5 controller persists two option bits by directly masking the
       controls' state words.  This is observed storage access, not an accessor
       API inferred for convenience. */
    unsigned int m_stateFlags;   /* paired state masks */
    unsigned int m_layoutFlags;  /* layout and input behavior masks */
    bool m_alternateTextEnabled; /* alternate text-selection flag */
    unsigned char pad_3d[3];
    int m_imageObject;
    int m_imageFrame;
    int m_normalSprite;
    int m_pressedSprite;
    int m_alternatePressedSprite;
    int m_alternateNormalSprite;
    int m_disabledSprite;
    short m_measured_w; /* -1 until measured */
    short m_measured_h;

public:
    W8TextBuffer m_textBuffer; /* complete typed subobject */
    int m_pressedTextOffset;
    /* Several owning panels install their listener immediately after
       construction; the pointer is the shared callback attachment point. */
    Listener* m_listener;

protected:
    void UpdateTextLayout();
    void NotifyPrimaryActivation();
    void NotifySecondaryActivation();
};
static_assert(sizeof(W8TextControl) == 0xb8, "W8TextControl_size");

// VTABLE: WIZ8 0x005ed758
class W8HelpTextControl : public W8TextControl {
public:
    W8HelpTextControl(Controls* panel, unsigned int region, int left, int top, int right,
                      int bottom);

    void SetRegionHelp(const wchar_t* text);
    virtual void OnMouseEnter(int event) override;
    virtual void OnLeftButtonDown(int event) override;
    virtual void OnRightButtonDown(int event) override;
    virtual void OnLeftButtonUp(int event) override;
    virtual void OnRightButtonUp(int event) override;
    virtual void OnLeftButtonDoubleClick(int event) override;

protected:
    wchar_t m_regionHelp[200];
};
static_assert(sizeof(W8HelpTextControl) == 0x248, "W8HelpTextControl_size");

/* Separate panel-owned arrays retain their slots until each control has been
   destroyed. Panel storage and region sets are released by the caller. */
inline void DestroyTextControls(W8TextControl** controls, int count)
{
    for (int index = 0; index < count; ++index) {
        if (controls[index] != 0) {
            delete controls[index];
            controls[index] = 0;
        }
    }
}
