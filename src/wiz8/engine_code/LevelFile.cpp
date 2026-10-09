#include "wiz8/engine_code/stLight.hpp"
#include "wiz8/engine_code/LevelFile.h"
#include "wiz8/engine_code/ReadMesh.h"
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
static W8LevelFile* g_level_file;

/* Scratch message buffer for the mesh/anim-object load failures; original name
   unknown. The next defined symbol is g_level_file at 0x006833FC, so the
   buffer is at most 0x404 bytes; 0x400 matches this file's other 0x400 sprintf
   buffers. */
// GLOBAL: WIZ8 0x00682ff8
static char g_level_file_error[0x400];

static unsigned char ReadMaterialRecord(int file, W8MaterialRecord* material)
{
    unsigned char success = FileRead(file, material, 0x11a, 0);
    if (material->version >= 4) {
        success &= FileRead(file, material->texture_modes, 0x10, 0);
    }
    return success;
}

static unsigned char WriteMaterialRecord(int file, W8MaterialRecord* material)
{
    unsigned char success = FileWrite(file, material, 0x11a, 0);
    if (material->version >= 4) {
        success &= FileWrite(file, material->texture_modes, 0x10, 0);
    }
    return success;
}

// FUNCTION: WIZ8 0x004CFDC0
W8LevelFile* ReadLevelFile(int hFile)
{
    W8LevelFile* pLevel = static_cast<W8LevelFile*>(malloc(sizeof(W8LevelFile)));
    if (pLevel == 0) {
        srAssertFail("pLevel", LEVELFILE_CPP, 0x2b, 0);
    }
    memset(pLevel, 0, sizeof(W8LevelFile));
    pLevel->submesh_count = 1;
    pLevel->mesh_count = 1;
    pLevel->num_switch_triggers = 0;
    memset(pLevel->switch_triggers, 0, sizeof(pLevel->switch_triggers));
    pLevel->num_invisible_planes = 0;
    memset(pLevel->invisible_planes, 0, sizeof(pLevel->invisible_planes));
    pLevel->num_linked_records = 0;
    memset(pLevel->linked_records, 0, sizeof(pLevel->linked_records));
    g_level_file = pLevel;

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
        ok = ReadMaterialRecord(hFile, pTexture);
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
            if (pMonster->num_mon_path != 0) {
                pMonster->MonPath = static_cast<W8LevelFilePathNode*>(
                    malloc(pMonster->num_mon_path * sizeof(W8LevelFilePathNode)));
                if (pMonster->MonPath == 0) {
                    srAssertFail("pLevel->pMonsters[i1].MonPath", LEVELFILE_CPP, 0x69, 0);
                }
                memset(pMonster->MonPath, 0, pMonster->num_mon_path * sizeof(W8LevelFilePathNode));
                fSuccess &= FileRead(hFile, pMonster->MonPath,
                                     pMonster->num_mon_path * sizeof(W8LevelFilePathNode), 0);
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

    FileRead(hFile, &pLevel->missile_count, sizeof(pLevel->missile_count), 0);
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
    fSuccess &= fSuccess2;
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
            unsigned char ok = FileRead(hFile, &pCamera->positional0, 4, 0);
            ok &= FileRead(hFile, &pCamera->positional1, 4, 0);
            ok &= FileRead(hFile, &pCamera->has_scale, 1, 0);
            ok &= FileRead(hFile, pCamera->name, sizeof(pCamera->name), 0);
            if (pCamera->has_scale != 0) {
                ok &= FileRead(hFile, &pCamera->scale, 4, 0);
            }
            fSuccess &= ReadPathAIFile(hFile, &pCamera->pathAI) & ok;
        }
        if (fSuccess == 0) {
            return 0;
        }
    }

    fSuccess &= FileRead(hFile, &pLevel->has_block, sizeof(pLevel->has_block), 0);
    if (pLevel->has_block != 0) {
        fSuccess &= ReadLevelFileBlock(hFile, &pLevel->block);
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

    ok = FileRead(hFile, &pLevel->camera_mode, sizeof(pLevel->camera_mode), 0);
    ok &= FileRead(hFile, &pLevel->nClippingPlanes, sizeof(pLevel->nClippingPlanes), 0);
    ok &= fSuccess;
    if (pLevel->nClippingPlanes != 0) {
        ok &= FileRead(hFile, &pLevel->clipping_plane_version, 1, 0);
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

    ok &= FileRead(hFile, &pLevel->environment_offset, sizeof(pLevel->environment_offset), 0);
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
                5,
                reinterpret_cast<const char*>( // reinterpret-ok: String returns a logging buffer
                    String("Named Position: %s (%f, %f, %f)\n", pPosition->name,
                           pPosition->position.x, pPosition->position.y, pPosition->position.z)));
        }
    }

    FileRead(hFile, pLevel->unknown_6b9, sizeof(pLevel->unknown_6b9), 0);
    pLevel->read_end_position = FileGetPos(hFile);
    return pLevel;
}

