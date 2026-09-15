#include "wiz8/engine_code/OctPath.h"
#include "wiz8/engine_code/GDProp.h"
#include "wiz8/engine_code/LevelFile.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/materials.h"
#include "wiz8/float_constants.h"
#include "wiz8/local_screens/AutomapScreen.h"
#include "wiz8/vector.h"
#include "wiz8/sr_api.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OCTPREPATH_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\OctPrePath.cpp"

/* The vertical-link slack LinkPathNodes multiplies the cell size by. */
// GLOBAL: WIZ8 0x005ED300
static float g_prepath_link_height_5ed300 = 1.1f;

/* OctPrePathLog is a build-time ASCII density map of the path grid: one row of
   space-padded characters per z cell, one column per x cell, plus a parallel
   grid recording each edge node's link count.  Only the constructor, the
   marker below and PrePathing's teardown ever touch it. */
class OctPrePathLog {
public:
    OctPrePathLog(float scale, const float* bounds); /* 0x004CCE00 */
    void MarkPathNode(W8PrePathNode* node);          /* 0x004CCF50 */

    int width_00;
    int rows_04;
    float scale_08;
    float minimum_0c[3];
    char** path_strings_18;
    char** link_strings_1c;
};

static_assert(sizeof(OctPrePathLog) == 0x20, "OctPrePathLog_must_be_0x20");

// FUNCTION: WIZ8 0x004CCE00
OctPrePathLog::OctPrePathLog(float scale, const float* bounds)
{
    width_00 = 0;
    rows_04 = 0;
    scale_08 = scale;
    if (bounds != 0) {
        width_00 = static_cast<int>((bounds[3] - bounds[0]) / scale) + 1;
        rows_04 = static_cast<int>((bounds[5] - bounds[2]) / scale) + 1;
        minimum_0c[0] = bounds[0];
        minimum_0c[1] = bounds[1];
        minimum_0c[2] = bounds[2];
        path_strings_18 = static_cast<char**>(malloc(rows_04 << 2));
        if (path_strings_18 == 0) {
            Function497690(7, "OctPrePathLog: Could not allocate m_pPathStrings.\n");
            return;
        }
        for (int i = 0; i < rows_04; ++i) {
            path_strings_18[i] = static_cast<char*>(malloc(width_00 + 1));
            memset(path_strings_18[i], ' ', width_00);
            path_strings_18[i][width_00] = 0;
        }
        link_strings_1c = static_cast<char**>(malloc(rows_04 << 2));
        if (link_strings_1c == 0) {
            Function497690(7, "OctPrePathLog: Could not allocate m_pLinkStrings.\n");
            return;
        }
        for (int j = 0; j < rows_04; ++j) {
            link_strings_1c[j] = static_cast<char*>(malloc(width_00 + 1));
            memset(link_strings_1c[j], ' ', width_00);
            link_strings_1c[j][width_00] = 0;
        }
    }
}

// FUNCTION: WIZ8 0x004CCF50
void OctPrePathLog::MarkPathNode(W8PrePathNode* node)
{
    unsigned int row = node->cell >> 0x10;
    unsigned int column = node->cell & 0xffff;
    int links = 0;
    for (int i = 0; i < 8; ++i) {
        if ((node->level_flags & (1 << (i + 0x10))) != 0) {
            ++links;
        }
    }
    char* cell = path_strings_18[row] + column;
    if (*cell == ' ') {
        *cell = '1';
        link_strings_1c[row][column] = links + '0';
        return;
    }
    ++*cell;
    link_strings_1c[row][column] = links + '0';
}

// FUNCTION: WIZ8 0x004CCFD0
PrePathing::PrePathing()
{
    path_node_list_240 = 0;
    path_log_244 = 0;
    owned_248 = 0;
    cell_map_254 = 0;
    named_positions_250 = 0;
    node_chunks_258[0] = static_cast<W8PrePathNode*>(malloc(0x3e80));
    memset(node_chunks_258[0], 0, 0x3e80);
    chunk_index_11f8 = 0;
    chunk_node_count_11fc = 0;
}

