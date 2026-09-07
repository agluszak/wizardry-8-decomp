#pragma once


// VTABLE: WIZ8 0x005efa98
class W8DialogButton {
public:
    W8DialogButton();            /* 0x005DB1B0 */
    virtual ~W8DialogButton();   /* 0x005DB260 */
    void Draw();
    void SetPosition(int x, int y);
    int GetWidth();
    int GetHeight();
    int GetX();
    int GetY();
    void SetEnabled(unsigned char enabled);
    unsigned char IsEnabled();
    void SetPressed(unsigned char pressed);
    unsigned char IsPressed();
    void SetVisible(unsigned char visible);
    int GetUserData();

private:
    int unknown_004;
    int unknown_008;
    int unknown_00c;
    int unknown_010;
    int unknown_014;
    int m_resource_018;
    int m_resource_01c;
    int unknown_020;
    int unknown_024;
    int unknown_028;
    int unknown_02c;
    int unknown_030;
    unsigned char unknown_034;
    unsigned char unknown_035;
    unsigned char unknown_036;
    unsigned char unknown_037;
    unsigned char unknown_038;
    unsigned char unknown_039;
    unsigned char unknown_03a;
    unsigned char unknown_03b;
    unsigned char unknown_03c;
    unsigned char unknown_03d[3];
    int unknown_040;
    int unknown_044;
};                                      /* 0x48 */
