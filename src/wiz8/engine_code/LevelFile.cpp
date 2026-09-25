#include "wiz8/engine_code/LevelFile.h"
#include "wiz8/engine_code/OctMeshModel.h"
#include "wiz8/sr_api.h"
#include "wiz8/local_screens/AutomapScreen.h"
#include "wiz8/float_constants.h"

#include "FileMan.h"
#include "DEBUG.H"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LEVELFILE_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\LevelFile.cpp"

// GLOBAL: WIZ8 0x006833fc
W8LevelFile* g_level_file_6833fc;

/* Scratch message buffer for the mesh/anim-object load failures; original name
   unknown. The next defined symbol is g_level_file_6833fc at 0x006833FC, so the
   buffer is at most 0x404 bytes; 0x400 matches this file's other 0x400 sprintf
   buffers. */
// GLOBAL: WIZ8 0x00682ff8
char g_level_file_error_682ff8[0x400];

// FUNCTION: WIZ8 0x004CFDC0
W8LevelFile* ReadLevelFile004CFDC0(int hFile)
{
    W8LevelFile* pLevel = static_cast<W8LevelFile*>(malloc(sizeof(W8LevelFile)));
    if (pLevel == 0) {
        srAssertFail("pLevel", LEVELFILE_CPP, 0x2b, 0);
    }
    memset(pLevel, 0, sizeof(W8LevelFile));
    pLevel->field_00 = 1;
    pLevel->field_04 = 1;
    pLevel->num_switch_triggers_6c1 = 0;
    memset(pLevel->switch_triggers_6c5, 0, sizeof(pLevel->switch_triggers_6c5));
    pLevel->num_invisible_planes_1665 = 0;
    memset(pLevel->invisible_planes_1669, 0, sizeof(pLevel->invisible_planes_1669));
    pLevel->num_linked_records_2609 = 0;
    memset(pLevel->linked_records_260d, 0, sizeof(pLevel->linked_records_260d));
    g_level_file_6833fc = pLevel;

    pLevel->pMeshes = static_cast<W8LevelFileMesh*>(malloc(sizeof(W8LevelFileMesh)));
    if (pLevel->pMeshes == 0) {
        srAssertFail("pLevel->pMeshes", LEVELFILE_CPP, 0x38, 0);
    }
    memset(pLevel->pMeshes, 0, sizeof(W8LevelFileMesh));
    ReadMeshFile(hFile, pLevel->pMeshes);

    FileRead(hFile, &pLevel->nTextures, sizeof(pLevel->nTextures), 0);
    if (pLevel->nTextures == 0) {
        return 0;
    }
    pLevel->pTextures =
        static_cast<W8MaterialRecord*>(malloc(pLevel->nTextures * sizeof(W8MaterialRecord)));
    if (pLevel->pTextures == 0) {
        srAssertFail("pLevel->pTextures", LEVELFILE_CPP, 0x41, 0);
    }
    memset(pLevel->pTextures, 0, pLevel->nTextures * sizeof(W8MaterialRecord));
    unsigned char fSuccess = 1;
    unsigned char ok;
    int i;
    for (i = 0; i < pLevel->nTextures; ++i) {
        W8MaterialRecord* pTexture = pLevel->pTextures + i;
        ok = FileRead(hFile, pTexture, 0x11a, 0);
        if (pTexture->version_00 >= 4) {
            ok &= FileRead(hFile, pTexture->texture_modes_11a, 0x10, 0);
        }
        if (ok == 0) {
            return 0;
        }
    }
    if (ok == 0) {
        return 0;
    }

    FileRead(hFile, &pLevel->nLights, sizeof(pLevel->nLights), 0);
    if (pLevel->nLights != 0) {
        pLevel->pLights =
            static_cast<W8LevelFileLight*>(malloc(pLevel->nLights * sizeof(W8LevelFileLight)));
        if (pLevel->pLights == 0) {
            srAssertFail("pLevel->pLights", LEVELFILE_CPP, 0x4d, 0);
        }
        memset(pLevel->pLights, 0, pLevel->nLights * sizeof(W8LevelFileLight));
        for (i = 0; i < pLevel->nLights; ++i) {
            if (ReadLightFile(hFile, pLevel->pLights + i) == 0) {
                return 0;
            }
        }
    }

    fSuccess = FileRead(hFile, &pLevel->nMonsters, sizeof(pLevel->nMonsters), 0);
    if (pLevel->nMonsters != 0) {
        pLevel->pMonsters = static_cast<W8LevelFileMonster*>(
            malloc(pLevel->nMonsters * sizeof(W8LevelFileMonster)));
        if (pLevel->pMonsters == 0) {
            srAssertFail("pLevel->pMonsters", LEVELFILE_CPP, 0x5d, 0);
        }
        memset(pLevel->pMonsters, 0, pLevel->nMonsters * sizeof(W8LevelFileMonster));
        for (i = 0; i < pLevel->nMonsters; ++i) {
            W8LevelFileMonster* pMonster = pLevel->pMonsters + i;
            fSuccess &= FileRead(hFile, pMonster, 0x22, 0);
            if (fSuccess == 0) {
                return 0;
            }
            if (pMonster->num_mon_path_1e != 0) {
                pMonster->MonPath_22 = static_cast<W8LevelFilePathNode*>(
                    malloc(pMonster->num_mon_path_1e * sizeof(W8LevelFilePathNode)));
                if (pMonster->MonPath_22 == 0) {
                    srAssertFail("pLevel->pMonsters[i1].MonPath", LEVELFILE_CPP, 0x69, 0);
                }
                memset(pMonster->MonPath_22, 0,
                       pMonster->num_mon_path_1e * sizeof(W8LevelFilePathNode));
                fSuccess &= FileRead(hFile, pMonster->MonPath_22,
                                     pMonster->num_mon_path_1e * sizeof(W8LevelFilePathNode), 0);
                if (fSuccess == 0) {
                    return 0;
                }
            }
        }
    }

    FileRead(hFile, &pLevel->nItems, sizeof(pLevel->nItems), 0);
    if (pLevel->nItems != 0) {
        pLevel->pItems = static_cast<W8LevelFileItemRecord*>(
            malloc(pLevel->nItems * sizeof(W8LevelFileItemRecord)));
        if (pLevel->pItems == 0) {
            srAssertFail("pLevel->pItems", LEVELFILE_CPP, 0x79, 0);
        }
        memset(pLevel->pItems, 0, pLevel->nItems * sizeof(W8LevelFileItemRecord));
        if (FileRead(hFile, pLevel->pItems, pLevel->nItems * sizeof(W8LevelFileItemRecord), 0) ==
            0) {
            return 0;
        }
    }

    FileRead(hFile, &pLevel->missile_count_2c, sizeof(pLevel->missile_count_2c), 0);
    fSuccess = FileRead(hFile, &pLevel->nProps, sizeof(pLevel->nProps), 0);
    if (pLevel->nProps != 0) {
        pLevel->pProps = ReadPropsFile(hFile, pLevel->nProps);
        if (pLevel->pProps == 0) {
            srAssertFail("pLevel->pProps", LEVELFILE_CPP, 0x89, 0);
        }
    }
    unsigned char fSuccess2 = FileRead(hFile, &pLevel->nBitmaps, sizeof(pLevel->nBitmaps), 0);
    if (pLevel->nBitmaps != 0) {
        pLevel->pBitmaps = ReadPropsFile(hFile, pLevel->nBitmaps);
        if (pLevel->pBitmaps == 0) {
            srAssertFail("pLevel->pBitmaps", LEVELFILE_CPP, 0x91, 0);
        }
    }

    fSuccess = FileRead(hFile, &pLevel->nCameras, sizeof(pLevel->nCameras), 0);
    fSuccess = fSuccess & fSuccess2;
    if (pLevel->nCameras != 0) {
        pLevel->pCameras =
            static_cast<W8LevelFileCamera*>(malloc(pLevel->nCameras * sizeof(W8LevelFileCamera)));
        if (pLevel->pCameras == 0) {
            srAssertFail("pLevel->pCameras", LEVELFILE_CPP, 0xa7, 0);
        }
        memset(pLevel->pCameras, 0, pLevel->nCameras * sizeof(W8LevelFileCamera));
        for (i = 0; i < pLevel->nCameras; ++i) {
            W8LevelFileCamera* pCamera = pLevel->pCameras + i;
            memset(pCamera, 0, sizeof(W8LevelFileCamera));
            unsigned char ok = FileRead(hFile, &pCamera->positional_00, 4, 0);
            ok &= FileRead(hFile, &pCamera->positional_04, 4, 0);
            ok &= FileRead(hFile, &pCamera->has_scale_08, 1, 0);
            ok &= FileRead(hFile, pCamera->positional_09, 0x14, 0);
            if (pCamera->has_scale_08 != 0) {
                ok &= FileRead(hFile, &pCamera->scale_1d, 4, 0);
            }
            fSuccess &= ReadPathAIFile(hFile, &pCamera->pathAI_21) & ok;
        }
        if (fSuccess == 0) {
            return 0;
        }
    }

    fSuccess &= FileRead(hFile, &pLevel->has_block_48, sizeof(pLevel->has_block_48), 0);
    if (pLevel->has_block_48 != 0) {
        fSuccess &= ReadLevelFileBlock(hFile, &pLevel->block_04c);
        if (fSuccess == 0) {
            return 0;
        }
    }

    fSuccess &= FileRead(hFile, &pLevel->nTriggers, sizeof(pLevel->nTriggers), 0);
    if (pLevel->nTriggers != 0) {
        pLevel->pTriggers = static_cast<W8LevelFileTrigger*>(
            malloc(pLevel->nTriggers * sizeof(W8LevelFileTrigger)));
        if (pLevel->pTriggers == 0) {
            srAssertFail("pLevel->pTriggers", LEVELFILE_CPP, 0xbc, 0);
        }
        memset(pLevel->pTriggers, 0, pLevel->nTriggers * sizeof(W8LevelFileTrigger));
        for (i = 0; i < pLevel->nTriggers; ++i) {
            fSuccess &= ReadTriggerFile(hFile, pLevel->pTriggers + i);
        }
        if (fSuccess == 0) {
            return 0;
        }
    }

    ok = FileRead(hFile, &pLevel->camera_mode_688, sizeof(pLevel->camera_mode_688), 0);
    ok &= FileRead(hFile, &pLevel->nClippingPlanes, sizeof(pLevel->nClippingPlanes), 0);
    ok &= fSuccess;
    if (pLevel->nClippingPlanes != 0) {
        ok &= FileRead(hFile, &pLevel->clipping_plane_version_690, 1, 0);
        pLevel->pClippingPlanes = static_cast<W8LevelFileClippingPlaneRecord*>(
            malloc(pLevel->nClippingPlanes * sizeof(W8LevelFileClippingPlaneRecord)));
        if (pLevel->pClippingPlanes == 0) {
            srAssertFail("pLevel->pClippingPlanes", LEVELFILE_CPP, 0xcb, 0);
        }
        ok &= FileRead(hFile, pLevel->pClippingPlanes,
                       pLevel->nClippingPlanes * sizeof(W8LevelFileClippingPlaneRecord), 0);
        if (ok == 0) {
            return 0;
        }
    }

    ok &=
        FileRead(hFile, &pLevel->environment_offset_695, sizeof(pLevel->environment_offset_695), 0);
    ok &= FileRead(hFile, &pLevel->nParticleSystems, sizeof(pLevel->nParticleSystems), 0);
    if (pLevel->nParticleSystems != 0) {
        pLevel->pParticleSystems = static_cast<W8LevelFileParticleSystem*>(
            malloc(pLevel->nParticleSystems * sizeof(W8LevelFileParticleSystem)));
        if (pLevel->pParticleSystems == 0) {
            srAssertFail("pLevel->pParticleSystems", LEVELFILE_CPP, 0xd6, 0);
        }
        for (i = 0; i < pLevel->nParticleSystems; ++i) {
            if (ok == 0) {
                return 0;
            }
            ok &= ReadParticleSystemFile(hFile, pLevel->pParticleSystems + i);
        }
        if (ok == 0) {
            return 0;
        }
    }

    ok &= FileRead(hFile, &pLevel->nNamedPositions, sizeof(pLevel->nNamedPositions), 0);
    if (pLevel->nNamedPositions != 0) {
        pLevel->pNamedPositions = static_cast<W8LevelFileNamedPosition*>(
            malloc(pLevel->nNamedPositions * sizeof(W8LevelFileNamedPosition)));
        if (pLevel->pNamedPositions == 0) {
            srAssertFail("pLevel->pNamedPositions", LEVELFILE_CPP, 0xe3, 0);
        }
        unsigned int uiBytesRead;
        for (i = 0; i < pLevel->nNamedPositions; ++i) {
            ok &= FileRead(hFile, pLevel->pNamedPositions + i, sizeof(W8LevelFileNamedPosition),
                           &uiBytesRead);
        }
        if (ok == 0) {
            return 0;
        }
        for (i = 0; i < pLevel->nNamedPositions; ++i) {
            W8LevelFileNamedPosition* pPosition = pLevel->pNamedPositions + i;
            ReportBuildStatus(
                5, reinterpret_cast<const char*>( // reinterpret-ok: String returns a logging buffer
                       String("Named Position: %s (%f, %f, %f)\n", pPosition->name_01,
                              pPosition->position_81.x, pPosition->position_81.y,
                              pPosition->position_81.z)));
        }
    }

    FileRead(hFile, pLevel->unknown_6b9, sizeof(pLevel->unknown_6b9), 0);
    pLevel->field_6bd = FileGetPos(hFile);
    return pLevel;
}

