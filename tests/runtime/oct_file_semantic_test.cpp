/* In-process semantic scenario for the OctPreTree cluster's serialization
   path. The scenario builds a minimal but non-empty OctPreTree and W8GameData
   in memory, writes them through the recovered WriteOctFile, verifies the
   emitted bytes against the retail-defined layout (header fields, section
   sentinels, terminator positions, total size), then loads the file back
   through the recovered W8Octree(path, &game_data) reader - the ReadOctFile
   body - and compares the round-tripped state.

   The writer leaves header +0xb4 and the +0xc9..+0xf4 tail unwritten exactly
   like retail, so a whole-file golden comparison is impossible; the checks
   target every offset the format defines.

   It also exercises the shared stHash sort helpers the build path uses:
   InsertionSort/QuickSort and the SortByKey ordering heuristics.

   The created objects are deliberately leaked: the runtime image stubs the
   destructor-side helpers (~W8GameData calls an unrecovered body), and the
   scenario exits the process immediately after reporting. The globals the
   constructors publish through are restored so the report path runs clean. */

#include "oct_file_semantic_test.h"

#include "wiz8/engine_code/3d.h"
#include "wiz8/engine_code/GDProp.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/OctBuildPreTree.h"
#include "wiz8/engine_code/OctPath.h"
#include "wiz8/engine_code/OctPreTree.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/stHash.hpp"

#include "FileMan.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned int LoadDword(const unsigned char* bytes, unsigned long offset)
{
    unsigned int value;
    memcpy(&value, bytes + offset, sizeof(value));
    return value;
}

static unsigned short LoadWord(const unsigned char* bytes, unsigned long offset)
{
    unsigned short value;
    memcpy(&value, bytes + offset, sizeof(value));
    return value;
}

static float LoadFloat(const unsigned char* bytes, unsigned long offset)
{
    float value;
    memcpy(&value, bytes + offset, sizeof(value));
    return value;
}

static unsigned char SortedAscending(const unsigned long* values, int count)
{
    for (int index = 1; index < count; ++index) {
        if (values[index - 1] > values[index]) {
            return 0;
        }
    }
    return 1;
}

static void FillScrambled(unsigned long* values, int count, unsigned int seed)
{
    for (int index = 0; index < count; ++index) {
        seed = seed * 1103515245u + 12345u;
        values[index] = (seed >> 8) & 0xffff;
    }
}

/* InsertionSort sorts values[first..last): the small-range tail every
   QuickSort ends in. */
static unsigned char CheckInsertionSort()
{
    unsigned long values[8] = {7, 1, 5, 3, 0, 6, 2, 4};
    InsertionSort(values, 0, 8);
    return SortedAscending(values, 8);
}

/* 64 scrambled entries drive QuickSort through its partitioning recursion. */
static unsigned char CheckQuickSortPartition()
{
    unsigned long values[64];
    FillScrambled(values, 64, 0x8c30u);
    QuickSort(values, 0, 63);
    return SortedAscending(values, 64);
}

/* SortByKey early-outs on an ordered run; the arrays must come back
   untouched. */
static unsigned char CheckSortByKeyOrdered()
{
    unsigned short items[6] = {10, 20, 30, 40, 50, 60};
    unsigned long keys[6] = {1, 2, 3, 4, 5, 6};
    SortByKey(items, keys, 6);
    for (int index = 0; index < 6; ++index) {
        if (items[index] != static_cast<unsigned short>((index + 1) * 10)) {
            return 0;
        }
    }
    return SortedAscending(keys, 6);
}

/* A strictly descending run hits the reverse-in-place fast path. */
static unsigned char CheckSortByKeyReverse()
{
    unsigned short items[8] = {8, 7, 6, 5, 4, 3, 2, 1};
    unsigned long keys[8] = {80, 70, 60, 50, 40, 30, 20, 10};
    SortByKey(items, keys, 8);
    for (int index = 0; index < 8; ++index) {
        if (keys[index] != static_cast<unsigned long>(index + 1) * 10 ||
            items[index] != static_cast<unsigned short>(index + 1)) {
            return 0;
        }
    }
    return 1;
}

/* Degenerate sizes: SortByKey early-outs below two elements and the direct
   sorts must not touch or read outside a one-element range. */
