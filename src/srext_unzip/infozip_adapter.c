/* Sir-Tech's narrow adaptation of the Info-ZIP 5.4 memory API. */

#include <windows.h>

#define UNZIP_INTERNAL
#include "unzip.h"
#include "unzpriv.h"
#include "windll/decs.h"
#include "windll/structs.h"

/* FUNCTION: SREXT_UNZIP 0x1000DA10 */
int WINAPI srWizUnzipToMemory(
    char* archive,
    char* member,
    LPUSERFUNCTIONS callbacks,
    UzpBuffer* result,
    int case_insensitive)
{
    int extracted;

    CONSTRUCTGLOBALS();
    if (!Wiz_Init((zvoid*)&G, callbacks)) {
        DESTROYGLOBALS();
        return PK_BADERR;
    }

    G.redirect_data = 1;
    uO.C_flag = case_insensitive;
    extracted = (unzipToMemory(__G__ archive, member, result) == PK_COOL);

    DESTROYGLOBALS();
    if (!extracted && result->strlength != 0) {
        free(result->strptr);
        result->strptr = NULL;
    }
    return extracted;
}
