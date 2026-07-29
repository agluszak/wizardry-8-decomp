#include <cstdlib>
#include <cstring>

#include "surrender/srHeap.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/sr_api.h"

extern unsigned long g_octree_storage_00659770;
extern unsigned long g_octree_state_00659890;
extern void Function46CDC0(void);
extern void Function46CDD0(void);
extern void Function4331F0(void* value);
extern void Function432D60(void* value);
extern void Function459400(int value);
extern void Function439140(void);
extern void Function457B10(void* value);
extern void Function434020(int value);

namespace {

void DestroyBitArray(BitArray*& bits)
{
    if (bits != 0) {
        bits->FreeIndex();
        operator delete(bits);
        bits = 0;
    }
}

struct W8OctreeIndex {
    void* buckets;
    void* entries;
    long last_entry;
    unsigned long bucket_count;
};

void DestroyIndex(W8OctreeIndex* index)
{
    if (index != 0) {
        operator delete(index->buckets);
        operator delete(index->entries);
        operator delete(index);
    }
}

W8OctreeIndex* CreateIndex()
{
    W8OctreeIndex* index = new W8OctreeIndex;
    long* entries = new long[12];
    long* buckets = new long[4];
    int position;

    for (position = 0; position < 4; ++position) {
        buckets[position] = -1;
        entries[position * 3] = position + 1;
        entries[position * 3 + 1] = -1;
        entries[position * 3 + 2] = -1;
    }
    entries[9] = -1;
    index->buckets = buckets;
    index->entries = entries;
    index->last_entry = 0;
    index->bucket_count = 4;
    return index;
}

template <class T>
T ReadHeader(const unsigned char* header, unsigned int offset)
{
    T result;
    memcpy(&result, header + offset, sizeof(result));
    return result;
}

template <class T>
void WriteMember(W8Octree* octree, unsigned int offset, T value)
{
    memcpy(reinterpret_cast<unsigned char*>(octree) + offset, &value, sizeof(value));
}

} // namespace

/* The raw 0x29c allocation is reset before ReadOctFile applies its header.
   The image spells the clears individually; memset expresses the same POD
   construction invariant while the positional fields are still unnamed. */
// FUNCTION: WIZ8 0x0042d040
void W8Octree::Reset()
{
    g_octree_storage_00659770 = 0;
    g_octree_state_00659890 = 0;
    Function46CDC0();
    memset(this, 0, sizeof(*this));
    m_positional_120 = -1;
}

/* ReadOctFile calls this after Reset. The file header is packed and several
   values are intentionally unaligned, so each read is copied rather than
   reached through an incorrectly aligned C++ record. Positional writes retain
   the proven offsets until the corresponding octree concepts are named. */
