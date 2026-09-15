#pragma once

template <class T> class srVector3T;

/* 0x00490AF0: toggle the 3D world cursor. If a cursor exists, release it;
   if not, initialize one. */
void ToggleWorldCursor(void);
/* 0x004914E0: when the cursor's group-bind flag is set, locate the monster
   group recorded on the cursor and attach its leader at the cursor point. */
void BindCursorMonsterToGroup004914E0(void);
/* 0x00491EC0: place the world cursor's target point g_float_60ab48 units in
   front of the camera and re-derive the cursor position from it. */
void UpdateWorldCursorPlacement00491EC0(void);
/* 0x004919E0: unresolved gap body, declared for
   UpdateWorldCursorPlacement00491EC0. */
char Function4919E0(srVector3T<float>* position);
/* 0x004921E0: resolve the world cursor's target position into the out
   vector: copy the cursor position, ground-probe its candidate offsets and
   keep the settled height. */
int ResolveWorldCursorTarget004921E0(srVector3T<float>* position);
void GetWorldCursorTargetPosition00492500(srVector3T<float>* position);
void SetFloat60AB48(void); /* 0x00492530 */
