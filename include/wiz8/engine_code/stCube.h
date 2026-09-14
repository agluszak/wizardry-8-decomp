#pragma once

#include "surrender/srMath.h"

struct W8World;
class W8WorldCursorNode;

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
