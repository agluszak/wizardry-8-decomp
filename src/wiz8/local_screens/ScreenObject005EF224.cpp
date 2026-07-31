#include "wiz8/local_screens/ScreenObject005EF224.h"

#include "wiz8/dialog_base.h"
#include "wiz8/game_state.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/render_state.h"
#include "wiz8/screen_state.h"
#include "Font.h"

#include <new>
#include <string.h>
#include <wchar.h>

/*
 * Lifecycle record 3's enter path and the 0x1B28-byte object it constructs.
 * The GainLevel.wav branch and the Menus.MPL bring-up place this near the
 * character-review / level-up family; the class keeps an address-qualified
 * name until a source path names it.
 *
 * Inheritance is MI: W8ScreenPrimary005EF224 at +0 and W8ControlBase005ED664
 * at +4. The compiler installs 0x005EF224 / 0x005EF21C; do not store vptrs by
 * hand. SetupWidgets wires a Controls panel and five text buttons, pointing
 * each button's listener at the secondary base.
 */

struct W8Character;

extern "C" {

extern void SetViewport(int left, int top, int right, int bottom);
extern void Function40B290(void);
extern void ResetRegions(void);
extern void UpdateHeldItemCursor(void);
extern unsigned char Function48FC10(const char* playlist, int immediate, int replace);
extern void PlaySound(const char* path, int flags);
extern wchar_t* FormatWideString(const wchar_t* format, ...);
extern unsigned int CharacterPointerToPartySlot(W8Character* character);
extern void ActivateDialogRegion(int region_set); /* 0x004F2040 */

extern int g_font_683660;
extern int g_wiz_text_bold_font_683664;
extern unsigned short* g_colour_68ee08;
extern unsigned short* g_font_palette_wiz_text_bold_68ee0c;
extern unsigned char g_in_combat_00683f94;

// GLOBAL: WIZ8 0x0069C2E4
unsigned int g_screen_object_region_set_0069c2e4;

// GLOBAL: WIZ8 0x0069C2E8
W8ScreenObject005EF224* g_screen_object_0069c2e8;

}

// VTABLE: WIZ8 0x005ef224 W8ScreenObject005EF224
// VTABLE: WIZ8 0x005ef21c W8ControlBase005ED664
// class W8ScreenObject005EF224

void W8ScreenObject005EF224::VMethod00() {}
void W8ScreenObject005EF224::VMethod01(int) {}
void W8ScreenObject005EF224::VMethod02() {}
void W8ScreenObject005EF224::VMethod03() {}
void W8ScreenObject005EF224::VMethod04() {}
void W8ScreenObject005EF224::VMethod05() {}
void W8ScreenObject005EF224::VMethod06() {}
void W8ScreenObject005EF224::VMethod07() {}
void W8ScreenObject005EF224::VMethod08() {}
void W8ScreenObject005EF224::VMethod09() {}
void W8ScreenObject005EF224::VMethod10() {}

/* Text-control listener primary callback (secondary vtable slot 0). */
void W8ScreenObject005EF224::vslot0(void*)
{
}

/* Option-page switcher invoked once the four tab flags are known. */
void W8ScreenObject005EF224::SelectOption005B0D50(int)
{
}

/*
 * Builds the dialog that the GainLevel path raises. Retail thiscall on the
 * screen object; the free-function spelling was wrong.
 */
// FUNCTION: WIZ8 0x005b1430
void W8ScreenObject005EF224::ShowDialog005B1430(wchar_t* text, int a, int b)
{
    void* memory;
    W8DialogBase005D25B0* dialog;

    memory = ::operator new(0x98);
    if (memory == 0) {
        dialog = 0;
    }
    else {
        dialog = new (memory) W8DialogBase005D25B0();
    }

    dialog_arg_1b20 = static_cast<unsigned int>(b);
    dialog_1b1c = dialog;
    if (dialog != 0) {
        /* Extent / origin / background / message virtuals still need their
           real signatures on W8DialogBase005DC7A0; ActivateDialogRegion is
           the one recovered callee this body already owns. */
        (void)text;
        (void)a;
        ActivateDialogRegion(0x138);
        flag_1b24 = 1;
    }
}

