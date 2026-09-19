#pragma once

#include "wiz8/dialog_code/DialogBase.h"
#include "wiz8/dialog_code/DialogButton.h"
#include "wiz8/local_code/ControlsRect.h"
#include "wiz8/local_code/TextBuffer.h"
#include "wiz8/vector.h"

#include "Button System.h"
#include "input.h"

struct W8WorldItem;
struct W8ItemInstance;
class Trigger;

/* The small numeric entry field embedded by the factory dialogs. Constructor
   initialization at 0x005E1460 fills 0x30 bytes; its value, active flag and
   backing button are what the owning dialog reads and writes. The other
   dialogs in Dialog Code use the same field through 0x005DDA60 and 0x005DE120,
   so this declaration is the shared owner until an original name is proven. */
class W8DialogNumericInput {
public:
    /* 0x005E1460: the only call site (0x005DDA60) allocates 0x30 bytes, null
       checks the result and merges the returned `this`, which is the ordinary
       VC6 `new T(args)` shape rather than a separate initializer call. Retail
       leaves m_active and unknown_01e uninitialized. */
    W8DialogNumericInput(int control_id, const W8ControlsRect* bounds, int value, int font,
                         W8DialogBase* dialog, W8DialogButton* button);
    void SetValue(int value);                                 /* 0x005E14C0 */
    void SetActive(unsigned char active);                     /* 0x005E14D0 */
    void SetActive(unsigned char active, const POINT* point); /* 0x005E1500 */
    void Draw(unsigned char force);                           /* 0x005E15C0 */
    unsigned char HandleInput(const InputAtom* input);        /* 0x005E19A0 */

private:
    /* 0x005E17A0: write a digit character at the caret position and re-parse
       the text into m_value, rejecting results above m_maximum. */
    void TypeDigit005E17A0(wchar_t digit);
    /* 0x005E1840: VK_DELETE — remove the character right of the caret. */
    void DeleteForward005E1840();
    /* 0x005E18F0: VK_BACK — remove the character left of the caret. */
    void Backspace005E18F0();

public:
    W8ControlsRect m_bounds; /* 0x00 */
    /* 0x10: -1 while inactive; otherwise the count of characters to the right
       of the caret. */
    int m_caret;
    int m_font;             /* 0x14 */
    int m_value;            /* 0x18 */
    unsigned char m_dirty;  /* 0x1c */
    unsigned char m_active; /* 0x1d */
    unsigned char unknown_01e[2];
    /* 0x20: -1, then the stack total for the split dialogs. The acceptance
       test in TypeDigit005E17A0 compares unsigned, so -1 is "no maximum". */
    unsigned int m_maximum;
    W8DialogBase* m_dialog;   /* 0x24 */
    W8DialogButton* m_button; /* 0x28 */
    /* 0x2c: echoed back as the argument of m_dialog->OnNumericInputChanged. */
    int m_control_id;
};
static_assert(sizeof(W8DialogNumericInput) == 0x30, "W8DialogNumericInput_size");

/* Factory kinds 3 and 5 each have a distinct primary vtable and complete
   lifecycle family. Their original names are not exposed by retail evidence. */