// FUNCTION: WIZ8 0x004D07C0
BOOLEAN WriteLevelFile004D07C0(int hFile, int hFileIn, W8LevelFile* pLevel)
{
    unsigned int uiBytes;
    unsigned char fSuccess;
    unsigned char ok;
    int iCount;
    int i;
    char buffer[0x400];
    unsigned int chunk;

    if (pLevel == 0) {
        srAssertFail("pLevel", LEVELFILE_CPP, 0x10c, 0);
    }
    if (hFile == 0) {
        srAssertFail("hFile", LEVELFILE_CPP, 0x10d, 0);
    }
    if (pLevel->pMeshes == 0) {
        srAssertFail("pLevel->pMeshes", LEVELFILE_CPP, 0x10e, 0);
    }
    FileWrite(hFile, &pLevel->field_00, 4, 0);
    FileWrite(hFile, &pLevel->field_04, 4, 0);
    FileWrite(hFile, &pLevel->nTextures, 2, 0);
    iCount = pLevel->nTextures;
    if (iCount == 0) {
        return FALSE;
    }
    ok = 1;
    for (i = 0; i < (short)iCount; ++i) {
        W8MaterialRecord* pTexture = pLevel->pTextures + i;
        ok = FileWrite(hFile, pTexture, 0x11a, 0);
        if (pTexture->version_00 >= 4) {
            ok &= FileWrite(hFile, pTexture->texture_modes_11a, 0x10, 0);
        }
        if (ok == 0) {
            return FALSE;
        }
    }
    if (ok == 0) {
        return FALSE;
    }
    if ((FileWrite(hFile, &iCount, 4, 0) & ok) == 0) {
        return FALSE;
    }
    free(pLevel->pTextures);
    if (pLevel->field_00 != 0) {
        OctMeshModel* pModel = pLevel->pModels_0c;
        for (i = 0; (unsigned int)i < (unsigned int)pLevel->field_00; ++i) {
            pModel->Write0049E5D0(hFile);
            ++pModel;
        }
    }
    FileWrite(hFile, &iCount, 4, 0);
    if (pLevel->pModels_0c != 0) {
        delete[] pLevel->pModels_0c;
    }
    if (pLevel->pMeshes != 0) {
        if (pLevel->pMeshes->pstVertices != 0) {
            free(pLevel->pMeshes->pstVertices);
        }
        if (pLevel->pMeshes->pstFaces != 0) {
            free(pLevel->pMeshes->pstFaces);
        }
        free(pLevel->pMeshes);
    }
    FileWrite(hFile, &pLevel->nLights, 2, 0);
    if (pLevel->nLights != 0) {
        for (i = 0; i < pLevel->nLights; ++i) {
            if (WriteLightFile(hFile, pLevel->pLights + i) == 0) {
                return FALSE;
            }
        }
        free(pLevel->pLights);
    }
    FileWrite(hFile, &iCount, 4, 0);
    fSuccess = FileWrite(hFile, &pLevel->nMonsters, 4, 0);
    if (pLevel->nMonsters != 0) {
        for (i = 0; i < pLevel->nMonsters; ++i) {
            W8LevelFileMonster* pMonster = pLevel->pMonsters + i;
            fSuccess &= FileWrite(hFile, pMonster, 0x22, 0);
            if (fSuccess == 0) {
                return FALSE;
            }
            if (pMonster->num_mon_path_1e != 0) {
                fSuccess &= FileWrite(hFile, pMonster->MonPath_22,
                                      pMonster->num_mon_path_1e * sizeof(W8LevelFilePathNode), 0);
                if (fSuccess == 0) {
                    return FALSE;
                }
                free(pMonster->MonPath_22);
            }
        }
        free(pLevel->pMonsters);
    }
    FileWrite(hFile, &iCount, 4, 0);
    FileWrite(hFile, &pLevel->nItems, 4, 0);
    if (pLevel->nItems != 0) {
        if (FileWrite(hFile, pLevel->pItems, pLevel->nItems * sizeof(W8LevelFileItemRecord), 0) ==
            0) {
            return FALSE;
        }
        free(pLevel->pItems);
    }
    FileWrite(hFile, &iCount, 4, 0);
    FileWrite(hFile, &pLevel->missile_count_2c, 4, 0);
    FileWrite(hFile, &iCount, 4, 0);
    FileWrite(hFile, &pLevel->nProps, 4, 0);
    if ((pLevel->nProps != 0) && (WritePropsFile(hFile, pLevel->nProps, pLevel->pProps) == 0)) {
        return FALSE;
    }
    FileWrite(hFile, &iCount, 4, 0);
    ok = FileWrite(hFile, &pLevel->nBitmaps, 4, 0);
    if ((pLevel->nBitmaps != 0) &&
        (ok = WritePropsFile(hFile, pLevel->nBitmaps, pLevel->pBitmaps), ok == 0)) {
        return FALSE;
    }
    fSuccess = FileWrite(hFile, &iCount, 4, 0);
    fSuccess = FileWrite(hFile, &pLevel->nCameras, 4, 0) & fSuccess & ok;
    if (pLevel->nCameras != 0) {
        if (pLevel->pCameras == 0) {
            srAssertFail("pLevel->pCameras", LEVELFILE_CPP, 0x189, 0);
        }
        for (i = 0; i < pLevel->nCameras; ++i) {
            W8LevelFileCamera* pCamera = pLevel->pCameras + i;
            unsigned char okCam = FileWrite(hFile, &pCamera->positional_00, 4, 0);
            okCam &= FileWrite(hFile, &pCamera->positional_04, 4, 0);
            okCam &= FileWrite(hFile, &pCamera->has_scale_08, 1, 0);
            okCam &= FileWrite(hFile, pCamera->positional_09, 0x14, 0);
            if (pCamera->has_scale_08 != 0) {
                okCam &= FileWrite(hFile, &pCamera->scale_1d, 4, 0);
            }
            fSuccess &= WritePathAIFile(hFile, &pCamera->pathAI_21) & okCam;
        }
        if (fSuccess == 0) {
            return FALSE;
        }
        free(pLevel->pCameras);
    }
    fSuccess = FileWrite(hFile, &iCount, 4, 0);
    fSuccess = FileWrite(hFile, &pLevel->has_block_48, 4, 0) & fSuccess;
    if (pLevel->has_block_48 != 0) {
        fSuccess &= WriteLevelFileBlock(hFile, &pLevel->block_04c);
        if (fSuccess == 0) {
            return FALSE;
        }
    }
    fSuccess = FileWrite(hFile, &iCount, 4, 0);
    fSuccess = FileWrite(hFile, &pLevel->nTriggers, 4, 0) & fSuccess;
    if (pLevel->nTriggers != 0) {
        if (pLevel->pTriggers == 0) {
            srAssertFail("pLevel->pTriggers", LEVELFILE_CPP, 0x1a0, 0);
        }
        for (i = 0; i < pLevel->nTriggers; ++i) {
            fSuccess &= WriteTriggerFile(hFile, pLevel->pTriggers + i);
        }
        if (fSuccess == 0) {
            return FALSE;
        }
        free(pLevel->pTriggers);
    }
    fSuccess = FileWrite(hFile, &iCount, 4, 0);
    fSuccess = FileWrite(hFile, &pLevel->camera_mode_688, 4, 0) & fSuccess;
    fSuccess = FileWrite(hFile, &pLevel->nClippingPlanes, 4, 0) & fSuccess;
    if (pLevel->nClippingPlanes != 0) {
        fSuccess &= FileWrite(hFile, &pLevel->clipping_plane_version_690, 1, 0);
        fSuccess &= FileWrite(hFile, pLevel->pClippingPlanes,
                              pLevel->nClippingPlanes * sizeof(W8LevelFileClippingPlaneRecord), 0);
        free(pLevel->pClippingPlanes);
        if (fSuccess == 0) {
            return FALSE;
        }
    }
    fSuccess = FileWrite(hFile, &iCount, 4, 0);
    fSuccess = FileWrite(hFile, &pLevel->environment_offset_695,
                         sizeof(pLevel->environment_offset_695), 0) &
               fSuccess;
    fSuccess = FileWrite(hFile, &pLevel->nParticleSystems, 4, 0) & fSuccess;
    if (pLevel->nParticleSystems != 0) {
        for (i = 0; i < pLevel->nParticleSystems; ++i) {
            if (fSuccess == 0) {
                break;
            }
            fSuccess &= WriteParticleSystemFile(hFile, pLevel->pParticleSystems + i);
        }
        free(pLevel->pParticleSystems);
        if (fSuccess == 0) {
            return FALSE;
        }
    }
    fSuccess = FileWrite(hFile, &iCount, 4, 0);
    fSuccess = FileWrite(hFile, &pLevel->nNamedPositions, 4, 0) & fSuccess;
    if (pLevel->nNamedPositions != 0) {
        fSuccess &= FileWrite(hFile, pLevel->pNamedPositions, pLevel->nNamedPositions * 0x9d, 0);
        free(pLevel->pNamedPositions);
        if (fSuccess == 0) {
            return FALSE;
        }
    }
    FileWrite(hFile, &iCount, 4, 0);
    float level_scale = GetFloat64B914();
    fSuccess = FileWrite(hFile, &level_scale, 4, 0) & fSuccess;
    fSuccess = FileWrite(hFile, &pLevel->num_automap_nodes_6b1, 4, 0) & fSuccess;
    if (pLevel->num_automap_nodes_6b1 != 0) {
        fSuccess &=
            FileWrite(hFile, pLevel->automap_nodes_6b5, pLevel->num_automap_nodes_6b1 * 4, 0);
        free(pLevel->automap_nodes_6b5);
        if (fSuccess == 0) {
            return FALSE;
        }
    }
    fSuccess = FileWrite(hFile, &iCount, 4, 0);
    fSuccess = FileWrite(hFile, pLevel->unknown_6b9, 4, 0) & fSuccess;
    chunk = 0x400;
    unsigned char fDone;
    do {
        fDone = 0;
        if (fSuccess == 0) {
            break;
        }
        fDone = FileRead(hFileIn, buffer, 0x400, &uiBytes);
        fSuccess &= fDone;
        if (uiBytes < 0x400) {
            fSuccess = 1;
            chunk = uiBytes;
        }
        fDone = FileWrite(hFile, buffer, chunk, &uiBytes);
        fSuccess &= fDone;
    } while (chunk == 0x400);
    pLevel->num_switch_triggers_6c1 = 0;
    pLevel->num_invisible_planes_1665 = 0;
    free(pLevel);
    return fSuccess;
}

// FUNCTION: WIZ8 0x004D1110
BOOLEAN ReadMeshFile(int hFile, W8LevelFileMesh* pMesh)
{
    int i;
    memset(pMesh, 0, sizeof(W8LevelFileMesh));
    BOOLEAN fSuccess = TRUE;
    fSuccess &= FileRead(hFile, &pMesh->version_00, 4, 0);
    fSuccess &= FileRead(hFile, &pMesh->num_vertices_04, 4, 0);
    fSuccess &= FileRead(hFile, &pMesh->num_faces_08, 4, 0);
    if (fSuccess == 0) {
        return FALSE;
    }
    int count = pMesh->num_vertices_04;
    if (count >= 0x186a1 || count <= 0) {
        sprintf(g_level_file_error_682ff8, "Invalid number of vertices in mesh (%d vertices).\n",
                count);
        ReportBuildStatus(7, g_level_file_error_682ff8);
        return FALSE;
    }
    count = pMesh->num_faces_08;
    if (count >= 0x30d41 || count <= 0) {
        sprintf(g_level_file_error_682ff8, "Invalid number of faces in mesh (%d faces).\n", count);
        ReportBuildStatus(7, g_level_file_error_682ff8);
        return FALSE;
    }
    if (pMesh->version_00 >= 3) {
        fSuccess = FileRead(hFile, &pMesh->flags_0c, 1, 0);
    }
    if (pMesh->version_00 >= 2) {
        fSuccess &= FileRead(hFile, &pMesh->location_10, 0xc, 0);
        fSuccess &= FileRead(hFile, &pMesh->rotation_angle_1c, 0x10, 0);
        fSuccess &= FileRead(hFile, &pMesh->scale_2c, 0xc, 0);
    }
    if (pMesh->version_00 >= 4) {
        fSuccess &= FileRead(hFile, &pMesh->mapping_count_38, 1, 0);
        if (pMesh->mapping_count_38 != 0) {
            fSuccess &= FileRead(hFile, &pMesh->mapped_value_3c, 4, 0);
        }
    }
    if (fSuccess == 0) {
        return FALSE;
    }
    if ((pMesh->flags_0c & 1) == 0) {
        pMesh->pstVertices = static_cast<float*>(malloc(pMesh->num_vertices_04 * 0x18));
        if (pMesh->pstVertices == 0) {
            srAssertFail("pMesh->pstVertices", LEVELFILE_CPP, 0x29a, 0);
        }
        memset(pMesh->pstVertices, 0, pMesh->num_vertices_04 * 0x18);
        if (FileRead(hFile, pMesh->pstVertices, pMesh->num_vertices_04 * 0xc, 0) == 0) {
            return FALSE;
        }
    } else {
        fSuccess &= FileRead(hFile, &pMesh->lod_mode_40, 1, 0);
        fSuccess &= FileRead(hFile, &pMesh->num_lods_42, 2, 0);
        if ((pMesh->flags_0c & 2) == 0) {
            float** pLods = static_cast<float**>(malloc(pMesh->num_lods_42 * 4));
            if (pLods == 0) {
                return FALSE;
            }
            for (i = 0; i < pMesh->num_lods_42; ++i) {
                pLods[i] = static_cast<float*>(malloc(pMesh->num_vertices_04 * 0xc));
                if (pLods[i] == 0) {
                    return FALSE;
                }
                fSuccess &= FileRead(hFile, pLods[i], pMesh->num_vertices_04 * 0xc, 0);
                if (fSuccess == 0) {
                    return FALSE;
                }
            }
            pMesh->lods_48 = pLods;
        } else {
            if (pMesh->lod_mode_40 >= 2) {
                FileRead(hFile, &pMesh->lod_scale_58, 4, 0);
            }
            short** pLods = static_cast<short**>(malloc(pMesh->num_lods_42 * 4));
            if (pLods == 0) {
                return FALSE;
            }
            for (i = 0; i < pMesh->num_lods_42; ++i) {
                pLods[i] = static_cast<short*>(malloc(pMesh->num_vertices_04 * 6));
                if (pLods[i] == 0) {
                    return FALSE;
                }
                fSuccess &= FileRead(hFile, pLods[i], pMesh->num_vertices_04 * 6, 0);
                if (fSuccess == 0) {
                    return FALSE;
                }
            }
            pMesh->lod_shorts_44 = pLods;
        }
    }
    if ((pMesh->flags_0c & 4) != 0) {
        pMesh->pstCompFaces =
            static_cast<W8LevelFileCompressedFace*>(malloc(pMesh->num_faces_08 * 0x21));
        if (pMesh->pstCompFaces == 0) {
            srAssertFail("pMesh->pstCompFaces", LEVELFILE_CPP, 0x2a7, 0);
        }
        memset(pMesh->pstCompFaces, 0, pMesh->num_faces_08 * 0x21);
        return FileRead(hFile, pMesh->pstCompFaces, pMesh->num_faces_08 * 0x21, 0);
    }
    pMesh->pstFaces = static_cast<W8ReadMeshFace*>(malloc(pMesh->num_faces_08 * 0x52));
    if (pMesh->pstFaces == 0) {
        srAssertFail("pMesh->pstFaces", LEVELFILE_CPP, 0x2b2, 0);
    }
    memset(pMesh->pstFaces, 0, pMesh->num_faces_08 * 0x52);
    return FileRead(hFile, pMesh->pstFaces, pMesh->num_faces_08 * 0x29, 0);
}