// FUNCTION: WIZ8 0x0042d2a0
void W8Octree::Initialize(const void* raw_header)
{
    const unsigned char* header = static_cast<const unsigned char*>(raw_header);
    unsigned int axis;

    if (header != 0) {
        WriteMember(this, 0x04, ReadHeader<unsigned long>(header, 0x02));
        WriteMember(this, 0x08, ReadHeader<unsigned long>(header, 0x06));
        WriteMember(this, 0x70, ReadHeader<unsigned long>(header, 0x0a));
        for (axis = 0; axis < 3; ++axis) {
            WriteMember(this, 0x27c + axis * 4, 0UL);
            WriteMember(this, 0x288 + axis * 4, 0UL);
            WriteMember(this, 0x0c + axis * 4,
                        ReadHeader<unsigned long>(header, 0x0e + axis * 4));
            WriteMember(this, 0x18 + axis * 4,
                        ReadHeader<unsigned long>(header, 0x1a + axis * 4));
            WriteMember(this, 0x24 + axis * 4,
                        ReadHeader<unsigned long>(header, 0x26 + axis * 4));
            WriteMember(this, 0x30 + axis * 4,
                        ReadHeader<unsigned long>(header, 0x32 + axis * 4));
            WriteMember(this, 0x78 + axis * 4,
                        ReadHeader<unsigned long>(header, 0x3e + axis * 4));
            WriteMember(this, 0x84 + axis * 4,
                        ReadHeader<unsigned long>(header, 0x4a + axis * 4));
            WriteMember(this, 0xa4 + axis * 4,
                        ReadHeader<unsigned long>(header, 0x56 + axis * 4));
        }

        WriteMember(this, 0x64,
                    ReadHeader<unsigned long>(reinterpret_cast<unsigned char*>(this), 0xa8) *
                        ReadHeader<unsigned long>(reinterpret_cast<unsigned char*>(this), 0xac));
        WriteMember(this, 0x68,
                    ReadHeader<unsigned long>(reinterpret_cast<unsigned char*>(this), 0xac));
        WriteMember(this, 0x44, ReadHeader<unsigned short>(header, 0x62));
        WriteMember(this, 0x58, ReadHeader<unsigned short>(header, 0x64));
        WriteMember(this, 0x46, ReadHeader<unsigned short>(header, 0x60));
        WriteMember(this, 0x52, ReadHeader<unsigned short>(header, 0x62));
        WriteMember(this, 0x74, ReadHeader<unsigned long>(header, 0x66));
        m_positional_1a8 = ReadHeader<unsigned long>(header, 0x9a);
        m_positional_1ac = ReadHeader<unsigned long>(header, 0xa2);
        m_positional_1b4 = ReadHeader<unsigned long>(header, 0x9e);
        WriteMember(this, 0xb4, ReadHeader<unsigned long>(header, 0x6a));
        WriteMember(this, 0xb8, ReadHeader<unsigned long>(header, 0x6e));
        WriteMember(this, 0x3c, ReadHeader<unsigned long>(header, 0x72));
        WriteMember(this, 0xc8, ReadHeader<unsigned long>(header, 0x76));
        WriteMember(this, 0x40, ReadHeader<unsigned long>(header, 0x7a));
        m_positional_0cc = ReadHeader<unsigned long>(header, 0x82);
        m_positional_124 = ReadHeader<unsigned long>(header, 0x86);
        m_positional_128 = ReadHeader<unsigned long>(header, 0x8a);
        m_positional_138 = ReadHeader<unsigned long>(header, 0x92);
        WriteMember(this, 0x54, ReadHeader<unsigned long>(header, 0xa6));
        m_positional_178 = ReadHeader<unsigned long>(header, 0xac);
        m_positional_17c = ReadHeader<unsigned long>(header, 0xb4);
        WriteMember(this, 0x60, ReadHeader<unsigned long>(header, 0xb9));
        m_ulNumProps = ReadHeader<unsigned long>(header, 0xbd);
        m_positional_0e8 = ReadHeader<unsigned short>(header, 0xc5);
        m_positional_0f4 = ReadHeader<unsigned short>(header, 0xc7);
        m_ulNumParticles = ReadHeader<unsigned long>(header, 0xc1);

        if (m_ulNumParticles != 0) {
            m_owned_0fc = new BitArray(m_ulNumParticles);
            m_owned_100 = new BitArray(m_ulNumParticles);
            m_owned_10c = new BitArray(m_ulNumParticles);
            m_papParticles = static_cast<void**>(malloc(m_ulNumParticles * 4 + 8));
        }
        if (m_ulNumProps != 0) {
            m_owned_104 = new BitArray(m_ulNumProps);
            m_owned_108 = new BitArray(m_ulNumProps);
            m_owned_110 = new BitArray(m_ulNumProps);
            m_papProps = static_cast<void**>(malloc(m_ulNumProps * 4 + 8));
        }

        m_owned_190 = new BitArray(ReadHeader<unsigned long>(header, 0x72));
        m_owned_154 = new BitArray(ReadHeader<unsigned short>(header, 0x64) + 1);
        m_owned_15c = new BitArray(ReadHeader<unsigned long>(header, 0x66) + 1);
        m_owned_160 = new BitArray(ReadHeader<unsigned long>(header, 0x66) + 1);
        m_owned_164 = new BitArray(ReadHeader<unsigned long>(header, 0x66) + 1);
        m_owned_194 = new BitArray(
            ReadHeader<unsigned long>(header, 0x7a) < 5000
                ? 5000
                : ReadHeader<unsigned long>(header, 0x7a));
        m_owned_198 = new BitArray(ReadHeader<unsigned long>(header, 0x66) + 1);
        m_owned_19c = new BitArray(ReadHeader<unsigned long>(header, 0x72));

        W8OctreeIndex** pair = new W8OctreeIndex*[2];
        pair[0] = CreateIndex();
        pair[1] = CreateIndex();
        m_owned_0bc = pair;
        m_owned_150 = CreateIndex();

        unsigned int visited_size = ReadHeader<unsigned short>(header, 0x64) + 1;
        m_pfRegsVisited = static_cast<unsigned char*>(malloc(visited_size));
        if (m_pfRegsVisited == 0) {
            srAssertFail("m_pfRegsVisited",
                         "C:\\Projects\\Wizardry 8\\Engine Code\\Octree.cpp",
                         0x36a, "InitOctree: Couldn't allocate m_pfRegsVisited.");
        }
        memset(m_pfRegsVisited, 0, visited_size);
        m_positional_168 = 1;
    }

    m_aulGDObjs = static_cast<unsigned long*>(malloc(40000));
    if (m_aulGDObjs == 0) {
        srAssertFail("m_aulGDObjs",
                     "C:\\Projects\\Wizardry 8\\Engine Code\\Octree.cpp",
                     0x372, "InitOctree: Couldn't allocate m_aulGDObjs.");
    }
    WriteMember(this, 0x6c, static_cast<unsigned short>(3));
    m_fAccumulating = 1;
    Function434020(0);
}

