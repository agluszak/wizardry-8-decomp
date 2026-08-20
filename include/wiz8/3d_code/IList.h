#ifndef WIZ8_3D_CODE_ILIST_H
#define WIZ8_3D_CODE_ILIST_H

/*
 * 3D Code\\IList.cpp. The integer-valued sibling of PList; all ten
 * source-owned functions agree on the same three-field layout.
 */

typedef struct W8IList {
    int* data;                            /* 0x00 */
    int capacity;                         /* 0x04: IListInit allocates 10 */
    int count;                            /* 0x08 */
} W8IList;

W8IList* ILCreate(void);
unsigned char IListInit(W8IList* pls);
int IListAdd(W8IList* pls, int value);
unsigned char ILDestroy(W8IList* pls);
int IListIndexOf(W8IList* pls, int value);
unsigned char IListFreeData(W8IList* pls);
void IListClear(W8IList* pls);
int IListRemove(W8IList* pls, int value);
unsigned int ILLength(W8IList* pls);
int IListGetAt(W8IList* pls, int index);

#endif
