#ifndef WIZ8_3D_CODE_PLIST_H
#define WIZ8_3D_CODE_PLIST_H

#include "wiz8/layouts/plist.h"

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
