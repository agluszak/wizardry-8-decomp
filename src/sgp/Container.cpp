/* Modified for the Wizardry 8 reconstruction, 2026-10-03.
   Add matching markers for retained SGP functions and globals.
   Collapse the released JA2, utility, and precompiled-header branches to the Wizardry build.
   Remove released functions that are neither retained in the Wizardry 8 retail image nor referenced by retained code.
   Formatting normalized for the Wizardry 8 reconstruction, 2026-10-06.
   Distributed under the accompanying SFI Source Code license agreement. */
//*****************************************************************************
//
// Filename : Container.c
//
// Purpose : Function definition for the Container
//
// Modification History :
// 25 nov 96 TS creation
//
// 19 Dec 97 AM Replace all memcpy() with memmove(), since overlap possibilities
//							abound (and there were already bugs in do_copy() for ordered lists)
//							While the memcpy() was working in _DEBUG mode, it was failing
//							reproducibly in RELEASE mode whenever regions overlapped!
//							Having read the code, I strongly suggest that you DO NOT use this
//							stuff at all and write your own instead.  Tarun was no Carmack...
//																									- Alex Meduna
// 1998	KM Detached all references to this file from JA2 as it caused a lot of hard to debug
//         crashes.  The VOBJECT/VSURFACE lists are now self-maintained and no longer use the
//				 this crap.  DON'T USE THIS -- NO MATTER WHAT!!!
//*****************************************************************************

#include "types.h"
#include <stdlib.h>
#include <malloc.h>
#include <stdio.h>
#include "windows.h"
#include "MemMan.h"
#include "Debug.h"
#include "Container.h"
#include <iostream.h>

//*****************************************************************************
//
// Defines and typedefs
//
//
//*****************************************************************************
#define STRICT

typedef struct StackHeaderTag {
    UINT32 uiTotal_items;
    UINT32 uiSiz_of_elem;
    UINT32 uiMax_size;

} StackHeader;

typedef struct HeaderTag {
    UINT32 uiTotal_items;
    UINT32 uiSiz_of_elem;
    UINT32 uiMax_size;
    UINT32 uiHead;
    UINT32 uiTail;

} QueueHeader, ListHeader;

typedef struct OrdHeaderTag {
    UINT32 uiTotal_items;
    UINT32 uiSiz_of_elem;
    UINT32 uiMax_size;
    UINT32 uiHead;
    UINT32 uiTail;
    INT8 (*pCompare)(void*, void*, UINT32);

} OrdListHeader;

typedef struct test {
    UINT32 me;
    long you;
    char* k;
    char* p;

} TEST;

//*****************************************************************************
//
// CreateStack
//
// Parameter List : num_items - estimated number
//									of items in stack
//									siz_each - size of each item
// Return Value	NULL if unsuccesful
//							 pointer to allocated memory
// Modification History :
// Nov 26th 1996 -> modified for use by Wizardry
//
//*****************************************************************************

// FUNCTION: WIZ8 0x00405970
HSTACK CreateStack(UINT32 uiNum_items, UINT32 uiSiz_each)
{
    UINT32 uiAmount;
    HSTACK hStack;
    StackHeader* pStack;

    // assign an initial amount of memory to allocate
    if ((uiNum_items > 0) && (uiSiz_each > 0))
        uiAmount = uiNum_items * uiSiz_each;
    else {
        DbgMessage(TOPIC_STACK_CONTAINERS, DBG_LEVEL_0,
                   "Requested stack items and size have to be >0");
        return NULL;
    }
    // allocate the container memory
    if ((hStack = MemAlloc(uiAmount + sizeof(StackHeader))) == 0) {
        DbgMessage(TOPIC_STACK_CONTAINERS, DBG_LEVEL_0,
                   "Could not allocate stack container memory");
        return NULL;
    }
    pStack = (StackHeader*)hStack;
    //initialize the header variables
    pStack->uiMax_size = uiAmount + sizeof(StackHeader);
    pStack->uiTotal_items = 0;
    pStack->uiSiz_of_elem = uiSiz_each;

    // return the pointer to the memory

    return hStack;
}

