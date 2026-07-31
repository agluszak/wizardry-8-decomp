#pragma once

#include <stddef.h>

#include "wiz8/compat/compiler.h"
#include "wiz8/local_code/Controls.h"

/*
 * Lifecycle record 3's screen object. Constructor 0x005B0040 installs the
 * paired vftables at 0x005EF224 / 0x005EF21C into a 0x1B28-byte block that
 * enter 0x005B1750 allocates.
 *
 * Hierarchy: a size-4 primary interface (11 virtuals) plus
 * W8ControlBase005ED664 as the secondary base at +4. Retail writes the
 * secondary base vtable first, then derived fields, then replaces both
 * vptrs — classic MI, not a hand-stored member vptr. Mode at +8 overlays
 * the secondary base's m_value_4; context begins at +0x14 after the 0x10
 * secondary subobject. SetupWidgets stores the secondary subobject into each
 * text control's listener slot, which is why that base carries the one-arg
 * primary callback. The name stays address-qualified until a source path
 * names the class.
 */

/* Primary base: vptr only. Slot targets are owned by the complete object. */
class W8ScreenPrimary005EF224 {
public:
    virtual void VMethod00() = 0;
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

    virtual void VMethod00() override;
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
    unsigned int unknown_1b0c;
    unsigned int unknown_1b10;
    unsigned int unknown_1b14;
    unsigned int unknown_1b18;
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

extern "C" W8ScreenObject005EF224* g_screen_object_0069c2e8;