// VTABLE: WIZ8 0x005ef7c8
class W8ListBoxDialog005CBB40 : public W8DialogBase {
public:
    W8ListBoxDialog005CBB40(); /* 0x005CBB40 */
    virtual ~W8ListBoxDialog005CBB40() override;
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
       the confirmation buttons. Recovered in Dialog Code\stListBox.cpp. */
    static void TextAreaButtonCallback(GUI_BUTTON* button, INT32 reason);    /* 0x005CCE70 */
    static void UpButtonCallback(GUI_BUTTON* button, INT32 reason);          /* 0x005CCF30 */
    static void DownButtonCallback(GUI_BUTTON* button, INT32 reason);        /* 0x005CCFE0 */
    static void OkButtonCallback(GUI_BUTTON* button, INT32 reason);          /* 0x005CD090 */
    static void CancelButtonCallback(GUI_BUTTON* button, INT32 reason);      /* 0x005CD130 */
    static void SliderTrackButtonCallback(GUI_BUTTON* button, INT32 reason); /* 0x005CD1E0 */

public:
    /* 0x054: the displayed text lines; the dialog owns and frees each one. */
    W8GrowableVector<wchar_t*> m_lines_054;
    /* 0x064: pointer vector of the W8MasterFunction (void (*)(int)) pointer
       specialization emitted by MasterFunctionList.cpp (vtable 0x005ED43C,
       ctor 0x004D9A70, scalar deleting destructor 0x004D9A40). This unit only
       constructs, clears and destroys it; `void*` would erase the proven
       element type. */
    W8GrowableVector<void (*)(int)> m_field_064;
    int m_field_074;              /* 0x074 */
    float m_field_078;            /* 0x078: 0.05 */
    float m_field_07c;            /* 0x07c: 0.2 */
    float m_field_080;            /* 0x080: 0.9 */
    float m_field_084;            /* 0x084: 0.75 */
    int m_field_088;              /* 0x088: highlight fill colour */
    int m_text_button_08c;        /* 0x08c */
    int m_second_text_button_090; /* 0x090 */
    short m_inlay_image_094;      /* 0x094: DialogInlay inlay for 0x098 */
    short unknown_096;
    int m_area_button_098;   /* 0x098: scrolling text area */
    int m_up_button_09c;     /* 0x09c */
    int m_up_image_0a0;      /* 0x0a0 */
    int m_down_button_0a4;   /* 0x0a4 */
    int m_down_image_0a8;    /* 0x0a8 */
    int m_slider_button_0ac; /* 0x0ac */
    int m_slider_image_0b0;  /* 0x0b0 */
    short m_inlay_image_0b4; /* 0x0b4: DialogInlay inlay for 0x0b8 */
    short unknown_0b6;
    int m_third_text_button_0b8; /* 0x0b8 */
    int m_ok_button_0bc;         /* 0x0bc */
    int m_ok_image_0c0;          /* 0x0c0 */
    /* 0x0c4 and 0x0dc are the inclusive click rectangles ProcessInput tests
       with the cursor position. No writer for either has been recovered in
       this unit, so the producer remains unknown. */
    W8ControlsRect m_ok_rect_0c4;
    int m_cancel_button_0d4; /* 0x0d4 */
    int m_cancel_image_0d8;  /* 0x0d8 */
    W8ControlsRect m_cancel_rect_0dc;
    unsigned char m_scrollable; /* 0x0ec: scrolling area is scrollable */
    unsigned char unknown_0ed[3];
    int m_first_visible_line_0f0; /* 0x0f0 */
    int m_selected_line_0f4;      /* 0x0f4 */
    short m_inlay_image_0f8;      /* 0x0f8: DialogEdge inlay for the text buttons */
    short unknown_0fa;
}; /* 0xfc */

// VTABLE: WIZ8 0x005ef9f0
class W8SplitAmountDialog : public W8DialogBase {
    /* NPCInteractionSubscreen's destroy callback reads m_taken_084 and
       m_result_08c back out of the closing dialog. */
    friend void OnNpcTradeSplitDialogDestroy00572870(W8DialogBase* dialog);

public:
    W8SplitAmountDialog(); /* 0x005D97D0 */
    /* 0x005D9890: the split-size entry point; the pool total seeds both the
       remaining and total fields. */
    W8SplitAmountDialog(int total);
    virtual ~W8SplitAmountDialog() override;
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

    /* Per-button callbacks stored through W8DialogButton::Configure. */
    static void SplitDecrementOne005DA440(W8DialogButton* button);
    static void SplitDecrementFive005DA490(W8DialogButton* button);
    static void SplitIncrementOne005DA4E0(W8DialogButton* button);
    static void SplitIncrementFive005DA530(W8DialogButton* button);
    static void SplitAccept005DA580(W8DialogButton* button);
    static void SplitCancel005DA5A0(W8DialogButton* button);
    static void SplitActivateField005DA5C0(W8DialogButton* button);

private:
    W8DialogButton* m_buttons_054[6];
    W8TextBuffer* m_field_6c;
    W8TextBuffer* m_field_70;
    W8TextBuffer* m_field_74;
    W8DialogNumericInput* m_split_input_078;
    /* 0x07c: the numeric field while the cursor or keyboard owns it. */
    W8DialogNumericInput* m_active_field_7c;
    int m_remaining_080; /* 0x080: total minus the field value */
    int m_taken_084;     /* 0x084: the field value */
    int m_total_088;     /* 0x088 */
    int m_result_08c;    /* 0x08c: 1 confirms, 2 cancels */
}; /* 0x90 */

