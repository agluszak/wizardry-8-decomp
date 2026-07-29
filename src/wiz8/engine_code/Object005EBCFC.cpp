/* A one-slot polymorphic class at vtable 0x005EBCFC over the base whose
   destructor is 0x00439A00. Its whole teardown is the two instructions an
   empty derived destructor emits: restore the class vtable, tail-jump to the
   base destructor.

   The image also holds an adjustor form at 0x00421890 - the same body behind
   `add ecx, 0xc4` - so this class is embedded at +0xc4 of some larger object.
   That owner is not recovered here, and the census's 0x28 allocation hint
   rests on writer attribution the padding heuristic got wrong, so no extent
   beyond the vtable pointer is claimed.

   Nothing names the class, so it is qualified by its vtable address. */

class W8ObjectBase00439A00 {
public:
    virtual ~W8ObjectBase00439A00();     /* 0x00439A00 */
};

class W8Object005EBCFC : public W8ObjectBase00439A00 {
public:
    virtual ~W8Object005EBCFC() override;         /* 0x004218D0 */
};

// FUNCTION: WIZ8 0x004218d0
W8Object005EBCFC::~W8Object005EBCFC()
{
}
