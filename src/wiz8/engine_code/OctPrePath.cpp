#include "wiz8/engine_code/OctPath.h"
#include "wiz8/engine_code/GDProp.h"
#include "wiz8/sr_api.h"

#include <stdlib.h>
#include <string.h>

#define OCTPREPATH_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\OctPrePath.cpp"

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
                    ((unsigned int)pProp->pStopMeshes[j].m_prop_number_02 << 16) | (i + 1);
                int slot = pCondValues->FindNextEntry(&key, -1);
                if (slot != -1) {
                    do {
                        unsigned int* puValue = pCondValues->entries[slot].value;
                        if ((puValue != 0) &&
                            (FindConditionalPathValue00458970(puValue[1], *puValue) != 0)) {
                            if (ppCondPaths[i] == 0) {
                                if ((unsigned int)lNumProps < (unsigned int)ulOriginalCount) {
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

    if (((unsigned int)m_ulNumCondFrames < 2) || ((unsigned int)m_ulNumCondNodes < 2)) {
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
