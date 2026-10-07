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

enum W8ItemSplitMode { W8_ITEM_SPLIT_INVENTORY = 0, W8_ITEM_SPLIT_SELL = 1, W8_ITEM_SPLIT_BUY = 2 };

enum W8SplitDialogResult {
    W8_SPLIT_RESULT_PENDING = 0,
    W8_SPLIT_RESULT_CONFIRMED = 1,
    W8_SPLIT_RESULT_CANCELLED = 2
};

/* The small numeric entry field embedded by the split dialogs. */
class W8DialogNumericInput {
public:
    W8DialogNumericInput(int control_id, const W8ControlsRect* bounds, int value, int font,
                         W8DialogBase* dialog, W8DialogButton* button);
    void SetValue(int value);
    void SetActive(bool active);
    void SetActive(bool active, const POINT* point);
    void Draw(bool force);
    bool HandleInput(const InputAtom* input);

private:
    /* Write a digit character at the caret position and re-parse
       the text into m_value, rejecting results above m_maximum. */
    void TypeDigit(wchar_t digit);
    void NotifyValueChanged();
    /* VK_DELETE — remove the character right of the caret. */
    void DeleteForward();
    /* VK_BACK — remove the character left of the caret. */
    void Backspace();

public:
    W8ControlsRect m_bounds;
    /* -1 while inactive; otherwise the count of characters to the right
       of the caret. */
    int m_caret;
    int m_font;
    int m_value;
    bool m_dirty;
    bool m_active;
    /* The stack total for the split dialogs; -1 is "no maximum". */
    unsigned int m_maximum;
    W8DialogBase* m_dialog;
    W8DialogButton* m_button;
    /* Echoed back as the argument of m_dialog->OnNumericInputChanged. */
    int m_control_id;
};
static_assert(sizeof(W8DialogNumericInput) == 0x30, "W8DialogNumericInput_size");

// VTABLE: WIZ8 0x005ef7c8
class W8ListBoxDialog : public W8DialogBase {
public:
    W8ListBoxDialog();
    virtual ~W8ListBoxDialog() override;
    virtual int CreateControls() override;
    virtual void DestroyControls() override;
    virtual void Draw() override;
    virtual W8DialogKind GetDialogType() override;
    virtual void SetText(const wchar_t* text) override;
    virtual bool ProcessInput() override;

private:
    /* Text-area button height divided by the dialog font height. */
    int GetVisibleLineCount();
    /* Select a text line and scroll it into the visible range. */
    void SetCurrentLine(int line);
    void SetFirstVisibleLine(int line);
    void ClearLines();
    /* Keyboard handling for the visible text list. */
    bool HandleInputEvent(const InputAtom* input);

    /* SGP move/click callbacks for the text area, the scroll arrow buttons and
       the confirmation buttons. */
    static void TextAreaButtonCallback(GUI_BUTTON* button, INT32 reason);
    static void UpButtonCallback(GUI_BUTTON* button, INT32 reason);
    static void DownButtonCallback(GUI_BUTTON* button, INT32 reason);
    static void OkButtonCallback(GUI_BUTTON* button, INT32 reason);
    static void CancelButtonCallback(GUI_BUTTON* button, INT32 reason);
    static void SliderTrackButtonCallback(GUI_BUTTON* button, INT32 reason);

public:
    /* The displayed text lines; the dialog owns and frees each one. */
    W8GrowableVector<wchar_t*> m_lines;
    W8GrowableVector<void (*)(int)> m_field_064;
    int m_field_074;
    float m_field_078; /* 0.05 */
    float m_field_07c; /* 0.2 */
    float m_field_080; /* 0.9 */
    float m_field_084; /* 0.75 */
    int m_fill_colour; /* highlight fill colour */
    int m_text_button;
    int m_second_text_button;
    short area_inlay;  /* DialogInlay inlay for m_area_button */
    int m_area_button; /* scrolling text area */
    int m_up_button;
    int m_up_image;
    int m_down_button;
    int m_down_image;
    int m_slider_button;
    int m_slider_image;
    short third_btn_inlay; /* DialogInlay inlay for m_third_text_button */
    int m_third_text_button;
    int m_ok_button;
    int m_ok_image;
    /* Click rectangles read by ProcessInput; nothing writes them. */
    W8ControlsRect m_ok_rect;
    int m_cancel_button;
    int m_cancel_image;
    W8ControlsRect m_cancel_rect;
    bool m_scrollable; /* scrolling area is scrollable */
    int m_first_visible_line;
    int m_selected_line;
    short edge_inlay; /* DialogEdge inlay for the text buttons */
    short flags;
};

