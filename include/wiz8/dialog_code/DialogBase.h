#pragma once

#include "wiz8/vector.h"

#include <wchar.h>

class W8DialogBase005DC7A0;
typedef void (*W8DialogDestroyCallback)(W8DialogBase005DC7A0* dialog);
extern "C" void Function5CF580(
    W8DialogBase005DC7A0* dialog, W8DialogDestroyCallback callback);

/* The released binary proves this shared dialog base and the three embedded
   member identities, but exposes none of their original source names. Address-
   qualified positional names preserve that distinction. */
// VTABLE: WIZ8 0x005efaf8
class W8DialogBase005DC7A0 {
public:
    W8DialogBase005DC7A0();              /* 0x005DC7A0 */
    virtual ~W8DialogBase005DC7A0();     /* 0x005DC860 */
    virtual int vslot1();            /* 0x005DCAF0 */
    virtual void ResetSubobjectAndRefresh();
    virtual void vslot3();                 /* 0x005DC890 */
    virtual int vslot4();
    virtual void vslot5(const wchar_t* text); /* 0x005DC940 */
    /* Slots 6, 7 and 8 of the table at 0x005EFAF8 are 0x005DC9C0, 0x005DC9F0 and
       0x005DCA70 - the three setters a derived constructor calls directly
       because its dynamic type is fixed, and that a caller holding a base
       pointer reaches through the table. */
    virtual void SetOrigin(int x, int y);            /* 0x005DC9C0 */
    virtual void SetExtent(int width, int height);   /* 0x005DC9F0 */
    virtual void SetBackground(const char* path, int flags); /* 0x005DCA70 */
    virtual unsigned char ProcessInput();            /* 0x005DCCE0 */
    virtual void vslot10(int value);
    virtual void vslot11();
    virtual void ClearField41IfEnabled();
    virtual void vslot13(int value);

    friend void Function5CF580(
        W8DialogBase005DC7A0* dialog, W8DialogDestroyCallback callback);

protected:
    unsigned int m_dirty_flags;           /* 0x04 */
    int m_error;                          /* 0x08 */
    int m_resource;                       /* 0x0c */
    wchar_t* m_text;                      /* 0x10 */
    int m_font;                           /* 0x14 */
    unsigned char m_foreground;           /* 0x18 */
    unsigned char m_background;           /* 0x19 */
    unsigned char unknown_01a[2];
    char* m_background_path;              /* 0x1c */
    int m_background_flags;               /* 0x20 */
    short m_border;                       /* 0x24 */
    unsigned char unknown_026[2];
    int m_x;                              /* 0x28 */
    int m_y;                              /* 0x2c */
    int m_width;                          /* 0x30 */
    int m_height;                         /* 0x34 */
    unsigned char unknown_038[8];
    unsigned char m_initialized;          /* 0x40 */
    unsigned char m_field_41;            /* 0x41 */
    unsigned char unknown_042[2];
    W8DialogDestroyCallback m_destroy_callback; /* 0x44 */
    int m_field_48;                      /* 0x48 */
    int m_field_4c;                      /* 0x4c */
    unsigned char m_field_50;            /* 0x50 */
    unsigned char unknown_051[3];
};                                      /* 0x54 */

static_assert(sizeof(W8DialogBase005DC7A0) == 0x54,
              "W8DialogBase005DC7A0_must_be_0x54");

/* Factory kinds 3 and 5 each have a distinct primary vtable and complete
   lifecycle family. Their original names are not exposed by retail evidence. */
// VTABLE: WIZ8 0x005ef7c8
class W8Dialog005CBB40 : public W8DialogBase005DC7A0 {
public:
    W8Dialog005CBB40();                  /* 0x005CBB40 */
    virtual ~W8Dialog005CBB40() override;
    virtual int vslot1() override;
    virtual void ResetSubobjectAndRefresh() override;
    virtual void vslot3() override;
    virtual int vslot4() override;
    virtual void vslot5(const wchar_t* text) override;
    virtual unsigned char ProcessInput() override;

private:
    unsigned char unknown_054[0xa8];
};                                      /* 0xfc */

