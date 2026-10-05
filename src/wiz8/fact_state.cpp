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
    SetFact(W8_FACT_FACTION_HIGARDI_FELLOW_FRIENDLY, 1, 0);
    SetFact(W8_FACT_NARGISST_MONITOR_OVERFLOW_ON, 1, 0);
    SetFact(W8_FACT_RAPAX_QUEEN_LOCKED_UP, 1, 0);
    SetFact(W8_FACT_RODAN_LOCKED_IN_CAGE, 1, 0);
    SetFact(W8_FACT_DRAZIC_LOCKED_IN_CAGE, 1, 0);
    SetFact(W8_FACT_NARGISST_MONITOR_OVERFLOW_ON, 1, 0);
    SetFact(W8_FACT_FACTION_RATTKIN_COMMON_FRIENDLY, 1, 0);
    SetFactNotificationsSuppressed(0);
}

/* Reads the fact array back, then re-applies the consequences that do not
   survive a save. */
// FUNCTION: WIZ8 0x005064a0
void LoadFactState(int save_handle)
{
    W8NpcState* npc;
    unsigned int bytes_read;

    FileRead(save_handle, g_fact_values, 1000, &bytes_read);
    if (GetFact(W8_FACT_RFS81_HAS_BEEN_FIXED)) {
        npc = GetNpcStateByKind(0x20);
        if (npc && npc->has_monster) {
            ReleaseNpcScriptFile(npc->script_file);
            ReloadNpcScriptResources(npc);
        }
    }
    if (!GetFact(W8_FACT_IMPORT_TRANG)) {
        if (!GetFact(W8_FACT_IMPORT_UMPANI)) {
            if (!GetFact(W8_FACT_VIRGIN)) {
                SetFact(W8_FACT_IMPORT_NOALIGN, 1, 0);
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
unsigned char EvaluateFact(W8FactId fact_id)
{
    unsigned char value;
    unsigned char count;

    if (fact_id < W8_FACT_FACTION_HIGARDI_FELLOW_FRIENDLY) {
        if (fact_id == W8_FACT_FACTION_HIGARDI_BANK_FRIENDLY) {
            return GetFactionDisposition(W8_FACTION_HIGARDI_BANK) == W8_FACTION_FRIENDLY;
        }
        switch (fact_id) {
        case W8_FACT_ARNIKA_PARTY_HAS_MARTEN_BADGE:
            return FindItemOnParty(0x27b, 0, 0, 2, 0);
        case W8_FACT_FACTION_HIGARDI_COMMON_FRIENDLY:
            return GetFactionDisposition(W8_FACTION_HIGARDI_COMMON) == W8_FACTION_FRIENDLY;
        case W8_FACT_ALIGNMENT_NONE:
            value = GetFact(W8_FACT_ALIGNMENT_TRANG);
            if (value == 0) {
                value = GetFact(W8_FACT_ALIGNMENT_UMPANI);
                if (value == 0) {
                    return 1;
                }
            }
            return 0;
        case W8_FACT_FACTION_HIGARDI_HLL_FRIENDLY:
            return GetFactionDisposition(W8_FACTION_HIGARDI_HLL) == W8_FACTION_FRIENDLY;
        case W8_FACT_TWO_IN_PARTY:
            return CountLeadingPartySlots() == 2;
        case W8_FACT_MOOK_MOOK_IN_PARTY: {
            unsigned int slot = 0;
            while (g_status.buffers.XChar[slot].fOccupied == 0 ||
                   g_status.buffers.Char[slot].iRace != W8_RACE_MOOK ||
                   g_status.buffers.Char[slot].highest_condition > W8_CONDITION_WEBBED) {
                if (slot >= 7) {
                    return 0;
                }
                ++slot;
            }
            break;
        }
        case W8_FACT_PEACE_DRAZIC_IN_PARTY:
            return NpcLeadHasNameStyle(0x10) != 0;
        case W8_FACT_PEACE_RODAN_IN_PARTY:
            return NpcLeadHasNameStyle(0x11) != 0;
        case W8_FACT_PEACE_TWO_IN_PARTY:
            if (NpcLeadHasNameStyle(0x11) == 0 || NpcLeadHasNameStyle(0x10) == 0) {
                return 0;
            }
            break;
        case W8_FACT_VI_IN_PARTY:
            return NpcLeadHasNameStyle(0x18) != 0;
        case W8_FACT_GLUMPH_DEAD: {
            W8NpcState* npc = GetNpcStateByKind(0x2b);
            if (npc == 0) {
                return 0;
            }
            return static_cast<unsigned char>(npc->spawned);
        }
        case W8_FACT_UMISSION_TRAIN_FIRE_NOAMMO:
            value = GetFact(W8_FACT_UMISSION_TRAIN_COVERT_ASSIGN);
            if (value == 0) {
                value = GetFact(W8_FACT_UMISSION_TRAIN_FIRE_ASSIGN);
                if (value != 0 && FindItemOnParty(0x271, 0, 0, 2, 0) == 0 &&
                    FindItemOnParty(0x272, 0, 0, 2, 0) == 0) {
                    return 1;
                }
            }
            return 0;
        case W8_FACT_UMISSION_TRAIN_COVERT_DONE:
            if (g_status.fact_88_latch == 0) {
                if (CountItemOnParty(0x1c4, 0, 0, 2) < 5) {
                    return 0;
                }
                g_status.fact_88_latch = true;
                return 1;
            }
            break;
        case W8_FACT_UMISSION_TRAIN_WETSUIT:
            return EveryCharacterHasItem(0x1e5, 0);
        case W8_FACT_UMISSION_IUFPASS_LEVEL2:
            return FindItemOnParty(0x268, 0, 0, 2, 0);
        case W8_FACT_RAPAX_AWAY_CAMP_EXISTS:
            count = FindItemOnParty(0x242, 0, 0, 2, 0) != 0;
            if (FindItemOnParty(0x243, 0, 0, 2, 0) != 0) {
                ++count;
            }
            if (FindItemOnParty(0x244, 0, 0, 2, 0) != 0) {
                ++count;
            }
            return count >= 2;
        case W8_FACT_DEVICE_THREE:
        case W8_FACT_DEVICE_TWO:
        triple:
            count = FindItemOnParty(0x243, 0, 0, 2, 0) != 0;
            if (FindItemOnParty(0x242, 0, 0, 2, 0) != 0) {
                ++count;
            }
            if (FindItemOnParty(0x244, 0, 0, 2, 0) != 0) {
                ++count;
            }
            if (fact_id == W8_FACT_DEVICE_ONE) {
                return count == 1;
            }
            if (fact_id == W8_FACT_DEVICE_TWO) {
                return count == 2;
            }
            if (fact_id == W8_FACT_DEVICE_THREE) {
                return count == 3;
            }
            return 0;
        default:
            break;
        }
    } else if (fact_id < W8_FACT_TRYNNIE_FOUNTAIN_ANSWERED_CORRECT) {
        if (fact_id == W8_FACT_FACTION_TRYNNIE_COMMON_FRIENDLY) {
            return GetFactionDisposition(W8_FACTION_TRYNNIE) == W8_FACTION_FRIENDLY;
        }
        switch (fact_id) {
        case W8_FACT_TRYNNIE_PC_HAS_HELM:
            return FindItemOnParty(0x239, 0, 0, 2, 0);
        case W8_FACT_FACTION_UMPANI_COMMON_FRIENDLY:
            return GetFactionDisposition(W8_FACTION_UMPANI) == W8_FACTION_FRIENDLY;
        case W8_FACT_FACTION_TRANG_COMMON_FRIENDLY:
            return GetFactionDisposition(W8_FACTION_TRANG) == W8_FACTION_FRIENDLY;
        case W8_FACT_FACTION_HIGARDI_FELLOW_FRIENDLY:
            return GetFactionDisposition(W8_FACTION_BROTHERHOOD) == W8_FACTION_FRIENDLY;
        case W8_FACT_MYLES_IN_PARTY:
            return NpcLeadHasNameStyle(7) != 0;
        case W8_FACT_FATHER_IN_PARTY:
            if (g_status.rpc_active != 0) {
                unsigned int slot = 0;
                do {
                    if (g_status.buffers.XChar[slot].fOccupied != 0 &&
                        slot == static_cast<unsigned int>(g_status.sedexus_party_slot)) {
                        return 1;
                    }
                    ++slot;
                } while (slot < 8);
            }
            return 0;
        case W8_FACT_PARTY_HAS_BLOODLUST_SWORD:
            return FindItemOnParty(0x294, 0, 0, 2, 0);
        case W8_FACT_RAPAX_SAVANT_ALLIANCE:
            value = GetFact(W8_FACT_ASTRAL_POSSESS);
            if (value != 0) {
                return 1;
            }
            value = GetFact(W8_FACT_DESTINAE_POSSESS);
            if (value != 0) {
                return 1;
            }
            value = GetFact(W8_FACT_CHAOS_POSSESS);
            if (value != 0) {
                return 1;
            }
            return 0;
        case W8_FACT_DEVICE_ONE:
            goto triple;
        default:
            break;
        }
    } else if (fact_id < W8_FACT_QUEST_SHAMAN_BRIDGE_EASY) {
        if (fact_id == W8_FACT_QUEST_SHAMAN_GET_HELM) {
            return FindItemOnParty(0x239, 0, 0, 2, 0) == 0;
        }
        switch (fact_id) {
        case W8_FACT_FACTION_RAPAX_COMMON_FRIENDLY:
            return GetFactionDisposition(W8_FACTION_RAPAX_COMMON) == W8_FACTION_FRIENDLY;
        case W8_FACT_TRYNNIE_SPARKLE_IN_PARTY:
            return NpcLeadHasNameStyle(0x38) != 0;
        case W8_FACT_VI_IS_DEAD: {
            if (NpcLeadHasNameStyle(0x18) == 0) {
                return g_fact_values[fact_id];
            }
            W8NpcState* npc = GetNpcStateByKind(0x18);
            if (npc != 0 &&
                g_status.buffers.Char[npc->group_index].highest_condition >= W8_CONDITION_ASLEEP) {
                return 1;
            }
            return 0;
        }
        case W8_FACT_QUEST_SHAMAN_DESTINY_TWO:
            value = GetFact(W8_FACT_QUEST_MARTEN_DIARY);
            if (value == 0) {
                return 1;
            }
            value = GetFact(W8_FACT_QUEST_MARTEN_IDOL);
            if (value == 0) {
                return 1;
            }
            return 0;
        default:
            break;
        }
    } else {
        if (fact_id == W8_FACT_QUEST_PEACE_UNLOCK_CAGES) {
            value = GetFact(W8_FACT_RODAN_LOCKED_IN_CAGE);
            if (value != 0) {
                return 1;
            }
            value = GetFact(W8_FACT_DRAZIC_LOCKED_IN_CAGE);
            if (value != 0) {
                return 1;
            }
            return 0;
        }
        if (fact_id == W8_FACT_FACTION_RAPAX_TEMPLAR_FRIENDLY) {
            return GetFactionDisposition(W8_FACTION_RAPAX_TEMPLAR) == W8_FACTION_FRIENDLY;
        }
        if (fact_id == W8_FACT_PARTY_DEACTIVATED_BOMB) {
            value = GetFact(W8_FACT_DS_BOMB_DEACTIVATED);
            return value;
        }
    }
    return g_fact_values[fact_id];
}
