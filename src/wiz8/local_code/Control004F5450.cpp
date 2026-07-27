#include "wiz8/vector.h"

/* Local Code\Controls.cpp. A control whose constructor at 0x004F5450 carries
   its base's construction inlined and embeds one growable vector at +0x10 -
   the shared template in wiz8/vector.h, not a fourth private copy of it. The
   family map reads the shape straight off the vtable stores: the base's table
   at +0, the vector's base and derived tables at +0x10, and the control's own
   table last, which is construction order exactly.

   Neither class is named by the image, so both carry address-qualified
   positional names. */

/* No destructor is declared here on purpose. Under /GX a user-declared base
   destructor makes the derived constructor carry an unwind frame, because the
   vector's operator new can throw after the base is built; the canonical body
   has no frame, so the original base's destructor is implicit. */
class W8ControlBase005ED664 {
public:
    W8ControlBase005ED664();
    virtual void vslot0();
    virtual void vslot1();

protected:
    int m_value_4;                       /* 0x04 */
    int m_value_8;                       /* 0x08 */
    int m_index_c;                       /* 0x0c: the -1 sentinel when unset */
};                                       /* 0x10 */

/* The embedded vector installs two tables at +0x10, the template's and its
   own, which is the second-vtable shape wiz8/vector.h describes. Its element
   type is unproven, so it is named for the vtable the image gives it. */
class W8VectorElement005ED65C;

class W8ControlEntryVector005ED65C
    : public W8GrowableVector<W8VectorElement005ED65C*> {
public:
    W8ControlEntryVector005ED65C();
    virtual ~W8ControlEntryVector005ED65C();
};                                       /* 0x10 */

class W8Control005ED654 : public W8ControlBase005ED664 {
public:
    W8Control005ED654();
    virtual ~W8Control005ED654();

protected:
    W8ControlEntryVector005ED65C m_entries_10; /* 0x10 */
    int m_value_20;                      /* 0x20 */
};                                       /* 0x24 established */

__forceinline W8ControlBase005ED664::W8ControlBase005ED664()
{
    m_value_4 = 0;
    m_value_8 = 0;
    m_index_c = -1;
}

__forceinline W8ControlEntryVector005ED65C::W8ControlEntryVector005ED65C()
{
}

// FUNCTION: WIZ8 0x004F5450
W8Control005ED654::W8Control005ED654()
{
    m_value_20 = 0;
}
