/* The 18-slot widget base at vtable 0x005ED5BC. Its teardown is one function:
   VC6 folded the complete destructor into the scalar deleting destructor at
   0x004F3D90, which vtable slot 0 points at.

   The image names neither the class nor its fields, so both carry
   address-qualified positional names. What the body does establish is that
   the class owns a region registration, released on teardown unless it is the
   -1 sentinel the RegionManager uses for "none". */

extern void ReleaseRegion(int region_index);   /* 0x004F23D0 */

class W8WidgetBase005ED5BC {
public:
    virtual ~W8WidgetBase005ED5BC();

protected:
    unsigned char unknown_004;
    unsigned char m_flag_5;              /* 0x05: cleared on teardown */
    unsigned char unknown_006[0x12];
    int m_region_18;                     /* 0x18: released unless it is -1 */
};                                       /* 0x1c established */

// FUNCTION: WIZ8 0x004F3D90
W8WidgetBase005ED5BC::~W8WidgetBase005ED5BC()
{
    m_flag_5 = 0;
    if (m_region_18 != -1) {
        ReleaseRegion(m_region_18);
    }
}
