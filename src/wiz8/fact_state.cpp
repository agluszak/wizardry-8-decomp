#include "wiz8/fact_state.h"
#include "wiz8/character.h"
#include "wiz8/combat_state.h"
#include "wiz8/factions.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/local_code/NPCManager.h"
#include "wiz8/local_code/NPCScripting.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_screens/JournalScreen.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/utility.h"
#include "wiz8/npc_state.h"
#include "wiz8/virtual_file.h"
#include "wiz8/xstatus.h"

#include "soundman.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

// GLOBAL: WIZ8 0x00688290
unsigned char g_log_fact_checks;
// GLOBAL: WIZ8 0x0068de63
unsigned char g_import_party_loaded;
// GLOBAL: WIZ8 0x0068de5d
int g_import_ending_choice;
// GLOBAL: WIZ8 0x00689b78
unsigned char g_fact_values[1001];
// GLOBAL
unsigned char g_import_flags[0x60];

/* 0x0055A0A0 and 0x00524CA0 tear down an NPC item list. */
extern void Function55A0A0(int handle);
extern void Function524CA0(W8NpcState* npc);

// FUNCTION: WIZ8 0x00506280
unsigned char GetFact(int fact_id)
{
    unsigned char value;
    wchar_t display_value[10];

    if (fact_id > 1000) {
        return 0;
    }

    value = EvaluateFact(fact_id);
    if (g_log_fact_checks) {
        if (value) {
            wcscpy(display_value, L"TRUE");
        }
        else {
            wcscpy(display_value, L"FALSE");
        }
        WriteGameLog(
            5,
            L"Checking fact %S which is %s",
            g_fact_records[fact_id].symbolic_name,
            display_value);
    }
    return value;
}

// FUNCTION: WIZ8 0x005061a0
void SetFact(
    int fact_id, unsigned char value, unsigned char suppress_side_effects)
{
    unsigned char previous_value;
    wchar_t display_value[10];

    if (fact_id > 1000) {
        return;
    }

    previous_value = g_fact_values[fact_id];
    g_fact_values[fact_id] = value;

    if (fact_id < (int)gXStatus.uiFactsInDatabase) {
        if (value) {
            sprintf((char*)display_value, "TRUE");
        }
        else {
            sprintf((char*)display_value, "FALSE");
        }
    }

    if (!suppress_side_effects) {
        if (g_fact_values[fact_id] != previous_value) {
            RecordFactChangeForJournal(fact_id);
        }
        HandleFactChange(fact_id, value);

        if (g_log_fact_checks) {
            if (value) {
                wcscpy(display_value, L"TRUE");
            }
            else {
                wcscpy(display_value, L"FALSE");
            }
            WriteGameLog(
                5,
                L"%S set to %s",
                g_fact_records[fact_id].symbolic_name,
                display_value);
        }
    }
}

/* The whole 1001-byte fact array minus its last entry goes to the save file in
   one write. The original passes the address of its own parameter as the
   bytes-written out-parameter: the handle has already been copied into a
   register, so the incoming slot is dead and doubles as the scratch the callee
   requires. Reproduced literally, because a separate local would cost a stack
   frame the canonical body does not have. */
// FUNCTION: WIZ8 0x00506480
void SaveFactState(int save_handle)
{
    FileWrite(save_handle, g_fact_values, 1000, (unsigned int*)&save_handle);
}

/* Clears every fact, then seeds the ones a fresh party starts with. A party
   imported from Wizardry 7 gets a different set, keyed off the option byte and
   flag mask the importer unpacked.
   The canonical body clears the suppression flag twice, once inside the
   innermost branch ahead of an early return and once at the exit. That
   duplication is VC6's, not the source's: writing both out costs a byte,
   because the compiler then merges the two argument cleanups into one
   add esp,0x10 where the original keeps an add esp,0xc and a pop ecx. One
   trailing call, duplicated by the compiler, is byte-exact. */
