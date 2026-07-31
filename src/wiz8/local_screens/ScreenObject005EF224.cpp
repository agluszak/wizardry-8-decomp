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
 * Lifecycle record 3's enter / tick / finish path and the 0x1B28-byte object
 * it constructs. Inheritance is MI: W8ScreenPrimary005EF224 at +0 and
 * W8ControlBase005ED664 at +4. SelectOption switches the four tab pages.
 */

struct W8Character;
struct W8ScreenPoint {
    int x;
    int y;
};

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
extern void RequestScreenTransition(void);
extern void NoOp(void);
extern void ShutdownDisplayList(void);
extern void GetScreenPoint004284F0(W8ScreenPoint* point);
extern void Function40B510(int a, int x, int y, int b, int c);
extern void Function4F1360(int x, int y);
extern unsigned char Function402140(void* event);
extern unsigned char DispatchScreenInput004F1910(const void* event);
extern void Function426790(void);
extern void Function556DC0(void* dst, void* src);
extern void Function556CC0(void* dst, void* src);
extern void Function558180(void* dst, void* src);
extern void Function4EFA30(void* character);
extern void CalcCharacterTableValue(W8Character* character);

extern int g_font_683660;
extern int g_wiz_text_bold_font_683664;
extern unsigned short* g_colour_68ee08;
extern unsigned short* g_font_palette_wiz_text_bold_68ee0c;
extern unsigned char g_in_combat_00683f94;
extern int g_dword_6f04e8;
extern int g_dword_6f04ed;

// GLOBAL: WIZ8 0x0069C2E4
unsigned int g_screen_object_region_set_0069c2e4;

// GLOBAL: WIZ8 0x0069C2E8
W8ScreenObject005EF224* g_screen_object_0069c2e8;

}

// VTABLE: WIZ8 0x005ef224 W8ScreenObject005EF224
// VTABLE: WIZ8 0x005ef21c W8ControlBase005ED664
// class W8ScreenObject005EF224

void W8ScreenObject005EF224::VMethod00(W8PageBase005EF1E4*) {}
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

/* Text-control listener primary callback (secondary vtable slot 0).
 * Retail this points at the secondary subobject (+4); the body below is the
 * complete-object override MSVC reaches through an adjustor thunk. */
// FUNCTION: WIZ8 0x005b0a40
void W8ScreenObject005EF224::vslot0(void* arg)
{
    W8TextControl005ED604* control =
        static_cast<W8TextControl005ED604*>(arg);
    int index;

    if (control != text_1b00) {
        if (control == text_1afc) {
            if (m_value_8 >= 0 && page_1b0c[m_value_8] != 0) {
                page_1b0c[m_value_8]->VMethod07();
            }
            return;
        }
        if (control == text_1af4) {
            index = m_value_8 - 1;
            if (index >= 0) {
                while (option_flags_1b08[index] == 0) {
                    index = index - 1;
                    if (index < 0) {
                        return;
                    }
                }
                if (index != -1) {
                    SelectOption005B0D50(index);
                }
            }
            return;
        }
        if (control == text_1af8) {
            AdvanceOption005B0B50(0);
            return;
        }
        if (control == text_1b04) {
            option_flags_1b08[0] = 1;
            option_flags_1b08[1] = 1;
            option_flags_1b08[2] = 1;
            option_flags_1b08[3] = 1;
            flag_1aee = 1;
            Function556DC0(payload_018, unknown_187a);
            m_value_4 = 0;
            if (page_1b0c[3] != 0) {
                /* Retail clears a field at page+0x68 (mode). */
                page_1b0c[3]->m_mode_68 = 0;
            }
            text_1b04->SetEnabled(0);
            SelectOption005B0D50(0);
        }
        return;
    }

    if (m_value_4 == 1 && text_1afc->m_flag_4 == 0) {
        RequestScreenTransition();
        return;
    }
    ShowDialog005B1430(gppStringList[0x34c / 4], 1, 2);
}

/* Stubs called from finish / vslot0; deepened only as far as the compare
   bundle needs for control flow. */
