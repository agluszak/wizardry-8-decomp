#ifndef WIZ8_ENGINE_CODE_CAMERA_H
#define WIZ8_ENGINE_CODE_CAMERA_H

struct W8World;
struct W8PathAI;
template <class T> class srVector3T;

/* One entry of a W8World camera-path list (the W8PList at W8World+0x0c).
   The name occupies offset zero - callers _stricmp the record pointer
   directly; the flag and the path engine pointer are what
   UpdateCameraPathState0048F2F0 toggles. */
struct W8CameraPath {
    char name_00[0x14];
    unsigned char active_14;
    unsigned char unknown_15[3];
    W8PathAI* path_18;
};

/* Engine Code\Camera.cpp. The TU's only anchor; turns a camera path on and
   off for the world and dispatches the per-path end actions. */
void UpdateCameraPathState0048F2F0(W8World* world, W8CameraPath* path, float fTime);

/* Unresolved gap callees, declared for the call sites in this unit. */
/* 0x0041AAE0: stashes and returns the environment load flag while a camera
   path runs. */
unsigned char Function41AAE0(unsigned char flag);
/* 0x00420FB0: re-aims something at the tracked point when tracking fails. */
void Function420FB0(const srVector3T<float>* target);

#endif