static_assert(sizeof(W8ListBoxDialog005CBB40) == 0xfc, "W8ListBoxDialog005CBB40_must_be_0xfc");
static_assert(sizeof(W8SplitAmountDialog) == 0x90, "W8SplitAmountDialog_must_be_0x90");

/* The trigger-owned item picker. Its constructor is 0x005CD710, its primary
   table 0x005EF810, and its complete object 0xB0 bytes. The 13 button slots
   and the two vectors are proven by the constructor, DestroyControls and the
   item merge path; the item group at +0xac is what the trigger hands in and
   the destroy callback hands back. */
// VTABLE: WIZ8 0x005ef810
class W8TriggerItemPickerDialog : public W8DialogBase {
public:
    W8TriggerItemPickerDialog();                   /* 0x005CD710 */
    virtual ~W8TriggerItemPickerDialog() override; /* 0x005CD820 */
    virtual int CreateControls() override;         /* 0x005CDC10 */
    virtual void DestroyControls() override;       /* 0x005CDC40 */
    virtual void Draw() override;                  /* 0x005CDC70 */
    virtual int GetDialogType() override;          /* 0x005CF240 */
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
    /* Move every flagged item to the destination: -1 copies it into the shared
       party pool, any other value gives it to that party slot's character.
       Each successful transfer unlinks the item and its flag; a failure plays
       the beep. An emptied picker closes itself. */
    void TransferSelectedItems005CE4C0(int destination);
    /* Handle one event the picker owns: keyboard list navigation and clicks. */
    unsigned char HandleInputEvent005CEC20(const InputAtom* input);

    /* Per-button callbacks stored through W8DialogButton::Configure.
       CloseOwningDialog005CE6E0 is public because AssayDialog also stores it
       on its close button. */
    static void ToggleAllItems005CE5F0(W8DialogButton* button);          /* 0x005CE5F0 */
    static void TakeSelectedToParty005CE6A0(W8DialogButton* button);     /* 0x005CE6A0 */
    static void TakeSelectedToCharacter005CE6C0(W8DialogButton* button); /* 0x005CE6C0 */

public:
    static void CloseOwningDialog005CE6E0(W8DialogButton* button); /* 0x005CE6E0 */

private:
    static void ToggleVisibleItem005CE6F0(W8DialogButton* button);   /* 0x005CE6F0 */
    static void ToggleVisibleItem005CE790(W8DialogButton* button);   /* 0x005CE790 */
    static void ToggleVisibleItem005CE830(W8DialogButton* button);   /* 0x005CE830 */
    static void ToggleVisibleItem005CE8D0(W8DialogButton* button);   /* 0x005CE8D0 */
    static void ShowVisibleItemInfo005CE970(W8DialogButton* button); /* 0x005CE970 */
    static void ShowVisibleItemInfo005CE9B0(W8DialogButton* button); /* 0x005CE9B0 */
    static void ShowVisibleItemInfo005CE9F0(W8DialogButton* button); /* 0x005CE9F0 */
    static void ShowVisibleItemInfo005CEA30(W8DialogButton* button); /* 0x005CEA30 */
    static void ScrollItemsUp005CEA70(W8DialogButton* button);       /* 0x005CEA70 */
    static void ScrollItemsDown005CEAB0(W8DialogButton* button);     /* 0x005CEAB0 */
    static void ScrollItemsToMouse005CEAF0(W8DialogButton* button);  /* 0x005CEAF0 */

public:
    W8GrowableVector<W8WorldItem*> items_54;
    W8GrowableVector<unsigned char> flags_64;
    W8DialogButton* m_buttons_74[13];
    int m_first_item_0a8;
    W8WorldItem* m_item_group_0ac;
};

static_assert(sizeof(W8TriggerItemPickerDialog) == 0xb0, "W8TriggerItemPickerDialog_must_be_0xb0");