// VTABLE: WIZ8 0x005ef9f0
class W8Dialog005D97D0 : public W8DialogBase005DC7A0 {
public:
    W8Dialog005D97D0();                  /* 0x005D97D0 */
    virtual ~W8Dialog005D97D0() override;
    virtual int vslot1() override;
    virtual void ResetSubobjectAndRefresh() override;
    virtual void vslot3() override;
    virtual unsigned char ProcessInput() override;
    virtual void vslot10(int value) override;

private:
    int m_fields_54[6];
    int m_field_6c;
    int m_field_70;
    int m_field_74;
    int m_field_78;
    int m_field_7c;
    int m_field_80;
    int m_field_84;
    int m_field_88;
    int m_field_8c;
};                                      /* 0x90 */

static_assert(sizeof(W8Dialog005CBB40) == 0xfc,
              "W8Dialog005CBB40_must_be_0xfc");
static_assert(sizeof(W8Dialog005D97D0) == 0x90,
              "W8Dialog005D97D0_must_be_0x90");

class W8DialogMember005E0C40 {
public:
    W8DialogMember005E0C40();            /* 0x005E0C40 */
    ~W8DialogMember005E0C40();
    void Reset();                        /* 0x005E0E00 */

private:
    unsigned char unknown_000;           /* 0x00 */
    unsigned char unknown_001;           /* 0x01 */
    unsigned char unknown_002;           /* 0x02 */
    unsigned char unknown_003;
    int unknown_004;                     /* 0x04 */
    int unknown_008;                     /* 0x08 */
    int unknown_00c;                     /* 0x0c */
    int unknown_010;                     /* 0x10 */
    int unknown_014[4];                  /* 0x14 */
    int unknown_024;                     /* 0x24 */
    int unknown_028;                     /* 0x28 */
    int unknown_02c;                     /* 0x2c */
    int unknown_030;                     /* 0x30 */
    int unknown_034;                     /* 0x34 */
    int unknown_038;                     /* 0x38 */
    int unknown_03c;                     /* 0x3c */
    int unknown_040;                     /* 0x40 */
    int unknown_044;                     /* 0x44 */
    int unknown_048;                     /* 0x48 */
};                                      /* 0x4c */

class W8DialogMember005DB1B0 {
public:
    W8DialogMember005DB1B0();            /* 0x005DB1B0 */
    virtual ~W8DialogMember005DB1B0();   /* 0x005DB260 */

private:
    int unknown_004;
    int unknown_008;
    int unknown_00c;
    int unknown_010;
    int unknown_014;
    int m_resource_018;
    int m_resource_01c;
    int unknown_020;
    int unknown_024;
    int unknown_028;
    int unknown_02c;
    int unknown_030;
    unsigned char unknown_034;
    unsigned char unknown_035;
    unsigned char unknown_036;
    unsigned char unknown_037;
    unsigned char unknown_038;
    unsigned char unknown_039;
    unsigned char unknown_03a;
    unsigned char unknown_03b;
    unsigned char unknown_03c;
    unsigned char unknown_03d[3];
    int unknown_040;
    int unknown_044;
};                                      /* 0x48 */

class W8DialogOwned005D14D0 {
public:
    virtual ~W8DialogOwned005D14D0();
};

/* Two instances of this pointer-vector specialization are embedded in
   W8DialogMember005D14D0. */
// VTABLE: WIZ8 0x005ef898
// class W8GrowableVector<W8DialogOwned005D14D0*>

class W8DialogMember005D14D0 {
public:
    W8DialogMember005D14D0();            /* 0x005D14D0 */
    ~W8DialogMember005D14D0();           /* 0x005D1590 */
    unsigned char Function5D1AE0(unsigned int command);
    unsigned char Function5D1C00(unsigned int command);

private:
    unsigned char unknown_000[0x10];
    int unknown_010;
    int unknown_014;
    int unknown_018;
    W8GrowableVector<W8DialogOwned005D14D0*> m_vector_01c;
    W8GrowableVector<W8DialogOwned005D14D0*> m_vector_02c;
    unsigned char unknown_03c;
    unsigned char unknown_03d;
    unsigned char unknown_03e;
    unsigned char unknown_03f;
    int unknown_040;
    int unknown_044;
    int unknown_048;
    int unknown_04c;
    int unknown_050;
    unsigned char unknown_054;
    signed char unknown_055;
    unsigned char unknown_056;
    unsigned char unknown_057;
};                                      /* modeled minimum 0x58 */