static unsigned char CheckSortEdgeCases()
{
    unsigned short item = 42;
    unsigned long key = 7;
    unsigned long value = 9;
    SortByKey(&item, &key, 0);
    SortByKey(&item, &key, 1);
    QuickSortByKey(&item, &key, 0, -1);
    QuickSortByKey(&item, &key, 0, 0);
    QuickSort(&value, 0, 0);
    InsertionSort(&value, 0, 0);
    return item == 42 && key == 7 && value == 9;
}

/* Duplicate keys: items must stay attached to the key they entered with.
   Exercises the unsigned short item instantiation directly. */
static unsigned char CheckSortDuplicatePairs()
{
    unsigned long original_keys[5] = {5, 2, 5, 2, 9};
    unsigned long keys[5] = {5, 2, 5, 2, 9};
    unsigned short items[5] = {0, 1, 2, 3, 4};
    QuickSortByKey(items, keys, 0, 4);
    if (!SortedAscending(keys, 5)) {
        return 0;
    }
    for (int index = 0; index < 5; ++index) {
        if (original_keys[items[index]] != keys[index]) {
            return 0;
        }
    }
    return 1;
}

/* The unsigned long item instantiation: scrambled keys, item = original
   position, so original_keys[item] == key verifies pairing after any number
   of swaps. */
static unsigned char CheckSortUnsignedLongPairs()
{
    unsigned long original_keys[16];
    unsigned long keys[16];
    unsigned long items[16];
    FillScrambled(original_keys, 16, 0x46a0u);
    int index;
    for (index = 0; index < 16; ++index) {
        keys[index] = original_keys[index];
        items[index] = static_cast<unsigned long>(index);
    }
    QuickSortByKey(items, keys, 0, 15);
    if (!SortedAscending(keys, 16)) {
        return 0;
    }
    for (index = 0; index < 16; ++index) {
        if (original_keys[items[index]] != keys[index]) {
            return 0;
        }
    }
    return 1;
}

/* The srVector3i item instantiation used to order path nodes: 12-byte
   records must move whole with their keys. */
static unsigned char CheckSortVectorPairs()
{
    unsigned long original_keys[10];
    unsigned long keys[10];
    srVector3i items[10];
    FillScrambled(original_keys, 10, 0x6ca0u);
    int index;
    for (index = 0; index < 10; ++index) {
        keys[index] = original_keys[index];
        items[index].x = index;
        items[index].y = index * 3;
        items[index].z = -index;
    }
    QuickSortByKey(items, keys, 0, 9);
    if (!SortedAscending(keys, 10)) {
        return 0;
    }
    for (index = 0; index < 10; ++index) {
        if (original_keys[items[index].x] != keys[index] || items[index].y != items[index].x * 3 ||
            items[index].z != -items[index].x) {
            return 0;
        }
    }
    return 1;
}

/* 64 entries with all but two pairs ordered: SortByKey takes the
   QuickSortByKey path (ordered_pairs >= 50) and the items must track their
   keys. */
static unsigned char CheckSortByKeyMixed()
{
    unsigned short items[64];
    unsigned long keys[64];
    int index;
    for (index = 0; index < 64; ++index) {
        keys[index] = static_cast<unsigned long>(index) * 4;
        items[index] = static_cast<unsigned short>(index);
    }
    keys[10] = 900;
    keys[40] = 3;
    SortByKey(items, keys, 64);
    if (!SortedAscending(keys, 64)) {
        return 0;
    }
    for (index = 0; index < 64; ++index) {
        unsigned long expected;
        if (keys[index] == 3) {
            expected = 40;
        } else if (keys[index] == 900) {
            expected = 10;
        } else {
            expected = keys[index] / 4;
        }
        if (items[index] != static_cast<unsigned short>(expected)) {
            return 0;
        }
    }
    return 1;
}

/* The sibling files CheckLevelAssetSet wants beside a ready .oct: LVL and
   WGD prove the level set exists, PVL is the timestamp reference. Writing
   them before the .oct makes the oct newest, which selects the direct-read
   path (return 0). */
static void WriteEmptySibling(const char* name)
{
    HWFILE file = FileOpen(const_cast<char*>(name), FILE_ACCESS_WRITE | FILE_CREATE_ALWAYS, 0);
    if (file != 0) {
        FileClose(file);
    }
}