// FUNCTION: WIZ8 0x004CD030
PrePathing::~PrePathing()
{
    if (path_node_list_240 != 0) {
        free(path_node_list_240);
    }
    if (path_log_244 != 0) {
        if (path_log_244->path_strings_18 != 0 && path_log_244->link_strings_1c != 0) {
            for (int i = 0; i < path_log_244->rows_04; ++i) {
                if (path_log_244->path_strings_18[i] != 0) {
                    free(path_log_244->path_strings_18[i]);
                }
                if (path_log_244->link_strings_1c[i] != 0) {
                    free(path_log_244->link_strings_1c[i]);
                }
            }
            free(path_log_244->path_strings_18);
            free(path_log_244->link_strings_1c);
        }
        delete path_log_244;
    }
    if (owned_248 != 0) {
        free(owned_248);
    }
    if (named_positions_250 != 0) {
        delete[] named_positions_250;
    }
    for (int i = 0; i <= chunk_index_11f8; ++i) {
        free(node_chunks_258[i]);
    }
}

// FUNCTION: WIZ8 0x004CD130
int PrePathing::SnapNamedPositions004CD130(W8LevelFileNamedPosition* positions, int count,
                                           unsigned int min_component_percent, OctPreTree* octree)
{
    min_component_percent_1200 = min_component_percent;
    named_position_count_24c = count;
    if (count != 0) {
        named_positions_250 = new srVector3T<float>[count];
        for (int i = 0; i < named_position_count_24c; ++i) {
            named_positions_250[i].x = positions[i].x_81 * g_world_scale_005ebc40;
            named_positions_250[i].y = positions[i].y_85 * g_world_scale_005ebc40;
            named_positions_250[i].z = positions[i].z_89 * g_world_scale_005ebc40;
            octree->SnapToGround(&named_positions_250[i], 0);
        }
    }
    return named_position_count_24c;
}

// FUNCTION: WIZ8 0x004CD210
W8PrePathNode* PrePathing::GetPathNode()
{
    if (1000 <= static_cast<unsigned int>(chunk_node_count_11fc)) {
        ++chunk_index_11f8;
        if (1000 <= static_cast<unsigned int>(chunk_index_11f8)) {
            Function497690(7, "There are over one million path nodes required for this level!");
        }
        node_chunks_258[chunk_index_11f8] = static_cast<W8PrePathNode*>(malloc(0x3e80));
        if (node_chunks_258[chunk_index_11f8] == 0) {
            Function497690(7, "PrePathing::GetPathNode -- Could not allocate path nodes.");
        }
        memset(node_chunks_258[chunk_index_11f8], 0, 0x3e80);
        chunk_node_count_11fc = 0;
    }
    W8PrePathNode* node = node_chunks_258[chunk_index_11f8] + chunk_node_count_11fc;
    ++chunk_node_count_11fc;
    return node;
}

// FUNCTION: WIZ8 0x004CD2C0
unsigned char PrePathing::BuildPathList(W8PrePathNode* nodes,
                                        W8HashTable<unsigned int, int>* cell_map)
{
    cell_map_254 = cell_map;
    path_log_244 = new OctPrePathLog(grid_scale_01c, level_bounds);
    path_node_list_240 = static_cast<W8PrePathNode**>(malloc(size_004 << 2));
    if (path_node_list_240 == 0) {
        char message[0x100];
        sprintf(message, "BuildPathList: Could not allocate path node list, length %d nodes.\n",
                size_004);
        Function497690(7, message);
        return 0;
    }
    for (int i = 0; i < size_004; ++i) {
        path_node_list_240[i] = nodes;
        nodes = nodes->next;
    }
    if (LinkPathNodes004CD390() == 0) {
        return 0;
    }
    CreatePathNodeArray();
    return 1;
}

