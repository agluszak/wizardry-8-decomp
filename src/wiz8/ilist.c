#include "gameplay_boundaries.h"
#include "sr_api.h"

#include <stdlib.h>

#define ILIST_CPP "C:\\Projects\\Wizardry 8\\3D Code\\IList.cpp"

/* 3D Code\IList.cpp, the integer sibling of 3D Code\PList.cpp. Same shape -
   elements at +0x00, count at +0x08, free functions - but the elements are
   ints, which is why the failed lookup returns -1 where PListGetAt returns
   null. Every assertion in the unit names the parameter pls. */

// FUNCTION: WIZ8 0x005E2A60
unsigned char IListFreeData(W8IList* pls)
{
    if (!pls) {
        srAssertFail("pls", ILIST_CPP, 0x9a, 0);
    }
    free(pls->data);
    pls->data = 0;
    return 1;
}

// FUNCTION: WIZ8 0x005E2B50
void IListClear(W8IList* pls)
{
    if (!pls) {
        srAssertFail("pls", ILIST_CPP, 0x147, 0);
    }
    pls->count = 0;
}

// FUNCTION: WIZ8 0x005E2C80
int IListGetAt(W8IList* pls, int index)
{
    if (!pls) {
        srAssertFail("pls", ILIST_CPP, 0x1aa, 0);
    }
    if (index < pls->count) {
        return pls->data[index];
    }
    return -1;
}

// FUNCTION: WIZ8 0x005E29A0
unsigned char IListInit(W8IList* pls)
{
    int* data;

    if (!pls) {
        srAssertFail("pls", ILIST_CPP, 0x64, 0);
    }
    if (pls->data) {
        free(pls->data);
    }
    data = (int*)malloc(10 * sizeof(int));
    pls->data = data;
    pls->capacity = 10;
    pls->count = 0;
    return data != 0;
}

// FUNCTION: WIZ8 0x005E2A00
unsigned char IListDestroy(W8IList* pls)
{
    /* The 0x9a assertion is IListFreeData's, inlined here; VC6 merges the two
       null tests into one. */
    if (!pls) {
        srAssertFail("pls", ILIST_CPP, 0x83, 0);
        srAssertFail("pls", ILIST_CPP, 0x9a, 0);
    }
    free(pls->data);
    pls->data = 0;
    free(pls);
    return 1;
}

// FUNCTION: WIZ8 0x005E2CC0
int IListIndexOf(W8IList* pls, int value)
{
    int count;
    int index;

    if (!pls) {
        srAssertFail("pls", ILIST_CPP, 0x21e, 0);
    }
    count = pls->count;
    for (index = 0; index < count; ++index) {
        if (pls->data[index] == value) {
            goto done;
        }
    }
    index = -1;

done:
    return index;
}