//*****************************************************************************
//
// CreateList
//
// Parameter List : num_items - estimated number
//									of items in ordered list
//									siz_each - size of each item
// Return Value	NULL if unsuccesful
//							 pointer to allocated memory
// Modification History :
// Nov 26th 1996 -> modified for use by Wizardry
//
//*****************************************************************************
// FUNCTION: WIZ8 0x004059b0
HLIST CreateList(UINT32 uiNum_items, UINT32 uiSiz_each)
{
    UINT32 uiAmount;
    HLIST hList;
    ListHeader* pList;

    // check to see if the queue has more than 1
    // element to be created and that the size > 1

    if ((uiNum_items > 0) && (uiSiz_each > 0))
        uiAmount = uiNum_items * uiSiz_each;
    else {
        DbgMessage(TOPIC_LIST_CONTAINERS, DBG_LEVEL_0,
                   "Requested queue items and size have to be >0");
        return 0;
    }

    // allocate the list memory
    if ((hList = MemAlloc(uiAmount + sizeof(ListHeader))) == 0) {
        DbgMessage(TOPIC_LIST_CONTAINERS, DBG_LEVEL_0, "Could not allocate queue container memory");
        return 0;
    }

    pList = (ListHeader*)hList;
    //initialize the list structure

    pList->uiMax_size = uiAmount + sizeof(ListHeader);
    pList->uiTotal_items = 0;
    pList->uiSiz_of_elem = uiSiz_each;
    pList->uiTail = pList->uiHead = sizeof(ListHeader);

    // return the pointer to memory

    return hList;
}

