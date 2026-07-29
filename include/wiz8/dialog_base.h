#pragma once

#include "wiz8/monster_info_dialog.h"

/* The shared dialog base at vtable 0x005EF8B0, between W8DialogBase005DC7A0
   and the concrete dialogs. Its fifteen slots are what every derived dialog
   inherits: W8Dialog005A80A0's table is identical except slot 0, which is each
   class's own scalar deleting destructor, and slot 9, which it overrides.

   The released binary exposes no original name for it, so the name is
   qualified by its constructor address. */
class W8DialogBase005D25B0 : public W8DialogBase005DC7A0 {
public:
    W8DialogBase005D25B0();                     /* 0x005D25B0 */
    virtual ~W8DialogBase005D25B0() override;            /* 0x005D2610 */
    virtual void ResetSubobjectAndRefresh() override;    /* slot 2, 0x005D2F40 */
    virtual void vslot14();                     /* slot 14, 0x005D3020 */

protected:
    /* The slot 9 body. A derived override calls it non-virtually, which is
       why it is declared here rather than reached through the vtable. */
    void BaseClose();                                /* 0x005D3080 */
    void SetExtent(int width, int height);           /* 0x005DC9C0 */
    void SetOrigin(int x, int y);                    /* 0x005DC9F0 */
    void SetBackground(const char* path, int flags); /* 0x005DCA70 */
    void SetClientExtent(int width, int height);     /* 0x005D2CB0 */
    void SetMessage(void* payload, int a, int b, int c, int d,
                    int e, int f, int g, int h);     /* 0x005D2800 */

    unsigned char m_field_54;            /* 0x54: cleared; a derived close passes it on */
    unsigned char m_field_55;            /* 0x55: set, and gates the close path */
    short m_field_56;                    /* 0x56 */
    int m_field_58;                      /* 0x58 */
    int m_field_5c;                      /* 0x5c */
    int m_field_60;                      /* 0x60 */
    unsigned char unknown_064[0x10];
    int m_field_74;                      /* 0x74 */
    int m_field_78;                      /* 0x78 */
    unsigned char unknown_07c[0x10];
    int m_field_8c;                      /* 0x8c */
    int m_field_90;                      /* 0x90 */
    unsigned char unknown_094[4];
};                                       /* 0x98 */
