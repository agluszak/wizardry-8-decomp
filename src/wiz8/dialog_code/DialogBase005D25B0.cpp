#include "wiz8/dialog_base.h"

#include "english.h"
#include "mousesystem_macros.h"

#include <ctype.h>

/* Dialog Code. The shared dialog base at vtable 0x005EF8B0. Its lifetime
   bodies are what every derived dialog runs before and after its own. */

// FUNCTION: WIZ8 0x005d25b0
W8DialogBase005D25B0::W8DialogBase005D25B0()
    : m_field_54(0),
      m_field_55(1),
      m_field_56(-1),
      m_field_58(-1),
      m_field_5c(-1),
      m_field_60(-1),
      m_field_74(-1),
      m_field_78(-1),
      m_field_8c(0),
      m_field_90(0)
{
}

/* A virtual called from a destructor has a fixed dynamic type, so the
   compiler dispatches it directly; that direct call is slot 2. */
// FUNCTION: WIZ8 0x005d2610
W8DialogBase005D25B0::~W8DialogBase005D25B0()
{
    ResetSubobjectAndRefresh();
}

// FUNCTION: WIZ8 0x005d3020
unsigned char W8DialogBase005D25B0::HandleInput005D3020(
    const InputAtom* input)
{
    if (input->usEvent != KEY_DOWN) {
        return m_field_55;
    }

    if (m_field_95 != 0) {
        int key = toupper(input->usParam);
        if (key == ESC) {
            m_field_54 = 0;
            m_field_55 = 0;
            return 0;
        }
        if (key != '\r') {
            return m_field_55;
        }
    }

    m_field_54 = 1;
    m_field_55 = 0;
    return 0;
}

// FUNCTION: WIZ8 0x005d3080
unsigned char W8DialogBase005D25B0::Close()
{
    SGPPoint mouse;
    InputAtom input;

    GetMousePos(&mouse);
    MSYS_SGP_Mouse_Handler_Hook(
        MOUSE_POS,
        mouse.iX,
        mouse.iY,
        gfLeftButtonState,
        gfRightButtonState);

    while (DequeueEvent(&input)) {
        unsigned short mouse_event;

        switch (input.usEvent) {
        case LEFT_BUTTON_DOWN:
        case LEFT_BUTTON_REPEAT:
            mouse_event = LEFT_BUTTON_DOWN;
            break;
        case LEFT_BUTTON_UP:
            mouse_event = LEFT_BUTTON_UP;
            break;
        case RIGHT_BUTTON_DOWN:
            mouse_event = RIGHT_BUTTON_DOWN;
            break;
        case RIGHT_BUTTON_UP:
            mouse_event = RIGHT_BUTTON_UP;
            break;
        default:
            return HandleInput005D3020(&input);
        }

        MSYS_SGP_Mouse_Handler_Hook(
            mouse_event,
            mouse.iX,
            mouse.iY,
            gfLeftButtonState,
            gfRightButtonState);
    }

    return m_field_55;
}
