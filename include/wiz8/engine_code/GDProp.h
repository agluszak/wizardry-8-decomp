#ifndef WIZ8_ENGINE_CODE_GDPROP_H
#define WIZ8_ENGINE_CODE_GDPROP_H

#include "wiz8/3d_code/PList.h"

class srNode;
class W8Prop;

/* Engine Code\GDProp.cpp. Prop.cpp allocates 0x58 bytes for this object,
   constructs it at 0x004B6E00, and owns it at Prop+0x38. Assertions in the
   same-object method at 0x004B6F30 retain the original m_pGDSurfaces and
   m_pVertices member names and establish their offsets. */
class GDProp {
    friend class W8Prop;

public:
    GDProp(
        srNode* instance,
        const unsigned char* path_name,
        unsigned short prop_number,
        unsigned char surface_flag,
        unsigned char vertex_flag);      /* 0x004B6E00 */
    ~GDProp();                            /* 0x004B6ED0 */
    unsigned char ContainsPathCoordinate004B75F0(
        unsigned short x, unsigned short y) const;
    unsigned char HasListEntries004B7BA0();

private:
    void Initialize(
        srNode* instance,
        unsigned char attach,
        unsigned short prop_number,
        unsigned char surface_flag,
        unsigned char vertex_flag);      /* 0x004B7060 */

    unsigned short m_flags_00;           /* 0x00 */
    unsigned short m_prop_number_02;     /* 0x02 */
    unsigned int m_path_handle_04;       /* 0x04 */
    unsigned short m_surface_index_08;   /* 0x08 */
    unsigned short m_vertex_index_0a;    /* 0x0a */
    void* m_allocation_0c;               /* 0x0c; released by CRT free */
    void* m_allocation_10;               /* 0x10; released by CRT free */
    int m_surface_count_14;               /* 0x14; m_pGDSurfaces count */
    int m_vertex_count_18;                /* 0x18; m_pVertices count */
    void* m_pGDSurfaces;                 /* 0x1c; 0x4c-byte elements */
    void* m_pVertices;                   /* 0x20; 0x0c-byte elements */
    srNode* m_instance_24;               /* 0x24 */
    float m_path_range_28;               /* 0x28 */
    float m_path_sentinel_2c;            /* 0x2c */
    float m_path_range_30;               /* 0x30 */
    unsigned char unknown_034[0x18];
    unsigned short m_path_bound_4c;      /* 0x4c */
    unsigned short m_path_bound_4e;      /* 0x4e */
    unsigned short m_path_bound_50;      /* 0x50 */
    unsigned short m_path_bound_52;      /* 0x52 */
    W8PList* m_list_54;                  /* 0x54 */
};                                       /* 0x58 */

#endif
