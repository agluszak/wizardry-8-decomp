#pragma once

#include "surrender/srMath.h"

struct W8World;
class W8WorldCursorNode0048DB30;

W8WorldCursorNode0048DB30* CreateWorldCursorCube0048D080(void);
void DrawWorldCursorNodeLabel0048DCB0(W8WorldCursorNode0048DB30* entry);
void SetWorldCursorNodeColor0048E400(W8WorldCursorNode0048DB30* entry, unsigned long color);
void DrawWorldBox0048DF30(W8World* world, srVector3T<float> minimum, srVector3T<float> maximum,
                          unsigned long color);
void Function48E6D0(int handle);
void Function48EAD0(int handle);
