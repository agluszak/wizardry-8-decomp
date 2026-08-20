#include "wiz8/fact_state.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/utility.h"
#include "wiz8/npc_item_lists.h"
#include "wiz8/virtual_file.h"
#include "wiz8/xstatus.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

/* 0x005080F0, reviewed in evidence/reviewed/wiz8/claims.csv. */
extern unsigned char EvaluateFact(int fact_id);
/* Not yet identified; named by address as elsewhere in src/wiz8. 0x0058AAD0 is a
   wide-string logger taking a channel, 0x0055A0A0 and 0x00524CA0 tear down an
   NPC item list. */
extern void Function58AAD0(int channel, const wchar_t* format, const char* name,
                           const wchar_t* text);
extern void Function55A0A0(int handle);
extern void Function524CA0(W8NPCItemList* list);
/* Provisional semantic name for the journal/notification path at 0x005588f0. */
extern void RecordFactChangeForJournal(int fact_id);
extern void HandleFactChange(int fact_id, unsigned char value);

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
    WriteVirtualFile(save_handle, g_fact_values, 1000, (unsigned int*)&save_handle);
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
        Function58AAD0(5, L"Checking fact %S which is %s",
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
    W8NPCItemList* list;

    ReadVirtualFile(save_handle, g_fact_values, 1000, (unsigned int*)&save_handle);
    if (CheckFactLogged(0x44)) {
        list = GetNPCItemListByID(0x20);
        if (list && list->flag_1a) {
            Function55A0A0(list->unknown_00);
            Function524CA0(list);
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
