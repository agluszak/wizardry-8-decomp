#include "wiz8/3d_code/PList.h"
#include "wiz8/sr_api.h"

#include <stdlib.h>

#define PLIST_CPP "C:\\Projects\\Wizardry 8\\3D Code\\PList.cpp"

/* 3D Code\PList.cpp. The parameter names ppl and pEntry come from the canonical
   assertions at lines 540 and 541. This is a different container from
   W8GrowableVector: the element array is at +0x00 and the count at +0x08, with
   no vptr, and the accessors are free functions rather than methods. */

// FUNCTION: WIZ8 0x005e22c0
W8PList* PLCreate(void)
{
    W8PList* ppl;
    void** data;

    ppl = (W8PList*)malloc(sizeof(W8PList));
    if (!ppl) {
        srAssertFail("ppl", PLIST_CPP, 0x37, 0);
    }
    ppl->count = 0;
    ppl->data = 0;

    /* PListInit is inlined here. */
    if (!ppl) {
        srAssertFail("ppl", PLIST_CPP, 0x56, 0);
    }
    if (ppl->count != 0) {
        srAssertFail("ppl->iNumUsed==0", PLIST_CPP, 0x58, 0);
    }
    if (ppl->data) {
        free(ppl->data);
    }
    data = (void**)malloc(10 * sizeof(void*));
    ppl->data = data;
    ppl->capacity = 10;
    ppl->count = 0;
    if (!data) {
        free(ppl);
        return 0;
    }
    return ppl;
}

// FUNCTION: WIZ8 0x005e2370
unsigned char PListInit(W8PList* ppl)
{
    unsigned char created;

    if (!ppl) {
        srAssertFail("ppl", PLIST_CPP, 0x56, 0);
    }
    if (ppl->count != 0) {
        srAssertFail("ppl->iNumUsed==0", PLIST_CPP, 0x58, 0);
    }
    if (ppl->data) {
        free(ppl->data);
    }
    ppl->data = (void**)malloc(10 * sizeof(void*));
    created = ppl->data != 0;
    ppl->capacity = 10;
    ppl->count = 0;
    return created;
}

// FUNCTION: WIZ8 0x005e23e0
unsigned char PLDestroy(W8PList* ppl)
{
    /* The second assertion is PListFreeData's, retained after inlining. */
    if (!ppl) {
        srAssertFail("ppl", PLIST_CPP, 0x77, 0);
        srAssertFail("ppl", PLIST_CPP, 0x8e, 0);
    }
    if (ppl->data) {
        free(ppl->data);
        ppl->data = 0;
    }
    free(ppl);
    return 1;
}

// FUNCTION: WIZ8 0x005e2440
unsigned char PListFreeData(W8PList* ppl)
{
    if (!ppl) {
        srAssertFail("ppl", PLIST_CPP, 0x8e, 0);
    }
    if (ppl->data) {
        free(ppl->data);
        ppl->data = 0;
    }
    return 1;
}

// FUNCTION: WIZ8 0x005e2480
int PLAdoptAppend(W8PList* ppl, void* pEntry)
{
    void** pTemp;
    int index;

    if (ppl->count >= ppl->capacity) {
        if (!ppl) {
            srAssertFail("ppl", PLIST_CPP, 0x1d6, 0);
        }
        pTemp = (void**)malloc((ppl->capacity + 5) * sizeof(void*));
        if (!pTemp) {
            srAssertFail("pTemp", PLIST_CPP, 0x1d9, 0);
        }
        for (index = 0; index < ppl->count; ++index) {
            pTemp[index] = ppl->data[index];
        }
        free(ppl->data);
        ppl->data = pTemp;
        ppl->capacity += 5;
    }
    ppl->data[ppl->count] = pEntry;
    ++ppl->count;
    return ppl->count - 1;
}