void W8ScreenObject005EF224::AdvanceOption005B0B50(unsigned char)
{
}

void W8ScreenObject005EF224::RefreshHeader005B1110()
{
}

void W8ScreenObject005EF224::UpdateDialog005B04B0()
{
}

/*
 * Syncs mode/context into the page payload before bring-up. Branches follow
 * the retail index/mode tests; callees stay extern until owned.
 */
// FUNCTION: WIZ8 0x005b0f30
void W8ScreenObject005EF224::SyncPagePayload005B0F30(int index)
{
    if (m_value_8 > index) {
        return;
    }
    if (index == 0) {
        if (m_value_4 == 0) {
            Function556DC0(payload_018, unknown_187a);
            return;
        }
        if (m_value_4 == 2) {
            Function556CC0(payload_018, unknown_187a);
        }
        return;
    }
    if (index == 1) {
        Function558180(payload_018, unknown_187a);
        return;
    }
    if (index == 3) {
        if (*reinterpret_cast<int*>(payload_018 + 0x81) < 0) {
            Function4EFA30(payload_018);
        }
        if (*reinterpret_cast<int*>(payload_018 + 0x79) < 0) {
            CalcCharacterTableValue(
                reinterpret_cast<W8Character*>(payload_018));
        }
    }
}

/* Option-page switcher invoked once the four tab flags are known. */
// FUNCTION: WIZ8 0x005b0d50
void W8ScreenObject005EF224::SelectOption005B0D50(int index)
{
    W8PageBase005EF1E4* page;
    int scan;
    int help_id;

    if (m_value_8 >= 0) {
        page = page_1b0c[m_value_8];
        if (page != 0) {
            page->SetEnabled(0);
            page->Deactivate();
        }
    }

    SyncPagePayload005B0F30(index);

    if (page_1b0c[index] == 0) {
        page = 0;
        switch (index) {
        case 0:
            page = CreatePage005CBA90();
            break;
        case 1:
            page = CreatePage005C8DE0();
            break;
        case 2:
            page = CreatePage005C7CC0();
            break;
        case 3:
            page = CreatePage005C73F0();
            break;
        }
        if (page != 0) {
            page->m_parent_5c = this;
            page->BringUp(payload_018, unknown_187a, m_value_4);
            page_1b0c[index] = page;
        }
    }

    m_value_8 = index;
    page = page_1b0c[index];
    if (page != 0) {
        page->Activate();
        page->SetEnabled(1);
    }

    scan = index;
    do {
        scan = scan - 1;
        if (scan < 0) {
            scan = -1;
            break;
        }
    } while (option_flags_1b08[scan] == 0);
    text_1af4->SetEnabled(static_cast<unsigned char>(scan != -1));

    scan = index;
    do {
        scan = scan + 1;
        if (scan > 3) {
            scan = -1;
            break;
        }
    } while (option_flags_1b08[scan] == 0);

    if (scan == -1) {
        text_1af8->SetButtonArt(0x10, 0x12, 0x11, 0x12, 0x13);
        help_id = 0xe0;
    }
    else {
        text_1af8->SetButtonArt(8, 10, 9, 10, 0xb);
        help_id = 0xdc;
    }
    text_1af8->EnableRegionHelp(help_id);
    controls_1af0->SetEnabled(0);
    if (page != 0) {
        page->Prepare();
    }
    m_index_c = 1;
    VMethod00(page);
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

    m_value_8 = -1;
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
    int index;

    m_value_4 = mode;
    context_014 = context;
    m_index_c = 0;
    flag_1aec = 0;
    flag_1aed = 0;
    flag_1aee = 0;
    dialog_1b1c = 0;
    flag_1b24 = 0;

    if (context != 0) {
        memcpy(payload_018, context, 0x1862);
        payload_018[4] = 0;
    }

    for (index = 0; index < 4; ++index) {
        page_1b0c[index] = 0;
    }

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

/* Leave / tick for record 3. On the leaving pass, tear down pages and the
   screen object; always then reset the display list and regions. */
// FUNCTION: WIZ8 0x005b1840
extern "C" unsigned char TickScreen005B1840(int leaving)
{
    W8ScreenObject005EF224* object;
    W8PageBase005EF1E4* page;
    int index;
    Controls* controls;

    object = g_screen_object_0069c2e8;
    if (leaving != 0) {
        if (object != 0) {
            if (object->TabIndex() >= 0) {
                page = object->page_1b0c[object->TabIndex()];
                if (page != 0) {
                    page->Deactivate();
                }
            }
            for (index = 0; index < 4; ++index) {
                page = object->page_1b0c[index];
                if (page != 0) {
                    delete page;
                    object->page_1b0c[index] = 0;
                }
            }
            controls = object->controls_1af0;
            if (controls != 0) {
                controls->DestroyAllControls();
                controls->~Controls();
                ::operator delete(controls);
            }
            ::operator delete(object);
        }
        g_screen_object_0069c2e8 = 0;
    }
    NoOp();
    ShutdownDisplayList();
    ResetRegions();
    return 1;
}

/* Per-frame finish for record 3. Drains input; page Redraw / Handle path on
   the idle side; key cases forward to SelectOption / dialogs. */
// FUNCTION: WIZ8 0x005b18e0
extern "C" void FinishScreen005B18E0(void)
{
    W8ScreenPoint point;
    unsigned char event[0x10];
    unsigned char status;
    W8ScreenObject005EF224* object;
    short* event_words;
    int index;

    GetScreenPoint004284F0(&point);
    object = g_screen_object_0069c2e8;
    if (object != 0) {
        object->UpdateDialog005B04B0();
    }
    Function40B510(0x400, point.x, point.y, g_dword_6f04ed, g_dword_6f04e8);
    Function4F1360(point.x, point.y);
    status = Function402140(event);
    object = g_screen_object_0069c2e8;
    event_words = reinterpret_cast<short*>(event + 6);

    for (;;) {
        if (status != 1) {
            if (object != 0) {
                if (object->HeaderDirty() != 0) {
                    object->RefreshHeader005B1110();
                }
                if (object->TabIndex() >= 0 &&
                    object->page_1b0c[object->TabIndex()] != 0) {
                    object->page_1b0c[object->TabIndex()]->Redraw();
                }
                if (object->controls_1af0 != 0) {
                    object->controls_1af0->Redraw();
                }
                if (object->dialog_1b1c != 0) {
                    /* Dialog virtual at +0x0c — kept as an opaque call. */
                    (*reinterpret_cast<void (***)(void*)>(object->dialog_1b1c))[3](
                        object->dialog_1b1c);
                }
            }
            Function426790();
            return;
        }

        if (object != 0 && object->TabIndex() >= 0 &&
            object->page_1b0c[object->TabIndex()] != 0) {
            object->page_1b0c[object->TabIndex()]->VMethod09(event);
        }
        status = DispatchScreenInput004F1910(event);
        if (status == 0 && event_words[0] == 1) {
            switch (event_words[1] | (event_words[2] << 16)) {
            case 0xd:
            case 0x27:
            case 0x4e:
                if (object != 0) {
                    object->AdvanceOption005B0B50(1);
                }
                break;
            case 0x1b:
                if (object != 0) {
                    if (object->ModeValue() == 1 &&
                        object->text_1afc->m_flag_4 == 0) {
                        RequestScreenTransition();
                    }
                    else {
                        object->ShowDialog005B1430(
                            gppStringList[0x34c / 4], 1, 2);
                    }
                }
                break;
            case 0x25:
            case 0x42:
                if (object != 0) {
                    index = object->TabIndex();
                    if (index != 3) {
                        do {
                            index = index - 1;
                            if (index < 0) {
                                break;
                            }
                        } while (object->option_flags_1b08[index] == 0);
                        if (index != -1) {
                            object->SelectOption005B0D50(index);
                        }
                    }
                }
                break;
            }
        }
        status = Function402140(event);
        object = g_screen_object_0069c2e8;
    }
}
