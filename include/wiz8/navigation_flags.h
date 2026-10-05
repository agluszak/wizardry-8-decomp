#pragma once

/* Capability/filter masks shared by navigators and waypoint edges. */
enum {
    W8_NAV_GROUP_MASK = 0x0000ffffu,
    W8_NAV_WALK = 0x00010000u,
    W8_NAV_FLY = 0x00020000u,
    W8_NAV_SWIM = 0x00040000u,
    W8_NAV_MOVEMENT_MASK = W8_NAV_WALK | W8_NAV_FLY | W8_NAV_SWIM,
    W8_NAV_TINY = 0x00080000u,
    W8_NAV_SMALL = 0x00100000u,
    W8_NAV_MEDIUM = 0x00200000u,
    W8_NAV_LARGE = 0x00400000u,
    W8_NAV_HUGE = 0x00800000u,
    /* Retail's filter tests only the first three size bits. */
    W8_NAV_SIZE_FILTER_MASK = W8_NAV_TINY | W8_NAV_SMALL | W8_NAV_MEDIUM,
    W8_NAV_NO_GEOMETRY_COLLISION = 0x02000000u,
    W8_NAV_THROUGH_DOORS = 0x10000000u,
    W8_PATH_EDGE_TELEPORTAL = 0x01000000u,
    W8_PATH_EDGE_CONDITIONAL = 0x20000000u,
    W8_PATH_EDGE_DISABLED = 0x80000000u
};

/* Packed path-cell values have a different domain from edge capabilities.
   The middle byte is a neighbor mask when HAS_DIRECTIONS is set and clearance
   depth otherwise. The low half stores the height level. */
enum {
    W8_PATH_CELL_HEIGHT_MASK = 0x0000ffffu,
    W8_PATH_CELL_NEIGHBOR_OR_DEPTH_MASK = 0x00ff0000u,
    W8_PATH_CELL_HAS_DIRECTIONS = 0x01000000u,
    W8_PATH_CELL_BLOCKING_FRAME = 0x02000000u,
    W8_PATH_CELL_CONDITIONAL = 0x04000000u,
    W8_PATH_CELL_DOOR = 0x08000000u,
    W8_PATH_CELL_INACTIVE = 0x10000000u,
    W8_PATH_CELL_BLOCKED = 0x20000000u
};

/* W8PathSurface::flags, including transient A* heap state. */
enum {
    W8_WAYPOINT_IN_HEAP = 0x0010u,
    W8_WAYPOINT_DISABLED = 0x0020u,
    W8_WAYPOINT_CONDITIONAL = 0x0040u
};

/* W8PathSearchNode::flags; these are separate from waypoint and edge flags. */
enum {
    W8_PATH_SEARCH_ROUTE = 0x0002u,
    W8_PATH_SEARCH_EXPANDED = 0x0004u,
    W8_PATH_SEARCH_BLOCKED = 0x0100u,
    W8_PATH_SEARCH_DISPLACED = 0x0200u,
    W8_PATH_SEARCH_INACTIVE_CELL = 0x0800u,
    W8_PATH_SEARCH_CONDITIONAL_CELL = 0x1000u,
    W8_PATH_SEARCH_PROBE_OVERLAP = 0x2000u
};
