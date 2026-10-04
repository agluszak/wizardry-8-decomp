/* Layout evidence for include/wiz8/engine_code/OctPreTree.h. Included after its declarations. */

static_assert(sizeof(W8OctSpatialState) == 0x9c, "W8OctSpatialState_must_be_0x9c");
static_assert(sizeof(W8OctRegionVolume) == 0xe8, "W8OctRegionVolume_must_be_0xe8");
static_assert(sizeof(W8OctPreTreeVertex) == 0x60, "W8OctPreTreeVertex_must_be_0x60");
static_assert(sizeof(W8OctSubmeshBuild) == 0x34, "W8OctSubmeshBuild_must_be_0x34");
static_assert(offsetof(W8OctSubmeshBuild, flags_00) == 0x00, "W8OctSubmeshBuild_flags_00");
static_assert(offsetof(W8OctSubmeshBuild, index_04) == 0x04, "W8OctSubmeshBuild_index_04");
static_assert(offsetof(W8OctSubmeshBuild, m_kind) == 0x08, "W8OctSubmeshBuild_kind_08");
static_assert(offsetof(W8OctSubmeshBuild, m_prev_link) == 0x0c, "W8OctSubmeshBuild_prev_link_0c");
static_assert(offsetof(W8OctSubmeshBuild, m_next_link) == 0x10, "W8OctSubmeshBuild_next_link_10");
static_assert(offsetof(W8OctSubmeshBuild, vertex_count) == 0x14,
              "W8OctSubmeshBuild_vertex_count");
static_assert(offsetof(W8OctSubmeshBuild, m_map_count) == 0x18, "W8OctSubmeshBuild_map_count_18");
static_assert(offsetof(W8OctSubmeshBuild, m_polygon_count) == 0x1c,
              "W8OctSubmeshBuild_polygon_count_1c");
static_assert(offsetof(W8OctSubmeshBuild, m_vertex_ids) == 0x20, "W8OctSubmeshBuild_vertex_ids_20");
static_assert(offsetof(W8OctSubmeshBuild, m_polygon_ids) == 0x24,
              "W8OctSubmeshBuild_polygon_ids_24");
static_assert(offsetof(W8OctSubmeshBuild, m_poly_vertices) == 0x28,
              "W8OctSubmeshBuild_poly_vertices_28");
static_assert(offsetof(W8OctSubmeshBuild, m_poly_uv_index) == 0x2c,
              "W8OctSubmeshBuild_poly_uv_index_2c");
static_assert(offsetof(W8OctSubmeshBuild, m_uv_map) == 0x30, "W8OctSubmeshBuild_uv_map_30");
static_assert(sizeof(W8OctFileHeader) == 0xf5, "W8OctFileHeader_must_be_0xf5");
static_assert(offsetof(W8OctFileHeader, version) == 0x00, "W8OctFileHeader_version");
static_assert(offsetof(W8OctFileHeader, m_extent) == 0x02, "W8OctFileHeader_extent_02");
static_assert(offsetof(W8OctFileHeader, m_cell_size) == 0x06, "W8OctFileHeader_cell_size_06");
static_assert(offsetof(W8OctFileHeader, m_node_extent) == 0x0a, "W8OctFileHeader_node_extent_0a");
static_assert(offsetof(W8OctFileHeader, m_bounds) == 0x0e, "W8OctFileHeader_bounds_0e");
static_assert(offsetof(W8OctFileHeader, m_grid_dims) == 0x56, "W8OctFileHeader_grid_dims_56");
static_assert(offsetof(W8OctFileHeader, m_depth) == 0x62, "W8OctFileHeader_depth_62");
static_assert(offsetof(W8OctFileHeader, m_region_id_bound) == 0x64,
              "W8OctFileHeader_region_id_bound_64");
static_assert(offsetof(W8OctFileHeader, m_submesh_count) == 0x66,
              "W8OctFileHeader_submesh_count_66");