// FUNCTION: WIZ8 0x004CD390
unsigned char PrePathing::LinkPathNodes004CD390()
{
    float link_height = grid_scale_01c * g_prepath_link_height_5ed300;
    int links_found = 0;
    int last_percent = 0;
    char message[0x400];
    unsigned int i;

    for (i = 1; i < static_cast<unsigned int>(size_004); ++i) {
        int percent =
            static_cast<int>(static_cast<float>(i) * 100.0f / static_cast<float>(size_004));
        if (last_percent + 5 < percent) {
            last_percent += 5;
            sprintf(message, "Linking:  %d%% Complete:  %d Links Found  \r", last_percent,
                    links_found);
            ReportStartupMessage004969D0(message);
        }
        W8PrePathNode* node = path_node_list_240[i];
        if (node->level_flags == 0) {
            continue;
        }
        for (int direction = 0; direction < 4; ++direction) {
            int x = node->cell & 0xffff;
            int z = node->cell >> 0x10;
            StepPathCell004622D0(&x, &z, direction);
            unsigned int key = (z << 0x10) + x;
            W8PrePathNode* target = 0;
            unsigned int index = cell_map_254->Lookup(&key);
            if (index != 0 && index < static_cast<unsigned int>(size_004)) {
                target = path_node_list_240[index];
            }
            if (target != 0) {
                while (link_height < fabsf(target->y - node->y)) {
                    target = target->next;
                    if (target == 0 || (target->level_flags & 0x10000000) != 0) {
                        goto next_direction;
                    }
                }
                if (target != 0) {
                    node->level_flags |= 1 << (direction + 0x10);
                    ++links_found;
                    target->level_flags |= 1 << (direction + 0x14);
                }
            }
        next_direction:;
        }
    }
    DeleteUnreachableAreas004CD7C0();
    for (i = 1; i < static_cast<unsigned int>(size_004); ++i) {
        W8PrePathNode* edge = path_node_list_240[i];
        edge->level_flags &= 0xfffffff;
        if ((edge->level_flags & 0xff0000) != 0xff0000) {
            edge->level_flags |= 0x1000000;
        }
    }
    last_percent = 1;
    for (i = 1; i < static_cast<unsigned int>(size_004); ++i) {
        int percent =
            static_cast<int>(static_cast<float>(i) * 100.0f / static_cast<float>(size_004));
        if (last_percent + 1 < percent) {
            ++last_percent;
            sprintf(message, "Computing Pathnode Clearance:  %d%% Complete  \r", last_percent);
            ReportStartupMessage004969D0(message);
        }
        W8PrePathNode* edge = path_node_list_240[i];
        if ((edge->level_flags & 0x1000000) != 0) {
            PropagatePathNodeClearance004CD650(edge, 0);
        }
    }
    return 1;
}

// FUNCTION: WIZ8 0x004CD650
void PrePathing::PropagatePathNodeClearance004CD650(W8PrePathNode* node, unsigned int depth)
{
    if (depth == 0) {
        if ((node->level_flags & 0x1000000) == 0) {
            return;
        }
    } else {
        if (0x32 <= depth) {
            return;
        }
        unsigned int flags = node->level_flags;
        if ((flags & 0x1000000) != 0) {
            return;
        }
        if (((flags >> 0x10) & 0xff) <= depth) {
            return;
        }
        node->level_flags = (flags & 0xff00ffff) | (depth << 0x10);
        ++depth;
    }
    ++depth;
    float link_height = grid_scale_01c * g_prepath_link_height_5ed300;
    for (int direction = 0; direction < 4; ++direction) {
        int x = node->cell & 0xffff;
        int z = node->cell >> 0x10;
        StepPathCell004622D0(&x, &z, direction);
        unsigned int key = (z << 0x10) + x;
        W8PrePathNode* neighbor = 0;
        unsigned int index = cell_map_254->Lookup(&key);
        if (index != 0 && index < static_cast<unsigned int>(size_004)) {
            neighbor = path_node_list_240[index];
        }
        if (neighbor != 0) {
            while (link_height < fabsf(neighbor->y - node->y)) {
                unsigned int cell = neighbor->cell;
                neighbor = neighbor->next;
                if (neighbor == 0 || neighbor->cell != cell) {
                    goto next_direction;
                }
            }
            PropagatePathNodeClearance004CD650(neighbor, depth);
        }
    next_direction:;
    }
}

