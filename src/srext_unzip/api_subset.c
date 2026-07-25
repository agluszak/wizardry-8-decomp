/* The five Info-ZIP 5.4 memory-extraction helpers retained by Sir-Tech. */

#include <setjmp.h>

#define UNZIP_INTERNAL
#include "unzip.h"

jmp_buf dll_error_return;

// FUNCTION: SREXT_UNZIP 0x10001000
void setFileNotFound(__G)
    __GDEF
{
    G.filenotfound++;
}

// FUNCTION: SREXT_UNZIP 0x10001010
int unzipToMemory(
    __GPRO__ char* archive,
    char* member,
    UzpBuffer* result)
{
    int status;
    char* included_names[2];

    G.process_all_files = FALSE;
    G.extract_flag = TRUE;
    uO.qflag = 2;
    G.wildzipfn = archive;

    G.pfnames = included_names;
    included_names[0] = member;
    included_names[1] = NULL;
    G.filespecs = 1;

    status = process_zipfiles(__G);
    if (result != NULL) {
        result->strptr = (char*)G.redirect_buffer;
        result->strlength = G.redirect_size;
    }
    return status;
}

// FUNCTION: SREXT_UNZIP 0x10001080
int redirect_outfile(__G)
    __GDEF
{
    if (G.redirect_size != 0 || G.redirect_buffer != NULL) {
        return FALSE;
    }

#ifndef NO_SLIDE_REDIR
    G.redirect_slide = !G.pInfo->textmode;
#endif
    G.redirect_size = G.pInfo->textmode
        ? G.lrec.ucsize * lenEOL
        : G.lrec.ucsize;
    G.redirect_pointer = G.redirect_buffer = malloc(G.redirect_size + 1);
    if (G.redirect_buffer == NULL) {
        return FALSE;
    }
    G.redirect_pointer[G.redirect_size] = '\0';
    return TRUE;
}

// FUNCTION: SREXT_UNZIP 0x10001100
int writeToMemory(__GPRO__ uch* raw_buffer, ulg size)
{
    if (raw_buffer != G.redirect_pointer) {
        memcpy(G.redirect_pointer, raw_buffer, size);
    }
    G.redirect_pointer += size;
    return 0;
}

// FUNCTION: SREXT_UNZIP 0x10001160
int close_redirect(__G)
    __GDEF
{
    if (G.pInfo->textmode) {
        *G.redirect_pointer = '\0';
        G.redirect_size = G.redirect_pointer - G.redirect_buffer;
        G.redirect_buffer =
            realloc(G.redirect_buffer, G.redirect_size + 1);
        if (G.redirect_buffer == NULL) {
            G.redirect_size = 0;
            return EOF;
        }
    }
    return 0;
}
