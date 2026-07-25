/* Sir-Tech's narrowed Info-ZIP 5.4 WinDLL front end.
 *
 * The stock windll.c object also contains the general extraction, grep, and
 * validation entry points.  They are absent from srEXT_Unzip.dll even though
 * the linker retains unreferenced functions.  The target keeps only this
 * callback/setup subset plus the separate memory wrapper.
 */

#include <windows.h>

#define UNZIP_INTERNAL
#include "unzip.h"
#include "crypt.h"
#include "version.h"
#include "windll.h"
#include "structs.h"
#include "consts.h"

HANDLE hInst;
int fNoPrinting = 0;

static int UZ_EXP DllMessagePrint(
    zvoid* pG, uch* buffer, ulg size, int flag);
static void WINAPI DummySound(void);
static int UZ_EXP Wiz_StatReportCB(
    zvoid* pG,
    int function_flag,
    ZCONST char* archive_name,
    ZCONST char* entry_name,
    ZCONST zvoid* details);

// FUNCTION: SREXT_UNZIP 0x1000D830
BOOL WINAPI DllMain(
    HINSTANCE instance,
    DWORD reason,
    LPVOID reserved)
{
    BOOL result = TRUE;
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        hInst = instance;
        break;
    case DLL_PROCESS_DETACH:
        break;
    default:
        break;
    }
    return result;
}

// FUNCTION: SREXT_UNZIP 0x1000D850
BOOL WINAPI Wiz_Init(zvoid* pG, LPUSERFUNCTIONS callbacks)
{
    G.message = DllMessagePrint;
    G.statreportcb = Wiz_StatReportCB;
    if (callbacks->sound == NULL) {
        callbacks->sound = DummySound;
    }
    G.lpUserFunctions = callbacks;
    return callbacks->print != NULL &&
           callbacks->sound != NULL &&
           callbacks->replace != NULL;
}

// FUNCTION: SREXT_UNZIP 0x1000D8B0
int win_fprintf(zvoid* pG, FILE* file, unsigned int size, char far* buffer)
{
    if (file != stderr && file != stdout) {
        return write(fileno(file), (char far*)buffer, size);
    }
    if (!fNoPrinting) {
        return G.lpUserFunctions->print((LPSTR)buffer, size);
    }
    return (int)size;
}

// FUNCTION: SREXT_UNZIP 0x1000D910
static int UZ_EXP DllMessagePrint(
    zvoid* pG, uch* buffer, ulg size, int flag)
{
    if (!fNoPrinting) {
        return G.lpUserFunctions->print((LPSTR)buffer, size);
    }
    return (int)size;
}

// FUNCTION: SREXT_UNZIP 0x1000D940
int UZ_EXP UzpPassword(
    zvoid* pG,
    int* retry_count,
    char* password,
    int size,
    ZCONST char* archive_name,
    ZCONST char* entry_name)
{
#if CRYPT
    LPSTR message;

    if (*retry_count == 0) {
        *retry_count = 2;
        message = "Enter password for: ";
    }
    else {
        --*retry_count;
        message = "Password incorrect--reenter: ";
    }
    return G.lpUserFunctions->password(password, size, message, entry_name);
#else
    return IZ_PW_ERROR;
#endif
}

// FUNCTION: SREXT_UNZIP 0x1000D990
void WINAPI Wiz_NoPrinting(int disabled)
{
    fNoPrinting = disabled;
}

// FUNCTION: SREXT_UNZIP 0x1000D9A0
static void WINAPI DummySound(void)
{
}

// FUNCTION: SREXT_UNZIP 0x1000D9B0
static int UZ_EXP Wiz_StatReportCB(
    zvoid* pG,
    int function_flag,
    ZCONST char* archive_name,
    ZCONST char* entry_name,
    ZCONST zvoid* details)
{
    int result = UZ_ST_CONTINUE;

    switch (function_flag) {
    case UZ_ST_START_EXTRACT:
        if (G.lpUserFunctions->sound != NULL) {
            G.lpUserFunctions->sound();
        }
        break;
    case UZ_ST_FINISH_MEMBER:
        if (G.lpUserFunctions->ServCallBk != NULL &&
            G.lpUserFunctions->ServCallBk(
                entry_name, *((unsigned long*)details))) {
            result = UZ_ST_BREAK;
        }
        break;
    case UZ_ST_IN_PROGRESS:
    default:
        break;
    }
    return result;
}
