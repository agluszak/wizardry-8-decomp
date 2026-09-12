#ifndef WIZ8_3D_CODE_PLIST_H
#define WIZ8_3D_CODE_PLIST_H

/*
 * 3D Code\\PList.cpp. A growable array of void pointers - the engine's
 * general-purpose list, distinct from the typed growable-vector template.
 * All twelve source-owned functions agree on this layout.
 */

struct W8PList {
    void** data;  /* 0x00 */
    int capacity; /* 0x04: PListInit allocates 10 */
    int count;    /* 0x08 */
};

static_assert(sizeof(W8PList) == 0x0c, "W8PList_must_be_0x0c");

W8PList* PLCreate(void);
unsigned char PListInit(W8PList* ppl);
unsigned char PLDestroy(W8PList* ppl);
unsigned char PListFreeData(W8PList* ppl);
int PLAdoptAppend(W8PList* ppl, void* pEntry);
int PListInsert(W8PList* ppl, int position, void* pEntry);
void PListClear(W8PList* ppl);
void* PListRemove(W8PList* ppl, void* pEntry);
void* PLRemoveAt(W8PList* ppl, int position);
unsigned int PLLength(W8PList* ppl);
void* PLGet(W8PList* ppl, int index);
int PListIndexOf(W8PList* ppl, void* pEntry);

#endif