// FUNCTION: WIZ8 0x004D1510
BOOLEAN WriteMeshFile(int hFile, W8LevelFileMesh* pMesh)
{
    BOOLEAN fSuccess = TRUE;
    fSuccess &= FileWrite(hFile, &pMesh->version_00, 4, 0);
    fSuccess &= FileWrite(hFile, &pMesh->num_vertices_04, 4, 0);
    fSuccess &= FileWrite(hFile, &pMesh->num_faces_08, 4, 0);
    if (fSuccess == 0) {
        return FALSE;
    }
    if (pMesh->version_00 >= 3) {
        fSuccess = FileWrite(hFile, &pMesh->flags_0c, 1, 0);
    }
    if (pMesh->version_00 >= 2) {
        fSuccess &= FileWrite(hFile, &pMesh->location_10, 0xc, 0);
        fSuccess &= FileWrite(hFile, &pMesh->rotation_angle_1c, 0x10, 0);
        fSuccess &= FileWrite(hFile, &pMesh->scale_2c, 0xc, 0);
    }
    if (pMesh->version_00 >= 4) {
        fSuccess &= FileWrite(hFile, &pMesh->mapping_count_38, 1, 0);
        if (pMesh->mapping_count_38 != 0) {
            fSuccess &= FileWrite(hFile, &pMesh->mapped_value_3c, 4, 0);
        }
    }
    if (fSuccess == 0) {
        return FALSE;
    }
    if ((pMesh->flags_0c & 1) == 0) {
        if (pMesh->pstVertices == 0) {
            srAssertFail("pMesh->pstVertices", LEVELFILE_CPP, 0x315, 0);
        }
        if (FileWrite(hFile, pMesh->pstVertices, pMesh->num_vertices_04 * 0xc, 0) == 0) {
            ReportBuildStatus(7, "WriteFileMesh: Could not write mesh vertices.\n");
        }
    } else {
        fSuccess &= FileWrite(hFile, &pMesh->lod_mode_40, 1, 0);
        fSuccess &= FileWrite(hFile, &pMesh->num_lods_42, 2, 0);
        if (pMesh->lod_mode_40 >= 2) {
            fSuccess &= FileWrite(hFile, &pMesh->lod_scale_58, 4, 0);
        }
        if ((pMesh->flags_0c & 2) == 0) {
            float** pLods = pMesh->lods_48;
            if (pLods == 0) {
                return FALSE;
            }
            for (int i = 0; i < pMesh->num_lods_42; ++i) {
                if (pLods[i] == 0) {
                    return FALSE;
                }
                fSuccess &= FileWrite(hFile, pLods[i], pMesh->num_vertices_04 * 0xc, 0);
                if (fSuccess == 0) {
                    ReportBuildStatus(7, "WriteFileMesh: Could not write mesh vertices.\n");
                }
                free(pLods[i]);
            }
        } else {
            short** pLods = pMesh->lod_shorts_44;
            if (pLods == 0) {
                return FALSE;
            }
            for (int i = 0; i < pMesh->num_lods_42; ++i) {
                if (pLods[i] == 0) {
                    return FALSE;
                }
                fSuccess &= FileWrite(hFile, pLods[i], pMesh->num_vertices_04 * 6, 0);
                if (fSuccess == 0) {
                    return FALSE;
                }
                free(pLods[i]);
            }
        }
    }
    free(pMesh->pstVertices);
    if ((pMesh->flags_0c & 4) != 0) {
        if (pMesh->pstCompFaces == 0) {
            srAssertFail("pMesh->pstCompFaces", LEVELFILE_CPP, 799, 0);
        }
        unsigned char ok = FileWrite(hFile, pMesh->pstCompFaces, pMesh->num_faces_08 * 0x21, 0);
        free(pMesh->pstCompFaces);
        return ok;
    }
    if (pMesh->pstFaces == 0) {
        srAssertFail("pMesh->pstFaces", LEVELFILE_CPP, 0x326, 0);
    }
    unsigned char ok = FileWrite(hFile, pMesh->pstFaces, pMesh->num_faces_08 * 0x29, 0);
    free(pMesh->pstFaces);
    return ok;
}

// FUNCTION: WIZ8 0x004D1820
BOOLEAN ReadLightFile(int hFile, W8LevelFileLight* pLight)
{
    BOOLEAN fSuccess = TRUE;
    fSuccess &= FileRead(hFile, &pLight->version_00, 2, 0);
    fSuccess &= FileRead(hFile, &pLight->create_02, 4, 0);
    fSuccess &= FileRead(hFile, pLight->unknown_06, 2, 0);
    fSuccess &= FileRead(hFile, &pLight->position_08, 0xc, 0);
    fSuccess &= FileRead(hFile, &pLight->colour_14, 0xc, 0);
    fSuccess &= FileRead(hFile, &pLight->intensity_20, 4, 0);
    fSuccess &= FileRead(hFile, &pLight->range_24, 4, 0);
    if (fSuccess == 0) {
        return FALSE;
    }
    if (pLight->version_00 >= 2) {
        fSuccess = FileRead(hFile, pLight->name_28, 0x14, 0) != 0;
        if ((pLight->flags_04 & 2) != 0) {
            pLight->create_02 = 1;
            pLight->pExtra_3c =
                static_cast<W8LevelFileLightExtra*>(malloc(sizeof(W8LevelFileLightExtra)));
            if (pLight->pExtra_3c == 0) {
                return FALSE;
            }
            fSuccess &= FileRead(hFile, pLight->pExtra_3c, sizeof(W8LevelFileLightExtra), 0);
            if (fSuccess == 0) {
                return FALSE;
            }
            if ((pLight->pExtra_3c->flags_00 & 0x10) != 0) {
                pLight->pPathAI_40 =
                    static_cast<W8LevelFilePathAI*>(malloc(sizeof(W8LevelFilePathAI)));
                if (pLight->pPathAI_40 == 0) {
                    return FALSE;
                }
                fSuccess = ReadPathAIFile(hFile, pLight->pPathAI_40);
            }
        }
    }
    if (fSuccess == 0) {
        srAssertFail("fSuccess", LEVELFILE_CPP, 0x35f, "Couldn't read light.");
    }
    return fSuccess;
}

// FUNCTION: WIZ8 0x004D1960
BOOLEAN WriteLightFile(int hFile, W8LevelFileLight* pLight)
{
    BOOLEAN fSuccess = TRUE;
    fSuccess &= FileWrite(hFile, &pLight->version_00, 2, 0);
    fSuccess &= FileWrite(hFile, &pLight->create_02, 4, 0);
    fSuccess &= FileWrite(hFile, pLight->unknown_06, 2, 0);
    fSuccess &= FileWrite(hFile, &pLight->position_08, 0xc, 0);
    fSuccess &= FileWrite(hFile, &pLight->colour_14, 0xc, 0);
    fSuccess &= FileWrite(hFile, &pLight->intensity_20, 4, 0);
    fSuccess &= FileWrite(hFile, &pLight->range_24, 4, 0);
    if (fSuccess == 0) {
        return FALSE;
    }
    if (pLight->version_00 >= 2) {
        fSuccess = FileWrite(hFile, pLight->name_28, 0x14, 0) != 0;
        if (((pLight->flags_04 & 2) != 0) && (pLight->pExtra_3c != 0)) {
            fSuccess &= FileWrite(hFile, pLight->pExtra_3c, sizeof(W8LevelFileLightExtra), 0);
            if (fSuccess == 0) {
                return FALSE;
            }
            if (((pLight->pExtra_3c->flags_00 & 0x10) != 0) && (pLight->pPathAI_40 != 0)) {
                fSuccess = WritePathAIFile(hFile, pLight->pPathAI_40);
                free(pLight->pPathAI_40);
            }
            free(pLight->pExtra_3c);
        }
    }
    if (fSuccess == 0) {
        srAssertFail("fSuccess", LEVELFILE_CPP, 0x38e, "Couldn't Write light.");
    }
    return fSuccess;
}

// FUNCTION: WIZ8 0x004D1A90
BOOLEAN ReadAnimLightFile(int hFile, W8LevelFileAnimLight* pLight)
{
    BOOLEAN fSuccess = TRUE;
    fSuccess &= FileRead(hFile, &pLight->version_00, 1, 0);
    fSuccess &= FileRead(hFile, &pLight->position_01, 0xc, 0);
    fSuccess &= FileRead(hFile, &pLight->color_0d, 0xc, 0);
    fSuccess &= FileRead(hFile, &pLight->intensity_19, 4, 0);
    fSuccess &= FileRead(hFile, &pLight->range_1d, 4, 0);
    if (fSuccess == 0) {
        return FALSE;
    }
    if (pLight->version_00 >= 2) {
        pLight->pExtra_21 =
            static_cast<W8LevelFileLightExtra*>(malloc(sizeof(W8LevelFileLightExtra)));
        if (pLight->pExtra_21 == 0) {
            return FALSE;
        }
        fSuccess &= FileRead(hFile, pLight->pExtra_21, sizeof(W8LevelFileLightExtra), 0);
    }
    if (fSuccess == 0) {
        srAssertFail("fSuccess", LEVELFILE_CPP, 0x3b2, "Couldn't read anim light.");
    }
    return fSuccess;
}

// FUNCTION: WIZ8 0x004D1B50
BOOLEAN WriteAnimLightFile(int hFile, W8LevelFileAnimLight* pLight)
{
    BOOLEAN fSuccess = TRUE;
    fSuccess &= FileWrite(hFile, &pLight->version_00, 1, 0);
    fSuccess &= FileWrite(hFile, &pLight->position_01, 0xc, 0);
    fSuccess &= FileWrite(hFile, &pLight->color_0d, 0xc, 0);
    fSuccess &= FileWrite(hFile, &pLight->intensity_19, 4, 0);
    fSuccess &= FileWrite(hFile, &pLight->range_1d, 4, 0);
    if (fSuccess == 0) {
        return FALSE;
    }
    if ((pLight->version_00 >= 2) && (pLight->pExtra_21 != 0)) {
        fSuccess &= FileWrite(hFile, pLight->pExtra_21, sizeof(W8LevelFileLightExtra), 0);
        free(pLight->pExtra_21);
    }
    if (fSuccess == 0) {
        srAssertFail("fSuccess", LEVELFILE_CPP, 0x3d4, "Couldn't write anim light.");
    }
    return fSuccess;
}

