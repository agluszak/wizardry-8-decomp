#ifndef WIZ8_ENGINE_CODE_CAMERA_H
#define WIZ8_ENGINE_CODE_CAMERA_H

struct W8World;
struct W8PathAI;
struct W8MonsterInfo;
template <class T> class srVector3T;

/* One entry of a W8World camera-path list (the W8PList at W8World+0x0c).
   The name occupies offset zero - callers _stricmp the record pointer
   directly; the flag and the path engine pointer are what
   UpdateCameraPathState0048F2F0 toggles. */
struct W8CameraPath {
    char name_00[0x14];
    bool active_14;
    unsigned char unknown_15[3];
    W8PathAI* path_18;
};

/* Engine Code\Camera.cpp. The TU's only anchor; turns a camera path on and
   off for the world and dispatches the per-path end actions. */
void UpdateCameraPathState0048F2F0(W8World* world, W8CameraPath* path, float fTime);
/* 0x0048F650: face the camera at a monster's head; force overrides the
   tracking-mode gate, animate chooses the eased transition over the snap. */
void PointCameraAtMonster(W8MonsterInfo* monster_info, unsigned char force, unsigned char animate);
/* 0x0048F800: the position-taking variant of the camera orientation helper. */
void PointCameraAtTarget(srVector3T<float>* position, unsigned char force, unsigned char animate);

#endif
