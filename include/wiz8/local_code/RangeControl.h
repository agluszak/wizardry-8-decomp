#pragma once

#include "wiz8/local_code/Controls.h"
#include "wiz8/local_code/TextControl.h"

/* Shared declaration owner; implementation remains in Local Code\Controls.cpp. */

class W8RangeControl;

// VTABLE: WIZ8 0x005ed6fc
class W8RangeButton : public W8TextControl {
public:
    W8RangeButton(Controls* panel, unsigned int region, int left, int top, int right, int bottom,
                  int text_40, int text_44, int text_48, int text_4c, int text_54, int text_50,
                  int text_58, short direction, W8RangeControl* range);
    virtual void OnLeftButtonDown(int event) override;
    virtual void ActivatePrimary(int event) override;
    virtual void AdjustValue(int steps) override;

protected:
    short m_direction; /* 0xb8: zero decrements */
    unsigned short pad_ba;
    W8RangeControl* m_range; /* 0xbc */
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

    W8RangeControl(int left, int top, int right, int bottom, unsigned int* shared_region_set);
    ~W8RangeControl();

    void SetRange(int first, int second);
    void SetValue(int value);
    void Decrement();
    void Increment();
    void SetRangeEnabled(bool enabled);

    int m_minimum;                 /* 0x4c */
    int m_maximum;                 /* 0x50 */
    int m_value;                   /* 0x54 */
    W8Widget* m_decrement;         /* 0x58 */
    W8Widget* m_increment;         /* 0x5c */
    W8VerticalRangeThumb* m_thumb; /* 0x60 */
    W8RangeListener* m_listener;   /* 0x64 */
    bool m_enabled;                /* 0x68: binary enable gating Decrement/Increment */
};
static_assert(sizeof(W8RangeControl) == 0x6c, "W8RangeControl_size");

class W8HorizontalRangeThumb;

class W8HorizontalRangeThumbListener {
public:
    virtual void OnDrag(W8HorizontalRangeThumb* thumb) = 0;
    virtual void OnDragEnd(W8HorizontalRangeThumb* thumb) = 0;
};

// VTABLE: WIZ8 0x005ed66c
class W8HorizontalRangeThumb : public W8Widget {
public:
    virtual ~W8HorizontalRangeThumb() override;
    W8HorizontalRangeThumb(Controls* panel, unsigned int region, int left, int top,
                           int render_arg_0, int render_arg_1, int background_sprite,
                           int normal_thumb_sprite, int hovered_thumb_sprite,
                           int disabled_thumb_sprite);
    virtual void Redraw(int full_redraw) override;
    void UpdatePixelPosition();
    virtual void OnLeftButtonDown(int event) override;
    virtual void OnLeftButtonUp(int event) override;
    virtual void OnMouseEnter(int event) override;
    virtual void OnMouseLeave(int event) override;
    virtual void OnMouseMove(int event) override;

protected:
    int m_renderArg0; /* 0x34 */
    int m_renderArg1;
    int m_backgroundSprite;
    int m_normalThumbSprite;
    int m_hoveredThumbSprite;
    int m_disabledThumbSprite;
    int m_trackLength; /* 0x4c */
    int m_thumbWidth;
    int m_pixelPosition;
    int m_dragCoordinate;
    bool m_hovered;  /* cursor over the thumb */
    bool m_dragging; /* thumb drag in progress */
    unsigned char pad_5e[2];

public:
    /* OptionsScreen.cpp sets these bounds and attaches the listener directly. */
    float m_minimumPosition; /* 0x60 */
    float m_maximumPosition;
    float m_position;
    W8HorizontalRangeThumbListener* m_listener;

protected:
    __forceinline void InvalidateThumb();
    __forceinline void ClampPositionAndInvalidate();
};

static_assert(sizeof(W8HorizontalRangeThumb) == 0x70, "W8HorizontalRangeThumb_size");
