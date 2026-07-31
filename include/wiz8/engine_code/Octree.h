#pragma once

#include "surrender/srMath.h"
#include "wiz8/engine_code/BitArray.h"

class GDProp;
struct W8Position;
struct W8NavigatorMovement004572C0;

class W8OctreeQueue00437000 {
public:
    void Queue00437000(int kind, int id, const int* point);
};

class W8Pathing00457CF0 {
public:
    /* Only the two bytes 0x00434A30 tests are witnessed; the filler around them
       records nothing beyond their offsets. */
    unsigned char m_positional_000[0x0c];
    int value_00c;                       /* 0x0c */
    unsigned char m_positional_010[0x1b8];
    unsigned char flag_1c8;              /* 0x1c8 */

    unsigned int FindPathHandle(
        const unsigned char* path_name,
        unsigned short* path_bounds,
        float* path_range);              /* 0x00457CF0 */
    void LinkPropSurfaces(GDProp* prop);  /* 0x00460020 */
    void LinkPropVertices(GDProp* prop);  /* 0x004600B0 */
};

/* Engine Code\Octree.cpp. LoadWorld allocates exactly 0x29c bytes. This object
   is deliberately non-polymorphic: every owner calls the complete teardown at
   0x0042DE60 and then operator delete separately. */
class W8Octree {
public:
    W8Octree(const char* path, void** game_data);
    void Reset();
    void Initialize(const void* header);
    ~W8Octree();
    void AddLoadedProp(void* prop);
    void AddLoadedParticle(void* particle);
    void SetVisitedSet0042E3E0(BitArray* visited);
    int MarkVisited0042E400(int offset);
    void AddCollidablePropBounds0042EAB0(
        int index, const srVector3T<float>* bounds);
    void VisitPointCopy0042E620(
        unsigned short location_id, srVector3T<float>* position);
    void UpdateMonsterLocation0042E540(
        unsigned short location_id, const W8Position* position);
    bool HasLineOfSight00434B60(
        const W8Position* from, W8Position* to, char allow_fallback);
    short TraceLineOfSight00434F20(
        const W8Position* from, const W8Position* to, char trace_world,
        int from_location_id, int to_location_id, char visit_octree,
        int trace_mode);
    void AdjustPortalDestination00434A30(
        W8Position* destination, const W8Position* source);
    unsigned int AdvanceNavigator00434620(
        W8NavigatorMovement004572C0* movement,
        float radius, float separation);
    void QueueOctreeKind130042E810(
        int id, const srVector3T<float>* position);

