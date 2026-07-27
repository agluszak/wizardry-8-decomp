/* Engine Code\Prop.cpp. The complete destructor at 0x0044BEC0 releases four
   owned members, and each release names the shape of what it owns:

     +0x14  delete through vtable slot 0 with the deleting flag - a class with
            a virtual destructor
     +0x20  a null check and a bare operator delete - a pointer to something
            with no destructor at all
     +0x28  the same virtual-destructor shape as +0x14
     +0x38  its destructor called directly and then operator delete - a class
            with a non-virtual destructor

   Nothing names the class or its members, so all keep positional names, and
   the gaps between the four members stay opaque. */

/* Owned through a virtual destructor; two members share this shape and the
   image does not say whether they share a type. */
class W8PropOwnedPolymorphic {
public:
    virtual ~W8PropOwnedPolymorphic();
};

/* Released with a null check and then a bare operator delete. A pointer to
   something trivially destructible skips the check entirely, so the check
   without a destructor call is what a declared-but-empty destructor emits. */
class W8PropOwned0020 {
public:
    ~W8PropOwned0020();
};

/* Destroyed by a direct call to 0x004B6ED0 followed by operator delete, which
   is what a non-virtual destructor compiles to. */
class W8PropOwned004B6ED0 {
public:
    ~W8PropOwned004B6ED0();              /* 0x004B6ED0 */
};

class W8PropBase004B6B60 {
public:
    virtual ~W8PropBase004B6B60();       /* 0x004B6B60 */

protected:
    unsigned char unknown_004[0x10];
};                                       /* 0x14 */

class W8Prop005EC1E0 : public W8PropBase004B6B60 {
public:
    virtual ~W8Prop005EC1E0();           /* 0x0044BEC0 */

private:
    W8PropOwnedPolymorphic* m_owned_14;  /* 0x14 */
    unsigned char unknown_018[0x8];
    W8PropOwned0020* m_owned_20;         /* 0x20 */
    unsigned char unknown_024[0x4];
    W8PropOwnedPolymorphic* m_owned_28;  /* 0x28 */
    unsigned char unknown_02c[0xc];
    W8PropOwned004B6ED0* m_owned_38;     /* 0x38 */
};                                       /* 0x3c established */

__forceinline W8PropOwned0020::~W8PropOwned0020()
{
}

// FUNCTION: WIZ8 0x0044BEC0
W8Prop005EC1E0::~W8Prop005EC1E0()
{
    delete m_owned_14;
    delete m_owned_20;
    delete m_owned_28;
    delete m_owned_38;
}
