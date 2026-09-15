#ifndef WIZ8_TESTS_RUNTIME_OCT_FILE_SEMANTIC_TEST_H
#define WIZ8_TESTS_RUNTIME_OCT_FILE_SEMANTIC_TEST_H

/* In-process semantic scenario for the OctPreTree cluster: writes a minimal
   NewLevel.oct through WriteOctFile, verifies the serialized layout
   byte-for-byte where retail defines it, reads the file back through the
   recovered W8Octree(path) reader, and exercises the shared sort helpers the
   build path uses. */

struct OctFileSemanticResult {
    unsigned char octree_io_enabled;
    unsigned char write_ok;
    unsigned char file_size_matches;
    unsigned char sentinel_layout_ok;
    unsigned char header_fields_ok;
    unsigned char load_ok;
    unsigned char spatial_roundtrip;
    unsigned char tables_roundtrip;
    unsigned char leaf_flag_cleared;
    unsigned char gamedata_roundtrip;
    unsigned char sort_insertion_ok;
    unsigned char sort_partition_ok;
    unsigned char sort_by_key_ordered_ok;
    unsigned char sort_by_key_reverse_ok;
    unsigned char sort_by_key_mixed_ok;
    unsigned char sort_edge_ok;
    unsigned char sort_dup_pair_ok;
    unsigned char sort_ulong_pair_ok;
    unsigned char sort_vec3_pair_ok;
    unsigned char uv_seam_ok;
    unsigned char path_node_chunk_ok;
    unsigned char cond_nodes_ok;
    unsigned char y_bits_ok;
};

bool RunOctFileSemanticTests(OctFileSemanticResult* result);
void PrintOctFileSemanticResults(const OctFileSemanticResult* result);

#endif
