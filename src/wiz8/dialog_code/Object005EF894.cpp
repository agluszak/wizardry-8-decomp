#include "wiz8/local_code/Controls.h"

/* A one-slot polymorphic class at vtable 0x005EF894 over the text-buffer base
   whose destructor is 0x004F3480. Its teardown is the two instructions an
   empty derived destructor emits: restore the class vtable, tail-jump to the
   base destructor, with the scalar deleting destructor generated from the
   same declaration.

   Nothing names the derived class, so it remains qualified by its vtable
   address. The recovered base layout is 0x50 bytes; no added derived storage
   or constructor is identified. */

class W8Object005EF894 : public W8TextBuffer005ED5B8 {
public:
    virtual ~W8Object005EF894();         /* 0x005D1020 */
};

// FUNCTION: WIZ8 0x005D1020
W8Object005EF894::~W8Object005EF894()
{
}
