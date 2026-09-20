#ifndef WIZ8_ENGINE_CODE_LEVELFILE_H
#define WIZ8_ENGINE_CODE_LEVELFILE_H

/* Engine Code\LevelFile.cpp. The level-editor serializer: ReadLevelFile loads
   a complete 0x279d-byte level workspace plus all sub-records, WriteLevelFile
   writes it back out while releasing the allocations. All serialized records
   are packed(1); pointers inside the records are heap allocations. */

#include "wiz8/engine_code/materials.h"
#include "wiz8/engine_code/OctMeshModel.h"

#include <stddef.h>

#pragma pack(push, 1)

struct W8ReadMeshFace;

/* One serialized path keyframe: position, an axis/angle rotation. The scaled
   variant appended a per-frame scale vector. */
struct W8LevelFilePathNode { /* 0x1c */
    srVector3T<float> position_00;
    float angle_0c;
    srVector3T<float> axis_10;
};

struct W8LevelFileScaledPathNode { /* 0x28 */
    W8LevelFilePathNode path;
    srVector3T<float> scale;
};

struct W8LevelFilePathAI {
    unsigned char version_00;
    unsigned char scaled_01; /* == 2 -> pScaledPaths */
    unsigned char unknown_02[4];
    unsigned char unknown_06[4];
    int path_count_0a;
    W8LevelFileScaledPathNode* pScaledPaths; /* 0x0e: path_count_0a records */
    W8LevelFilePathNode* pPaths;             /* 0x12: path_count_0a records */
};

struct W8LevelFileFramePosition {
    unsigned short frame;
    unsigned short unknown_02;
};

struct W8LevelFileCompressedFace { /* 0x21 */
    unsigned short vertex_indices_00[3];
    unsigned char unknown_06[0x1b];
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
    short num_lods_42;     /* flags_0c & 1 */
    short** lod_shorts_44; /* flags_0c & 2: num_lods_42 elements of num_vertices_04 * 3 shorts */
    float**
        lods_48; /* flags_0c & 1 && !(flags_0c & 2): num_lods_42 elements of num_vertices_04 * 0xc */
    float*
        pstVertices; /* 0x4c: !(flags_0c & 1): num_vertices_04 * 0x18 allocated, 0x12a..0xc read each */
    W8LevelFileCompressedFace* pstCompFaces; /* 0x50: flags_0c & 4: num_faces_08 records */
    W8ReadMeshFace* pstFaces;                /* 0x54: 0x52 allocated each, 0x29 read each */
    float lod_scale_58;                      /* flags_0c & 1 && lod_mode_40 > 1 */
};

/* flags_00 bit 0x10 marks the light as owning a path-AI block. */
struct W8LevelFileLightExtra { /* 0x3c */
    unsigned char flags_00;
    unsigned char unknown_01[0x3b];
};

struct W8LevelFileAnimLightExtra { /* 0x3c */
    unsigned char unknown_00[0x3c];
};

struct W8LevelFileMonsterPathEntry { /* 0x1c */
    unsigned char unknown_00[0x1c];
};

struct W8LevelFileItemRecord { /* 0x44 */
    unsigned char unknown_00[0x44];
};

/* Serialized clipping-plane entry. ReadWorldClipPlanes consumes the same
   section as a 64-byte name followed by a four-float position record. */
struct W8LevelFileClippingPlaneRecord { /* 0x50 */
    char name_00[0x40];
    srVector4T<float> serialized_position_40;
};

struct W8LevelFileLight {
    short version_00;
    int flags_02; /* bit 0x200 -> pExtra_3c */
    unsigned char unknown_06[2];
    srVector3T<float> position_08; /* consumed by the vertex-lighting pass */
    srVector3T<float> colour_14;
    float intensity_20;
    float range_24;
    char name_28[0x14];               /* version_00 > 1; sun/moon/lightning classify it */
    W8LevelFileLightExtra* pExtra_3c; /* flags_02 & 0x200 */
    W8LevelFilePathAI* pPathAI_40;    /* pExtra_3c->flags_00 & 0x10 */
};

struct W8LevelFileAnimLight {
    char version_00;
    unsigned char unknown_01[0xc];
    unsigned char unknown_0d[0xc];
    unsigned char unknown_19[4];
    unsigned char unknown_1d[4];
    W8LevelFileAnimLightExtra* pExtra_21; /* version_00 > 1 */
};