// FUNCTION: WIZ8 0x004D07C0
BOOLEAN WriteLevelFile(int hFile, int hFileIn, W8LevelFile* pLevel)
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
    FileWrite(hFile, &pLevel->submesh_count, 4, 0);
    FileWrite(hFile, &pLevel->mesh_count, 4, 0);
    FileWrite(hFile, &pLevel->nTextures, 2, 0);
    iCount = pLevel->nTextures;
    if (iCount == 0) {
        return FALSE;
    }
    ok = 1;
    for (i = 0; i < static_cast<short>(iCount); ++i) {
        W8MaterialRecord* pTexture = pLevel->pTextures + i;
        ok = WriteMaterialRecord(hFile, pTexture);
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
    if (pLevel->submesh_count != 0) {
        OctMeshModel* pModel = pLevel->pModels;
        for (i = 0; static_cast<unsigned int>(i) < pLevel->submesh_count; ++i) {
            pModel->Write(hFile);
            ++pModel;
        }
    }
    FileWrite(hFile, &iCount, 4, 0);
    if (pLevel->pModels != 0) {
        delete[] pLevel->pModels;
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
            if (pMonster->num_mon_path != 0) {
                fSuccess &= FileWrite(hFile, pMonster->MonPath,
                                      pMonster->num_mon_path * sizeof(W8LevelFilePathNode), 0);
                if (fSuccess == 0) {
                    return FALSE;
                }
                free(pMonster->MonPath);
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
    FileWrite(hFile, &pLevel->missile_count, 4, 0);
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
            unsigned char okCam = FileWrite(hFile, &pCamera->positional0, 4, 0);
            okCam &= FileWrite(hFile, &pCamera->positional1, 4, 0);
            okCam &= FileWrite(hFile, &pCamera->has_scale, 1, 0);
            okCam &= FileWrite(hFile, pCamera->name, sizeof(pCamera->name), 0);
            if (pCamera->has_scale != 0) {
                okCam &= FileWrite(hFile, &pCamera->scale, 4, 0);
            }
            fSuccess &= WritePathAIFile(hFile, &pCamera->pathAI) & okCam;
        }
        if (fSuccess == 0) {
            return FALSE;
        }
        free(pLevel->pCameras);
    }
    fSuccess = FileWrite(hFile, &iCount, 4, 0);
    fSuccess = FileWrite(hFile, &pLevel->has_block, 4, 0) & fSuccess;
    if (pLevel->has_block != 0) {
        fSuccess &= WriteLevelFileBlock(hFile, &pLevel->block);
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
    fSuccess = FileWrite(hFile, &pLevel->camera_mode, 4, 0) & fSuccess;
    fSuccess = FileWrite(hFile, &pLevel->nClippingPlanes, 4, 0) & fSuccess;
    if (pLevel->nClippingPlanes != 0) {
        fSuccess &= FileWrite(hFile, &pLevel->clipping_plane_version, 1, 0);
        fSuccess &= FileWrite(hFile, pLevel->pClippingPlanes,
                              pLevel->nClippingPlanes * sizeof(W8LevelFileClippingPlaneRecord), 0);
        free(pLevel->pClippingPlanes);
        if (fSuccess == 0) {
            return FALSE;
        }
    }
    fSuccess = FileWrite(hFile, &iCount, 4, 0);
    fSuccess =
        FileWrite(hFile, &pLevel->environment_offset, sizeof(pLevel->environment_offset), 0) &
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
        fSuccess &= FileWrite(hFile, pLevel->pNamedPositions,
                              pLevel->nNamedPositions * sizeof(W8LevelFileNamedPosition), 0);
        free(pLevel->pNamedPositions);
        if (fSuccess == 0) {
            return FALSE;
        }
    }
    FileWrite(hFile, &iCount, 4, 0);
    float level_scale = GetAutomapGridCellSize();
    fSuccess = FileWrite(hFile, &level_scale, 4, 0) & fSuccess;
    fSuccess = FileWrite(hFile, &pLevel->num_automap_nodes, 4, 0) & fSuccess;
    if (pLevel->num_automap_nodes != 0) {
        fSuccess &= FileWrite(hFile, pLevel->automap_nodes, pLevel->num_automap_nodes * 4, 0);
        free(pLevel->automap_nodes);
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
    pLevel->num_switch_triggers = 0;
    pLevel->num_invisible_planes = 0;
    free(pLevel);
    return fSuccess;
}

// FUNCTION: WIZ8 0x004D1110
BOOLEAN ReadMeshFile(int hFile, W8LevelFileMesh* pMesh)
{
    int i;
    memset(pMesh, 0, sizeof(W8LevelFileMesh));
    BOOLEAN fSuccess = TRUE;
    fSuccess &= FileRead(hFile, &pMesh->version, 4, 0);
    fSuccess &= FileRead(hFile, &pMesh->num_vertices, 4, 0);
    fSuccess &= FileRead(hFile, &pMesh->num_faces, 4, 0);
    if (fSuccess == 0) {
        return FALSE;
    }
    int count = pMesh->num_vertices;
    if (count >= 0x186a1 || count <= 0) {
        sprintf(g_level_file_error, "Invalid number of vertices in mesh (%d vertices).\n", count);
        ReportBuildStatus(7, g_level_file_error);
        return FALSE;
    }
    count = pMesh->num_faces;
    if (count >= 0x30d41 || count <= 0) {
        sprintf(g_level_file_error, "Invalid number of faces in mesh (%d faces).\n", count);
        ReportBuildStatus(7, g_level_file_error);
        return FALSE;
    }
    if (pMesh->version >= 3) {
        fSuccess = FileRead(hFile, &pMesh->flags, 1, 0);
    }
    if (pMesh->version >= 2) {
        fSuccess &= FileRead(hFile, &pMesh->location, sizeof(pMesh->location), 0);
        fSuccess &= FileRead(hFile, &pMesh->rotation_angle, 0x10, 0);
        fSuccess &= FileRead(hFile, &pMesh->scale, sizeof(pMesh->scale), 0);
    }
    if (pMesh->version >= 4) {
        fSuccess &= FileRead(hFile, &pMesh->mapping_count, 1, 0);
        if (pMesh->mapping_count != 0) {
            fSuccess &= FileRead(hFile, &pMesh->mapped_value, 4, 0);
        }
    }
    if (fSuccess == 0) {
        return FALSE;
    }
    if ((pMesh->flags & W8_LEVEL_MESH_LOD_VERTICES) == 0) {
        pMesh->pstVertices = static_cast<srVector3T<float>*>(
            malloc(pMesh->num_vertices * 2 * sizeof(*pMesh->pstVertices)));
        if (pMesh->pstVertices == 0) {
            srAssertFail("pMesh->pstVertices", LEVELFILE_CPP, 0x29a, 0);
        }
        memset(pMesh->pstVertices, 0, pMesh->num_vertices * 2 * sizeof(*pMesh->pstVertices));
        if (FileRead(hFile, pMesh->pstVertices, pMesh->num_vertices * sizeof(*pMesh->pstVertices),
                     0) == 0) {
            return FALSE;
        }
    } else {
        fSuccess &= FileRead(hFile, &pMesh->lod_mode, 1, 0);
        fSuccess &= FileRead(hFile, &pMesh->num_lods, 2, 0);
        if ((pMesh->flags & W8_LEVEL_MESH_SHORT_LOD_VERTICES) == 0) {
            srVector3T<float>** pLods =
                static_cast<srVector3T<float>**>(malloc(pMesh->num_lods * sizeof(*pLods)));
            if (pLods == 0) {
                return FALSE;
            }
            for (i = 0; i < pMesh->num_lods; ++i) {
                pLods[i] = static_cast<srVector3T<float>*>(
                    malloc(pMesh->num_vertices * sizeof(*pLods[i])));
                if (pLods[i] == 0) {
                    return FALSE;
                }
                fSuccess &= FileRead(hFile, pLods[i], pMesh->num_vertices * sizeof(*pLods[i]), 0);
                if (fSuccess == 0) {
                    return FALSE;
                }
            }
            pMesh->lods = pLods;
        } else {
            if (pMesh->lod_mode >= 2) {
                FileRead(hFile, &pMesh->lod_scale, 4, 0);
            }
            short** pLods = static_cast<short**>(malloc(pMesh->num_lods * sizeof(short*)));
            if (pLods == 0) {
                return FALSE;
            }
            for (i = 0; i < pMesh->num_lods; ++i) {
                pLods[i] = static_cast<short*>(malloc(pMesh->num_vertices * 3 * sizeof(short)));
                if (pLods[i] == 0) {
                    return FALSE;
                }
                fSuccess &= FileRead(hFile, pLods[i], pMesh->num_vertices * 3 * sizeof(short), 0);
                if (fSuccess == 0) {
                    return FALSE;
                }
            }
            pMesh->lod_shorts = pLods;
        }
    }
    if ((pMesh->flags & W8_LEVEL_MESH_COMPRESSED_FACES) != 0) {
        pMesh->pstCompFaces = static_cast<W8LevelFileCompressedFace*>(
            malloc(pMesh->num_faces * sizeof(W8LevelFileCompressedFace)));
        if (pMesh->pstCompFaces == 0) {
            srAssertFail("pMesh->pstCompFaces", LEVELFILE_CPP, 0x2a7, 0);
        }
        memset(pMesh->pstCompFaces, 0, pMesh->num_faces * sizeof(W8LevelFileCompressedFace));
        return FileRead(hFile, pMesh->pstCompFaces,
                        pMesh->num_faces * sizeof(W8LevelFileCompressedFace), 0);
    }
    pMesh->pstFaces =
        static_cast<W8ReadMeshFace*>(malloc(pMesh->num_faces * 2 * sizeof(*pMesh->pstFaces)));
    if (pMesh->pstFaces == 0) {
        srAssertFail("pMesh->pstFaces", LEVELFILE_CPP, 0x2b2, 0);
    }
    memset(pMesh->pstFaces, 0, pMesh->num_faces * 2 * sizeof(*pMesh->pstFaces));
    return FileRead(hFile, pMesh->pstFaces, pMesh->num_faces * sizeof(*pMesh->pstFaces), 0);
}

// FUNCTION: WIZ8 0x004D1510
BOOLEAN WriteMeshFile(int hFile, W8LevelFileMesh* pMesh)
{
    BOOLEAN fSuccess = TRUE;
    fSuccess &= FileWrite(hFile, &pMesh->version, 4, 0);
    fSuccess &= FileWrite(hFile, &pMesh->num_vertices, 4, 0);
    fSuccess &= FileWrite(hFile, &pMesh->num_faces, 4, 0);
    if (fSuccess == 0) {
        return FALSE;
    }
    if (pMesh->version >= 3) {
        fSuccess = FileWrite(hFile, &pMesh->flags, 1, 0);
    }
    if (pMesh->version >= 2) {
        fSuccess &= FileWrite(hFile, &pMesh->location, sizeof(pMesh->location), 0);
        fSuccess &= FileWrite(hFile, &pMesh->rotation_angle, 0x10, 0);
        fSuccess &= FileWrite(hFile, &pMesh->scale, sizeof(pMesh->scale), 0);
    }
    if (pMesh->version >= 4) {
        fSuccess &= FileWrite(hFile, &pMesh->mapping_count, 1, 0);
        if (pMesh->mapping_count != 0) {
            fSuccess &= FileWrite(hFile, &pMesh->mapped_value, 4, 0);
        }
    }
    if (fSuccess == 0) {
        return FALSE;
    }
    if ((pMesh->flags & W8_LEVEL_MESH_LOD_VERTICES) == 0) {
        if (pMesh->pstVertices == 0) {
            srAssertFail("pMesh->pstVertices", LEVELFILE_CPP, 0x315, 0);
        }
        if (FileWrite(hFile, pMesh->pstVertices, pMesh->num_vertices * sizeof(*pMesh->pstVertices),
                      0) == 0) {
            ReportBuildStatus(7, "WriteFileMesh: Could not write mesh vertices.\n");
        }
    } else {
        fSuccess &= FileWrite(hFile, &pMesh->lod_mode, 1, 0);
        fSuccess &= FileWrite(hFile, &pMesh->num_lods, 2, 0);
        if (pMesh->lod_mode >= 2) {
            fSuccess &= FileWrite(hFile, &pMesh->lod_scale, 4, 0);
        }
        if ((pMesh->flags & W8_LEVEL_MESH_SHORT_LOD_VERTICES) == 0) {
            srVector3T<float>** pLods = pMesh->lods;
            if (pLods == 0) {
                return FALSE;
            }
            for (int i = 0; i < pMesh->num_lods; ++i) {
                if (pLods[i] == 0) {
                    return FALSE;
                }
                fSuccess &= FileWrite(hFile, pLods[i], pMesh->num_vertices * sizeof(*pLods[i]), 0);
                if (fSuccess == 0) {
                    ReportBuildStatus(7, "WriteFileMesh: Could not write mesh vertices.\n");
                }
                free(pLods[i]);
            }
        } else {
            short** pLods = pMesh->lod_shorts;
            if (pLods == 0) {
                return FALSE;
            }
            for (int i = 0; i < pMesh->num_lods; ++i) {
                if (pLods[i] == 0) {
                    return FALSE;
                }
                fSuccess &= FileWrite(hFile, pLods[i], pMesh->num_vertices * 3 * sizeof(short), 0);
                if (fSuccess == 0) {
                    return FALSE;
                }
                free(pLods[i]);
            }
        }
    }
    free(pMesh->pstVertices);
    if ((pMesh->flags & W8_LEVEL_MESH_COMPRESSED_FACES) != 0) {
        if (pMesh->pstCompFaces == 0) {
            srAssertFail("pMesh->pstCompFaces", LEVELFILE_CPP, 799, 0);
        }
        unsigned char ok = FileWrite(hFile, pMesh->pstCompFaces,
                                     pMesh->num_faces * sizeof(W8LevelFileCompressedFace), 0);
        free(pMesh->pstCompFaces);
        return ok;
    }
    if (pMesh->pstFaces == 0) {
        srAssertFail("pMesh->pstFaces", LEVELFILE_CPP, 0x326, 0);
    }
    unsigned char ok =
        FileWrite(hFile, pMesh->pstFaces, pMesh->num_faces * sizeof(*pMesh->pstFaces), 0);
    free(pMesh->pstFaces);
    return ok;
}

// FUNCTION: WIZ8 0x004D1820
BOOLEAN ReadLightFile(int hFile, W8LevelFileLight* pLight)
{
    BOOLEAN fSuccess = TRUE;
    fSuccess &= FileRead(hFile, &pLight->version, 2, 0);
    fSuccess &= FileRead(hFile, &pLight->create, 4, 0);
    fSuccess &= FileRead(hFile, pLight->unknown_06, 2, 0);
    fSuccess &= FileRead(hFile, &pLight->position, sizeof(pLight->position), 0);
    fSuccess &= FileRead(hFile, &pLight->colour, sizeof(pLight->colour), 0);
    fSuccess &= FileRead(hFile, &pLight->intensity, 4, 0);
    fSuccess &= FileRead(hFile, &pLight->range, 4, 0);
    if (fSuccess == 0) {
        return FALSE;
    }
    if (pLight->version >= 2) {
        fSuccess = FileRead(hFile, pLight->name, 0x14, 0) != 0;
        if ((pLight->flags & W8_LEVEL_LIGHT_HAS_DEFINITION) != 0) {
            pLight->create = 1;
            pLight->pExtra =
                static_cast<W8LevelFileLightExtra*>(malloc(sizeof(W8LevelFileLightExtra)));
            if (pLight->pExtra == 0) {
                return FALSE;
            }
            fSuccess &= FileRead(hFile, pLight->pExtra, sizeof(W8LevelFileLightExtra), 0);
            if (fSuccess == 0) {
                return FALSE;
            }
            if ((pLight->pExtra->flags & W8_PARAM_LIGHT_HAS_PATH) != 0) {
                pLight->pPathAI =
                    static_cast<W8LevelFilePathAI*>(malloc(sizeof(W8LevelFilePathAI)));
                if (pLight->pPathAI == 0) {
                    return FALSE;
                }
                fSuccess = ReadPathAIFile(hFile, pLight->pPathAI);
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
    fSuccess &= FileWrite(hFile, &pLight->version, 2, 0);
    fSuccess &= FileWrite(hFile, &pLight->create, 4, 0);
    fSuccess &= FileWrite(hFile, pLight->unknown_06, 2, 0);
    fSuccess &= FileWrite(hFile, &pLight->position, sizeof(pLight->position), 0);
    fSuccess &= FileWrite(hFile, &pLight->colour, sizeof(pLight->colour), 0);
    fSuccess &= FileWrite(hFile, &pLight->intensity, 4, 0);
    fSuccess &= FileWrite(hFile, &pLight->range, 4, 0);
    if (fSuccess == 0) {
        return FALSE;
    }
    if (pLight->version >= 2) {
        fSuccess = FileWrite(hFile, pLight->name, 0x14, 0) != 0;
        if (((pLight->flags & W8_LEVEL_LIGHT_HAS_DEFINITION) != 0) && (pLight->pExtra != 0)) {
            fSuccess &= FileWrite(hFile, pLight->pExtra, sizeof(W8LevelFileLightExtra), 0);
            if (fSuccess == 0) {
                return FALSE;
            }
            if (((pLight->pExtra->flags & W8_PARAM_LIGHT_HAS_PATH) != 0) &&
                (pLight->pPathAI != 0)) {
                fSuccess = WritePathAIFile(hFile, pLight->pPathAI);
                free(pLight->pPathAI);
            }
            free(pLight->pExtra);
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
    fSuccess &= FileRead(hFile, &pLight->version, 1, 0);
    fSuccess &= FileRead(hFile, &pLight->position, sizeof(pLight->position), 0);
    fSuccess &= FileRead(hFile, &pLight->color, sizeof(pLight->color), 0);
    fSuccess &= FileRead(hFile, &pLight->intensity, 4, 0);
    fSuccess &= FileRead(hFile, &pLight->range, 4, 0);
    if (fSuccess == 0) {
        return FALSE;
    }
    if (pLight->version >= 2) {
        pLight->pExtra = static_cast<W8LevelFileLightExtra*>(malloc(sizeof(W8LevelFileLightExtra)));
        if (pLight->pExtra == 0) {
            return FALSE;
        }
        fSuccess &= FileRead(hFile, pLight->pExtra, sizeof(W8LevelFileLightExtra), 0);
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
    fSuccess &= FileWrite(hFile, &pLight->version, 1, 0);
    fSuccess &= FileWrite(hFile, &pLight->position, sizeof(pLight->position), 0);
    fSuccess &= FileWrite(hFile, &pLight->color, sizeof(pLight->color), 0);
    fSuccess &= FileWrite(hFile, &pLight->intensity, 4, 0);
    fSuccess &= FileWrite(hFile, &pLight->range, 4, 0);
    if (fSuccess == 0) {
        return FALSE;
    }
    if ((pLight->version >= 2) && (pLight->pExtra != 0)) {
        fSuccess &= FileWrite(hFile, pLight->pExtra, sizeof(W8LevelFileLightExtra), 0);
        free(pLight->pExtra);
    }
    if (fSuccess == 0) {
        srAssertFail("fSuccess", LEVELFILE_CPP, 0x3d4, "Couldn't write anim light.");
    }
    return fSuccess;
}

// FUNCTION: WIZ8 0x004D1C10
BOOLEAN ReadTriggerFile(int hFile, W8LevelFileTrigger* pTrigger)
{
    unsigned char fSuccess =
        FileRead(hFile, &pTrigger->version, 1, 0) && FileRead(hFile, &pTrigger->type, 1, 0);
    switch (pTrigger->type) {
    case 1: {
        W8LevelFileSwitch* pSwitch =
            static_cast<W8LevelFileSwitch*>(malloc(sizeof(W8LevelFileSwitch)));
        if (pSwitch == 0) {
            srAssertFail("pSwitch", LEVELFILE_CPP, 0x3f8, 0);
        }
        memset(pSwitch, 0, sizeof(W8LevelFileSwitch));
        fSuccess &= FileRead(hFile, &pSwitch->version, 1, 0);
        fSuccess &= FileRead(hFile, &pSwitch->cycle_bounce, 4, 0);
        fSuccess &= FileRead(hFile, &pSwitch->state_count, 4, 0);
        fSuccess &= FileRead(hFile, &pSwitch->animate_states, 4, 0);
        fSuccess &= FileRead(hFile, &pSwitch->range, 4, 0);
        fSuccess &= FileRead(hFile, &pSwitch->action, 4, 0);
        fSuccess &= FileRead(hFile, &pSwitch->value, 4, 0);
        fSuccess &= FileRead(hFile, &pSwitch->animate_action, 4, 0);
        fSuccess &= FileRead(hFile, &pSwitch->packed_flags, 1, 0);
        fSuccess &= FileRead(hFile, &pSwitch->enabled, 1, 0);
        fSuccess &= FileRead(hFile, pSwitch->name, sizeof(pSwitch->name), 0);
        fSuccess &= FileRead(hFile, pSwitch->recipients, sizeof(pSwitch->recipients), 0);
        fSuccess &= FileRead(hFile, pSwitch->sound, sizeof(pSwitch->sound), 0);
        if (fSuccess == 0) {
            srAssertFail("fSuccess", LEVELFILE_CPP, 0x408, 0);
        }
        ReportBuildStatus(
            5,
            reinterpret_cast< // reinterpret-ok: String returns a logging buffer
                const char*>(String(
                "Switch Trigger: %s, recipients: %s\n", // reinterpret-ok: String returns a logging buffer
                pSwitch->name, pSwitch->recipients)));
        if (pSwitch->version > 1) {
            fSuccess &= FileRead(hFile, &pSwitch->minimum_range, 4, 0);
            fSuccess &= FileRead(hFile, pSwitch->surface_id, sizeof(pSwitch->surface_id), 0);
            ReportBuildStatus(
                5, reinterpret_cast<const char*>( // reinterpret-ok: String returns a logging buffer
                       String("Switch Trigger name: %s\n", pSwitch->surface_id)));
        }
        if (pSwitch->version > 2) {
            fSuccess &= FileRead(hFile, &pSwitch->has_door_trigger, 1, 0);
            if (pSwitch->has_door_trigger != 0) {
                fSuccess &= FileRead(hFile, &pSwitch->door.kind, 1, 0);
                if (pSwitch->door.kind == 1) {
                    fSuccess &= ReadDoorTriggerFile(hFile, &pSwitch->door);
                    if (fSuccess == 0) {
                        ReportBuildStatus(7, "Problem reading door trigger.\n");
                        return 0;
                    }
                }
            }
        }
        if (pSwitch->version > 3) {
            fSuccess &= FileRead(hFile, &pSwitch->action_value, 4, 0);
        }
        pTrigger->pData = pSwitch;
        g_level_file->switch_triggers[g_level_file->num_switch_triggers] = pSwitch;
        ++g_level_file->num_switch_triggers;
        return fSuccess;
    }
    case 2: {
        W8LevelFileInvisible* pInvis =
            static_cast<W8LevelFileInvisible*>(malloc(sizeof(W8LevelFileInvisible)));
        if (pInvis == 0) {
            srAssertFail("pInvis", LEVELFILE_CPP, 0x42f, 0);
        }
        memset(pInvis, 0, sizeof(W8LevelFileInvisible));
        fSuccess &= FileRead(hFile, &pInvis->version, 1, 0);
        fSuccess &= FileRead(hFile, &pInvis->range, 4, 0);
        fSuccess &= FileRead(hFile, &pInvis->position, sizeof(pInvis->position), 0);
        fSuccess &= FileRead(hFile, &pInvis->action, 4, 0);
        fSuccess &= FileRead(hFile, &pInvis->searchable, 4, 0);
        fSuccess &= FileRead(hFile, &pInvis->fire_linked, 1, 0);
        fSuccess &= FileRead(hFile, &pInvis->enabled, 1, 0);
        fSuccess &= FileRead(hFile, pInvis->name, sizeof(pInvis->name), 0);
        fSuccess &= FileRead(hFile, pInvis->recipients, sizeof(pInvis->recipients), 0);
        ReportBuildStatus(
            5,
            reinterpret_cast< // reinterpret-ok: String returns a logging buffer
                const char*>(String(
                "Invisible Trigger: %s, recipients: %s\n", // reinterpret-ok: String returns a logging buffer
                pInvis->name, pInvis->recipients)));
        if (pInvis->version > 1) {
            fSuccess &= FileRead(hFile, &pInvis->plane_flag, 1, 0);
            pInvis->pPlane = static_cast<W8LevelFilePlane*>(malloc(sizeof(W8LevelFilePlane)));
            if (pInvis->pPlane == 0) {
                srAssertFail("pInvis->pPlane", LEVELFILE_CPP, 0x441, 0);
            }
            memset(pInvis->pPlane, 0, sizeof(W8LevelFilePlane));
            fSuccess &= FileRead(hFile, pInvis->pPlane, sizeof(W8LevelFilePlane), 0);
        }
        if (pInvis->version > 2) {
            fSuccess &= FileRead(hFile, &pInvis->angle, 4, 0);
            fSuccess &= FileRead(hFile, &pInvis->direction, sizeof(pInvis->direction), 0);
            fSuccess &= FileRead(hFile, &pInvis->unused, 1, 0);
            fSuccess &= FileRead(hFile, pInvis->action_string, sizeof(pInvis->action_string), 0);
        }
        if (pInvis->version > 3) {
            fSuccess &= FileRead(hFile, &pInvis->flag, 1, 0);
            fSuccess &= FileRead(hFile, &pInvis->action_value, 4, 0);
        }
        if (pInvis->version > 4) {
            fSuccess &= FileRead(hFile, &pInvis->has_legacy_geometry, 1, 0);
            if (pInvis->has_legacy_geometry != 0) {
                fSuccess &= FileRead(hFile, &pInvis->geometry_kind, 1, 0);
                if (pInvis->linked_record_kind_gate == 2) {
                    W8LevelFileLinkedRecord* pRecord = static_cast<W8LevelFileLinkedRecord*>(
                        malloc(sizeof(W8LevelFileLinkedRecord)));
                    unsigned char okRecord = 0;
                    if (pRecord != 0) {
                        okRecord = FileRead(hFile, &pRecord->kind, 1, 0);
                        okRecord &=
                            FileRead(hFile, pRecord->vertices, sizeof(pRecord->vertices), 0);
                        okRecord &= FileRead(hFile, &pRecord->linked_face, 2, 0);
                        g_level_file->linked_records[g_level_file->num_linked_records] = pRecord;
                        ++g_level_file->num_linked_records;
                        pInvis->pRecord = pRecord;
                    }
                    if ((fSuccess & okRecord) != 0) {
                        pInvis->pRecord->normal_scale = pInvis->range;
                        pInvis->pRecord->forward_scale = 1.0f;
                        pTrigger->pData = pInvis;
                        return fSuccess & okRecord;
                    }
                    return 0;
                }
            }
        }
        g_level_file->invisible_planes[g_level_file->num_invisible_planes] = pInvis->pPlane;
        ++g_level_file->num_invisible_planes;
        pTrigger->pData = pInvis;
        return fSuccess;
    }
    case 3: {
        W8LevelFileSound* pSound = static_cast<W8LevelFileSound*>(malloc(sizeof(W8LevelFileSound)));
        if (pSound == 0) {
            srAssertFail("pSound", LEVELFILE_CPP, 0x46c, 0);
        }
        memset(pSound, 0, sizeof(W8LevelFileSound));
        fSuccess &= FileRead(hFile, &pSound->version, 1, 0);
        fSuccess &= FileRead(hFile, &pSound->volume_min, 4, 0);
        fSuccess &= FileRead(hFile, &pSound->volume_max, 4, 0);
        fSuccess &= FileRead(hFile, &pSound->speed_min, 4, 0);
        fSuccess &= FileRead(hFile, &pSound->speed_max, 4, 0);
        fSuccess &= FileRead(hFile, &pSound->time_min, 4, 0);
        fSuccess &= FileRead(hFile, &pSound->time_max, 4, 0);
        fSuccess &= FileRead(hFile, &pSound->unbounded, 4, 0);
        fSuccess &= FileRead(hFile, &pSound->radius, 4, 0);
        fSuccess &= FileRead(hFile, &pSound->position, sizeof(pSound->position), 0);
        fSuccess &= FileRead(hFile, &pSound->region_u, sizeof(pSound->region_u), 0);
        fSuccess &= FileRead(hFile, &pSound->region_v, sizeof(pSound->region_v), 0);
        fSuccess &= FileRead(hFile, pSound->wave, sizeof(pSound->wave), 0);
        if (pSound->version > 1) {
            fSuccess &= FileRead(hFile, &pSound->has_position, 1, 0);
            fSuccess &= FileRead(hFile, &pSound->looping, 1, 0);
        }
        if (pSound->version > 2) {
            fSuccess &= FileRead(hFile, &pSound->region_center, sizeof(pSound->region_center), 0);
            fSuccess &= FileRead(hFile, &pSound->region_angle, 4, 0);
            fSuccess &= FileRead(hFile, &pSound->region_min, sizeof(pSound->region_min), 0);
            fSuccess &= FileRead(hFile, &pSound->region_max, sizeof(pSound->region_max), 0);
        }
        if (pSound->version > 3) {
            fSuccess &= FileRead(hFile, pSound->name, sizeof(pSound->name), 0);
            ReportBuildStatus(
                5, reinterpret_cast< // reinterpret-ok: String returns a logging buffer
                       const char*>(
                       String("Sound Trigger: %s\n",
                              pSound->name))); // reinterpret-ok: String returns a logging buffer
        }
        if (pSound->version > 4) {
            fSuccess &= FileRead(hFile, &pSound->shared, 1, 0);
        }
        pTrigger->pData = pSound;
        return fSuccess;
    }
    case 4:
        return ReadSuperTriggerFile(hFile, pTrigger) & fSuccess;
    default:
        return fSuccess;
    }
}

// FUNCTION: WIZ8 0x004D23F0
BOOLEAN WriteTriggerFile(int hFile, W8LevelFileTrigger* pTrigger)
{
    unsigned char fSuccess = FileWrite(hFile, &pTrigger->version, 1, 0) != 0;
    fSuccess &= fSuccess && FileWrite(hFile, &pTrigger->type, 1, 0);
    switch (pTrigger->type) {
    case 1: {
        W8LevelFileSwitch* pSwitch = static_cast<W8LevelFileSwitch*>(pTrigger->pData);
        if (pSwitch == 0) {
            srAssertFail("pSwitch", LEVELFILE_CPP, 0x4be, 0);
        }
        fSuccess &= FileWrite(hFile, &pSwitch->version, 1, 0);
        fSuccess &= FileWrite(hFile, &pSwitch->cycle_bounce, 4, 0);
        fSuccess &= FileWrite(hFile, &pSwitch->state_count, 4, 0);
        fSuccess &= FileWrite(hFile, &pSwitch->animate_states, 4, 0);
        fSuccess &= FileWrite(hFile, &pSwitch->range, 4, 0);
        fSuccess &= FileWrite(hFile, &pSwitch->action, 4, 0);
        fSuccess &= FileWrite(hFile, &pSwitch->value, 4, 0);
        fSuccess &= FileWrite(hFile, &pSwitch->animate_action, 4, 0);
        fSuccess &= FileWrite(hFile, &pSwitch->packed_flags, 1, 0);
        fSuccess &= FileWrite(hFile, &pSwitch->enabled, 1, 0);
        fSuccess &= FileWrite(hFile, pSwitch->name, sizeof(pSwitch->name), 0);
        fSuccess &= FileWrite(hFile, pSwitch->recipients, sizeof(pSwitch->recipients), 0);
        fSuccess &= FileWrite(hFile, pSwitch->sound, sizeof(pSwitch->sound), 0);
        if (fSuccess == 0) {
            srAssertFail("fSuccess", LEVELFILE_CPP, 0x4cd, 0);
        }
        if (pSwitch->version > 1) {
            fSuccess &= FileWrite(hFile, &pSwitch->minimum_range, 4, 0);
            fSuccess &= FileWrite(hFile, pSwitch->surface_id, sizeof(pSwitch->surface_id), 0);
        }
        if (pSwitch->version > 2) {
            fSuccess &= FileWrite(hFile, &pSwitch->has_door_trigger, 1, 0);
            if (pSwitch->has_door_trigger != 0) {
                fSuccess &= FileWrite(hFile, &pSwitch->door.kind, 1, 0);
                if (pSwitch->door.kind == 1) {
                    fSuccess &= WriteDoorTriggerFile(hFile, &pSwitch->door);
                }
            }
        }
        if (pSwitch->version > 3) {
            fSuccess &= FileWrite(hFile, &pSwitch->action_value, 4, 0);
        }
        free(pSwitch);
        return fSuccess;
    }
    case 2: {
        W8LevelFileInvisible* pInvis = static_cast<W8LevelFileInvisible*>(pTrigger->pData);
        if (pInvis == 0) {
            srAssertFail("pInvis", LEVELFILE_CPP, 0x4ea, 0);
        }
        fSuccess &= FileWrite(hFile, &pInvis->version, 1, 0);
        fSuccess &= FileWrite(hFile, &pInvis->range, 4, 0);
        fSuccess &= FileWrite(hFile, &pInvis->position, sizeof(pInvis->position), 0);
        fSuccess &= FileWrite(hFile, &pInvis->action, 4, 0);
        fSuccess &= FileWrite(hFile, &pInvis->searchable, 4, 0);
        fSuccess &= FileWrite(hFile, &pInvis->fire_linked, 1, 0);
        fSuccess &= FileWrite(hFile, &pInvis->enabled, 1, 0);
        fSuccess &= FileWrite(hFile, pInvis->name, sizeof(pInvis->name), 0);
        fSuccess &= FileWrite(hFile, pInvis->recipients, sizeof(pInvis->recipients), 0);
        if (pInvis->version > 1) {
            fSuccess &= FileWrite(hFile, &pInvis->plane_flag, 1, 0);
            fSuccess &= FileWrite(hFile, pInvis->pPlane, sizeof(W8LevelFilePlane), 0);
            free(pInvis->pPlane);
        }
        if (pInvis->version > 2) {
            fSuccess &= FileWrite(hFile, &pInvis->angle, 4, 0);
            fSuccess &= FileWrite(hFile, &pInvis->direction, sizeof(pInvis->direction), 0);
            fSuccess &= FileWrite(hFile, &pInvis->unused, 1, 0);
            fSuccess &= FileWrite(hFile, pInvis->action_string, sizeof(pInvis->action_string), 0);
        }
        if (pInvis->version > 3) {
            fSuccess &= FileWrite(hFile, &pInvis->flag, 1, 0);
            fSuccess &= FileWrite(hFile, &pInvis->action_value, 4, 0);
        }
        if (pInvis->version > 4) {
            fSuccess &= FileWrite(hFile, &pInvis->has_legacy_geometry, 1, 0);
            if (pInvis->has_legacy_geometry != 0) {
                fSuccess &= FileWrite(hFile, &pInvis->geometry_kind, 1, 0);
                if (pInvis->linked_record_kind_gate == 2) {
                    W8LevelFileLinkedRecord* pRecord = pInvis->pRecord;
                    unsigned char okRecord = 0;
                    if (pRecord != 0) {
                        okRecord = FileWrite(hFile, &pRecord->kind, 1, 0);
                        okRecord &=
                            FileWrite(hFile, pRecord->vertices, sizeof(pRecord->vertices), 0);
                        okRecord &= FileWrite(hFile, &pRecord->linked_face, 2, 0);
                        free(pRecord);
                    }
                    fSuccess &= okRecord;
                    if (fSuccess == 0) {
                        return 0;
                    }
                }
            }
        }
        free(pInvis);
        return fSuccess;
    }
    case 3: {
        W8LevelFileSound* pSound = static_cast<W8LevelFileSound*>(pTrigger->pData);
        if (pSound == 0) {
            srAssertFail("pSound", LEVELFILE_CPP, 0x51e, 0);
        }
        fSuccess &= FileWrite(hFile, &pSound->version, 1, 0);
        fSuccess &= FileWrite(hFile, &pSound->volume_min, 4, 0);
        fSuccess &= FileWrite(hFile, &pSound->volume_max, 4, 0);
        fSuccess &= FileWrite(hFile, &pSound->speed_min, 4, 0);
        fSuccess &= FileWrite(hFile, &pSound->speed_max, 4, 0);
        fSuccess &= FileWrite(hFile, &pSound->time_min, 4, 0);
        fSuccess &= FileWrite(hFile, &pSound->time_max, 4, 0);
        fSuccess &= FileWrite(hFile, &pSound->unbounded, 4, 0);
        fSuccess &= FileWrite(hFile, &pSound->radius, 4, 0);
        fSuccess &= FileWrite(hFile, &pSound->position, sizeof(pSound->position), 0);
        fSuccess &= FileWrite(hFile, &pSound->region_u, sizeof(pSound->region_u), 0);
        fSuccess &= FileWrite(hFile, &pSound->region_v, sizeof(pSound->region_v), 0);
        fSuccess &= FileWrite(hFile, pSound->wave, sizeof(pSound->wave), 0);
        if (pSound->version > 1) {
            fSuccess &= FileWrite(hFile, &pSound->has_position, 1, 0);
            fSuccess &= FileWrite(hFile, &pSound->looping, 1, 0);
        }
        if (pSound->version > 2) {
            fSuccess &= FileWrite(hFile, &pSound->region_center, sizeof(pSound->region_center), 0);
            fSuccess &= FileWrite(hFile, &pSound->region_angle, 4, 0);
            fSuccess &= FileWrite(hFile, &pSound->region_min, sizeof(pSound->region_min), 0);
            fSuccess &= FileWrite(hFile, &pSound->region_max, sizeof(pSound->region_max), 0);
        }
        if (pSound->version > 3) {
            fSuccess &= FileWrite(hFile, pSound->name, sizeof(pSound->name), 0);
        }
        if (pSound->version > 4) {
            fSuccess &= FileWrite(hFile, &pSound->shared, 1, 0);
        }
        free(pSound);
        return fSuccess;
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
    unsigned char fSuccess = FileRead(hFile, &pSuper->version, 1, 0);
    fSuccess &= FileRead(hFile, pSuper->name, sizeof(pSuper->name), 0);
    fSuccess &= FileRead(hFile, &pSuper->flags, 1, 0);
    fSuccess &= FileRead(hFile, &pSuper->active, 1, 0);
    fSuccess &= FileRead(hFile, &pSuper->kind, 1, 0);
    fSuccess &= FileRead(hFile, &pSuper->when_active, 1, 0);
    fSuccess &= FileRead(hFile, &pSuper->prop_index, 1, 0);
    fSuccess &= FileRead(hFile, &pSuper->activation_count, 1, 0);
    fSuccess &= FileRead(hFile, &pSuper->inactive_count, 1, 0);
    fSuccess &= FileRead(hFile, &pSuper->trigger, 4, 0);
    fSuccess &= FileRead(hFile, &pSuper->trigger_on, 4, 0);
    fSuccess &= FileRead(hFile, &pSuper->trigger_off, 4, 0);
    fSuccess &= FileRead(hFile, pSuper->recipients, sizeof(pSuper->recipients), 0);
    fSuccess &= FileRead(hFile, &pSuper->ataxia_or_cure, 1, 0);
    fSuccess &= FileRead(hFile, pSuper->ps_events, sizeof(pSuper->ps_events), 0);
    fSuccess &= FileRead(hFile, &pSuper->allow_save, 1, 0);
    fSuccess &= FileRead(hFile, &pSuper->price, 4, 0);
    fSuccess &= FileRead(hFile, &pSuper->door_kind, 1, 0);
    fSuccess &= FileRead(hFile, pSuper->animation, sizeof(pSuper->animation), 0);
    if (!fSuccess) {
        return FALSE;
    }
    ReportBuildStatus(
        5,
        reinterpret_cast<const char*>( // reinterpret-ok: String returns a logging buffer
            String("Super Trigger: %s, recipients: %s\n", pSuper->name, pSuper->recipients)));
    if (pSuper->version >= 2) {
        fSuccess &= FileRead(hFile, pSuper->size, sizeof(pSuper->size), 0);
        fSuccess &= FileRead(hFile, &pSuper->direction, 4, 0);
        fSuccess &= FileRead(hFile, &pSuper->wait0, 1, 0);
        fSuccess &= FileRead(hFile, &pSuper->wait1, 1, 0);
        fSuccess &= FileRead(hFile, &pSuper->wait2, 1, 0);
        fSuccess &= FileRead(hFile, &pSuper->loop, 1, 0);
        fSuccess &= FileRead(hFile, &pSuper->speed, 0x10, 0);
        if (!fSuccess) {
            return FALSE;
        }
    }
    fSuccess &= FileRead(hFile, &pSuper->ignore, 1, 0);
    fSuccess &= FileRead(hFile, &pSuper->group, 1, 0);
    fSuccess &= FileRead(hFile, &pSuper->set_group, 1, 0);
    fSuccess &= FileRead(hFile, pSuper->groups, sizeof(pSuper->groups), 0);
    fSuccess &= FileRead(hFile, pSuper->objects, sizeof(pSuper->objects), 0);
    fSuccess &= FileRead(hFile, &pSuper->close_door, 1, 0);
    if (!fSuccess) {
        return FALSE;
    }
    fSuccess &= FileRead(hFile, &pSuper->wait3, 4, 0);
    fSuccess &= FileRead(hFile, &pSuper->field, 4, 0);
    fSuccess &= FileRead(hFile, pSuper->event, sizeof(pSuper->event), 0);
    fSuccess &= FileRead(hFile, &pSuper->normal_scale, 4, 0);
    if (!fSuccess) {
        return FALSE;
    }
    if (pSuper->version >= 3) {
        fSuccess &= FileRead(hFile, pSuper->particle_system, sizeof(pSuper->particle_system), 0);
    }
    if ((pSuper->flags & 1) == 0) {
        fSuccess &= FileRead(hFile, &pSuper->placement_kind, 1, 0);
        if (pSuper->placement_kind == 1) {
            pSuper->pPosition = static_cast<W8LevelFileTriggerPosition*>(
                malloc(sizeof(W8LevelFileTriggerPosition)));
            if (pSuper->pPosition == 0) {
                ReportBuildStatus(
                    7, "ReadSuperTrigger: Could not allocate Trigger Position structure.\n");
                return FALSE;
            }
            fSuccess &= FileRead(hFile, pSuper->pPosition, sizeof(W8LevelFileTriggerPosition), 0);
        } else if (pSuper->placement_kind == 2) {
            pSuper->pPlane = static_cast<W8LevelFilePlane*>(malloc(sizeof(W8LevelFilePlane)));
            if (pSuper->pPlane == 0) {
                ReportBuildStatus(
                    7, "ReadSuperTrigger: Could not allocate Trigger Plane structure.\n");
                return FALSE;
            }
            fSuccess &= FileRead(hFile, pSuper->pPlane, sizeof(W8LevelFilePlane), 0);
            g_level_file->invisible_planes[g_level_file->num_invisible_planes] = pSuper->pPlane;
            ++g_level_file->num_invisible_planes;
        }
        if (!fSuccess) {
            return FALSE;
        }
        fSuccess &= FileRead(hFile, &pSuper->has_hotspot, 1, 0);
        if (pSuper->has_hotspot != 0) {
            pSuper->pHotSpot =
                static_cast<W8LevelFileTriggerHotSpot*>(malloc(sizeof(W8LevelFileTriggerHotSpot)));
            if (pSuper->pHotSpot == 0) {
                ReportBuildStatus(
                    7, "ReadSuperTrigger: Could not allocate Trigger HotSpot structure.\n");
                return FALSE;
            }
            fSuccess &= FileRead(hFile, pSuper->pHotSpot, sizeof(W8LevelFileTriggerHotSpot), 0);
        }
    }
    if (!fSuccess) {
        return FALSE;
    }
    fSuccess &= FileRead(hFile, &pSuper->has_door, 1, 0);
    if (pSuper->has_door != 0) {
        fSuccess &= FileRead(hFile, &pSuper->door.kind, 1, 0);
        if (pSuper->door.kind == 1) {
            fSuccess = ReadDoorTriggerFile(hFile, &pSuper->door);
        } else if (pSuper->door.kind == 2) {
            W8LevelFileLinkedRecord* pRecord =
                static_cast<W8LevelFileLinkedRecord*>(malloc(sizeof(W8LevelFileLinkedRecord)));
            unsigned char ok = 0;
            if (pRecord != 0) {
                ok = FileRead(hFile, &pRecord->kind, 1, 0);
                ok &= FileRead(hFile, pRecord->vertices, sizeof(pRecord->vertices), 0);
                ok &= FileRead(hFile, &pRecord->linked_face, 2, 0);
                g_level_file->linked_records[g_level_file->num_linked_records] = pRecord;
                ++g_level_file->num_linked_records;
                pSuper->pRecord = pRecord;
            }
            fSuccess &= ok;
            /* Retail stores through pRecord even when the allocation failed. */
            pSuper->pRecord->normal_scale = pSuper->normal_scale;
            pSuper->pRecord->forward_scale = pSuper->direction;
        }
    }
    pTrigger->pData = pSuper;
    return fSuccess;
}

// FUNCTION: WIZ8 0x004D3000
BOOLEAN WriteSuperTriggerFile(int hFile, W8LevelFileTrigger* pTrigger)
{
    W8LevelFileSuperTrigger* pSuper = static_cast<W8LevelFileSuperTrigger*>(pTrigger->pData);
    if (pSuper == 0) {
        ReportBuildStatus(7, "WriteSuperTrigger: Couldn't create SuperTrigger structure.\n");
        return 0;
    }
    unsigned char fSuccess = FileWrite(hFile, &pSuper->version, 1, 0);
    fSuccess &= FileWrite(hFile, pSuper->name, sizeof(pSuper->name), 0);
    fSuccess &= FileWrite(hFile, &pSuper->flags, 1, 0);
    fSuccess &= FileWrite(hFile, &pSuper->active, 1, 0);
    fSuccess &= FileWrite(hFile, &pSuper->kind, 1, 0);
    fSuccess &= FileWrite(hFile, &pSuper->when_active, 1, 0);
    fSuccess &= FileWrite(hFile, &pSuper->prop_index, 1, 0);
    fSuccess &= FileWrite(hFile, &pSuper->activation_count, 1, 0);
    fSuccess &= FileWrite(hFile, &pSuper->inactive_count, 1, 0);
    fSuccess &= FileWrite(hFile, &pSuper->trigger, 4, 0);
    fSuccess &= FileWrite(hFile, &pSuper->trigger_on, 4, 0);
    fSuccess &= FileWrite(hFile, &pSuper->trigger_off, 4, 0);
    fSuccess &= FileWrite(hFile, pSuper->recipients, sizeof(pSuper->recipients), 0);
    fSuccess &= FileWrite(hFile, &pSuper->ataxia_or_cure, 1, 0);
    fSuccess &= FileWrite(hFile, pSuper->ps_events, sizeof(pSuper->ps_events), 0);
    fSuccess &= FileWrite(hFile, &pSuper->allow_save, 1, 0);
    fSuccess &= FileWrite(hFile, &pSuper->price, 4, 0);
    fSuccess &= FileWrite(hFile, &pSuper->door_kind, 1, 0);
    fSuccess &= FileWrite(hFile, pSuper->animation, sizeof(pSuper->animation), 0);
    if (fSuccess == 0) {
        return 0;
    }
    if (pSuper->version >= 2) {
        fSuccess &= FileWrite(hFile, pSuper->size, sizeof(pSuper->size), 0);
        fSuccess &= FileWrite(hFile, &pSuper->direction, 4, 0);
        fSuccess &= FileWrite(hFile, &pSuper->wait0, 1, 0);
        fSuccess &= FileWrite(hFile, &pSuper->wait1, 1, 0);
        fSuccess &= FileWrite(hFile, &pSuper->wait2, 1, 0);
        fSuccess &= FileWrite(hFile, &pSuper->loop, 1, 0);
        fSuccess &= FileWrite(hFile, &pSuper->speed, 0x10, 0);
        if (fSuccess == 0) {
            return 0;
        }
    }
    fSuccess &= FileWrite(hFile, &pSuper->ignore, 1, 0);
    fSuccess &= FileWrite(hFile, &pSuper->group, 1, 0);
    fSuccess &= FileWrite(hFile, &pSuper->set_group, 1, 0);
    fSuccess &= FileWrite(hFile, pSuper->groups, sizeof(pSuper->groups), 0);
    fSuccess &= FileWrite(hFile, pSuper->objects, sizeof(pSuper->objects), 0);
    fSuccess &= FileWrite(hFile, &pSuper->close_door, 1, 0);
    if (fSuccess == 0) {
        return 0;
    }
    fSuccess &= FileWrite(hFile, &pSuper->wait3, 4, 0);
    fSuccess &= FileWrite(hFile, &pSuper->field, 4, 0);
    fSuccess &= FileWrite(hFile, pSuper->event, sizeof(pSuper->event), 0);
    fSuccess &= FileWrite(hFile, &pSuper->normal_scale, 4, 0);
    if (fSuccess == 0) {
        return 0;
    }
    if (pSuper->version >= 3) {
        fSuccess &= FileWrite(hFile, pSuper->particle_system, sizeof(pSuper->particle_system), 0);
    }
    if ((pSuper->flags & 1) == 0) {
        fSuccess &= FileWrite(hFile, &pSuper->placement_kind, 1, 0);
        if (pSuper->placement_kind == 1) {
            if (pSuper->pPosition == 0) {
                ReportBuildStatus(7, "WriteSuperTrigger: No Trigger Position structure.\n");
                return 0;
            }
            fSuccess &= FileWrite(hFile, pSuper->pPosition, sizeof(W8LevelFileTriggerPosition), 0);
            free(pSuper->pPosition);
        } else if (pSuper->placement_kind == 2) {
            if (pSuper->pPlane == 0) {
                ReportBuildStatus(7, "WriteSuperTrigger: No FileTriggerPlane structure.\n");
                return 0;
            }
            fSuccess &= FileWrite(hFile, pSuper->pPlane, sizeof(W8LevelFilePlane), 0);
            free(pSuper->pPlane);
        }
        if (fSuccess == 0) {
            return 0;
        }
        fSuccess &= FileWrite(hFile, &pSuper->has_hotspot, 1, 0);
        if (pSuper->has_hotspot != 0) {
            if (pSuper->pHotSpot == 0) {
                ReportBuildStatus(7, "WriteSuperTrigger: No Trigger HotSpot structure.\n");
                return 0;
            }
            fSuccess &= FileWrite(hFile, pSuper->pHotSpot, sizeof(W8LevelFileTriggerHotSpot), 0);
            free(pSuper->pHotSpot);
        }
    }
    if (fSuccess == 0) {
        return 0;
    }
    fSuccess &= FileWrite(hFile, &pSuper->has_door, 1, 0);
    if (pSuper->has_door != 0) {
        fSuccess &= FileWrite(hFile, &pSuper->door.kind, 1, 0);
        if (pSuper->door.kind == 1) {
            fSuccess = WriteDoorTriggerFile(hFile, &pSuper->door);
        } else if (pSuper->door.kind == 2) {
            W8LevelFileLinkedRecord* pRecord = pSuper->pRecord;
            unsigned char ok;
            if (pRecord == 0) {
                ok = 0;
            } else {
                ok = FileWrite(hFile, &pRecord->kind, 1, 0);
                ok &= FileWrite(hFile, pRecord->vertices, sizeof(pRecord->vertices), 0);
                ok &= FileWrite(hFile, &pRecord->linked_face, 2, 0);
                free(pRecord);
            }
            fSuccess &= ok;
        }
    }
    free(pSuper);
    return fSuccess;
}

// FUNCTION: WIZ8 0x004D3540
BOOLEAN ReadDoorTriggerFile(int hFile, W8LevelFileDoorRef* pDoor)
{
    W8LevelFileDoor* pDoorRec = static_cast<W8LevelFileDoor*>(malloc(sizeof(W8LevelFileDoor)));
    if (pDoorRec == 0) {
        return 0;
    }
    unsigned char fSuccess = FileRead(hFile, &pDoorRec->version, 1, 0);
    fSuccess &= FileRead(hFile, &pDoorRec->flags[0], 1, 0);
    fSuccess &= FileRead(hFile, &pDoorRec->flags[1], 1, 0);
    fSuccess &= FileRead(hFile, &pDoorRec->flags[2], 1, 0);
    fSuccess &= FileRead(hFile, &pDoorRec->flags[3], 1, 0);
    fSuccess &= FileRead(hFile, &pDoorRec->flags[4], 1, 0);
    fSuccess &= FileRead(hFile, &pDoorRec->flags[5], 1, 0);
    fSuccess &= FileRead(hFile, &pDoorRec->flags[6], 1, 0);
    fSuccess &= FileRead(hFile, &pDoorRec->flags[7], 1, 0);
    fSuccess &= FileRead(hFile, &pDoorRec->flags[8], 1, 0);
    fSuccess &= FileRead(hFile, &pDoorRec->item, 2, 0);
    fSuccess &= FileRead(hFile, &pDoorRec->has_position, 1, 0);
    fSuccess &= FileRead(hFile, &pDoorRec->position, sizeof(pDoorRec->position), 0);
    fSuccess &= FileRead(hFile, pDoorRec->linked_trigger, 0x80, 0);
    pDoor->door = pDoorRec;
    return fSuccess;
}

// FUNCTION: WIZ8 0x004D3660
BOOLEAN WriteDoorTriggerFile(int hFile, W8LevelFileDoorRef* pDoor)
{
    W8LevelFileDoor* pDoorRec = pDoor->door;
    if (pDoorRec == 0) {
        return 0;
    }
    unsigned char fSuccess = FileWrite(hFile, &pDoorRec->version, 1, 0);
    fSuccess &= FileWrite(hFile, &pDoorRec->flags[0], 1, 0);
    fSuccess &= FileWrite(hFile, &pDoorRec->flags[1], 1, 0);
    fSuccess &= FileWrite(hFile, &pDoorRec->flags[2], 1, 0);
    fSuccess &= FileWrite(hFile, &pDoorRec->flags[3], 1, 0);
    fSuccess &= FileWrite(hFile, &pDoorRec->flags[4], 1, 0);
    fSuccess &= FileWrite(hFile, &pDoorRec->flags[5], 1, 0);
    fSuccess &= FileWrite(hFile, &pDoorRec->flags[6], 1, 0);
    fSuccess &= FileWrite(hFile, &pDoorRec->flags[7], 1, 0);
    fSuccess &= FileWrite(hFile, &pDoorRec->flags[8], 1, 0);
    fSuccess &= FileWrite(hFile, &pDoorRec->item, 2, 0);
    fSuccess &= FileWrite(hFile, &pDoorRec->has_position, 1, 0);
    fSuccess &= FileWrite(hFile, &pDoorRec->position, sizeof(pDoorRec->position), 0);
    fSuccess &= FileWrite(hFile, pDoorRec->linked_trigger, 0x80, 0);
    free(pDoorRec);
    return fSuccess;
}

// FUNCTION: WIZ8 0x004D3770
BOOLEAN ReadPathAIFile(int hFile, W8LevelFilePathAI* pPathAI)
{
    unsigned char fSuccess = FileRead(hFile, &pPathAI->version, 1, 0);
    fSuccess &= FileRead(hFile, &pPathAI->scaled, 1, 0);
    fSuccess &= FileRead(hFile, &pPathAI->position, 4, 0);
    fSuccess &= FileRead(hFile, pPathAI->unknown_06, 4, 0);
    fSuccess &= FileRead(hFile, &pPathAI->path_count, 4, 0);
    if (pPathAI->scaled == 2) {
        if (pPathAI->path_count != 0) {
            pPathAI->pScaledPaths = static_cast<W8LevelFileScaledPathNode*>(
                malloc(pPathAI->path_count * sizeof(W8LevelFileScaledPathNode)));
            if (pPathAI->pScaledPaths == 0) {
                srAssertFail("pPathAI->pScaledPaths", LEVELFILE_CPP, 0x732, 0);
            }
            fSuccess &= FileRead(hFile, pPathAI->pScaledPaths,
                                 pPathAI->path_count * sizeof(W8LevelFileScaledPathNode), 0);
        }
    } else if (pPathAI->path_count != 0) {
        pPathAI->pPaths = static_cast<W8LevelFilePathNode*>(
            malloc(pPathAI->path_count * sizeof(W8LevelFilePathNode)));
        if (pPathAI->pPaths == 0) {
            srAssertFail("pPathAI->pPaths", LEVELFILE_CPP, 0x73d, 0);
        }
        fSuccess &=
            FileRead(hFile, pPathAI->pPaths, pPathAI->path_count * sizeof(W8LevelFilePathNode), 0);
    }
    return fSuccess;
}

// FUNCTION: WIZ8 0x004D38E0
BOOLEAN WritePathAIFile(int hFile, W8LevelFilePathAI* pPathAI)
{
    unsigned char fSuccess = FileWrite(hFile, &pPathAI->version, 1, 0);
    fSuccess &= FileWrite(hFile, &pPathAI->scaled, 1, 0);
    fSuccess &= FileWrite(hFile, &pPathAI->position, 4, 0);
    fSuccess &= FileWrite(hFile, pPathAI->unknown_06, 4, 0);
    fSuccess &= FileWrite(hFile, &pPathAI->path_count, 4, 0);
    if (pPathAI->scaled == 2) {
        if (pPathAI->path_count != 0) {
            if (pPathAI->pScaledPaths == 0) {
                srAssertFail("pPathAI->pScaledPaths", LEVELFILE_CPP, 0x761, 0);
            }
            fSuccess &= FileWrite(hFile, pPathAI->pScaledPaths,
                                  pPathAI->path_count * sizeof(W8LevelFileScaledPathNode), 0);
            free(pPathAI->pScaledPaths);
            pPathAI->pScaledPaths = 0;
        }
    } else if (pPathAI->path_count != 0) {
        if (pPathAI->pPaths == 0) {
            srAssertFail("pPathAI->pPaths", LEVELFILE_CPP, 0x76b, 0);
        }
        fSuccess &=
            FileWrite(hFile, pPathAI->pPaths, pPathAI->path_count * sizeof(W8LevelFilePathNode), 0);
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
    fSuccess &= FileRead(hFile, &pAnimObj->version, 1, 0);
    fSuccess &= FileRead(hFile, &pAnimObj->num_anims, 1, 0);
    fSuccess &= FileRead(hFile, &pAnimObj->animation_playing, 1, 0);
    fSuccess &= FileRead(hFile, &pAnimObj->frame_method, 1, 0);
    fSuccess &= FileRead(hFile, &pAnimObj->behaviour, 1, 0);
    fSuccess &= FileRead(hFile, &pAnimObj->cycle, 1, 0);
    fSuccess &= FileRead(hFile, &pAnimObj->path_lists, 1, 0);
    if (pAnimObj->version >= 3) {
        fSuccess = fSuccess && FileRead(hFile, &pAnimObj->playback_scale, 4, 0);
    } else {
        pAnimObj->playback_scale = 15.0f;
    }
    if (pAnimObj->version >= 5) {
        fSuccess = fSuccess && FileRead(hFile, &pAnimObj->start_frame, 1, 0);
    } else {
        pAnimObj->start_frame = 0;
    }
    if (pAnimObj->version >= 6) {
        fSuccess = fSuccess && FileRead(hFile, &pAnimObj->random_play, 1, 0) &&
                   FileRead(hFile, &pAnimObj->play_chance, 4, 0);
    } else {
        pAnimObj->random_play = 0;
        pAnimObj->play_chance = 1.0f;
    }
    fSuccess = fSuccess && FileRead(hFile, pAnimObj->discarded, 0x32, 0);
    if (!fSuccess) {
        srAssertFail("fSuccess", LEVELFILE_CPP, 0x7a5, 0);
    }
    if (pAnimObj->num_anims != 0) {
        pAnimObj->abHowMany = static_cast<char*>(malloc(pAnimObj->num_anims));
        if (pAnimObj->abHowMany == 0) {
            srAssertFail("pAnimObj->abHowMany", LEVELFILE_CPP, 0x7aa, 0);
        }
        fSuccess &= FileRead(hFile, pAnimObj->abHowMany, pAnimObj->num_anims, 0);
    }
    if (pAnimObj->version >= 7) {
        fSuccess &= FileRead(hFile, &pAnimObj->num_bound_box, 1, 0);
        if (pAnimObj->num_bound_box != 0) {
            pAnimObj->pBoundBox = static_cast<W8LevelFileBounds*>(
                malloc(pAnimObj->num_bound_box * sizeof(W8LevelFileBounds)));
            if (pAnimObj->pBoundBox == 0) {
                srAssertFail("pAnimObj->pBoundBox", LEVELFILE_CPP, 0x7b3, 0);
            }
            fSuccess &= FileRead(hFile, pAnimObj->pBoundBox,
                                 pAnimObj->num_bound_box * sizeof(W8LevelFileBounds), 0);
        }
    }
    if (fSuccess == 0) {
        return FALSE;
    }
    if (pAnimObj->version >= 8) {
        fSuccess = FileRead(hFile, &pAnimObj->num_anim_lights, 1, 0);
        if (fSuccess == 0) {
            return FALSE;
        }
        if (pAnimObj->num_anim_lights != 0) {
            pAnimObj->pAnimLights = static_cast<W8LevelFileAnimLight*>(
                malloc(pAnimObj->num_anim_lights * sizeof(W8LevelFileAnimLight)));
            if (pAnimObj->pAnimLights == 0) {
                return FALSE;
            }
            memset(pAnimObj->pAnimLights, 0,
                   pAnimObj->num_anim_lights * sizeof(W8LevelFileAnimLight));
            for (i = 0; i < pAnimObj->num_anim_lights; ++i) {
                fSuccess &= ReadAnimLightFile(hFile, pAnimObj->pAnimLights + i);
                if (fSuccess == 0) {
                    return FALSE;
                }
            }
        }
    }
    if (pAnimObj->path_lists == 0) {
        if ((pAnimObj->version >= 9) &&
            (FileRead(hFile, &pAnimObj->has_path_ai, 1, 0), pAnimObj->has_path_ai != 0)) {
            pAnimObj->pPathAI = static_cast<W8LevelFilePathAI*>(malloc(sizeof(W8LevelFilePathAI)));
            if (pAnimObj->pPathAI == 0) {
                return FALSE;
            }
            fSuccess = ReadPathAIFile(hFile, pAnimObj->pPathAI);
            if (fSuccess == 0) {
                return FALSE;
            }
        }
        if (pAnimObj->num_anims != 0) {
            pAnimObj->pMorphs = static_cast<W8LevelFileMorph*>(
                malloc(pAnimObj->num_anims * sizeof(W8LevelFileMorph)));
            if (pAnimObj->pMorphs == 0) {
                srAssertFail("pAnimObj->pMorphs", LEVELFILE_CPP, 0x7e1, 0);
            }
            memset(pAnimObj->pMorphs, 0, pAnimObj->num_anims * sizeof(W8LevelFileMorph));
            for (i = 0; i < pAnimObj->num_anims; ++i) {
                W8LevelFileMorph* pMorph = pAnimObj->pMorphs + i;
                fSuccess &= FileRead(hFile, &pMorph->channel, 1, 0);
                fSuccess &= FileRead(hFile, &pMorph->num_frames, 1, 0);
                if (pMorph->num_frames != 0) {
                    pMorph->LODMesh.pFrames = static_cast<W8LevelFileFrame*>(
                        malloc(pMorph->num_frames * sizeof(W8LevelFileFrame)));
                    if (pMorph->LODMesh.pFrames == 0) {
                        srAssertFail("pAnimObj->pMorphs[i].LODMesh.pFrames", LEVELFILE_CPP, 0x7eb,
                                     0);
                    }
                    memset(pMorph->LODMesh.pFrames, 0,
                           pMorph->num_frames * sizeof(W8LevelFileFrame));
                    if (pMorph->num_frames != 0) {
                        usFrame = 0;
                        do {
                            W8LevelFileFrame* pFrame =
                                pMorph->LODMesh.pFrames + static_cast<short>(usFrame);
                            fSuccess &= FileRead(hFile, &pFrame->flags, 1, 0);
                            fSuccess &= ReadMeshFile(hFile, &pFrame->mesh);
                            fSuccess &= FileRead(hFile, &pFrame->num_textures, 2, 0);
                            if (pFrame->num_textures != 0) {
                                pFrame->pTextures = static_cast<W8MaterialRecord*>(
                                    malloc(pFrame->num_textures * sizeof(W8MaterialRecord)));
                                if (pFrame->pTextures == 0) {
                                    srAssertFail(
                                        "pAnimObj->pMorphs[i].LODMesh.pFrames[i2].pTextures",
                                        LEVELFILE_CPP, 0x7f9, 0);
                                }
                                memset(pFrame->pTextures, 0,
                                       pFrame->num_textures * sizeof(W8MaterialRecord));
                                unsigned char fTextures = 1;
                                for (j = 0; j < pFrame->num_textures; ++j) {
                                    W8MaterialRecord* pTexture = pFrame->pTextures + j;
                                    fTextures = ReadMaterialRecord(hFile, pTexture);
                                    if (fTextures == 0) {
                                        return FALSE;
                                    }
                                }
                                if (fTextures == 0) {
                                    return FALSE;
                                }
                            }
                            if ((usFrame == 0) && ((pMorph->LODMesh.pFrames->mesh.flags &
                                                    W8_LEVEL_MESH_LOD_VERTICES) != 0)) {
                                usFrame = pMorph->num_frames;
                            }
                            ++usFrame;
                        } while (static_cast<short>(usFrame) <
                                 static_cast<short>(pMorph->num_frames));
                    }
                }
            }
        }
    } else {
        fSuccess &= FileRead(hFile, &pAnimObj->num_transforms, 1, 0);
        if (pAnimObj->num_transforms != 0) {
            pAnimObj->pTransforms = static_cast<W8LevelFileTransform*>(
                malloc(pAnimObj->num_transforms * sizeof(W8LevelFileTransform)));
            if (pAnimObj->pTransforms == 0) {
                srAssertFail("pAnimObj->pTransforms", LEVELFILE_CPP, 0x80e, 0);
            }
            memset(pAnimObj->pTransforms, 0,
                   pAnimObj->num_transforms * sizeof(W8LevelFileTransform));
            for (i = 0; i < pAnimObj->num_transforms; ++i) {
                W8LevelFileTransform* pTransform = pAnimObj->pTransforms + i;
                fSuccess &= FileRead(hFile, &pTransform->channel, 1, 0);
                fSuccess &= FileRead(hFile, &pTransform->num_frames, 1, 0);
                if (pTransform->num_frames != 0) {
                    pTransform->LODMesh.pFrames = static_cast<W8LevelFileFrame*>(
                        malloc(pTransform->num_frames * sizeof(W8LevelFileFrame)));
                    if (pTransform->LODMesh.pFrames == 0) {
                        srAssertFail("pAnimObj->pTransforms[i].LODMesh.pFrames", LEVELFILE_CPP,
                                     0x818, 0);
                    }
                    memset(pTransform->LODMesh.pFrames, 0,
                           pTransform->num_frames * sizeof(W8LevelFileFrame));
                    if (pTransform->num_frames != 0) {
                        short iFrame = 0;
                        do {
                            W8LevelFileFrame* pFrame = pTransform->LODMesh.pFrames + iFrame;
                            fSuccess &= FileRead(hFile, &pFrame->flags, 1, 0);
                            fSuccess &= ReadMeshFile(hFile, &pFrame->mesh);
                            fSuccess &= FileRead(hFile, &pFrame->num_textures, 2, 0);
                            if ((pFrame->num_textures < 0) || (pFrame->num_textures > 500)) {
                                sprintf(g_level_file_error,
                                        "Invalid number of materials in mesh (%d materials).\n",
                                        static_cast<int>(pFrame->num_textures));
                                ReportBuildStatus(7, g_level_file_error);
                                return FALSE;
                            }
                            if (pFrame->num_textures != 0) {
                                pFrame->pTextures = static_cast<W8MaterialRecord*>(
                                    malloc(pFrame->num_textures * sizeof(W8MaterialRecord)));
                                if (pFrame->pTextures == 0) {
                                    srAssertFail(
                                        "pAnimObj->pTransforms[i].LODMesh.pFrames[i2].pTextures",
                                        LEVELFILE_CPP, 0x82e, 0);
                                }
                                memset(pFrame->pTextures, 0,
                                       pFrame->num_textures * sizeof(W8MaterialRecord));
                                unsigned char fTextures = 1;
                                for (j = 0; j < pFrame->num_textures; ++j) {
                                    W8MaterialRecord* pTexture = pFrame->pTextures + j;
                                    fTextures = ReadMaterialRecord(hFile, pTexture);
                                    if (fTextures == 0) {
                                        return FALSE;
                                    }
                                }
                                if (fTextures == 0) {
                                    return FALSE;
                                }
                            }
                            ++iFrame;
                        } while (iFrame < static_cast<short>(pTransform->num_frames));
                    }
                }
                fSuccess &= ReadPathAIFile(hFile, &pTransform->pathAI);
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
    fSuccess &= FileWrite(hFile, &pAnimObj->version, 1, 0);
    fSuccess &= FileWrite(hFile, &pAnimObj->num_anims, 1, 0);
    fSuccess &= FileWrite(hFile, &pAnimObj->animation_playing, 1, 0);
    fSuccess &= FileWrite(hFile, &pAnimObj->frame_method, 1, 0);
    fSuccess &= FileWrite(hFile, &pAnimObj->behaviour, 1, 0);
    fSuccess &= FileWrite(hFile, &pAnimObj->cycle, 1, 0);
    fSuccess &= FileWrite(hFile, &pAnimObj->path_lists, 1, 0);
    if (pAnimObj->version >= 3) {
        fSuccess = fSuccess && FileWrite(hFile, &pAnimObj->playback_scale, 4, 0);
    }
    if (pAnimObj->version >= 5) {
        fSuccess = fSuccess && FileWrite(hFile, &pAnimObj->start_frame, 1, 0);
    }
    if (pAnimObj->version >= 6) {
        fSuccess = fSuccess && FileWrite(hFile, &pAnimObj->random_play, 1, 0) &&
                   FileWrite(hFile, &pAnimObj->play_chance, 4, 0);
    }
    fSuccess = fSuccess && FileWrite(hFile, pAnimObj->discarded, 0x32, 0);
    if (!fSuccess) {
        srAssertFail("fSuccess", LEVELFILE_CPP, 0x867, 0);
    }
    if (pAnimObj->num_anims != 0) {
        if (pAnimObj->abHowMany == 0) {
            srAssertFail("pAnimObj->abHowMany", LEVELFILE_CPP, 0x86b, 0);
        }
        fSuccess &= FileWrite(hFile, pAnimObj->abHowMany, pAnimObj->num_anims, 0);
        free(pAnimObj->abHowMany);
    }
    if (pAnimObj->version >= 7) {
        fSuccess &= FileWrite(hFile, &pAnimObj->num_bound_box, 1, 0);
        if (pAnimObj->num_bound_box != 0) {
            if (pAnimObj->pBoundBox == 0) {
                srAssertFail("pAnimObj->pBoundBox", LEVELFILE_CPP, 0x874, 0);
            }
            fSuccess &= FileWrite(hFile, pAnimObj->pBoundBox,
                                  pAnimObj->num_bound_box * sizeof(W8LevelFileBounds), 0);
            free(pAnimObj->pBoundBox);
        }
    }
    if (fSuccess == 0) {
        return FALSE;
    }
    if (pAnimObj->version >= 8) {
        fSuccess = FileWrite(hFile, &pAnimObj->num_anim_lights, 1, 0);
        if ((pAnimObj->num_anim_lights != 0) && (pAnimObj->pAnimLights != 0)) {
            for (i = 0; i < pAnimObj->num_anim_lights; ++i) {
                fSuccess &= WriteAnimLightFile(hFile, pAnimObj->pAnimLights + i);
                if (fSuccess == 0) {
                    return FALSE;
                }
            }
            free(pAnimObj->pAnimLights);
        }
        if (fSuccess == 0) {
            return FALSE;
        }
    }
    if (pAnimObj->path_lists == 0) {
        if ((pAnimObj->version >= 9) &&
            (FileWrite(hFile, &pAnimObj->has_path_ai, 1, 0), pAnimObj->has_path_ai != 0) &&
            (pAnimObj->pPathAI != 0)) {
            fSuccess = WritePathAIFile(hFile, pAnimObj->pPathAI);
            free(pAnimObj->pPathAI);
            if (fSuccess == 0) {
                return FALSE;
            }
        }
        if (pAnimObj->num_anims != 0) {
            if (pAnimObj->pMorphs == 0) {
                srAssertFail("pAnimObj->pMorphs", LEVELFILE_CPP, 0x89e, 0);
            }
            for (i = 0; i < pAnimObj->num_anims; ++i) {
                W8LevelFileMorph* pMorph = pAnimObj->pMorphs + i;
                fSuccess &= FileWrite(hFile, &pMorph->channel, 1, 0);
                fSuccess &= FileWrite(hFile, &pMorph->num_frames, 1, 0);
                if (pMorph->num_frames != 0) {
                    if (pMorph->LODMesh.pFrames == 0) {
                        srAssertFail("pAnimObj->pMorphs[i].LODMesh.pFrames", LEVELFILE_CPP, 0x8a5,
                                     0);
                    }
                    usFrame = 0;
                    do {
                        W8LevelFileFrame* pFrame =
                            pMorph->LODMesh.pFrames + static_cast<short>(usFrame);
                        fSuccess &= FileWrite(hFile, &pFrame->flags, 1, 0);
                        fSuccess &= WriteMeshFile(hFile, &pFrame->mesh);
                        fSuccess &= FileWrite(hFile, &pFrame->num_textures, 2, 0);
                        if (pFrame->num_textures != 0) {
                            if (pFrame->pTextures == 0) {
                                srAssertFail("pAnimObj->pMorphs[i].LODMesh.pFrames[i2].pTextures",
                                             LEVELFILE_CPP, 0x8af, 0);
                            }
                            fSuccess = 1;
                            for (j = 0; j < pFrame->num_textures; ++j) {
                                W8MaterialRecord* pTexture = pFrame->pTextures + j;
                                fSuccess = WriteMaterialRecord(hFile, pTexture);
                                if (fSuccess == 0) {
                                    return FALSE;
                                }
                            }
                            if (fSuccess == 0) {
                                return FALSE;
                            }
                            free(pFrame->pTextures);
                            pFrame->pTextures = 0;
                        }
                        if ((usFrame == 0) && ((pMorph->LODMesh.pFrames->mesh.flags &
                                                W8_LEVEL_MESH_LOD_VERTICES) != 0)) {
                            usFrame = pMorph->num_frames;
                        }
                        ++usFrame;
                    } while (static_cast<short>(usFrame) < static_cast<short>(pMorph->num_frames));
                    free(pMorph->LODMesh.pFrames);
                    pMorph->LODMesh.pFrames = 0;
                }
            }
            free(pAnimObj->pMorphs);
            pAnimObj->pMorphs = 0;
            return fSuccess;
        }
    } else {
        fSuccess &= FileWrite(hFile, &pAnimObj->num_transforms, 1, 0);
        if (pAnimObj->num_transforms != 0) {
            if (pAnimObj->pTransforms == 0) {
                srAssertFail("pAnimObj->pTransforms", LEVELFILE_CPP, 0x8c7, 0);
            }
            for (i = 0; i < pAnimObj->num_transforms; ++i) {
                W8LevelFileTransform* pTransform = pAnimObj->pTransforms + i;
                fSuccess &= FileWrite(hFile, &pTransform->channel, 1, 0);
                fSuccess &= FileWrite(hFile, &pTransform->num_frames, 1, 0);
                if (pTransform->num_frames != 0) {
                    if (pTransform->LODMesh.pFrames == 0) {
                        srAssertFail("pAnimObj->pTransforms[i].LODMesh.pFrames", LEVELFILE_CPP,
                                     0x8ce, 0);
                    }
                    short iFrame = 0;
                    do {
                        W8LevelFileFrame* pFrame = pTransform->LODMesh.pFrames + iFrame;
                        fSuccess &= FileWrite(hFile, &pFrame->flags, 1, 0);
                        fSuccess &= WriteMeshFile(hFile, &pFrame->mesh);
                        fSuccess &= FileWrite(hFile, &pFrame->num_textures, 2, 0);
                        if (pFrame->num_textures != 0) {
                            if (pFrame->pTextures == 0) {
                                srAssertFail(
                                    "pAnimObj->pTransforms[i].LODMesh.pFrames[i2].pTextures",
                                    LEVELFILE_CPP, 0x8d8, 0);
                            }
                            fSuccess = 1;
                            for (j = 0; j < pFrame->num_textures; ++j) {
                                W8MaterialRecord* pTexture = pFrame->pTextures + j;
                                fSuccess = WriteMaterialRecord(hFile, pTexture);
                                if (fSuccess == 0) {
                                    return FALSE;
                                }
                            }
                            if (fSuccess == 0) {
                                return FALSE;
                            }
                            free(pFrame->pTextures);
                            pFrame->pTextures = 0;
                        }
                        ++iFrame;
                    } while (iFrame < static_cast<short>(pTransform->num_frames));
                    free(pTransform->LODMesh.pFrames);
                    pTransform->LODMesh.pFrames = 0;
                }
                fSuccess &= WritePathAIFile(hFile, &pTransform->pathAI);
            }
            free(pAnimObj->pTransforms);
            pAnimObj->pTransforms = 0;
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
        fSuccess &= FileRead(hFile, &pProp->version, 1, 0);
        fSuccess &= FileRead(hFile, &pProp->bNumFrames, 1, 0);
        if (pProp->version >= 5) {
            fSuccess &= FileRead(hFile, &pProp->option, 1, 0);
            fSuccess &= FileRead(hFile, &pProp->position, sizeof(pProp->position), 0);
        }
        if (pProp->version >= 6) {
            fSuccess &= FileRead(hFile, &pProp->flags, 4, 0);
        }
        if (pProp->version >= 7) {
            fSuccess &= FileRead(hFile, pProp->name, sizeof(pProp->name), 0);
            ReportBuildStatus(
                5, reinterpret_cast< // reinterpret-ok: String returns a logging buffer
                       const char*>(
                       String("Prop: %s\n",
                              pProp->name))); // reinterpret-ok: String returns a logging buffer
        }
        if (fSuccess == 0) {
            srAssertFail("fSuccess", LEVELFILE_CPP, 0x918, 0);
        }
        if (pProp->version >= 8) {
            fSuccess &= FileRead(hFile, &pProp->num_frame_pos, 1, 0);
            if (pProp->num_frame_pos != 0) {
                pProp->usFrame_Pos = static_cast<W8LevelFileFramePosition*>(
                    malloc(pProp->num_frame_pos * sizeof(W8LevelFileFramePosition)));
                if (pProp->usFrame_Pos == 0) {
                    srAssertFail(
                        "pProps[i1].usFrame_Pos", LEVELFILE_CPP, 0x91f,
                        reinterpret_cast< // reinterpret-ok: String returns a logging buffer
                            const char*>( // reinterpret-ok: String returns a logging buffer
                            String("Could not allocate %d segments for prop '%s'!",
                                   static_cast<int>(pProp->num_frame_pos), pProp->name)));
                }
                fSuccess &= FileRead(hFile, pProp->usFrame_Pos, pProp->num_frame_pos << 2, 0);
            }
        }
        unsigned char okAnimObj = ReadAnimObjFile(hFile, &pProp->anim_obj);
        if ((fSuccess & okAnimObj) == 0) {
            srAssertFail("fSuccess", LEVELFILE_CPP, 0x924, 0);
        }
        fSuccess = fSuccess & okAnimObj & FileRead(hFile, &pProp->has_trigger, 1, 0);
        if (fSuccess == 0) {
            srAssertFail("fSuccess", LEVELFILE_CPP, 0x931, 0);
        }
        if (pProp->has_trigger != 0) {
            pProp->pTrigger = static_cast<W8LevelFileTrigger*>(malloc(sizeof(*pProp->pTrigger)));
            if (pProp->pTrigger == 0) {
                srAssertFail("pProps[i1].pTrigger", LEVELFILE_CPP, 0x935, 0);
            }
            memset(pProp->pTrigger, 0, sizeof(W8LevelFileTrigger));
            fSuccess &= ReadTriggerFile(hFile, pProp->pTrigger);
            if (fSuccess == 0) {
                srAssertFail("fSuccess", LEVELFILE_CPP, 0x938, 0);
            }
        }
        if (pProp->version >= 9) {
            fSuccess &= FileRead(hFile, &pProp->has_footsteps, 1, 0);
            if (fSuccess == 0) {
                srAssertFail("fSuccess", LEVELFILE_CPP, 0x93e, 0);
            }
            if (pProp->has_footsteps != 0) {
                fSuccess &= FileRead(hFile, &pProp->footstep_surface, 1, 0);
                fSuccess &= FileRead(hFile, &pProp->footstep_material, 1, 0);
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
        fSuccess &= FileWrite(hFile, &pProp->version, 1, 0);
        fSuccess &= FileWrite(hFile, &pProp->bNumFrames, 1, 0);
        if (pProp->version >= 5) {
            fSuccess &= FileWrite(hFile, &pProp->option, 1, 0);
            fSuccess &= FileWrite(hFile, &pProp->position, sizeof(pProp->position), 0);
        }
        if (pProp->version >= 6) {
            fSuccess &= FileWrite(hFile, &pProp->flags, 4, 0);
        }
        if (pProp->version >= 7) {
            fSuccess &= FileWrite(hFile, pProp->name, sizeof(pProp->name), 0);
        }
        if (fSuccess == 0) {
            srAssertFail("fSuccess", LEVELFILE_CPP, 0x970, 0);
        }
        if (pProp->version >= 8) {
            fSuccess &= FileWrite(hFile, &pProp->num_frame_pos, 1, 0);
            if (pProp->num_frame_pos != 0) {
                if (pProp->usFrame_Pos == 0) {
                    srAssertFail("pProps[i1].usFrame_Pos", LEVELFILE_CPP, 0x976, 0);
                }
                fSuccess &= FileWrite(hFile, pProp->usFrame_Pos, pProp->num_frame_pos << 2, 0);
                free(pProp->usFrame_Pos);
            }
        }
        fSuccess &= WriteAnimObjFile(hFile, &pProp->anim_obj);
        fSuccess &= FileWrite(hFile, &pProp->has_trigger, 1, 0);
        if (fSuccess == 0) {
            srAssertFail("fSuccess", LEVELFILE_CPP, 0x987, 0);
        }
        if (pProp->has_trigger != 0) {
            fSuccess &= WriteTriggerFile(hFile, pProp->pTrigger);
            if (fSuccess == 0) {
                srAssertFail("fSuccess", LEVELFILE_CPP, 0x98b, 0);
            }
            free(pProp->pTrigger);
            pProp->pTrigger = 0;
        }
        if (pProp->version >= 9) {
            fSuccess &= FileWrite(hFile, &pProp->has_footsteps, 1, 0);
            if (fSuccess == 0) {
                srAssertFail("fSuccess", LEVELFILE_CPP, 0x993, 0);
            }
            if (pProp->has_footsteps != 0) {
                fSuccess &= FileWrite(hFile, &pProp->footstep_surface, 1, 0);
                fSuccess &= FileWrite(hFile, &pProp->footstep_material, 1, 0);
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
    if (pSystem->version > 1) {
        fSuccess &= FileRead(hFile, &pSystem->particle.attachment_key, 2, 0);
    } else {
        pSystem->particle.attachment_key = 0;
    }
    if (pSystem->version > 2) {
        fSuccess &= FileRead(hFile, &pSystem->particle.emission_limit, 4, 0);
        fSuccess &= FileRead(hFile, &pSystem->particle.requires_sorted_renderer, 1, 0);
    } else {
        pSystem->particle.emission_limit = 0;
        pSystem->particle.requires_sorted_renderer = 0;
    }
    if (pSystem->version > 3) {
        fSuccess &= FileRead(hFile, &pSystem->particle.start_frame, 4, 0);
        fSuccess &= FileRead(hFile, &pSystem->particle.end_frame, 4, 0);
    } else {
        pSystem->particle.start_frame = 0;
    }
    if (fSuccess == 0) {
        srAssertFail("fSuccess", LEVELFILE_CPP, 0xa03, "Couldn't read particle system.");
    }
    ReportBuildStatus(
        5,
        reinterpret_cast<const char*>( // reinterpret-ok: String returns a logging buffer
            String("Particle System: %s, Position %f, %f, %f\n", pSystem->particle.name,
                   pSystem->particle.location.x * g_world_scale,
                   pSystem->particle.location.y * g_world_scale,
                   pSystem->particle.location.z * g_world_scale)));
    return fSuccess;
}

// FUNCTION: WIZ8 0x004D5370
BOOLEAN WriteParticleSystemFile(int hFile, W8LevelFileParticleSystem* pSystem)
{
    BOOLEAN fSuccess = TRUE;
    fSuccess &= FileWrite(hFile, pSystem, 0x217, 0);
    if (pSystem->version > 1) {
        fSuccess &= FileWrite(hFile, &pSystem->particle.attachment_key, 2, 0);
    }
    if (pSystem->version > 2) {
        fSuccess &= FileWrite(hFile, &pSystem->particle.emission_limit, 4, 0);
        fSuccess &= FileWrite(hFile, &pSystem->particle.requires_sorted_renderer, 1, 0);
    }
    if (pSystem->version > 3) {
        fSuccess &= FileWrite(hFile, &pSystem->particle.start_frame, 4, 0);
        fSuccess &= FileWrite(hFile, &pSystem->particle.end_frame, 4, 0);
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
    fSuccess &= FileRead(hFile, &pBlock->fog_enabled, 1, 0);
    fSuccess &= FileRead(hFile, &pBlock->environment_red, 4, 0);
    fSuccess &= FileRead(hFile, &pBlock->environment_green, 4, 0);
    fSuccess &= FileRead(hFile, &pBlock->environment_blue, 4, 0);
    fSuccess &= FileRead(hFile, &pBlock->intensity, 4, 0);
    fSuccess &= FileRead(hFile, &pBlock->view_distance, 4, 0);
    fSuccess &= FileRead(hFile, &pBlock->camera_mode, 1, 0);
    if (pBlock->camera_mode >= 1) {
        fSuccess &= FileRead(hFile, &pBlock->camera_position, sizeof(pBlock->camera_position), 0);
    }
    if (pBlock->camera_mode >= 2) {
        fSuccess &= FileRead(hFile, &pBlock->camera_angle, 4, 0);
        fSuccess &= FileRead(hFile, &pBlock->camera_axis, sizeof(pBlock->camera_axis), 0);
    }
    fSuccess = fSuccess != 0 && FileRead(hFile, &pBlock->has_light_colours, 1, 0) != 0;
    if (pBlock->has_light_colours != 0) {
        fSuccess &= FileRead(hFile, pBlock->light_colours, 0x300, 0);
    }
    fSuccess = fSuccess != 0 && FileRead(hFile, &pBlock->has_environment_colours, 1, 0) != 0;
    if (pBlock->has_environment_colours != 0) {
        fSuccess &= FileRead(hFile, pBlock->environment_colours, 0x300, 0);
    }
    return fSuccess;
}

// FUNCTION: WIZ8 0x004D5580
BOOLEAN WriteLevelFileBlock(int hFile, W8LevelFileBlock* pBlock)
{
    BOOLEAN fSuccess = TRUE;
    fSuccess &= FileWrite(hFile, &pBlock->fog_enabled, 1, 0);
    fSuccess &= FileWrite(hFile, &pBlock->environment_red, 4, 0);
    fSuccess &= FileWrite(hFile, &pBlock->environment_green, 4, 0);
    fSuccess &= FileWrite(hFile, &pBlock->environment_blue, 4, 0);
    fSuccess &= FileWrite(hFile, &pBlock->intensity, 4, 0);
    fSuccess &= FileWrite(hFile, &pBlock->view_distance, 4, 0);
    fSuccess &= FileWrite(hFile, &pBlock->camera_mode, 1, 0);
    if (pBlock->camera_mode >= 1) {
        fSuccess &= FileWrite(hFile, &pBlock->camera_position, sizeof(pBlock->camera_position), 0);
    }
    if (pBlock->camera_mode >= 2) {
        fSuccess &= FileWrite(hFile, &pBlock->camera_angle, 4, 0);
        fSuccess &= FileWrite(hFile, &pBlock->camera_axis, sizeof(pBlock->camera_axis), 0);
    }
    fSuccess = fSuccess != 0 && FileWrite(hFile, &pBlock->has_light_colours, 1, 0) != 0;
    if (pBlock->has_light_colours != 0) {
        fSuccess &= FileWrite(hFile, pBlock->light_colours, 0x300, 0);
    }
    fSuccess = fSuccess != 0 && FileWrite(hFile, &pBlock->has_environment_colours, 1, 0) != 0;
    if (pBlock->has_environment_colours != 0) {
        fSuccess &= FileWrite(hFile, pBlock->environment_colours, 0x300, 0);
    }
    return fSuccess;
}
