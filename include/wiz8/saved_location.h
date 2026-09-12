#ifndef WIZ8_SAVED_LOCATION_H
#define WIZ8_SAVED_LOCATION_H

#include "surrender/srMath.h"

#pragma pack(push, 1)

/* The 0x3c-byte CamPos record named by GetWorldCameraState and
   RestoreWorldCameraState (3dapi.cpp:1092, 1102, 1160). Get writes the camera
   location, then zeros two six-word records and stores GDCamera yaw at +0x24
   and pitch at +0x0c. Restore reads those same first floats through
   SetCameraOrientation.

   This is the party/camera pose snapshot, not srMatrix4x3T and not a
   decomposed mat3+scale transform. The character recall anchor, the pending
   cross-level move, and the automap camera save are this record: spell 0x4b
   fills a character's copy with GetWorldCameraState, recall and LoadLevel
   pass that copy to RestoreWorldCameraState, and the cross-level path copies
   all 0x3c bytes with one rep movsd. */
struct W8CamPos {
    srVector3T<float> position; /* 0x00 */
    float pitch_record[6];      /* 0x0c */
    float yaw_record[6];        /* 0x24 */
}; /* 0x3c */

#pragma pack(pop)

static_assert(sizeof(W8CamPos) == 0x3c, "W8CamPos_must_be_0x3c");

#endif
