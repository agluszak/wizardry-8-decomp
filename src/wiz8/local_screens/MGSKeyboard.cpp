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

#pragma pack(push, 1)
struct MGSKeyName {
    const wchar_t* name;
    unsigned short key;
};

// GLOBAL: WIZ8 0x00647770
static const MGSKeyName g_mgs_key_names[] = {
    {L"F1", 112}, {L"F2", 113}, {L"F3", 114}, {L"F4", 115},
    {L"F5", 116}, {L"F6", 117}, {L"F7", 118}, {L"F8", 119},
    {L"F9", 120}, {L"F10", 121}, {L"F11", 122}, {L"F12", 123},
    {L"ESC", 27}, {L"TAB", 9}, {L"PAUSE", 19}, {L"NUM_LOCK", 144},
    {L"BACKSPACE", 8}, {L"INSERT", 45}, {L"DEL", 46}, {L"KEY_END", 35},
    {L"PGDN", 34}, {L"PGUP", 33}, {L"HOME", 36}, {L"ENTER", 13},
    {L"SPACE", 32}, {L"DNARROW", 40}, {L"LEFTARROW", 37},
    {L"RIGHTARROW", 39}, {L"UPARROW", 38}, {L"NUM_0", 96},
    {L"NUM_1", 97}, {L"NUM_2", 98}, {L"NUM_3", 99}, {L"NUM_4", 100},
    {L"NUM_5", 101}, {L"NUM_6", 102}, {L"NUM_7", 103}, {L"NUM_8", 104},
    {L"NUM_9", 105}, {L"NUM_TIMES", 106}, {L"NUM_PLUS", 107},
    {L"NUM_ENTER", 108}, {L"NUM_MINUS", 109}, {L"NUM_PERIOD", 110},
    {L"NUM_SLASH", 111}, {L"SCRL_LOCK", 145}, {L"/", 191},
    {L"-", 189}, {L".", 190}
};
#pragma pack(pop)

// GLOBAL: WIZ8 0x0069b7e4
MGSKeyboard* g_mgs_keyboard;

// FUNCTION: WIZ8 0x0055CFD0
MGSKeyboard::MGSKeyboard()
{
}

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

// FUNCTION: WIZ8 0x0055D800
unsigned char MGSKeyboard::LoadDefaults(const char* path)
{
    int handle = FileOpen((char*)path, FILE_ACCESS_READ, 0);
    if (handle == 0) {
        srAssertFail(
            "hFile", "C:\\Projects\\Wizardry 8\\Local Code\\InputMapper.cpp",
            478, FormatString("Couldn't open keyboard init file %s", path));
    }

    wchar_t line[200];
    unsigned char more;
    while (!FileCheckEndOfFile(handle)) {
        if (!ReadWideTextLine004CEED0(handle, line, 200, &more) ||
            line[0] == L'*') {
            continue;
        }

        wchar_t* token = wcstok(line, L" \t\r\n");
        if (token == 0 || wcslen(token) == 0 || !iswdigit(*token)) {
            continue;
        }
        int command = _wtoi(token);
        token = wcstok(0, L" \t\r\n");
        if (token == 0) {
            continue;
        }

        unsigned short key = *token;
        int index;
        for (index = 0;
             index < (int)(sizeof(g_mgs_key_names) / sizeof(g_mgs_key_names[0]));
             ++index) {
            if (wcscmp(token, g_mgs_key_names[index].name) == 0) {
                key = g_mgs_key_names[index].key;
                break;
            }
        }
        if (index == (int)(sizeof(g_mgs_key_names) / sizeof(g_mgs_key_names[0]))) {
            if (wcslen(token) != 1 ||
                (key = TranslateCharacterToKey(key)) == 0) {
                continue;
            }
        }

        unsigned short modifiers = 0;
        while ((token = wcstok(0, L" \t\r\n")) != 0) {
            if (wcscmp(token, L"SHIFT_DOWN") == 0) {
                modifiers |= SHIFT_DOWN;
            }
            else if (wcscmp(token, L"CTRL_DOWN") == 0) {
                modifiers |= CTRL_DOWN;
            }
            else if (wcscmp(token, L"ALT_DOWN") == 0) {
                modifiers |= ALT_DOWN;
            }
        }

        MGSKeyBinding* binding = new MGSKeyBinding;
        binding->key = key;
        binding->modifiers = modifiers;
        binding->unknown_004 = 1;
        binding->command = command;

        int old_index = FindBinding(command);
        if (old_index != -1) {
            delete m_bindings.RemoveAt(old_index);
            m_command_index.Remove((const unsigned int*)&command);
        }
        if (m_bindings.Add(binding) != -1) {
            m_command_index.Insert((const unsigned int*)&command, &binding);
        }
    }
    FileClose(handle);
    return 1;
}

// FUNCTION: WIZ8 0x00592BE0
void ResetMGSKeyboardBindings()
{
    if (g_mgs_keyboard == 0) {
        g_mgs_keyboard = new MGSKeyboard;
    }
    else {
        g_mgs_keyboard->Clear();
    }
    g_mgs_keyboard->LoadDefaults("Data\\Strings\\MGSKeyboard.ini");
}