// FUNCTION: WIZ8 0x004D1C10
BOOLEAN ReadTriggerFile(int hFile, W8LevelFileTrigger* pTrigger)
{
    bool fSuccess = false;
    unsigned char ok;

    ok = FileRead(hFile, &pTrigger->version_00, 1, 0);
    if ((ok != 0) && (ok = FileRead(hFile, &pTrigger->type_01, 1, 0), ok != 0)) {
        fSuccess = 1;
    }
    switch (pTrigger->type_01) {
    case 1: {
        W8LevelFileSwitch* pSwitch = static_cast<W8LevelFileSwitch*>(malloc(0x271));
        if (pSwitch == 0) {
            srAssertFail("pSwitch", LEVELFILE_CPP, 0x3f8, 0);
        }
        memset(pSwitch, 0, sizeof(W8LevelFileSwitch));
        ok = FileRead(hFile, &pSwitch->version_00, 1, 0);
        ok &= FileRead(hFile, &pSwitch->cycle_bounce_01, 4, 0);
        ok &= FileRead(hFile, &pSwitch->state_count_05, 4, 0);
        ok &= FileRead(hFile, &pSwitch->flag_09, 4, 0);
        ok &= FileRead(hFile, &pSwitch->range_0d, 4, 0);
        ok &= FileRead(hFile, &pSwitch->action_11, 4, 0);
        ok &= FileRead(hFile, &pSwitch->value_15, 4, 0);
        ok &= FileRead(hFile, &pSwitch->flag_19, 4, 0);
        ok &= FileRead(hFile, &pSwitch->packed_flags_1d, 1, 0);
        ok &= FileRead(hFile, &pSwitch->enabled_1e, 1, 0);
        ok &= FileRead(hFile, pSwitch->name_1f, 0x80, 0);
        ok &= FileRead(hFile, pSwitch->recipients_9f, 0x100, 0);
        ok &= FileRead(hFile, pSwitch->sound_19f, 0x80, 0);
        ok &= fSuccess;
        if (ok == 0) {
            srAssertFail("fSuccess", LEVELFILE_CPP, 0x408, 0);
        }
        ReportBuildStatus(
            5,
            reinterpret_cast< // reinterpret-ok: String returns a logging buffer
                const char*>(String(
                "Switch Trigger: %s, recipients: %s\n", // reinterpret-ok: String returns a logging buffer
                pSwitch->name_1f, pSwitch->recipients_9f)));
        if (pSwitch->version_00 > 1) {
            ok &= FileRead(hFile, &pSwitch->minimum_range_21f, 4, 0);
            ok &= FileRead(hFile, pSwitch->surface_id_223, 0x40, 0);
            ReportBuildStatus(
                5, reinterpret_cast<const char*>( // reinterpret-ok: String returns a logging buffer
                       String("Switch Trigger name: %s\n", pSwitch->surface_id_223)));
        }
        if (pSwitch->version_00 > 2) {
            ok &= FileRead(hFile, &pSwitch->has_door_trigger_263, 1, 0);
            if (pSwitch->has_door_trigger_263 != 0) {
                ok &= FileRead(hFile, &pSwitch->door_264.kind_00, 1, 0);
                if (pSwitch->door_264.kind_00 == 1) {
                    ok &= ReadDoorTriggerFile004D3540(hFile, &pSwitch->door_264);
                    if (ok == 0) {
                        ReportBuildStatus(7, "Problem reading door trigger.\n");
                        return FALSE;
                    }
                }
            }
        }
        if (pSwitch->version_00 > 3) {
            ok &= FileRead(hFile, &pSwitch->action_value_26d, 4, 0);
        }
        pTrigger->pData_02 = pSwitch;
        g_level_file_6833fc->switch_triggers_6c5[g_level_file_6833fc->num_switch_triggers_6c1] =
            pSwitch;
        ++g_level_file_6833fc->num_switch_triggers_6c1;
        return ok;
    }
    case 3: {
        W8LevelFileSound* pSound = static_cast<W8LevelFileSound*>(malloc(0x170));
        if (pSound == 0) {
            srAssertFail("pSound", LEVELFILE_CPP, 0x46c, 0);
        }
        memset(pSound, 0, sizeof(W8LevelFileSound));
        ok = FileRead(hFile, &pSound->version_00, 1, 0);
        ok &= FileRead(hFile, &pSound->volume_min_01, 4, 0);
        ok &= FileRead(hFile, &pSound->volume_max_05, 4, 0);
        ok &= FileRead(hFile, &pSound->speed_min_09, 4, 0);
        ok &= FileRead(hFile, &pSound->speed_max_0d, 4, 0);
        ok &= FileRead(hFile, &pSound->time_min_11, 4, 0);
        ok &= FileRead(hFile, &pSound->time_max_15, 4, 0);
        ok &= FileRead(hFile, &pSound->unbounded_19, 4, 0);
        ok &= FileRead(hFile, &pSound->radius_1d, 4, 0);
        ok &= FileRead(hFile, &pSound->position_21, 0xc, 0);
        ok &= FileRead(hFile, &pSound->region_u_2d, 0xc, 0);
        ok &= FileRead(hFile, &pSound->region_v_39, 0xc, 0);
        ok &= FileRead(hFile, pSound->wave_45, 0x80, 0);
        ok &= fSuccess;
        if (pSound->version_00 > 1) {
            ok &= FileRead(hFile, &pSound->has_position_c5, 1, 0);
            ok &= FileRead(hFile, &pSound->looping_c6, 1, 0);
        }
        if (pSound->version_00 > 2) {
            ok &= FileRead(hFile, &pSound->region_center_c7, 0xc, 0);
            ok &= FileRead(hFile, &pSound->region_angle_d3, 4, 0);
            ok &= FileRead(hFile, &pSound->region_min_d7, 0xc, 0);
            ok &= FileRead(hFile, &pSound->region_max_e3, 0xc, 0);
        }
        if (pSound->version_00 > 3) {
            ok &= FileRead(hFile, pSound->name_ef, 0x80, 0);
            ReportBuildStatus(
                5, reinterpret_cast< // reinterpret-ok: String returns a logging buffer
                       const char*>(
                       String("Sound Trigger: %s\n",
                              pSound->name_ef))); // reinterpret-ok: String returns a logging buffer
        }
        if (pSound->version_00 > 4) {
            ok &= FileRead(hFile, &pSound->shared_16f, 1, 0);
        }
        pTrigger->pData_02 = pSound;
        return ok;
    }
    case 4:
        return ReadSuperTriggerFile(hFile, pTrigger) & fSuccess;
    case 2:
        break;
    default:
        return fSuccess;
    }

    W8LevelFileInvisible* pInvis = static_cast<W8LevelFileInvisible*>(malloc(0x241));
    if (pInvis == 0) {
        srAssertFail("pInvis", LEVELFILE_CPP, 0x42f, 0);
    }
    memset(pInvis, 0, sizeof(W8LevelFileInvisible));
    ok = FileRead(hFile, &pInvis->version_00, 1, 0);
    ok &= FileRead(hFile, &pInvis->range_01, 4, 0);
    ok &= FileRead(hFile, &pInvis->position_05, 0xc, 0);
    ok &= FileRead(hFile, &pInvis->action_11, 4, 0);
    ok &= FileRead(hFile, &pInvis->searchable_15, 4, 0);
    ok &= FileRead(hFile, &pInvis->fire_linked_19, 1, 0);
    ok &= FileRead(hFile, &pInvis->enabled_1a, 1, 0);
    ok &= FileRead(hFile, pInvis->name_1b, 0x80, 0);
    ok &= FileRead(hFile, pInvis->recipients_9b, 0x100, 0);
    ok &= fSuccess;
    ReportBuildStatus(
        5,
        reinterpret_cast< // reinterpret-ok: String returns a logging buffer
            const char*>(String(
            "Invisible Trigger: %s, recipients: %s\n", // reinterpret-ok: String returns a logging buffer
            pInvis->name_1b, pInvis->recipients_9b)));
    if (pInvis->version_00 > 1) {
        ok &= FileRead(hFile, &pInvis->plane_flag_19b, 1, 0);
        pInvis->pPlane_19c = static_cast<W8LevelFilePlane*>(malloc(0x30));
        if (pInvis->pPlane_19c == 0) {
            srAssertFail("pInvis->pPlane", LEVELFILE_CPP, 0x441, 0);
        }
        memset(pInvis->pPlane_19c, 0, sizeof(W8LevelFilePlane));
        ok &= FileRead(hFile, pInvis->pPlane_19c, 0x30, 0);
    }
    if (pInvis->version_00 > 2) {
        ok &= FileRead(hFile, &pInvis->angle_1a0, 4, 0);
        ok &= FileRead(hFile, &pInvis->direction_1a4, 0xc, 0);
        ok &= FileRead(hFile, &pInvis->unused_1b0, 1, 0);
        ok &= FileRead(hFile, pInvis->action_string_1b1, 0x80, 0);
    }
    if (pInvis->version_00 > 3) {
        ok &= FileRead(hFile, &pInvis->flag_231, 1, 0);
        ok &= FileRead(hFile, &pInvis->action_value_232, 4, 0);
    }
    if (pInvis->version_00 > 4) {
        ok &= FileRead(hFile, &pInvis->has_legacy_geometry_236, 1, 0);
        if (pInvis->has_legacy_geometry_236 != 0) {
            ok &= FileRead(hFile, &pInvis->geometry_kind_238, 1, 0);
            if (pInvis->field_237 == 2) {
                W8LevelFileLinkedRecord* pRecord =
                    static_cast<W8LevelFileLinkedRecord*>(malloc(0x1bb));
                unsigned char okRecord = 0;
                if (pRecord != 0) {
                    okRecord = FileRead(hFile, &pRecord->kind_00, 1, 0);
                    okRecord &= FileRead(hFile, pRecord->vertices_01, 0x1b0, 0);
                    okRecord &= FileRead(hFile, &pRecord->linked_face_1b1, 2, 0);
                    g_level_file_6833fc
                        ->linked_records_260d[g_level_file_6833fc->num_linked_records_2609] =
                        pRecord;
                    ++g_level_file_6833fc->num_linked_records_2609;
                    pInvis->pRecord_23d = pRecord;
                }
                if ((ok & okRecord) != 0) {
                    pInvis->pRecord_23d->normal_scale_1b3 = pInvis->range_01;
                    pInvis->pRecord_23d->forward_scale_1b7 = 1.0f;
                    pTrigger->pData_02 = pInvis;
                    return ok & okRecord;
                }
                return FALSE;
            }
        }
    }
    g_level_file_6833fc->invisible_planes_1669[g_level_file_6833fc->num_invisible_planes_1665] =
        pInvis->pPlane_19c;
    ++g_level_file_6833fc->num_invisible_planes_1665;
    pTrigger->pData_02 = pInvis;
    return ok;
}

// FUNCTION: WIZ8 0x004D23F0
BOOLEAN WriteTriggerFile(int hFile, W8LevelFileTrigger* pTrigger)
{
    bool fSuccess;
    unsigned char ok;

    ok = FileWrite(hFile, &pTrigger->version_00, 1, 0);
    if ((ok != 0) && (FileWrite(hFile, &pTrigger->type_01, 1, 0) != 0)) {
        fSuccess = 1;
    } else {
        fSuccess = 0;
    }
    fSuccess = (ok != 0) & fSuccess;
    switch (pTrigger->type_01) {
    case 1: {
        W8LevelFileSwitch* pSwitch = static_cast<W8LevelFileSwitch*>(pTrigger->pData_02);
        if (pSwitch == 0) {
            srAssertFail("pSwitch", LEVELFILE_CPP, 0x4be, 0);
        }
        ok = FileWrite(hFile, &pSwitch->version_00, 1, 0);
        ok &= FileWrite(hFile, &pSwitch->cycle_bounce_01, 4, 0);
        ok &= FileWrite(hFile, &pSwitch->state_count_05, 4, 0);
        ok &= FileWrite(hFile, &pSwitch->flag_09, 4, 0);
        ok &= FileWrite(hFile, &pSwitch->range_0d, 4, 0);
        ok &= FileWrite(hFile, &pSwitch->action_11, 4, 0);
        ok &= FileWrite(hFile, &pSwitch->value_15, 4, 0);
        ok &= FileWrite(hFile, &pSwitch->flag_19, 4, 0);
        ok &= FileWrite(hFile, &pSwitch->packed_flags_1d, 1, 0);
        ok &= FileWrite(hFile, &pSwitch->enabled_1e, 1, 0);
        ok &= FileWrite(hFile, pSwitch->name_1f, 0x80, 0);
        ok &= FileWrite(hFile, pSwitch->recipients_9f, 0x100, 0);
        ok &= FileWrite(hFile, pSwitch->sound_19f, 0x80, 0);
        ok &= fSuccess;
        if (ok == 0) {
            srAssertFail("fSuccess", LEVELFILE_CPP, 0x4cd, 0);
        }
        if (pSwitch->version_00 > 1) {
            ok &= FileWrite(hFile, &pSwitch->minimum_range_21f, 4, 0);
            ok &= FileWrite(hFile, pSwitch->surface_id_223, 0x40, 0);
        }
        if (pSwitch->version_00 > 2) {
            ok &= FileWrite(hFile, &pSwitch->has_door_trigger_263, 1, 0);
            if (pSwitch->has_door_trigger_263 != 0) {
                ok &= FileWrite(hFile, &pSwitch->door_264.kind_00, 1, 0);
                if (pSwitch->door_264.kind_00 == 1) {
                    ok &= WriteDoorTriggerFile(hFile, &pSwitch->door_264);
                }
            }
        }
        if (pSwitch->version_00 > 3) {
            ok &= FileWrite(hFile, &pSwitch->action_value_26d, 4, 0);
        }
        free(pSwitch);
        return ok;
    }
    case 2: {
        W8LevelFileInvisible* pInvis = static_cast<W8LevelFileInvisible*>(pTrigger->pData_02);
        if (pInvis == 0) {
            srAssertFail("pInvis", LEVELFILE_CPP, 0x4ea, 0);
        }
        ok = FileWrite(hFile, &pInvis->version_00, 1, 0);
        ok &= FileWrite(hFile, &pInvis->range_01, 4, 0);
        ok &= FileWrite(hFile, &pInvis->position_05, 0xc, 0);
        ok &= FileWrite(hFile, &pInvis->action_11, 4, 0);
        ok &= FileWrite(hFile, &pInvis->searchable_15, 4, 0);
        ok &= FileWrite(hFile, &pInvis->fire_linked_19, 1, 0);
        ok &= FileWrite(hFile, &pInvis->enabled_1a, 1, 0);
        ok &= FileWrite(hFile, pInvis->name_1b, 0x80, 0);
        ok &= FileWrite(hFile, pInvis->recipients_9b, 0x100, 0);
        ok &= fSuccess;
        if (pInvis->version_00 > 1) {
            ok &= FileWrite(hFile, &pInvis->plane_flag_19b, 1, 0);
            ok &= FileWrite(hFile, pInvis->pPlane_19c, 0x30, 0);
            free(pInvis->pPlane_19c);
        }
        if (pInvis->version_00 > 2) {
            ok &= FileWrite(hFile, &pInvis->angle_1a0, 4, 0);
            ok &= FileWrite(hFile, &pInvis->direction_1a4, 0xc, 0);
            ok &= FileWrite(hFile, &pInvis->unused_1b0, 1, 0);
            ok &= FileWrite(hFile, pInvis->action_string_1b1, 0x80, 0);
        }
        if (pInvis->version_00 > 3) {
            ok &= FileWrite(hFile, &pInvis->flag_231, 1, 0);
            ok &= FileWrite(hFile, &pInvis->action_value_232, 4, 0);
        }
        if (pInvis->version_00 > 4) {
            ok &= FileWrite(hFile, &pInvis->has_legacy_geometry_236, 1, 0);
            if (pInvis->has_legacy_geometry_236 != 0) {
                ok &= FileWrite(hFile, &pInvis->geometry_kind_238, 1, 0);
                if (pInvis->field_237 == 2) {
                    W8LevelFileLinkedRecord* pRecord = pInvis->pRecord_23d;
                    unsigned char okRecord = 0;
                    if (pRecord != 0) {
                        okRecord = FileWrite(hFile, &pRecord->kind_00, 1, 0);
                        okRecord &= FileWrite(hFile, pRecord->vertices_01, 0x1b0, 0);
                        okRecord &= FileWrite(hFile, &pRecord->linked_face_1b1, 2, 0);
                        free(pRecord);
                    }
                    ok &= okRecord;
                    if (ok == 0) {
                        return FALSE;
                    }
                }
            }
        }
        free(pInvis);
        return ok;
    }
    case 3: {
        W8LevelFileSound* pSound = static_cast<W8LevelFileSound*>(pTrigger->pData_02);
        if (pSound == 0) {
            srAssertFail("pSound", LEVELFILE_CPP, 0x51e, 0);
        }
        ok = FileWrite(hFile, &pSound->version_00, 1, 0);
        ok &= FileWrite(hFile, &pSound->volume_min_01, 4, 0);
        ok &= FileWrite(hFile, &pSound->volume_max_05, 4, 0);
        ok &= FileWrite(hFile, &pSound->speed_min_09, 4, 0);
        ok &= FileWrite(hFile, &pSound->speed_max_0d, 4, 0);
        ok &= FileWrite(hFile, &pSound->time_min_11, 4, 0);
        ok &= FileWrite(hFile, &pSound->time_max_15, 4, 0);
        ok &= FileWrite(hFile, &pSound->unbounded_19, 4, 0);
        ok &= FileWrite(hFile, &pSound->radius_1d, 4, 0);
        ok &= FileWrite(hFile, &pSound->position_21, 0xc, 0);
        ok &= FileWrite(hFile, &pSound->region_u_2d, 0xc, 0);
        ok &= FileWrite(hFile, &pSound->region_v_39, 0xc, 0);
        ok &= FileWrite(hFile, pSound->wave_45, 0x80, 0);
        ok &= fSuccess;
        if (pSound->version_00 > 1) {
            ok &= FileWrite(hFile, &pSound->has_position_c5, 1, 0);
            ok &= FileWrite(hFile, &pSound->looping_c6, 1, 0);
        }
        if (pSound->version_00 > 2) {
            ok &= FileWrite(hFile, &pSound->region_center_c7, 0xc, 0);
            ok &= FileWrite(hFile, &pSound->region_angle_d3, 4, 0);
            ok &= FileWrite(hFile, &pSound->region_min_d7, 0xc, 0);
            ok &= FileWrite(hFile, &pSound->region_max_e3, 0xc, 0);
        }
        if (pSound->version_00 > 3) {
            ok &= FileWrite(hFile, pSound->name_ef, 0x80, 0);
        }
        if (pSound->version_00 > 4) {
            ok &= FileWrite(hFile, &pSound->shared_16f, 1, 0);
        }
        free(pSound);
        return ok;
    }
    case 4:
        return WriteSuperTriggerFile(hFile, pTrigger) & fSuccess;
    default:
        return fSuccess;
    }
}