/* Byte-exact checks against the serialized layout: header fields at their
   retail-defined offsets, the 0xffffffff section sentinels, and the total
   file size. */
static unsigned char CheckFileLayout(const OctPreTree* tree, const W8OctPreTreeGeometry* geometry)
{
    FILE* file = fopen("NewLevel.oct", "rb");
    if (file == 0) {
        return 0;
    }
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    unsigned char* bytes = static_cast<unsigned char*>(malloc(size));
    if (bytes == 0 || fread(bytes, 1, size, file) != static_cast<size_t>(size)) {
        fclose(file);
        free(bytes);
        return 0;
    }
    fclose(file);

    unsigned char ok = 1;
    const W8OctSpatialState* spatial = &tree->spatial_000;

    if (LoadWord(bytes, 0x00) != 0x22 || LoadFloat(bytes, 0x02) != spatial->extent_04 ||
        LoadFloat(bytes, 0x06) != spatial->cell_size_08 ||
        LoadFloat(bytes, 0x0a) != spatial->node_extent_70 ||
        LoadFloat(bytes, 0x0e) != spatial->minimum_0c.x ||
        LoadFloat(bytes, 0x1a) != spatial->maximum_18.x ||
        LoadFloat(bytes, 0x26) != spatial->clipped_minimum_24.x ||
        LoadFloat(bytes, 0x32) != spatial->clipped_maximum_30.x ||
        LoadFloat(bytes, 0x3e) != spatial->working_minimum_78.x ||
        LoadFloat(bytes, 0x4a) != spatial->working_maximum_84.x ||
        LoadDword(bytes, 0x56) != tree->m_leaf_grid_dim_x_0a4 ||
        LoadDword(bytes, 0x5a) != tree->m_leaf_grid_dim_y_0a8 ||
        LoadDword(bytes, 0x5e) != tree->m_leaf_grid_dim_z_0ac ||
        LoadWord(bytes, 0x62) != spatial->depth_44 ||
        LoadWord(bytes, 0x64) != spatial->region_id_bound_58 ||
        LoadDword(bytes, 0x66) != spatial->submesh_count_74 ||
        LoadDword(bytes, 0x6a) != tree->m_branch_count_0b4 ||
        LoadDword(bytes, 0x6e) != tree->m_leaf_count_0b8 ||
        LoadDword(bytes, 0x72) != geometry->polygon_count_08 ||
        LoadDword(bytes, 0x76) != geometry->vertex_count_00 || LoadDword(bytes, 0x7a) != 0 ||
        LoadDword(bytes, 0x7e) != 0 || LoadDword(bytes, 0x82) != tree->polygon_cursor_3a0 ||
        LoadDword(bytes, 0x86) != tree->m_gd_surface_stream_len_124 ||
        LoadDword(bytes, 0x8a) != 0 || LoadDword(bytes, 0x8e) != 0 ||
        LoadDword(bytes, 0x92) != tree->m_region_list_len_138 ||
        LoadWord(bytes, 0x96) != spatial->region_count_46 ||
        LoadWord(bytes, 0x98) != spatial->leaf_level_52 ||
        LoadDword(bytes, 0x9a) != tree->m_root_mesh_count_1a8 ||
        LoadDword(bytes, 0x9e) != tree->m_meshCount_1b4 ||
        LoadDword(bytes, 0xa2) != tree->m_kind1_submesh_count_1ac ||
        LoadFloat(bytes, 0xa6) != spatial->region_grid_cell_54 ||
        LoadFloat(bytes, 0xac) != tree->m_region_cell_178 || LoadDword(bytes, 0xb0) != 0 ||
        bytes[0xb8] != 0 || LoadFloat(bytes, 0xb9) != spatial->max_region_radius_60 ||
        LoadDword(bytes, 0xbd) != 0 || LoadDword(bytes, 0xc1) != 0 || LoadWord(bytes, 0xc5) != 0 ||
        LoadWord(bytes, 0xc7) != 0) {
        ok = 0;
    }

    /* Every section separator is the same 0xffffffff dword. */
    unsigned long cursor = 0xf5;
    if (LoadDword(bytes, cursor) != 0xffffffff) {
        ok = 0;
    }
    cursor += 4;
    cursor += tree->m_branch_count_0b4 * 0x24;
    cursor += tree->m_leaf_count_0b8 * 0x28;
    cursor += tree->polygon_cursor_3a0 * 4;
    cursor +=
        tree->m_leaf_grid_dim_x_0a4 * tree->m_leaf_grid_dim_y_0a8 * tree->m_leaf_grid_dim_z_0ac * 4;
    cursor += geometry->polygon_count_08 * 4;
    cursor += tree->m_region_list_len_138 * 2;
    cursor += tree->m_gd_surface_stream_len_124 * 4;
    /* Trigger list skipped (count 0); region array skipped (count <= 1). */
    if (LoadDword(bytes, cursor) != 0xffffffff) {
        ok = 0;
    }
    cursor += 4;
    cursor += (spatial->submesh_count_74 + 1) * 0x10;
    /* Alpha bits, particle and prop lookup tables skipped (counts 0). */
    if (LoadDword(bytes, cursor) != 0xffffffff) {
        ok = 0;
    }
    cursor += 4;
    /* Path nodes and prop sun bits skipped. */
    if (LoadDword(bytes, cursor) != 0xffffffff) {
        ok = 0;
    }
    cursor += 4;
    /* Game-data block: 0x68-byte header, no banks, one 0x44 environ record. */
    if (LoadDword(bytes, cursor) != 1) {
        ok = 0;
    }
    cursor += 0x68 + 0x44;
    if (LoadDword(bytes, cursor) != 0xffffffff) {
        ok = 0;
    }
    cursor += 4;
    if (cursor != static_cast<unsigned long>(size)) {
        ok = 0;
    }
    free(bytes);
    return ok;
}