// VTABLE: WIZ8 0x005ef9f0
class W8SplitAmountDialog : public W8DialogBase {
    /* NPCInteractionSubscreen's destroy callback reads m_taken and
       m_result back out of the closing dialog. */
    friend void OnNpcTradeSplitDialogDestroy(W8DialogBase* dialog);

public:
    W8SplitAmountDialog();
    /* The split-size entry point; the pool total seeds both the
       remaining and total fields. */
    W8SplitAmountDialog(int total);
    virtual ~W8SplitAmountDialog() override;
    virtual int CreateControls() override;
    virtual void DestroyControls() override;
    virtual void Draw() override;
    virtual bool ProcessInput() override;
    virtual void OnNumericInputChanged(int value) override;

private:
    /* Create and place the six dialog buttons. */
    bool CreateButtons();
    /* Create the three text buffers above the numeric field. */
    bool CreateTextBuffers();
    /* Create the numeric entry field and its backing button. */
    bool CreateNumericInput();
    /* Enable the plus/minus buttons from the current split. */
    void UpdateButtonStates();
    /* Push the split into the text buffers and the numeric field. */
    void UpdateTextBuffers();
    /* Keyboard handling for the plus/minus buttons and the field. */
    void DecreaseAmount(int amount);
    void IncreaseAmount(int amount);
    bool HandleInputEvent(const InputAtom* input);

    /* Per-button callbacks stored through W8DialogButton::Configure. */
    static void SplitDecrementOne(W8DialogButton* button);
    static void SplitDecrementFive(W8DialogButton* button);
    static void SplitIncrementOne(W8DialogButton* button);
    static void SplitIncrementFive(W8DialogButton* button);
    static void SplitAccept(W8DialogButton* button);
    static void SplitCancel(W8DialogButton* button);
    static void SplitActivateField(W8DialogButton* button);

private:
    W8DialogButton* m_buttons[6];
    /* The labels named by g_split_amount_string_ids; [2] shows the
       amount still remaining. */
    W8TextBuffer* m_text_buffers[3];
    W8DialogNumericInput* m_split_input;
    /* The numeric field while the cursor or keyboard owns it. */
    W8DialogNumericInput* m_active_field;
    int m_remaining; /* total minus the field value */
    int m_taken;     /* the field value */
    int m_total;
    W8SplitDialogResult m_result; /* 1 confirms, 2 cancels */
};

static_assert(sizeof(W8ListBoxDialog) == 0xfc, "W8ListBoxDialog005CBB40_must_be_0xfc");
static_assert(sizeof(W8SplitAmountDialog) == 0x90, "W8SplitAmountDialog_must_be_0x90");

/* The trigger-owned item picker. The item group is what the trigger hands in
   and the destroy callback hands back. */
// VTABLE: WIZ8 0x005ef810
class W8TriggerItemPickerDialog : public W8DialogBase {
public:
    W8TriggerItemPickerDialog();
    virtual ~W8TriggerItemPickerDialog() override;
    virtual int CreateControls() override;
    virtual void DestroyControls() override;
    virtual void Draw() override;
    virtual W8DialogKind GetDialogType() override;
    virtual bool ProcessInput() override;

    int AddItem(W8WorldItem* item);
    W8WorldItem* ReturnItemsToGroup();
    void SetItemGroup(W8WorldItem* group);

private:
    bool CreateButtons();
    /* Clamp and apply the first visible item row. Fewer than five items force
       the first row. Out-of-range input is ignored, not clamped. */
    void SetFirstVisible(int index);
    /* Sync the four scroll buttons' pressed/visible state with the scroll
       offset and the per-item enable flags. */
    void RefreshScrollButtons();
    bool IsItemSelected(int index);
    void SetItemSelected(int index, bool selected);
    void ToggleItem(int index, W8DialogButton* button);
    static void ShowVisibleItemInfo(W8DialogButton* button, int row);
    /* Move every flagged item to the destination: -1 copies it into the shared
       party pool, any other value gives it to that party slot's character.
       Each successful transfer unlinks the item and its flag; a failure plays
       the beep. An emptied picker closes itself. */
    void TransferSelectedItems(int destination);
    /* Handle one event the picker owns: keyboard list navigation and clicks. */
    bool HandleInputEvent(const InputAtom* input);

