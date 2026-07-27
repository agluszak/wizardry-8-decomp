/* A one-slot polymorphic class at vtable 0x005EF894 over the base whose
   destructor is 0x004F3480 - a base with fifty-three callers, so this is one
   leaf of a wide family. Its teardown is the two instructions an empty derived
   destructor emits: restore the class vtable, tail-jump to the base
   destructor, with the scalar deleting destructor generated from the same
   declaration.

   Nothing names the class, so it is qualified by its vtable address, and no
   extent beyond the vtable pointer is claimed because no constructor for it is
   identified yet. */

class W8ObjectBase004F3480 {
public:
    virtual ~W8ObjectBase004F3480();     /* 0x004F3480 */
};

class W8Object005EF894 : public W8ObjectBase004F3480 {
public:
    virtual ~W8Object005EF894();         /* 0x005D1020 */
};

// FUNCTION: WIZ8 0x005D1020
W8Object005EF894::~W8Object005EF894()
{
}