//*****************************************************************************
//
// push
//
// Parameter List : void * - pointer to stack
//									container
//									data - data to add to stack
//
// Return Value	BOOLEAN true if push ok
//							 else	false
//
// Modification History :
// Nov 26th 1996 -> modified for use by Wizardry
//
//*****************************************************************************
// FUNCTION: WIZ8 0x00405a00
HSTACK Push(HSTACK hStack, void* pdata)
{
    StackHeader* pTemp_cont;
    UINT32 uiOffset;
    UINT32 uiNew_size;
    void* pvoid;
    BYTE* pbyte;

    // check for a NULL pointer

    if (hStack == NULL) {
        DbgMessage(TOPIC_STACK_CONTAINERS, DBG_LEVEL_0, "This is not a valid pointer to the stack");
        return NULL;
    }

    // some valid data should be passed in
    if (pdata == NULL) {
        DbgMessage(TOPIC_STACK_CONTAINERS, DBG_LEVEL_0, "Data to be pushed onto stack is NULL");
        return NULL;
    }

    //perform operations to calculate offset and decide if the container has to resized
    pTemp_cont = (StackHeader*)hStack;
    uiOffset = (pTemp_cont->uiSiz_of_elem * pTemp_cont->uiTotal_items) + sizeof(StackHeader);

    if ((uiOffset + pTemp_cont->uiSiz_of_elem) > pTemp_cont->uiMax_size) {
        uiNew_size = pTemp_cont->uiMax_size + (pTemp_cont->uiMax_size - sizeof(StackHeader));
        pTemp_cont->uiMax_size = uiNew_size;
        if ((hStack = MemRealloc(hStack, uiNew_size)) == NULL) {
            DbgMessage(TOPIC_STACK_CONTAINERS, DBG_LEVEL_0,
                       "Could not resize stack container memory");
            return NULL;
        }
        pTemp_cont = (StackHeader*)hStack;
    }
    pbyte = (BYTE*)hStack;
    pbyte += uiOffset;
    pvoid = (void*)pbyte;
    //copy data from pdata to pvoid - the stack
    memmove(pvoid, pdata, pTemp_cont->uiSiz_of_elem);
    pTemp_cont->uiTotal_items++;
    //return push succeeded
    return hStack;
}
//*****************************************************************************
//
// pop
//
// Parameter List : void * - pointer to stack
//									container
//
//
// Return Value : void * - pointer to stack
//								after pushing element
//
// Modification History :
// Nov 26th 1996 -> modified for use by Wizardry
//
//*****************************************************************************
// FUNCTION: WIZ8 0x00405a70
BOOLEAN Pop(HSTACK hStack, void* pdata)
{
    StackHeader* pTemp_cont;
    UINT32 uiOffset;
    UINT32 uiSize_of_each;
    UINT32 uiTotal;
    void* pvoid;
    BYTE* pbyte;

    // check for a NULL queue

    if (hStack == NULL) {
        DbgMessage(TOPIC_STACK_CONTAINERS, DBG_LEVEL_0, "This is not a valid pointer to the stack");
        return FALSE;
    }
    if (pdata == NULL) {
        DbgMessage(TOPIC_STACK_CONTAINERS, DBG_LEVEL_0,
                   "Variable where data is to be stored is NULL");
        return FALSE;
    }
    pTemp_cont = (StackHeader*)hStack;
    uiTotal = pTemp_cont->uiTotal_items;
    uiSize_of_each = pTemp_cont->uiSiz_of_elem;
    if (uiTotal == 0) {
        DbgMessage(TOPIC_STACK_CONTAINERS, DBG_LEVEL_0, "There is no data in stack to pop");
        return FALSE;
    }

    // calculate offsets to decide if the page should be rezied
    uiOffset = (uiSize_of_each * uiTotal) + sizeof(StackHeader);
    uiOffset -= uiSize_of_each;
    pbyte = (BYTE*)hStack;
    pbyte += uiOffset;
    pvoid = (void*)pbyte;
    // get the data from pvoid and store in pdata
    memmove(pdata, pvoid, uiSize_of_each);
    pTemp_cont->uiTotal_items--;
    return TRUE;
}
//*****************************************************************************
//
// PeekStack
//
// Parameter List : void * - buffer to hold data
//
//
// Return Value : TRUE if stack not empty
//
// Modification History :
// Apr 14 2000 SCT -> Created
//
//*****************************************************************************
// FUNCTION: WIZ8 0x00405ac0
BOOLEAN PeekStack(HSTACK hStack, void* pdata)
{
    StackHeader* pTemp_cont;
    UINT32 uiOffset;
    UINT32 uiSize_of_each;
    UINT32 uiTotal;
    void* pvoid;
    BYTE* pbyte;

    // check for a NULL queue

    if (hStack == NULL) {
        DbgMessage(TOPIC_STACK_CONTAINERS, DBG_LEVEL_0, "This is not a valid pointer to the stack");
        return FALSE;
    }
    if (pdata == NULL) {
        DbgMessage(TOPIC_STACK_CONTAINERS, DBG_LEVEL_0,
                   "Variable where data is to be stored is NULL");
        return FALSE;
    }
    pTemp_cont = (StackHeader*)hStack;
    uiTotal = pTemp_cont->uiTotal_items;
    uiSize_of_each = pTemp_cont->uiSiz_of_elem;
    if (uiTotal == 0) {
        DbgMessage(TOPIC_STACK_CONTAINERS, DBG_LEVEL_0, "There is no data in stack to pop");
        return FALSE;
    }

    // calculate offsets to decide if the page should be rezied
    uiOffset = (uiSize_of_each * uiTotal) + sizeof(StackHeader);
    uiOffset -= uiSize_of_each;
    pbyte = (BYTE*)hStack;
    pbyte += uiOffset;
    pvoid = (void*)pbyte;
    // get the data from pvoid and store in pdata
    memmove(pdata, pvoid, uiSize_of_each);
    return TRUE;
}
//*****************************************************************************
//
// DeleteStack
//
// Parameter List : pointer to memory
//
// Return Value	: BOOLEAN
//
// Modification History :
// Nov 26th 1996 -> modified for use by Wizardry
//
//*****************************************************************************
// FUNCTION: WIZ8 0x00405b00
BOOLEAN DeleteStack(HSTACK hStack)
{
    if (hStack == NULL) {
        DbgMessage(TOPIC_STACK_CONTAINERS, DBG_LEVEL_0, "This is not a valid pointer to the stack");
        return FALSE;
    }
    // free the memory assigned to the handle
    MemFree(hStack);
    return TRUE;
}
//*****************************************************************************
//
// DeleteList
//
// Parameter List : pointer to memory
//
// Return Value	: BOOLEAN
//
// Modification History :
// Nov 26th 1996 -> modified for use by Wizardry
//
//*****************************************************************************
BOOLEAN DeleteList(HLIST hList)
{
    if (hList == NULL) {
        DbgMessage(TOPIC_LIST_CONTAINERS, DBG_LEVEL_0, "This is not a valid pointer to the list");
        return FALSE;
    }
    // free the memory assigned to the list
    MemFree(hList);
    return TRUE;
}
//*****************************************************************************
//
// InitializeContainers
//
// Parameter List : none
//
// Return Value	: void
//
// Modification History :
// Nov 26th 1996 -> modified for use by Wizardry
//
//*****************************************************************************