// FUNCTION: WIZ8 0x004D2A30
BOOLEAN ReadSuperTriggerFile(int hFile, W8LevelFileTrigger* pTrigger)
{
    W8LevelFileSuperTrigger* pSuper =
        static_cast<W8LevelFileSuperTrigger*>(malloc(sizeof(W8LevelFileSuperTrigger)));
    if (pSuper == 0) {
        ReportBuildStatus(7, "ReadSuperTrigger: Could not allocate SuperTrigger strucutre.\n");
        return FALSE;
    }
    unsigned char fSuccess = FileRead(hFile, &pSuper->version_00, 1, 0);
    fSuccess &= FileRead(hFile, pSuper->name_01, 0x80, 0);
    fSuccess &= FileRead(hFile, &pSuper->flags_81, 1, 0);
    fSuccess &= FileRead(hFile, &pSuper->active_82, 1, 0);
    fSuccess &= FileRead(hFile, &pSuper->kind_83, 1, 0);
    fSuccess &= FileRead(hFile, &pSuper->when_active_84, 1, 0);
    fSuccess &= FileRead(hFile, &pSuper->prop_index_85, 1, 0);
    fSuccess &= FileRead(hFile, &pSuper->activation_count_86, 1, 0);
    fSuccess &= FileRead(hFile, &pSuper->inactive_count_87, 1, 0);
    fSuccess &= FileRead(hFile, &pSuper->trigger_88, 4, 0);
    fSuccess &= FileRead(hFile, &pSuper->trigger_on_8c, 4, 0);
    fSuccess &= FileRead(hFile, &pSuper->trigger_off_90, 4, 0);
    fSuccess &= FileRead(hFile, pSuper->recipients_94, 0x100, 0);
    fSuccess &= FileRead(hFile, &pSuper->ataxia_or_cure_194, 1, 0);
    fSuccess &= FileRead(hFile, pSuper->ps_events_195, 0x100, 0);
    fSuccess &= FileRead(hFile, &pSuper->allow_save_295, 1, 0);
    fSuccess &= FileRead(hFile, &pSuper->price_296, 4, 0);
    fSuccess &= FileRead(hFile, &pSuper->door_kind_29a, 1, 0);
    fSuccess &= FileRead(hFile, pSuper->animation_29b, 0x80, 0);
    if (!fSuccess) {
        return FALSE;
    }
    ReportBuildStatus(
        5,
        reinterpret_cast<const char*>( // reinterpret-ok: String returns a logging buffer
            String("Super Trigger: %s, recipients: %s\n", pSuper->name_01, pSuper->recipients_94)));
    if (pSuper->version_00 >= 2) {
        fSuccess &= FileRead(hFile, pSuper->size_49b, 0xc, 0);
        fSuccess &= FileRead(hFile, &pSuper->direction_4a7, 4, 0);
        fSuccess &= FileRead(hFile, &pSuper->wait_4ab, 1, 0);
        fSuccess &= FileRead(hFile, &pSuper->wait_4ac, 1, 0);
        fSuccess &= FileRead(hFile, &pSuper->wait_4ad, 1, 0);
        fSuccess &= FileRead(hFile, &pSuper->loop_4ae, 1, 0);
        fSuccess &= FileRead(hFile, &pSuper->speed_4af, 0x10, 0);
        if (!fSuccess) {
            return FALSE;
        }
    }
    fSuccess &= FileRead(hFile, &pSuper->ignore_4bf, 1, 0);
    fSuccess &= FileRead(hFile, &pSuper->group_4c0, 1, 0);
    fSuccess &= FileRead(hFile, &pSuper->set_group_4c1, 1, 0);
    fSuccess &= FileRead(hFile, pSuper->groups_4c2, 0x100, 0);
    fSuccess &= FileRead(hFile, pSuper->objects_5c2, 0x100, 0);
    fSuccess &= FileRead(hFile, &pSuper->close_door_6c2, 1, 0);
    if (!fSuccess) {
        return FALSE;
    }
    fSuccess &= FileRead(hFile, &pSuper->wait_6c3, 4, 0);
    fSuccess &= FileRead(hFile, &pSuper->field_6c7, 4, 0);
    fSuccess &= FileRead(hFile, pSuper->event_6cb, 0x100, 0);
    fSuccess &= FileRead(hFile, &pSuper->field_7cb, 4, 0);
    if (!fSuccess) {
        return FALSE;
    }
    if (pSuper->version_00 >= 3) {
        fSuccess &= FileRead(hFile, pSuper->particle_system_7cf, 0x80, 0);
    }
    if ((pSuper->flags_81 & 1) == 0) {
        fSuccess &= FileRead(hFile, &pSuper->placement_kind_84f, 1, 0);
        if (pSuper->placement_kind_84f == 1) {
            pSuper->pPosition_850 = static_cast<W8LevelFileTriggerPosition*>(
                malloc(sizeof(W8LevelFileTriggerPosition)));
            if (pSuper->pPosition_850 == 0) {
                ReportBuildStatus(
                    7, "ReadSuperTrigger: Could not allocate Trigger Position structure.\n");
                return FALSE;
            }
            fSuccess &=
                FileRead(hFile, pSuper->pPosition_850, sizeof(W8LevelFileTriggerPosition), 0);
        } else if (pSuper->placement_kind_84f == 2) {
            pSuper->pPlane_854 = static_cast<W8LevelFilePlane*>(malloc(sizeof(W8LevelFilePlane)));
            if (pSuper->pPlane_854 == 0) {
                ReportBuildStatus(
                    7, "ReadSuperTrigger: Could not allocate Trigger Plane structure.\n");
                return FALSE;
            }
            fSuccess &= FileRead(hFile, pSuper->pPlane_854, sizeof(W8LevelFilePlane), 0);
            g_level_file_6833fc
                ->invisible_planes_1669[g_level_file_6833fc->num_invisible_planes_1665] =
                pSuper->pPlane_854;
            ++g_level_file_6833fc->num_invisible_planes_1665;
        }
        if (!fSuccess) {
            return FALSE;
        }
        fSuccess &= FileRead(hFile, &pSuper->has_hotspot_858, 1, 0);
        if (pSuper->has_hotspot_858 != 0) {
            pSuper->pHotSpot_859 =
                static_cast<W8LevelFileTriggerHotSpot*>(malloc(sizeof(W8LevelFileTriggerHotSpot)));
            if (pSuper->pHotSpot_859 == 0) {
                ReportBuildStatus(
                    7, "ReadSuperTrigger: Could not allocate Trigger HotSpot structure.\n");
                return FALSE;
            }
            fSuccess &= FileRead(hFile, pSuper->pHotSpot_859, sizeof(W8LevelFileTriggerHotSpot), 0);
        }
    }
    if (!fSuccess) {
        return FALSE;
    }
    fSuccess &= FileRead(hFile, &pSuper->field_85d, 1, 0);
    if (pSuper->field_85d != 0) {
        fSuccess &= FileRead(hFile, &pSuper->door_85e.kind_00, 1, 0);
        if (pSuper->door_85e.kind_00 == 1) {
            fSuccess = ReadDoorTriggerFile004D3540(hFile, &pSuper->door_85e);
        } else if (pSuper->door_85e.kind_00 == 2) {
            W8LevelFileLinkedRecord* pRecord = static_cast<W8LevelFileLinkedRecord*>(malloc(0x1bb));
            unsigned char ok = 0;
            if (pRecord != 0) {
                ok = FileRead(hFile, &pRecord->kind_00, 1, 0);
                ok &= FileRead(hFile, pRecord->vertices_01, 0x1b0, 0);
                ok &= FileRead(hFile, &pRecord->linked_face_1b1, 2, 0);
                g_level_file_6833fc
                    ->linked_records_260d[g_level_file_6833fc->num_linked_records_2609] = pRecord;
                ++g_level_file_6833fc->num_linked_records_2609;
                pSuper->pRecord_863 = pRecord;
            }
            fSuccess &= ok;
            /* Retail stores through pRecord_863 even when the allocation failed. */
            pSuper->pRecord_863->normal_scale_1b3 = pSuper->field_7cb;
            pSuper->pRecord_863->forward_scale_1b7 = pSuper->direction_4a7;
        }
    }
    pTrigger->pData_02 = pSuper;
    return fSuccess;
}

// FUNCTION: WIZ8 0x004D3000
BOOLEAN WriteSuperTriggerFile(int hFile, W8LevelFileTrigger* pTrigger)
{
    W8LevelFileSuperTrigger* pSuper = static_cast<W8LevelFileSuperTrigger*>(pTrigger->pData_02);
    if (pSuper == 0) {
        ReportBuildStatus(7, "WriteSuperTrigger: Couldn't create SuperTrigger structure.\n");
        return FALSE;
    }
    unsigned char fSuccess = FileWrite(hFile, &pSuper->version_00, 1, 0);
    fSuccess &= FileWrite(hFile, pSuper->name_01, 0x80, 0);
    fSuccess &= FileWrite(hFile, &pSuper->flags_81, 1, 0);
    fSuccess &= FileWrite(hFile, &pSuper->active_82, 1, 0);
    fSuccess &= FileWrite(hFile, &pSuper->kind_83, 1, 0);
    fSuccess &= FileWrite(hFile, &pSuper->when_active_84, 1, 0);
    fSuccess &= FileWrite(hFile, &pSuper->prop_index_85, 1, 0);
    fSuccess &= FileWrite(hFile, &pSuper->activation_count_86, 1, 0);
    fSuccess &= FileWrite(hFile, &pSuper->inactive_count_87, 1, 0);
    fSuccess &= FileWrite(hFile, &pSuper->trigger_88, 4, 0);
    fSuccess &= FileWrite(hFile, &pSuper->trigger_on_8c, 4, 0);
    fSuccess &= FileWrite(hFile, &pSuper->trigger_off_90, 4, 0);
    fSuccess &= FileWrite(hFile, pSuper->recipients_94, 0x100, 0);
    fSuccess &= FileWrite(hFile, &pSuper->ataxia_or_cure_194, 1, 0);
    fSuccess &= FileWrite(hFile, pSuper->ps_events_195, 0x100, 0);
    fSuccess &= FileWrite(hFile, &pSuper->allow_save_295, 1, 0);
    fSuccess &= FileWrite(hFile, &pSuper->price_296, 4, 0);
    fSuccess &= FileWrite(hFile, &pSuper->door_kind_29a, 1, 0);
    fSuccess &= FileWrite(hFile, pSuper->animation_29b, 0x80, 0);
    if (!fSuccess) {
        return FALSE;
    }
    if (pSuper->version_00 >= 2) {
        fSuccess &= FileWrite(hFile, pSuper->size_49b, 0xc, 0);
        fSuccess &= FileWrite(hFile, &pSuper->direction_4a7, 4, 0);
        fSuccess &= FileWrite(hFile, &pSuper->wait_4ab, 1, 0);
        fSuccess &= FileWrite(hFile, &pSuper->wait_4ac, 1, 0);
        fSuccess &= FileWrite(hFile, &pSuper->wait_4ad, 1, 0);
        fSuccess &= FileWrite(hFile, &pSuper->loop_4ae, 1, 0);
        fSuccess &= FileWrite(hFile, &pSuper->speed_4af, 0x10, 0);
        if (!fSuccess) {
            return FALSE;
        }
    }
    fSuccess &= FileWrite(hFile, &pSuper->ignore_4bf, 1, 0);
    fSuccess &= FileWrite(hFile, &pSuper->group_4c0, 1, 0);
    fSuccess &= FileWrite(hFile, &pSuper->set_group_4c1, 1, 0);
    fSuccess &= FileWrite(hFile, pSuper->groups_4c2, 0x100, 0);
    fSuccess &= FileWrite(hFile, pSuper->objects_5c2, 0x100, 0);
    fSuccess &= FileWrite(hFile, &pSuper->close_door_6c2, 1, 0);
    if (!fSuccess) {
        return FALSE;
    }
    fSuccess &= FileWrite(hFile, &pSuper->wait_6c3, 4, 0);
    fSuccess &= FileWrite(hFile, &pSuper->field_6c7, 4, 0);
    fSuccess &= FileWrite(hFile, pSuper->event_6cb, 0x100, 0);
    fSuccess &= FileWrite(hFile, &pSuper->field_7cb, 4, 0);
    if (!fSuccess) {
        return FALSE;
    }
    if (pSuper->version_00 >= 3) {
        fSuccess &= FileWrite(hFile, pSuper->particle_system_7cf, 0x80, 0);
    }
    if ((pSuper->flags_81 & 1) == 0) {
        fSuccess &= FileWrite(hFile, &pSuper->placement_kind_84f, 1, 0);
        if (pSuper->placement_kind_84f == 1) {
            if (pSuper->pPosition_850 == 0) {
                ReportBuildStatus(7, "WriteSuperTrigger: No Trigger Position structure.\n");
                return FALSE;
            }
            fSuccess &=
                FileWrite(hFile, pSuper->pPosition_850, sizeof(W8LevelFileTriggerPosition), 0);
            free(pSuper->pPosition_850);
        } else if (pSuper->placement_kind_84f == 2) {
            if (pSuper->pPlane_854 == 0) {
                ReportBuildStatus(7, "WriteSuperTrigger: No FileTriggerPlane structure.\n");
                return FALSE;
            }
            fSuccess &= FileWrite(hFile, pSuper->pPlane_854, sizeof(W8LevelFilePlane), 0);
            free(pSuper->pPlane_854);
        }
        if (!fSuccess) {
            return FALSE;
        }
        fSuccess &= FileWrite(hFile, &pSuper->has_hotspot_858, 1, 0);
        if (pSuper->has_hotspot_858 != 0) {
            if (pSuper->pHotSpot_859 == 0) {
                ReportBuildStatus(7, "WriteSuperTrigger: No Trigger HotSpot structure.\n");
                return FALSE;
            }
            fSuccess &=
                FileWrite(hFile, pSuper->pHotSpot_859, sizeof(W8LevelFileTriggerHotSpot), 0);
            free(pSuper->pHotSpot_859);
        }
    }
    if (!fSuccess) {
        return FALSE;
    }
    fSuccess &= FileWrite(hFile, &pSuper->field_85d, 1, 0);
    if (pSuper->field_85d != 0) {
        fSuccess &= FileWrite(hFile, &pSuper->door_85e.kind_00, 1, 0);
        if (pSuper->door_85e.kind_00 == 1) {
            fSuccess = WriteDoorTriggerFile(hFile, &pSuper->door_85e);
        } else if (pSuper->door_85e.kind_00 == 2) {
            W8LevelFileLinkedRecord* pRecord = pSuper->pRecord_863;
            unsigned char ok = 0;
            if (pRecord != 0) {
                ok = FileWrite(hFile, &pRecord->kind_00, 1, 0);
                ok &= FileWrite(hFile, pRecord->vertices_01, 0x1b0, 0);
                ok &= FileWrite(hFile, &pRecord->linked_face_1b1, 2, 0);
                free(pRecord);
            }
            fSuccess &= ok;
        }
    }
    free(pSuper);
    return fSuccess;
}