/* Compares the serialized header fields that ReadOctFile restores onto the
   loaded octree. Two fields do not round-trip by name:
   +0x46 loads from the high word of grid dim z (header 0x60) and +0x52 loads
   from depth (header 0x62) - the asymmetry is retail's, so the check asserts
   exactly what the reader produces. */
static unsigned char CheckLoadedSpatial(const OctPreTree* written, const W8Octree* loaded)
{
    const W8OctSpatialState* source = &written->spatial_000;
    const W8OctSpatialState* back = &loaded->spatial_000;
    return back->extent_04 == source->extent_04 && back->cell_size_08 == source->cell_size_08 &&
           back->node_extent_70 == source->node_extent_70 &&
           back->minimum_0c.x == source->minimum_0c.x &&
           back->maximum_18.z == source->maximum_18.z &&
           back->clipped_minimum_24.y == source->clipped_minimum_24.y &&
           back->working_maximum_84.z == source->working_maximum_84.z &&
           back->depth_44 == source->depth_44 &&
           back->region_count_46 ==
               static_cast<unsigned short>(written->m_leaf_grid_dim_z_0ac >> 16) &&
           back->leaf_level_52 == source->depth_44 &&
           back->region_id_bound_58 == source->region_id_bound_58 &&
           loaded->m_leaf_grid_dim_x_0a4 == written->m_leaf_grid_dim_x_0a4 &&
           loaded->m_leaf_grid_dim_y_0a8 == written->m_leaf_grid_dim_y_0a8 &&
           loaded->m_leaf_grid_dim_z_0ac == written->m_leaf_grid_dim_z_0ac &&
           loaded->m_leaf_polygon_stream_len_0cc == written->polygon_cursor_3a0 &&
           loaded->m_gd_surface_stream_len_124 == written->m_gd_surface_stream_len_124 &&
           loaded->m_region_cell_178 == written->m_region_cell_178;
}

/* One vertex shared by two polygons with a different corner uv must grow the
   pool: the seam walk may only reuse an exact u/v match, and the end of a
   link chain allocates a fresh entry.  Polygon 0 maps its corners straight
   onto the vertex defaults; polygon 1 reuses vertex 0 with a different uv
   (seam -> new entry), vertex 2 with an identical uv (exact-match reuse) and
   vertex 3 (fresh default). */
