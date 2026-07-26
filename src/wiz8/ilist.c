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
