#ifndef WIZ8_ENGINE_CODE_GDPROP_H
#define WIZ8_ENGINE_CODE_GDPROP_H

#include "wiz8/3d_code/PList.h"

/* Engine Code\GDProp.cpp. Prop.cpp allocates 0x58 bytes for this object,
   constructs it at 0x004B6E00, and owns it at Prop+0x38. Assertions in the
   same-object method at 0x004B6F30 retain the original m_pGDSurfaces and
   m_pVertices member names and establish their offsets. */
class GDProp {
public:
    ~GDProp();                            /* 0x004B6ED0 */

private:
    unsigned char unknown_000[0xc];
    void* m_allocation_0c;               /* 0x0c; released by CRT free */
    void* m_allocation_10;               /* 0x10; released by CRT free */
    int m_surface_count_14;               /* 0x14; m_pGDSurfaces count */
    int m_vertex_count_18;                /* 0x18; m_pVertices count */
    void* m_pGDSurfaces;                 /* 0x1c; 0x4c-byte elements */
    void* m_pVertices;                   /* 0x20; 0x0c-byte elements */
    unsigned char unknown_024[0x30];
    W8PList* m_list_54;                  /* 0x54 */
};                                       /* 0x58 */

#endif
