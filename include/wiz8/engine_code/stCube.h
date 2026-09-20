#pragma once

#include "surrender/srMath.h"
#include "surrender/srNode.h"

struct W8World;

/* One world-cursor node: the scene node at +0x04, the three label numbers at
   +0x0c/+0x10/+0x14, the text scratch buffer at +0x18 and its length at +0x1c,
   the packed fill colour at +0x20. The 0x0048E6D0 save body writes the 0x20
   bytes at +0x24 and then that buffer and length. The TU name stCube.cpp and
   the runtime model name "stCube" are suggestive but do not prove the class
   spelling, so the name is a recovered descriptive name. Retail writes the two
   zeros before the vptr; the C++ constructor installs the vptr first. */
// VTABLE: WIZ8 0x005ecab8
class W8WorldCursorNode {
public:
    W8WorldCursorNode()
    {
        pUserdata = 0;
        size_1c = 0;
    }
    virtual ~W8WorldCursorNode() {}
    /* 0x0048D050: copy the node's world location out; answers 0 when the node
       chain is absent. */
    unsigned char GetLocation0048D050(srVector3T<float>* position);
    srNode* node_04; /* 0x04 */
    unsigned int value_08;
    int numbers_0c[3];      /* 0x0c, 0x10, 0x14 */
    void* pUserdata;        /* 0x18 */
    int size_1c;            /* 0x1c */
    unsigned long color_20; /* 0x20 */
    char name_24[0x20];
};
static_assert(sizeof(W8WorldCursorNode) == 0x44, "W8WorldCursorNode_size");

extern double g_world_cursor_scale_005ebf50;

W8WorldCursorNode* CreateWorldCursorCube0048D080(void);
void DrawWorldCursorNodeLabel0048DCB0(W8WorldCursorNode* entry);
void MoveWorldCursorNode0048DBF0(W8WorldCursorNode* entry, srVector3T<float>* position);
void DestroyWorldCursorCube0048DA80(W8WorldCursorNode* entry);
int GetWorldCursorNodeParameter0048E2B0(W8WorldCursorNode* entry, int index);
void SetWorldCursorNodeParameter0048E2D0(W8WorldCursorNode* entry, int index, int value);
/* Answer the next table node after `after` whose world-space bounds contain
   `point`; a null `after` starts the walk at the head of the table. */
W8WorldCursorNode* FindWorldCursorNodeAtPoint0048EDD0(W8WorldCursorNode* after,
                                                      srVector3T<float>* point);
/* Copy the node's userdata pointer and size into the caller's slots; either
   out pointer may be null. The userdata is the scratch the master-function
   handlers flag per node. */
void GetWorldCursorNodeUserdata0048EF00(W8WorldCursorNode* entry, char** buffer, int* size);
/* Allocate the node's userdata scratch, or release it when `size` is zero. */
void SetWorldCursorNodeUserdataSize0048EF40(W8WorldCursorNode* entry, int size);
void ScaleWorldCursorNodeX0048DE40(W8WorldCursorNode* entry, double scale);
void ScaleWorldCursorNodeY0048DE90(W8WorldCursorNode* entry, double scale);
void ScaleWorldCursorNodeZ0048DEE0(W8WorldCursorNode* entry, double scale);
void SetWorldCursorNodeColorComponents0048E420(W8WorldCursorNode* entry, float red, float green,
                                               float blue);
/* 0x0048DCA0 forwards to the full label painter at 0x0048DCB0. */
void RefreshWorldCursorNodeLabel0048DCA0(W8WorldCursorNode* entry);
/* Return the indexed cursor node, or the first node when the index is past the end. */
W8WorldCursorNode* GetWorldCursorNode0048ED10(int index);
void AttachWorldCursorNode0048ED30(W8WorldCursorNode* entry, unsigned char attached);
void SetWorldCursorNodeName0048F110(W8WorldCursorNode* entry, const char* name);
void SetWorldCursorNodeColor0048E400(W8WorldCursorNode* entry, unsigned long color);
/* 0x0048E3E0: the world-cursor node under the screen point, or 0. */
W8WorldCursorNode* PickWorldCursorNodeAtScreenPoint0048E3E0(int x, int y);
void DrawWorldBox0048DF30(W8World* world, srVector3T<float> minimum, srVector3T<float> maximum,
                          unsigned long color);
unsigned char SaveWorldCursorNodeStates0048E6D0(int handle);
unsigned char SaveWorldCursorNodes0048EAD0(int handle);
unsigned int LoadWorldCursorNodes0048E7B0(int handle);
unsigned int LoadWorldCursorNodeStates0048E470(int handle);
