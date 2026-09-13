#ifndef WIZ8_ENGINE_CODE_LEVELFILE_H
#define WIZ8_ENGINE_CODE_LEVELFILE_H

/* Engine Code\LevelFile.cpp. The level-editor serializer: ReadLevelFile loads
   a complete 0x279d-byte level workspace plus all sub-records, WriteLevelFile
   writes it back out while releasing the allocations. All serialized records
   are packed(1); pointers inside the records are heap allocations. */

#include "wiz8/engine_code/materials.h"
#include "wiz8/engine_code/OctMeshModel.h"

#pragma pack(push, 1)

struct W8LevelFilePathAI {
    unsigned char version_00;
    unsigned char scaled_01; /* == 2 -> pScaledPaths */
    unsigned char unknown_02[4];
    unsigned char unknown_06[4];
    int path_count_0a;
    void* pScaledPaths; /* 0x0e: path_count_0a * 0x28 */
    void* pPaths;       /* 0x12: path_count_0a * 0x1c */
};

struct W8LevelFileMesh {
    int version_00;
    int num_vertices_04;
    int num_faces_08;
    unsigned char flags_0c; /* bit0: LOD vertices; bit1: short LOD verts; bit2: compressed faces */
    unsigned char unknown_0d[3];
    unsigned char unknown_10[0xc];  /* version > 1 */
    unsigned char unknown_1c[0x10]; /* version > 1 */
    unsigned char unknown_2c[0xc];  /* version > 1 */
    char field_38;                  /* version > 3 */
    unsigned char unknown_39[3];
    unsigned char unknown_3c[4]; /* version > 3 && field_38 != 0 */
    char lod_mode_40;            /* flags_0c & 1 */
    unsigned char unknown_41;
    short num_lods_42;    /* flags_0c & 1 */
    void** lod_shorts_44; /* flags_0c & 2: num_lods_42 elements of num_vertices_04 * 6 */
    void**
        lods_48; /* flags_0c & 1 && !(flags_0c & 2): num_lods_42 elements of num_vertices_04 * 0xc */
    void*
        pstVertices; /* 0x4c: !(flags_0c & 1): num_vertices_04 * 0x18 allocated, 0x12a..0xc read each */
    void* pstCompFaces;          /* 0x50: flags_0c & 4: num_faces_08 * 0x21 */
    void* pstFaces;              /* 0x54: num_faces_08 * 0x52 allocated, 0x29 read each */
    unsigned char unknown_58[4]; /* flags_0c & 1 && lod_mode_40 > 1 */
};

struct W8LevelFileLight {
    short version_00;
    int flags_02; /* bit 0x200 -> pExtra_3c */
    unsigned char unknown_06[2];
    unsigned char unknown_08[0xc];
    unsigned char unknown_14[0xc];
    unsigned char unknown_20[8];
    unsigned char unknown_28[0x14]; /* version_00 > 1 */
    void* pExtra_3c;                /* 0x3c record, flags_02 & 0x200 */
    W8LevelFilePathAI* pPathAI_40;  /* *pExtra_3c & 0x10 */
};

struct W8LevelFileAnimLight {
    char version_00;
    unsigned char unknown_01[0xc];
    unsigned char unknown_0d[0xc];
    unsigned char unknown_19[4];
    unsigned char unknown_1d[4];
    void* pExtra_21; /* 0x3c record, version_00 > 1 */
};

struct W8LevelFileMonster {
    unsigned char unknown_00[0x1e];
    int num_mon_path_1e;
    void* MonPath_22; /* num_mon_path_1e * 0x1c */
};

struct W8LevelFileCamera {
    unsigned char unknown_00[4];
    unsigned char unknown_04[4];
    char flag_08;
    unsigned char unknown_09[0x14];
    unsigned char unknown_1d[4]; /* flag_08 != 0 */
    W8LevelFilePathAI pathAI_21;
};

struct W8LevelFileDoor { /* 0x99 */
    unsigned char unknown_00[0xa];
    unsigned char unknown_0a[2];
    unsigned char unknown_0c;
    unsigned char unknown_0d[0xc];
    char name_19[0x80];
};

/* The 0x1bb record reachable from invisible and super triggers; appended to
   the +0x2609/+0x260d registry of the level workspace. */
struct W8LevelFileLinkedRecord {
    unsigned char kind_00;
    unsigned char unknown_01[0x1b0];
    unsigned char unknown_1b1[2];
    int value_1b3;
    float value_1b7;
};

