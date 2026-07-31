#pragma once

#include "wiz8/compat/compiler.h"
#include "wiz8/local_code/Controls.h"
#include "wiz8/engine_code/game_timer.h"

/*
 * Tab pages hosted by lifecycle record 3's screen object. Shared base
 * 0x005AFD90 derives from Controls and adds a two-slot notify secondary at
 * +0x4c; extent is 0x70. SelectOption constructs one of four concrete
 * subclasses and stores them at screen+0x1b0c.
 */

class W8ScreenObject005EF224;

/* Secondary at +0x4c on the page base. Vtable 0x005EF214. */
class W8PageNotify005EF214 {
public:
    virtual void Notify0() = 0;
    virtual void Notify1() = 0;
};
WIZ8_ASSERT_SIZE(W8PageNotify005EF214, 0x4);

class W8PageBase005EF1E4 : public Controls, public W8PageNotify005EF214 {
public:
    W8PageBase005EF1E4(int render_target); /* 0x005AFD90 */
    virtual ~W8PageBase005EF1E4();

    virtual void Invalidate(const W8ControlsRect* rect) override; /* 0x005AFF50 */
    virtual void Redraw() override;                              /* 0x005AFF20 */
    virtual void BringUp(void* payload, void* aux, int mode);    /* 0x005AFF00 */
    virtual void Activate() = 0;                                 /* slot +0x14 */
    virtual void Deactivate() = 0;                               /* slot +0x18 */
    virtual void VMethod07() = 0;
    virtual void VMethod08() = 0;
    virtual void VMethod09(void* event);                         /* slot +0x24 */
    virtual void VMethod10();                                    /* slot +0x28 */
    virtual void Prepare();                                      /* 0x005AFFA0 */

    virtual void Notify0() override;
    virtual void Notify1() override;

    int m_count_50;                          /* 0x50 */
    int m_capacity_54;                       /* 0x54 */
    void** m_entries_58;                     /* 0x58: 5 slots, 0x14 bytes */
    W8ScreenObject005EF224* m_parent_5c;     /* 0x5c */
    void* m_payload_60;                      /* 0x60 */
    void* m_aux_64;                          /* 0x64 */
    int m_mode_68;                           /* 0x68 */
    unsigned char m_flag_6c;                 /* 0x6c */
    unsigned char m_flag_6d;                 /* 0x6d */
    unsigned char pad_6e[2];
};
WIZ8_ASSERT_SIZE(W8PageBase005EF1E4, 0x70);

/* Case 0 — size 0xA0, vtable 0x005EF778. Extra secondaries left as padding
   until their interfaces are triaged; primary overrides are what SelectOption
   dispatches. */
class W8Page005EF778 : public W8PageBase005EF1E4 {
public:
    W8Page005EF778();
    virtual void Activate() override;
    virtual void Deactivate() override;
    virtual void VMethod07() override;
    virtual void VMethod08() override;
    unsigned char unknown_70[0x30];
};
WIZ8_ASSERT_SIZE(W8Page005EF778, 0xa0);

/* Case 1 — size 0x624, vtable 0x005EF664. */
class W8Page005EF664 : public W8PageBase005EF1E4 {
public:
    W8Page005EF664();
    virtual void Activate() override;
    virtual void Deactivate() override;
    virtual void VMethod07() override;
    virtual void VMethod08() override;
    W8Timer005EC0A4 m_timer_70;
    unsigned char unknown_94[0x590];
};
WIZ8_ASSERT_SIZE(W8Page005EF664, 0x624);

/* Case 2 — size 0x78, vtable 0x005EF5C8 / secondary 0x005EF5C0. */
class W8PageExtra005EF5C0 {
public:
    virtual void Extra0() = 0;
    virtual void Extra1() = 0;
};
WIZ8_ASSERT_SIZE(W8PageExtra005EF5C0, 0x4);

class W8Page005EF5C8 : public W8PageBase005EF1E4, public W8PageExtra005EF5C0 {
public:
    W8Page005EF5C8();
    virtual void Activate() override;
    virtual void Deactivate() override;
    virtual void VMethod07() override;
    virtual void VMethod08() override;
    virtual void Extra0() override;
    virtual void Extra1() override;
    unsigned char pad_74[4];
};
WIZ8_ASSERT_SIZE(W8Page005EF5C8, 0x78);

/* Case 3 — size 0x100, vtable 0x005EF57C. */
class W8Page005EF57C : public W8PageBase005EF1E4 {
public:
    W8Page005EF57C();
    virtual void Activate() override;
    virtual void Deactivate() override;
    virtual void VMethod07() override;
    virtual void VMethod08() override;
    W8Timer005EC0A4 m_timer_70;
    unsigned char unknown_94[0x68];
    unsigned char flag_fc;
    unsigned char pad_fd[3];
};
WIZ8_ASSERT_SIZE(W8Page005EF57C, 0x100);

W8Page005EF778* CreatePage005CBA90();
W8Page005EF664* CreatePage005C8DE0();
W8Page005EF5C8* CreatePage005C7CC0();
W8Page005EF57C* CreatePage005C73F0();
