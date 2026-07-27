#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"
#include "wiz8/vector.h"

/* Local Code\Controls.cpp. A control whose constructor at 0x004F5450 carries
   its base's construction inlined and embeds one growable vector at +0x10 -
   the shared template in wiz8/vector.h, not a fourth private copy of it. The
   family map reads the shape straight off the vtable stores: the base's table
   at +0, the vector's base and derived tables at +0x10, and the control's own
   table last, which is construction order exactly.

   Neither class is named by the image, so both carry address-qualified
   positional names, with one exception. The assertion at Controls.cpp:2679
   reads iSelected < m_lsButtons.Length(), and the length it tests is the count
   of the vector at +0x10, so that member is named by the original source
   rather than by its offset. The index at +0x0c is what the same assertion
   guards against that length, which is what it holds; the source does not name
   the member itself, so it keeps its positional name. */

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

    void SetSelected(int iSelected);
    void SelectEntry(W8VectorElement005ED65C* entry);

protected:
    W8ControlEntryVector005ED65C m_lsButtons;  /* 0x10: named by Controls.cpp:2679 */
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

/*
 * Selects the entry equal to the pointer given, asserting that it is present.
 *
 * The assert does not stop anything: srAssertFail returns, and the original
 * falls straight through into the selection with the -1 it just complained
 * about. That is worth keeping visible rather than tidying into an early
 * return, because it means a missing entry reaches SetSelected, whose own
 * assert then fires on the same value.
 */
// FUNCTION: WIZ8 0x004F55C0
void W8Control005ED654::SelectEntry(W8VectorElement005ED65C* entry)
{
    int iIndex;

    iIndex = m_lsButtons.IndexOf(entry);
    if (iIndex == -1) {
        srAssertFail("iIndex != -1",
                     "C:\\Projects\\Wizardry 8\\Local Code\\Controls.cpp",
                     0xa9e,
                     0);
    }
    SetSelected(iIndex);
}
