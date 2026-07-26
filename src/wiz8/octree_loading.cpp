#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"

/* Engine Code\Octree.cpp. The member names, and the UINT16/UINT32 split between
   the loaded counters and the totals, come from the canonical assertion
   expressions at lines 1157 and 1181; their messages name the class "Octree".
   Offsets come from the asserting bodies. Everything else stays opaque. */
struct W8Octree {
    unsigned char unknown_000[0xc4];
    unsigned char m_fAccumulating;          /* 0x0c4: gates both accumulators */
    unsigned char unknown_0c5[0x33];
    unsigned int m_ulNumParticles;          /* 0x0f8 */
    unsigned char unknown_0fc[0x18];
    void** m_papProps;                      /* 0x114 */
    void** m_papParticles;                  /* 0x118 */
    unsigned short m_usNumPropsLoaded;      /* 0x11c */
    unsigned short m_usNumParticlesLoaded;  /* 0x11e */
    unsigned char unknown_120[0x68];
    unsigned int m_ulNumProps;              /* 0x188 */

    void AddLoadedProp(void* prop);
    void AddLoadedParticle(void* particle);
};

// FUNCTION: WIZ8 0x0042E440
void W8Octree::AddLoadedProp(void* prop)
{
    if (m_fAccumulating) {
        if (m_usNumPropsLoaded >= (unsigned short)m_ulNumProps) {
            srAssertFail(
                "m_usNumPropsLoaded<(UINT16)m_ulNumProps",
                "C:\\Projects\\Wizardry 8\\Engine Code\\Octree.cpp",
                0x485,
                "Too many props loaded for Octree");
        }
        m_papProps[m_usNumPropsLoaded] = prop;
        m_usNumPropsLoaded++;
        m_papProps[m_usNumPropsLoaded] = 0;
    }
}

// FUNCTION: WIZ8 0x0042E4C0
void W8Octree::AddLoadedParticle(void* particle)
{
    if (m_fAccumulating) {
        if (m_usNumParticlesLoaded >= (unsigned short)m_ulNumParticles) {
            srAssertFail(
                "m_usNumParticlesLoaded<(UINT16)m_ulNumParticles",
                "C:\\Projects\\Wizardry 8\\Engine Code\\Octree.cpp",
                0x49d,
                "Too many particles loaded for Octree");
        }
        m_papParticles[m_usNumParticlesLoaded] = particle;
        m_usNumParticlesLoaded++;
        m_papParticles[m_usNumParticlesLoaded] = 0;
    }
}