struct W8LevelFileSwitch { /* 0x271 */
    unsigned char version_00;
    unsigned char unknown_01[0x1c];
    unsigned char unknown_1d;
    unsigned char unknown_1e;
    char name_1f[0x80];
    char recipients_9f[0x100];
    unsigned char unknown_19f[0x80];
    unsigned char unknown_21f[4]; /* version_00 > 1 */
    char switch_name_223[0x40];   /* version_00 > 1 */
    unsigned char field_263;      /* version_00 > 2 */
    unsigned char door_kind_264;  /* field_263 != 0 */
    W8LevelFileDoor* pDoor_265;   /* door_kind_264 == 1 */
    unsigned char unknown_269[4];
    unsigned char unknown_26d[4]; /* version_00 > 3 */
};

struct W8LevelFilePlane { /* 0x30 */
    unsigned char unknown_00[0x30];
};

struct W8LevelFileInvisible { /* 0x241 */
    unsigned char version_00;
    int field_01;
    unsigned char unknown_05[0xc];
    unsigned char unknown_11[4];
    unsigned char unknown_15[4];
    unsigned char unknown_19;
    unsigned char unknown_1a;
    char name_1b[0x80];
    char recipients_9b[0x100];
    unsigned char unknown_19b;       /* version_00 > 1 */
    W8LevelFilePlane* pPlane_19c;    /* version_00 > 1: 0x30 record */
    unsigned char unknown_1a0[4];    /* version_00 > 2 */
    unsigned char unknown_1a4[0xc];  /* version_00 > 2 */
    unsigned char unknown_1b0;       /* version_00 > 2 */
    unsigned char unknown_1b1[0x80]; /* version_00 > 2 */
    unsigned char unknown_231;       /* version_00 > 3 */
    unsigned char unknown_232[4];    /* version_00 > 3 */
    unsigned char field_236;         /* version_00 > 4 */
    unsigned char field_237;
    unsigned char kind_238;
    unsigned char unknown_239[4];
    W8LevelFileLinkedRecord* pRecord_23d; /* kind == 2 */
};

struct W8LevelFileSound { /* 0x170 */
    unsigned char version_00;
    unsigned char unknown_01[0x20];
    unsigned char unknown_21[0xc];
    unsigned char unknown_2d[0xc];
    unsigned char unknown_39[0xc];
    char name_45[0x80];
    unsigned char unknown_c5;      /* version_00 > 1 */
    unsigned char unknown_c6;      /* version_00 > 1 */
    unsigned char unknown_c7[0xc]; /* version_00 > 2 */
    unsigned char unknown_d3[4];   /* version_00 > 2 */
    unsigned char unknown_d7[0xc]; /* version_00 > 2 */
    unsigned char unknown_e3[0xc]; /* version_00 > 2 */
    char field_ef[0x80];           /* version_00 > 3 */
    unsigned char field_16f;       /* version_00 > 4 */
};

struct W8LevelFileSuperTrigger { /* 0x867 */
    unsigned char version_00;
    char name_01[0x80];
    unsigned char flags_81; /* bit0 suppresses the door-kind half */
    unsigned char field_82;
    unsigned char field_83;
    unsigned char field_84;
    unsigned char field_85;
    unsigned char field_86;
    unsigned char field_87;
    int field_88;
    int field_8c;
    int field_90;
    char recipients_94[0x100];
    unsigned char field_194;
    char field_195[0x100];
    unsigned char field_295;
    int field_296;
    unsigned char field_29a;
    char field_29b[0x80];
    unsigned char unknown_31b[0x180];
    float field_49b[3];      /* version_00 > 1 */
    float field_4a7;         /* version_00 > 1 */
    unsigned char field_4ab; /* version_00 > 1 */
    unsigned char field_4ac; /* version_00 > 1 */
    unsigned char field_4ad; /* version_00 > 1 */
    unsigned char field_4ae; /* version_00 > 1 */
    float field_4af[4];      /* version_00 > 1 */
    unsigned char field_4bf;
    unsigned char field_4c0;
    unsigned char field_4c1;
    char field_4c2[0x100];
    char field_5c2[0x100];
    unsigned char field_6c2;
    int field_6c3;
    int field_6c7;
    char field_6cb[0x100];
    int field_7cb;
    char field_7cf[0x80];        /* version_00 > 2 */
    unsigned char door_kind_84f; /* !(flags_81 & 1) */
    void* pType1_850;            /* door_kind_84f == 1: 0x1c record */
    void* pPlane_854;            /* door_kind_84f == 2: 0x30 record */
    unsigned char field_858;     /* !(flags_81 & 1) */
    void* pRecord_859;           /* field_858 != 0: 0x85 record */
    unsigned char field_85d;
    unsigned char kind_85e;               /* field_85d != 0 */
    void* pDoor_85f;                      /* kind_85e == 1 */
    W8LevelFileLinkedRecord* pRecord_863; /* kind_85e == 2 */
};