// FUNCTION: WIZ8 0x005e2530
int PListInsert(W8PList* ppl, int position, void* pEntry)
{
    void** pTemp;
    int index;

    if (!ppl) {
        srAssertFail("ppl", PLIST_CPP, 0xc1, 0);
    }
    if (position > ppl->count) {
        if (ppl->count >= ppl->capacity) {
            if (!ppl) {
                srAssertFail("ppl", PLIST_CPP, 0x1d6, 0);
            }
            pTemp = (void**)malloc((ppl->capacity + 5) * sizeof(void*));
            if (!pTemp) {
                srAssertFail("pTemp", PLIST_CPP, 0x1d9, 0);
            }
            for (index = 0; index < ppl->count; ++index) {
                pTemp[index] = ppl->data[index];
            }
            free(ppl->data);
            ppl->data = pTemp;
            ppl->capacity += 5;
        }
        ppl->data[ppl->count] = pEntry;
        ++ppl->count;
        return ppl->count - 1;
    }
    if (ppl->count >= ppl->capacity) {
        if (!ppl) {
            srAssertFail("ppl", PLIST_CPP, 0x1d6, 0);
        }
        pTemp = (void**)malloc((ppl->capacity + 5) * sizeof(void*));
        if (!pTemp) {
            srAssertFail("pTemp", PLIST_CPP, 0x1d9, 0);
        }
        for (index = 0; index < ppl->count; ++index) {
            pTemp[index] = ppl->data[index];
        }
        free(ppl->data);
        ppl->data = pTemp;
        ppl->capacity += 5;
    }
    for (index = ppl->count; index > position; --index) {
        ppl->data[index] = ppl->data[index - 1];
    }
    ppl->data[position] = pEntry;
    ++ppl->count;
    return position;
}

// FUNCTION: WIZ8 0x005e26b0
void PListClear(W8PList* ppl)
{
    if (!ppl) {
        srAssertFail("ppl", PLIST_CPP, 0x15a, 0);
    }
    ppl->count = 0;
}

// FUNCTION: WIZ8 0x005e26e0
void* PListRemove(W8PList* ppl, void* pEntry)
{
    int index;
    int shift_index;
    void* removed;

    if (!ppl) {
        srAssertFail("ppl", PLIST_CPP, 0x16f, 0);
    }
    for (index = 0; index < ppl->count; ++index) {
        if (ppl->data[index] == pEntry) {
            if (!ppl) {
                srAssertFail("ppl", PLIST_CPP, 0x18a, 0);
            }
            if (index >= ppl->count) {
                srAssertFail("iPosition < ppl->iNumUsed", PLIST_CPP, 0x18b, 0);
            }
            removed = ppl->data[index];
            for (shift_index = index; shift_index < ppl->count - 1; ++shift_index) {
                ppl->data[shift_index] = ppl->data[shift_index + 1];
            }
            --ppl->count;
            if ((double)ppl->count / (double)ppl->capacity < 0.25 && !ppl) {
                srAssertFail("ppl", PLIST_CPP, 0x1f8, 0);
            }
            return removed;
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x005e27c0
void* PLRemoveAt(W8PList* ppl, int position)
{
    void* entry;
    int index;

    if (!ppl) {
        srAssertFail("ppl", PLIST_CPP, 0x18a, 0);
    }
    if (position >= ppl->count) {
        srAssertFail("iPosition < ppl->iNumUsed", PLIST_CPP, 0x18b, 0);
    }
    entry = ppl->data[position];
    for (index = position; index < ppl->count - 1; ++index) {
        ppl->data[index] = ppl->data[index + 1];
    }
    --ppl->count;
    if ((double)ppl->count / (double)ppl->capacity < 0.25 && !ppl) {
        srAssertFail("ppl", PLIST_CPP, 0x1f8, 0);
    }
    return entry;
}

// FUNCTION: WIZ8 0x005e2870
void* PLGet(W8PList* ppl, int index)
{
    if (ppl && index < ppl->count) {
        return ppl->data[index];
    }
    return 0;
}

// FUNCTION: WIZ8 0x005e2890
int PListIndexOf(W8PList* ppl, void* pEntry)
{
    int count;
    int index;

    if (!ppl) {
        srAssertFail("ppl", PLIST_CPP, 0x21c, 0);
    }
    if (!pEntry) {
        srAssertFail("pEntry", PLIST_CPP, 0x21d, 0);
    }
    count = ppl->count;
    for (index = 0; index < count; ++index) {
        if (ppl->data[index] == pEntry) {
            goto done;
        }
    }
    index = -1;

done:
    return index;
}

/* The retail linker folds this ordinary PList.cpp function with ILLength. The
   retained body and address marker belong to the IList.cpp contribution. */
unsigned int PLLength(W8PList* ppl)
{
    if (!ppl) {
        return 0;
    }
    return ppl->count;
}
