#include "wiz8/gameplay_boundaries.h"

/* Dialog Code. This is the reviewed class W8Dialog005A80A0 in
   evidence/reviewed/wiz8/classes.csv - a modal popup
   built on the unnamed base constructed by 0x005D25B0. Only the members this
   body touches are modelled; the base storage stays opaque, and the two own
   fields at 0x98 and 0x9c are the ones the port is meant to prove. */

/* The object the dialog notifies when it closes. Only its first virtual slot
   is reached, and only with these two arguments. */
struct W8DialogNotifyTarget {
    virtual void Notify(unsigned char reason, int value) = 0;
};

struct W8Dialog005A80A0 {
    unsigned char unknown_000[0x54];
    unsigned char flag_54;                /* 0x54: passed to the notify target */
    unsigned char flag_55;                /* 0x55: gates the close path and is returned */
    unsigned char unknown_056[0x42];
    int notify_value_98;                  /* 0x98 */
    W8DialogNotifyTarget* notify_target;  /* 0x9c */

    void BaseClose();
    unsigned char Close();
};

// FUNCTION: WIZ8 0x005A81A0
unsigned char W8Dialog005A80A0::Close()
{
    unsigned char handled;

    BaseClose();
    handled = flag_55;
    if (!handled) {
        ClearActiveRegionIfMatches(0x138);
        if (notify_target) {
            notify_target->Notify(flag_54, notify_value_98);
        }
    }
    return handled;
}