static unsigned char CheckUvSeam(OctPreTree* tree)
{
    W8OctSubmeshBuild record;
    W8OctRegionPolygon polygons[2];
    srVector3i poly_vertices[2];
    int polygon_ids[2];
    W8OctPreTreeGeometry geometry;

    memset(&record, 0, sizeof(record));
    memset(polygons, 0, sizeof(polygons));
    record.vertex_count_14 = 4;
    record.polygon_count_1c = 2;
    poly_vertices[0].x = 0;
    poly_vertices[0].y = 1;
    poly_vertices[0].z = 2;
    poly_vertices[1].x = 0;
    poly_vertices[1].y = 2;
    poly_vertices[1].z = 3;
    record.poly_vertices_28 = poly_vertices;
    polygon_ids[0] = 0;
    polygon_ids[1] = 1;
    record.polygon_ids_24 = polygon_ids;
    polygons[0].face_48.texture_coordinates[0].Set(0.1f, 0.2f);
    polygons[0].face_48.texture_coordinates[1].Set(0.3f, 0.4f);
    polygons[0].face_48.texture_coordinates[2].Set(0.5f, 0.6f);
    polygons[1].face_48.texture_coordinates[0].Set(0.7f, 0.8f);
    polygons[1].face_48.texture_coordinates[1].Set(0.5f, 0.6f);
    polygons[1].face_48.texture_coordinates[2].Set(0.9f, 1.0f);
    memset(&geometry, 0, sizeof(geometry));
    geometry.polygons_0c = polygons;

    if (tree->SplitUVMaps0046A4B0(&record, &geometry) != 5 || record.poly_uv_index_2c == 0 ||
        record.uv_map_30 == 0) {
        return 0;
    }
    return record.poly_uv_index_2c[0].x == 0 && record.poly_uv_index_2c[0].y == 1 &&
           record.poly_uv_index_2c[0].z == 2 && record.poly_uv_index_2c[1].x == 4 &&
           record.poly_uv_index_2c[1].y == 2 && record.poly_uv_index_2c[1].z == 3 &&
           record.uv_map_30[4].x == 0.7f && record.uv_map_30[4].y == 0.8f;
}

/* InsertConditionalNodes packs (stop-prop number << 16) | (preprop index + 1)
   keys into the caller's hash and tags blocked-path records' value word with
   0x2000000.  Two preprop records with disjoint prop-number windows exercise
   both the support and the block scans: prop 5 lands in window [0,10) at stop
   index 5, prop 12 in window [10,18) at stop index 2. */
static unsigned char CheckCondNodes(OctPreTree* tree)
{
    W8PreProp preprops[2];
    W8HashTable<unsigned int, CondPathNode*> nodes;

    memset(preprops, 0, sizeof(preprops));
    preprops[0].num_stop_meshes_40 = 10;
    preprops[0].first_prop_number_42 = 0;
    /* GDPreProp's zeroing ctor is unrecovered in this image; zeroed storage
       reproduces it for the one field the body reads. */
    preprops[0].pStopMeshes = static_cast<GDPreProp*>(calloc(10, sizeof(GDPreProp)));
    preprops[1].num_stop_meshes_40 = 8;
    preprops[1].first_prop_number_42 = 10;
    preprops[1].pStopMeshes = static_cast<GDPreProp*>(calloc(8, sizeof(GDPreProp)));
    if (preprops[0].pStopMeshes == 0 || preprops[1].pStopMeshes == 0) {
        return 0;
    }
    /* The private GDProp prop number sits at +0x02; OctPreTree is the
       declared friend, the harness is not. */
    *reinterpret_cast<unsigned short*>(reinterpret_cast<char*>(preprops[0].pStopMeshes + 5) +
                                       0x02) = 0x40; /* reinterpret-ok: pokes the private
        m_prop_number_02 field at its retail offset for the friend reader */
    *reinterpret_cast<unsigned short*>(reinterpret_cast<char*>(preprops[1].pStopMeshes + 2) +
                                       0x02) = 0x80; /* reinterpret-ok: same private-field
        poke at +0x02 */

    tree->m_lNumSupports_2a8 = 1;
    tree->m_lSupports_2b0[0] = 5;
    tree->m_lNumBlocks_2ac = 1;
    tree->m_lBlocks_328[0] = 12;
    if (tree->InsertConditionalNodes0046B9D0(&nodes, 0x1234, 0x56, preprops, 2) != 1) {
        return 0;
    }
    /* The malloc'd CondPathNode payloads and the stop-mesh arrays leak with
       the rest of the scenario. */
    unsigned int support_key = (0x40u << 16) | 1u;
    unsigned int block_key = (0x80u << 16) | 2u;
    const CondPathNode* support = nodes.Lookup(&support_key);
    const CondPathNode* block = nodes.Lookup(&block_key);
    return support != 0 && support->value == 0x56 && support->cell == 0x1234 && block != 0 &&
           block->value == (0x56u | 0x2000000u) && block->cell == 0x1234;
}