struct W8LevelFileTrigger {
    unsigned char version_00;
    unsigned char type_01; /* 1 switch, 2 invisible, 3 sound, 4 super */
    void* pData_02;
};

/* One LOD/morph frame: a flag byte, an embedded mesh record, and a texture
   table. Frame byte +0x0d (mesh.flags_0c) bit0 marks LOD frames. */
struct W8LevelFileFrame {
    unsigned char flags_00;
    W8LevelFileMesh mesh_01; /* 0x5c */
    short num_textures_5d;
    void* pTextures_5f; /* num_textures_5d * 0x12a */
};

struct W8LevelFileLODMesh {
    W8LevelFileFrame* pFrames;
};

struct W8LevelFileMorph { /* 0x6 */
    unsigned char field_00;
    unsigned char num_frames_01;
    W8LevelFileLODMesh LODMesh_02;
};

struct W8LevelFileTransform { /* 0x1c */
    unsigned char field_00;
    unsigned char num_frames_01;
    W8LevelFileLODMesh LODMesh_02;
    W8LevelFilePathAI pathAI_06;
};

/* Serialized AnimObj embedded in a prop record; distinct from the runtime
   W8AnimObj (0x4c, AnimObj.h). */
struct W8LevelFileAnimObj { /* 0x5f */
    char version_00;
    char num_anims_01; /* also morph count */
    char unknown_02;
    char unknown_03;
    char unknown_04;
    char unknown_05;
    char kind_06;    /* 0 -> morphs, else transforms */
    float field_07;  /* version_00 >= 3; default 15.0f */
    char unknown_0b; /* version_00 >= 5 */
    char unknown_0c; /* version_00 >= 6 */
    float field_0d;  /* version_00 >= 6; default 1.0f */
    unsigned char unknown_11[0x32];
    char* abHowMany;                      /* 0x43: num_anims_01 bytes */
    unsigned char num_bound_box_47;       /* version_00 > 6 */
    void* pBoundBox;                      /* 0x48: num_bound_box_47 * 0x18 */
    unsigned char num_anim_lights_4c;     /* version_00 > 7 */
    W8LevelFileAnimLight* pAnimLights_4d; /* num_anim_lights_4c * 0x25 */
    unsigned char has_path_ai_51;         /* version_00 > 8 && kind_06 == 0 */
    W8LevelFilePathAI* pPathAI_52;
    W8LevelFileMorph* pMorphs_56;         /* kind_06 == 0: num_anims_01 * 6 */
    char num_transforms_5a;               /* kind_06 != 0 */
    W8LevelFileTransform* pTransforms_5b; /* kind_06 != 0: num_transforms_5a * 0x1c */
};

struct W8LevelFileProp { /* 0xbf */
    char version_00;
    unsigned char unknown_01;
    unsigned char unknown_02;      /* version_00 > 4 */
    unsigned char unknown_03[0xc]; /* version_00 > 4 */
    unsigned char unknown_0f[4];   /* version_00 > 5 */
    char name_13[0x40];            /* version_00 > 6 */
    W8LevelFileAnimObj anim_obj_53;
    char has_trigger_b2;
    W8LevelFileTrigger* pTrigger; /* 0xb3 */
    char num_frame_pos_b7;        /* version_00 > 7 */
    unsigned int* usFrame_Pos;    /* 0xb8: num_frame_pos_b7 * 4 */
    char flag_bc;                 /* version_00 > 8 */
    unsigned char unknown_bd;
    unsigned char unknown_be;
};

struct W8LevelFileParticleSystem { /* 0x226 */
    char version_00;
    char name_01[0x40];
    float position_41[3];
    unsigned char unknown_04d[0x1ca];
    unsigned char unknown_217[2]; /* version_00 >= 2 */
    unsigned char unknown_219[4]; /* version_00 >= 3 */
    unsigned char unknown_21d;    /* version_00 >= 3 */
    unsigned char unknown_21e[4]; /* version_00 >= 4 */
    unsigned char unknown_222[4]; /* version_00 >= 4 */
};