// FUNCTION: WIZ8 0x0042e440
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

// FUNCTION: WIZ8 0x0042e4c0
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

/* Complete non-deleting teardown. LoadWorld and DestroyWorld both perform the
   matching operator delete only after this method returns. */
// FUNCTION: WIZ8 0x0042de60
W8Octree::~W8Octree()
{
    W8OctreeIndex** pair;

    if (m_positional_16c != 0) {
        Function4331F0(m_owned_0c0);
    }
    if (m_positional_16d != 0) {
        Function432D60(m_owned_0c0);
    }
    if (m_owned_180 != 0) {
        Function459400(0);
    }

    free(m_aulGDObjs);
    free(m_owned_09c);
    free(m_owned_0a0);
    free(m_owned_0d0);
    free(m_owned_0d4);
    DestroyBitArray(m_owned_0dc);
    free(m_owned_0b0);
    free(m_owned_12c);
    free(m_owned_148);
    free(m_owned_130);

    if (m_owned_150 != 0) {
        W8OctreeIndex* index = static_cast<W8OctreeIndex*>(m_owned_150);
        operator delete(index->buckets);
        operator delete(index->entries);
        index->buckets = 0;
        index->entries = 0;
        index->last_entry = -1;
        index->bucket_count = 0;
        Function439140();
        DestroyIndex(index);
        m_owned_150 = 0;
    }

    free(m_pfRegsVisited);
    DestroyBitArray(m_owned_190);
    DestroyBitArray(m_owned_154);
    DestroyBitArray(m_owned_15c);
    DestroyBitArray(m_owned_160);
    DestroyBitArray(m_owned_164);

    free(m_owned_0e0);
    m_owned_0e0 = 0;
    free(m_owned_0e4);
    m_owned_0e4 = 0;
    free(m_owned_0ec);
    m_owned_0ec = 0;
    free(m_owned_0f0);
    m_owned_0f0 = 0;
    free(m_papProps);
    free(m_papParticles);

    DestroyBitArray(m_owned_0fc);
    DestroyBitArray(m_owned_100);
    DestroyBitArray(m_owned_104);
    DestroyBitArray(m_owned_108);
    DestroyBitArray(m_owned_10c);
    DestroyBitArray(m_owned_110);

    m_positional_170 = 0;
    if (m_sr_owned_174 != 0) {
        srHeap.free(m_sr_owned_174);
    }
    free(m_owned_0c0);
    DestroyBitArray(m_owned_194);
    DestroyBitArray(m_owned_198);
    DestroyBitArray(m_owned_19c);
    DestroyBitArray(m_owned_1a0);
    DestroyBitArray(m_owned_1a4);

    pair = static_cast<W8OctreeIndex**>(m_owned_0bc);
    if (pair != 0) {
        DestroyIndex(pair[0]);
        DestroyIndex(pair[1]);
        operator delete(pair);
    }

    free(m_owned_0d8);
    if (m_owned_180 != 0) {
        Function457B10(m_owned_180);
        operator delete(m_owned_180);
        m_owned_180 = 0;
    }
    DestroyBitArray(m_owned_18c);
    Function46CDD0();
}