// FUNCTION: WIZ8 0x004CD7C0
unsigned int PrePathing::DeleteUnreachableAreas004CD7C0()
{
    float link_height = grid_scale_01c * g_prepath_link_height_5ed300;
    int deleted = 0;
    int last_percent = 0;
    unsigned int minimum = 0;
    char message[0x400];

    if (min_component_percent_1200 != 0) {
        if (0x32 < min_component_percent_1200) {
            min_component_percent_1200 = 0x32;
        }
        minimum = static_cast<unsigned int>(size_004 * min_component_percent_1200) / 100;
    }
    visible_waypoints_058 = new BitArray(size_004);
    rendered_waypoints_05c = new BitArray(size_004);
    collected_waypoints_060 = new BitArray(size_004);
    Function497690(6, "Deleting Unreacheable Areas.\n");
    Function497690(6, "Deleting Nodes: \t");
    for (unsigned int i = 1; i < static_cast<unsigned int>(size_004); ++i) {
        int percent =
            static_cast<int>(static_cast<float>(i) * 100.0f / static_cast<float>(size_004));
        if (last_percent < percent) {
            last_percent = percent;
            sprintf(message, "Deleting:  %d%% Complete:  %d Pathnodes Deleted  \r", percent,
                    deleted);
            ReportStartupMessage004969D0(message);
        }
        if (rendered_waypoints_05c->Test(i)) {
            continue;
        }
        int component_size = 0;
        visible_waypoints_058->ClearAll();
        collected_waypoints_060->ClearAll();
        int pending = i + 1;
        while (pending != 0) {
            do {
                unsigned int index = pending - 1;
                if (path_node_list_240[index] != 0) {
                    visible_waypoints_058->Clear(index);
                    collected_waypoints_060->Set(index);
                    ++component_size;
                    for (int direction = 0; direction < 8; ++direction) {
                        W8PrePathNode* node = path_node_list_240[index];
                        if ((node->level_flags & (1 << (direction + 0x10))) == 0) {
                            continue;
                        }
                        int x = node->cell & 0xffff;
                        int z = node->cell >> 0x10;
                        bool scanning = true;
                        bool first_probe = true;
                        StepPathCell004622D0(&x, &z, direction);
                        unsigned int key = (z << 0x10) + x;
                        unsigned int next_index = cell_map_254->Lookup(&key);
                        if (static_cast<unsigned int>(size_004) <= next_index) {
                            next_index = 0;
                        }
                        W8PrePathNode* neighbor = 0;
                        while (next_index != 0 && scanning) {
                            neighbor = path_node_list_240[next_index];
                            if (static_cast<unsigned int>(size_004) < next_index) {
                                neighbor = 0;
                                scanning = false;
                            } else {
                                float height_diff =
                                    fabsf(neighbor->y - path_node_list_240[index]->y);
                                if (link_height <= height_diff) {
                                    if (first_probe || (neighbor->level_flags & 0x10000000) == 0) {
                                        ++next_index;
                                        first_probe = false;
                                    } else {
                                        neighbor = 0;
                                        scanning = false;
                                    }
                                } else {
                                    scanning = false;
                                    if ((neighbor->level_flags & 0x40000000) != 0) {
                                        neighbor = 0;
                                    }
                                }
                            }
                        }
                        if (neighbor != 0 && !collected_waypoints_060->Test(next_index)) {
                            if (!rendered_waypoints_05c->Test(next_index)) {
                                visible_waypoints_058->Set(next_index);
                            } else {
                                component_size = minimum + 1;
                            }
                        }
                    }
                }
                pending = visible_waypoints_058->NextSetBit(0);
            } while (pending != 0);
            pending = visible_waypoints_058->NextSetBit(1);
        }
        if (component_size < static_cast<int>(minimum)) {
            bool clear_of_named = true;
            pending = collected_waypoints_060->NextSetBit(1);
            if (pending != 0) {
                do {
                    if (!clear_of_named) {
                        goto component_done;
                    }
                    W8PrePathNode* node = path_node_list_240[pending - 1];
                    unsigned int cell = node->cell;
                    float world_y = node->y;
                    float world_x =
                        ((cell & 0xffff) + g_float_005ebc7c) * grid_scale_01c + level_bounds[0];
                    float world_z =
                        ((cell >> 0x10) + g_float_005ebc7c) * grid_scale_01c + level_bounds[2];
                    for (int n = 0; n < named_position_count_24c && clear_of_named; ++n) {
                        float dx = world_x - named_positions_250[n].x;
                        float dy = world_y - named_positions_250[n].y;
                        float dz = world_z - named_positions_250[n].z;
                        if (sqrtf(dx * dx + dy * dy + dz * dz) < grid_scale_01c + grid_scale_01c) {
                            clear_of_named = false;
                        }
                    }
                    pending = collected_waypoints_060->NextSetBit(0);
                } while (pending != 0);
                if (!clear_of_named) {
                    goto component_done;
                }
            }
            pending = collected_waypoints_060->NextSetBit(1);
            while (pending != 0) {
                path_node_list_240[pending - 1]->level_flags |= 0x40000000;
                ++deleted;
                pending = collected_waypoints_060->NextSetBit(0);
            }
        }
    component_done:
        rendered_waypoints_05c->UnionWith(*collected_waypoints_060);
    }
    cell_map_254->Clear();
    int kept = 1;
    for (unsigned int j = 1; j < static_cast<unsigned int>(size_004); ++j) {
        W8PrePathNode* node = path_node_list_240[j];
        if ((node->level_flags & 0x40000000) == 0) {
            path_node_list_240[kept] = node;
            cell_map_254->Insert(&node->cell, &kept);
            if (j != static_cast<unsigned int>(kept)) {
                path_node_list_240[j] = 0;
            }
            ++kept;
        }
    }
    delete visible_waypoints_058;
    visible_waypoints_058 = 0;
    delete rendered_waypoints_05c;
    rendered_waypoints_05c = 0;
    delete collected_waypoints_060;
    collected_waypoints_060 = 0;
    sprintf(message, "Nodes Deleted: %d\n", size_004 - kept);
    Function497690(6, message);
    size_004 = kept;
    return kept;
}

