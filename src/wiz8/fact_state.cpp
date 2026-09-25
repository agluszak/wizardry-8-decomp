#include "wiz8/fact_state.h"
#include "wiz8/layouts/character.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/PartyImport.h"
#include "wiz8/local_code/UtilityFunctions.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/local_code/Factions.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/local_code/NPCManager.h"
#include "wiz8/local_code/NPCScripting.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_screens/JournalScreen.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/NPCInteractionSubscreen.h"
#include "wiz8/utility.h"
#include "wiz8/layouts/npc_state.h"
#include "wiz8/npc_script_file.h"
#include "wiz8/virtual_file.h"
#include "wiz8/xstatus.h"

#include "soundman.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "wiz8/layouts/game_status.h"
#include "wiz8/local_screens/PartySelectionScreen.h"
#include "wiz8/local_code/PartyImport.h"

/* Runs once the new-game level has finished loading: records the two starting
   transcript keywords from string-table entries 0x7e7/0x7e8 and
   seeds the starting fact set, all with notifications suppressed. */
// FUNCTION: WIZ8 0x005063e0
void PostNewGameLoad(void)
{
    SetFactNotificationsSuppressed(1);
    AddDialogueTranscriptKeyword(gppStringList[0x7e7], 3);
    AddDialogueTranscriptKeyword(gppStringList[0x7e8], 3);
    SetFact(0xcc, 1, 0);
    SetFact(0x42, 1, 0);
    SetFact(0x1e9, 1, 0);
    SetFact(0x7d, 1, 0);
    SetFact(0x3e, 1, 0);
    SetFact(0x42, 1, 0);
    SetFact(0x18b, 1, 0);
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
    if (g_status.log_fact_checks_3120) {
        if (value) {
            wcscpy(text, L"TRUE");
        } else {
            wcscpy(text, L"FALSE");
        }
        ShowNoticef(5, L"Checking fact %S which is %s", g_fact_records[fact_id].symbolic_name,
                    text);
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
            ReleaseNpcScriptFile(npc->script_file);
            ReloadNpcScriptResources(npc);
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
            return GetFactionDisposition(W8_FACTION_HIGARDI_BANK) == W8_FACTION_FRIENDLY;
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
            if (g_status.log_fact_checks_3120) {
                wcscpy(text, value ? L"TRUE" : L"FALSE");
                ShowNoticef(5, L"Checking fact %S which is %s", g_fact_records[58].symbolic_name,
                            text);
            }
            if (value == 0) {
                value = EvaluateFact(0x3c);
                if (g_status.log_fact_checks_3120) {
                    wcscpy(text, value ? L"TRUE" : L"FALSE");
                    ShowNoticef(5, L"Checking fact %S which is %s",
                                g_fact_records[60].symbolic_name, text);
                }
                if (value == 0) {
                    return 1;
                }
            }
            return 0;
        case 0x4f:
            if (NpcLeadHasNameStyle(0x11) == 0 || NpcLeadHasNameStyle(0x10) == 0) {
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
            return static_cast<unsigned char>(npc->spawned_04);
        }
        case 0x81:
            value = EvaluateFact(0x86);
            if (g_status.log_fact_checks_3120) {
                wcscpy(text, value ? L"TRUE" : L"FALSE");
                ShowNoticef(5, L"Checking fact %S which is %s", g_fact_records[134].symbolic_name,
                            text);
            }
            if (value == 0) {
                value = EvaluateFact(0x83);
                if (g_status.log_fact_checks_3120) {
                    wcscpy(text, value ? L"TRUE" : L"FALSE");
                    ShowNoticef(5, L"Checking fact %S which is %s",
                                g_fact_records[131].symbolic_name, text);
                }
                if (value != 0 && FindItemOnParty(0x271, 0, 0, 2, 0) == 0 &&
                    FindItemOnParty(0x272, 0, 0, 2, 0) == 0) {
                    return 1;
                }
            }
            return 0;
        case 0x88:
            if (g_status.fact_88_latch_40c1 == 0) {
                if (CountItemOnParty(0x1c4, 0, 0, 2) < 5) {
                    return 0;
                }
                g_status.fact_88_latch_40c1 = true;
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
            while (g_status.buffers.XChar[slot].fOccupied == 0 ||
                   g_status.buffers.Char[slot].iRace != 10 ||
                   g_status.buffers.Char[slot].highest_condition > 0xe) {
                if (slot >= 7) {
                    return 0;
                }
                ++slot;
            }
            break;
        }
        case 0xbd:
            return NpcLeadHasNameStyle(0x10) != 0;
        case 0xbe:
            return NpcLeadHasNameStyle(0x11) != 0;
        case 0xc3:
            return NpcLeadHasNameStyle(0x18) != 0;
        case 0xc9:
            return GetFactionDisposition(W8_FACTION_HIGARDI_HLL) == W8_FACTION_FRIENDLY;
        case 0xca:
            return GetFactionDisposition(W8_FACTION_HIGARDI_COMMON) == W8_FACTION_FRIENDLY;
        }
    } else if (fact_id < 0x195) {
        if (fact_id == 0x194) {
            return GetFactionDisposition(W8_FACTION_TRYNNIE) == W8_FACTION_FRIENDLY;
        }
        switch (fact_id) {
        case 0xcc:
            return GetFactionDisposition(W8_FACTION_BROTHERHOOD) == W8_FACTION_FRIENDLY;
        case 0xce:
            return FindItemOnParty(0x294, 0, 0, 2, 0);
        case 0xd1:
            return NpcLeadHasNameStyle(7) != 0;
        case 0x103:
            goto triple;
        case 0x10c:
            value = EvaluateFact(0x22);
            if (g_status.log_fact_checks_3120) {
                wcscpy(text, value ? L"TRUE" : L"FALSE");
                ShowNoticef(5, L"Checking fact %S which is %s", g_fact_records[34].symbolic_name,
                            text);
            }
            if (value != 0) {
                return 1;
            }
            value = EvaluateFact(0x30);
            if (g_status.log_fact_checks_3120) {
                wcscpy(text, value ? L"TRUE" : L"FALSE");
                ShowNoticef(5, L"Checking fact %S which is %s", g_fact_records[48].symbolic_name,
                            text);
            }
            if (value != 0) {
                return 1;
            }
            value = EvaluateFact(0x31);
            if (g_status.log_fact_checks_3120) {
                wcscpy(text, value ? L"TRUE" : L"FALSE");
                ShowNoticef(5, L"Checking fact %S which is %s", g_fact_records[49].symbolic_name,
                            text);
            }
            if (value != 0) {
                return 1;
            }
            return 0;
        case 0x11e:
            return GetFactionDisposition(W8_FACTION_TRANG) == W8_FACTION_FRIENDLY;
        case 0x14c:
            if (g_status.rpc_active_2489 != 0) {
                unsigned int slot = 0;
                do {
                    if (g_status.buffers.XChar[slot].fOccupied != 0 &&
                        slot == static_cast<unsigned int>(g_status.sedexus_party_slot_247f)) {
                        return 1;
                    }
                    ++slot;
                } while (slot < 8);
            }
            return 0;
        case 0x172:
            return FindItemOnParty(0x239, 0, 0, 2, 0);
        case 0x183:
            return GetFactionDisposition(W8_FACTION_UMPANI) == W8_FACTION_FRIENDLY;
        }
    } else if (fact_id < 0x26a) {
        if (fact_id == 0x269) {
            return FindItemOnParty(0x239, 0, 0, 2, 0) == 0;
        }
        switch (fact_id) {
        case 0x19a:
            return NpcLeadHasNameStyle(0x38) != 0;
        case 0x1a8:
            return GetFactionDisposition(W8_FACTION_RAPAX_COMMON) == W8_FACTION_FRIENDLY;
        case 0x216: {
            if (NpcLeadHasNameStyle(0x18) == 0) {
                return g_fact_values[fact_id];
            }
            W8NpcState* npc = GetNpcStateByKind(0x18);
            if (npc != 0 && g_status.buffers.Char[npc->group_index].highest_condition >= 0xf) {
                return 1;
            }
            return 0;
        }
        case 0x265:
            value = EvaluateFact(0x268);
            if (g_status.log_fact_checks_3120) {
                wcscpy(text, value ? L"TRUE" : L"FALSE");
                ShowNoticef(5, L"Checking fact %S which is %s", g_fact_records[616].symbolic_name,
                            text);
            }
            if (value == 0) {
                return 1;
            }
            value = EvaluateFact(0x323);
            if (g_status.log_fact_checks_3120) {
                wcscpy(text, value ? L"TRUE" : L"FALSE");
                ShowNoticef(5, L"Checking fact %S which is %s", g_fact_records[803].symbolic_name,
                            text);
            }
            if (value == 0) {
                return 1;
            }
            return 0;
        }
    } else {
        if (fact_id == 0x295) {
            value = EvaluateFact(0x7d);
            if (g_status.log_fact_checks_3120) {
                wcscpy(text, value ? L"TRUE" : L"FALSE");
                ShowNoticef(5, L"Checking fact %S which is %s", g_fact_records[125].symbolic_name,
                            text);
            }
            if (value != 0) {
                return 1;
            }
            value = EvaluateFact(0x3e);
            if (g_status.log_fact_checks_3120) {
                wcscpy(text, value ? L"TRUE" : L"FALSE");
                ShowNoticef(5, L"Checking fact %S which is %s", g_fact_records[62].symbolic_name,
                            text);
            }
            if (value != 0) {
                return 1;
            }
            return 0;
        }
        if (fact_id == 0x314) {
            return GetFactionDisposition(W8_FACTION_RAPAX_TEMPLAR) == W8_FACTION_FRIENDLY;
        }
        if (fact_id == 0x31a) {
            value = EvaluateFact(0x156);
            if (g_status.log_fact_checks_3120) {
                wcscpy(text, value ? L"TRUE" : L"FALSE");
                ShowNoticef(5, L"Checking fact %S which is %s", g_fact_records[342].symbolic_name,
                            text);
            }
            return value;
        }
    }
    return g_fact_values[fact_id];
}
