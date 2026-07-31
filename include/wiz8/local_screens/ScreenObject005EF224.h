#pragma once

#include <stddef.h>

#include "wiz8/compat/compiler.h"
#include "wiz8/local_code/Controls.h"
#include "wiz8/local_screens/ScreenPage005EF1E4.h"

/*
 * Lifecycle record 3's screen object. Constructor 0x005B0040 installs the
 * paired vftables at 0x005EF224 / 0x005EF21C into a 0x1B28-byte block that
 * enter 0x005B1750 allocates.
 *
 * Hierarchy: a size-4 primary interface (11 virtuals) plus
 * W8ControlBase005ED664 as the secondary base at +4. Mode at +8 overlays
 * m_value_4; the active tab at +0xc overlays m_value_8; the header-dirty
 * flag at +0x10 overlays the low byte of m_index_c. Context begins at +0x14.
 */

/* Primary base: vptr only. Slot targets are owned by the complete object. */
class W8ScreenPrimary005EF224 {
public:
    virtual void VMethod00(W8PageBase005EF1E4* page) = 0;
    virtual void VMethod01(int) = 0;
    virtual void VMethod02() = 0;
    virtual void VMethod03() = 0;
    virtual void VMethod04() = 0;
    virtual void VMethod05() = 0;
    virtual void VMethod06() = 0;
    virtual void VMethod07() = 0;
    virtual void VMethod08() = 0;
    virtual void VMethod09() = 0;
    virtual void VMethod10() = 0;
};
WIZ8_ASSERT_SIZE(W8ScreenPrimary005EF224, 0x4);

class W8ScreenObject005EF224 : public W8ScreenPrimary005EF224,
                               public W8ControlBase005ED664 {
public:
    W8ScreenObject005EF224(int mode, void* context); /* 0x005B0040 */
    void SetupWidgets005B0140();                     /* 0x005B0140 */
    void ShowDialog005B1430(wchar_t* text, int a, int b); /* 0x005B1430 */
    void SelectOption005B0D50(int index);            /* 0x005B0D50 */
    void SyncPagePayload005B0F30(int index);         /* 0x005B0F30 */
    void AdvanceOption005B0B50(unsigned char forward); /* 0x005B0B50 */
    void RefreshHeader005B1110();                    /* 0x005B1110 */
    void UpdateDialog005B04B0();                     /* 0x005B04B0 */

    virtual void VMethod00(W8PageBase005EF1E4* page) override;
    virtual void VMethod01(int) override;
    virtual void VMethod02() override;
    virtual void VMethod03() override;
    virtual void VMethod04() override;
    virtual void VMethod05() override;
    virtual void VMethod06() override;
    virtual void VMethod07() override;
    virtual void VMethod08() override;
    virtual void VMethod09() override;
    virtual void VMethod10() override;
    virtual void vslot0(void* arg) override;

    void* context_014; /* 0x14 */
    unsigned char payload_018[0x1862];
    unsigned char unknown_187a[0x272]; /* 0x187a .. 0x1aec */
    unsigned char flag_1aec;
    unsigned char flag_1aed;
    unsigned char flag_1aee;
    unsigned char pad_1aef;
    Controls* controls_1af0;                 /* 0x1af0 */
    W8TextControl005ED604* text_1af4;        /* 0x1af4 */
    W8TextControl005ED604* text_1af8;        /* 0x1af8 */
    W8TextControl005ED604* text_1afc;        /* 0x1afc */
    W8TextControl005ED604* text_1b00;        /* 0x1b00 */
    W8TextControl005ED604* text_1b04;        /* 0x1b04 */
    unsigned char option_flags_1b08[4];
    W8PageBase005EF1E4* page_1b0c[4];
    void* dialog_1b1c;
    unsigned int dialog_arg_1b20;
    unsigned char flag_1b24;
    unsigned char unknown_1b25[3];
};

WIZ8_ASSERT_SIZE(W8ScreenObject005EF224, 0x1b28);
static_assert(sizeof(W8ScreenPrimary005EF224) == 0x4,
              "W8ScreenPrimary005EF224_vptr_only");
static_assert(sizeof(W8ScreenPrimary005EF224) + 4 == 0x08,
              "W8ScreenObject005EF224_mode_at_0x08");
static_assert(offsetof(W8ScreenObject005EF224, context_014) == 0x14,
              "W8ScreenObject005EF224_context_at_0x14");
static_assert(offsetof(W8ScreenObject005EF224, controls_1af0) == 0x1af0,
              "W8ScreenObject005EF224_controls_at_0x1af0");
static_assert(offsetof(W8ScreenObject005EF224, option_flags_1b08) == 0x1b08,
              "W8ScreenObject005EF224_flags_at_0x1b08");
static_assert(offsetof(W8ScreenObject005EF224, page_1b0c) == 0x1b0c,
              "W8ScreenObject005EF224_pages_at_0x1b0c");

extern "C" W8ScreenObject005EF224* g_screen_object_0069c2e8;
extern "C" unsigned char EnterScreen005B1750(void);
extern "C" unsigned char TickScreen005B1840(int leaving);
extern "C" void FinishScreen005B18E0(void);