// FUNCTION: WIZ8 0x004CDF70
int PrePathing::CreatePathNodeArray()
{
    unsigned int i = 1;
    char message[0x400];

    edge_node_count_008 = 1;
    path_nodes_044 = static_cast<unsigned int*>(malloc(size_004 << 3));
    if (path_nodes_044 == 0) {
        sprintf(message, "CreatePathNodeArray: Could not allocate m_pulNodeHashArray.\n");
        Function497690(7, message);
    }
    if (1 < static_cast<unsigned int>(size_004)) {
        do {
            W8PrePathNode* node = path_node_list_240[i];
            unsigned int flags = node->level_flags;
            if ((flags & 0x1000000) != 0) {
                path_log_244->MarkPathNode(node);
                ++edge_node_count_008;
            }
            path_nodes_044[i * 2 - 2] = path_node_list_240[i]->cell;
            path_nodes_044[i * 2 - 1] = flags & 0xfffffff;
            ++i;
        } while (i < static_cast<unsigned int>(size_004));
    }
    sprintf(message, "  %d Total Pathnodes, %d of which are Edge Nodes.\n", size_004,
            edge_node_count_008);
    Function497690(6, message);
    sprintf(message, "Total memory taken by Path Nodes: %dk.\n",
            (static_cast<unsigned int>(size_004) & 0x1fffffff) >> 7);
    Function497690(6, message);
    m_ulNumSurfaces = 0;
    file_waypoints_050 = 0;
    return 1;
}

// FUNCTION: WIZ8 0x004CE070
unsigned char PrePathing::CreateAutomapNodes004CE070(W8LevelFile* level)
{
    int last_percent = 0;
    W8GrowableVector<int> node_keys;
    W8HashTable<unsigned int, unsigned char> used_keys;
    char message[0x400];

    if (path_node_list_240 == 0) {
        return 0;
    }
    SetFloat64B914(Function585320() != 0 ? 4000.0f : 2000.0f);
    int created = 0;
    unsigned int i = 1;
    if (1 < static_cast<unsigned int>(size_004)) {
        do {
            int percent =
                static_cast<int>(static_cast<float>(i) * 100.0f / static_cast<float>(size_004));
            if (last_percent < percent) {
                sprintf(message, "Creating Automap Nodes:  %d%% Complete:  %d Nodes created  \r",
                        percent, created);
                ReportStartupMessage004969D0(message);
                last_percent = percent;
            }
            W8PrePathNode* node = path_node_list_240[i];
            srVector3T<float> position;
            position.y = static_cast<float>(node->level_flags & 0xffff) * span_020;
            position.x = static_cast<float>(node->cell & 0xffff) * grid_scale_01c;
            position.z = static_cast<float>(node->cell >> 0x10) * grid_scale_01c;
            unsigned int key = AutomapNodeKey005852B0(&position);
            if (used_keys.FindNextEntry(&key, -1) == -1) {
                node_keys.Add(key);
                unsigned char present = 1;
                used_keys.Insert(&key, &present);
                ++created;
            }
            ++i;
        } while (i < static_cast<unsigned int>(size_004));
    }
    used_keys.Clear();
    level->num_automap_nodes_6b1 = node_keys.GetCount();
    sprintf(message, "Creating Automap Nodes:  100%% Complete:  %d Automap Nodes created  \n",
            node_keys.GetCount());
    ReportStartupMessage004969D0(message);
    sprintf(message, "  %d Total Automap Nodes.\n", level->num_automap_nodes_6b1);
    Function497690(6, message);
    level->automap_nodes_6b5 =
        static_cast<unsigned long*>(malloc(level->num_automap_nodes_6b1 << 2));
    if (level->automap_nodes_6b5 == 0) {
        level->num_automap_nodes_6b1 = 0;
    } else {
        for (i = 0; i < static_cast<unsigned int>(level->num_automap_nodes_6b1); ++i) {
            level->automap_nodes_6b5[i] = *node_keys.GetAt(i);
        }
        QuickSort(level->automap_nodes_6b5, 0, level->num_automap_nodes_6b1 - 1);
    }
    for (i = 1; i < static_cast<unsigned int>(size_004); ++i) {
        path_node_list_240[i] = 0;
    }
    free(path_node_list_240);
    path_node_list_240 = 0;
    return 1;
}

