#pragma once

#include "wiz8/local_code/Controls.h"
#include "wiz8/local_code/TextControl.h"

/* Shared declaration owner; implementation remains in Local Code\Controls.cpp. */

class W8RangeControl;

// VTABLE: WIZ8 0x005ed6fc
class W8RangeButton : public W8TextControl {
public:
    W8RangeButton(Controls* panel, unsigned int region,
                          int left, int top, int right, int bottom,
                          int text_40, int text_44, int text_48, int text_4c,
                          int text_54, int text_50, int text_58,
                          short direction, W8RangeControl* range);
    virtual void OnLeftButtonDown(int event) override;
    virtual void ActivatePrimary(int event) override;
    virtual void AdjustValue(int steps) override;

protected:
    short m_direction;                   /* 0xb8: zero decrements */
    unsigned short pad_ba;
    W8RangeControl* m_range;     /* 0xbc */
};
static_assert(sizeof(W8RangeButton) == 0xc0, "W8RangeButton_size");

class W8VerticalRangeThumb;

class W8RangeListener {
public:
    virtual void OnRangeChanged(W8RangeControl* control) = 0;
};

/* The range panel is shared by Controls.cpp and the state-5 party-selection
   controls. Its listener, value and child ownership are therefore part of the
   shared range-control declaration rather than a translation-unit-local sketch. */
class W8RangeControl : public Controls {
public:
    friend class W8VerticalRangeThumb;

    W8RangeControl(int left, int top, int right, int bottom,
                           unsigned int* shared_region_set);
    ~W8RangeControl();

    void SetRange(int first, int second);
    void SetValue(int value);
    void Decrement();
    void Increment();
    void SetRangeEnabled(unsigned char enabled);

    int m_minimum;                       /* 0x4c */
    int m_maximum;                       /* 0x50 */
    int m_value;                         /* 0x54 */
    W8Widget* m_decrement;   /* 0x58 */
    W8Widget* m_increment;   /* 0x5c */
    W8VerticalRangeThumb* m_thumb; /* 0x60 */
    W8RangeListener* m_listener;         /* 0x64 */
    unsigned char m_enabled;             /* 0x68 */
};
static_assert(sizeof(W8RangeControl) == 0x6c, "W8RangeControl_size");