struct W8LevelFileNamedPosition { /* 0x9d */
    unsigned char unknown_00;
    char name_01[0x80];
    float x_81;
    float y_85;
    float z_89;
    unsigned char unknown_08d[0x10];
};

/* The level workspace ReadLevelFile builds. Assert-proven pointer members;
   unproven spans are kept as unknown byte arrays. */
struct W8LevelFile {
    int field_00;                        /* written 1 */
    int field_04;                        /* written 1 */
    W8LevelFileMesh* pMeshes;            /* 0x08: one 0x5c record */
    OctMeshModel* pModels_0c;            /* 0x0c: written/freed, elements 0x48 */
    short nTextures;                     /* 0x10 */
    W8MaterialRecord004B8A70* pTextures; /* 0x12: nTextures * 0x12a */
    short nLights;                       /* 0x16 */
    W8LevelFileLight* pLights;           /* 0x18: nLights * 0x44 */
    int nMonsters;                       /* 0x1c */
    W8LevelFileMonster* pMonsters;       /* 0x20: nMonsters * 0x26 */
    int nItems;                          /* 0x24 */
    void* pItems;                        /* 0x28: nItems * 0x44 */
    unsigned char unknown_2c[4];         /* 0x2c */
    int nProps;                          /* 0x30 */
    W8LevelFileProp* pProps;             /* 0x34: nProps * 0xbf */
    int nBitmaps;                        /* 0x38 */
    W8LevelFileProp* pBitmaps;           /* 0x3c: nBitmaps * 0xbf */
    int nCameras;                        /* 0x40 */
    W8LevelFileCamera* pCameras;         /* 0x44: nCameras * 0x37 */
    int has_block_48;                    /* 0x48: gates the opaque block */
    unsigned char unknown_04c[0x634];    /* 0x4c: read by Function004D5430 */
    int nTriggers;                       /* 0x680 */
    W8LevelFileTrigger* pTriggers;       /* 0x684: nTriggers * 6 */
    unsigned char unknown_688[4];
    int nClippingPlanes;            /* 0x68c */
    unsigned char unknown_690;      /* read when nClippingPlanes != 0 */
    unsigned char* pClippingPlanes; /* 0x691: nClippingPlanes * 0x50 */
    unsigned char unknown_695[0xc];
    int nParticleSystems;                        /* 0x6a1 */
    W8LevelFileParticleSystem* pParticleSystems; /* 0x6a5: nParticleSystems * 0x226 */
    int nNamedPositions;                         /* 0x6a9 */
    W8LevelFileNamedPosition* pNamedPositions;   /* 0x6ad: nNamedPositions * 0x9d */
    int field_6b1;
    int* pIntTable_6b5; /* field_6b1 * 4 */
    unsigned char unknown_6b9[4];
    int field_6bd; /* FileGetPos result on read */
    int num_switch_triggers_6c1;
    W8LevelFileSwitch* switch_triggers_6c5[1000];
    int num_invisible_planes_1665;
    W8LevelFilePlane* invisible_planes_1669[1000];
    int num_linked_records_2609;
    W8LevelFileLinkedRecord* linked_records_260d[100];
};

#pragma pack(pop)

