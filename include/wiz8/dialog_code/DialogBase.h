#pragma once

#include "wiz8/vector.h"
#include "wiz8/engine_code/game_timer.h"

#include <wchar.h>

class W8DialogBase;
class W8TextBuffer005ED5B8;
typedef void (*W8DialogDestroyCallback)(W8DialogBase* dialog);
extern "C" void SetDialogDestroyCallback(
    W8DialogBase* dialog, W8DialogDestroyCallback callback);

/* Names describe recovered roles; retail does not expose their original
   source spellings. Address markers retain the binary identities. */
// VTABLE: WIZ8 0x005efaf8
class W8DialogBase {
public:
    W8DialogBase();              /* 0x005DC7A0 */
    virtual ~W8DialogBase();     /* 0x005DC860 */
    virtual int CreateControls();          /* 0x005DCAF0 */
    virtual void DestroyControls();        /* 0x005DCC30 */
    virtual void Draw();                   /* 0x005DC890 */
    virtual int GetDialogType();           /* 0x005D6FA0: base=0, modal=1, list=3 */
    virtual void SetText(const wchar_t* text); /* 0x005DC940 */
    /* Slots 6, 7 and 8 of the table at 0x005EFAF8 are 0x005DC9C0, 0x005DC9F0 and
       0x005DCA70 - the three setters a derived constructor calls directly
       because its dynamic type is fixed, and that a caller holding a base
       pointer reaches through the table. */
    virtual void SetOrigin(int x, int y);            /* 0x005DC9C0 */
    virtual void SetExtent(int width, int height);   /* 0x005DC9F0 */
    virtual void SetBackground(const char* path, int flags); /* 0x005DCA70 */
    virtual unsigned char ProcessInput();            /* 0x005DCCE0 */
    virtual void OnNumericInputChanged(int control_id);
    virtual void OnRightButtonDown();
    virtual void OnRightButtonUp();
    virtual void OnMouseWheel(int delta);

    friend void SetDialogDestroyCallback(
        W8DialogBase* dialog, W8DialogDestroyCallback callback);

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
public:
    /* CharacterScreen closes a completed modal directly through this byte. */
    unsigned char m_field_41;            /* 0x41 */
protected:
    unsigned char unknown_042[2];
    W8DialogDestroyCallback m_destroy_callback; /* 0x44 */
    int m_field_48;                      /* 0x48 */
    int m_field_4c;                      /* 0x4c */
    unsigned char m_field_50;            /* 0x50 */
    unsigned char unknown_051[3];
};                                      /* 0x54 */

static_assert(sizeof(W8DialogBase) == 0x54,
              "W8DialogBase_must_be_0x54");

/* Factory kinds 3 and 5 each have a distinct primary vtable and complete
   lifecycle family. Their original names are not exposed by retail evidence. */
// VTABLE: WIZ8 0x005ef7c8
class W8Dialog005CBB40 : public W8DialogBase {
public:
    W8Dialog005CBB40();                  /* 0x005CBB40 */
    virtual ~W8Dialog005CBB40() override;
    virtual int CreateControls() override;
    virtual void DestroyControls() override;
    virtual void Draw() override;
    virtual int GetDialogType() override;
    virtual void SetText(const wchar_t* text) override;
    virtual unsigned char ProcessInput() override;

private:
    unsigned char unknown_054[0xa8];
};                                      /* 0xfc */

// VTABLE: WIZ8 0x005ef9f0
class W8Dialog005D97D0 : public W8DialogBase {
public:
    W8Dialog005D97D0();                  /* 0x005D97D0 */
    virtual ~W8Dialog005D97D0() override;
    virtual int CreateControls() override;
    virtual void DestroyControls() override;
    virtual void Draw() override;
    virtual unsigned char ProcessInput() override;
    virtual void OnNumericInputChanged(int value) override;

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

class W8DialogScrollBar {
public:
    W8DialogScrollBar();         /* 0x005E0C40 */
    ~W8DialogScrollBar();
    void DestroyControls();              /* 0x005E0E00 */
    void UpdateThumb();                  /* 0x005E1000 */
    void Draw(unsigned char force);      /* 0x005E10B0 */
    void ScrollUp();                     /* 0x005E1170 */
    void ScrollDown();                   /* 0x005E11A0 */

private:
    unsigned char m_initialized;         /* 0x00 */
    unsigned char m_visible;             /* 0x01 */
    unsigned char m_dirty;               /* 0x02 */
    unsigned char unknown_003;
    int m_entry_count;                   /* 0x04 */
    int m_first_visible_entry;           /* 0x08 */
    int m_entry_height;                  /* 0x0c */
    int m_view_height;                   /* 0x10 */
    int m_track_bounds[4];               /* 0x14: left, top, right, bottom */
    int unknown_024;                     /* 0x24 */
    int m_up_image;                      /* 0x28 */
    int m_up_button;                     /* 0x2c */
    int m_down_image;                    /* 0x30 */
    int m_down_button;                   /* 0x34 */
    int m_thumb_image;                   /* 0x38 */
    int m_thumb_button;                  /* 0x3c */
    int m_track_image;                   /* 0x40 */
    int m_track_button;                  /* 0x44 */
    void (*m_on_scroll)(W8DialogScrollBar* scroll_bar, int first_visible_entry); /* 0x48 */
};                                      /* 0x4c */

// VTABLE: WIZ8 0x005efa98
class W8DialogButton {
public:
    W8DialogButton();            /* 0x005DB1B0 */
    virtual ~W8DialogButton();   /* 0x005DB260 */
    void Draw();
    void SetPosition(int x, int y);
    int GetWidth();
    int GetHeight();
    int GetX();
    int GetY();
    void SetEnabled(unsigned char enabled);
    unsigned char IsEnabled();
    void SetPressed(unsigned char pressed);
    unsigned char IsPressed();

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

/* Two instances of this pointer-vector specialization are embedded in
   W8DialogTextArea. */
// VTABLE: WIZ8 0x005ef898
// class W8GrowableVector<W8TextBuffer005ED5B8*>

class W8DialogTextArea {
public:
    W8DialogTextArea();           /* 0x005D14D0 */
    ~W8DialogTextArea();          /* 0x005D1590 */
    unsigned char ScrollDown(unsigned char check_only);
    unsigned char ScrollUp(unsigned char check_only);
    void SetFirstVisibleEntry(unsigned int index);

private:
    int m_left_000;
    int m_top_004;
    int m_right_008;
    int m_bottom_00c;
    int unknown_010;
    int unknown_014;
    int unknown_018;
    W8GrowableVector<W8TextBuffer005ED5B8*> m_all_lines_01c;
    W8GrowableVector<W8TextBuffer005ED5B8*> m_visible_lines_02c;
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

// VTABLE: WIZ8 0x005efab0
class W8SpellInfoDialog005EFAB0 : public W8DialogBase {
public:
    W8SpellInfoDialog005EFAB0(unsigned int spell); /* 0x005DBB60 */
    virtual ~W8SpellInfoDialog005EFAB0() override;

private:
    unsigned int m_spell_054;
    W8DialogScrollBar m_scroll_bar_058;
    W8DialogButton m_button_0a4;
    W8DialogTextArea m_text_area_0ec;
    W8GameTimer m_timer_144;
    unsigned int m_value_168;
};
static_assert(sizeof(W8SpellInfoDialog005EFAB0) == 0x16c,
              "W8SpellInfoDialog005EFAB0_size");