/* Setup that wires the Controls panel and five text-widget children. */
// FUNCTION: WIZ8 0x005b0140
void W8ScreenObject005EF224::SetupWidgets005B0140()
{
    void* memory;
    W8TextControl005ED604::Listener* listener;
    int option;
    unsigned int party_slot;

    memory = ::operator new(0x4c);
    if (memory == 0) {
        controls_1af0 = 0;
    }
    else {
        controls_1af0 = new (memory) Controls(0, 0x1c2, 0, 0, 0x107, 0, 4);
    }
    controls_1af0->AcquireRegionSet(&g_screen_object_region_set_0069c2e4);

    listener = reinterpret_cast<W8TextControl005ED604::Listener*>(
        static_cast<W8ControlBase005ED664*>(this));

    memory = ::operator new(0xb8);
    if (memory == 0) {
        text_1af8 = 0;
    }
    else {
        text_1af8 = new (memory) W8TextControl005ED604(
            controls_1af0, 0xffffffff, 0x254, 0, 0, 0, 0x106, 0, 8, 10, 9, 10,
            0xb);
    }
    text_1af8->EnableRegionHelp(0xdc);
    text_1af8->SetListener(listener);

    memory = ::operator new(0xb8);
    if (memory == 0) {
        text_1af4 = 0;
    }
    else {
        text_1af4 = new (memory) W8TextControl005ED604(
            controls_1af0, 0xffffffff, 0x228, 0, 0, 0, 0x106, 0, 0xc, 0xe, 0xd,
            0xe, 0xf);
    }
    text_1af4->EnableRegionHelp(0xdd);
    text_1af4->SetListener(listener);

    memory = ::operator new(0xb8);
    if (memory == 0) {
        text_1afc = 0;
    }
    else {
        text_1afc = new (memory) W8TextControl005ED604(
            controls_1af0, 0xffffffff, 0x1fc, 0, 0, 0, 0x106, 0, 0x14, 0x16,
            0x15, 0x16, 0x17);
    }
    text_1afc->EnableRegionHelp(0xde);
    text_1afc->SetListener(listener);

    memory = ::operator new(0xb8);
    if (memory == 0) {
        text_1b00 = 0;
    }
    else {
        text_1b00 = new (memory) W8TextControl005ED604(
            controls_1af0, 0xffffffff, 0x1d0, 0, 0, 0, 0x106, 0, 4, 6, 5, 6, 7);
    }
    text_1b00->EnableRegionHelp(0xdf);
    text_1b00->SetListener(listener);

    memory = ::operator new(0xb8);
    if (memory == 0) {
        text_1b04 = 0;
    }
    else {
        text_1b04 = new (memory) W8TextControl005ED604(
            controls_1af0, 0xffffffff, 0, 0, 0, 0, 0x106, 0, 0x1d, 0x1f, 0x1e,
            0x1f, 0x20);
    }
    text_1b04->EnableRegionHelp(0xe1);
    text_1b04->SetListener(listener);

    controls_1af0->SetEnabled(1);
    controls_1af0->EnableRegionSet(1);
    controls_1af0->Invalidate(0);
    text_1b04->SetEnabled(0);

    if (m_value_4 == 1 && g_status_685170.unknown_000c[0] != 0) {
        party_slot = CharacterPointerToPartySlot(
            reinterpret_cast<W8Character*>(context_014));
        if (party_slot > 1) {
            text_1b04->SetEnabled(1);
            if (g_in_combat_00683f94 != 0) {
                text_1b04->SetVisible(0);
            }
        }
    }

    m_index_c = -1;
    option = 0;
    do {
        if (option_flags_1b08[option] != 0) {
            SelectOption005B0D50(option);
            return;
        }
        option = option + 1;
    } while (option < 4);
    SelectOption005B0D50(-1);
}

// FUNCTION: WIZ8 0x005b0040
W8ScreenObject005EF224::W8ScreenObject005EF224(int mode, void* context)
{
    m_value_4 = mode;
    context_014 = context;
    flag_1aec = 0;
    flag_1aed = 0;
    flag_1aee = 0;
    dialog_1b1c = 0;
    flag_1b24 = 0;

    if (context != 0) {
        memcpy(payload_018, context, 0x1862);
        payload_018[4] = 0;
    }

    unknown_1b0c = 0;
    unknown_1b10 = 0;
    unknown_1b14 = 0;
    unknown_1b18 = 0;

    if (m_value_4 == 3) {
        m_value_4 = 2;
        PlaySound("Data\\Sound\\Misc\\GainLevel.wav", 0);
        ShowDialog005B1430(
            FormatWideString(
                gppStringList[0x364 / 4],
                reinterpret_cast<char*>(this) + 0x1d,
                0,
                0),
            0,
            0);
    }

    option_flags_1b08[0] = static_cast<unsigned char>(m_value_4 != 1);
    option_flags_1b08[1] = static_cast<unsigned char>(m_value_4 != 1);
    option_flags_1b08[2] = static_cast<unsigned char>(m_value_4 != 1);
    option_flags_1b08[3] = static_cast<unsigned char>(m_value_4 != 2);
}

/* Lifecycle record 3 enter. Shares the viewport / region / font-palette
   prelude with the other screen enters, then builds the 0x1B28 object. */
// FUNCTION: WIZ8 0x005b1750
extern "C" unsigned char EnterScreen005B1750(void)
{
    void* memory;

    SetViewport(0, 0, 0x280, 0x1e0);
    Function425570(0);
    Function40B290();
    ResetRegions();
    UpdateHeldItemCursor();
    SetFontObjectPalette16BPP(g_font_683660, g_colour_68ee08);
    SetFontObjectPalette16BPP(
        g_wiz_text_bold_font_683664, g_font_palette_wiz_text_bold_68ee0c);

    memory = ::operator new(0x1b28);
    if (memory == 0) {
        g_screen_object_0069c2e8 = 0;
    }
    else {
        g_screen_object_0069c2e8 = new (memory) W8ScreenObject005EF224(
            g_screen_state_0068ec78.state.mode,
            g_screen_state_0068ec78.state.parameter_3);
    }

    /* Retail always passes the allocation, including null; Setup itself null-
       checks the receiver in the branches that need it. */
    if (g_screen_object_0069c2e8 != 0) {
        g_screen_object_0069c2e8->SetupWidgets005B0140();
    }

    if (g_status_685170.unknown_000c[0] == 0 &&
        (g_screen_state_0068ec78.state.mode == 0 ||
         g_screen_state_0068ec78.state.mode == 2)) {
        Function48FC10("Menus.MPL", 1, 1);
    }

    return 1;
}
