#pragma once

#include "wiz8/dialog_code/DialogBase.h"
#include "wiz8/dialog_code/DialogButton.h"
#include "wiz8/vector.h"

#include "input.h"

struct W8WorldItem;
class Trigger;

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
    W8DialogButton* m_fields_54[6];
    W8DialogButton* m_field_6c;
    W8DialogButton* m_field_70;
    W8DialogButton* m_field_74;
    void* m_field_78;
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
    /* Sync the four scroll buttons' pressed/visible state with the scroll
       offset and the per-item enable flags. */
    void RefreshScrollButtons005CE420();
    /* Handle one event the picker owns: keyboard list navigation and clicks. */
    void HandleInputEvent005CEC20(const InputAtom* input);

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