    /* Per-button callbacks stored through W8DialogButton::Configure.
       CloseOwningDialog is public because AssayDialog also stores it
       on its close button. */
    static void ToggleAllItems(W8DialogButton* button);
    static void TakeSelectedToParty(W8DialogButton* button);
    static void TakeSelectedToCharacter(W8DialogButton* button);

public:
    static void CloseOwningDialog(W8DialogButton* button);

private:
    static void ToggleVisibleItem0(W8DialogButton* button);
    static void ToggleVisibleItem1(W8DialogButton* button);
    static void ToggleVisibleItem2(W8DialogButton* button);
    static void ToggleVisibleItem3(W8DialogButton* button);
    static void ShowVisibleItemInfo0(W8DialogButton* button);
    static void ShowVisibleItemInfo1(W8DialogButton* button);
    static void ShowVisibleItemInfo2(W8DialogButton* button);
    static void ShowVisibleItemInfo3(W8DialogButton* button);
    static void ScrollItemsUp(W8DialogButton* button);
    static void ScrollItemsDown(W8DialogButton* button);
    static void ScrollItemsToMouse(W8DialogButton* button);

public:
    W8Vector<W8WorldItem*> items;
    W8GrowableVector<unsigned char> flags;
    W8DialogButton* m_buttons[13];
    int m_first_item;
    W8WorldItem* m_item_group;
};

static_assert(sizeof(W8TriggerItemPickerDialog) == 0xb0, "W8TriggerItemPickerDialog_must_be_0xb0");

/* The item-split dialog opened for stackable item stacks. The destroy
   callback reads back split_count and tests split_result. */
// VTABLE: WIZ8 0x005efb78
class W8SplitItemDialog : public W8DialogBase {
public:
    W8SplitItemDialog(W8ItemSplitMode mode, W8ItemInstance* item, int count);
    virtual ~W8SplitItemDialog() override;
    virtual int CreateControls() override;
    virtual void DestroyControls() override;
    virtual void Draw() override;
    virtual bool ProcessInput() override;
    virtual void OnNumericInputChanged(int value) override;

private:
    /* Create and place the arrow, frame, accept and cancel
       buttons; eight for inventory splits, ten in trade modes. */
    bool CreateButtons();
    void DestroyButtons();
    /* Create the label text buffers and fill the item-name rows. */
    bool CreateTextBuffers();
    void DestroyTextBuffers();
    /* Create the count entry field over its backing button. */
    bool CreateNumericInput();
    /* Refresh the two trade-price labels in trade modes. */
    void UpdateCostLabels();
    /* Enable the minus/plus arrows while each side has count. */
    void UpdateArrowStates();
    /* Enable accept when the split is nonzero and affordable. */
    void UpdateAcceptButton();
    /* Shared refresh for the numeric-field and split-count callbacks. */
    void UpdateTotals();
    /* Numeric-field and Enter/Escape handling for ProcessInput. */
    bool HandleInputEvent(const InputAtom* input);

    /* Per-button callbacks stored through W8DialogButton::Configure. */
    static void OnSplitDecrement(W8DialogButton* button);
    static void OnSplitIncrement(W8DialogButton* button);
    static void OnSplitDecrementMany(W8DialogButton* button);
    static void OnSplitIncrementMany(W8DialogButton* button);
    static void OnAccept(W8DialogButton* button);
    static void OnCancel(W8DialogButton* button);
    static void OnCountFieldClick(W8DialogButton* button);

public:
    W8DialogButton* m_buttons[10]; /* minus/plus, frames, accept/cancel */
    W8TextBuffer* m_texts[14];
    W8DialogNumericInput* m_count_input;
    /* The numeric field while a click or keypress owns it. */
    W8DialogNumericInput* m_active_input;
    int m_remaining; /* the count left in the source stack */
    int split_count;
    int m_stack_total; /* stack_count when the dialog opened */
    W8SplitDialogResult split_result;

private:
    W8ItemSplitMode m_mode; /* 0 inventory, 1 and 2 trade modes */
    W8ItemInstance* m_item;
    bool m_first_draw; /* draw the item icon once */
};

static_assert(sizeof(W8SplitItemDialog) == 0xd8, "W8SplitItemDialog_must_be_0xd8");

extern const W8SplitDialogResult g_amount_split_confirm_result;
extern const W8SplitDialogResult g_item_split_confirm_result;
extern const W8ItemSplitMode g_item_split_inventory_mode;
extern const W8ItemSplitMode g_item_split_sell_mode;
extern const W8ItemSplitMode g_item_split_buy_mode;