static_assert(sizeof(W8LevelFilePathAI) == 0x16, "W8LevelFilePathAI_must_be_0x16");
static_assert(sizeof(W8LevelFileMesh) == 0x5c, "W8LevelFileMesh_must_be_0x5c");
static_assert(sizeof(W8LevelFileLight) == 0x44, "W8LevelFileLight_must_be_0x44");
static_assert(sizeof(W8LevelFileAnimLight) == 0x25, "W8LevelFileAnimLight_must_be_0x25");
static_assert(sizeof(W8LevelFileMonster) == 0x26, "W8LevelFileMonster_must_be_0x26");
static_assert(sizeof(W8LevelFileCamera) == 0x37, "W8LevelFileCamera_must_be_0x37");
static_assert(sizeof(W8LevelFileDoor) == 0x99, "W8LevelFileDoor_must_be_0x99");
static_assert(sizeof(W8LevelFileLinkedRecord) == 0x1bb, "W8LevelFileLinkedRecord_must_be_0x1bb");
static_assert(sizeof(W8LevelFileSwitch) == 0x271, "W8LevelFileSwitch_must_be_0x271");
static_assert(sizeof(W8LevelFilePlane) == 0x30, "W8LevelFilePlane_must_be_0x30");
static_assert(sizeof(W8LevelFileInvisible) == 0x241, "W8LevelFileInvisible_must_be_0x241");
static_assert(sizeof(W8LevelFileSound) == 0x170, "W8LevelFileSound_must_be_0x170");
static_assert(sizeof(W8LevelFileSuperTrigger) == 0x867, "W8LevelFileSuperTrigger_must_be_0x867");
static_assert(sizeof(W8LevelFileTrigger) == 6, "W8LevelFileTrigger_must_be_6");
static_assert(sizeof(W8LevelFileFrame) == 0x63, "W8LevelFileFrame_must_be_0x63");
static_assert(sizeof(W8LevelFileMorph) == 6, "W8LevelFileMorph_must_be_6");
static_assert(sizeof(W8LevelFileTransform) == 0x1c, "W8LevelFileTransform_must_be_0x1c");
static_assert(sizeof(W8LevelFileAnimObj) == 0x5f, "W8LevelFileAnimObj_must_be_0x5f");
static_assert(sizeof(W8LevelFileProp) == 0xbf, "W8LevelFileProp_must_be_0xbf");
static_assert(sizeof(W8LevelFileParticleSystem) == 0x226,
              "W8LevelFileParticleSystem_must_be_0x226");
static_assert(sizeof(W8LevelFileNamedPosition) == 0x9d, "W8LevelFileNamedPosition_must_be_0x9d");
static_assert(sizeof(W8LevelFile) == 0x279d, "W8LevelFile_must_be_0x279d");

/* These two read/write the level workspace's opaque +0x4c block. They sit in
   the unresolved gap 0x4D5370..0x4D57A0 immediately past the LevelFile hull;
   declared here for the call sites only - TU ownership is unproven. */
unsigned char Function004D5430(int hFile, unsigned char* block);
unsigned char Function004D5580(int hFile, unsigned char* block);

W8LevelFile* ReadLevelFile004CFDC0(int hFile);
unsigned char WriteLevelFile004D07C0(int hFile, int hFileIn, W8LevelFile* pLevel);
unsigned char ReadMeshFile004D1110(int hFile, W8LevelFileMesh* pMesh);
unsigned char WriteMeshFile004D1510(int hFile, W8LevelFileMesh* pMesh);
unsigned char ReadLightFile004D1820(int hFile, W8LevelFileLight* pLight);
unsigned char WriteLightFile004D1960(int hFile, W8LevelFileLight* pLight);
unsigned char ReadAnimLightFile004D1A90(int hFile, W8LevelFileAnimLight* pLight);
unsigned char WriteAnimLightFile004D1B50(int hFile, W8LevelFileAnimLight* pLight);
unsigned char ReadTriggerFile004D1C10(int hFile, W8LevelFileTrigger* pTrigger);
unsigned char WriteTriggerFile004D23F0(int hFile, W8LevelFileTrigger* pTrigger);
unsigned char ReadSuperTriggerFile004D2A30(int hFile, W8LevelFileTrigger* pTrigger);
unsigned char WriteSuperTriggerFile004D3000(int hFile, W8LevelFileTrigger* pTrigger);
unsigned char ReadDoorTriggerFile004D3540(int hFile, unsigned char* pDoor);
unsigned char WriteDoorTriggerFile004D3660(int hFile, unsigned char* pDoor);
unsigned char ReadPathAIFile004D3770(int hFile, W8LevelFilePathAI* pPathAI);
unsigned char WritePathAIFile004D38E0(int hFile, W8LevelFilePathAI* pPathAI);
unsigned char ReadAnimObjFile004D3A10(int hFile, W8LevelFileAnimObj* pAnimObj,
                                      unsigned char fSuccess = 1);
unsigned char WriteAnimObjFile004D4480(int hFile, W8LevelFileAnimObj* pAnimObj);
W8LevelFileProp* ReadPropsFile004D4CB0(int hFile, int count);
unsigned char WritePropsFile004D4FC0(int hFile, int count, W8LevelFileProp* pProps);
unsigned char ReadParticleSystemFile004D5240(int hFile, W8LevelFileParticleSystem* pSystem);
unsigned char WriteParticleSystemFile004D5370(int hFile, W8LevelFileParticleSystem* pSystem);

#endif
