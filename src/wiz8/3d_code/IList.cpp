#include "wiz8/3d_code/IList.h"
#include "wiz8/sr_api.h"

#include <stdlib.h>

#define ILIST_CPP "C:\\Projects\\Wizardry 8\\3D Code\\IList.cpp"

// FUNCTION: WIZ8 0x005e2900
W8IList* ILCreate(void)
{
    W8IList* pls;

    pls = (W8IList*)malloc(sizeof(W8IList));
    if (!pls) {
        srAssertFail("pls", ILIST_CPP, 0x45, 0);
    }
    pls->iNumUsed = 0;
    pls->data = 0;

    if (!IListInit(pls)) {
        free(pls);
        return 0;
    }
    return pls;
}

// FUNCTION: WIZ8 0x005e29a0
unsigned char IListInit(W8IList* pls)
{
    if (!pls) {
        srAssertFail("pls", ILIST_CPP, 0x64, 0);
    }
    if (pls->data) {
        free(pls->data);
    }
    pls->data = (int*)malloc(10 * sizeof(int));
    pls->capacity = 10;
    pls->iNumUsed = 0;
    return pls->data != 0;
}

// FUNCTION: WIZ8 0x005e2a00
unsigned char ILDestroy(W8IList* pls)
{
    if (!pls) {
        srAssertFail("pls", ILIST_CPP, 0x83, 0);
    }
    IListFreeData(pls);
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

// FUNCTION: WIZ8 0x005e2aa0
int IListAdd(W8IList* pls, int value)
{
    int* pTemp;
    int index;

    if (pls->iNumUsed >= pls->capacity) {
        if (!pls) {
            srAssertFail("pls", ILIST_CPP, 0x1db, 0);
        }
        pTemp = (int*)malloc((pls->capacity + 5) * sizeof(int));
        if (!pTemp) {
            srAssertFail("pTemp", ILIST_CPP, 0x1de, 0);
        }
        for (index = 0; index < pls->iNumUsed; ++index) {
            pTemp[index] = pls->data[index];
        }
        free(pls->data);
        pls->data = pTemp;
        pls->capacity += 5;
    }
    pls->data[pls->iNumUsed] = value;
    ++pls->iNumUsed;
    return pls->iNumUsed - 1;
}

// FUNCTION: WIZ8 0x005e2b50
void IListClear(W8IList* pls)
{
    if (!pls) {
        srAssertFail("pls", ILIST_CPP, 0x147, 0);
    }
    pls->iNumUsed = 0;
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
    for (index = 0; index < pls->iNumUsed; ++index) {
        if (pls->data[index] == value) {
            if (!pls) {
                srAssertFail("pls", ILIST_CPP, 0x177, 0);
            }
            if (index >= pls->iNumUsed) {
                srAssertFail("lPosition < pls->iNumUsed", ILIST_CPP, 0x178, 0);
            }
            removed = pls->data[index];
            for (shift_index = index; shift_index < pls->iNumUsed - 1; ++shift_index) {
                pls->data[shift_index] = pls->data[shift_index + 1];
            }
            --pls->iNumUsed;
            if (static_cast<double>(pls->iNumUsed) / pls->capacity < 0.25 && !pls) {
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
    return pls->iNumUsed;
}

// FUNCTION: WIZ8 0x005e2c80
int IListGetAt(W8IList* pls, int index)
{
    if (!pls) {
        srAssertFail("pls", ILIST_CPP, 0x1aa, 0);
    }
    if (index < pls->iNumUsed) {
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
    count = pls->iNumUsed;
    for (index = 0; index < count; ++index) {
        if (pls->data[index] == value) {
            goto done;
        }
    }
    index = -1;

done:
    return index;
}