/* The PrePathing chunk allocator hands out consecutive 0x10-byte records
   from its 0x3e80-byte (1000-record) chunks, zero-initialised, counting
   chunk_node_count_11fc per chunk. */
static unsigned char CheckPathNodeChunking()
{
    PrePathing* prepath = new PrePathing();
    if (prepath == 0) {
        return 0;
    }
    W8PrePathNode* first = prepath->GetPathNode();
    W8PrePathNode* second = prepath->GetPathNode();
    /* Deliberately leaked: ~W8PathingService frees g_path_scratch_00659c64 and
       clears g_pathing_00659c60 - globals this object does not own. */
    return first == prepath->node_chunks_258[0] && second == first + 1 && first->level_flags == 0 &&
           first->cell == 0 && prepath->chunk_index_11f8 == 0 &&
           prepath->chunk_node_count_11fc == 2;
}

/* Regression coverage for the BuildPathLists node store `record->y = node.y`:
   the +0x08 field carries the raw IEEE bits, so a non-integral negative
   height must survive the assignment bit-identical - an integer-typed field
   or a truncating conversion drops them. */
static unsigned char CheckPathNodeYBits()
{
    const float heights[2] = {-37.5f, -1.0e10f};
    PrePathing* prepath = new PrePathing();
    if (prepath == 0) {
        return 0;
    }
    /* Deliberately leaked like CheckPathNodeChunking's instance. */
    for (int index = 0; index < 2; ++index) {
        W8PrePathNode* record = prepath->GetPathNode();
        unsigned int expected;
        unsigned int stored;
        record->y = heights[index];
        memcpy(&expected, &heights[index], sizeof(expected));
        memcpy(&stored, &record->y, sizeof(stored));
        if (stored != expected) {
            return 0;
        }
    }
    return 1;
}