// FUNCTION: WIZ8 0x004D3540
BOOLEAN ReadDoorTriggerFile004D3540(int hFile, W8LevelFileDoorRef* pDoor)
{
    W8LevelFileDoor* pDoorRec = static_cast<W8LevelFileDoor*>(malloc(0x99));
    if (pDoorRec != 0) {
        unsigned char fSuccess = FileRead(hFile, &pDoorRec->version_00, 10, 0);
        fSuccess &= FileRead(hFile, &pDoorRec->item_0a, 2, 0);
        fSuccess &= FileRead(hFile, &pDoorRec->has_position_0c, 1, 0);
        fSuccess &= FileRead(hFile, &pDoorRec->position_0d, 0xc, 0);
        fSuccess &= FileRead(hFile, pDoorRec->linked_trigger_19, 0x80, 0);
        ReportBuildStatus(
            5,
            reinterpret_cast< // reinterpret-ok: String returns a logging buffer
                const char*>(String(
                "Door: %s",
                pDoorRec->linked_trigger_19))); // reinterpret-ok: String returns a logging buffer
        pDoor->door_01 = pDoorRec;
        return fSuccess;
    }
    return FALSE;
}

// FUNCTION: WIZ8 0x004D3660
BOOLEAN WriteDoorTriggerFile(int hFile, W8LevelFileDoorRef* pDoor)
{
    W8LevelFileDoor* pDoorRec = pDoor->door_01;
    unsigned char fSuccess = FileWrite(hFile, &pDoorRec->version_00, 10, 0);
    fSuccess &= FileWrite(hFile, &pDoorRec->item_0a, 2, 0);
    fSuccess &= FileWrite(hFile, &pDoorRec->has_position_0c, 1, 0);
    fSuccess &= FileWrite(hFile, &pDoorRec->position_0d, 0xc, 0);
    fSuccess &= FileWrite(hFile, pDoorRec->linked_trigger_19, 0x80, 0);
    free(pDoorRec);
    return fSuccess;
}

// FUNCTION: WIZ8 0x004D3770
BOOLEAN ReadPathAIFile(int hFile, W8LevelFilePathAI* pPathAI)
{
    unsigned char fSuccess = FileRead(hFile, &pPathAI->version_00, 1, 0);
    fSuccess &= FileRead(hFile, &pPathAI->scaled_01, 1, 0);
    fSuccess &= FileRead(hFile, &pPathAI->position_02, 4, 0);
    fSuccess &= FileRead(hFile, pPathAI->unknown_06, 4, 0);
    fSuccess &= FileRead(hFile, &pPathAI->path_count_0a, 4, 0);
    if (pPathAI->scaled_01 == 2) {
        if (pPathAI->path_count_0a != 0) {
            pPathAI->pScaledPaths =
                static_cast<W8LevelFileScaledPathNode*>(malloc(pPathAI->path_count_0a * 0x28));
            if (pPathAI->pScaledPaths == 0) {
                srAssertFail("pPathAI->pScaledPaths", LEVELFILE_CPP, 0x732, 0);
            }
            fSuccess &= FileRead(hFile, pPathAI->pScaledPaths, pPathAI->path_count_0a * 0x28, 0);
        }
    } else if (pPathAI->path_count_0a != 0) {
        pPathAI->pPaths = static_cast<W8LevelFilePathNode*>(malloc(pPathAI->path_count_0a * 0x1c));
        if (pPathAI->pPaths == 0) {
            srAssertFail("pPathAI->pPaths", LEVELFILE_CPP, 0x73d, 0);
        }
        fSuccess &= FileRead(hFile, pPathAI->pPaths, pPathAI->path_count_0a * 0x1c, 0);
    }
    return fSuccess;
}

// FUNCTION: WIZ8 0x004D38E0
BOOLEAN WritePathAIFile(int hFile, W8LevelFilePathAI* pPathAI)
{
    unsigned char fSuccess = FileWrite(hFile, &pPathAI->version_00, 1, 0);
    fSuccess &= FileWrite(hFile, &pPathAI->scaled_01, 1, 0);
    fSuccess &= FileWrite(hFile, &pPathAI->position_02, 4, 0);
    fSuccess &= FileWrite(hFile, pPathAI->unknown_06, 4, 0);
    fSuccess &= FileWrite(hFile, &pPathAI->path_count_0a, 4, 0);
    if (pPathAI->scaled_01 == 2) {
        if (pPathAI->path_count_0a != 0) {
            if (pPathAI->pScaledPaths == 0) {
                srAssertFail("pPathAI->pScaledPaths", LEVELFILE_CPP, 0x761, 0);
            }
            fSuccess &= FileWrite(hFile, pPathAI->pScaledPaths, pPathAI->path_count_0a * 0x28, 0);
            free(pPathAI->pScaledPaths);
            pPathAI->pScaledPaths = 0;
        }
    } else if (pPathAI->path_count_0a != 0) {
        if (pPathAI->pPaths == 0) {
            srAssertFail("pPathAI->pPaths", LEVELFILE_CPP, 0x76b, 0);
        }
        fSuccess &= FileWrite(hFile, pPathAI->pPaths, pPathAI->path_count_0a * 0x1c, 0);
        free(pPathAI->pPaths);
        pPathAI->pPaths = 0;
    }
    return fSuccess;
}

// FUNCTION: WIZ8 0x004D3A10
BOOLEAN ReadAnimObjFile(int hFile, W8LevelFileAnimObj* pAnimObj)
{
    unsigned short usFrame;
    short i;
    short j;

    memset(pAnimObj, 0, sizeof(W8LevelFileAnimObj));
    BOOLEAN fSuccess = TRUE;
    fSuccess &= FileRead(hFile, &pAnimObj->version_00, 1, 0);
    fSuccess &= FileRead(hFile, &pAnimObj->num_anims_01, 1, 0);
    fSuccess &= FileRead(hFile, &pAnimObj->animation_playing_02, 1, 0);
    fSuccess &= FileRead(hFile, &pAnimObj->frame_method_03, 1, 0);
    fSuccess &= FileRead(hFile, &pAnimObj->behaviour_04, 1, 0);
    fSuccess &= FileRead(hFile, &pAnimObj->cycle_05, 1, 0);
    fSuccess &= FileRead(hFile, &pAnimObj->path_lists_06, 1, 0);
    if (pAnimObj->version_00 >= 3) {
        fSuccess = fSuccess && FileRead(hFile, &pAnimObj->playback_scale_07, 4, 0);
    } else {
        pAnimObj->playback_scale_07 = 15.0f;
    }
    if (pAnimObj->version_00 >= 5) {
        fSuccess = fSuccess && FileRead(hFile, &pAnimObj->start_frame_0b, 1, 0);
    } else {
        pAnimObj->start_frame_0b = 0;
    }
    if (pAnimObj->version_00 >= 6) {
        fSuccess = fSuccess && FileRead(hFile, &pAnimObj->random_play_0c, 1, 0) &&
                   FileRead(hFile, &pAnimObj->play_chance_0d, 4, 0);
    } else {
        pAnimObj->random_play_0c = 0;
        pAnimObj->play_chance_0d = 1.0f;
    }
    fSuccess = fSuccess && FileRead(hFile, pAnimObj->discarded_11, 0x32, 0);
    if (!fSuccess) {
        srAssertFail("fSuccess", LEVELFILE_CPP, 0x7a5, 0);
    }
    if (pAnimObj->num_anims_01 != 0) {
        pAnimObj->abHowMany = static_cast<char*>(malloc(pAnimObj->num_anims_01));
        if (pAnimObj->abHowMany == 0) {
            srAssertFail("pAnimObj->abHowMany", LEVELFILE_CPP, 0x7aa, 0);
        }
        fSuccess &= FileRead(hFile, pAnimObj->abHowMany, pAnimObj->num_anims_01, 0);
    }
    if (pAnimObj->version_00 >= 7) {
        fSuccess &= FileRead(hFile, &pAnimObj->num_bound_box_47, 1, 0);
        if (pAnimObj->num_bound_box_47 != 0) {
            pAnimObj->pBoundBox =
                static_cast<W8LevelFileBounds*>(malloc(pAnimObj->num_bound_box_47 * 0x18));
            if (pAnimObj->pBoundBox == 0) {
                srAssertFail("pAnimObj->pBoundBox", LEVELFILE_CPP, 0x7b3, 0);
            }
            fSuccess &= FileRead(hFile, pAnimObj->pBoundBox, pAnimObj->num_bound_box_47 * 0x18, 0);
        }
    }
    if (fSuccess == 0) {
        return FALSE;
    }
    if (pAnimObj->version_00 >= 8) {
        fSuccess = FileRead(hFile, &pAnimObj->num_anim_lights_4c, 1, 0);
        if (fSuccess == 0) {
            return FALSE;
        }
        if (pAnimObj->num_anim_lights_4c != 0) {
            pAnimObj->pAnimLights_4d = static_cast<W8LevelFileAnimLight*>(
                malloc(pAnimObj->num_anim_lights_4c * sizeof(W8LevelFileAnimLight)));
            if (pAnimObj->pAnimLights_4d == 0) {
                return FALSE;
            }
            memset(pAnimObj->pAnimLights_4d, 0,
                   pAnimObj->num_anim_lights_4c * sizeof(W8LevelFileAnimLight));
            for (i = 0; i < pAnimObj->num_anim_lights_4c; ++i) {
                fSuccess &= ReadAnimLightFile(hFile, pAnimObj->pAnimLights_4d + i);
                if (fSuccess == 0) {
                    return FALSE;
                }
            }
        }
    }
    if (pAnimObj->path_lists_06 == 0) {
        if ((pAnimObj->version_00 >= 9) &&
            (FileRead(hFile, &pAnimObj->has_path_ai_51, 1, 0), pAnimObj->has_path_ai_51 != 0)) {
            pAnimObj->pPathAI_52 =
                static_cast<W8LevelFilePathAI*>(malloc(sizeof(W8LevelFilePathAI)));
            if (pAnimObj->pPathAI_52 == 0) {
                return FALSE;
            }
            fSuccess = ReadPathAIFile(hFile, pAnimObj->pPathAI_52);
            if (fSuccess == 0) {
                return FALSE;
            }
        }
        if (pAnimObj->num_anims_01 != 0) {
            pAnimObj->pMorphs_56 = static_cast<W8LevelFileMorph*>(
                malloc(pAnimObj->num_anims_01 * sizeof(W8LevelFileMorph)));
            if (pAnimObj->pMorphs_56 == 0) {
                srAssertFail("pAnimObj->pMorphs", LEVELFILE_CPP, 0x7e1, 0);
            }
            memset(pAnimObj->pMorphs_56, 0, pAnimObj->num_anims_01 * sizeof(W8LevelFileMorph));
            for (i = 0; i < pAnimObj->num_anims_01; ++i) {
                W8LevelFileMorph* pMorph = pAnimObj->pMorphs_56 + i;
                fSuccess &= FileRead(hFile, &pMorph->channel_00, 1, 0) &
                            FileRead(hFile, &pMorph->num_frames_01, 1, 0);
                if (pMorph->num_frames_01 != 0) {
                    pMorph->LODMesh_02.pFrames = static_cast<W8LevelFileFrame*>(
                        malloc(pMorph->num_frames_01 * sizeof(W8LevelFileFrame)));
                    if (pMorph->LODMesh_02.pFrames == 0) {
                        srAssertFail("pAnimObj->pMorphs[i].LODMesh.pFrames", LEVELFILE_CPP, 0x7eb,
                                     0);
                    }
                    memset(pMorph->LODMesh_02.pFrames, 0,
                           pMorph->num_frames_01 * sizeof(W8LevelFileFrame));
                    if (pMorph->num_frames_01 != 0) {
                        usFrame = 0;
                        do {
                            W8LevelFileFrame* pFrame = pMorph->LODMesh_02.pFrames + (short)usFrame;
                            fSuccess &= FileRead(hFile, &pFrame->flags_00, 1, 0) &
                                        ReadMeshFile(hFile, &pFrame->mesh_01) &
                                        FileRead(hFile, &pFrame->num_textures_5d, 2, 0);
                            if (pFrame->num_textures_5d != 0) {
                                pFrame->pTextures_5f = static_cast<W8MaterialRecord*>(
                                    malloc(pFrame->num_textures_5d * 0x12a));
                                if (pFrame->pTextures_5f == 0) {
                                    srAssertFail(
                                        "pAnimObj->pMorphs[i].LODMesh.pFrames[i2].pTextures",
                                        LEVELFILE_CPP, 0x7f9, 0);
                                }
                                memset(pFrame->pTextures_5f, 0, pFrame->num_textures_5d * 0x12a);
                                unsigned char fTextures = 1;
                                for (j = 0; j < pFrame->num_textures_5d; ++j) {
                                    W8MaterialRecord* pTexture = pFrame->pTextures_5f + j;
                                    fTextures = FileRead(hFile, pTexture, 0x11a, 0);
                                    if (pTexture->version_00 >= 4) {
                                        fTextures &=
                                            FileRead(hFile, pTexture->texture_modes_11a, 0x10, 0);
                                    }
                                    if (fTextures == 0) {
                                        return FALSE;
                                    }
                                }
                                if (fTextures == 0) {
                                    return FALSE;
                                }
                            }
                            if ((usFrame == 0) &&
                                ((pMorph->LODMesh_02.pFrames->mesh_01.flags_0c & 1) != 0)) {
                                usFrame = pMorph->num_frames_01;
                            }
                            usFrame = usFrame + 1;
                        } while ((short)usFrame < (short)pMorph->num_frames_01);
                    }
                }
            }
        }
    } else {
        fSuccess &= FileRead(hFile, &pAnimObj->num_transforms_5a, 1, 0);
        if (pAnimObj->num_transforms_5a != 0) {
            pAnimObj->pTransforms_5b = static_cast<W8LevelFileTransform*>(
                malloc(pAnimObj->num_transforms_5a * sizeof(W8LevelFileTransform)));
            if (pAnimObj->pTransforms_5b == 0) {
                srAssertFail("pAnimObj->pTransforms", LEVELFILE_CPP, 0x80e, 0);
            }
            memset(pAnimObj->pTransforms_5b, 0,
                   pAnimObj->num_transforms_5a * sizeof(W8LevelFileTransform));
            for (i = 0; i < pAnimObj->num_transforms_5a; ++i) {
                W8LevelFileTransform* pTransform = pAnimObj->pTransforms_5b + i;
                fSuccess &= FileRead(hFile, &pTransform->channel_00, 1, 0) &
                            FileRead(hFile, &pTransform->num_frames_01, 1, 0);
                if (pTransform->num_frames_01 != 0) {
                    pTransform->LODMesh_02.pFrames = static_cast<W8LevelFileFrame*>(
                        malloc(pTransform->num_frames_01 * sizeof(W8LevelFileFrame)));
                    if (pTransform->LODMesh_02.pFrames == 0) {
                        srAssertFail("pAnimObj->pTransforms[i].LODMesh.pFrames", LEVELFILE_CPP,
                                     0x818, 0);
                    }
                    memset(pTransform->LODMesh_02.pFrames, 0,
                           pTransform->num_frames_01 * sizeof(W8LevelFileFrame));
                    if (pTransform->num_frames_01 != 0) {
                        short iFrame = 0;
                        do {
                            W8LevelFileFrame* pFrame = pTransform->LODMesh_02.pFrames + iFrame;
                            fSuccess &= FileRead(hFile, &pFrame->flags_00, 1, 0) &
                                        ReadMeshFile(hFile, &pFrame->mesh_01) &
                                        FileRead(hFile, &pFrame->num_textures_5d, 2, 0);
                            if ((pFrame->num_textures_5d < 0) || (pFrame->num_textures_5d > 500)) {
                                sprintf(g_level_file_error_682ff8,
                                        "Invalid number of materials in mesh: %d\n",
                                        (int)pFrame->num_textures_5d);
                                ReportBuildStatus(7, g_level_file_error_682ff8);
                                return FALSE;
                            }
                            if (pFrame->num_textures_5d != 0) {
                                pFrame->pTextures_5f = static_cast<W8MaterialRecord*>(
                                    malloc(pFrame->num_textures_5d * 0x12a));
                                if (pFrame->pTextures_5f == 0) {
                                    srAssertFail(
                                        "pAnimObj->pTransforms[i].LODMesh.pFrames[i2].pTextures",
                                        LEVELFILE_CPP, 0x82e, 0);
                                }
                                memset(pFrame->pTextures_5f, 0, pFrame->num_textures_5d * 0x12a);
                                unsigned char fTextures = 1;
                                for (j = 0; j < pFrame->num_textures_5d; ++j) {
                                    W8MaterialRecord* pTexture = pFrame->pTextures_5f + j;
                                    fTextures = FileRead(hFile, pTexture, 0x11a, 0);
                                    if (pTexture->version_00 >= 4) {
                                        fTextures &=
                                            FileRead(hFile, pTexture->texture_modes_11a, 0x10, 0);
                                    }
                                    if (fTextures == 0) {
                                        return FALSE;
                                    }
                                }
                                if (fTextures == 0) {
                                    return FALSE;
                                }
                            }
                            iFrame = iFrame + 1;
                        } while (iFrame < (short)pTransform->num_frames_01);
                    }
                }
                fSuccess &= ReadPathAIFile(hFile, &pTransform->pathAI_06);
            }
        }
    }
    return fSuccess;
}

