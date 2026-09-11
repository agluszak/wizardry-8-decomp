#pragma once

#include "wiz8/local_code/Widget.h"
#include "wiz8/local_code/TextBuffer.h"

/* Shared declaration owner; implementation remains in Local Code\Controls.cpp. */

extern const unsigned int g_W8TextControlMask005ED56C;
extern const unsigned int g_W8TextControlMask005ED570;
extern const unsigned int g_W8TextControlMask005ED578;
extern const unsigned int g_W8TextControlMask005ED588;
extern const unsigned int g_W8TextControlMask005ED594;

// VTABLE: WIZ8 0x005ed604
class W8TextControl : public W8Widget {
public:
    friend class W8ControlSelection;

    /* Retail 0x005ED664: pure primary callback, default secondary no-op.
       Journal and character-profile listener tables retain that second slot. */
    // VTABLE: WIZ8 0x005ed664
    class Listener {
    public:
        virtual void OnPrimary(W8TextControl* control) = 0;
        virtual void OnSecondary(W8TextControl*) {}
    };

    W8TextControl();
    W8TextControl(Controls* panel, unsigned int region,
                          int left, int top, int right, int bottom,
                          int text_40, int text_44, int text_48, int text_4c,
                          int text_54, int text_50, int text_58);
    virtual ~W8TextControl() override;
    unsigned char MeasureText004F4800();
    void GetTextOrigin(int unused, int* px, int* py);
    void Invalidate(unsigned char immediate);
    virtual void SetEnabled(bool enabled) override;
    virtual void Redraw(int full_redraw) override;
    void SetFlaggedRegionBounds(short left, short top, unsigned short right);
    virtual void AddLayoutFlags(unsigned int flags) override;
    virtual void SetAlternateTextEnabled(unsigned char enabled) override;
    void RemoveLayoutFlags(unsigned int flags);
    virtual void EnableSecondaryState(unsigned char immediate);
    virtual void DisableSecondaryState(unsigned char immediate);
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
    unsigned int m_stateFlags;           /* 0x34: paired state masks */
    unsigned int m_flags_38;             /* 0x38: 0x02 builds layout, 0x04 pins left */
    unsigned char m_alternateTextEnabled;/* 0x3c: alternate text-selection flag */
    unsigned char pad_3d[3];
    int m_imageObject;
    int m_imageFrame;
    int m_normalSprite;
    int m_pressedSprite;
    int m_alternatePressedSprite;
    int m_alternateNormalSprite;
    int m_disabledSprite;
    short m_measured_w;                  /* 0x5c: -1 until measured */
    short m_measured_h;                  /* 0x5e */

public:
    W8TextBuffer m_textBuffer;   /* 0x60: complete typed subobject */
    int m_pressedTextOffset;
    /* Several owning panels install their listener immediately after
       construction; the pointer is the shared callback attachment point. */
    Listener* m_listener;                /* 0xb4 */

protected:
    __forceinline void InvalidateCore(unsigned char immediate);
};
static_assert(sizeof(W8TextControl) == 0xb8, "W8TextControl_size");

// VTABLE: WIZ8 0x005ed758
class W8HelpTextControl : public W8TextControl {
public:
    W8HelpTextControl(Controls* panel, unsigned int region,
                              int left, int top, int right, int bottom);
    void SetRegionHelp(const wchar_t* text);
    virtual void OnMouseEnter(int event) override;
    virtual void OnLeftButtonDown(int event) override;
    virtual void OnRightButtonDown(int event) override;
    virtual void OnLeftButtonUp(int event) override;
    virtual void OnRightButtonUp(int event) override;
    virtual void OnLeftButtonDoubleClick(int event) override;

protected:
    wchar_t m_regionHelp[200];            /* 0xb8 */
};
static_assert(sizeof(W8HelpTextControl) == 0x248, "W8HelpTextControl_size");
