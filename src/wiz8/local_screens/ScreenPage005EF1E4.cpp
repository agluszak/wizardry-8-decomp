#include "wiz8/local_screens/ScreenPage005EF1E4.h"

#include <new>
#include <string.h>

/*
 * Page base and the four tab-page factories SelectOption constructs. Bodies
 * past BringUp / Prepare / Invalidate / Redraw stay stubs until a caller in
 * this slice needs them; factories exist so the screen object can `new` typed
 * pages without inventing parallel inventories.
 */

// SYNTHETIC: WIZ8 0x005afe20
// W8PageBase005EF1E4::`scalar deleting destructor'

// VTABLE: WIZ8 0x005ef1e4 W8PageBase005EF1E4
// VTABLE: WIZ8 0x005ef214 W8PageNotify005EF214
// class W8PageBase005EF1E4

// FUNCTION: WIZ8 0x005afd90
W8PageBase005EF1E4::W8PageBase005EF1E4(int render_target)
    : Controls(0xc3, 0x2b, 0x280, 0x1c1, render_target, 0, 0)
{
    void* memory;

    m_count_50 = 0;
    memory = ::operator new(0x14);
    m_entries_58 = static_cast<void**>(memory);
    if (memory == 0) {
        m_capacity_54 = 0;
    }
    else {
        m_capacity_54 = 5;
        memset(memory, 0, 0x14);
    }
    m_parent_5c = 0;
    m_payload_60 = 0;
    m_aux_64 = 0;
    m_mode_68 = 0;
    m_flag_6c = 0;
    m_flag_6d = 0;
}

// FUNCTION: WIZ8 0x005afe40
W8PageBase005EF1E4::~W8PageBase005EF1E4()
{
    if (m_entries_58 != 0) {
        ::operator delete(m_entries_58);
        m_entries_58 = 0;
    }
}

// FUNCTION: WIZ8 0x005aff00
void W8PageBase005EF1E4::BringUp(void* payload, void* aux, int mode)
{
    m_payload_60 = payload;
    m_aux_64 = aux;
    m_mode_68 = mode;
}

// FUNCTION: WIZ8 0x005aff50
void W8PageBase005EF1E4::Invalidate(const W8ControlsRect* rect)
{
    int index;
    int* entry;

    Controls::Invalidate(rect);
    index = 0;
    if (m_count_50 > 0) {
        do {
            entry = reinterpret_cast<int*>(
                reinterpret_cast<char*>(m_entries_58) + index * 4);
            /* Entry layout still opaque; derived pages own the pointees. */
            (void)entry;
            index = index + 1;
        } while (index < m_count_50);
    }
}

// FUNCTION: WIZ8 0x005aff20
void W8PageBase005EF1E4::Redraw()
{
    int index;

    Controls::Redraw();
    index = 0;
    if (m_count_50 > 0) {
        do {
            index = index + 1;
        } while (index < m_count_50);
    }
}

// FUNCTION: WIZ8 0x005affa0
void W8PageBase005EF1E4::Prepare()
{
    Invalidate(0);
    m_flag_6d = 1;
    m_flag_6c = 1;
}

void W8PageBase005EF1E4::VMethod09(void*) {}
void W8PageBase005EF1E4::VMethod10() {}

/* Shared empty notify bodies also used as page-secondary slots. */
// FUNCTION: WIZ8 0x005b1bc0
void W8PageBase005EF1E4::Notify0() {}

// FUNCTION: WIZ8 0x005b1b90
void W8PageBase005EF1E4::Notify1() {}

// VTABLE: WIZ8 0x005ef778 W8Page005EF778
// class W8Page005EF778

W8Page005EF778::W8Page005EF778()
    : W8PageBase005EF1E4(0x104)
{
    memset(unknown_70, 0, sizeof(unknown_70));
}

void W8Page005EF778::Activate() {}
void W8Page005EF778::Deactivate() {}
void W8Page005EF778::VMethod07() {}
void W8Page005EF778::VMethod08() {}

// FUNCTION: WIZ8 0x005cba90
W8Page005EF778* CreatePage005CBA90()
{
    void* memory;

    memory = ::operator new(0xa0);
    if (memory == 0) {
        return 0;
    }
    return new (memory) W8Page005EF778();
}

// VTABLE: WIZ8 0x005ef664 W8Page005EF664
// class W8Page005EF664

W8Page005EF664::W8Page005EF664()
    : W8PageBase005EF1E4(0x109), m_timer_70(0.05f, 1)
{
    memset(unknown_94, 0, sizeof(unknown_94));
}

void W8Page005EF664::Activate() {}
void W8Page005EF664::Deactivate() {}
void W8Page005EF664::VMethod07() {}
void W8Page005EF664::VMethod08() {}

// FUNCTION: WIZ8 0x005c8de0
W8Page005EF664* CreatePage005C8DE0()
{
    void* memory;

    memory = ::operator new(0x624);
    if (memory == 0) {
        return 0;
    }
    return new (memory) W8Page005EF664();
}

// VTABLE: WIZ8 0x005ef5c8 W8Page005EF5C8
// VTABLE: WIZ8 0x005ef5c0 W8PageExtra005EF5C0
// class W8Page005EF5C8

W8Page005EF5C8::W8Page005EF5C8()
    : W8PageBase005EF1E4(0x108)
{
    pad_74[0] = 0;
    pad_74[1] = 0;
    pad_74[2] = 0;
    pad_74[3] = 0;
}

void W8Page005EF5C8::Activate() {}
void W8Page005EF5C8::Deactivate() {}
void W8Page005EF5C8::VMethod07() {}
void W8Page005EF5C8::VMethod08() {}
void W8Page005EF5C8::Extra0() {}
void W8Page005EF5C8::Extra1() {}

// FUNCTION: WIZ8 0x005c7cc0
W8Page005EF5C8* CreatePage005C7CC0()
{
    void* memory;

    memory = ::operator new(0x78);
    if (memory == 0) {
        return 0;
    }
    return new (memory) W8Page005EF5C8();
}

// VTABLE: WIZ8 0x005ef57c W8Page005EF57C
// class W8Page005EF57C

W8Page005EF57C::W8Page005EF57C()
    : W8PageBase005EF1E4(0x105), m_timer_70(0.4f, 1)
{
    memset(unknown_94, 0, sizeof(unknown_94));
    flag_fc = 0;
}

void W8Page005EF57C::Activate() {}
void W8Page005EF57C::Deactivate() {}
void W8Page005EF57C::VMethod07() {}
void W8Page005EF57C::VMethod08() {}

// FUNCTION: WIZ8 0x005c73f0
W8Page005EF57C* CreatePage005C73F0()
{
    void* memory;

    memory = ::operator new(0x100);
    if (memory == 0) {
        return 0;
    }
    return new (memory) W8Page005EF57C();
}