static void RunOctFileRoundTrip(OctFileSemanticResult* result)
{
    OctPreTree* saved_pre_tree = g_oct_pre_tree_659c74;
    W8Octree* saved_octree = g_octree_6598a4;
    W8GameData* saved_game_data = g_octree_game_data_00652db0;
    W8EnvironRecord* saved_environ = g_environ_00652DB4;
    W8OctPreTreeGeometry geometry;
    W8GameData* loaded_data = 0;
    W8Octree* loaded;
    int index;

    WriteEmptySibling("NewLevel.LVL");
    WriteEmptySibling("NewLevel.WGD");
    WriteEmptySibling("NewLevel.PVL");

    OctPreTree* tree = new OctPreTree();
    W8GameData* game_data = new W8GameData(0, false);
    if (tree == 0 || game_data == 0) {
        goto restore;
    }
    result->uv_seam_ok = CheckUvSeam(tree);
    result->cond_nodes_ok = CheckCondNodes(tree);

    tree->spatial_000.extent_04 = 64.0f;
    tree->spatial_000.cell_size_08 = 4.0f;
    tree->spatial_000.node_extent_70 = 128.0f;
    tree->spatial_000.minimum_0c.Set(8.0f, 16.0f, 24.0f);
    tree->spatial_000.maximum_18.Set(72.0f, 40.0f, 88.0f);
    tree->spatial_000.clipped_minimum_24.Set(12.0f, 20.0f, 28.0f);
    tree->spatial_000.clipped_maximum_30.Set(68.0f, 36.0f, 84.0f);
    tree->spatial_000.working_minimum_78.Set(4.0f, 8.0f, 12.0f);
    tree->spatial_000.working_maximum_84.Set(76.0f, 44.0f, 92.0f);
    tree->spatial_000.depth_44 = 3;
    tree->spatial_000.region_id_bound_58 = 7;
    tree->spatial_000.region_count_46 = 1;
    tree->spatial_000.leaf_level_52 = 5;
    tree->spatial_000.region_grid_cell_54 = 1.5f;
    tree->spatial_000.max_region_radius_60 = 2.5f;
    /* One submesh entry exercises the mesh block and the reader's
       max-scan/allocation of the region-index store. */
    tree->spatial_000.submesh_count_74 = 1;
    tree->m_leaf_grid_dim_x_0a4 = 2;
    tree->m_leaf_grid_dim_y_0a8 = 2;
    tree->m_leaf_grid_dim_z_0ac = 2;
    tree->m_branch_count_0b4 = 1;
    tree->m_leaf_count_0b8 = 1;
    tree->polygon_cursor_3a0 = 1;
    tree->m_gd_surface_stream_len_124 = 1;
    tree->m_region_list_len_138 = 1;
    tree->m_root_mesh_count_1a8 = 4;
    tree->m_kind1_submesh_count_1ac = 3;
    tree->m_region_cell_178 = 8.0f;

    tree->m_owned_09c = static_cast<W8OctPreTreeBranch*>(malloc(0x24));
    tree->m_owned_0a0 = static_cast<W8OctPreTreeLeaf*>(malloc(0x28));
    tree->m_owned_0b0 = static_cast<unsigned long*>(malloc(8 * 4));
    tree->m_owned_0d0 = static_cast<unsigned long*>(malloc(4));
    tree->m_aulPolyLookup = static_cast<unsigned long*>(malloc(4));
    tree->m_owned_12c = static_cast<unsigned long*>(malloc(4));
    tree->m_owned_148 = static_cast<unsigned short*>(malloc(2));
    tree->m_pSubmeshes = static_cast<W8OctSubmesh*>(malloc(2 * 0x10));
    if (tree->m_owned_09c == 0 || tree->m_owned_0a0 == 0 || tree->m_owned_0b0 == 0 ||
        tree->m_owned_0d0 == 0 || tree->m_aulPolyLookup == 0 || tree->m_owned_12c == 0 ||
        tree->m_owned_148 == 0 || tree->m_pSubmeshes == 0) {
        goto restore;
    }
    memset(tree->m_owned_09c, 0xa5, 0x24);
    memset(tree->m_owned_0a0, 0x5a, 0x28);
    /* The writer's leaf fixup strips bit 0 before serializing: 3 becomes 2
       in the file and after the round-trip. */
    tree->m_owned_0a0[0].flags_00 = 3;
    for (index = 0; index < 8; ++index) {
        tree->m_owned_0b0[index] = 0x1000 + index * 0x11;
    }
    tree->m_owned_0d0[0] = 0x11223344;
    tree->m_aulPolyLookup[0] = 0xaabbccdd;
    tree->m_owned_12c[0] = 0xdeadbeef;
    tree->m_owned_148[0] = 0x5a5a;
    memset(tree->m_pSubmeshes, 0, 2 * 0x10);
    tree->m_pSubmeshes[1].polygon_count_0c = 5;

    memset(&geometry, 0, sizeof(geometry));
    geometry.vertex_count_00 = 3;
    geometry.polygon_count_08 = 1;

    result->write_ok = tree->WriteOctFile004683F0(&geometry, game_data);
    if (result->write_ok == 0) {
        goto restore;
    }
    result->header_fields_ok = CheckFileLayout(tree, &geometry);
    result->sentinel_layout_ok = result->header_fields_ok;
    result->file_size_matches = result->header_fields_ok;

    loaded = new W8Octree("NewLevel.oct", &loaded_data);
    result->load_ok = loaded != 0 && loaded_data != 0 && g_octree_6598a4 == loaded &&
                      (loaded->spatial_000.flags_00 & 0x80000000) == 0;
    if (result->load_ok != 0) {
        result->spatial_roundtrip = CheckLoadedSpatial(tree, loaded);
        result->leaf_flag_cleared = loaded->m_owned_0a0[0].flags_00 == 2;
        result->tables_roundtrip =
            loaded->m_branch_count_0b4 == 1 && loaded->m_leaf_count_0b8 == 1 &&
            loaded->m_owned_0d0[0] == 0x11223344 && loaded->m_aulPolyLookup[0] == 0xaabbccdd &&
            loaded->m_owned_12c[0] == 0xdeadbeef && loaded->m_owned_148[0] == 0x5a5a &&
            loaded->m_pSubmeshes[1].polygon_count_0c == 5 &&
            memcmp(loaded->m_owned_0b0, tree->m_owned_0b0, 8 * 4) == 0;
        result->gamedata_roundtrip =
            loaded_data->m_iNumEnvirons == 1 && loaded_data->m_ppEnvirons != 0 &&
            loaded_data->m_ppEnvirons[0] != 0 && loaded_data->m_ppEnvirons[0]->value_1c == 0.05f;
    }

restore:
    /* Everything allocated above is deliberately left for process teardown:
       ~W8GameData and parts of ~W8Octree route through unrecovered stubs in
       this image. The reader's W8GameData ctor deleted the writer-side
       object's environ record through g_environ_00652DB4 and republished the
       globals, so all four are restored here. */
    g_oct_pre_tree_659c74 = saved_pre_tree;
    g_octree_6598a4 = saved_octree;
    g_octree_game_data_00652db0 = saved_game_data;
    g_environ_00652DB4 = saved_environ;
}