// FUNCTION: WIZ8 0x00506310
void InitializeFactState(void)
{
    memset(g_fact_values, 0, 1000);
    SetFactNotificationsSuppressed(1);
    if (g_import_party_loaded) {
        SetFact(0x75, 1, 0);
        switch (g_import_ending_choice) {
        case 1:
            SetFact(0x4c, 1, 0);
            break;
        case 2:
            SetFact(0x4b, 1, 0);
            break;
        default:
            SetFact(0x4d, 1, 0);
            break;
        }
        if (g_import_flags[0x0b]) {
            SetFact(0x199, 1, 0);
        }
        if (g_import_flags[0x05]) {
            SetFact(0x7b, 1, 0);
        }
    } else {
        SetFact(0x4e, 1, 0);
        SetFact(0x279, 1, 0);
        SetFact(0x27a, 1, 0);
        SetFact(0x27b, 1, 0);
    }
    SetFactNotificationsSuppressed(0);
}

/* The four fact checks in LoadFactState share one block that VC6 inlined at
   each site: evaluate the fact, and when logging is enabled copy TRUE or FALSE
   into a local and print it beside the fact's symbolic name. Written as an
   inline helper rather than four times, so the shared wide buffer stays a
   single local. */
static __inline unsigned char CheckFactLogged(int fact_id)
{
    unsigned char value;
    wchar_t text[10];

    value = EvaluateFact(fact_id);
    if (g_log_fact_checks) {
        if (value) {
            wcscpy(text, L"TRUE");
        } else {
            wcscpy(text, L"FALSE");
        }
        WriteGameLog(5, L"Checking fact %S which is %s",
                     g_fact_records[fact_id].symbolic_name, text);
    }
    return value;
}

/* Reads the fact array back, then re-applies the consequences that do not
   survive a save. As in SaveFactState the handle's own incoming slot doubles as
   the bytes-read scratch. */
// FUNCTION: WIZ8 0x005064a0
void LoadFactState(int save_handle)
{
    W8NpcState* npc;

    FileRead(save_handle, g_fact_values, 1000, (unsigned int*)&save_handle);
    if (CheckFactLogged(0x44)) {
        npc = GetNpcStateByKind(0x20);
        if (npc && npc->has_monster) {
            Function55A0A0(npc->unknown_00);
            Function524CA0(npc);
        }
    }
    if (!CheckFactLogged(0x4b)) {
        if (!CheckFactLogged(0x4c)) {
            if (!CheckFactLogged(0x4e)) {
                SetFact(0x4d, 1, 0);
            }
        }
    }
}

/* Evaluate one derived fact from the party's live state. Most facts either
   read the stored fact array directly or combine a handful of primary facts;
   the hard-coded rules below are the original's. The helper evaluates
   recursively for the combined facts and logs each dependency when the
   fact-check log is enabled. */