struct W8LevelFileMonster {
    unsigned char unknown_00[0x1e];
    int num_mon_path_1e;
    W8LevelFileMonsterPathEntry* MonPath_22; /* num_mon_path_1e records */
};

struct W8LevelFileType1Record { /* 0x1c: door_kind_84f == 1 payload */
    unsigned char unknown_00[0x1c];
};

struct W8LevelFileRecord859 { /* 0x85: field_858 != 0 payload */
    unsigned char unknown_00[0x85];
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

/* Packed discriminator + payload pair used by both switch and super triggers.
   ReadDoorTriggerFile receives this pair at the discriminator address; retail
   stores the owned W8LevelFileDoor* in the following four bytes. */
struct W8LevelFileDoorRef { /* 0x05 */
    unsigned char kind_00;
    W8LevelFileDoor* door_01;
};

/* The 0x1bb record reachable from invisible and super triggers; appended to
   the +0x2609/+0x260d registry of the level workspace. */
struct W8LevelFileLinkedRecord {
    unsigned char kind_00;
    /* 0x1b0-byte payload: the linked region's 36 scaled vertex triples,
       forwarded as a unit to W8GameData's linked-record pass. */
    unsigned char unknown_01[0x1b0];
    /* 0x1b1: the AddTriggerPlane overload at 0x00448C60 compares each of the
       twelve generated surfaces against this byte and links the matching one
       to a new environment record. */
    signed char linked_face_1b1;
    unsigned char unknown_1b2;
    /* 0x1b3: float factor handed to the environment-record builder
       (0x00448E60), which multiplies the surface normal by it. */
    float value_1b3;
    /* 0x1b7: float factor applied to the new environment record's +0x34. */
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
    unsigned char unknown_21f[4];       /* version_00 > 1 */
    char switch_name_223[0x40];         /* version_00 > 1 */
    unsigned char has_door_trigger_263; /* version_00 > 2 */
    W8LevelFileDoorRef door_264;         /* has_door_trigger_263 != 0; kind 1 owns door */
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
    unsigned char field_236;         /* version_00 > 4: serialized gate */
    /* Retail zeroes the record, serializes kind_238, but tests this distinct
       byte for kind 2 in both the reader and writer. */
    unsigned char field_237;
    unsigned char kind_238; /* field_236 != 0: serialized kind */
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
    unsigned char active_82;
    unsigned char kind_83;
    unsigned char when_active_84;
    unsigned char prop_index_85;
    unsigned char activation_count_86;
    unsigned char inactive_count_87;
    int trigger_88;
    int trigger_on_8c;
    int trigger_off_90;
    char recipients_94[0x100];
    unsigned char ataxia_or_cure_194;
    char ps_events_195[0x100];
    unsigned char allow_save_295;
    int price_296;
    unsigned char door_kind_29a;
    char animation_29b[0x80];
    unsigned char unknown_31b[0x180];
    float size_49b[3];      /* version_00 > 1 */
    float direction_4a7;    /* version_00 > 1 */
    unsigned char wait_4ab; /* version_00 > 1 */
    unsigned char wait_4ac; /* version_00 > 1 */
    unsigned char wait_4ad; /* version_00 > 1 */
    unsigned char loop_4ae; /* version_00 > 1 */
    float speed_4af;        /* version_00 > 1 */
    float unknown_4b3[3];   /* version_00 > 1 */
    unsigned char ignore_4bf;
    unsigned char group_4c0;
    unsigned char set_group_4c1;
    char groups_4c2[0x100];
    char objects_5c2[0x100];
    unsigned char close_door_6c2;
    int wait_6c3;
    int field_6c7;
    char event_6cb[0x100];
    int field_7cb;
    char particle_system_7cf[0x80];     /* version_00 > 2 */
    unsigned char door_kind_84f;        /* !(flags_81 & 1) */
    W8LevelFileType1Record* pType1_850; /* door_kind_84f == 1 */
    W8LevelFilePlane* pPlane_854;       /* door_kind_84f == 2: 0x30 record */
    unsigned char field_858;            /* !(flags_81 & 1) */
    W8LevelFileRecord859* pRecord_859;  /* field_858 != 0 */
    unsigned char field_85d;
    W8LevelFileDoorRef door_85e;           /* field_85d != 0; kind 1 owns door */
    W8LevelFileLinkedRecord* pRecord_863;  /* door_85e.kind_00 == 2 */
};

/* Serialized trigger payload; type_01 discriminates the record. */
union W8LevelFileTriggerData {
    W8LevelFileSwitch* switch_trigger;
    W8LevelFileInvisible* invisible;
    W8LevelFileSound* sound;
    W8LevelFileSuperTrigger* super;
    void* raw;
};

struct W8LevelFileTrigger {
    unsigned char version_00;
    unsigned char type_01; /* 1 switch, 2 invisible, 3 sound, 4 super */
    W8LevelFileTriggerData data_02;
};

/* One LOD/morph frame: a flag byte, an embedded mesh record, and a texture
   table. Frame byte +0x0d (mesh.flags_0c) bit0 marks LOD frames. */
struct W8LevelFileFrame {
    unsigned char flags_00;
    W8LevelFileMesh mesh_01; /* 0x5c */
    short num_textures_5d;
    W8MaterialRecord004B8A70* pTextures_5f; /* num_textures_5d * 0x12a */
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

/* One serialized bounds pair: minimum then maximum corner. */
struct W8LevelFileBounds { /* 0x18 */
    srVector3T<float> minimum_00;
    srVector3T<float> maximum_0c;
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
    W8LevelFileBounds* pBoundBox;         /* 0x48: num_bound_box_47 * 0x18 */
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
    unsigned char bNumFrames;      /* 0x01: original name from the
       CreatePathProps assertion text (frame-count upper bound) */
    unsigned char unknown_02;      /* version_00 > 4 */
    srVector3T<float> position_03; /* version_00 > 4: serialized prop position */
    unsigned char unknown_0f[4];   /* version_00 > 5 */
    char name_13[0x40];            /* version_00 > 6 */
    W8LevelFileAnimObj anim_obj_53;
    char has_trigger_b2;
    W8LevelFileTrigger* pTrigger; /* 0xb3 */
    char num_frame_pos_b7;        /* version_00 > 7 */
    /* 0xb8: num_frame_pos_b7 records; CreatePathProps consumes the frame
       index, the trailing short's meaning is unrecovered. */
    W8LevelFileFramePosition* usFrame_Pos;
    char flag_bc; /* version_00 > 8 */
    unsigned char unknown_bd;
    unsigned char unknown_be;
};

/* The 0x225-byte particle body shared by the level file's particle systems
   and the world file's particle records. ReadWorldParticles reads it
   standalone after taking the version byte itself; the level file keeps it
   embedded behind the version byte. Its 0x225-byte extent and every named
   offset come directly from the version-sized reads and subsequent uses in
   0x004BD0D0. */
struct W8LevelParticleRecord004BD0D0 {
    char name[64];                      /* 0x000 */
    srVector3T<float> location;         /* 0x040 */
    float rotation_angle;               /* 0x04c */
    srVector3T<float> rotation_axis;    /* 0x050 */
    unsigned char positional_05c[0x0c]; /* 0x05c */
    unsigned int particle_count;        /* 0x068 */
    float source_06c;                   /* 0x06c */
    float source_070;                   /* 0x070 */
    float source_074;                   /* 0x074 */
    int has_acceleration;               /* 0x078 */
    srVector3T<float> acceleration;     /* 0x07c */
    int positional_088;                 /* 0x088 */
    int bounds_mode;                    /* 0x08c */
    srVector3T<float> bounds_origin;    /* 0x090 */
    float bounds_radius;                /* 0x09c */
    srVector3T<float> bounds_extent;    /* 0x0a0 */
    unsigned int lifetime;              /* 0x0ac */
    int velocity_mode;                  /* 0x0b0 */
    unsigned int emission_interval;     /* 0x0b4 */
    int positional_0b8;                 /* 0x0b8 */
    int placement_mode;                 /* 0x0bc */
    float placement_0c0;                /* 0x0c0 */
    float placement_0c4;                /* 0x0c4 */
    float placement_0c8;                /* 0x0c8 */
    float particle_value;               /* 0x0cc */
    int flutter_mode;                   /* 0x0d0 */
    float flutter_value;                /* 0x0d4 */
    float flutter_period;               /* 0x0d8 */
    int direction_mode;                 /* 0x0dc */
    float direction_0e0;                /* 0x0e0 */
    float direction_0e4;                /* 0x0e4 */
    int initially_active;               /* 0x0e8 */
    W8MaterialRecord004B8A70 material;  /* 0x0ec */
    short value_216;                    /* 0x216, version >= 2 */
    int state_218;                      /* 0x218, version >= 3 */
    unsigned char value_21c;            /* 0x21c, version >= 3 */
    int start_frame_21d;                /* 0x21d, unaligned, version >= 4 */
    int end_frame_221;                  /* 0x221, version >= 4 */
};

struct W8LevelFileParticleSystem { /* 0x226 */
    char version_00;
    W8LevelParticleRecord004BD0D0 particle_01;
};

/* Serialized form of W8NamedPosition: one version byte precedes the runtime
   record's name, position and trailing scalar fields. */
struct W8LevelFileNamedPosition { /* 0x9d */
    unsigned char version_00;
    char name_01[0x80];
    srVector3T<float> position_81;
    int value_08d;
    float value_091;
    float value_095;
    float value_099;
};

/* The serialized block gated by has_block_48, between the camera table and
   the trigger table. Layout proven by the 0x004D5430 reader / 0x004D5580
   writer pair; field semantics are unproven - no runtime consumer reads it. */
struct W8LevelFileBlock { /* 0x634 */
    unsigned char flag_00;
    int field_01;
    int field_05;
    int field_09;
    int field_0d;
    int field_11;
    unsigned char count_15; /* != 0 -> record_16; > 1 -> field_22/record_26 */
    unsigned char record_16[0xc];
    int field_22;
    unsigned char record_26[0xc];
    unsigned char flag_32; /* != 0 -> buffer_33 serialized */
    unsigned char buffer_33[0x300];
    unsigned char flag_333; /* != 0 -> buffer_334 serialized */
    unsigned char buffer_334[0x300];
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
    W8LevelFileItemRecord* pItems;       /* 0x28: nItems records */
    unsigned char unknown_2c[4];         /* 0x2c */
    int nProps;                          /* 0x30 */
    W8LevelFileProp* pProps;             /* 0x34: nProps * 0xbf */
    int nBitmaps;                        /* 0x38 */
    W8LevelFileProp* pBitmaps;           /* 0x3c: nBitmaps * 0xbf */
    int nCameras;                        /* 0x40 */
    W8LevelFileCamera* pCameras;         /* 0x44: nCameras * 0x37 */
    int has_block_48;                    /* 0x48: gates block_04c */
    W8LevelFileBlock block_04c;          /* 0x4c */
    int nTriggers;                       /* 0x680 */
    W8LevelFileTrigger* pTriggers;       /* 0x684: nTriggers * 6 */
    unsigned char unknown_688[4];
    int nClippingPlanes;                                   /* 0x68c */
    unsigned char clipping_plane_version_690;              /* nClippingPlanes != 0 */
    W8LevelFileClippingPlaneRecord* pClippingPlanes;       /* 0x691: nClippingPlanes records */
    unsigned char unknown_695[0xc];
    int nParticleSystems;                        /* 0x6a1 */
    W8LevelFileParticleSystem* pParticleSystems; /* 0x6a5: nParticleSystems * 0x226 */
    int nNamedPositions;                         /* 0x6a9 */
    W8LevelFileNamedPosition* pNamedPositions;   /* 0x6ad: nNamedPositions * 0x9d */
    /* PrePathing::CreateAutomapNodes fills these with the sorted automap
       cell keys; LevelFile.cpp writes them after the named positions. */
    int num_automap_nodes_6b1;
    unsigned long* automap_nodes_6b5; /* num_automap_nodes_6b1 * 4 */
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

static_assert(sizeof(W8LevelFileCompressedFace) == 0x21, "W8LevelFileCompressedFace_must_be_0x21");
static_assert(sizeof(W8LevelFilePathNode) == 0x1c, "W8LevelFilePathNode_must_be_0x1c");
static_assert(sizeof(W8LevelFileScaledPathNode) == 0x28, "W8LevelFileScaledPathNode_must_be_0x28");
static_assert(sizeof(W8LevelFileBounds) == 0x18, "W8LevelFileBounds_must_be_0x18");
static_assert(sizeof(W8LevelFilePathAI) == 0x16, "W8LevelFilePathAI_must_be_0x16");
static_assert(sizeof(W8LevelFileMesh) == 0x5c, "W8LevelFileMesh_must_be_0x5c");
static_assert(sizeof(W8LevelFileLight) == 0x44, "W8LevelFileLight_must_be_0x44");
static_assert(sizeof(W8LevelFileAnimLight) == 0x25, "W8LevelFileAnimLight_must_be_0x25");
static_assert(sizeof(W8LevelFileMonster) == 0x26, "W8LevelFileMonster_must_be_0x26");
static_assert(sizeof(W8LevelFileCamera) == 0x37, "W8LevelFileCamera_must_be_0x37");
static_assert(sizeof(W8LevelFileDoor) == 0x99, "W8LevelFileDoor_must_be_0x99");
static_assert(sizeof(W8LevelFileDoorRef) == 5, "W8LevelFileDoorRef_must_be_5");
static_assert(sizeof(W8LevelFileLinkedRecord) == 0x1bb, "W8LevelFileLinkedRecord_must_be_0x1bb");
static_assert(sizeof(W8LevelFileSwitch) == 0x271, "W8LevelFileSwitch_must_be_0x271");
static_assert(sizeof(W8LevelFilePlane) == 0x30, "W8LevelFilePlane_must_be_0x30");
static_assert(sizeof(W8LevelFileInvisible) == 0x241, "W8LevelFileInvisible_must_be_0x241");
static_assert(sizeof(W8LevelFileSound) == 0x170, "W8LevelFileSound_must_be_0x170");
static_assert(sizeof(W8LevelFileSuperTrigger) == 0x867, "W8LevelFileSuperTrigger_must_be_0x867");
static_assert(offsetof(W8LevelFileSwitch, door_264) == 0x264, "W8LevelFileSwitch_door_264");
static_assert(offsetof(W8LevelFileSuperTrigger, door_85e) == 0x85e,
              "W8LevelFileSuperTrigger_door_85e");
static_assert(sizeof(W8LevelFileLightExtra) == 0x3c, "W8LevelFileLightExtra_must_be_0x3c");
static_assert(sizeof(W8LevelFileAnimLightExtra) == 0x3c, "W8LevelFileAnimLightExtra_must_be_0x3c");
static_assert(sizeof(W8LevelFileMonsterPathEntry) == 0x1c,
              "W8LevelFileMonsterPathEntry_must_be_0x1c");
static_assert(sizeof(W8LevelFileItemRecord) == 0x44, "W8LevelFileItemRecord_must_be_0x44");
static_assert(sizeof(W8LevelFileClippingPlaneRecord) == 0x50,
              "W8LevelFileClippingPlaneRecord_must_be_0x50");
static_assert(sizeof(W8LevelFileFramePosition) == 4, "W8LevelFileFramePosition_must_be_4");
static_assert(sizeof(W8LevelFileType1Record) == 0x1c, "W8LevelFileType1Record_must_be_0x1c");
static_assert(sizeof(W8LevelFileRecord859) == 0x85, "W8LevelFileRecord859_must_be_0x85");
static_assert(sizeof(W8LevelFileTrigger) == 6, "W8LevelFileTrigger_must_be_6");
static_assert(sizeof(W8LevelFileFrame) == 0x63, "W8LevelFileFrame_must_be_0x63");
static_assert(sizeof(W8LevelFileMorph) == 6, "W8LevelFileMorph_must_be_6");
static_assert(sizeof(W8LevelFileTransform) == 0x1c, "W8LevelFileTransform_must_be_0x1c");
static_assert(sizeof(W8LevelFileAnimObj) == 0x5f, "W8LevelFileAnimObj_must_be_0x5f");
static_assert(sizeof(W8LevelFileProp) == 0xbf, "W8LevelFileProp_must_be_0xbf");
static_assert(offsetof(W8LevelFileProp, version_00) == 0x00, "W8LevelFileProp_version_00");
static_assert(offsetof(W8LevelFileProp, bNumFrames) == 0x01, "W8LevelFileProp_bNumFrames");
static_assert(offsetof(W8LevelFileProp, unknown_02) == 0x02, "W8LevelFileProp_unknown_02");
static_assert(offsetof(W8LevelFileProp, position_03) == 0x03, "W8LevelFileProp_position_03");
static_assert(offsetof(W8LevelFileProp, unknown_0f) == 0x0f, "W8LevelFileProp_unknown_0f");
static_assert(offsetof(W8LevelFileProp, name_13) == 0x13, "W8LevelFileProp_name_13");
static_assert(offsetof(W8LevelFileProp, anim_obj_53) == 0x53, "W8LevelFileProp_anim_obj_53");
static_assert(offsetof(W8LevelFileProp, has_trigger_b2) == 0xb2, "W8LevelFileProp_has_trigger_b2");
static_assert(offsetof(W8LevelFileProp, pTrigger) == 0xb3, "W8LevelFileProp_pTrigger");
static_assert(offsetof(W8LevelFileProp, num_frame_pos_b7) == 0xb7,
              "W8LevelFileProp_num_frame_pos_b7");
static_assert(offsetof(W8LevelFileProp, usFrame_Pos) == 0xb8, "W8LevelFileProp_usFrame_Pos");
static_assert(offsetof(W8LevelFileProp, flag_bc) == 0xbc, "W8LevelFileProp_flag_bc");
static_assert(offsetof(W8LevelFileProp, unknown_bd) == 0xbd, "W8LevelFileProp_unknown_bd");
static_assert(offsetof(W8LevelFileProp, unknown_be) == 0xbe, "W8LevelFileProp_unknown_be");
static_assert(sizeof(W8LevelParticleRecord004BD0D0) == 0x225,
              "W8LevelParticleRecord004BD0D0_must_be_0x225");
static_assert(sizeof(W8LevelFileParticleSystem) == 0x226,
              "W8LevelFileParticleSystem_must_be_0x226");
static_assert(offsetof(W8LevelFileParticleSystem, particle_01.location) == 0x41,
              "W8LevelFileParticleSystem_position_41");
static_assert(offsetof(W8LevelFileParticleSystem, particle_01.value_216) == 0x217,
              "W8LevelFileParticleSystem_value_217");
static_assert(sizeof(W8LevelFileNamedPosition) == 0x9d, "W8LevelFileNamedPosition_must_be_0x9d");
static_assert(offsetof(W8LevelFileNamedPosition, position_81) == 0x81,
              "W8LevelFileNamedPosition_position_81");
static_assert(sizeof(W8LevelFileBlock) == 0x634, "W8LevelFileBlock_must_be_0x634");
static_assert(sizeof(W8LevelFile) == 0x279d, "W8LevelFile_must_be_0x279d");
static_assert(offsetof(W8LevelFile, pClippingPlanes) == 0x691,
              "W8LevelFile_pClippingPlanes");

W8LevelFile* ReadLevelFile004CFDC0(int hFile);
bool WriteLevelFile004D07C0(int hFile, int hFileIn, W8LevelFile* pLevel);
bool ReadMeshFile004D1110(int hFile, W8LevelFileMesh* pMesh);
bool WriteMeshFile004D1510(int hFile, W8LevelFileMesh* pMesh);
bool ReadLightFile004D1820(int hFile, W8LevelFileLight* pLight);
bool WriteLightFile004D1960(int hFile, W8LevelFileLight* pLight);
bool ReadAnimLightFile004D1A90(int hFile, W8LevelFileAnimLight* pLight);
bool WriteAnimLightFile004D1B50(int hFile, W8LevelFileAnimLight* pLight);
bool ReadTriggerFile004D1C10(int hFile, W8LevelFileTrigger* pTrigger);
bool WriteTriggerFile004D23F0(int hFile, W8LevelFileTrigger* pTrigger);
bool ReadSuperTriggerFile004D2A30(int hFile, W8LevelFileTrigger* pTrigger);
bool WriteSuperTriggerFile004D3000(int hFile, W8LevelFileTrigger* pTrigger);
bool ReadDoorTriggerFile004D3540(int hFile, W8LevelFileDoorRef* pDoor);
bool WriteDoorTriggerFile004D3660(int hFile, W8LevelFileDoorRef* pDoor);
bool ReadPathAIFile004D3770(int hFile, W8LevelFilePathAI* pPathAI);
bool WritePathAIFile004D38E0(int hFile, W8LevelFilePathAI* pPathAI);
bool ReadAnimObjFile004D3A10(int hFile, W8LevelFileAnimObj* pAnimObj, unsigned char fSuccess = 1);
bool WriteAnimObjFile004D4480(int hFile, W8LevelFileAnimObj* pAnimObj);
W8LevelFileProp* ReadPropsFile004D4CB0(int hFile, int count);
bool WritePropsFile004D4FC0(int hFile, int count, W8LevelFileProp* pProps);
bool ReadParticleSystemFile004D5240(int hFile, W8LevelFileParticleSystem* pSystem);
bool WriteParticleSystemFile004D5370(int hFile, W8LevelFileParticleSystem* pSystem);
bool ReadLevelFileBlock004D5430(int hFile, W8LevelFileBlock* pBlock);
bool WriteLevelFileBlock004D5580(int hFile, W8LevelFileBlock* pBlock);

#endif
