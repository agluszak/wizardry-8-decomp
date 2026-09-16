#ifndef WIZ8_ENGINE_CODE_GDPROP_H
#define WIZ8_ENGINE_CODE_GDPROP_H

#include "wiz8/3d_code/PList.h"
#include "wiz8/geometry.h"

#include <stddef.h>

class srModelInstance;
class W8Prop;
class Trigger;
struct W8WorldItem;
class W8Octree;
class OctPreTree;
struct W8GameData;
struct W8LevelFileAnimObj;

/* Engine Code\GDProp.cpp. Prop.cpp allocates 0x58 bytes for this object,
   constructs it at 0x004B6E00, and owns it at Prop+0x38. Assertions in the
   same-object method at 0x004B6F30 retain the original m_pGDSurfaces and
   m_pVertices member names and establish their offsets. */
class GDProp {
    friend class W8Prop;
    friend class W8PathingService;
    /* W8Octree's AABB occupancy test and W8GameData's prop-surface trace read
       the geometry members directly in retail. */
    friend class W8Octree;
    /* TestPathPropBounds/CreatePathProps read m_flags_00 and call the bound
       helpers on GDPreProp elements. */
    friend class OctPreTree;
    friend struct W8GameData;

public:
    GDProp(srModelInstance* instance, const char* path_name, unsigned short prop_number,
           unsigned char surface_flag, unsigned char vertex_flag); /* 0x004B6E00 */
    ~GDProp();                                                     /* 0x004B6ED0 */
    void BindTrigger(Trigger* owner);
    unsigned char ContainsPathCoordinate004B75F0(unsigned short x, unsigned short y) const;
    unsigned char HasListEntries004B7BA0();
    /* Rebuilds m_pVertices for the given animation frame of the level prop. */
    char ApplyAnimFrame004B7C00(unsigned short frame, W8LevelFileAnimObj* anim);
    /* Computes the vertex AABB into m_bound_min_34/m_bound_max_40 and copies
       it to the out parameters. */
    void ComputeBounds004B7500(srVector3T<float>* minimum, srVector3T<float>* maximum);
    /* Box-vs-bound test used by the pre-tree path obstruction pass. */
    char BoundsOverlap004B7620(const srVector3T<float>* minimum, const srVector3T<float>* maximum);

private:
    void Initialize(srModelInstance* instance, unsigned char attach, unsigned short prop_number,
                    unsigned char surface_flag, unsigned char vertex_flag); /* 0x004B7060 */
    void PrepareGeometry004B6F30(srModelInstance* instance);

    unsigned short m_flags_00;          /* 0x00 */
    unsigned short m_prop_number_02;    /* 0x02 */
    unsigned int m_path_handle_04;      /* 0x04 */
    unsigned short m_link_count_08;     /* 0x08 */
    unsigned short m_waypoint_count_0a; /* 0x0a */
    unsigned short* m_links_0c;         /* 0x0c; released by CRT free */
    unsigned short* m_waypoints_10;     /* 0x10; released by CRT free */
    int m_surface_count_14;             /* 0x14; m_pGDSurfaces count */
    int m_vertex_count_18;              /* 0x18; m_pVertices count */
    W8GDSurface* m_pGDSurfaces;         /* 0x1c */
    srVector3T<float>* m_pVertices;     /* 0x20 */
    Trigger* m_owner_24;                /* 0x24: installed by 0x004B7470 */
    float m_path_range_28;              /* 0x28 */
    float m_path_sentinel_2c;           /* 0x2c */
    float m_path_range_30;              /* 0x30 */
    /* Vertex AABB cached by ComputeBounds004B7500 and tested by
       BoundsOverlap004B7620. */
    srVector3T<float> m_bound_min_34; /* 0x34 */
    srVector3T<float> m_bound_max_40; /* 0x40 */
    unsigned short m_path_bound_4c;   /* 0x4c */
    unsigned short m_path_bound_4e;   /* 0x4e */
    unsigned short m_path_bound_50;   /* 0x50 */
    unsigned short m_path_bound_52;   /* 0x52 */
    W8PList* m_list_54;               /* 0x54 */
}; /* 0x58 */

/* OctPreTree.cpp's per-prop path record element: the GDProp plus the frame
   index 0x0046C0F0 writes at +0x58 and W8PathingService::LinkCollideableProps
   reads back. */
class GDPreProp : public GDProp {
public:
    /* Zero-initializing default ctor at 0x004B7BC0; required for
       `new GDPreProp[n]` in CreatePathProps. */
    GDPreProp();
    unsigned short last_frame_58; /* 0x58 */
}; /* 0x5c */

static_assert(sizeof(GDPreProp) == 0x5c, "GDPreProp_must_be_0x5c");
static_assert(offsetof(GDPreProp, last_frame_58) == 0x58, "GDPreProp_last_frame_58");

void AddItemToSector(int sector, W8WorldItem* item); /* 0x004B7AD0 */
void RegisterPathSurface004B7730(unsigned int index, const int* point);
void RegisterPathVertex004B7830(unsigned int index, const int* point, const int* second);
void RemoveItemFromSector(int sector, W8WorldItem* item); /* 0x004B7B50 */

#endif
