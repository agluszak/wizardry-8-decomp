#pragma once

#include "wiz8/dialog_code/DialogBase.h"

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
