#include "wiz8/dialog_base.h"

/* Dialog Code. The shared dialog base at vtable 0x005EF8B0. Its lifetime
   bodies are what every derived dialog runs before and after its own. */

/* The split between the initializer list and the body is recovered, not
   stylistic: VC6 emits initializer-list stores before the implicit vtable
   store and body assignments after it, and the canonical body has exactly six
   stores above the vtable store and four below. */
// FUNCTION: WIZ8 0x005D25B0
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
// FUNCTION: WIZ8 0x005D2610
W8DialogBase005D25B0::~W8DialogBase005D25B0()
{
    ResetSubobjectAndRefresh();
}
