#pragma once

#include "gameplay_boundaries.h"
#include "surrender/srMath.h"

#pragma pack(push, 1)

typedef struct W8WorldItem {
    int runtime_id;                      /* 0x00 */
    void* unknown_04;
    unsigned char unknown_08;
    W8ItemInstance item;                 /* 0x09 */
    srVector3T<float> position;          /* 0x15 */
    unsigned char unknown_21[4];
    int unknown_25;
    unsigned int flags;                  /* 0x29 */
    unsigned char unknown_2d[8];
    int unknown_35;
    int sector_id;                       /* 0x39 */
    unsigned char unknown_3d[0x70];
} W8WorldItem;                           /* 0xad */

#pragma pack(pop)

extern "C" {

W8WorldItem* CreateWorldItem(
    const W8ItemInstance* item,
    const srVector3T<float>* position,
    int unknown,
    unsigned char add_to_world);
W8WorldItem* SpawnItem(
    int item_id,
    const srVector3T<float>* position,
    int unknown,
    unsigned char add_to_world);

}
