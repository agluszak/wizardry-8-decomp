#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"

#include <stdlib.h>

#define ILIST_CPP "C:\\Projects\\Wizardry 8\\3D Code\\IList.cpp"

/* 3D Code\IList.cpp, the integer sibling of 3D Code\PList.cpp. Same shape -
   elements at +0x00, capacity at +0x04, count at +0x08, and free functions -
   but the elements are ints, which is why a failed lookup returns -1 where
   PLGet returns null. Every assertion in the unit names the parameter
   pls. */

// FUNCTION: WIZ8 0x005e2900
W8IList* ILCreate(void)
{
    W8IList* pls;
    int* data;

    pls = (W8IList*)malloc(sizeof(W8IList));
    if (!pls) {
        srAssertFail("pls", ILIST_CPP, 0x45, 0);
    }
    pls->count = 0;
    pls->data = 0;

    /* IListInit inlined; its own null assertion at line 100 survives as a
       second test because the two stores above separate the two checks. */
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
    if (!data) {
        free(pls);
        return 0;
    }
    return pls;
}

// FUNCTION: WIZ8 0x005e29a0
unsigned char IListInit(W8IList* pls)
{
    unsigned char created;

    if (!pls) {
        srAssertFail("pls", ILIST_CPP, 0x64, 0);
    }
    if (pls->data) {
        free(pls->data);
    }
    /* The original sets the flags immediately after the store and materialises
       the result only at the return, as a bare `setne al` with no zero-extend.
       Comparing in the return statement instead makes VC6 compute the value
       early into CL and widen it; a byte local assigned here reproduces the
       original's split between testing and materialising. */
    pls->data = (int*)malloc(10 * sizeof(int));
    created = pls->data != 0;
    pls->capacity = 10;
    pls->count = 0;
    return created;
}

// FUNCTION: WIZ8 0x005e2a00
unsigned char ILDestroy(W8IList* pls)
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

// FUNCTION: WIZ8 0x005e2a60
unsigned char IListFreeData(W8IList* pls)
{
    if (!pls) {
        srAssertFail("pls", ILIST_CPP, 0x9a, 0);
    }
    free(pls->data);
    pls->data = 0;
    return 1;
}

// Grows by five, and the growth assertion names its temporary pTemp.
// FUNCTION: WIZ8 0x005e2aa0
int IListAdd(W8IList* pls, int value)
{
    int* pTemp;
    int index;

    if (pls->count >= pls->capacity) {
        if (!pls) {
            srAssertFail("pls", ILIST_CPP, 0x1db, 0);
        }
        pTemp = (int*)malloc((pls->capacity + 5) * sizeof(int));
        if (!pTemp) {
            srAssertFail("pTemp", ILIST_CPP, 0x1de, 0);
        }
        for (index = 0; index < pls->count; ++index) {
            pTemp[index] = pls->data[index];
        }
        free(pls->data);
        pls->data = pTemp;
        pls->capacity += 5;
    }
    pls->data[pls->count] = value;
    ++pls->count;
    return pls->count - 1;
}

// FUNCTION: WIZ8 0x005e2b50
void IListClear(W8IList* pls)
{
    if (!pls) {
        srAssertFail("pls", ILIST_CPP, 0x147, 0);
    }
    pls->count = 0;
}

// FUNCTION: WIZ8 0x005e2b80
int IListRemove(W8IList* pls, int value)
{
    int index;
    int shift_index;
    int removed;

    if (!pls) {
        srAssertFail("pls", ILIST_CPP, 0x15c, 0);
    }
    for (index = 0; index < pls->count; ++index) {
        if (pls->data[index] == value) {
            if (!pls) {
                srAssertFail("pls", ILIST_CPP, 0x177, 0);
            }
            if (index >= pls->count) {
                srAssertFail("lPosition < pls->iNumUsed", ILIST_CPP, 0x178, 0);
            }
            removed = pls->data[index];
            for (shift_index = index; shift_index < pls->count - 1; ++shift_index) {
                pls->data[shift_index] = pls->data[shift_index + 1];
            }
            --pls->count;
            if ((double)pls->count / (double)pls->capacity < 0.25 && !pls) {
                srAssertFail("pls", ILIST_CPP, 0x1fd, 0);
            }
            return removed;
        }
    }
    return -1;
}

// FUNCTION: WIZ8 0x005e2c70
unsigned int ILLength(W8IList* pls)
{
    if (!pls) {
        return 0;
    }
    return pls->count;
}

// FUNCTION: WIZ8 0x005e2c80
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

// FUNCTION: WIZ8 0x005e2cc0
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
