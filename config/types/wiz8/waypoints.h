#ifndef WIZ8_FORMATS_WAYPOINTS_H
#define WIZ8_FORMATS_WAYPOINTS_H

#include <stdint.h>

#pragma pack(push, 1)

typedef struct W8WaypointFileHeader {
    uint32_t version;                   /* 0x00: canonical files use version 2 */
    uint32_t unknown_04;                /* 0x04: usually zero */
    uint32_t waypoint_count;            /* 0x08: includes the index-zero sentinel */
    uint32_t link_count;                /* 0x0c: includes the index-zero sentinel */
} W8WaypointFileHeader;                 /* 0x10 */

typedef struct W8WaypointDisk {
    uint16_t flags;                     /* 0x00 */
    uint16_t first_link;                /* 0x02: index into W8WaypointLinkDisk */
    float position[3];                  /* 0x04 */
} W8WaypointDisk;                       /* 0x10 */

typedef struct W8WaypointLinkDisk {
    uint32_t flags;                     /* 0x00 */
    uint16_t source_waypoint;           /* 0x04: absent from version 1 files */
    uint16_t destination_waypoint;      /* 0x06 */
    float distance;                     /* 0x08 */
    uint16_t next_link;                 /* 0x0c: next edge for the same source */
} W8WaypointLinkDisk;                   /* 0x0e */

#pragma pack(pop)

#endif
