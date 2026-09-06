#pragma once

#include "wiz8/dialog_code/DialogBase.h"

class W8MonsterInfoDialog : public W8DialogBase005DC7A0 {
public:
    virtual ~W8MonsterInfoDialog() override;
    virtual int vslot1() override;
    virtual void ResetSubobjectAndRefresh() override;
    virtual void vslot3() override;
    virtual void ClearField41IfEnabled() override;
    virtual void vslot13(int value) override;

private:
    void* m_constructor_argument_54;      /* 0x54 */
    W8DialogMember005E0C40 m_member_58;  /* 0x58 */
    W8DialogMember005DB1B0 m_member_a4;  /* 0xa4 */
    W8DialogMember005D14D0 m_member_ec;  /* 0xec */
};                                      /* modeled minimum 0x144 */