static_assert(offsetof(W8OctFileHeader, m_branch_count) == 0x6a, "W8OctFileHeader_branch_count_6a");
static_assert(offsetof(W8OctFileHeader, m_leaf_count) == 0x6e, "W8OctFileHeader_leaf_count_6e");
static_assert(offsetof(W8OctFileHeader, m_polygon_count) == 0x72,
              "W8OctFileHeader_polygon_count_72");
static_assert(offsetof(W8OctFileHeader, m_vertex_count) == 0x76, "W8OctFileHeader_vertex_count_76");
static_assert(offsetof(W8OctFileHeader, m_surface_count) == 0x7a,
              "W8OctFileHeader_surface_count_7a");
static_assert(offsetof(W8OctFileHeader, m_path_nodes) == 0x7e, "W8OctFileHeader_path_nodes_7e");
static_assert(offsetof(W8OctFileHeader, m_leaf_polygon_stream_len) == 0x82,
              "W8OctFileHeader_leaf_polygon_stream_len_82");
static_assert(offsetof(W8OctFileHeader, m_gd_surface_stream_len) == 0x86,
              "W8OctFileHeader_gd_surface_stream_len_86");
static_assert(offsetof(W8OctFileHeader, m_trigger_count) == 0x8a,
              "W8OctFileHeader_trigger_count_8a");
static_assert(offsetof(W8OctFileHeader, m_zero) == 0x8e, "W8OctFileHeader_zero_8e");
static_assert(offsetof(W8OctFileHeader, m_region_list_len) == 0x92,
              "W8OctFileHeader_region_list_len_92");
static_assert(offsetof(W8OctFileHeader, m_region_count) == 0x96, "W8OctFileHeader_region_count_96");
static_assert(offsetof(W8OctFileHeader, m_leaf_level) == 0x98, "W8OctFileHeader_leaf_level_98");
static_assert(offsetof(W8OctFileHeader, m_root_mesh_count) == 0x9a,
              "W8OctFileHeader_root_mesh_count_9a");
static_assert(offsetof(W8OctFileHeader, m_mesh_total) == 0x9e, "W8OctFileHeader_mesh_total_9e");
static_assert(offsetof(W8OctFileHeader, m_kind1_submesh_count) == 0xa2,
              "W8OctFileHeader_kind1_submesh_count_a2");
static_assert(offsetof(W8OctFileHeader, m_region_grid_cell) == 0xa6,
              "W8OctFileHeader_region_grid_cell_a6");
static_assert(offsetof(W8OctFileHeader, pad_aa) == 0xaa, "W8OctFileHeader_pad_aa");
static_assert(offsetof(W8OctFileHeader, m_region_cell) == 0xac, "W8OctFileHeader_region_cell_ac");
static_assert(offsetof(W8OctFileHeader, m_edge_node_count) == 0xb0,
              "W8OctFileHeader_edge_node_count_b0");
static_assert(offsetof(W8OctFileHeader, m_path_clearance) == 0xb4,
              "W8OctFileHeader_path_clearance_b4");
static_assert(offsetof(W8OctFileHeader, m_prop_sun_bits) == 0xb8,
              "W8OctFileHeader_prop_sun_bits_b8");
static_assert(offsetof(W8OctFileHeader, m_max_region_radius) == 0xb9,
              "W8OctFileHeader_max_region_radius_b9");
static_assert(offsetof(W8OctFileHeader, m_prop_count) == 0xbd, "W8OctFileHeader_prop_count_bd");
static_assert(offsetof(W8OctFileHeader, m_particle_count) == 0xc1,
              "W8OctFileHeader_particle_count_c1");
static_assert(offsetof(W8OctFileHeader, m_particle_len) == 0xc5, "W8OctFileHeader_particle_len_c5");
static_assert(offsetof(W8OctFileHeader, m_prop_len) == 0xc7, "W8OctFileHeader_prop_len_c7");
static_assert(offsetof(W8OctFileHeader, pad_c9) == 0xc9, "W8OctFileHeader_pad_c9");
