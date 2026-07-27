#include "wiz8/dialog_base.h"
#include "wiz8/gameplay_boundaries.h"

/* Dialog Code. This is the reviewed class W8Dialog005A80A0 in
   evidence/reviewed/wiz8/classes.csv - a modal popup built on the unnamed base
   constructed by 0x005D25B0 and destroyed by 0x005D2610. Only the members
   these bodies touch are modelled; the base storage stays opaque, and the two
   own fields at 0x98 and 0x9c are what the ports prove.

   The base is the shared W8DialogBase005D25B0 in wiz8/dialog_base.h, whose
   fifteen slots this class inherits; it overrides only slot 9. */

/* The object the dialog notifies when it closes. Only its first virtual slot
   is reached, and only with these two arguments. */
struct W8DialogNotifyTarget {
    virtual void Notify(unsigned char reason, int value) = 0;
};

/* Table of message payloads the dialog is constructed against; the caller
   passes an index into it. */
extern void** g_dialog_message_table;    /* 0x0068C09C */

/* Registers the dialog's region set. */
extern void ActivateDialogRegion(int region_set);  /* 0x004F2040 */

class W8Dialog005A80A0 : public W8DialogBase005D25B0 {
public:
    W8Dialog005A80A0(int message_index, int caption_id, int notify_value);
    virtual ~W8Dialog005A80A0();         /* 0x005A8190 */
    /* The canonical table places this override at the inherited slot 9, whose
       base body is BaseClose. The model cannot say so yet: overriding needs
       the base slot's recovered signature, and only its return type is known
       here - so the position is asserted in this comment rather than by the
       type system, and the emitted body is unaffected either way. */
    virtual unsigned char Close();       /* 0x005A81A0, canonical slot 9 */

private:
    int notify_value_98;                 /* 0x98 */
    W8DialogNotifyTarget* notify_target; /* 0x9c */
};                                       /* 0xa0 */

/* The base constructor runs first and the vtable install follows it, so the
   body is just the two own fields and the dialog setup sequence. The /GX EH
   frame comes from the base having a destructor: an exception in any setup
   call has to unwind it. */
// FUNCTION: WIZ8 0x005A80A0
W8Dialog005A80A0::W8Dialog005A80A0(int message_index, int caption_id, int notify_value)
    : notify_value_98(notify_value), notify_target(0)
{
    SetExtent(0xf0, 0xbe);
    SetOrigin(0xa0, 100);
    SetBackground("Data\\Dialogs\\DialogBackground.sti", 0);
    SetClientExtent(0xfa, 200);
    SetMessage(g_dialog_message_table[message_index], 1, 0x32, 1, caption_id, 1, 1, 0, 0x15e);
    ActivateDialogRegion(0x138);
}

/* Restoring the vtable and tail-jumping to the base destructor is the whole
   body; the compiler generates the scalar deleting destructor at 0x005A8170
   from this same declaration. */
// FUNCTION: WIZ8 0x005A8190
W8Dialog005A80A0::~W8Dialog005A80A0()
{
}

// FUNCTION: WIZ8 0x005A81A0
unsigned char W8Dialog005A80A0::Close()
{
    unsigned char handled;

    BaseClose();
    handled = m_field_55;
    if (!handled) {
        ClearActiveRegionIfMatches(0x138);
        if (notify_target) {
            notify_target->Notify(m_field_54, notify_value_98);
        }
    }
    return handled;
}
