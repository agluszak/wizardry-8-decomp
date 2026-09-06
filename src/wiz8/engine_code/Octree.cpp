#include <cstdlib>
#include <cstring>

#include "surrender/srHeap.h"
#include "wiz8/engine_code/OctPath.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/float_constants.h"
#include "wiz8/geometry.h"
#include "wiz8/engine_code/Navigator.h"
#include "wiz8/engine_code/Object0043A910.h"
#include "wiz8/sr_api.h"
#include "wiz8/virtual_file.h"
#include "FileMan.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/engine_code/World.h"

#include <math.h>

#define OCTREE_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\Octree.cpp"

// FUNCTION: WIZ8 0x00433a70
void W8Octree::GetPathSurfaceNormal00433A70(
    const srVector3T<float>* position, srVector3T<float>* normal)
{
    if (pathing_180 != 0) {
        pathing_180->GetPathSurfaceNormal0045B730(position, normal);
        return;
    }
    normal->x = 0.0f;
    normal->y = 1.0f;
    normal->z = 0.0f;
}


extern unsigned long g_octree_storage_00659770;
extern unsigned long g_octree_state_00659890;
extern void Function4331F0(void* value);
extern void Function432D60(void* value);
extern void Function439140(void);
extern void Function434020(int value);
extern unsigned char g_navigator_link_mode_00659c10;
extern float g_rate_006068EC;
extern float g_world_scale_005ebc40;

// GLOBAL: WIZ8 0x005ec02c
static const float NAVIGATOR_MINIMUM_HORIZONTAL_DISTANCE = 50.0f;

// FUNCTION: WIZ8 0x00434250
unsigned char W8Octree::PrepareNavigatorTarget00434250(
    W8NavigatorMovementState* movement, float radius, float separation)
{
    unsigned char result = 0;
    unsigned char hit = 0;
    if (movement->target_position_04c.y > spatial_000.clipped_maximum_30.y) {
        movement->target_position_04c.y = spatial_000.clipped_maximum_30.y;
    }
    srVector3T<float> probe = movement->target_position_04c;
    SettleToGround00433820(&probe, &hit, 1, 500.0f);
    if (hit != 0) {
        movement->target_position_04c.y = probe.y;
    }
    if (pathing_180 == 0) {
        return 1;
    }
    srVector3T<float> delta;
    delta.x = movement->target_position_04c.x - movement->position_040.x;
    delta.y = 0.0f;
    delta.z = movement->target_position_04c.z - movement->position_040.z;
    if (sqrt(delta.x * delta.x + delta.z * delta.z) < NAVIGATOR_MINIMUM_HORIZONTAL_DISTANCE) {
        return 0;
    }
    if ((movement->attachment_0ac->flags_00 & 0x10000) == 0) {
        srVector3T<float> target = movement->target_position_04c;
        if (pathing_180->FindPathCell00459D60(&target, 0, 1) != 0) {
            if (pathing_180->TestWaypointSpan0045A1B0(&movement->position_040, &target, 0, 0) == 0) {
                movement->attachment_0ac->InitializeSegment004563E0(&movement->position_040, &target);
                movement->attachment_0ac->separation_54 = separation;
                result = pathing_180->BuildAttachmentPath00460950(
                    movement->attachment_0ac, movement->unknown_000);
                if (result != 0) {
                    W8NavigatorAttachment* attachment = movement->attachment_0ac;
                    attachment->position_4c[attachment->path_position_index_08] =
                        movement->target_position_04c;
                    attachment->position_1c = attachment->position_4c[attachment->path_position_index_08];
                    pathing_180->AdvanceAttachmentWaypoint00462DE0(&movement->position_040, attachment);
                    movement->attachment_0ac->GetNextPosition00456660(&movement->target_position_04c);
                    return result;
                }
                result = pathing_180->ProbeAttachmentPath00462360(movement->attachment_0ac);
                if (result != 0) {
                    W8NavigatorAttachment* attachment = movement->attachment_0ac;
                    attachment->position_4c[attachment->path_position_index_08] =
                        movement->target_position_04c;
                    attachment->position_1c = attachment->position_4c[attachment->path_position_index_08];
                    return result;
                }
            } else {
                movement->attachment_0ac->InitializeSegment004563E0(
                    &movement->position_040, &movement->target_position_04c);
                result = 1;
            }
        }
        return result;
    }
    delta.x = movement->target_position_04c.x - movement->position_040.x;
    delta.y = movement->target_position_04c.y - movement->position_040.y;
    delta.z = movement->target_position_04c.z - movement->position_040.z;
    float squared_length = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
    float gap = (float)sqrt(squared_length) - separation;
    if (gap < g_float_005ebb34) {
        movement->attachment_0ac->InitializeSegment004563E0(
            &movement->position_040, &movement->position_040);
        return 1;
    }
    if (gap < g_world_scale_005ebc40) {
        if (squared_length != g_zero_005ebb40) {
            float scale = (float)(gap * g_float_005ec028 / sqrt(squared_length));
            delta.x *= scale;
            delta.y *= scale;
            delta.z *= scale;
        }
        delta.x += movement->position_040.x;
        delta.y += movement->position_040.y;
        delta.z += movement->position_040.z;
        movement->attachment_0ac->InitializeSegment004563E0(&movement->position_040, &delta);
        return 1;
    }
    movement->attachment_0ac->InitializeSegment004563E0(
        &movement->position_040, &movement->target_position_04c);
    movement->attachment_0ac->separation_54 = separation;
    return pathing_180->PlanMovement00463460(movement, radius, separation) != 0;
}

// FUNCTION: WIZ8 0x004347d0
unsigned char __stdcall IsNavigatorAtTarget004347D0(W8NavigatorMovementState* movement)
{
    srVector3T<float> target;
    if (movement->attachment_0ac != 0) {
        W8NavigatorAttachment* attachment = movement->attachment_0ac;
        if (attachment->value_04 < attachment->path_position_index_08 ||
            (attachment->flags_00 & 0x80000) != 0) {
            return 0;
        }
        attachment->GetNextPosition00456660(&target);
    } else {
        target = movement->target_position_04c;
    }
    float dx = target.x - movement->position_040.x;
    float dy = target.y - movement->position_040.y;
    float dz = target.z - movement->position_040.z;
    if (movement->movement_scale_060 * g_world_scale_005ebc40 < sqrt(dx * dx + dy * dy + dz * dz)) {
        return 0;
    }
    return 1;
}

// FUNCTION: WIZ8 0x00434880
unsigned char W8Octree::PrepareNavigatorPatrol00434880(
    W8NavigatorMovementState* movement, float minimum, float maximum)
{
    unsigned char result = 0;
    if (pathing_180 != 0) {
        srVector3T<float> velocity = movement->velocity_034;
        movement->attachment_0ac->InitializeSegment004563E0(
            &movement->position_040, &movement->target_position_04c);
        result = pathing_180->BuildPatrolPath00461960(
            movement->attachment_0ac, movement->unknown_000,
            &movement->target_position_04c, minimum, &velocity, maximum);
        if (result == 0) {
            return 0;
        }
        double step = movement->movement_scale_060 * g_world_scale_005ebc40;
        movement->attachment_0ac->GetNextPosition00456660(&movement->target_position_04c);
        srVector3T<float> delta;
        delta.x = movement->target_position_04c.x - movement->position_040.x;
        delta.y = movement->target_position_04c.y - movement->position_040.y;
        delta.z = movement->target_position_04c.z - movement->position_040.z;
        float squared_length = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
        if (step < sqrt(squared_length) && squared_length != g_zero_005ebb40) {
            float scale = (float)(step / sqrt(squared_length));
            delta.x *= scale;
            delta.y *= scale;
            delta.z *= scale;
        }
        movement->target_position_04c.x = delta.x + movement->position_040.x;
        movement->target_position_04c.y = delta.y + movement->position_040.y;
        movement->target_position_04c.z = delta.z + movement->position_040.z;
    }
    return result;
}

// FUNCTION: WIZ8 0x00434a00
unsigned char W8Octree::LinkNavigatorTarget00434A00(
    W8NavigatorMovementState* movement, const srVector3T<float>* target, float separation)
{
    if (pathing_180 != 0) {
        return pathing_180->LinkAttachmentTarget004612A0(
            movement->attachment_0ac, movement->unknown_000, target, separation);
    }
    return 0;
}

/* ReadOctFile's own direct callees. Their bodies are not recovered, so they
   keep address-qualified names. */
extern int CheckLevelAssetSet0042CCC0(const char* level_path);
extern char BuildPreprocessedFiles00492E60(const char* level_path);
extern void ReportStartupMessage004969D0(const char* message);
extern unsigned char BitArrayLoad0043AEC0(BitArray* bits, int handle);
extern unsigned char ReadLevelName00432E90(const char* name);
extern void ApplyLevelName00432B80(const char* name);
extern void ReadWaypointFile0043A0F0(void);
/* The cell-walk probes and the trace helpers the two line-of-sight bodies use.
   None of their bodies are recovered, so they keep address-qualified names. */
