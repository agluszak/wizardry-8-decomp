#pragma once

class W8DialogBase;
class W8DialogButton;

/* Callbacks stored through Configure and dispatched by the button's own SGP
   callback with the W8DialogButton, not the raw SGP button, as the subject. */
typedef void (*W8DialogButtonCallback)(W8DialogButton* button);

// VTABLE: WIZ8 0x005efa98
class W8DialogButton {
public:
    W8DialogButton();          /* 0x005DB1B0 */
    virtual ~W8DialogButton(); /* 0x005DB260 */
    void Draw();
    void SetPosition(int x, int y);
    int GetWidth();
    int GetHeight();
    int GetX();
    int GetY();
    void SetEnabled(bool enabled);
    bool IsEnabled();
    void SetPressed(bool pressed);
    unsigned char IsPressed();
    void SetVisible(bool visible);
    int GetUserData();
    /* 0x005DB3E0: load the frame images, create the SGP button, and store the
       per-button callbacks, auto-repeat value, priority and tooltip index. */
    unsigned char Configure(const char* image_path, int gray_frame, int off_normal_frame,
                            int off_hover_frame, int on_normal_frame, int on_hover_frame,
                            W8DialogButtonCallback left_callback,
                            W8DialogButtonCallback move_callback, unsigned char flag_36,
                            short priority, int tooltip_index,
                            W8DialogButtonCallback right_callback,
                            W8DialogButtonCallback wheel_callback);

private:
    int unknown_004;
    int unknown_008;
    int unknown_00c;
    int unknown_010;
    int unknown_014;
    int m_image_018;  /* loaded button-image handle */
    int m_button_01c; /* SGP button handle */
    int unknown_020;
    int unknown_024;
    int unknown_028;
    int unknown_02c;
    int unknown_030;
    unsigned char unknown_034;
    bool m_enabled_035;
    unsigned char unknown_036;

public:
    /* 0x37: raised by the dialog factories on their item-row buttons; the SGP
       dispatch callback tests it while handling toggle events. */
    unsigned char unknown_037;
    bool m_dirty; /* 0x38: set by owning screens before Draw */
private:
    unsigned char unknown_039;
    unsigned char unknown_03a;
    unsigned char unknown_03b;
    unsigned char unknown_03c;
    unsigned char unknown_03d[3];

public:
    /* 0x40: set by the dialog factories to the owning dialog; the per-button
       dispatch callback reads it back when a stored callback needs it. */
    W8DialogBase* m_owner_040;

private:
    int unknown_044;
}; /* 0x48 */
