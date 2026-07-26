#include "gameplay_boundaries.h"
#include "sr_api.h"

#define PLIST_CPP "C:\\Projects\\Wizardry 8\\3D Code\\PList.cpp"

/* 3D Code\PList.cpp. The parameter names ppl and pEntry come from the canonical
   assertions at lines 540 and 541. This is a different container from
   W8PtrVector: the element array is at +0x00 and the count at +0x08, with no
   vptr, and the accessors are free functions rather than methods. */

// FUNCTION: WIZ8 0x005E2C70
unsigned int PListGetCount(W8PList* ppl)
{
    if (!ppl) {
        return 0;
    }
    return ppl->count;
}

// FUNCTION: WIZ8 0x005E2870
void* PListGetAt(W8PList* ppl, int index)
{
    if (ppl && index < ppl->count) {
        return ppl->data[index];
    }
    return 0;
}

// FUNCTION: WIZ8 0x005E2890
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
