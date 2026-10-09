#pragma once

#include "wiz8/local_code/Controls.h"
#include "wiz8/local_code/TextControl.h"

/* Shared declaration owner; implementation remains in Local Code\Controls.cpp. */

class W8RangeControl;

// VTABLE: WIZ8 0x005ed6fc
class W8RangeButton : public W8TextControl {
public:
    W8RangeButton(Controls* panel, unsigned int region, int left, int top, int right, int bottom,
                  int image_object, int image_frame, int normal_sprite, int pressed_sprite,
                  int alternate_normal_sprite, int alternate_pressed_sprite, int disabled_sprite,
                  short direction, W8RangeControl* range);

    virtual void OnLeftButtonDown(int event) override;
    virtual void ActivatePrimary(int event) override;
    virtual void AdjustValue(int steps) override;

protected:
    short m_direction; /* zero decrements */
    unsigned short pad_ba;
    W8RangeControl* m_range;
};
W8_ABI_ASSERT(sizeof(W8RangeButton) == 0xc0, "W8RangeButton_size");

class W8VerticalRangeThumb;

class W8RangeListener {
public:
    virtual void OnRangeChanged(W8RangeControl* control) = 0;
};

/* Common base of the camp screen's three range listeners (item, spell-realm
   and stats): each owns one W8RangeControl. */
class W8CampRangeListener : public W8RangeListener {
public:
    /* Re-invalidates the range control when the visible pool
       changed and always repaints it. */
    void UpdateRange(bool range_changed);
    W8RangeControl* m_range;
};

// VTABLE: WIZ8 0x005ed74c
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

    void AdjustValue(int steps)
    {
        if (steps > 0) {
            do {
                Decrement();
                --steps;
            } while (steps != 0);
        } else if (steps < 0) {
            steps = -steps;
            do {
                Increment();
                --steps;
            } while (steps != 0);
        }
    }

    int m_minimum;
    int m_maximum;
    int m_value;
    W8Widget* m_decrement;
    W8Widget* m_increment;
    W8VerticalRangeThumb* m_thumb;
    W8RangeListener* m_listener;
    bool m_enabled; /* binary enable gating Decrement/Increment */
};
W8_ABI_ASSERT(sizeof(W8RangeControl) == 0x6c, "W8RangeControl_size");

class W8HorizontalRangeThumb;

class W8HorizontalRangeThumbListener {
public:
    virtual void OnDrag(W8HorizontalRangeThumb* thumb) = 0;
    virtual void OnDragEnd(W8HorizontalRangeThumb* thumb) = 0;
};

// VTABLE: WIZ8 0x005ed66c
class W8HorizontalRangeThumb : public W8Widget {
public:
    W8HorizontalRangeThumb(Controls* panel, unsigned int region, int left, int top,
                           int catalog_object, int catalog_frame, int background_sprite,
                           int normal_thumb_sprite, int hovered_thumb_sprite,
                           int disabled_thumb_sprite);
    virtual void Redraw(bool full_redraw) override;
    void UpdatePixelPosition();
    virtual void OnLeftButtonDown(int event) override;
    virtual void OnLeftButtonUp(int event) override;
    virtual void OnMouseEnter(int event) override;
    virtual void OnMouseLeave(int event) override;
    virtual void OnMouseMove(int event) override;

protected:
    int m_catalogObject;
    int m_catalogFrame;
    int m_backgroundSprite;
    int m_normalThumbSprite;
    int m_hoveredThumbSprite;
    int m_disabledThumbSprite;
    int m_trackLength;
    int m_thumbWidth;
    int m_pixelPosition;
    int m_dragCoordinate;
    bool m_hovered;  /* cursor over the thumb */
    bool m_dragging; /* thumb drag in progress */
    unsigned char pad_5e[2];

public:
    /* OptionsScreen.cpp sets these bounds and attaches the listener directly. */
    float m_minimumPosition;
    float m_maximumPosition;
    float m_position;
    W8HorizontalRangeThumbListener* m_listener;

protected:
    void ClampPositionAndInvalidate();
};

W8_ABI_ASSERT(sizeof(W8HorizontalRangeThumb) == 0x70, "W8HorizontalRangeThumb_size");