// FUNCTION: WIZ8 0x004CE510
void W8PathingService::LinkCollideableProps(int lNumProps, W8PreProp* pPreProps,
                                            W8HashTable<unsigned int, unsigned int*>* pCondValues)
{
    int aiLookup[10000];
    unsigned int aulKeys[10000];
    unsigned short ausFrames[10000];
    unsigned int aulValues[10000];
    int i;

    m_ulNumCondPaths = 1;
    m_ulNumCondNodes = 1;
    m_ulNumCondFrames = 1;
    m_pPathValues_064 = new W8HashTable<unsigned int, unsigned int>;
    for (i = 0; i < size_004; ++i) {
        m_pPathValues_064->Insert(&path_nodes_044[i * 2], &path_nodes_044[i * 2 + 1]);
    }

    W8ConditionalPath** ppCondPaths = static_cast<W8ConditionalPath**>(malloc(lNumProps * 4 + 8));
    if (ppCondPaths == 0) {
        srAssertFail("ppCondPaths", OCTPREPATH_CPP, 1037,
                     "LinkCollideableProps: Couldn't allocate GDPropCondPaths objects.");
    }
    memset(ppCondPaths, 0, lNumProps * 4 + 8);

    int ulOriginalCount = 0;
    for (i = 0; i < lNumProps; ++i) {
        ++ulOriginalCount;
        W8PreProp* pProp = pPreProps + i;
        if (pProp->num_stop_meshes_40 != 0) {
            for (unsigned short j = 0; j < pProp->num_stop_meshes_40; ++j) {
                bool bWroteFrame = false;
                unsigned int key =
                    (static_cast<unsigned int>(pProp->pStopMeshes[j].m_prop_number_02) << 16) |
                    (i + 1);
                int slot = pCondValues->FindNextEntry(&key, -1);
                if (slot != -1) {
                    do {
                        unsigned int* puValue = pCondValues->entries[slot].value;
                        if ((puValue != 0) &&
                            (FindConditionalPathValue00458970(puValue[1], *puValue) != 0)) {
                            if (ppCondPaths[i] == 0) {
                                if (static_cast<unsigned int>(lNumProps) <
                                    static_cast<unsigned int>(ulOriginalCount)) {
                                    srAssertFail("ulOriginalCount <= (UINT32)lNumProps",
                                                 OCTPREPATH_CPP, 1065,
                                                 "LinkCollideableProps: Invalid count for "
                                                 "collideable props.");
                                }
                                W8ConditionalPath* pPath = static_cast<W8ConditionalPath*>(
                                    malloc(sizeof(W8ConditionalPath)));
                                ppCondPaths[i] = pPath;
                                memset(pPath, 0, sizeof(W8ConditionalPath));
                                strcpy(pPath->name, pProp->name);
                                pPath->lookup_index = m_ulNumCondFrames;
                                ++m_ulNumCondPaths;
                            }
                            if (!bWroteFrame) {
                                aiLookup[m_ulNumCondFrames + 1] = m_ulNumCondNodes;
                                ausFrames[m_ulNumCondFrames] =
                                    pProp->pStopMeshes[j].m_prop_number_02;
                                ++m_ulNumCondFrames;
                                if (m_ulNumCondFrames >= 10000) {
                                    srAssertFail("m_ulNumCondFrames < 10000", OCTPREPATH_CPP, 1077,
                                                 "LinkCollideableProps: Too many conditional "
                                                 "frames.");
                                }
                                bWroteFrame = true;
                            }
                            aulKeys[m_ulNumCondNodes] = puValue[1];
                            aulValues[m_ulNumCondNodes] = *puValue;
                            ++m_ulNumCondNodes;
                            if (m_ulNumCondNodes >= 10000) {
                                srAssertFail("m_ulNumCondNodes < 10000", OCTPREPATH_CPP, 1082,
                                             "LinkCollideableProps: Too many conditional "
                                             "nodes.");
                            }
                            free(puValue);
                        }
                        slot = pCondValues->FindNextEntry(&key, slot);
                    } while (slot != -1);
                }
                if (bWroteFrame) {
                    aulKeys[m_ulNumCondNodes] = 0;
                    aulValues[m_ulNumCondNodes] = 0;
                    ++m_ulNumCondNodes;
                }
            }
        }
        if (ppCondPaths[i] != 0) {
            if ((pProp->num_stop_meshes_40 == 1) && ((pProp->pStopMeshes[0].m_flags_00 & 1) != 0)) {
                aiLookup[m_ulNumCondFrames + 1] = aiLookup[m_ulNumCondFrames];
                ausFrames[m_ulNumCondFrames] = pProp->pStopMeshes[0].m_frame_58;
                ++m_ulNumCondFrames;
            }
            aiLookup[m_ulNumCondFrames + 1] = 0;
            ausFrames[m_ulNumCondFrames] = 0;
            ++m_ulNumCondFrames;
        }
    }

    if ((static_cast<unsigned int>(m_ulNumCondFrames) < 2) ||
        (static_cast<unsigned int>(m_ulNumCondNodes) < 2)) {
        m_ulNumCondNodes = 0;
        m_ulNumCondFrames = 0;
        return;
    }

    m_pCondPaths =
        static_cast<W8ConditionalPath*>(malloc(m_ulNumCondPaths * sizeof(W8ConditionalPath)));
    if (m_pCondPaths == 0) {
        srAssertFail("m_pCondPaths", OCTPREPATH_CPP, 1116,
                     "LinkCollideableProps: Couldn't allocate m_pCondPaths array.");
    }
    memset(m_pCondPaths, 0, m_ulNumCondPaths * sizeof(W8ConditionalPath));
    m_ulNumCondPaths = 0;
    if (ulOriginalCount > 1) {
        for (i = 1; i < ulOriginalCount; ++i) {
            if (ppCondPaths[i] != 0) {
                int index = m_ulNumCondPaths;
                ++m_ulNumCondPaths;
                memcpy(m_pCondPaths + index, ppCondPaths[i], sizeof(W8ConditionalPath));
            }
        }
    }

    m_pulCondLookup = static_cast<unsigned int*>(malloc(m_ulNumCondFrames << 2));
    if (m_pulCondLookup == 0) {
        srAssertFail("m_pulCondLookup", OCTPREPATH_CPP, 1127, 0);
    }
    memcpy(m_pulCondLookup, aiLookup + 1, m_ulNumCondFrames << 2);

    m_pusCondNodeFrames = static_cast<unsigned short*>(malloc(m_ulNumCondFrames << 1));
    if (m_pusCondNodeFrames == 0) {
        srAssertFail("m_pusCondNodeFrames", OCTPREPATH_CPP, 1130, 0);
    }
    memcpy(m_pusCondNodeFrames, ausFrames, m_ulNumCondFrames << 1);

    m_pulCondNodeKeys = static_cast<unsigned int*>(malloc(m_ulNumCondNodes << 2));
    if (m_pulCondNodeKeys == 0) {
        srAssertFail("m_pulCondNodeKeys", OCTPREPATH_CPP, 1134, 0);
    }
    memcpy(m_pulCondNodeKeys, aulKeys, m_ulNumCondNodes << 2);

    m_pulCondNodeValues = static_cast<unsigned int*>(malloc(m_ulNumCondNodes << 2));
    if (m_pulCondNodeValues == 0) {
        srAssertFail("m_pulCondNodeValues", OCTPREPATH_CPP, 1137, 0);
    }
    memcpy(m_pulCondNodeValues, aulValues, m_ulNumCondNodes << 2);
}

// TEMPLATE: WIZ8 0x004CECA0
// W8HashTable<unsigned int,unsigned char>::Grow

// TEMPLATE: WIZ8 0x004CEDF0
// InsertionSort
