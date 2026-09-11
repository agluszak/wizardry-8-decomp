#pragma once

#include "wiz8/dialog_code/DialogBase.h"
#include "wiz8/dialog_code/DialogButton.h"
#include "wiz8/local_code/ControlsRect.h"
#include "wiz8/local_code/TextBuffer.h"
#include "wiz8/text_types.h"
#include "wiz8/vector.h"

#include "Button System.h"
#include "input.h"

struct W8WorldItem;
class Trigger;

/* The small numeric entry field embedded by the factory dialogs. Constructor
   initialization at 0x005E1460 fills 0x30 bytes; its value, active flag and
   backing button are what the owning dialog reads and writes. The other
   dialogs in Dialog Code use the same field through 0x005DDA60 and 0x005DE120,
   so this declaration is the shared owner until an original name is proven. */
class W8DialogNumericInput {
public:
    void Initialize(int flags, const W8ControlsRect* bounds, int value, int font,
                    W8DialogBase* dialog, W8DialogButton* button); /* 0x005E1460 */
    void SetValue(int value);             /* 0x005E14C0 */
    void SetActive(unsigned char active); /* 0x005E14D0 */
    void Draw(unsigned char force);       /* 0x005E15C0 */
    unsigned char HandleInput(const InputAtom* input); /* 0x005E19A0 */

    W8ControlsRect m_bounds;        /* 0x00 */
    int m_caret;                    /* 0x10: -1 when inactive */
    int m_font;                     /* 0x14 */
    int m_value;                    /* 0x18 */
    unsigned char m_dirty;          /* 0x1c */
    unsigned char m_active;         /* 0x1d */
    unsigned char unknown_01e[2];
    int m_maximum;                  /* 0x20: -1, then 1000000 for the split dialog */
    W8DialogBase* m_dialog;         /* 0x24 */
    W8DialogButton* m_button;       /* 0x28 */
    int m_field_2c;                 /* 0x2c */
};
static_assert(sizeof(W8DialogNumericInput) == 0x30, "W8DialogNumericInput_size");

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
    /* 0x005CC650: text-area button height divided by the dialog font height. */
    int GetVisibleLineCount005CC650();
    /* 0x005CCB80: select a text line and scroll it into the visible range. */
    void SetCurrentLine005CCB80(int line);
    /* 0x005CD2B0: keyboard handling for the visible text list. */
    unsigned char HandleInputEvent005CD2B0(const InputAtom* input);

    /* SGP move/click callbacks for the text area, the scroll arrow buttons and
       the confirmation buttons. Bodies are not recovered in this change. */
    static void Function5CCE70(GUI_BUTTON* button, INT32 reason);
    static void Function5CCF30(GUI_BUTTON* button, INT32 reason);
    static void Function5CCFE0(GUI_BUTTON* button, INT32 reason);
    static void Function5CD090(GUI_BUTTON* button, INT32 reason);
    static void Function5CD130(GUI_BUTTON* button, INT32 reason);
    static void Function5CD1E0(GUI_BUTTON* button, INT32 reason);

public:
    /* 0x054: the displayed text lines; the dialog owns and frees each one. */
    W8GrowableVector<W8WideChar*> m_lines_054;
    /* 0x064: pointer vector of the W8MasterFunction (void (*)(int)) pointer
       specialization emitted by MasterFunctionList.cpp (vtable 0x005ED43C,
       ctor 0x004D9A70, scalar deleting destructor 0x004D9A40). This unit only
       constructs, clears and destroys it; `void*` would erase the proven
       element type. */
    W8GrowableVector<void (*)(int)> m_field_064;
    int m_field_074;                /* 0x074 */
    float m_field_078;              /* 0x078: 0.05 */
    float m_field_07c;              /* 0x07c: 0.2 */
    float m_field_080;              /* 0x080: 0.9 */
    float m_field_084;              /* 0x084: 0.75 */
    int m_field_088;                /* 0x088: highlight fill colour */
    int m_text_button_08c;          /* 0x08c */
    int m_second_text_button_090;   /* 0x090 */
    short m_inlay_image_094;        /* 0x094: DialogInlay inlay for 0x098 */
    short unknown_096;
    int m_area_button_098;          /* 0x098: scrolling text area */
    int m_up_button_09c;            /* 0x09c */
    int m_up_image_0a0;             /* 0x0a0 */
    int m_down_button_0a4;          /* 0x0a4 */
    int m_down_image_0a8;           /* 0x0a8 */
    int m_slider_button_0ac;        /* 0x0ac */
    int m_slider_image_0b0;         /* 0x0b0 */
    short m_inlay_image_0b4;        /* 0x0b4: DialogInlay inlay for 0x0b8 */
    short unknown_0b6;
    int m_third_text_button_0b8;    /* 0x0b8 */
    int m_ok_button_0bc;            /* 0x0bc */
    int m_ok_image_0c0;             /* 0x0c0 */
    /* 0x0c4 and 0x0dc are the inclusive click rectangles ProcessInput tests
       with the cursor position. No writer for either has been recovered in
       this unit, so the producer remains unknown. */
    W8ControlsRect m_ok_rect_0c4;
    int m_cancel_button_0d4;        /* 0x0d4 */
    int m_cancel_image_0d8;         /* 0x0d8 */
    W8ControlsRect m_cancel_rect_0dc;
    unsigned char m_field_0ec;      /* 0x0ec: scrolling area is scrollable */
    unsigned char unknown_0ed[3];
    int m_first_visible_line_0f0;   /* 0x0f0 */
    int m_selected_line_0f4;        /* 0x0f4 */
    short m_inlay_image_0f8;        /* 0x0f8: DialogEdge inlay for the text buttons */
    short unknown_0fa;
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
    /* 0x005D9B30: create and place the six dialog buttons. */
    unsigned char CreateButtons005D9B30();
    /* 0x005D9D10: create the three text buffers above the numeric field. */
    unsigned char CreateTextBuffers005D9D10();
    /* 0x005D9E30: create the numeric entry field and its backing button. */
    unsigned char CreateNumericInput005D9E30();
    /* 0x005DA090: enable the plus/minus buttons from the current split. */
    void UpdateButtonStates005DA090();
    /* 0x005DA000: push the split into the text buffers and the numeric field. */
    void UpdateTextBuffers005DA000();
    /* 0x005DA180: keyboard handling for the plus/minus buttons and the field. */
    unsigned char HandleInputEvent005DA180(const InputAtom* input);

    /* Per-button callbacks stored through W8DialogButton::Configure. Bodies
       are not recovered in this change. */
    static void Function5DA440(W8DialogButton* button);
    static void Function5DA490(W8DialogButton* button);
    static void Function5DA4E0(W8DialogButton* button);
    static void Function5DA530(W8DialogButton* button);
    static void Function5DA580(W8DialogButton* button);
    static void Function5DA5A0(W8DialogButton* button);
    static void Function5DA5C0(W8DialogButton* button);

