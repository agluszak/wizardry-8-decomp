#ifndef WIZ8_3D_CODE_PLIST_H
#define WIZ8_3D_CODE_PLIST_H

/*
 * 3D Code\\PList.cpp. A growable array of void pointers - the engine's
 * general-purpose list, distinct from the typed growable-vector template.
 * All twelve source-owned functions agree on this layout.
 */

typedef struct W8PList {
    void** data;                          /* 0x00 */
    int capacity;                         /* 0x04: PListInit allocates 10 */
    int count;                            /* 0x08 */
} W8PList;

W8PList* PListCreate(void);
unsigned char PListInit(W8PList* ppl);
unsigned char PListDestroy(W8PList* ppl);
unsigned char PListFreeData(W8PList* ppl);
int PListAdd(W8PList* ppl, void* pEntry);
int PListInsert(W8PList* ppl, int position, void* pEntry);
void PListClear(W8PList* ppl);
void* PListRemove(W8PList* ppl, void* pEntry);
void* PListRemoveAt(W8PList* ppl, int position);
unsigned int PListGetCount(W8PList* ppl);
void* PListGetAt(W8PList* ppl, int index);
int PListIndexOf(W8PList* ppl, void* pEntry);

template <class T>
void PListDestructor(W8PList* list);

#endif
