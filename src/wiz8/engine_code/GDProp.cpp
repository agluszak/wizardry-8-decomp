/* Engine Code\GDProp.cpp */

#include "wiz8/engine_code/GDProp.h"

#include "surrender/srHeap.h"

#include <stdlib.h>

// FUNCTION: WIZ8 0x004B6ED0
GDProp::~GDProp()
{
    if (m_pGDSurfaces != 0) {
        free(m_pGDSurfaces);
    }
    if (m_pVertices != 0) {
        srHeap.free(m_pVertices);
    }
    if (m_allocation_0c != 0) {
        free(m_allocation_0c);
    }
    if (m_allocation_10 != 0) {
        free(m_allocation_10);
    }
    if (m_list_54 != 0) {
        PListDestroy(m_list_54);
    }
}