// FUNCTION: WIZ8 0x005080f0
unsigned char EvaluateFact(int fact_id)
{
    wchar_t text[10];
    unsigned char value;
    unsigned char count;

    if (fact_id < 0xcc) {
        if (fact_id == 0xcb) {
            return GetFactionDisposition(10) == W8_FACTION_FRIENDLY;
        }
        switch (fact_id) {
        case 0x2c:
        case 0x2e:
        triple:
            count = FindItemOnParty(0x243, 0, 0, 2, 0) != 0;
            if (FindItemOnParty(0x242, 0, 0, 2, 0) != 0) {
                ++count;
            }
            if (FindItemOnParty(0x244, 0, 0, 2, 0) != 0) {
                ++count;
            }
            if (fact_id == 0x103) {
                return count == 1;
            }
            if (fact_id == 0x2e) {
                return count == 2;
            }
            if (fact_id == 0x2c) {
                return count == 3;
            }
            return 0;
        case 0x3d:
            value = EvaluateFact(0x3a);
            if (g_log_fact_checks) {
                wcscpy(text, value ? L"TRUE" : L"FALSE");
                Function58AAD0(5, L"Checking fact %S which is %s",
                               g_fact_records[58].symbolic_name, text);
            }
            if (value == 0) {
                value = EvaluateFact(0x3c);
                if (g_log_fact_checks) {
                    wcscpy(text, value ? L"TRUE" : L"FALSE");
                    Function58AAD0(5, L"Checking fact %S which is %s",
                                   g_fact_records[60].symbolic_name, text);
                }
                if (value == 0) {
                    return 1;
                }
            }
            return 0;
        case 0x4f:
            if (Function50B8F0(0x11) == 0 || Function50B8F0(0x10) == 0) {
                return 0;
            }
            break;
        case 0x5b:
            count = FindItemOnParty(0x242, 0, 0, 2, 0) != 0;
            if (FindItemOnParty(0x243, 0, 0, 2, 0) != 0) {
                ++count;
            }
            if (FindItemOnParty(0x244, 0, 0, 2, 0) != 0) {
                ++count;
            }
            return count >= 2;
        case 0x69: {
            W8NpcState* npc = GetNpcStateByKind(0x2b);
            if (npc == 0) {
                return 0;
            }
            return (unsigned char)npc->unknown_04;
        }
        case 0x81:
            value = EvaluateFact(0x86);
            if (g_log_fact_checks) {
                wcscpy(text, value ? L"TRUE" : L"FALSE");
                Function58AAD0(5, L"Checking fact %S which is %s",
                               g_fact_records[134].symbolic_name, text);
            }
            if (value == 0) {
                value = EvaluateFact(0x83);
                if (g_log_fact_checks) {
                    wcscpy(text, value ? L"TRUE" : L"FALSE");
                    Function58AAD0(5, L"Checking fact %S which is %s",
                                   g_fact_records[131].symbolic_name, text);
                }
                if (value != 0 && FindItemOnParty(0x271, 0, 0, 2, 0) == 0 &&
                    FindItemOnParty(0x272, 0, 0, 2, 0) == 0) {
                    return 1;
                }
            }
            return 0;
        case 0x88:
            if (g_status_685170.unknown_2498[0x1c29] == 0) {
                if (CountItemOnParty(0x1c4, 0, 0, 2) < 5) {
                    return 0;
                }
                g_status_685170.unknown_2498[0x1c29] = 1;
                return 1;
            }
            break;
        case 0x8f:
            return EveryCharacterHasItem(0x1e5, 0);
        case 0x93:
            return CountLeadingPartySlots() == 2;
        case 0xa0:
            return FindItemOnParty(0x268, 0, 0, 2, 0);
        case 0xab:
            return FindItemOnParty(0x27b, 0, 0, 2, 0);
        case 0xb5: {
            unsigned int slot = 0;
            while (g_party_slot_rows[slot].occupied == 0 ||
                   g_party_characters[slot].race != 10 ||
                   g_party_characters[slot].unknown_0b01 > 0xe) {
                if (slot >= 7) {
                    return 0;
                }
                ++slot;
            }
            break;
        }
        case 0xbd:
            return Function50B8F0(0x10) != 0;
        case 0xbe:
            return Function50B8F0(0x11) != 0;
        case 0xc3:
            return Function50B8F0(0x18) != 0;
        case 0xc9:
            return GetFactionDisposition(11) == W8_FACTION_FRIENDLY;
        case 0xca:
            return GetFactionDisposition(12) == W8_FACTION_FRIENDLY;
        }
    }
    else if (fact_id < 0x195) {
        if (fact_id == 0x194) {
            return GetFactionDisposition(13) == W8_FACTION_FRIENDLY;
        }
        switch (fact_id) {
        case 0xcc:
            return GetFactionDisposition(9) == W8_FACTION_FRIENDLY;
        case 0xce:
            return FindItemOnParty(0x294, 0, 0, 2, 0);
        case 0xd1:
            return Function50B8F0(7) != 0;
        case 0x103:
            goto triple;
        case 0x10c:
            value = EvaluateFact(0x22);
            if (g_log_fact_checks) {
                wcscpy(text, value ? L"TRUE" : L"FALSE");
                Function58AAD0(5, L"Checking fact %S which is %s",
                               g_fact_records[34].symbolic_name, text);
            }
            if (value != 0) {
                return 1;
            }
            value = EvaluateFact(0x30);
            if (g_log_fact_checks) {
                wcscpy(text, value ? L"TRUE" : L"FALSE");
                Function58AAD0(5, L"Checking fact %S which is %s",
                               g_fact_records[48].symbolic_name, text);
            }
            if (value != 0) {
                return 1;
            }
            value = EvaluateFact(0x31);
            if (g_log_fact_checks) {
                wcscpy(text, value ? L"TRUE" : L"FALSE");
                Function58AAD0(5, L"Checking fact %S which is %s",
                               g_fact_records[49].symbolic_name, text);
            }
            if (value != 0) {
                return 1;
            }
            return 0;
        case 0x11e:
            return GetFactionDisposition(5) == W8_FACTION_FRIENDLY;
        case 0x14c:
            if (g_status_685170.unknown_244b[0x3e] != 0) {
                int* marked_slot = reinterpret_cast<int*>(g_status_685170.unknown_2498 + 0x1da5); /* reinterpret-ok: packed status table */
                unsigned int slot = 0;
                do {
                    if (g_party_slot_rows[slot].occupied != 0 &&
                        slot == (unsigned int)*marked_slot) {
                        return 1;
                    }
                    ++slot;
                } while (slot < 8);
            }
            return 0;
        case 0x172:
            return FindItemOnParty(0x239, 0, 0, 2, 0);
        case 0x183:
            return GetFactionDisposition(4) == W8_FACTION_FRIENDLY;
        }
    }
    else if (fact_id < 0x26a) {
        if (fact_id == 0x269) {
            return FindItemOnParty(0x239, 0, 0, 2, 0) == 0;
        }
        switch (fact_id) {
        case 0x19a:
            return Function50B8F0(0x38) != 0;
        case 0x1a8:
            return GetFactionDisposition(15) == W8_FACTION_FRIENDLY;
        case 0x216: {
            if (Function50B8F0(0x18) == 0) {
                return g_fact_values[fact_id];
            }
            W8NpcState* npc = GetNpcStateByKind(0x18);
            if (npc != 0 &&
                g_party_characters[npc->group_index].unknown_0b01 >= 0xf) {
                return 1;
            }
            return 0;
        }
        case 0x265:
            value = EvaluateFact(0x268);
            if (g_log_fact_checks) {
                wcscpy(text, value ? L"TRUE" : L"FALSE");
                Function58AAD0(5, L"Checking fact %S which is %s",
                               g_fact_records[616].symbolic_name, text);
            }
            if (value == 0) {
                return 1;
            }
            value = EvaluateFact(0x323);
            if (g_log_fact_checks) {
                wcscpy(text, value ? L"TRUE" : L"FALSE");
                Function58AAD0(5, L"Checking fact %S which is %s",
                               g_fact_records[803].symbolic_name, text);
            }
            if (value == 0) {
                return 1;
            }
            return 0;
        }
    }
    else {
        if (fact_id == 0x295) {
            value = EvaluateFact(0x7d);
            if (g_log_fact_checks) {
                wcscpy(text, value ? L"TRUE" : L"FALSE");
                Function58AAD0(5, L"Checking fact %S which is %s",
                               g_fact_records[125].symbolic_name, text);
            }
            if (value != 0) {
                return 1;
            }
            value = EvaluateFact(0x3e);
            if (g_log_fact_checks) {
                wcscpy(text, value ? L"TRUE" : L"FALSE");
                Function58AAD0(5, L"Checking fact %S which is %s",
                               g_fact_records[62].symbolic_name, text);
            }
            if (value != 0) {
                return 1;
            }
            return 0;
        }
        if (fact_id == 0x314) {
            return GetFactionDisposition(16) == W8_FACTION_FRIENDLY;
        }
        if (fact_id == 0x31a) {
            value = EvaluateFact(0x156);
            if (g_log_fact_checks) {
                wcscpy(text, value ? L"TRUE" : L"FALSE");
                Function58AAD0(5, L"Checking fact %S which is %s",
                               g_fact_records[342].symbolic_name, text);
            }
            return value;
        }
    }
    return g_fact_values[fact_id];
}
