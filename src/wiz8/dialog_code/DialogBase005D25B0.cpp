#include "wiz8/dialog_base.h"

/* Dialog Code. The shared dialog base at vtable 0x005EF8B0. Its lifetime
   bodies are what every derived dialog runs before and after its own. */

/* All ten fields initialize here, which reproduces every instruction and both
   constant materializations; the sole residual is that VC6 places the implicit
   vtable store after the whole list where the canonical has it after the six
   -1 stores and before the four zero and one stores.

   Splitting the list six/four does move the vtable store to the canonical
   position, but it then costs more than it buys: VC6 materializes the zero
   into EAX at its first use and hoists the three zero stores above the vtable
   store, where the canonical holds zero in ECX from the top of the function.
   That is register caching the source cannot direct - the same residual class
   already recorded for Function4E3340 and Function54B560 - so the arrangement
   that keeps all nineteen instructions and both constants correct is the one
   kept here. */
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