void InitializeContainers(void)
{
    // register the appropriate debug topics
    RegisterDebugTopic(TOPIC_STACK_CONTAINERS, "Stack Container");
    RegisterDebugTopic(TOPIC_LIST_CONTAINERS, "List Container");
    RegisterDebugTopic(TOPIC_QUEUE_CONTAINERS, "Queue Container");
    RegisterDebugTopic(TOPIC_ORDLIST_CONTAINERS, "Ordered List Container");
}

//*****************************************************************************
//
// ShutdownContainers
//
// Parameter List : none
//
// Return Value	: void
//
// Modification History :
// Nov 26th 1996 -> modified for use by Wizardry
//
//*****************************************************************************

void ShutdownContainers(void)
{
    UnRegisterDebugTopic(TOPIC_STACK_CONTAINERS, "Stack Container");
    UnRegisterDebugTopic(TOPIC_LIST_CONTAINERS, "List Container");
    UnRegisterDebugTopic(TOPIC_QUEUE_CONTAINERS, "Queue Container");
    UnRegisterDebugTopic(TOPIC_ORDLIST_CONTAINERS, "Ordered List Container");
}
//*****************************************************************************
//
// PeekList - gets the specified item in the list without
// actually deleting it.
//
// Parameter List : hList - pointer to list
//									container
//									data - data where list element is stored
//
// Return Value	BOOLEAN
//
//
// Modification History :
// Nov 26th 1996 -> modified for use by Wizardry
//
//*****************************************************************************
// FUNCTION: WIZ8 0x00405b20
BOOLEAN PeekList(HLIST hList, void* pdata, UINT32 uiPos)
{
    ListHeader* pTemp_cont;
    void* pvoid;
    UINT32 uiOffsetSrc;
    BYTE* pbyte;

    // cannot check for invalid handle , only 0
    if (hList == NULL) {
        DbgMessage(TOPIC_LIST_CONTAINERS, DBG_LEVEL_0, "This is not a valid pointer to the list");
        return FALSE;
    }
    if (pdata == NULL) {
        DbgMessage(TOPIC_LIST_CONTAINERS, DBG_LEVEL_0,
                   "Memory fo Data to be removed from list is NULL");
        return FALSE;
    }

    //assign to temporary variables
    pTemp_cont = (ListHeader*)hList;

    // if theres no elements to peek return error
    if (pTemp_cont->uiTotal_items == 0) {
        DbgMessage(TOPIC_LIST_CONTAINERS, DBG_LEVEL_0, "There is nothing in the list");
        return FALSE;
    }
    if (uiPos >= pTemp_cont->uiTotal_items) {
        DbgMessage(TOPIC_LIST_CONTAINERS, DBG_LEVEL_0, "There is no item at this position");
        return FALSE;
    }

    //copy the element pointed to by uiHead
    uiOffsetSrc = pTemp_cont->uiHead + (uiPos * pTemp_cont->uiSiz_of_elem);
    if (uiOffsetSrc >= pTemp_cont->uiMax_size)
        uiOffsetSrc = sizeof(ListHeader) + (uiOffsetSrc - pTemp_cont->uiMax_size);

    pbyte = (BYTE*)hList;
    pbyte += uiOffsetSrc;
    pvoid = (void*)pbyte;
    memmove(pdata, pvoid, pTemp_cont->uiSiz_of_elem);

    return TRUE;
}