extern void SeedCellProbe00457640(const srVector3T<float>* from, const srVector3T<float>* to);
extern int ProbeCellForBlockers00435C40(const int* cell);
extern unsigned char TestProbeResult00435F00(void* result);
extern int ProbeCellForTrace00435B00(const int* cell);
extern char TestTraceResult0041C330(
    int value_1b8, int value_1bc, void* result, unsigned char value_134, int mode);
extern int TraceAgainstProps00436510(
    const srVector3T<float>* from, srVector3T<float>* to, int value_3, int value_4);
extern char ResolveTraceHit004353F0(
    void* result, srVector3T<float>* hit, int mode, int* out, int value_5, int value_6,
    int value_7);
extern int GetSectorForPosition00430BF0(const srVector3T<float>* position);
extern float g_octree_cell_scale_005ebcd0;
extern unsigned short g_path_reserve_0060827a;
extern float g_path_span_scale_005ec344;
extern float g_path_limit_006081e8;
extern void* CreatePathState004CAE40(void);
/* 0x00659888 accumulates every byte the loader reads, and 0x00652DB0 caches the
   game-data block LoadWorld hands back through its out parameter. */
extern unsigned long g_octree_bytes_read_00659888;

namespace {

void DestroyBitArray(BitArray*& bits)
{
    if (bits != 0) {
        delete bits;
        bits = 0;
    }
}

void DestroyIndex(W8OctreeIndex* index)
{
    if (index != 0) {
        delete[] static_cast<int*>(index->bucket_heads);
        delete[] static_cast<W8OctreeEntry*>(index->entries);
        delete index;
    }
}

W8OctreeIndex* CreateIndex()
{
    W8OctreeIndex* index = new W8OctreeIndex;
    long* entries = new long[12];
    long* bucket_heads = new long[4];
    int position;

    for (position = 0; position < 4; ++position) {
        bucket_heads[position] = -1;
        entries[position * 3] = position + 1;
        entries[position * 3 + 1] = -1;
        entries[position * 3 + 2] = -1;
    }
    entries[9] = -1;
    index->bucket_heads = bucket_heads;
    index->entries = entries;
    index->free_head = 0;
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

/* Follow one child bit per axis and level through the compact 9-word branch
   records.  Zero is the missing-child sentinel; live leaves start at one. */
// FUNCTION: WIZ8 0x00433660
unsigned long W8Octree::FindLeaf00433660(const int* point)
{
    unsigned long level = spatial_000.depth_44;
    unsigned long mask = 1 << spatial_000.depth_44;
    unsigned long node = 1;

    do {
        if ((long)level < 1) {
            break;
        }
        mask /= 2;
        int child = 0;
        if ((point[0] & mask) != 0) {
            child = 4;
        }
        if ((point[1] & mask) != 0) {
            child += 2;
        }
        if ((point[2] & mask) != 0) {
            child += 1;
        }
        node = m_owned_09c[node].children_04[child];
        --level;
    } while (node != 0);
    if (m_positional_0b8 < node) {
        return 0;
    }
    return node;
}

/* Whether one point can see another, and where the line stops if it cannot.

   Both of these walk the same cell line. A line inside one or two cells probes
   those directly; anything longer builds a walk and steps it, advancing the
   driving axis every iteration and each minor axis whenever its accumulator
   goes negative - and probing after every one of those advances, so a blocker
   in a diagonally-crossed cell is not stepped over. The walk stops at the first
   blocker.

   HasLineOfSight answers the question and lets the caller fall back to a prop
   trace; TraceLineOfSight additionally reports where the line was stopped and
   distinguishes a world hit from a prop hit by the sign of its answer. */
// FUNCTION: WIZ8 0x00434b60
bool W8Octree::HasLineOfSight(
    const srVector3T<float>* from, srVector3T<float>* to, char allow_fallback)
{
    W8OctreeWalk walk;
    int cell[5];
    int step[4];
    unsigned char result[12];
    srVector3T<float> hit;
    unsigned char blocked = 0;
    int span;
    int error_0;
    int error_1;
    int index;

    SeedCellProbe00457640(from, to);
    m_positional_1b8 = 0;
    m_owned_190->ClearAll();
    m_owned_160->ClearAll();
    cell[0] = (int)((from->x - spatial_000.minimum_0c.x) / spatial_000.node_extent_70);
    step[3] = (int)((to->x - spatial_000.minimum_0c.x) / spatial_000.node_extent_70);
    cell[1] = (int)((from->y - spatial_000.minimum_0c.y) / spatial_000.node_extent_70);
    step[2] = (int)((to->y - spatial_000.minimum_0c.y) / spatial_000.node_extent_70);
    cell[2] = (int)((from->z - spatial_000.minimum_0c.z) / spatial_000.node_extent_70);
    step[1] = (int)((to->z - spatial_000.minimum_0c.z) / spatial_000.node_extent_70);
    span = abs(cell[2] - step[1]) + abs(cell[1] - step[2]) + abs(cell[0] - step[3]);
    if (span < 2) {
        ProbeCellForBlockers00435C40(cell);
        blocked = TestProbeResult00435F00(result);
        if (blocked == 0 && span != 0) {
            ProbeCellForBlockers00435C40(&step[1]);
            blocked = TestProbeResult00435F00(result);
        }
    } else {
        BuildCellWalk(from, to, &walk);
        cell[0] = walk.cell_00[0];
        cell[1] = walk.cell_00[1];
        cell[2] = walk.cell_00[2];
        cell[4] = walk.minor_axis_20;
        step[0] = walk.step_0c[0];
        step[1] = walk.step_0c[1];
        step[2] = walk.step_0c[2];
        step[3] = walk.count_24;
        cell[3] = 0;
        error_1 = walk.error_38;
        error_0 = walk.error_2c;
        if (walk.count_24 > 0) {
            do {
                if (blocked != 0) {
                    break;
                }
                if (ProbeCellForBlockers00435C40(cell) != 0) {
                    blocked = TestProbeResult00435F00(result);
                }
                if (error_0 < error_1) {
                    if (error_0 < 0 && blocked == 0) {
                        error_0 += walk.error_reset_30;
                        cell[walk.minor_axis_1c] += step[walk.minor_axis_1c];
                        if (ProbeCellForBlockers00435C40(cell) != 0) {
                            blocked = TestProbeResult00435F00(result);
                        }
                        if (error_1 < 0 && blocked == 0) {
                            cell[cell[4]] += step[cell[4]];
                            error_1 += walk.error_reset_3c;
                            if (ProbeCellForBlockers00435C40(cell) != 0) {
                                blocked = TestProbeResult00435F00(result);
                            }
                        }
                    }
                } else if (error_1 < 0 && blocked == 0) {
                    cell[cell[4]] += step[cell[4]];
                    error_1 += walk.error_reset_3c;
                    if (ProbeCellForBlockers00435C40(cell) != 0) {
                        blocked = TestProbeResult00435F00(result);
                    }
                    if (error_0 < 0 && blocked == 0) {
                        error_0 += walk.error_reset_30;
                        cell[walk.minor_axis_1c] += step[walk.minor_axis_1c];
                        if (ProbeCellForBlockers00435C40(cell) != 0) {
                            blocked = TestProbeResult00435F00(result);
                        }
                    }
                }
                cell[walk.major_axis_18] += step[walk.major_axis_18];
                error_1 -= walk.error_delta_34;
                error_0 -= walk.error_delta_28;
                ++cell[3];
            } while (cell[3] < step[3]);
        }
        if (blocked == 0) {
            if (allow_fallback != 0 &&
                TraceAgainstProps00436510(from, to, 1, 1) != 0) {
                blocked = 1;
            }
            return blocked == 0;
        }
    }
    if (blocked != 0) {
        to->x = hit.x;
        to->y = hit.y;
        to->z = hit.z;
    }
    return blocked == 0;
}

// FUNCTION: WIZ8 0x00434f20
short W8Octree::TraceLineOfSight(
    const srVector3T<float>* from, const srVector3T<float>* to, char trace_world,
    int from_location_id, int to_location_id, char visit_octree, int trace_mode)
{
    W8OctreeWalk walk;
    int cell[5];
    int step[4];
    unsigned char result[12];
    srVector3T<float> hit;
    char blocked = 0;
    char previous = 0;
    int span;
    int error_0;
    int error_1;
    int minor_0;
    int minor_1;
    int index;
    int scratch;

    SeedCellProbe00457640(from, to);
    cell[3] = 0;
    if (visit_octree != 0) {
        m_positional_1b8 = 0;
        m_owned_194->ClearAll();
        cell[0] = (int)((from->x - spatial_000.minimum_0c.x) / spatial_000.node_extent_70);
        step[3] = (int)((to->x - spatial_000.minimum_0c.x) / spatial_000.node_extent_70);
        cell[1] = (int)((from->y - spatial_000.minimum_0c.y) / spatial_000.node_extent_70);
        step[1] = (int)((to->y - spatial_000.minimum_0c.y) / spatial_000.node_extent_70);
        cell[2] = (int)((from->z - spatial_000.minimum_0c.z) / spatial_000.node_extent_70);
        step[0] = (int)((to->z - spatial_000.minimum_0c.z) / spatial_000.node_extent_70);
        span = abs(cell[2] - step[0]) + abs(cell[1] - step[1]) + abs(cell[0] - step[3]);
        if (span < 2) {
            ProbeCellForTrace00435B00(cell);
            blocked = TestTraceResult0041C330(
                m_positional_1b8, m_positional_1bc, result, m_positional_134, 0);
            if (blocked == 0 && span != 0) {
                ProbeCellForTrace00435B00(&step[3]);
                blocked = TestTraceResult0041C330(
                    m_positional_1b8, m_positional_1bc, result, m_positional_134, 0);
            }
        } else {
            BuildCellWalk(from, to, &walk);
            cell[1] = walk.cell_00[1];
            cell[0] = walk.cell_00[0];
            cell[2] = walk.cell_00[2];
            minor_0 = walk.minor_axis_1c;
            scratch = walk.major_axis_18;
            minor_1 = walk.minor_axis_20;
            step[0] = walk.step_0c[0];
            cell[4] = walk.count_24;
            step[1] = walk.step_0c[1];
            step[2] = walk.step_0c[2];
            index = 0;
            error_1 = walk.error_38;
            error_0 = walk.error_2c;
            previous = 0;
            if (walk.count_24 > 0) {
                do {
                    blocked = previous;
                    if (blocked != 0) {
                        break;
                    }
                    if (ProbeCellForTrace00435B00(cell) != 0) {
                        blocked = TestTraceResult0041C330(
                            m_positional_1b8, m_positional_1bc, result, m_positional_134, 0);
                    }
                    if (error_0 < error_1) {
                        if (error_0 < 0 && blocked == 0) {
                            cell[minor_0] += step[minor_0];
                            error_0 += walk.error_reset_30;
                            if (ProbeCellForTrace00435B00(cell) != 0) {
                                blocked = TestTraceResult0041C330(
                                    m_positional_1b8, m_positional_1bc, result,
                                    m_positional_134, 0);
                            }
                            if (error_1 < 0 && blocked == 0) {
                                cell[minor_1] += step[minor_1];
                                error_1 += walk.error_reset_3c;
                                if (ProbeCellForTrace00435B00(cell) != 0) {
                                    blocked = TestTraceResult0041C330(
                                        m_positional_1b8, m_positional_1bc, result,
                                        m_positional_134, 0);
                                }
                            }
                        }
                    } else if (error_1 < 0 && blocked == 0) {
                        cell[minor_1] += step[minor_1];
                        error_1 += walk.error_reset_3c;
                        if (ProbeCellForTrace00435B00(cell) != 0) {
                            blocked = TestTraceResult0041C330(
                                m_positional_1b8, m_positional_1bc, result,
                                m_positional_134, 0);
                        }
                        if (error_0 < 0 && blocked == 0) {
                            cell[minor_0] += step[minor_0];
                            error_0 += walk.error_reset_30;
                            if (ProbeCellForTrace00435B00(cell) != 0) {
                                blocked = TestTraceResult0041C330(
                                    m_positional_1b8, m_positional_1bc, result,
                                    m_positional_134, 0);
                            }
                        }
                    }
                    cell[scratch] += step[scratch];
                    error_1 -= walk.error_delta_34;
                    error_0 -= walk.error_delta_28;
                    ++index;
                    previous = blocked;
                } while (index < cell[4]);
            }
        }
        if (trace_world != 0 &&
            TraceAgainstProps00436510(from, &hit, 0, 0) != 0) {
            blocked = 1;
        } else if (blocked == 0) {
            goto resolve;
        }
        cell[3] = 1;
        if (blocked != 0) {
            to = &hit;
            return 1;
        }
    }
resolve:
    if (from_location_id >= -2) {
        cell[4] = to_location_id;
        if (ResolveTraceHit004353F0(
                result, &hit, from_location_id, &cell[4], to_location_id, 0,
                trace_mode) != 0) {
            return -1;
        }
    }
    return (short)cell[3];
}

/* Double the index and rehash into it.

   Every live entry is re-linked under its key's new bucket, and whatever is
   left over becomes the free list, terminated at the last slot. The old two
   allocations go back only when there was a table to begin with. */
// FUNCTION: WIZ8 0x00439290
void GrowIndex00439290(W8OctreeIndex* index)
{
    W8OctreeEntry* entries;
    int* bucket_heads;
    W8OctreeEntry* fill;
    int* fill_bucket;
    W8OctreeEntry* source;
    unsigned int capacity;
    unsigned int remaining;
    unsigned int hash;
    int used = 0;
    int bucket;
    int slot;

    capacity = index->bucket_count << 1;
    if (capacity < 4) {
        capacity = 4;
    }
    entries = new W8OctreeEntry[capacity];
    bucket_heads = new int[capacity];
    fill = entries;
    fill_bucket = bucket_heads;
    remaining = capacity;
    if ((int)capacity > 0) {
        do {
            fill->next_index = -1;
            *fill_bucket = -1;
            --remaining;
            ++fill;
            ++fill_bucket;
        } while (remaining != 0);
    }
    if (index->bucket_count != 0) {
        for (bucket = 0; bucket < (int)index->bucket_count; ++bucket) {
            slot = static_cast<int*>(index->bucket_heads)[bucket];
            if (slot != -1) {
                do {
                    source = static_cast<W8OctreeEntry*>(index->entries) + slot;
                    entries[used].key = source->key;
                    hash = ((source->key >> 10 ^ source->key) >> 10 ^ source->key) &
                        (capacity - 1);
                    entries[used].value =
                        (static_cast<W8OctreeEntry*>(index->entries) + slot)->value;
                    entries[used].next_index = bucket_heads[hash];
                    bucket_heads[hash] = used;
                    slot = (static_cast<W8OctreeEntry*>(index->entries) + slot)->next_index;
                    ++used;
                } while (slot != -1);
            }
        }
        delete[] static_cast<int*>(index->bucket_heads);
        delete[] static_cast<W8OctreeEntry*>(index->entries);
    }
    if (used < (int)capacity) {
        for (slot = used; slot < (int)capacity; ) {
            ++slot;
            entries[slot - 1].next_index = slot;
        }
    }
    entries[capacity - 1].next_index = -1;
    index->bucket_count = capacity;
    index->bucket_heads = bucket_heads;
    index->free_head = used;
    index->entries = entries;
}

/* Take the next free entry, growing first when there is none. */
// FUNCTION: WIZ8 0x004393e0
int AllocateEntry004393E0(W8OctreeIndex* index)
{
    int slot;

    if (index->free_head == -1) {
        GrowIndex00439290(index);
    }
    slot = index->free_head;
    index->free_head = (static_cast<W8OctreeEntry*>(index->entries) + slot)->next_index;
    return slot;
}

/* Unlink one key/value pair and put its entry back on the free list. A key that
   is not there, or one whose chain holds a different value, is left alone. */
// FUNCTION: WIZ8 0x00438c90
void RemoveEntry00438C90(
    W8OctreeIndex* index, const unsigned int* key, const int* value)
{
    unsigned int wanted = *key;
    int* bucket = static_cast<int*>(index->bucket_heads) +
        (((wanted >> 10 ^ wanted) >> 10 ^ wanted) & (index->bucket_count - 1));
    int slot = *bucket;
    int previous = -1;
    W8OctreeEntry* entries;
    W8OctreeEntry* entry;

    if (slot == -1) {
        return;
    }
    entries = static_cast<W8OctreeEntry*>(index->entries);
    for (;;) {
        entry = entries + slot;
        if (entry->key == wanted && entry->value == *value) {
            break;
        }
        previous = slot;
        slot = entry->next_index;
        if (entry->next_index == -1) {
            return;
        }
    }
    if (previous != -1) {
        entries[previous].next_index = entry->next_index;
    } else {
        *bucket = entry->next_index;
    }
    (static_cast<W8OctreeEntry*>(index->entries) + slot)->next_index = index->free_head;
    index->free_head = slot;
}

/* Find the first entry for a key, or continue along the same bucket chain
   after a previously returned entry. Values are deliberately not considered:
   conditional paths use this to inspect every value stored under one cell. */
// FUNCTION: WIZ8 0x00438d50
int W8OctreeIndex::FindNextEntry00438D50(
    const unsigned int* key, int previous)
{
    W8OctreeEntry* index_entries = static_cast<W8OctreeEntry*>(entries);
    int slot;
    if (previous == -1) {
        unsigned int wanted = *key;
        unsigned int hash = (wanted >> 10 ^ wanted) >> 10 ^ wanted;
        slot = static_cast<int*>(bucket_heads)[hash & (bucket_count - 1)];
    }
    else {
        slot = index_entries[previous].next_index;
    }

    while (slot != -1) {
        if (index_entries[slot].key == *key) {
            return slot;
        }
        slot = index_entries[slot].next_index;
    }
    return -1;
}

/* Link one key to one value, growing first when there is no free entry. */
// FUNCTION: WIZ8 0x0055dbb0
void InsertEntry0055DBB0(
    W8OctreeIndex* index, const unsigned int* key, const int* value)
{
    W8OctreeEntry* entries;
    unsigned int hash;
    unsigned int stored;
    int capacity;
    int slot;

    if (index->free_head == -1) {
        GrowIndex00439290(index);
    }
    slot = index->free_head;
    capacity = index->bucket_count;
    entries = static_cast<W8OctreeEntry*>(index->entries);
    index->free_head = entries[slot].next_index;
    stored = *key;
    entries[slot].key = stored;
    entries[slot].value = *value;
    hash = ((stored >> 10 ^ stored) >> 10 ^ stored) & (capacity - 1);
    entries[slot].next_index = static_cast<int*>(index->bucket_heads)[hash];
    static_cast<int*>(index->bucket_heads)[hash] = slot;
}

/* Record that one object now occupies one cell.

   The object's key is its kind in the high half and its id in the low half.
   When it is already registered somewhere, the cell it was in is compared
   against the one being queued and an unchanged pairing is left completely
   alone; otherwise the old pairing comes out of both indexes first. A kind of
   twelve additionally maintains a second pairing under its own tag.

   The two indexes are the same pair AddCollidablePropBounds keeps: one keyed by
   cell, one keyed by object. */
// FUNCTION: WIZ8 0x00437000
unsigned char W8OctreeObjectRegistry::RegisterObjectCell(
    int kind, int id, const int* point)
{
    W8OctreeIndex** pair = reinterpret_cast<W8OctreeIndex**>(this);
    W8OctreeIndex* by_object = pair[1];
    W8OctreeIndex* by_cell;
    W8OctreeEntry* entries;
    unsigned int object_key;
    unsigned int tagged_key;
    unsigned int hash;
    int cell_key;
    int occupied;
    int slot;
    int previous;
    int* bucket;

    object_key = (kind & 0xffff) * 0x10000 + (id & 0xffff);
    hash = (object_key >> 10 ^ object_key) >> 10 ^ object_key;
    slot = static_cast<int*>(by_object->bucket_heads)[hash & (by_object->bucket_count - 1)];
    while (slot != -1) {
        entries = static_cast<W8OctreeEntry*>(by_object->entries);
        if (entries[slot].key == object_key) {
            occupied = entries[slot].value;
            if (occupied == 0) {
                break;
            }
            if (point[0] == 0 &&
                (int)(((occupied - 1) & 0xff00) << 8) == point[1] &&
                ((occupied - 1) & 0xff) == point[2]) {
                return 1;
            }
            bucket = static_cast<int*>(by_object->bucket_heads) +
                (hash & (by_object->bucket_count - 1));
            slot = *bucket;
            if (slot != -1) {
                entries = static_cast<W8OctreeEntry*>(by_object->entries);
                previous = -1;
                for (;;) {
                    if (entries[slot].key == object_key) {
                        if (previous == -1) {
                            *bucket = entries[slot].next_index;
                        } else {
                            entries[previous].next_index = entries[slot].next_index;
                        }
                        static_cast<W8OctreeEntry*>(by_object->entries)[slot].next_index =
                            by_object->free_head;
                        by_object->free_head = slot;
                        break;
                    }
                    previous = slot;
                    slot = entries[slot].next_index;
                    if (slot == -1) {
                        break;
                    }
                }
            }
            by_cell = pair[0];
            bucket = static_cast<int*>(by_cell->bucket_heads) +
                ((((unsigned int)occupied >> 10 ^ occupied) >> 10 ^ occupied) &
                 (by_cell->bucket_count - 1));
            if (*bucket != -1) {
                entries = static_cast<W8OctreeEntry*>(by_cell->entries);
                slot = *bucket;
                previous = -1;
                do {
                    if (entries[slot].key == (unsigned int)occupied &&
                        entries[slot].value == (int)object_key) {
                        if (previous == -1) {
                            *bucket = entries[slot].next_index;
                        } else {
                            entries[previous].next_index = entries[slot].next_index;
                        }
                        static_cast<W8OctreeEntry*>(by_cell->entries)[slot].next_index =
                            by_cell->free_head;
                        by_cell->free_head = slot;
                        break;
                    }
                    previous = slot;
                    slot = entries[slot].next_index;
                } while (entries[previous].next_index != -1);
            }
            goto record;
        }
        slot = entries[slot].next_index;
    }
    if ((short)kind == 0xc) {
        cell_key = ((point[0] << 8) + point[1]) * 0x100 + 1 + point[2];
        tagged_key = (id & 0xffff) + 0xd0000;
        RemoveEntry00438C90(by_object, &tagged_key, &cell_key);
        by_object = pair[1];
        slot = AllocateEntry004393E0(by_object);
        entries = static_cast<W8OctreeEntry*>(by_object->entries);
        hash = ((tagged_key >> 10 ^ tagged_key) >> 10 ^ tagged_key) &
            (by_object->bucket_count - 1);
        entries[slot].key = tagged_key;
        entries[slot].value = cell_key;
        entries[slot].next_index = static_cast<int*>(by_object->bucket_heads)[hash];
        static_cast<int*>(by_object->bucket_heads)[hash] = slot;
        RemoveEntry00438C90(pair[0], (const unsigned int*)&cell_key, (const int*)&tagged_key);
        InsertEntry0055DBB0(pair[0], (const unsigned int*)&cell_key, (const int*)&tagged_key);
    }
record:
    cell_key = ((point[0] << 8) + point[1]) * 0x100 + 1 + point[2];
    RemoveEntry00438C90(pair[1], &object_key, &cell_key);
    InsertEntry0055DBB0(pair[1], &object_key, &cell_key);
    RemoveEntry00438C90(pair[0], (const unsigned int*)&cell_key, (const int*)&object_key);
    InsertEntry0055DBB0(pair[0], (const unsigned int*)&cell_key, (const int*)&object_key);
    return 1;
}

/* Register one collidable prop against every octree cell its bounding box
   touches.

   Two indexes are kept in step: one keyed by cell so a cell can name its props,
   one keyed by prop so a prop can name its cells. Each cell first drops any
   stale pairing in both directions before the new one goes in, which is what
   makes repeated calls for a moving prop safe. The cell key packs x, y and z
   into one dword a byte apart, and the prop key carries its id in the low half
   with a tag above it. */
// FUNCTION: WIZ8 0x0042eab0
void W8Octree::AddCollidablePropBounds(
    int index, const srVector3T<float>* bounds)
{
    int minimum[3];
    int maximum[3];
    W8OctreeIndex** pair;
    W8OctreeIndex* by_prop;
    W8OctreeIndex* by_cell;
    W8OctreeEntry* entries;
    unsigned int prop_key;
    unsigned int cell_key;
    unsigned int hash;
    int slot;
    int axis;
    int x;
    int y;
    int z;

    for (axis = 0; axis < 3; ++axis) {
        minimum[axis] = (int)((bounds[0].x - spatial_000.minimum_0c.x) / spatial_000.node_extent_70);
    }
    minimum[0] = (int)((bounds[0].x - spatial_000.minimum_0c.x) / spatial_000.node_extent_70);
    minimum[1] = (int)((bounds[0].y - spatial_000.minimum_0c.y) / spatial_000.node_extent_70);
    minimum[2] = (int)((bounds[0].z - spatial_000.minimum_0c.z) / spatial_000.node_extent_70);
    maximum[0] = (int)((bounds[1].x - spatial_000.minimum_0c.x) / spatial_000.node_extent_70);
    maximum[1] = (int)((bounds[1].y - spatial_000.minimum_0c.y) / spatial_000.node_extent_70);
    maximum[2] = (int)((bounds[1].z - spatial_000.minimum_0c.z) / spatial_000.node_extent_70);

    prop_key = ((index + 1) & 0xffff) + 0x80000;
    for (x = minimum[0]; x <= maximum[0]; ++x) {
        for (y = minimum[1]; y <= maximum[1]; ++y) {
            for (z = minimum[2]; z <= maximum[2]; ++z) {
                cell_key = ((x << 8) + y) * 0x100 + 1 + z;
                pair = reinterpret_cast<W8OctreeIndex**>(object_registry);
                by_prop = pair[1];
                RemoveEntry00438C90(by_prop, &prop_key, (const int*)&cell_key);
                slot = AllocateEntry004393E0(by_prop);
                entries = static_cast<W8OctreeEntry*>(by_prop->entries);
                hash = ((prop_key >> 10 ^ prop_key) >> 10 ^ prop_key) &
                    (by_prop->bucket_count - 1);
                entries[slot].key = prop_key;
                entries[slot].value = cell_key;
                entries[slot].next_index = static_cast<int*>(by_prop->bucket_heads)[hash];
                static_cast<int*>(by_prop->bucket_heads)[hash] = slot;

                by_cell = pair[0];
                RemoveEntry00438C90(by_cell, &cell_key, (const int*)&prop_key);
                if (by_cell->free_head == -1) {
                    GrowIndex00439290(by_cell);
                }
                slot = by_cell->free_head;
                entries = static_cast<W8OctreeEntry*>(by_cell->entries);
                by_cell->free_head = entries[slot].next_index;
                entries[slot].key = cell_key;
                entries[slot].value = prop_key;
                hash = ((cell_key >> 10 ^ cell_key) >> 10 ^ cell_key) &
                    (by_cell->bucket_count - 1);
                entries[slot].next_index = static_cast<int*>(by_cell->bucket_heads)[hash];
                static_cast<int*>(by_cell->bucket_heads)[hash] = slot;
            }
        }
    }
}

/* Build the cell walk between two world points.

   The three axis deltas are taken in cells; the longest of them drives, and the
   other two each get an error triple seeded from where inside its cell the line
   starts. The step count covers the driving axis plus the partial cell at each
   end, which is why the remainder decides between one and two extra. */
// FUNCTION: WIZ8 0x004362d0
void W8Octree::BuildCellWalk(
    const srVector3T<float>* from, const srVector3T<float>* to, W8OctreeWalk* walk)
{
    int from_cell[3];
    int to_cell[3];
    float fraction[3];
    float delta[3];
    int step[3];
    int extent[3];
    float cell_size;
    int cell;
    int axis;
    int longest = 0;
    int major = 0;
    int minor_0;
    int minor_1;
    int span;

    cell_size = spatial_000.node_extent_70 * g_octree_cell_scale_005ebcd0;
    cell = (int)cell_size;
    from_cell[0] = (int)((from->x - spatial_000.minimum_0c.x) * g_octree_cell_scale_005ebcd0);
    to_cell[0] = (int)((to->x - spatial_000.minimum_0c.x) * g_octree_cell_scale_005ebcd0);
    from_cell[1] = (int)((from->y - spatial_000.minimum_0c.y) * g_octree_cell_scale_005ebcd0);
    to_cell[1] = (int)((to->y - spatial_000.minimum_0c.y) * g_octree_cell_scale_005ebcd0);
    from_cell[2] = (int)((from->z - spatial_000.minimum_0c.z) * g_octree_cell_scale_005ebcd0);
    to_cell[2] = (int)((to->z - spatial_000.minimum_0c.z) * g_octree_cell_scale_005ebcd0);

    for (axis = 0; axis < 3; ++axis) {
        span = to_cell[axis] - from_cell[axis];
        fraction[axis] = (float)(from_cell[axis] % cell) / cell_size;
        delta[axis] = (float)span;
        if (span < 0) {
            step[axis] = -1;
            span = -span;
        } else {
            step[axis] = 1;
            fraction[axis] = 1.0f - fraction[axis];
        }
        if (longest < span) {
            major = axis;
            longest = span;
        }
        extent[axis] = span;
    }

    minor_0 = (major + 1) % 3;
    minor_1 = (major + 2) % 3;
    walk->error_delta_28 = (int)((float)fabs(delta[minor_0] / delta[major]) * cell_size);
    walk->error_2c = (int)(cell_size * fraction[minor_0] -
                           (float)walk->error_delta_28 * fraction[major]);
    walk->error_delta_34 = (int)((float)fabs(delta[minor_1] / delta[major]) * cell_size);
    walk->error_38 = (int)(cell_size * fraction[minor_1] -
                           (float)walk->error_delta_34 * fraction[major]);
    walk->count_24 = longest % cell == 0 ? longest / cell + 1 : longest / cell + 2;

    walk->minor_axis_1c = minor_0;
    walk->error_reset_30 = cell;
    walk->error_reset_3c = cell;
    walk->minor_axis_20 = minor_1;
    walk->major_axis_18 = major;
    walk->cell_00[0] = from_cell[0] / cell;
    walk->cell_00[1] = from_cell[1] / cell;
    walk->cell_00[2] = from_cell[2] / cell;
    walk->step_0c[0] = step[0];
    walk->step_0c[1] = step[1];
    walk->step_0c[2] = step[2];
}

/* Tell a monster which mesh it now stands on, then queue its move.

   A location the octree cannot resolve, or one whose submesh has no live model
   instance, clears the monster's cached mesh rather than leaving a stale one. */
// FUNCTION: WIZ8 0x0042e540
void W8Octree::UpdateMonsterLocation(
    unsigned short location_id, const srVector3T<float>* position)
{
    int queue_id = location_id + 1;
    unsigned int monster_list_index;
    W8MonsterInfo* info;
    W8Monster* monster;
    int sector;
    int mesh;
    int point[3];

    if (location_id == 0) {
        return;
    }
    monster_list_index =
        MonsterGetIndexByLocationID(0x4c4, OCTREE_CPP, location_id, 1);
    info = MonsterGetScriptPartByLocationIndex(monster_list_index);
    if (info != 0 && info->monster != 0) {
        monster = info->monster;
        sector = GetSectorForPosition00430BF0(position);
        if (sector == 0 ||
            (mesh = reinterpret_cast<int*>(g_world->psrMeshes)
                 [reinterpret_cast<int*>(m_owned_0d8)[sector * 4 + 1]]) == 0) {
            monster->node_308 = 0;
        } else {
            monster->node_308 = reinterpret_cast<srNode*>(mesh);
        }
    }
    point[0] = (int)((position->x - spatial_000.minimum_0c.x) / spatial_000.node_extent_70);
    point[1] = (int)((position->y - spatial_000.minimum_0c.y) / spatial_000.node_extent_70);
    point[2] = (int)((position->z - spatial_000.minimum_0c.z) / spatial_000.node_extent_70);
    object_registry->RegisterObjectCell(0xc, queue_id, point);
}

/* Read one .oct file into a fresh octree.

   The original name is the image's own: fourteen assertions in this body spell
   it ReadOctFile. The shape is one block repeated for every table the file
   carries - allocate, read, accumulate the byte count, and on either failure
   copy a message into the local buffer and stop advancing. fSuccess threading
   is what makes the failures cascade rather than each one returning; the flag
   at +0x000 bit 31 is the load error the caller tests.

   A null path builds an empty octree, and a level whose preprocessed files
   cannot be built runs on the LVL file alone with the same error bit set. */
// FUNCTION: WIZ8 0x0042bc10
W8Octree::W8Octree(const char* path, void** game_data)
{
    unsigned char header[0xf5];
    char acMessage[256];
    int hOctFile;
    unsigned int uiRead;
    int uiTerminator;
    unsigned char fSuccess;
    unsigned char fLoaded;
    void* block;
    unsigned int index;
    unsigned int limit;
    unsigned int name_length;
    char* extension;
    void* pGameData = 0;

    Reset();
    if (path == 0) {
        Initialize(0);
        return;
    }
    if (CheckLevelAssetSet0042CCC0(path) < 0) {
        goto failed;
    }
    if (CheckLevelAssetSet0042CCC0(path) > 0 &&
        BuildPreprocessedFiles00492E60(path) == 0) {
        g_octree_6598a4 = 0;
        spatial_000.flags_00 |= 0x80000000;
        ReportStartupMessage004969D0("Cannot find or build current preprocessed files.");
        ReportStartupMessage004969D0(
            "Attempting to run with LVL file only -- SOME FEATURES DISABLED.");
        ReportStartupMessage004969D0(0);
        return;
    }
    hOctFile = FileOpen(const_cast<char*>(path), 1, 0);
    if (hOctFile == 0) {
        srAssertFail("hOctFile", OCTREE_CPP, 0xa3,
                     "ReadOctFile: Couldn't open octree file.");
    }
    fSuccess = ReadVirtualFile(hOctFile, header, 0xf5, &uiRead);
    g_octree_bytes_read_00659888 += uiRead;
    fLoaded = 0;
    if (fSuccess != 0) {
        fSuccess = ReadVirtualFile(hOctFile, &uiTerminator, 4, &uiRead);
        if (fSuccess == 0 || uiTerminator != -1) {
            srAssertFail("fSuccess && (uiTerminator==0xffffffff)", OCTREE_CPP, 0xac,
                         "ReadOctFile: Header of Oct file longer than expected.");
        }
        fLoaded = 0;
        if (fSuccess != 0) {
            Initialize(header);
            name_length = strlen(path) + 1;
            if (name_length != 1) {
                if (m_owned_0c0 != 0) {
                    free(m_owned_0c0);
                }
                m_owned_0c0 = malloc(name_length);
                if (m_owned_0c0 != 0) {
                    strcpy(static_cast<char*>(m_owned_0c0), path);
                    extension = strrchr(static_cast<char*>(m_owned_0c0), '.');
                    if (extension != 0 &&
                        extension - static_cast<char*>(m_owned_0c0) >
                            (int)(name_length - 7)) {
                        *extension = '\0';
                    }
                }
            }

            block = malloc((ReadHeader<unsigned long>(header, 0x6a) * 9 + 0x12) * 4);
            m_owned_09c = static_cast<W8OctPreTreeBranch*>(block);
            if (block == 0) {
                fSuccess = 0;
                strcpy(acMessage, "ReadOctFile: Couldn't allocate octree nodes.");
            } else {
                fSuccess = ReadVirtualFile(
                    hOctFile, block, ReadHeader<unsigned long>(header, 0x6a) * 0x24, &uiRead);
                if (fSuccess == 0) {
                    strcpy(acMessage, "ReadOctFile: Couldn't read octree nodes.");
                }
            }
            g_octree_bytes_read_00659888 += uiRead;
            fLoaded = 0;
            if (fSuccess != 0) {
                block = malloc((ReadHeader<unsigned long>(header, 0x6e) * 5 + 10) * 8);
                m_owned_0a0 = static_cast<W8OctPreTreeLeaf*>(block);
                if (block == 0) {
                    fSuccess = 0;
                    strcpy(acMessage, "ReadOctFile: Couldn't allocate octree leaves.");
                } else {
                    fSuccess = ReadVirtualFile(
                        hOctFile, block,
                        ReadHeader<unsigned long>(header, 0x6e) * 0x28, &uiRead);
                    if (fSuccess == 0) {
                        strcpy(acMessage, "ReadOctFile: Couldn't read octree leaves.");
                    }
                }
                g_octree_bytes_read_00659888 += uiRead;
                fLoaded = 0;
                if (fSuccess != 0) {
                    block = malloc(ReadHeader<unsigned long>(header, 0x82) * 4 + 8);
                    m_owned_0d0 = static_cast<unsigned long*>(block);
                    if (block == 0) {
                        fLoaded = 0;
                        strcpy(acMessage,
                               "ReadOctFile: Couldn't allocate polygon index list for leaves.");
                    } else {
                        fLoaded = ReadVirtualFile(
                            hOctFile, block,
                            ReadHeader<unsigned long>(header, 0x82) * 4, &uiRead);
                        if (fLoaded == 0) {
                            strcpy(acMessage,
                                   "ReadOctFile: Couldn't read polygon index list for leaves.");
                        }
                    }
                    g_octree_bytes_read_00659888 += uiRead;
                }
            }
        }
    }

    limit = m_positional_0a4 * m_positional_0a8 * m_positional_0ac;
    if (fLoaded != 0 && limit < 250000) {
        block = malloc(limit * 4);
        m_owned_0b0 = block;
        if (block == 0) {
            strcpy(acMessage, "ReadOctFile: Couldn't allocate polygon index list for regions.");
            goto finish;
        }
        fLoaded = ReadVirtualFile(
            hOctFile, block,
            m_positional_0a4 * m_positional_0a8 * m_positional_0ac * 4, &uiRead);
        if (fLoaded == 0) {
            strcpy(acMessage, "ReadOctFile: Couldn't read octree nodes.");
        }
        g_octree_bytes_read_00659888 += uiRead;
    } else {
        m_owned_0b0 = 0;
    }

    fSuccess = 0;
    if (fLoaded != 0) {
        block = malloc(ReadHeader<unsigned long>(header, 0x72) * 4 + 8);
        m_owned_0d4 = block;
        if (block == 0) {
            fSuccess = 0;
            strcpy(acMessage, "ReadOctFile: Couldn't allocate Poly Lookup table.");
        } else {
            fLoaded = ReadVirtualFile(
                hOctFile, block, ReadHeader<unsigned long>(header, 0x72) * 4, &uiRead);
            if (fLoaded == 0) {
                strcpy(acMessage, "ReadOctFile: Couldn't read Poly Lookup table.");
            }
            g_octree_bytes_read_00659888 += uiRead;
            fSuccess = 0;
            if (fLoaded != 0) {
                if (ReadHeader<unsigned long>(header, 0x92) != 0) {
                    block = malloc(ReadHeader<unsigned long>(header, 0x92) * 2 + 4);
                    m_owned_148 = static_cast<unsigned short*>(block);
                    if (block == 0) {
                        fSuccess = 0;
                        strcpy(acMessage, "ReadOctFile: Couldn't allocate region list.");
                        goto finish;
                    }
                    fLoaded = ReadVirtualFile(
                        hOctFile, block,
                        ReadHeader<unsigned long>(header, 0x92) * 2, &uiRead);
                    if (fLoaded == 0) {
                        strcpy(acMessage, "ReadOctFile: Couldn't read region list.");
                    }
                    g_octree_bytes_read_00659888 += uiRead;
                }
                fSuccess = 0;
                if (fLoaded != 0) {
                    if (ReadHeader<unsigned long>(header, 0x86) != 0) {
                        block = malloc(ReadHeader<unsigned long>(header, 0x86) * 4 + 8);
                        m_owned_12c = static_cast<unsigned long*>(block);
                        if (block == 0) {
                            fSuccess = 0;
                            strcpy(acMessage, "ReadOctFile: Couldn't allocate GD Poly list.");
                            goto finish;
                        }
                        fLoaded = ReadVirtualFile(
                            hOctFile, block,
                            ReadHeader<unsigned long>(header, 0x86) * 4, &uiRead);
                        if (fLoaded == 0) {
                            strcpy(acMessage, "ReadOctFile: Couldn't read GD Poly list.");
                        }
                        g_octree_bytes_read_00659888 += uiRead;
                    }
                    fSuccess = 0;
                    if (fLoaded != 0) {
                        if (ReadHeader<unsigned long>(header, 0x8a) != 0) {
                            block = malloc(ReadHeader<unsigned long>(header, 0x8a) * 2 + 4);
                            m_owned_130 = block;
                            if (block == 0) {
                                fSuccess = 0;
                                strcpy(acMessage,
                                       "ReadOctFile: Couldn't allocate Trigger list.");
                                goto finish;
                            }
                            fLoaded = ReadVirtualFile(
                                hOctFile, block,
                                ReadHeader<unsigned long>(header, 0x8a) * 2, &uiRead);
                            if (fLoaded == 0) {
                                strcpy(acMessage, "ReadOctFile: Couldn't read Trigger list.");
                            }
                            g_octree_bytes_read_00659888 += uiRead;
                        }
                        fSuccess = 0;
                        if (fLoaded != 0) {
                            if (ReadHeader<unsigned short>(header, 0x96) > 1) {
                                block = malloc(
                                    (ReadHeader<unsigned short>(header, 0x96) + 2) * 0xe8);
                                spatial_000.owned_5c =
                                    static_cast<W8OctRegionVolume0049E460*>(
                                        block);
                                if (block == 0) {
                                    fSuccess = 0;
                                    strcpy(acMessage,
                                           "ReadOctFile: Couldn't allocate region array.");
                                    goto finish;
                                }
                                fLoaded = ReadVirtualFile(
                                    hOctFile, block,
                                    ReadHeader<unsigned short>(header, 0x96) * 0xe8, &uiRead);
                                if (fLoaded == 0) {
                                    strcpy(acMessage,
                                           "ReadOctFile: Couldn't read region array.");
                                }
                                g_octree_bytes_read_00659888 += uiRead;
                            }
                            fSuccess = 0;
                            if (fLoaded != 0) {
                                fLoaded = ReadVirtualFile(
                                    hOctFile, &uiTerminator, 4, &uiRead);
                                if (fLoaded == 0 || uiTerminator != -1) {
                                    srAssertFail(
                                        "fSuccess && (uiTerminator==0xffffffff)",
                                        OCTREE_CPP, 0x15f,
                                        "ReadOctFile: PreRegions longer than expected.");
                                }
                                fSuccess = 0;
                                if (fLoaded != 0) {
                                    if (ReadHeader<unsigned long>(header, 0x66) != 0) {
                                        block = malloc(
                                            (ReadHeader<unsigned long>(header, 0x66) + 1) * 0x10);
                                        m_owned_0d8 = block;
                                        if (block == 0) {
                                            fLoaded = 0;
                                            strcpy(acMessage,
                                                   "ReadOctFile: Couldn't allocate submesh array.");
                                        } else {
                                            fLoaded = ReadVirtualFile(
                                                hOctFile, block,
                                                (ReadHeader<unsigned long>(header, 0x66) + 1) * 0x10,
                                                &uiRead);
                                            if (fLoaded == 0) {
                                                strcpy(acMessage,
                                                       "ReadOctFile: Couldn't read submesh array.");
                                            }
                                            g_octree_bytes_read_00659888 += uiRead;
                                            if (fLoaded != 0) {
                                                unsigned int* scan;
                                                unsigned int remaining;

                                                limit = 0;
                                                scan = reinterpret_cast<unsigned int*>(
                                                    static_cast<unsigned char*>(m_owned_0d8) + 0xc);
                                                remaining =
                                                    ReadHeader<unsigned long>(header, 0x66) + 1;
                                                do {
                                                    if (limit < *scan) {
                                                        limit = *scan;
                                                    }
                                                    scan += 4;
                                                    --remaining;
                                                } while (remaining != 0);
                                                ++limit;
                                                if (limit > 9999) {
                                                    srAssertFail("(i2 < 10000)", OCTREE_CPP,
                                                                 0x179, 0);
                                                }
                                                g_octree_storage_00659770 =
                                                    reinterpret_cast<unsigned long>(
                                                        malloc(limit * 4));
                                                if (g_octree_storage_00659770 == 0) {
                                                    fLoaded = 0;
                                                    strcpy(acMessage,
                                                           "ReadOctFile: Couldn't allocate polygon "
                                                           "index list for regions.");
                                                } else {
                                                    for (index = 0; index < limit; ++index) {
                                                        reinterpret_cast<unsigned int*>(
                                                            g_octree_storage_00659770)[index] =
                                                            index;
                                                    }
                                                }
                                            }
                                        }
                                        if (m_positional_1b4 != 0) {
                                            m_pAlphaBits = new BitArray(m_positional_1b4);
                                            if (m_pAlphaBits == 0) {
                                                srAssertFail(
                                                    "m_pAlphaBits", OCTREE_CPP, 0x18a,
                                                    "ReadOctFile: Failure allocating Alpha Bits.");
                                            }
                                            if (BitArrayLoad0043AEC0(m_pAlphaBits, hOctFile) == 0) {
                                                srAssertFail(
                                                    "m_pAlphaBits->Load(hOctFile)", OCTREE_CPP,
                                                    0x18b,
                                                    "ReadOctFile: Failure reading Alpha Bits.");
                                            }
                                        }
                                        if (fLoaded != 0 && m_ulNumParticles != 0) {
                                            m_pusMeshParticleLookup =
                                                static_cast<unsigned short*>(
                                                    malloc(m_positional_1b4 * 2 + 2));
                                            if (m_pusMeshParticleLookup == 0) {
                                                srAssertFail(
                                                    "m_pusMeshParticleLookup", OCTREE_CPP, 0x191,
                                                    "ReadOctFile: Couldn't allocate Mesh Particle "
                                                    "Lookup Table.");
                                            }
                                            fLoaded = ReadVirtualFile(
                                                hOctFile, m_pusMeshParticleLookup,
                                                m_positional_1b4 * 2 + 2, &uiRead);
                                            if (fLoaded == 0) {
                                                strcpy(acMessage,
                                                       "ReadOctFile: Couldn't read octree nodes.");
                                            }
                                            g_octree_bytes_read_00659888 += uiRead;
                                            m_pusMeshParticles = static_cast<unsigned short*>(
                                                malloc(m_positional_0e8 * 2));
                                            if (m_pusMeshParticles == 0) {
                                                srAssertFail(
                                                    "m_pusMeshParticles", OCTREE_CPP, 0x198,
                                                    "ReadOctFile: Couldn't allocate Mesh Particle "
                                                    "Link Table.");
                                            }
                                            fLoaded = ReadVirtualFile(
                                                hOctFile, m_pusMeshParticles,
                                                m_positional_0e8 * 2, &uiRead);
                                            if (fLoaded == 0) {
                                                strcpy(acMessage,
                                                       "ReadOctFile: Couldn't read octree nodes.");
                                            }
                                        }
                                        if (fLoaded != 0 && m_ulNumProps != 0) {
                                            m_pusMeshPropLookup = static_cast<unsigned short*>(
                                                malloc(m_positional_1b4 * 2 + 2));
                                            if (m_pusMeshPropLookup == 0) {
                                                srAssertFail(
                                                    "m_pusMeshPropLookup", OCTREE_CPP, 0x1a1,
                                                    "ReadOctFile: Couldn't allocate Mesh Prop "
                                                    "Lookup Table.");
                                            }
                                            fLoaded = ReadVirtualFile(
                                                hOctFile, m_pusMeshPropLookup,
                                                m_positional_1b4 * 2 + 2, &uiRead);
                                            if (fLoaded == 0) {
                                                strcpy(acMessage,
                                                       "ReadOctFile: Couldn't read octree nodes.");
                                            }
                                            g_octree_bytes_read_00659888 += uiRead;
                                            m_pusMeshProps = static_cast<unsigned short*>(
                                                malloc(m_positional_0f4 * 2));
                                            if (m_pusMeshProps == 0) {
                                                srAssertFail(
                                                    "m_pusMeshProps", OCTREE_CPP, 0x1a8,
                                                    "ReadOctFile: Couldn't allocate Mesh Prop Link "
                                                    "Table.");
                                            }
                                            fLoaded = ReadVirtualFile(
                                                hOctFile, m_pusMeshProps,
                                                m_positional_0f4 * 2, &uiRead);
                                            if (fLoaded == 0) {
                                                strcpy(acMessage,
                                                       "ReadOctFile: Couldn't read octree nodes.");
                                            }
                                        }
                                    }
                                    fSuccess = 0;
                                    if (fLoaded != 0) {
                                        fLoaded = ReadVirtualFile(
                                            hOctFile, &uiTerminator, 4, &uiRead);
                                        if (fLoaded == 0 || uiTerminator != -1) {
                                            srAssertFail(
                                                "fSuccess && (uiTerminator==0xffffffff)",
                                                OCTREE_CPP, 0x1b2,
                                                "ReadOctFile: Mesh, Prop, and Particle data longer "
                                                "than expected.");
                                        }
                                        fSuccess = 0;
                                        if (fLoaded != 0) {
                                            if (ReadHeader<unsigned long>(header, 0x7e) != 0) {
                                                pathing_180 = new W8PathingService();
                                                if (pathing_180 == 0) {
                                                    goto finish;
                                                }
                                                pathing_180->ConfigureForLevel(
                                                    ReadHeader<unsigned long>(header, 0x7e),
                                                    (float)ReadHeader<unsigned long>(header, 0xac),
                                                    ReadHeader<unsigned long>(header, 0xb4),
                                                    reinterpret_cast<const float*>(header + 0x0e),
                                                    static_cast<char*>(m_owned_0c0));
                                                fLoaded = pathing_180->Load00458CE0(hOctFile);
                                            }
                                            fSuccess = 0;
                                            if (fLoaded != 0) {
                                                if (m_ulNumProps != 0 &&
                                                    ReadHeader<char>(header, 0xb8) != 0) {
                                                    m_pPropSunBits = new BitArray(m_ulNumProps);
                                                    if (m_pPropSunBits == 0) {
                                                        srAssertFail(
                                                            "m_pPropSunBits", OCTREE_CPP, 0x1c5,
                                                            "ReadOctFile: Couldn't allocate Prop "
                                                            "Sun Bits.");
                                                    }
                                                    if (BitArrayLoad0043AEC0(
                                                            m_pPropSunBits, hOctFile) == 0) {
                                                        srAssertFail(
                                                            "m_pPropSunBits->Load(hOctFile)",
                                                            OCTREE_CPP, 0x1c6,
                                                            "ReadOctFile: Failure reading Prop Sun "
                                                            "Bits.");
                                                    }
                                                }
                                                fSuccess = ReadVirtualFile(
                                                    hOctFile, &uiTerminator, 4, &uiRead);
                                                if (fSuccess == 0) {
                                                    strcpy(acMessage,
                                                           "ReadOctFile: Couldn't read octree file "
                                                           "terminator.");
                                                }
                                                if (uiTerminator != -1) {
                                                    strcpy(acMessage,
                                                           "ReadOctFile: Octree file longer than "
                                                           "expected.");
                                                    fSuccess = 0;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

finish:
    g_octree_game_data_00652db0 = 0;
    *game_data = 0;
    if (fSuccess != 0 && ReadHeader<unsigned long>(header, 0x86) != 0) {
        pGameData = new W8GameData(hOctFile, 0);
        if (pGameData == 0) {
            strcpy(acMessage, "ReadOctFile: Couldn't allocate submesh array.");
            fSuccess = 0;
        } else {
            fSuccess = ReadVirtualFile(hOctFile, &uiTerminator, 4, &uiRead);
            if (fSuccess == 0 || uiTerminator != -1) {
                srAssertFail("fSuccess && (uiTerminator==0xffffffff)", OCTREE_CPP, 0x1e2,
                             "ReadOctFile: GameData portion of Oct file longer than expected.");
            }
        }
    }
    CloseVirtualFile(hOctFile);
    if (fSuccess != 0) {
        g_octree_6598a4 = this;
        *static_cast<W8Octree**>(pGameData) = this;
        *game_data = pGameData;
        g_octree_game_data_00652db0 = static_cast<W8GameData*>(pGameData);
        m_positional_169 = ReadLevelName00432E90(static_cast<char*>(m_owned_0c0));
        ApplyLevelName00432B80(static_cast<char*>(m_owned_0c0));
        if (pathing_180 != 0) {
            ReadWaypointFile0043A0F0();
        }
        return;
    }
failed:
    g_octree_6598a4 = 0;
    spatial_000.flags_00 |= 0x80000000;
}

/* The raw 0x29c allocation is reset before ReadOctFile applies its header.
   The image spells the clears individually; memset expresses the same POD
   construction invariant while the positional fields are still unnamed. */
// FUNCTION: WIZ8 0x0042d040
void W8Octree::Reset()
{
    g_octree_storage_00659770 = 0;
    g_octree_state_00659890 = 0;
    spatial_000.Reset0046CDC0();
    memset(this, 0, sizeof(*this));
    current_sector = -1;
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
        object_registry = reinterpret_cast<W8OctreeObjectRegistry*>(pair);
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
    if (pathing_180 != 0) {
        pathing_180->SaveWaypointSnapshot00459400(0);
    }

    free(m_aulGDObjs);
    free(m_owned_09c);
    free(m_owned_0a0);
    free(m_owned_0d0);
    free(m_owned_0d4);
    DestroyBitArray(m_pAlphaBits);
    free(m_owned_0b0);
    free(m_owned_12c);
    free(m_owned_148);
    free(m_owned_130);

    if (m_owned_150 != 0) {
        W8OctreeIndex* index = static_cast<W8OctreeIndex*>(m_owned_150);
        delete[] static_cast<int*>(index->bucket_heads);
        delete[] static_cast<W8OctreeEntry*>(index->entries);
        index->bucket_heads = 0;
        index->entries = 0;
        index->free_head = -1;
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

    free(m_pusMeshParticleLookup);
    m_pusMeshParticleLookup = 0;
    free(m_pusMeshParticles);
    m_pusMeshParticles = 0;
    free(m_pusMeshPropLookup);
    m_pusMeshPropLookup = 0;
    free(m_pusMeshProps);
    m_pusMeshProps = 0;
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

    pair = reinterpret_cast<W8OctreeIndex**>(object_registry);
    if (pair != 0) {
        DestroyIndex(pair[0]);
        DestroyIndex(pair[1]);
        delete[] pair;
    }

    free(m_owned_0d8);
    if (pathing_180 != 0) {
        delete pathing_180;
        pathing_180 = 0;
    }
    DestroyBitArray(m_pPropSunBits);
}

/* The node whose 0x1c is non-null is the only kind worth attaching. */
extern int g_shared_mark_006598ac;

extern unsigned int __stdcall OctreeTraverse(
    void* walker, void* arg_2, void* arg_3, int kind, unsigned int limit);   /* 0x0042F280 */

/* Attach a visited set to the walker, but only one that has been built. */
// FUNCTION: WIZ8 0x0042e3e0
void W8Octree::SetVisitedSet0042E3E0(BitArray* visited)
{
    if (visited->puiIndex != 0) {
        m_pPropSunBits = visited;
    }
}

/* Advance or reset the walk's mark. A negative offset takes the shared
   counter's value as this walk's base; anything else advances the shared
   counter and reports whether that mark has already been visited - which with
   no set attached is always no. */
// FUNCTION: WIZ8 0x0042e400
int W8Octree::MarkVisited0042E400(int offset)
{
    if (offset < 0) {
        mark_base_184 = g_shared_mark_006598ac;
        return 0;
    }
    g_shared_mark_006598ac = mark_base_184 + offset;
    if (m_pPropSunBits != 0 && m_pPropSunBits->Test(g_shared_mark_006598ac)) {
        return 1;
    }
    return 0;
}

/* Visit a point handed over by address, copied to the stack first so the
   caller's copy is not the one the traversal holds. */
// FUNCTION: WIZ8 0x0042e620
void W8Octree::VisitPointCopy0042E620(
    unsigned short location_id, srVector3T<float>* position)
{
    srVector3T<float> copy;

    copy.x = position->x;
    copy.y = position->y;
    copy.z = position->z;
    UpdateMonsterLocation(location_id, &copy);
}

/* Start a traversal of the twelfth kind. A limit of zero means no limit, which
   is what the -1 stands for. */
// FUNCTION: WIZ8 0x0042ef00
unsigned int __stdcall OctreeTraverseKind12(
    void* walker, void* arg_2, void* arg_3, unsigned short limit)
{
    unsigned int bound = (unsigned int)-1;

    if (limit != 0) {
        bound = limit;
    }
    return OctreeTraverse(walker, arg_2, arg_3, 0xc, bound);
}

/* Queue one node of the thirteenth kind, with its three coordinates converted
   from floating point - which is what puts three ftol calls in a row here. */
// FUNCTION: WIZ8 0x0042e810
void W8Octree::QueueOctreeKind130042E810(
    int id,
    const srVector3T<float>* position)
{
    int point[3];

    point[0] = (int)((position->x - spatial_000.minimum_0c.x) /
                     spatial_000.node_extent_70);
    point[1] = (int)((position->y - spatial_000.minimum_0c.y) /
                     spatial_000.node_extent_70);
    point[2] = (int)((position->z - spatial_000.minimum_0c.z) /
                     spatial_000.node_extent_70);
    object_registry->RegisterObjectCell(0xd, id + 1, point);
}

/* Step one navigator's movement tail towards its target.

   With a pathing service present the work belongs to that service, and which of
   its two entry points runs is the link mode's decision. Without one this walks
   the target itself. The direction is the full vector to the target with its
   height component dropped, so the step stays in the horizontal plane; its
   length is clamped to the distance remaining, and reaching the target is what
   the return value reports. The new position is mirrored into the attachment's
   own three copies. */
// FUNCTION: WIZ8 0x00434620
unsigned int W8Octree::AdvanceNavigator(
    W8NavigatorMovementState* movement, float radius, float separation)
{
    srVector3T<float> vecDir;
    srVector3T<float> vecPos;
    W8NavigatorAttachment* attachment;
    float distance;
    float step;
    unsigned char reached = 1;

    if (pathing_180 != 0) {
        if (g_navigator_link_mode_00659c10 == 0) {
            return pathing_180->StepAlongPath004669B0(movement, radius, separation);
        }
        return pathing_180->StepMonsterAlongPath00467150(
            movement, radius, separation);
    }
    if (g_navigator_link_mode_00659c10 != 0) {
        return 1;
    }
    vecDir.x = movement->target_position_04c.x - movement->position_040.x;
    vecDir.y = movement->target_position_04c.y - movement->position_040.y;
    vecDir.z = movement->target_position_04c.z - movement->position_040.z;
    vecDir.y = 0.0f;
    distance = (float)sqrt(vecDir.x * vecDir.x + vecDir.z * vecDir.z);
    step = g_object_6598bc->GetValue28() * movement->movement_scale_060 *
        g_rate_006068EC * g_world_scale_005ebc40;
    if (step >= distance) {
        step = distance;
    } else {
        reached = 0;
    }
    if ((double)(vecDir.x * vecDir.x + vecDir.z * vecDir.z) != g_zero_005ebb40) {
        vecDir.y = 0.0f;
        step = step / (float)sqrt(vecDir.x * vecDir.x + vecDir.z * vecDir.z);
        vecDir.x = vecDir.x * step;
        vecDir.z = vecDir.z * step;
    }
    vecPos.x = vecDir.x + movement->position_040.x;
    vecPos.y = vecDir.y + movement->position_040.y;
    vecPos.z = vecDir.z + movement->position_040.z;
    movement->position_040 = vecPos;
    attachment = movement->attachment_0ac;
    *attachment->position_4c = vecPos;
    attachment->position_34 = *attachment->position_4c;
    attachment->position_10 = *attachment->position_4c;
    return reached;
}

/* Settle both ends of a portal transition onto the ground before the move is
   applied. Each end is capped at the octree's height ceiling first, and its
   settled height is only taken when the drop actually hit something. The pair
   is then handed on together, which is why one body carries both. */
// FUNCTION: WIZ8 0x00434a30
void W8Octree::AdjustPortalDestination(
    srVector3T<float>* destination, const srVector3T<float>* source)
{
    srVector3T<float> local_destination;
    srVector3T<float> local_source;
    srVector3T<float> probe;
    unsigned char hit;

    if (pathing_180 == 0) {
        return;
    }
    if (pathing_180->flag_1c8 == 0 && pathing_180->m_ulNumSurfaces != 0) {
        return;
    }
    local_destination = *destination;
    local_source = *source;

    hit = 0;
    if (local_destination.y > spatial_000.clipped_maximum_30.y) {
        local_destination.y = spatial_000.clipped_maximum_30.y;
    }
    probe = local_destination;
    SettleToGround00433820(&probe, &hit, 1, 500.0f);
    if (hit != 0) {
        local_destination.y = probe.y;
    }

    hit = 0;
    if (local_source.y > spatial_000.clipped_maximum_30.y) {
        local_source.y = spatial_000.clipped_maximum_30.y;
    }
    probe = local_source;
    SettleToGround00433820(&probe, &hit, 1, 500.0f);
    if (hit != 0) {
        local_source.y = probe.y;
    }
    pathing_180->EditTeleportalLink(&local_destination, &local_source);
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