private:
    W8DialogButton* m_fields_54[6];
    W8TextBuffer* m_field_6c;
    W8TextBuffer* m_field_70;
    W8TextBuffer* m_field_74;
    W8DialogNumericInput* m_field_78;
    /* 0x07c: the numeric field while the cursor or keyboard owns it. */
    W8DialogNumericInput* m_active_field_7c;
    int m_remaining_080;            /* 0x080: total minus the field value */
    int m_taken_084;                /* 0x084: the field value */
    int m_total_088;                /* 0x088 */
    int m_result_08c;               /* 0x08c: 1 confirms, 2 cancels */
};                                      /* 0x90 */

static_assert(sizeof(W8Dialog005CBB40) == 0xfc,
              "W8Dialog005CBB40_must_be_0xfc");
static_assert(sizeof(W8Dialog005D97D0) == 0x90,
              "W8Dialog005D97D0_must_be_0x90");

/* The trigger-owned item picker. Its constructor is 0x005CD710, its primary
   table 0x005EF810, and its complete object 0xB0 bytes. The 13 button slots
   and the two vectors are proven by the constructor, DestroyControls and the
   item merge path; the item group at +0xac is what the trigger hands in and
   the destroy callback hands back. */
// VTABLE: WIZ8 0x005ef810
class W8Dialog005CD710 : public W8DialogBase {
public:
    W8Dialog005CD710();                       /* 0x005CD710 */
    virtual ~W8Dialog005CD710() override;     /* 0x005CD820 */
    virtual int CreateControls() override;    /* 0x005CDC10 */
    virtual void DestroyControls() override;  /* 0x005CDC40 */
    virtual void Draw() override;             /* 0x005CDC70 */
    virtual int GetDialogType() override;     /* 0x005CF240 */
    virtual unsigned char ProcessInput() override; /* 0x005CEF00 */

    int AddItem005CE210(W8WorldItem* item);
    W8WorldItem* ReturnItemsToGroup005CF110();
    void SetItemGroup005CF0C0(W8WorldItem* group);

private:
    unsigned char CreateButtons005CD8D0();
    /* Clamp and apply the first visible item row. Fewer than five items force
       the first row. Out-of-range input is ignored, not clamped. */
    void SetFirstVisible(int index);
    /* Sync the four scroll buttons' pressed/visible state with the scroll
       offset and the per-item enable flags. */
    void RefreshScrollButtons005CE420();
    /* Handle one event the picker owns: keyboard list navigation and clicks. */
    unsigned char HandleInputEvent005CEC20(const InputAtom* input);

    /* Per-button callbacks stored through W8DialogButton::Configure. Bodies
       are not recovered in this change. */
    static void Function5CE5F0(W8DialogButton* button);
    static void Function5CE6A0(W8DialogButton* button);
    static void Function5CE6C0(W8DialogButton* button);
    static void Function5CE6E0(W8DialogButton* button);
    static void Function5CE6F0(W8DialogButton* button);
    static void Function5CE790(W8DialogButton* button);
    static void Function5CE830(W8DialogButton* button);
    static void Function5CE8D0(W8DialogButton* button);
    static void Function5CE970(W8DialogButton* button);
    static void Function5CE9B0(W8DialogButton* button);
    static void Function5CE9F0(W8DialogButton* button);
    static void Function5CEA30(W8DialogButton* button);
    static void Function5CEA70(W8DialogButton* button);
    static void Function5CEAB0(W8DialogButton* button);
    static void Function5CEAF0(W8DialogButton* button);

public:

public:
    W8GrowableVector<W8WorldItem*> items_54;
    W8GrowableVector<unsigned char> flags_64;
    W8DialogButton* m_buttons_74[13];
    int m_first_item_0a8;
    W8WorldItem* m_item_group_0ac;
};

static_assert(sizeof(W8Dialog005CD710) == 0xb0,
              "W8Dialog005CD710_must_be_0xb0");