bool RunOctFileSemanticTests(OctFileSemanticResult* result)
{
    memset(result, 0, sizeof(*result));

    result->sort_insertion_ok = CheckInsertionSort();
    result->sort_partition_ok = CheckQuickSortPartition();
    result->sort_by_key_ordered_ok = CheckSortByKeyOrdered();
    result->sort_by_key_reverse_ok = CheckSortByKeyReverse();
    result->sort_by_key_mixed_ok = CheckSortByKeyMixed();
    result->sort_edge_ok = CheckSortEdgeCases();
    result->sort_dup_pair_ok = CheckSortDuplicatePairs();
    result->sort_ulong_pair_ok = CheckSortUnsignedLongPairs();
    result->sort_vec3_pair_ok = CheckSortVectorPairs();
    result->path_node_chunk_ok = CheckPathNodeChunking();
    result->y_bits_ok = CheckPathNodeYBits();

    /* The W8GameData constructor deletes g_environ_00652DB4 and
       g_level_data_00652dac when they are set, so the scenario requires a
       state where no level is loaded - which the main menu provides. */
    if (g_octree_disabled_6598a8 == 0 && g_level_data_00652dac == 0 && g_environ_00652DB4 == 0) {
        result->octree_io_enabled = 1;
        RunOctFileRoundTrip(result);
    }
    return result->octree_io_enabled && result->write_ok && result->file_size_matches &&
           result->sentinel_layout_ok && result->header_fields_ok && result->load_ok &&
           result->spatial_roundtrip && result->tables_roundtrip && result->leaf_flag_cleared &&
           result->gamedata_roundtrip && result->sort_insertion_ok && result->sort_partition_ok &&
           result->sort_by_key_ordered_ok && result->sort_by_key_reverse_ok &&
           result->sort_by_key_mixed_ok && result->sort_edge_ok && result->sort_dup_pair_ok &&
           result->sort_ulong_pair_ok && result->sort_vec3_pair_ok && result->uv_seam_ok &&
           result->path_node_chunk_ok && result->cond_nodes_ok && result->y_bits_ok;
}

void PrintOctFileSemanticResults(const OctFileSemanticResult* result)
{
    printf("oct-file semantic: io_enabled=%u write=%u size=%u "
           "sentinels=%u header=%u load=%u spatial=%u tables=%u leaf_flag=%u "
           "gamedata=%u sort_insert=%u sort_partition=%u sortkey_ordered=%u "
           "sortkey_reverse=%u sortkey_mixed=%u sort_edge=%u sort_dup=%u sort_ulong=%u "
           "sort_vec3=%u uv_seam=%u prepath_chunk=%u cond_nodes=%u y_bits=%u\n",
           result->octree_io_enabled, result->write_ok, result->file_size_matches,
           result->sentinel_layout_ok, result->header_fields_ok, result->load_ok,
           result->spatial_roundtrip, result->tables_roundtrip, result->leaf_flag_cleared,
           result->gamedata_roundtrip, result->sort_insertion_ok, result->sort_partition_ok,
           result->sort_by_key_ordered_ok, result->sort_by_key_reverse_ok,
           result->sort_by_key_mixed_ok, result->sort_edge_ok, result->sort_dup_pair_ok,
           result->sort_ulong_pair_ok, result->sort_vec3_pair_ok, result->uv_seam_ok,
           result->path_node_chunk_ok, result->cond_nodes_ok, result->y_bits_ok);
    fflush(stdout);
}