/* The item-split dialog RCSItemsPage.cpp opens for stackable item stacks.
   Derivation is proven by the retail static_cast to W8DialogBase at the
   OpenSplitStackDialog005BA400 call site, the virtual SetText/SetOrigin calls
   on the result and DisplayCampDialog(W8DialogBase*). The constructor stores
   this vtable at +0; the listed slots are the ones that differ from
   W8DialogBase (the rest reuse the base implementations). split_count_0c0 is
   the count the destroy callback SplitStackDialogResult005BAA80 reads back
   and split_result_0c8 is the dialog result kind it tests. */
// VTABLE: WIZ8 0x005efb78
class W8SplitItemDialog : public W8DialogBase {
public:
    W8SplitItemDialog(int kind, W8ItemInstance* item, int count); /* 0x005DCED0 */
    virtual ~W8SplitItemDialog() override;                        /* 0x005DD030 */
    virtual int CreateControls() override;                        /* 0x005DD130 */
    virtual void DestroyControls() override;                      /* 0x005DD3C0 */
    virtual void Draw() override;                                 /* 0x005DDB60 */
    virtual unsigned char ProcessInput() override;                /* 0x005DE1B0 */
    virtual void OnNumericInputChanged(int value) override;       /* 0x005DDFA0 */

private:
    /* 0x005DD480: create and place the arrow, frame, accept and cancel
       buttons; eight for inventory splits, ten in trade modes. */
    unsigned char CreateButtons005DD480();
    /* 0x005DD750: create the label text buffers and fill the item-name rows. */
    unsigned char CreateTextBuffers005DD750();
    /* 0x005DDA60: create the count entry field over its backing button. */
    unsigned char CreateNumericInput005DDA60();
    /* 0x005DCC00: refresh the two trade-price labels in trade modes. */
    void UpdateCostLabels005DCC00();
    /* 0x005DDE60: enable the minus/plus arrows while each side has count. */
    void UpdateArrowStates005DDE60();
    /* 0x005DDEE0: enable accept when the split is nonzero and affordable. */
    void UpdateAcceptButton005DDEE0();
    /* 0x005DE120: numeric-field and Enter/Escape handling for ProcessInput. */
    unsigned char HandleInputEvent005DE120(const InputAtom* input);

    /* Per-button callbacks stored through W8DialogButton::Configure. */
    static void OnSplitDecrement005DE350(W8DialogButton* button);     /* 0x005DE350 */
    static void OnSplitIncrement005DE4E0(W8DialogButton* button);     /* 0x005DE4E0 */
    static void OnSplitDecrementMany005DE670(W8DialogButton* button); /* 0x005DE670 */
    static void OnSplitIncrementMany005DE810(W8DialogButton* button); /* 0x005DE810 */
    static void OnAccept005DE9B0(W8DialogButton* button);             /* 0x005DE9B0 */
    static void OnCancel005DE9D0(W8DialogButton* button);             /* 0x005DE9D0 */
    static void OnCountFieldClick005DE9F0(W8DialogButton* button);    /* 0x005DE9F0 */

public:
    W8DialogButton* m_buttons_054[10];       /* 0x054: minus/plus, frames, accept/cancel */
    W8TextBuffer* m_texts_07c[14];           /* 0x07c */
    W8DialogNumericInput* m_count_input_0b4; /* 0x0b4 */
    /* 0x0b8: the numeric field while a click or keypress owns it. */
    W8DialogNumericInput* m_active_input_0b8;
    int m_remaining_0bc; /* 0x0bc: the count left in the source stack */
    int split_count_0c0;
    int m_stack_total_0c4; /* 0x0c4: stack_count when the dialog opened */
    int split_result_0c8;

private:
    unsigned int m_kind_0cc;        /* 0x0cc: 0 inventory, 1 and 2 trade modes */
    W8ItemInstance* m_item_0d0;     /* 0x0d0 */
    unsigned char m_first_draw_0d4; /* 0x0d4: draw the item icon once */
    unsigned char unknown_0d5[3];
};

static_assert(sizeof(W8SplitItemDialog) == 0xd8, "W8SplitItemDialog_must_be_0xd8");
