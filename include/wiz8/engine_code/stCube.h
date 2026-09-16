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
        buffer_18 = 0;
        size_1c = 0;
    }
    virtual ~W8WorldCursorNode() {}
    /* 0x0048D050: copy the node's world location out; answers 0 when the node
       chain is absent. */
    unsigned char GetLocation0048D050(srVector3T<float>* position);
    srNode* node_04; /* 0x04 */
    unsigned char unknown_08[4];
    int numbers_0c[3];              /* 0x0c, 0x10, 0x14 */
    void* buffer_18;                /* 0x18 */
    int size_1c;                    /* 0x1c */
    unsigned long color_20;         /* 0x20 */
    unsigned char flag_24;          /* 0x24 */
    unsigned char unknown_25[0x1f]; /* 0x25 */
};
static_assert(sizeof(W8WorldCursorNode) == 0x44, "W8WorldCursorNode_size");

W8WorldCursorNode* CreateWorldCursorCube0048D080(void);
void DrawWorldCursorNodeLabel0048DCB0(W8WorldCursorNode* entry);
void MoveWorldCursorNode0048DBF0(W8WorldCursorNode* entry, srVector3T<float>* position);
void DestroyWorldCursorCube0048DA80(W8WorldCursorNode* entry);
int GetWorldCursorNodeParameter0048E2B0(W8WorldCursorNode* entry, int index);
void SetWorldCursorNodeParameter0048E2D0(W8WorldCursorNode* entry, int index, int value);
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
void DrawWorldBox0048DF30(W8World* world, srVector3T<float> minimum, srVector3T<float> maximum,
                          unsigned long color);
void Function48E6D0(int handle);
void Function48EAD0(int handle);
unsigned int LoadWorldCursorNodes0048E7B0(int handle);
unsigned int LoadWorldCursorNodeStates0048E470(int handle);