/* The visit marking a traversal uses to avoid walking the same node twice, and
   three thin wrappers over the shared traversal entry points. */

#include "wiz8/engine_code/BitArray.h"

/* The octree walker. Only the two fields the visit marking reaches are
   established: the running mark and the bit array it is checked against. */
class W8OctreeWalker {
public:
    unsigned char unknown_000[0x184];
    /* 0x184: the base this walk's marks are counted from. A negative offset
       resets it from the shared counter instead of advancing it. */
    int mark_base;
    unsigned char unknown_188[4];
    /* 0x18c: the visited set. With no set attached nothing is ever reported
       as already seen. */
    BitArray* visited;
};

/* The node whose 0x1c is non-null is the only kind worth attaching. */
extern int g_shared_mark_006598ac;

extern void OctreeTraverse(
    void* walker, void* arg_2, void* arg_3, int kind, unsigned int limit);   /* 0x0042F280 */
extern void OctreeVisitPoint(void* walker, const float* point);              /* 0x0042E540 */
extern void OctreeQueue(int kind, int id, const int* point);                 /* 0x00437000 */
extern float OctreeNextCoordinate(void);

/* Attach a visited set to the walker, but only one that has been built. */
// FUNCTION: WIZ8 0x0042e3e0
void W8OctreeWalker_SetVisitedSet(W8OctreeWalker* walker, BitArray* visited)
{
    if (visited->puiIndex != 0) {
        walker->visited = visited;
    }
}

/* Advance or reset the walk's mark. A negative offset takes the shared
   counter's value as this walk's base; anything else advances the shared
   counter and reports whether that mark has already been visited - which with
   no set attached is always no. */
// FUNCTION: WIZ8 0x0042e400
int W8OctreeWalker_MarkVisited(W8OctreeWalker* walker, int offset)
{
    if (offset < 0) {
        walker->mark_base = g_shared_mark_006598ac;
        return 0;
    }
    g_shared_mark_006598ac = walker->mark_base + offset;
    if (walker->visited != 0 && walker->visited->Test(g_shared_mark_006598ac)) {
        return 1;
    }
    return 0;
}

/* Visit a point handed over by address, copied to the stack first so the
   caller's copy is not the one the traversal holds. */
// FUNCTION: WIZ8 0x0042e620
void W8OctreeWalker_VisitPointCopy(void* walker, const float* point)
{
    float copy[3];

    copy[0] = point[0];
    copy[1] = point[1];
    copy[2] = point[2];
    OctreeVisitPoint(walker, copy);
}

/* Start a traversal of the twelfth kind. A limit of zero means no limit, which
   is what the -1 stands for. */
// FUNCTION: WIZ8 0x0042ef00
void OctreeTraverseKind12(void* walker, void* arg_2, void* arg_3, unsigned short limit)
{
    unsigned int bound = (unsigned int)-1;

    if (limit != 0) {
        bound = limit;
    }
    OctreeTraverse(walker, arg_2, arg_3, 0xc, bound);
}

/* Queue one node of the thirteenth kind, with its three coordinates converted
   from floating point - which is what puts three ftol calls in a row here. */
// FUNCTION: WIZ8 0x0042e810
void OctreeQueueKind13(int id)
{
    int point[3];
    int index;

    for (index = 0; index < 3; ++index) {
        point[index] = (int)OctreeNextCoordinate();
    }
    OctreeQueue(0xd, id + 1, point);
}

/* Engine Code\Trigger.cpp lives in the same interval. Its list of live
   triggers is torn down wholesale rather than one at a time. */

/* 0x006598FC and 0x00659904: the live triggers, count and data. */
extern int g_trigger_count;
extern void** g_trigger_data;
extern int g_trigger_flag_00659994;
extern int g_trigger_flag_006598e4;

/* Release every live trigger and forget the three pieces of state that go with
   the list, which is what groups them. */
// FUNCTION: WIZ8 0x004445b0
void ReleaseAllTriggers(void)
{
    int index;

    for (index = 0; index < g_trigger_count; ++index) {
        operator delete(g_trigger_data[index]);
    }
    g_trigger_flag_00659994 = 0;
    g_trigger_count = 0;
    g_trigger_flag_006598e4 = 0;
}