// FUNCTION: WIZ8 0x004D4480
BOOLEAN WriteAnimObjFile(int hFile, W8LevelFileAnimObj* pAnimObj)
{
    unsigned short usFrame;
    short i;
    short j;

    BOOLEAN fSuccess = TRUE;
    fSuccess &= FileWrite(hFile, &pAnimObj->version_00, 1, 0);
    fSuccess &= FileWrite(hFile, &pAnimObj->num_anims_01, 1, 0);
    fSuccess &= FileWrite(hFile, &pAnimObj->animation_playing_02, 1, 0);
    fSuccess &= FileWrite(hFile, &pAnimObj->frame_method_03, 1, 0);
    fSuccess &= FileWrite(hFile, &pAnimObj->behaviour_04, 1, 0);
    fSuccess &= FileWrite(hFile, &pAnimObj->cycle_05, 1, 0);
    fSuccess &= FileWrite(hFile, &pAnimObj->path_lists_06, 1, 0);
    if (pAnimObj->version_00 >= 3) {
        fSuccess = fSuccess && FileWrite(hFile, &pAnimObj->playback_scale_07, 4, 0);
    }
    if (pAnimObj->version_00 >= 5) {
        fSuccess = fSuccess && FileWrite(hFile, &pAnimObj->start_frame_0b, 1, 0);
    }
    if (pAnimObj->version_00 >= 6) {
        fSuccess = fSuccess && FileWrite(hFile, &pAnimObj->random_play_0c, 1, 0) &&
                   FileWrite(hFile, &pAnimObj->play_chance_0d, 4, 0);
    }
    fSuccess = fSuccess && FileWrite(hFile, pAnimObj->discarded_11, 0x32, 0);
    if (!fSuccess) {
        srAssertFail("fSuccess", LEVELFILE_CPP, 0x867, 0);
    }
    if (pAnimObj->num_anims_01 != 0) {
        if (pAnimObj->abHowMany == 0) {
            srAssertFail("pAnimObj->abHowMany", LEVELFILE_CPP, 0x86b, 0);
        }
        fSuccess &= FileWrite(hFile, pAnimObj->abHowMany, pAnimObj->num_anims_01, 0);
        free(pAnimObj->abHowMany);
    }
    if (pAnimObj->version_00 >= 7) {
        fSuccess &= FileWrite(hFile, &pAnimObj->num_bound_box_47, 1, 0);
        if (pAnimObj->num_bound_box_47 != 0) {
            if (pAnimObj->pBoundBox == 0) {
                srAssertFail("pAnimObj->pBoundBox", LEVELFILE_CPP, 0x874, 0);
            }
            fSuccess &= FileWrite(hFile, pAnimObj->pBoundBox, pAnimObj->num_bound_box_47 * 0x18, 0);
            free(pAnimObj->pBoundBox);
        }
    }
    if (fSuccess == 0) {
        return FALSE;
    }
    if (pAnimObj->version_00 >= 8) {
        fSuccess = FileWrite(hFile, &pAnimObj->num_anim_lights_4c, 1, 0);
        if ((pAnimObj->num_anim_lights_4c != 0) && (pAnimObj->pAnimLights_4d != 0)) {
            for (i = 0; i < pAnimObj->num_anim_lights_4c; ++i) {
                fSuccess &= WriteAnimLightFile(hFile, pAnimObj->pAnimLights_4d + i);
                if (fSuccess == 0) {
                    return FALSE;
                }
            }
            free(pAnimObj->pAnimLights_4d);
        }
        if (fSuccess == 0) {
            return FALSE;
        }
    }
    if (pAnimObj->path_lists_06 == 0) {
        if ((pAnimObj->version_00 >= 9) &&
            (FileWrite(hFile, &pAnimObj->has_path_ai_51, 1, 0), pAnimObj->has_path_ai_51 != 0) &&
            (pAnimObj->pPathAI_52 != 0)) {
            fSuccess = WritePathAIFile(hFile, pAnimObj->pPathAI_52);
            free(pAnimObj->pPathAI_52);
            if (fSuccess == 0) {
                return FALSE;
            }
        }
        if (pAnimObj->num_anims_01 != 0) {
            if (pAnimObj->pMorphs_56 == 0) {
                srAssertFail("pAnimObj->pMorphs", LEVELFILE_CPP, 0x89e, 0);
            }
            for (i = 0; i < pAnimObj->num_anims_01; ++i) {
                W8LevelFileMorph* pMorph = pAnimObj->pMorphs_56 + i;
                fSuccess &= FileWrite(hFile, &pMorph->channel_00, 1, 0) &
                            FileWrite(hFile, &pMorph->num_frames_01, 1, 0);
                if (pMorph->num_frames_01 != 0) {
                    if (pMorph->LODMesh_02.pFrames == 0) {
                        srAssertFail("pAnimObj->pMorphs[i].LODMesh.pFrames", LEVELFILE_CPP, 0x8a5,
                                     0);
                    }
                    usFrame = 0;
                    do {
                        W8LevelFileFrame* pFrame = pMorph->LODMesh_02.pFrames + (short)usFrame;
                        fSuccess &= FileWrite(hFile, &pFrame->flags_00, 1, 0) &
                                    WriteMeshFile(hFile, &pFrame->mesh_01) &
                                    FileWrite(hFile, &pFrame->num_textures_5d, 2, 0);
                        if (pFrame->num_textures_5d != 0) {
                            if (pFrame->pTextures_5f == 0) {
                                srAssertFail("pAnimObj->pMorphs[i].LODMesh.pFrames[i2].pTextures",
                                             LEVELFILE_CPP, 0x8af, 0);
                            }
                            fSuccess = 1;
                            for (j = 0; j < pFrame->num_textures_5d; ++j) {
                                W8MaterialRecord* pTexture = pFrame->pTextures_5f + j;
                                fSuccess = FileWrite(hFile, pTexture, 0x11a, 0);
                                if (pTexture->version_00 >= 4) {
                                    fSuccess &=
                                        FileWrite(hFile, pTexture->texture_modes_11a, 0x10, 0);
                                }
                                if (fSuccess == 0) {
                                    return FALSE;
                                }
                            }
                            if (fSuccess == 0) {
                                return FALSE;
                            }
                            free(pFrame->pTextures_5f);
                            pFrame->pTextures_5f = 0;
                        }
                        if ((usFrame == 0) &&
                            ((pMorph->LODMesh_02.pFrames->mesh_01.flags_0c & 1) != 0)) {
                            usFrame = pMorph->num_frames_01;
                        }
                        usFrame = usFrame + 1;
                    } while ((short)usFrame < (short)pMorph->num_frames_01);
                    free(pMorph->LODMesh_02.pFrames);
                    pMorph->LODMesh_02.pFrames = 0;
                }
            }
            free(pAnimObj->pMorphs_56);
            pAnimObj->pMorphs_56 = 0;
            return fSuccess;
        }
    } else {
        fSuccess &= FileWrite(hFile, &pAnimObj->num_transforms_5a, 1, 0);
        if (pAnimObj->num_transforms_5a != 0) {
            if (pAnimObj->pTransforms_5b == 0) {
                srAssertFail("pAnimObj->pTransforms", LEVELFILE_CPP, 0x8c7, 0);
            }
            for (i = 0; i < pAnimObj->num_transforms_5a; ++i) {
                W8LevelFileTransform* pTransform = pAnimObj->pTransforms_5b + i;
                fSuccess &= FileWrite(hFile, &pTransform->channel_00, 1, 0) &
                            FileWrite(hFile, &pTransform->num_frames_01, 1, 0);
                if (pTransform->num_frames_01 != 0) {
                    if (pTransform->LODMesh_02.pFrames == 0) {
                        srAssertFail("pAnimObj->pTransforms[i].LODMesh.pFrames", LEVELFILE_CPP,
                                     0x8ce, 0);
                    }
                    short iFrame = 0;
                    do {
                        W8LevelFileFrame* pFrame = pTransform->LODMesh_02.pFrames + iFrame;
                        fSuccess &= FileWrite(hFile, &pFrame->flags_00, 1, 0) &
                                    WriteMeshFile(hFile, &pFrame->mesh_01) &
                                    FileWrite(hFile, &pFrame->num_textures_5d, 2, 0);
                        if (pFrame->num_textures_5d != 0) {
                            if (pFrame->pTextures_5f == 0) {
                                srAssertFail(
                                    "pAnimObj->pTransforms[i].LODMesh.pFrames[i2].pTextures",
                                    LEVELFILE_CPP, 0x8d8, 0);
                            }
                            fSuccess = 1;
                            for (j = 0; j < pFrame->num_textures_5d; ++j) {
                                W8MaterialRecord* pTexture = pFrame->pTextures_5f + j;
                                fSuccess = FileWrite(hFile, pTexture, 0x11a, 0);
                                if (pTexture->version_00 >= 4) {
                                    fSuccess &=
                                        FileWrite(hFile, pTexture->texture_modes_11a, 0x10, 0);
                                }
                                if (fSuccess == 0) {
                                    return FALSE;
                                }
                            }
                            if (fSuccess == 0) {
                                return FALSE;
                            }
                            free(pFrame->pTextures_5f);
                            pFrame->pTextures_5f = 0;
                        }
                        iFrame = iFrame + 1;
                    } while (iFrame < (short)pTransform->num_frames_01);
                    free(pTransform->LODMesh_02.pFrames);
                    pTransform->LODMesh_02.pFrames = 0;
                }
                fSuccess &= WritePathAIFile(hFile, &pTransform->pathAI_06);
            }
            free(pAnimObj->pTransforms_5b);
            pAnimObj->pTransforms_5b = 0;
        }
    }
    return fSuccess;
}

