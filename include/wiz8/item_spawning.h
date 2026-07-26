#pragma once

#include "wiz8/gameplay_boundaries.h"
#include "surrender/srMath.h"

#pragma pack(push, 1)

typedef struct W8WorldItem {
    int runtime_id;                      /* 0x00 */
    void* unknown_04;
    unsigned char unknown_08;
    W8ItemInstance item;                 /* 0x09 */
    srVector3T<float> position;          /* 0x15 */
    unsigned char unknown_21[4];
    /* 0x25 is a second flag word: the save and load paths clear bit 3 of it
       while a level restore is in progress. */
    int unknown_25;
    unsigned int flags;                  /* 0x29: tested by mask by 0x004F8130 */
    /* Both established by the LoadSaveGame.cpp serializers: the marker is set
       to 1 before each record is written, and the chain is walked and rebuilt
       through the link. The link is written to the file and reloaded with the
       record, so on disk it only records that another entry follows. */
    int saved_marker;                    /* 0x2d */
    struct W8WorldItem* next;            /* 0x31 */
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