    bool HasLoadError() const { return (m_flags_000 & 0x80000000) != 0; }
    unsigned long GetMeshCount() const { return m_mesh_count_074; }

public:
    unsigned long m_flags_000;
    unsigned char m_positional_004[8];
    srVector3T<float> octree_origin_00c;
    unsigned char m_positional_018[0x1c];
    /* 0x00434A30 clamps a world-space Y against this before settling it, which
       is what makes it a height ceiling rather than one more opaque dword. */
    float height_limit_034;              /* 0x34 */
    unsigned char m_positional_038[0x38];
    float octree_cell_size_070;
    unsigned long m_mesh_count_074;
    unsigned char m_positional_078[0x24];
    void* m_owned_09c;
    void* m_owned_0a0;
    unsigned long m_positional_0a4;
    unsigned long m_positional_0a8;
    unsigned long m_positional_0ac;
    void* m_owned_0b0;
    unsigned long m_positional_0b4;
    unsigned long m_positional_0b8;
    W8OctreeQueue00437000* octree_queue_0bc;
    void* m_owned_0c0;
    unsigned char m_fAccumulating;
    unsigned char m_positional_0c5[3];
    unsigned long m_positional_0c8;
    unsigned long m_positional_0cc;
    void* m_owned_0d0;
    void* m_owned_0d4;
    void* m_owned_0d8;
    /* Six original member names, from ReadOctFile's own assertion text at
       0x0042C68A, 0x0042C70C, 0x0042C7AB, 0x0042C850, 0x0042C8F5 and
       0x0042CAA4. The us prefix is the image's own, so the four lookup and
       link tables are unsigned short arrays. */
    BitArray* m_pAlphaBits;              /* 0xdc */
    unsigned short* m_pusMeshParticleLookup; /* 0xe0 */
    unsigned short* m_pusMeshParticles;   /* 0xe4 */
    unsigned short m_positional_0e8;
    unsigned short m_padding_0ea;
    unsigned short* m_pusMeshPropLookup;  /* 0xec */
    unsigned short* m_pusMeshProps;       /* 0xf0 */
    unsigned short m_positional_0f4;
    unsigned short m_padding_0f6;
    unsigned long m_ulNumParticles;
    BitArray* m_owned_0fc;
    BitArray* m_owned_100;
    BitArray* m_owned_104;
    BitArray* m_owned_108;
    BitArray* m_owned_10c;
    BitArray* m_owned_110;
    void** m_papProps;
    void** m_papParticles;
    unsigned short m_usNumPropsLoaded;
    unsigned short m_usNumParticlesLoaded;
    int current_sector;
    unsigned long m_positional_124;
    unsigned long m_positional_128;
    void* m_owned_12c;
    void* m_owned_130;
    unsigned long m_positional_134;
    unsigned long m_positional_138;
    unsigned long m_positional_13c;
    unsigned long m_positional_140;
    unsigned long m_positional_144;
    void* m_owned_148;
    unsigned char* m_pfRegsVisited;
    void* m_owned_150;
    BitArray* m_owned_154;
    unsigned long m_positional_158;
    BitArray* m_owned_15c;
    BitArray* m_owned_160;
    BitArray* m_owned_164;
    unsigned char m_positional_168;
    unsigned char m_positional_169;
    unsigned char m_positional_16a;
    unsigned char m_padding_16b;
    unsigned char m_positional_16c;
    unsigned char m_positional_16d;
    unsigned char m_padding_16e[2];
    unsigned long m_positional_170;
    void* m_sr_owned_174;
    unsigned long m_positional_178;
    unsigned long m_positional_17c;
    W8Pathing00457CF0* pathing_180;
    int mark_base_184;
    unsigned long m_ulNumProps;
    /* Named m_pPropSunBits by ReadOctFile's assertion at 0x0042CAA4. The
       earlier `visited` reading came from 0x0042E3E0's parameter, not from
       the image, and the assertion outranks it. */
    BitArray* m_pPropSunBits;            /* 0x18c */
    BitArray* m_owned_190;
    BitArray* m_owned_194;
    BitArray* m_owned_198;
    BitArray* m_owned_19c;
    BitArray* m_owned_1a0;
    BitArray* m_owned_1a4;
    unsigned long m_positional_1a8;
    unsigned long m_positional_1ac;
    unsigned long m_positional_1b0;
    unsigned long m_positional_1b4;
    unsigned long m_positional_1b8;
    unsigned long* m_aulGDObjs;
    unsigned char m_positional_1c0[0xbc];
    unsigned long m_positional_27c;
    unsigned long m_positional_280;
    unsigned long m_positional_284;
    unsigned long m_positional_288;
    unsigned long m_positional_28c;
    unsigned long m_positional_290;
    unsigned char m_positional_294;
    unsigned char m_padding_295;
    unsigned short m_positional_296;
    unsigned char m_padding_298;
    unsigned char m_positional_299;
    unsigned char m_padding_29a[2];
};

/* The shared spatial-service pointer at 0x006598A4 is the active W8Octree.
   Its callers reach fields at +0x70/+0x120/+0x180, while 0x0042E620 proves
   that the same receiver dispatches ordinary W8Octree methods. */
extern W8Octree* g_octree_6598a4;

/* Drops one world-space position onto the ground below it, reporting through
   the second argument whether anything was hit - a byte, and a float limit:
   0x00434A30 stores zero into that slot with a byte move and pushes its 500 as
   a single. ItemManager.cpp settles dropped items with it and Octree.cpp
   settles portal endpoints, so the name stays neutral. */
unsigned char SettleToGround00433820(
    float* position, unsigned char* out_hit, int mode, float limit);

static_assert(sizeof(W8Octree) == 0x29c, "W8Octree_must_be_0x29c");