// FUNCTION: WIZ8 0x004D4CB0
W8LevelFileProp* ReadPropsFile(int hFile, int count)
{
    unsigned char fSuccess = 1;
    if (count == 0) {
        return 0;
    }
    W8LevelFileProp* pProps =
        static_cast<W8LevelFileProp*>(malloc(count * sizeof(W8LevelFileProp)));
    if (pProps == 0) {
        srAssertFail("pProps", LEVELFILE_CPP, 0x905, 0);
    }
    memset(pProps, 0, count * sizeof(W8LevelFileProp));
    for (int i = 0; i < count; ++i) {
        W8LevelFileProp* pProp = pProps + i;
        fSuccess &=
            FileRead(hFile, &pProp->version_00, 1, 0) & FileRead(hFile, &pProp->bNumFrames, 1, 0);
        if (pProp->version_00 >= 5) {
            fSuccess &= FileRead(hFile, &pProp->option_02, 1, 0) &
                        FileRead(hFile, &pProp->position_03, sizeof(pProp->position_03), 0);
        }
        if (pProp->version_00 >= 6) {
            fSuccess &= FileRead(hFile, &pProp->flags_0f, 4, 0);
        }
        if (pProp->version_00 >= 7) {
            fSuccess &= FileRead(hFile, pProp->name_13, 0x40, 0);
            ReportBuildStatus(
                5, reinterpret_cast< // reinterpret-ok: String returns a logging buffer
                       const char*>(
                       String("Prop: %s\n",
                              pProp->name_13))); // reinterpret-ok: String returns a logging buffer
        }
        if (fSuccess == 0) {
            srAssertFail("fSuccess", LEVELFILE_CPP, 0x918, 0);
        }
        if (pProp->version_00 >= 8) {
            fSuccess &= FileRead(hFile, &pProp->num_frame_pos_b7, 1, 0);
            if (pProp->num_frame_pos_b7 != 0) {
                pProp->usFrame_Pos = static_cast<W8LevelFileFramePosition*>(
                    malloc(pProp->num_frame_pos_b7 * sizeof(W8LevelFileFramePosition)));
                if (pProp->usFrame_Pos == 0) {
                    srAssertFail(
                        "pProps[i1].usFrame_Pos", LEVELFILE_CPP, 0x91f,
                        reinterpret_cast< // reinterpret-ok: String returns a logging buffer
                            const char*>( // reinterpret-ok: String returns a logging buffer
                            String("Could not allocate %d segments for prop '%s'!",
                                   (int)pProp->num_frame_pos_b7, pProp->name_13)));
                }
                fSuccess &= FileRead(hFile, pProp->usFrame_Pos, pProp->num_frame_pos_b7 << 2, 0);
            }
        }
        unsigned char okAnimObj = ReadAnimObjFile(hFile, &pProp->anim_obj_53);
        if ((fSuccess & okAnimObj) == 0) {
            srAssertFail("fSuccess", LEVELFILE_CPP, 0x924, 0);
        }
        fSuccess = fSuccess & okAnimObj & FileRead(hFile, &pProp->has_trigger_b2, 1, 0);
        if (fSuccess == 0) {
            srAssertFail("fSuccess", LEVELFILE_CPP, 0x931, 0);
        }
        if (pProp->has_trigger_b2 != 0) {
            pProp->pTrigger = static_cast<W8LevelFileTrigger*>(malloc(6));
            if (pProp->pTrigger == 0) {
                srAssertFail("pProps[i1].pTrigger", LEVELFILE_CPP, 0x935, 0);
            }
            memset(pProp->pTrigger, 0, sizeof(W8LevelFileTrigger));
            fSuccess &= ReadTriggerFile(hFile, pProp->pTrigger);
            if (fSuccess == 0) {
                srAssertFail("fSuccess", LEVELFILE_CPP, 0x938, 0);
            }
        }
        if (pProp->version_00 >= 9) {
            fSuccess &= FileRead(hFile, &pProp->has_footsteps_bc, 1, 0);
            if (fSuccess == 0) {
                srAssertFail("fSuccess", LEVELFILE_CPP, 0x93e, 0);
            }
            if (pProp->has_footsteps_bc != 0) {
                fSuccess &= FileRead(hFile, &pProp->footstep_surface_bd, 1, 0) &
                            FileRead(hFile, &pProp->footstep_material_be, 1, 0);
            }
        }
    }
    if (fSuccess == 0) {
        return 0;
    }
    return pProps;
}

// FUNCTION: WIZ8 0x004D4FC0
BOOLEAN WritePropsFile(int hFile, int count, W8LevelFileProp* pProps)
{
    unsigned char fSuccess = 1;
    if (count == 0) {
        return TRUE;
    }
    if (pProps == 0) {
        srAssertFail("pProps", LEVELFILE_CPP, 0x962, 0);
    }
    for (int i = 0; i < count; ++i) {
        W8LevelFileProp* pProp = pProps + i;
        fSuccess &=
            FileWrite(hFile, &pProp->version_00, 1, 0) & FileWrite(hFile, &pProp->bNumFrames, 1, 0);
        if (pProp->version_00 >= 5) {
            fSuccess &= FileWrite(hFile, &pProp->option_02, 1, 0) &
                        FileWrite(hFile, &pProp->position_03, sizeof(pProp->position_03), 0);
        }
        if (pProp->version_00 >= 6) {
            fSuccess &= FileWrite(hFile, &pProp->flags_0f, 4, 0);
        }
        if (pProp->version_00 >= 7) {
            fSuccess &= FileWrite(hFile, pProp->name_13, 0x40, 0);
        }
        if (fSuccess == 0) {
            srAssertFail("fSuccess", LEVELFILE_CPP, 0x970, 0);
        }
        if (pProp->version_00 >= 8) {
            fSuccess &= FileWrite(hFile, &pProp->num_frame_pos_b7, 1, 0);
            if (pProp->num_frame_pos_b7 != 0) {
                if (pProp->usFrame_Pos == 0) {
                    srAssertFail("pProps[i1].usFrame_Pos", LEVELFILE_CPP, 0x976, 0);
                }
                fSuccess &= FileWrite(hFile, pProp->usFrame_Pos, pProp->num_frame_pos_b7 << 2, 0);
                free(pProp->usFrame_Pos);
            }
        }
        fSuccess &= WriteAnimObjFile(hFile, &pProp->anim_obj_53);
        fSuccess &= FileWrite(hFile, &pProp->has_trigger_b2, 1, 0);
        if (fSuccess == 0) {
            srAssertFail("fSuccess", LEVELFILE_CPP, 0x987, 0);
        }
        if (pProp->has_trigger_b2 != 0) {
            fSuccess &= WriteTriggerFile(hFile, pProp->pTrigger);
            if (fSuccess == 0) {
                srAssertFail("fSuccess", LEVELFILE_CPP, 0x98b, 0);
            }
            free(pProp->pTrigger);
            pProp->pTrigger = 0;
        }
        if (pProp->version_00 >= 9) {
            fSuccess &= FileWrite(hFile, &pProp->has_footsteps_bc, 1, 0);
            if (fSuccess == 0) {
                srAssertFail("fSuccess", LEVELFILE_CPP, 0x993, 0);
            }
            if (pProp->has_footsteps_bc != 0) {
                fSuccess &= FileWrite(hFile, &pProp->footstep_surface_bd, 1, 0) &
                            FileWrite(hFile, &pProp->footstep_material_be, 1, 0);
            }
        }
    }
    free(pProps);
    return fSuccess;
}

// FUNCTION: WIZ8 0x004D5240
BOOLEAN ReadParticleSystemFile(int hFile, W8LevelFileParticleSystem* pSystem)
{
    BOOLEAN fSuccess = TRUE;
    fSuccess &= FileRead(hFile, pSystem, 0x217, 0);
    if (pSystem->version_00 > 1) {
        fSuccess &= FileRead(hFile, &pSystem->particle_01.attachment_key_216, 2, 0);
    } else {
        pSystem->particle_01.attachment_key_216 = 0;
    }
    if (pSystem->version_00 > 2) {
        fSuccess &= FileRead(hFile, &pSystem->particle_01.emission_limit_218, 4, 0);
        fSuccess &= FileRead(hFile, &pSystem->particle_01.requires_positional_21c, 1, 0);
    } else {
        pSystem->particle_01.emission_limit_218 = 0;
        pSystem->particle_01.requires_positional_21c = 0;
    }
    if (pSystem->version_00 > 3) {
        fSuccess &= FileRead(hFile, &pSystem->particle_01.start_frame_21d, 4, 0);
        fSuccess &= FileRead(hFile, &pSystem->particle_01.end_frame_221, 4, 0);
    } else {
        pSystem->particle_01.start_frame_21d = 0;
    }
    if (fSuccess == 0) {
        srAssertFail("fSuccess", LEVELFILE_CPP, 0xa03, "Couldn't read particle system.");
    }
    ReportBuildStatus(
        5,
        reinterpret_cast<const char*>( // reinterpret-ok: String returns a logging buffer
            String("Particle System: %s, Position %f, %f, %f\n", pSystem->particle_01.name,
                   pSystem->particle_01.location.x * g_world_scale_005ebc40,
                   pSystem->particle_01.location.y * g_world_scale_005ebc40,
                   pSystem->particle_01.location.z * g_world_scale_005ebc40)));
    return fSuccess;
}

// FUNCTION: WIZ8 0x004D5370
BOOLEAN WriteParticleSystemFile(int hFile, W8LevelFileParticleSystem* pSystem)
{
    BOOLEAN fSuccess = TRUE;
    fSuccess &= FileWrite(hFile, pSystem, 0x217, 0);
    if (pSystem->version_00 > 1) {
        fSuccess &= FileWrite(hFile, &pSystem->particle_01.attachment_key_216, 2, 0);
    }
    if (pSystem->version_00 > 2) {
        fSuccess &= FileWrite(hFile, &pSystem->particle_01.emission_limit_218, 4, 0);
        fSuccess &= FileWrite(hFile, &pSystem->particle_01.requires_positional_21c, 1, 0);
    }
    if (pSystem->version_00 > 3) {
        fSuccess &= FileWrite(hFile, &pSystem->particle_01.start_frame_21d, 4, 0);
        fSuccess &= FileWrite(hFile, &pSystem->particle_01.end_frame_221, 4, 0);
    }
    if (fSuccess == 0) {
        srAssertFail("fSuccess", LEVELFILE_CPP, 0xa2a, "Couldn't Write particle system.");
    }
    return fSuccess;
}

// FUNCTION: WIZ8 0x004D5430
BOOLEAN ReadLevelFileBlock(int hFile, W8LevelFileBlock* pBlock)
{
    BOOLEAN fSuccess = TRUE;
    fSuccess &= FileRead(hFile, &pBlock->fog_enabled_00, 1, 0);
    fSuccess &= FileRead(hFile, &pBlock->environment_red_01, 4, 0);
    fSuccess &= FileRead(hFile, &pBlock->environment_green_05, 4, 0);
    fSuccess &= FileRead(hFile, &pBlock->environment_blue_09, 4, 0);
    fSuccess &= FileRead(hFile, &pBlock->intensity_0d, 4, 0);
    fSuccess &= FileRead(hFile, &pBlock->view_distance_11, 4, 0);
    fSuccess &= FileRead(hFile, &pBlock->camera_mode_15, 1, 0);
    if (pBlock->camera_mode_15 >= 1) {
        fSuccess &= FileRead(hFile, &pBlock->camera_position_16, 0xc, 0);
    }
    if (pBlock->camera_mode_15 >= 2) {
        fSuccess &= FileRead(hFile, &pBlock->camera_angle_22, 4, 0);
        fSuccess &= FileRead(hFile, &pBlock->camera_axis_26, 0xc, 0);
    }
    fSuccess = fSuccess != 0 && FileRead(hFile, &pBlock->has_light_colours_32, 1, 0) != 0;
    if (pBlock->has_light_colours_32 != 0) {
        fSuccess &= FileRead(hFile, pBlock->light_colours_33, 0x300, 0);
    }
    fSuccess = fSuccess != 0 && FileRead(hFile, &pBlock->has_environment_colours_333, 1, 0) != 0;
    if (pBlock->has_environment_colours_333 != 0) {
        fSuccess &= FileRead(hFile, pBlock->environment_colours_334, 0x300, 0);
    }
    return fSuccess;
}

// FUNCTION: WIZ8 0x004D5580
BOOLEAN WriteLevelFileBlock(int hFile, W8LevelFileBlock* pBlock)
{
    BOOLEAN fSuccess = TRUE;
    fSuccess &= FileWrite(hFile, &pBlock->fog_enabled_00, 1, 0);
    fSuccess &= FileWrite(hFile, &pBlock->environment_red_01, 4, 0);
    fSuccess &= FileWrite(hFile, &pBlock->environment_green_05, 4, 0);
    fSuccess &= FileWrite(hFile, &pBlock->environment_blue_09, 4, 0);
    fSuccess &= FileWrite(hFile, &pBlock->intensity_0d, 4, 0);
    fSuccess &= FileWrite(hFile, &pBlock->view_distance_11, 4, 0);
    fSuccess &= FileWrite(hFile, &pBlock->camera_mode_15, 1, 0);
    if (pBlock->camera_mode_15 >= 1) {
        fSuccess &= FileWrite(hFile, &pBlock->camera_position_16, 0xc, 0);
    }
    if (pBlock->camera_mode_15 >= 2) {
        fSuccess &= FileWrite(hFile, &pBlock->camera_angle_22, 4, 0);
        fSuccess &= FileWrite(hFile, &pBlock->camera_axis_26, 0xc, 0);
    }
    fSuccess = fSuccess != 0 && FileWrite(hFile, &pBlock->has_light_colours_32, 1, 0) != 0;
    if (pBlock->has_light_colours_32 != 0) {
        fSuccess &= FileWrite(hFile, pBlock->light_colours_33, 0x300, 0);
    }
    fSuccess = fSuccess != 0 && FileWrite(hFile, &pBlock->has_environment_colours_333, 1, 0) != 0;
    if (pBlock->has_environment_colours_333 != 0) {
        fSuccess &= FileWrite(hFile, pBlock->environment_colours_334, 0x300, 0);
    }
    return fSuccess;
}
