#include "wiz8/local_screens/MGSKeyboard.h"

// GLOBAL: WIZ8 0x0069b7e4
MGSKeyboard* g_mgs_keyboard;

// FUNCTION: WIZ8 0x0055d260
int MGSKeyboard::FindBinding(int command) const
{
    int count = m_bindings.GetCount();
    int index = 0;
    if (count > 0) {
        do {
            MGSKeyBinding* binding = *m_bindings.GetAt(index);
            if (binding->command == command) {
                return index;
            }
            ++index;
        } while (index < count);
    }
    return -1;
}

// FUNCTION: WIZ8 0x0055d300
MGSKeyBinding* MGSKeyboard::GetBinding(int index) const
{
    if (index >= 0) {
        MGSKeyBinding** binding = m_bindings.data;
        if (index < m_bindings.count) {
            binding += index;
        }
        return *binding;
    }
    return 0;
}
