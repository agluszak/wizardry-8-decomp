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

/* The vertical-link slack LinkPathNodes multiplies the cell size by. Retail
   Combat.cpp reads it directly, so it is not file-static. */
// GLOBAL: WIZ8 0x005ED300
const float g_prepath_link_height = 1.1f;

/* OctPrePathLog is a build-time ASCII density map of the path grid: one row of
   space-padded characters per z cell, one column per x cell, plus a parallel
   grid recording each edge node's link count.  Only the constructor, the
   marker below and PrePathing's teardown ever touch it. */
class OctPrePathLog {
public:
    OctPrePathLog(float scale, const W8BoundingBox* bounds); /* 0x004CCE00 */
    void MarkPathNode(W8PrePathNode* node);                  /* 0x004CCF50 */

    int width;
    int rows;
    float scale;
    srVector3T<float> m_minimum;
    char** m_pPathStrings;
    char** m_pLinkStrings;
};

static_assert(sizeof(OctPrePathLog) == 0x20, "OctPrePathLog_must_be_0x20");

// FUNCTION: WIZ8 0x004CCE00
OctPrePathLog::OctPrePathLog(float scale, const W8BoundingBox* bounds)
{
    width = 0;
    rows = 0;
    this->scale = scale;
    if (bounds != 0) {
        width = static_cast<int>((bounds->maximum.x - bounds->minimum.x) / scale) + 1;
        rows = static_cast<int>((bounds->maximum.z - bounds->minimum.z) / scale) + 1;
        m_minimum = bounds->minimum;
        m_pPathStrings = static_cast<char**>(malloc(rows << 2));
        if (m_pPathStrings == 0) {
            ReportBuildStatus(7, "OctPrePathLog: Could not allocate m_pPathStrings.\n");
            return;
        }
        for (int i = 0; i < rows; ++i) {
            m_pPathStrings[i] = static_cast<char*>(malloc(width + 1));
            memset(m_pPathStrings[i], ' ', width);
            m_pPathStrings[i][width] = 0;
        }
        m_pLinkStrings = static_cast<char**>(malloc(rows << 2));
        if (m_pLinkStrings == 0) {
            ReportBuildStatus(7, "OctPrePathLog: Could not allocate m_pLinkStrings.\n");
            return;
        }
        for (int j = 0; j < rows; ++j) {
            m_pLinkStrings[j] = static_cast<char*>(malloc(width + 1));
            memset(m_pLinkStrings[j], ' ', width);
            m_pLinkStrings[j][width] = 0;
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
    char* cell = m_pPathStrings[row] + column;
    if (*cell == ' ') {
        *cell = '1';
        m_pLinkStrings[row][column] = links + '0';
        return;
    }
    ++*cell;
    m_pLinkStrings[row][column] = links + '0';
}

// FUNCTION: WIZ8 0x004CCFD0
PrePathing::PrePathing()
{
    path_node_list = 0;
    path_log = 0;
    owned_248 = 0;
    cell_map = 0;
    named_positions = 0;
    node_chunks[0] =
        static_cast<W8PrePathNode*>(malloc(W8_PREPATH_NODES_PER_CHUNK * sizeof(W8PrePathNode)));
    memset(node_chunks[0], 0, W8_PREPATH_NODES_PER_CHUNK * sizeof(W8PrePathNode));
    chunk_index = 0;
    chunk_node_count = 0;
}

// FUNCTION: WIZ8 0x004CD030
PrePathing::~PrePathing()
{
    if (path_node_list != 0) {
        free(path_node_list);
    }
    if (path_log != 0) {
        if (path_log->m_pPathStrings != 0 && path_log->m_pLinkStrings != 0) {
            for (int i = 0; i < path_log->rows; ++i) {
                if (path_log->m_pPathStrings[i] != 0) {
                    free(path_log->m_pPathStrings[i]);
                }
                if (path_log->m_pLinkStrings[i] != 0) {
                    free(path_log->m_pLinkStrings[i]);
                }
            }
            free(path_log->m_pPathStrings);
            free(path_log->m_pLinkStrings);
        }
        delete path_log;
    }
    if (owned_248 != 0) {
        free(owned_248);
    }
    if (named_positions != 0) {
        delete[] named_positions;
    }
    for (int i = 0; i <= chunk_index; ++i) {
        free(node_chunks[i]);
    }
}

// FUNCTION: WIZ8 0x004CD130
int PrePathing::SnapNamedPositions(W8LevelFileNamedPosition* positions, int count,
                                   unsigned int min_component_percent, OctPreTree* octree)
{
    this->min_component_percent = min_component_percent;
    named_position_count = count;
    if (count != 0) {
        named_positions = new srVector3T<float>[count];
        for (int i = 0; i < named_position_count; ++i) {
            named_positions[i].Set(positions[i].position.x * g_world_scale,
                                   positions[i].position.y * g_world_scale,
                                   positions[i].position.z * g_world_scale);
            octree->SnapToGround(&named_positions[i], 0);
        }
    }
    return named_position_count;
}

// FUNCTION: WIZ8 0x004CD210
W8PrePathNode* PrePathing::GetPathNode()
{
    if (1000 <= static_cast<unsigned int>(chunk_node_count)) {
        ++chunk_index;
        if (1000 <= static_cast<unsigned int>(chunk_index)) {
            ReportBuildStatus(7, "There are over one million path nodes required for this level!");
        }
        node_chunks[chunk_index] =
            static_cast<W8PrePathNode*>(malloc(W8_PREPATH_NODES_PER_CHUNK * sizeof(W8PrePathNode)));
        if (node_chunks[chunk_index] == 0) {
            ReportBuildStatus(7, "PrePathing::GetPathNode -- Could not allocate path nodes.");
        }
        memset(node_chunks[chunk_index], 0, W8_PREPATH_NODES_PER_CHUNK * sizeof(W8PrePathNode));
        chunk_node_count = 0;
    }
    W8PrePathNode* node = node_chunks[chunk_index] + chunk_node_count;
    ++chunk_node_count;
    return node;
}

// FUNCTION: WIZ8 0x004CD2C0
unsigned char PrePathing::BuildPathList(W8PrePathNode* nodes,
                                        W8HashTable<unsigned int, int>* cell_map)
{
    this->cell_map = cell_map;
    path_log = new OctPrePathLog(grid_scale, &level_bounds);
    path_node_list = static_cast<W8PrePathNode**>(malloc(path_node_count << 2));
    if (path_node_list == 0) {
        char message[0x100];
        sprintf(message, "BuildPathList: Could not allocate path node list, length %d nodes.\n",
                path_node_count);
        ReportBuildStatus(7, message);
        return 0;
    }
    for (int i = 0; i < path_node_count; ++i) {
        path_node_list[i] = nodes;
        nodes = nodes->next;
    }
    if (LinkPathNodes() == 0) {
        return 0;
    }
    CreatePathNodeArray();
    return 1;
}

W8PrePathNode* PrePathing::FindAdjacentPathNode(const W8PrePathNode* node, int direction)
{
    int x = node->cell & 0xffff;
    int z = node->cell >> 0x10;
    StepPathCell(&x, &z, direction);
    unsigned int key = (z << 0x10) + x;
    W8PrePathNode* target = 0;
    unsigned int index = cell_map->Lookup(&key);
    if (index != 0 && index < static_cast<unsigned int>(path_node_count)) {
        target = path_node_list[index];
    }
    return target;
}

// FUNCTION: WIZ8 0x004CD390
unsigned char PrePathing::LinkPathNodes()
{
    float link_height = grid_scale * g_prepath_link_height;
    int links_found = 0;
    int last_percent = 0;
    char message[0x400];
    unsigned int i;

    for (i = 1; i < static_cast<unsigned int>(path_node_count); ++i) {
        int percent = static_cast<int>(i * 100.0f / path_node_count);
        if (last_percent + 5 < percent) {
            last_percent += 5;
            sprintf(message, "Linking:  %d%% Complete:  %d Links Found  \r", last_percent,
                    links_found);
            ReportStartupMessage(message);
        }
        W8PrePathNode* node = path_node_list[i];
        if (node->level_flags == 0) {
            continue;
        }
        for (int direction = 0; direction < 4; ++direction) {
            W8PrePathNode* target = FindAdjacentPathNode(node, direction);
            if (target != 0) {
                while (link_height < fabsf(target->y - node->y)) {
                    target = target->next;
                    if (target == 0 || (target->level_flags & W8_PATH_CELL_INACTIVE) != 0) {
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
    DeleteUnreachableAreas();
    for (i = 1; i < static_cast<unsigned int>(path_node_count); ++i) {
        W8PrePathNode* edge = path_node_list[i];
        edge->level_flags &= 0xfffffff;
        if ((edge->level_flags & W8_PATH_CELL_NEIGHBOR_OR_DEPTH_MASK) !=
            W8_PATH_CELL_NEIGHBOR_OR_DEPTH_MASK) {
            edge->level_flags |= W8_PATH_CELL_HAS_DIRECTIONS;
        }
    }
    last_percent = 1;
    for (i = 1; i < static_cast<unsigned int>(path_node_count); ++i) {
        int percent = static_cast<int>(i * 100.0f / path_node_count);
        if (last_percent + 1 < percent) {
            ++last_percent;
            sprintf(message, "Computing Pathnode Clearance:  %d%% Complete  \r", last_percent);
            ReportStartupMessage(message);
        }
        W8PrePathNode* edge = path_node_list[i];
        if ((edge->level_flags & W8_PATH_CELL_HAS_DIRECTIONS) != 0) {
            PropagatePathNodeClearance(edge, 0);
        }
    }
    return 1;
}

// FUNCTION: WIZ8 0x004CD650
void PrePathing::PropagatePathNodeClearance(W8PrePathNode* node, unsigned int depth)
{
    if (depth == 0) {
        if ((node->level_flags & W8_PATH_CELL_HAS_DIRECTIONS) == 0) {
            return;
        }
    } else {
        if (0x32 <= depth) {
            return;
        }
        unsigned int flags = node->level_flags;
        if ((flags & W8_PATH_CELL_HAS_DIRECTIONS) != 0) {
            return;
        }
        if (((flags >> 0x10) & 0xff) <= depth) {
            return;
        }
        node->level_flags = (flags & ~W8_PATH_CELL_NEIGHBOR_OR_DEPTH_MASK) | (depth << 0x10);
        ++depth;
    }
    ++depth;
    float link_height = grid_scale * g_prepath_link_height;
    for (int direction = 0; direction < 4; ++direction) {
        W8PrePathNode* neighbor = FindAdjacentPathNode(node, direction);
        if (neighbor != 0) {
            while (link_height < fabsf(neighbor->y - node->y)) {
                unsigned int cell = neighbor->cell;
                neighbor = neighbor->next;
                if (neighbor == 0 || neighbor->cell != cell) {
                    goto next_direction;
                }
            }
            PropagatePathNodeClearance(neighbor, depth);
        }
    next_direction:;
    }
}

// FUNCTION: WIZ8 0x004CD7C0
unsigned int PrePathing::DeleteUnreachableAreas()
{
    float link_height = grid_scale * g_prepath_link_height;
    int deleted = 0;
    int last_percent = 0;
    unsigned int minimum = 0;
    char message[0x400];

    if (min_component_percent != 0) {
        if (0x32 < min_component_percent) {
            min_component_percent = 0x32;
        }
        minimum = static_cast<unsigned int>(path_node_count * min_component_percent) / 100;
    }
    m_marked_path_nodes = new BitArray(path_node_count);
    m_visited_path_nodes = new BitArray(path_node_count);
    m_collected_path_nodes = new BitArray(path_node_count);
    ReportBuildStatus(6, "Deleting Unreacheable Areas.\n");
    ReportBuildStatus(6, "Deleting Nodes: \t");
    for (unsigned int i = 1; i < static_cast<unsigned int>(path_node_count); ++i) {
        int percent = static_cast<int>(i * 100.0f / path_node_count);
        if (last_percent < percent) {
            last_percent = percent;
            sprintf(message, "Deleting:  %d%% Complete:  %d Pathnodes Deleted  \r", percent,
                    deleted);
            ReportStartupMessage(message);
        }
        if (m_visited_path_nodes->Test(i)) {
            continue;
        }
        int component_size = 0;
        m_marked_path_nodes->ClearAll();
        m_collected_path_nodes->ClearAll();
        int pending = i + 1;
        while (pending != 0) {
            do {
                unsigned int index = pending - 1;
                if (path_node_list[index] != 0) {
                    m_marked_path_nodes->Clear(index);
                    m_collected_path_nodes->Set(index);
                    ++component_size;
                    for (int direction = 0; direction < 8; ++direction) {
                        W8PrePathNode* node = path_node_list[index];
                        if ((node->level_flags & (1 << (direction + 0x10))) == 0) {
                            continue;
                        }
                        int x = node->cell & 0xffff;
                        int z = node->cell >> 0x10;
                        bool scanning = true;
                        bool first_probe = true;
                        StepPathCell(&x, &z, direction);
                        unsigned int key = (z << 0x10) + x;
                        unsigned int next_index = cell_map->Lookup(&key);
                        if (static_cast<unsigned int>(path_node_count) <= next_index) {
                            next_index = 0;
                        }
                        W8PrePathNode* neighbor = 0;
                        while (next_index != 0 && scanning) {
                            neighbor = path_node_list[next_index];
                            if (static_cast<unsigned int>(path_node_count) < next_index) {
                                neighbor = 0;
                                scanning = false;
                            } else {
                                float height_diff = fabsf(neighbor->y - path_node_list[index]->y);
                                if (link_height <= height_diff) {
                                    if (first_probe ||
                                        (neighbor->level_flags & W8_PATH_CELL_INACTIVE) == 0) {
                                        ++next_index;
                                        first_probe = false;
                                    } else {
                                        neighbor = 0;
                                        scanning = false;
                                    }
                                } else {
                                    scanning = false;
                                    if ((neighbor->level_flags & W8_PREPATH_NODE_PRUNED) != 0) {
                                        neighbor = 0;
                                    }
                                }
                            }
                        }
                        if (neighbor != 0 && !m_collected_path_nodes->Test(next_index)) {
                            if (!m_visited_path_nodes->Test(next_index)) {
                                m_marked_path_nodes->Set(next_index);
                            } else {
                                component_size = minimum + 1;
                            }
                        }
                    }
                }
                pending = m_marked_path_nodes->NextSetBit(false);
            } while (pending != 0);
            pending = m_marked_path_nodes->NextSetBit(true);
        }
        if (component_size < static_cast<int>(minimum)) {
            bool clear_of_named = true;
            pending = m_collected_path_nodes->NextSetBit(true);
            if (pending != 0) {
                do {
                    if (!clear_of_named) {
                        goto component_done;
                    }
                    W8PrePathNode* node = path_node_list[pending - 1];
                    unsigned int cell = node->cell;
                    float world_y = node->y;
                    float world_x =
                        ((cell & 0xffff) + g_float_half) * grid_scale + level_bounds.minimum.x;
                    float world_z =
                        ((cell >> 0x10) + g_float_half) * grid_scale + level_bounds.minimum.z;
                    for (int n = 0; n < named_position_count && clear_of_named; ++n) {
                        float dx = world_x - named_positions[n].x;
                        float dy = world_y - named_positions[n].y;
                        float dz = world_z - named_positions[n].z;
                        if (sqrtf(dx * dx + dy * dy + dz * dz) < grid_scale + grid_scale) {
                            clear_of_named = false;
                        }
                    }
                    pending = m_collected_path_nodes->NextSetBit(false);
                } while (pending != 0);
                if (!clear_of_named) {
                    goto component_done;
                }
            }
            pending = m_collected_path_nodes->NextSetBit(true);
            while (pending != 0) {
                path_node_list[pending - 1]->level_flags |= W8_PREPATH_NODE_PRUNED;
                ++deleted;
                pending = m_collected_path_nodes->NextSetBit(false);
            }
        }
    component_done:
        m_visited_path_nodes->UnionWith(*m_collected_path_nodes);
    }
    cell_map->Clear();
    int kept = 1;
    for (unsigned int j = 1; j < static_cast<unsigned int>(path_node_count); ++j) {
        W8PrePathNode* node = path_node_list[j];
        if ((node->level_flags & W8_PREPATH_NODE_PRUNED) == 0) {
            path_node_list[kept] = node;
            cell_map->Insert(&node->cell, &kept);
            if (j != static_cast<unsigned int>(kept)) {
                path_node_list[j] = 0;
            }
            ++kept;
        }
    }
    delete m_marked_path_nodes;
    m_marked_path_nodes = 0;
    delete m_visited_path_nodes;
    m_visited_path_nodes = 0;
    delete m_collected_path_nodes;
    m_collected_path_nodes = 0;
    sprintf(message, "Nodes Deleted: %d\n", path_node_count - kept);
    ReportBuildStatus(6, message);
    path_node_count = kept;
    return kept;
}

// FUNCTION: WIZ8 0x004CDF70
int PrePathing::CreatePathNodeArray()
{
    unsigned int i = 1;
    char message[0x400];

    edge_node_count = 1;
    file_path_nodes =
        static_cast<W8FilePathNode*>(malloc(path_node_count * sizeof(W8FilePathNode)));
    if (file_path_nodes == 0) {
        sprintf(message, "CreatePathNodeArray: Could not allocate m_pulNodeHashArray.\n");
        ReportBuildStatus(7, message);
    }
    if (1 < static_cast<unsigned int>(path_node_count)) {
        do {
            W8PrePathNode* node = path_node_list[i];
            unsigned int flags = node->level_flags;
            if ((flags & W8_PATH_CELL_HAS_DIRECTIONS) != 0) {
                path_log->MarkPathNode(node);
                ++edge_node_count;
            }
            file_path_nodes[i - 1].cell = path_node_list[i]->cell;
            file_path_nodes[i - 1].level_flags = flags & 0xfffffff;
            ++i;
        } while (i < static_cast<unsigned int>(path_node_count));
    }
    sprintf(message, "  %d Total Pathnodes, %d of which are Edge Nodes.\n", path_node_count,
            edge_node_count);
    ReportBuildStatus(6, message);
    sprintf(message, "Total memory taken by Path Nodes: %dk.\n",
            (static_cast<unsigned int>(path_node_count) & 0x1fffffff) >> 7);
    ReportBuildStatus(6, message);
    m_ulNumWayPoints = 0;
    m_pFileWayPoints = 0;
    return 1;
}

// FUNCTION: WIZ8 0x004CE070
unsigned char PrePathing::CreateAutomapNodes(W8LevelFile* level)
{
    int last_percent = 0;
    W8GrowableVector<int> node_keys;
    W8HashTable<unsigned int, unsigned char> used_keys;
    char message[0x400];

    if (path_node_list == 0) {
        return 0;
    }
    SetAutomapGridCellSize(AutomapLevelIsLarge() ? 4000.0f : 2000.0f);
    int created = 0;
    unsigned int i;
    for (i = 1; i < static_cast<unsigned int>(path_node_count); ++i) {
        int percent = static_cast<int>(i * 100.0f / path_node_count);
        if (last_percent < percent) {
            sprintf(message, "Creating Automap Nodes:  %d%% Complete:  %d Nodes created  \r",
                    percent, created);
            ReportStartupMessage(message);
            last_percent = percent;
        }
        W8PrePathNode* node = path_node_list[i];
        srVector3T<float> position;
        position.Set((node->cell & 0xffff) * grid_scale,
                     (node->level_flags & W8_PATH_CELL_HEIGHT_MASK) * span,
                     (node->cell >> 0x10) * grid_scale);
        unsigned int key = AutomapNodeKey(&position);
        if (used_keys.FindNextEntry(&key, -1) == -1) {
            node_keys.Add(key);
            unsigned char present = 1;
            used_keys.Insert(&key, &present);
            ++created;
        }
    }
    used_keys.Clear();
    level->num_automap_nodes = node_keys.GetCount();
    sprintf(message, "Creating Automap Nodes:  100%% Complete:  %d Automap Nodes created  \n",
            node_keys.GetCount());
    ReportStartupMessage(message);
    sprintf(message, "  %d Total Automap Nodes.\n", level->num_automap_nodes);
    ReportBuildStatus(6, message);
    level->automap_nodes = static_cast<unsigned long*>(malloc(level->num_automap_nodes << 2));
    if (level->automap_nodes == 0) {
        level->num_automap_nodes = 0;
    } else {
        for (i = 0; i < static_cast<unsigned int>(level->num_automap_nodes); ++i) {
            level->automap_nodes[i] = *node_keys.GetAt(i);
        }
        QuickSort(level->automap_nodes, 0, level->num_automap_nodes - 1);
    }
    for (i = 1; i < static_cast<unsigned int>(path_node_count); ++i) {
        path_node_list[i] = 0;
    }
    free(path_node_list);
    path_node_list = 0;
    return 1;
}

// FUNCTION: WIZ8 0x004CE510
void W8PathingService::LinkCollideableProps(int lNumProps, W8PreProp* pPreProps,
                                            W8HashTable<unsigned int, CondPathNode*>* pCondValues)
{
    int aiLookup[10000];
    unsigned int aulKeys[10000];
    unsigned short ausFrames[10000];
    unsigned int aulValues[10000];
    int i;

    m_ulNumCondPaths = 1;
    m_ulNumCondNodes = 1;
    m_ulNumCondFrames = 1;
    m_pPathValues = new W8HashTable<unsigned int, unsigned int>;
    for (i = 0; i < path_node_count; ++i) {
        m_pPathValues->Insert(&file_path_nodes[i].cell, &file_path_nodes[i].level_flags);
    }

    GDPropCondPaths** ppCondPaths = static_cast<GDPropCondPaths**>(malloc(lNumProps * 4 + 8));
    if (ppCondPaths == 0) {
        srAssertFail("ppCondPaths", OCTPREPATH_CPP, 1037,
                     "LinkCollideableProps: Couldn't allocate GDPropCondPaths objects.");
    }
    memset(ppCondPaths, 0, lNumProps * 4 + 8);

    int ulOriginalCount = 0;
    for (i = 0; i < lNumProps; ++i) {
        ++ulOriginalCount;
        W8PreProp* pProp = pPreProps + i;
        if (pProp->num_stop_meshes != 0) {
            for (unsigned short j = 0; j < pProp->num_stop_meshes; ++j) {
                bool bWroteFrame = false;
                unsigned int key =
                    (static_cast<unsigned int>(pProp->pStopMeshes[j].m_prop_number) << 16) |
                    (i + 1);
                int slot = pCondValues->FindNextEntry(&key, -1);
                if (slot != -1) {
                    do {
                        CondPathNode* puValue = pCondValues->entries[slot].value;
                        if ((puValue != 0) &&
                            (FindConditionalPathValue(puValue->cell, puValue->value) != 0)) {
                            if (ppCondPaths[i] == 0) {
                                if (static_cast<unsigned int>(lNumProps) <
                                    static_cast<unsigned int>(ulOriginalCount)) {
                                    srAssertFail("ulOriginalCount <= (UINT32)lNumProps",
                                                 OCTPREPATH_CPP, 1065,
                                                 "LinkCollideableProps: Invalid count for "
                                                 "collideable props.");
                                }
                                GDPropCondPaths* pPath =
                                    static_cast<GDPropCondPaths*>(malloc(sizeof(GDPropCondPaths)));
                                ppCondPaths[i] = pPath;
                                memset(pPath, 0, sizeof(GDPropCondPaths));
                                strcpy(pPath->name, pProp->name);
                                pPath->lookup_index = m_ulNumCondFrames;
                                ++m_ulNumCondPaths;
                            }
                            if (!bWroteFrame) {
                                aiLookup[m_ulNumCondFrames + 1] = m_ulNumCondNodes;
                                ausFrames[m_ulNumCondFrames] = pProp->pStopMeshes[j].m_prop_number;
                                ++m_ulNumCondFrames;
                                if (m_ulNumCondFrames >= 10000) {
                                    srAssertFail("m_ulNumCondFrames < 10000", OCTPREPATH_CPP, 1077,
                                                 "LinkCollideableProps: Too many conditional "
                                                 "frames.");
                                }
                                bWroteFrame = true;
                            }
                            aulKeys[m_ulNumCondNodes] = puValue->cell;
                            aulValues[m_ulNumCondNodes] = puValue->value;
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
            if ((pProp->num_stop_meshes == 1) &&
                (pProp->pStopMeshes[0].m_flags.always_blocks_path)) {
                aiLookup[m_ulNumCondFrames + 1] = aiLookup[m_ulNumCondFrames];
                ausFrames[m_ulNumCondFrames] = pProp->pStopMeshes[0].last_frame;
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
        static_cast<GDPropCondPaths*>(malloc(m_ulNumCondPaths * sizeof(GDPropCondPaths)));
    if (m_pCondPaths == 0) {
        srAssertFail("m_pCondPaths", OCTPREPATH_CPP, 1116,
                     "LinkCollideableProps: Couldn't allocate m_pCondPaths array.");
    }
    memset(m_pCondPaths, 0, m_ulNumCondPaths * sizeof(GDPropCondPaths));
    m_ulNumCondPaths = 0;
    if (ulOriginalCount > 1) {
        for (i = 1; i < ulOriginalCount; ++i) {
            if (ppCondPaths[i] != 0) {
                int index = m_ulNumCondPaths;
                ++m_ulNumCondPaths;
                m_pCondPaths[index] = *ppCondPaths[i];
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
