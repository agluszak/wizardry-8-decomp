#pragma once

#include "surrender/srMath.h"
#include "surrender/srNode.h"

struct W8World;

/* One world-cursor node: the scene node at +0x04, the three label parameters at
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
        userdata_size = 0;
    }
    virtual ~W8WorldCursorNode() {}
    /* 0x0048D050: copy the node's world location out; answers 0 when the node
       chain is absent. */
    unsigned char GetLocation(srVector3T<float>* position);
    srNode* node; /* 0x04 */
    unsigned int value_08;
    int parameters[3]; /* 0x0c, 0x10, 0x14 */
    void* pUserdata;   /* 0x18 */
    int userdata_size; /* 0x1c */
    w8_ulong color;    /* 0x20 */
    char name[0x20];
};
W8_ABI_ASSERT(sizeof(W8WorldCursorNode) == 0x44, "W8WorldCursorNode_size");

extern const double g_world_cursor_scale;

W8WorldCursorNode* CreateWorldCursorCube(void);
void DrawWorldCursorNodeLabel(W8WorldCursorNode* entry);
void MoveWorldCursorNode(W8WorldCursorNode* entry, srVector3T<float>* position);
void DestroyWorldCursorCube(W8WorldCursorNode* entry);
int GetWorldCursorNodeParameter(W8WorldCursorNode* entry, int index);
void SetWorldCursorNodeParameter(W8WorldCursorNode* entry, int index, int value);
/* Answer the next table node after `after` whose world-space bounds contain
   `point`; a null `after` starts the walk at the head of the table. */
W8WorldCursorNode* FindWorldCursorNodeAtPoint(W8WorldCursorNode* after, srVector3T<float>* point);
/* Copy the node's userdata pointer and size into the caller's slots; either
   out pointer may be null. The userdata is the scratch the master-function
   handlers flag per node. */
void GetWorldCursorNodeUserdata(W8WorldCursorNode* entry, char** buffer, int* size);
/* Allocate the node's userdata scratch, or release it when `size` is zero. */
void SetWorldCursorNodeUserdataSize(W8WorldCursorNode* entry, int size);
void ScaleWorldCursorNodeX(W8WorldCursorNode* entry, double scale);
void ScaleWorldCursorNodeY(W8WorldCursorNode* entry, double scale);
void ScaleWorldCursorNodeZ(W8WorldCursorNode* entry, double scale);
void SetWorldCursorNodeColorComponents(W8WorldCursorNode* entry, float red, float green,
                                       float blue);
/* 0x0048DCA0 forwards to the full label painter at 0x0048DCB0. */
void RefreshWorldCursorNodeLabel(W8WorldCursorNode* entry);
/* Return the indexed cursor node, or the first node when the index is past the end. */
W8WorldCursorNode* GetWorldCursorNode(int index);
void AttachWorldCursorNode(W8WorldCursorNode* entry, bool attached);
/* 0x0048F110: copy `name` into the node's 0x20-byte name with a forced
   terminator. */
void SetWorldCursorNodeName(W8WorldCursorNode* entry, const char* name);
void SetWorldCursorNodeColor(W8WorldCursorNode* entry, w8_ulong color);
/* 0x0048E3E0: the world-cursor node under the screen point, or 0. */
W8WorldCursorNode* PickWorldCursorNodeAtScreenPoint(int x, int y);
void DrawWorldBox(W8World* world, srVector3T<float> minimum, srVector3T<float> maximum,
                  w8_ulong color);
unsigned char SaveWorldCursorNodeStates(int handle);
unsigned char SaveWorldCursorNodes(int handle);
unsigned int LoadWorldCursorNodes(int handle);
unsigned int LoadWorldCursorNodeStates(int handle);