//*****************************************************************************
//
// StoreListNode - Stores the contents of a list node with the given parameter.
//									Unlike SwapListNode(), this does NOT swap previous contents
//									back into the pdata buffer!
//
// Parameter List : hList - pointer to list container
//									pdata - pointer to data to be stored
//									uiPos - List position into which to store.
//
// Return Value	BOOLEAN - TRUE if successful, FALSE if function fails.
//
//
// Modification History :
//	Added to SGP by Alex Meduna for use with Wiz8. Oct 31 '97.
//		- This function is nearly identical to the SwapListNode() function.
//
//*****************************************************************************
// FUNCTION: WIZ8 0x00405b90
BOOLEAN StoreListNode(HLIST hList, void* pdata, UINT32 uiPos)
{
    ListHeader* pTemp_cont;
    UINT32 uiOffsetSrc;
    BYTE* pbyte;

    // cannot check for invalid handle , only 0
    if (hList == NULL) {
        DbgMessage(TOPIC_LIST_CONTAINERS, DBG_LEVEL_0, "Invalid pointer to list");
        return FALSE;
    }

    if (pdata == NULL) {
        DbgMessage(TOPIC_LIST_CONTAINERS, DBG_LEVEL_0,
                   "Data pointer to be swapped from list is NULL");
        return FALSE;
    }

    //assign to temporary variables
    pTemp_cont = (ListHeader*)hList;

    // if theres no elements to peek return error
    if (pTemp_cont->uiTotal_items == 0) {
        DbgMessage(TOPIC_LIST_CONTAINERS, DBG_LEVEL_0, "Empty list");
        return FALSE;
    }

    if (uiPos >= pTemp_cont->uiTotal_items) {
        DbgMessage(TOPIC_LIST_CONTAINERS, DBG_LEVEL_0, "Invalid list position");
        return FALSE;
    }

    uiOffsetSrc = pTemp_cont->uiHead + (uiPos * pTemp_cont->uiSiz_of_elem);
    if (uiOffsetSrc >= pTemp_cont->uiMax_size)
        uiOffsetSrc = sizeof(ListHeader) + (uiOffsetSrc - pTemp_cont->uiMax_size);

    pbyte = (BYTE*)hList;
    pbyte += uiOffsetSrc;

    memmove(pbyte, pdata, pTemp_cont->uiSiz_of_elem);

    return TRUE;
}

