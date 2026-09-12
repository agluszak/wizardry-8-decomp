#include "wiz8/local_screens/MGSKeyboard.h"
#include "input.h"
#include "wiz8/local_screens/CreditsScreen.h"
#include "wiz8/sr_api.h"
#include "wiz8/utility.h"
#include "wiz8/virtual_file.h"

#include "FileMan.h"
#include "input.h"

#include <wchar.h>
#include <wctype.h>

/* Local Screens\MGSKeyboard.cpp owns the binding vector, its command-keyed
   lookup and the singleton ResetMGSKeyboardBindings installs.
   MGSKeyboard::LoadDefaults moved to Local Code\InputMapper.cpp. */

// GLOBAL: WIZ8 0x0069b7e4
MGSKeyboard* g_mgs_keyboard;

// FUNCTION: WIZ8 0x0055CFD0
MGSKeyboard::MGSKeyboard() {}

// SYNTHETIC: WIZ8 0x0055D160
// MGSKeyboard::`scalar deleting destructor'

// FUNCTION: WIZ8 0x0055D180
MGSKeyboard::~MGSKeyboard()
{
    Clear();
}

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

// FUNCTION: WIZ8 0x0055d320
unsigned char MGSKeyboard::IsCommandPressed(unsigned int command) const
{
    MGSKeyBinding* binding = m_command_index.Lookup(&command);
    if (binding != 0 && gfKeyState[binding->key] != 0) {
        unsigned short modifiers = 0;
        if (gfKeyState[VK_SHIFT] != 0) {
            modifiers |= SHIFT_DOWN;
        }
        if (gfKeyState[VK_MENU] != 0) {
            modifiers |= ALT_DOWN;
        }
        if (gfKeyState[VK_CONTROL] != 0) {
            modifiers |= CTRL_DOWN;
        }
        if (modifiers == binding->modifiers) {
            return 1;
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x0055D3F0
void MGSKeyboard::Clear()
{
    for (int index = m_bindings.count - 1; index >= 0; --index) {
        delete m_bindings.RemoveAt(index);
    }
    m_command_index.Clear();
}

// FUNCTION: WIZ8 0x0055d590
unsigned char MGSKeyboard::Load(int handle, unsigned char clear)
{
    int count;

    if (clear != 0) {
        Clear();
    }
    FileRead(handle, &count, sizeof(count), 0);
    for (int index = 0; index < count; ++index) {
        MGSKeyBinding* binding = new MGSKeyBinding;
        FileRead(handle, binding, sizeof(*binding), 0);

        int old_index = FindBinding(binding->command);
        if (old_index != -1) {
            unsigned int command = binding->command;
            delete m_bindings.RemoveAt(old_index);
            m_command_index.Remove(&command);
        }
        if (m_bindings.Add(binding) != -1) {
            unsigned int command = binding->command;
            m_command_index.Insert(&command, &binding);
        }
    }
    return 1;
}

// FUNCTION: WIZ8 0x0055d7a0
unsigned char MGSKeyboard::Save(int handle) const
{
    int count = m_bindings.GetCount();
    FileWrite(handle, &count, sizeof(count), 0);
    for (int index = 0; index < count; ++index) {
        FileWrite(handle, *m_bindings.GetAt(index), sizeof(MGSKeyBinding), 0);
    }
    return 1;
}

// FUNCTION: WIZ8 0x00592BE0
void ResetMGSKeyboardBindings()
{
    if (g_mgs_keyboard == 0) {
        g_mgs_keyboard = new MGSKeyboard;
    } else {
        g_mgs_keyboard->Clear();
    }
    g_mgs_keyboard->LoadDefaults("Data\\Strings\\MGSKeyboard.ini");
}