//*****************************************************************************
//
// do_copy
//
// Parameter List : pointer to mem, source offset, dest offset, size
//
// Return Value	BOOLEAN
//
//
// Modification History :
// Nov 26th 1996 -> modified for use by Wizardry
//
//*****************************************************************************
BOOLEAN do_copy(void* pmem_void, UINT32 uiSourceOfst, UINT32 uiDestOfst, UINT32 uiSize)
{
    BYTE* pOffsetSrc;
    BYTE* pOffsetDst;
    void* pvoid_src;
    void* pvoid_dest;

    if ((uiSourceOfst < 0) || (uiDestOfst < 0) || (uiSize < 0)) {
        DbgMessage(TOPIC_LIST_CONTAINERS, DBG_LEVEL_0, "Invalid parameters passed to do_copy");
        return FALSE;
    }

    if (pmem_void == NULL) {
        DbgMessage(TOPIC_LIST_CONTAINERS, DBG_LEVEL_0, "Invalid pointer passed to do_copy");
        return FALSE;
    }
    pOffsetSrc = (BYTE*)pmem_void;
    pOffsetSrc += uiSourceOfst;
    pOffsetDst = (BYTE*)pmem_void;
    pOffsetDst += uiDestOfst;
    pvoid_src = (void*)pOffsetSrc;
    pvoid_dest = (void*)pOffsetDst;
    memmove(pvoid_dest, pvoid_src, uiSize);
    return TRUE;
}
//*****************************************************************************
//
// StackSize
//
// Parameter List : pointer to stack
//
// Return Value	UINT32 stack size
//
//
// Modification History :
// Nov 26th 1996 -> modified for use by Wizardry
//
//*****************************************************************************
// FUNCTION: WIZ8 0x00405c00
UINT32 StackSize(HSTACK hStack)
{
    StackHeader* pTemp_cont;
    if (hStack == NULL) {
        DbgMessage(TOPIC_LIST_CONTAINERS, DBG_LEVEL_0, "Stack pointer is NULL");
        return 0;
    }
    pTemp_cont = (StackHeader*)hStack;
    return pTemp_cont->uiTotal_items;
}
//*****************************************************************************
//
// ListSize
//
// Parameter List : pointer to queue
//
// Return Value	UINT32 list size
//
//
// Modification History :
// Nov 26th 1996 -> modified for use by Wizardry
//
//*****************************************************************************
UINT32 ListSize(HLIST hList)
{
    ListHeader* pTemp_cont;
    if (hList == NULL) {
        DbgMessage(TOPIC_LIST_CONTAINERS, DBG_LEVEL_0, "List pointer is NULL");
        return 0;
    }
    pTemp_cont = (ListHeader*)hList;
    return pTemp_cont->uiTotal_items;
}
//*****************************************************************************
//
// AddtoList
//
// Parameter List : HCONTAINER - handle to list
//									container
//									data - data to add to queue
//									position - position after which data is to added
//
// Return Value	BOOLEAN true if push ok
//							 else	false
//
// Modification History :
// Nov 26th 1996 -> modified for use by Wizardry
//
//*****************************************************************************
// FUNCTION: WIZ8 0x00405c10
HLIST AddtoList(HLIST hList, void* pdata, UINT32 uiPos)
{
    ListHeader* pTemp_cont;
    UINT32 uiMax_size;
    UINT32 uiSize_of_each;
    UINT32 uiTotal;
    UINT32 uiNew_size;
    UINT32 uiHead;
    UINT32 uiTail;
    void* pvoid;
    BYTE* pbyte;
    UINT32 uiOffsetSrc;
    UINT32 uiOffsetDst;
    UINT32 uiFinalLoc = 0;
    BOOLEAN fTail_check = FALSE;

    // check for invalid handle = 0
    if (hList == NULL) {
        DbgMessage(TOPIC_LIST_CONTAINERS, DBG_LEVEL_0, "This is not a valid handle to the list");
        return NULL;
    }

    // check for data = NULL
    if (pdata == NULL) {
        DbgMessage(TOPIC_LIST_CONTAINERS, DBG_LEVEL_0, "Data to be pushed onto list is NULL");
        return NULL;
    }
    // check for a 0 or negative position passed in
    if (uiPos < 0) {
        DbgMessage(TOPIC_LIST_CONTAINERS, DBG_LEVEL_0, "Data to be pushed onto list is NULL");
        return NULL;
    }

    // assign some temporary variables

    pTemp_cont = (ListHeader*)hList;
    if (uiPos > pTemp_cont->uiTotal_items) {
        DbgMessage(TOPIC_LIST_CONTAINERS, DBG_LEVEL_0, "There are not enough elements in the list");
        return NULL;
    }
    uiTotal = pTemp_cont->uiTotal_items;
    uiSize_of_each = pTemp_cont->uiSiz_of_elem;
    uiMax_size = pTemp_cont->uiMax_size;
    uiHead = pTemp_cont->uiHead;
    uiTail = pTemp_cont->uiTail;
    uiOffsetSrc = pTemp_cont->uiHead + (uiPos * pTemp_cont->uiSiz_of_elem);
    if (uiOffsetSrc >= uiMax_size)
        uiOffsetSrc = sizeof(ListHeader) + (uiOffsetSrc - uiMax_size);
    if (uiTail == uiOffsetSrc)
        fTail_check = TRUE;
    // copy appropriate blocks
    if (((uiTail + uiSize_of_each) <= uiMax_size) &&
        ((uiTail > uiHead) || ((uiTail == uiHead) && (uiHead == sizeof(ListHeader))))) {
        uiOffsetSrc = pTemp_cont->uiHead + (uiPos * pTemp_cont->uiSiz_of_elem);
        uiOffsetDst = uiOffsetSrc + pTemp_cont->uiSiz_of_elem;
        if (fTail_check == FALSE) {
            if (do_copy(hList, uiOffsetSrc, uiOffsetDst, uiTail - uiOffsetSrc) == FALSE) {
                DbgMessage(TOPIC_LIST_CONTAINERS, DBG_LEVEL_0, "Could not store the data in list");
                return NULL;
            }
        }
        if (fTail_check == FALSE)
            pTemp_cont->uiTail += uiSize_of_each;
        uiFinalLoc = uiOffsetSrc;
    }

    if ((((uiTail + uiSize_of_each) <= uiMax_size) && (uiTail < uiHead)) ||
        (((uiTail + uiSize_of_each) > uiMax_size) &&
         (uiHead >= (sizeof(ListHeader) + uiSize_of_each)))) {
        uiOffsetSrc = pTemp_cont->uiHead + (uiPos * pTemp_cont->uiSiz_of_elem);

        if (uiOffsetSrc >= uiMax_size) {
            uiOffsetSrc = sizeof(ListHeader) + (uiOffsetSrc - uiMax_size);
            uiOffsetDst = uiOffsetSrc + uiSize_of_each;
            if (do_copy(hList, uiOffsetDst, uiOffsetSrc, uiTail - uiOffsetSrc) == FALSE) {
                DbgMessage(TOPIC_LIST_CONTAINERS, DBG_LEVEL_0, "Could not store the data in list");
                return NULL;
            }
            uiFinalLoc = uiOffsetSrc;
        } else {
            uiOffsetSrc = sizeof(ListHeader);
            uiOffsetDst = uiOffsetSrc + uiSize_of_each;
            if (do_copy(hList, uiOffsetSrc, uiOffsetDst, uiTail - uiOffsetSrc) == FALSE) {
                DbgMessage(TOPIC_LIST_CONTAINERS, DBG_LEVEL_0, "Could not store the data in list");
                return NULL;
            }

            uiOffsetSrc = uiMax_size - uiSize_of_each;
            uiOffsetDst = sizeof(ListHeader);
            if (do_copy(hList, uiOffsetSrc, uiOffsetDst, uiSize_of_each) == FALSE) {
                DbgMessage(TOPIC_LIST_CONTAINERS, DBG_LEVEL_0, "Could not store the data in list");
                return NULL;
            }
            uiOffsetSrc = pTemp_cont->uiHead + (uiPos * pTemp_cont->uiSiz_of_elem);
            uiOffsetDst = uiOffsetSrc + uiSize_of_each;
            if (do_copy(hList, uiOffsetSrc, uiOffsetDst,
                        (uiMax_size - uiSize_of_each) - uiOffsetSrc) == FALSE) {
                DbgMessage(TOPIC_LIST_CONTAINERS, DBG_LEVEL_0, "Could not store the data in list");
                return NULL;
            }
        }
        pTemp_cont->uiTail += uiSize_of_each;
        uiFinalLoc = uiOffsetSrc;
    } // end if

    if ((((uiTail + uiSize_of_each) <= uiMax_size) && (uiTail == uiHead) &&
         (uiHead >= (sizeof(ListHeader) + uiSize_of_each))) ||
        (((uiTail + uiSize_of_each) > uiMax_size) && (uiHead == sizeof(ListHeader)))) {
        // need to resize the container
        uiNew_size = uiMax_size + (uiMax_size - sizeof(ListHeader));
        pTemp_cont->uiMax_size = uiNew_size;
        if ((hList = MemRealloc(hList, uiNew_size)) == NULL) {
            DbgMessage(TOPIC_LIST_CONTAINERS, DBG_LEVEL_0,
                       "Could not resize list container memory");
            return NULL;
        }
        pTemp_cont = (ListHeader*)hList;
        if (do_copy(hList, sizeof(ListHeader), uiMax_size, uiHead - sizeof(ListHeader)) == FALSE) {
            DbgMessage(TOPIC_LIST_CONTAINERS, DBG_LEVEL_0, "Could not copy list container memory");
            return NULL;
        }
        pTemp_cont->uiTail = uiMax_size + (uiHead - sizeof(ListHeader));

        // now make place for the actual element

        uiOffsetSrc = pTemp_cont->uiHead + (uiPos * pTemp_cont->uiSiz_of_elem);
        uiOffsetDst = uiOffsetSrc + pTemp_cont->uiSiz_of_elem;
        if (do_copy(hList, uiOffsetSrc, uiOffsetDst, uiTail - uiOffsetSrc) == FALSE) {
            DbgMessage(TOPIC_LIST_CONTAINERS, DBG_LEVEL_0, "Could not store the data in list");
            return NULL;
        }
        pTemp_cont->uiTail += uiSize_of_each;
        uiFinalLoc = uiOffsetSrc;
    }

    // finally insert data at position uiFinalLoc

    pbyte = (BYTE*)hList;
    if (uiFinalLoc == 0) {
        DbgMessage(TOPIC_LIST_CONTAINERS, DBG_LEVEL_0,
                   "This should never happen! report this problem!");
        return NULL;
    }
    pbyte += uiFinalLoc;
    pvoid = (void*)pbyte;

    memmove(pvoid, pdata, pTemp_cont->uiSiz_of_elem);
    pTemp_cont->uiTotal_items++;
    if (fTail_check == TRUE)
        pTemp_cont->uiTail += pTemp_cont->uiSiz_of_elem;
    return hList;
}
