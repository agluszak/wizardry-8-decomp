#ifndef WIZ8_ENGINE_CODE_LEVELFILE_H
#define WIZ8_ENGINE_CODE_LEVELFILE_H

/* Engine Code\LevelFile.cpp. The level-editor serializer: ReadLevelFile loads
   a complete 0x279d-byte level workspace plus all sub-records, WriteLevelFile
   writes it back out while releasing the allocations. All serialized records
   are packed(1); pointers inside the records are heap allocations. */

#include "Types.h"
#include "wiz8/engine_code/materials.h"
#include "wiz8/engine_code/OctMeshModel.h"

#include <stddef.h>

#pragma pack(push, 1)

struct W8ReadMeshFace;

/* One serialized path keyframe: position, an axis/angle rotation. The scaled
   variant appended a per-frame scale vector. */
struct W8LevelFilePathNode { /* 0x1c */
    srVector3T<float> position_00;
    float angle;
    srVector3T<float> axis;
};

struct W8LevelFileScaledPathNode { /* 0x28 */
    W8LevelFilePathNode path;
    srVector3T<float> scale;
};

struct W8LevelFilePathAI {
    unsigned char version_00;
    unsigned char scaled; /* == 2 -> pScaledPaths */
    int position;
    unsigned char unknown_06[4];
    int path_count;
    W8LevelFileScaledPathNode* pScaledPaths; /* 0x0e: path_count records */
    W8LevelFilePathNode* pPaths;             /* 0x12: path_count records */
};

struct W8LevelFileFramePosition {
    unsigned short frame;
    unsigned short tag;
};

/* Compressed mesh face; ReadMesh expands these into W8ReadMeshFace. */
struct W8LevelFileCompressedFace { /* 0x21 */
    unsigned short vertex_indices[3];
    srVector2T<float> texture_coordinates[3];
    unsigned short material_index;
    unsigned char flags;
};

struct W8LevelFileMesh {
    int version_00;
    int num_vertices;
    int num_faces;
    unsigned char flags; /* bit0: LOD vertices; bit1: short LOD verts; bit2: compressed faces */
    unsigned char padding_0d[3];     /* never serialized */
    srVector3T<float> location;      /* version > 1 */
    float rotation_angle;            /* version > 1 */
    srVector3T<float> rotation_axis; /* version > 1 */
    srVector3T<float> scale;         /* version > 1 */
    char mapping_count;              /* version > 3 */
    unsigned char padding_39[3];     /* never serialized */
    short mapped_value_3c;           /* version > 3 && mapping_count != 0 */
    short mapped_key;                /* version > 3 && mapping_count != 0 */
    char lod_mode;                   /* flags & 1 */
    unsigned char padding_41;        /* never serialized */
    short num_lods;                  /* flags & 1 */
    short** lod_shorts;              /* flags & 2: num_lods elements of num_vertices * 3 shorts */
    srVector3T<float>** lods;        /* float-position arrays, one per LOD */
    /* The reader reserves twice the serialized position count. */
    srVector3T<float>* pstVertices;          /* 0x4c: !(flags & 1) */
    W8LevelFileCompressedFace* pstCompFaces; /* 0x50: flags & 4: num_faces records */
    W8ReadMeshFace* pstFaces;                /* 0x54: 0x52 allocated each, 0x29 read each */
    float lod_scale;                         /* flags & 1 && lod_mode > 1 */
};

static_assert(sizeof(srVector3T<float>) == 0xc, "Level mesh position record size");
static_assert(offsetof(W8LevelFileMesh, pstVertices) == 0x4c, "Level mesh position pointer offset");

/* The 0x3c-byte serialized block covering stParametricLightDefinition fields
   flags_08 through subcycle_max_40: the runtime object's first 8 bytes
   (vtable/type) are not serialized. flags_00 bit 0x10 marks the light as
   owning a path-AI block. Serialized under a light's flags_04 bit1, and
   after every version_00 > 1 anim-light record. */
struct W8LevelFileLightExtra { /* 0x3c */
    unsigned int flags_00;
    float flicker_chance;
    srVector3T<float> color;
    srVector3T<float> color_to;
    float intensity;
    float intensity_to;
    float period;
    float rate;
    float path_speed;
    int subcycle_min;
    int subcycle_max;
};

/* Serialized world-item record: the .pvl item section
   (ReadWorldItems) stores the same fields plus an optional inline
   trigger behind has_trigger, which the .lvl keeps in its own table. */
struct W8LevelFileItemRecord {     /* 0x44 */
    char item_name[0x14];          /* item script name */
    srVector3T<float> position_14; /* scaled by world_scale on load */
    int positional_20;
    int positional_24;
    int positional_28;
    int positional_2c;
    unsigned char has_trigger; /* .pvl gates an inline trigger */
    unsigned char positional_31[3];
    int positional_34;
    int positional_38;
    int positional_3c;
    int positional_40;
};

/* Serialized clipping-plane entry. ReadWorldClipPlanes consumes the same
   section as a 64-byte name followed by a four-float position record. */
struct W8LevelFileClippingPlaneRecord { /* 0x50 */
    char name_00[0x40];
    srVector4T<float> serialized_position;
};

/* Serialized form of the .pvl W8LevelLightRecord (ReadLevel.cpp): the dword
   at +0x02 packs create, visible and the low flag bits. */
struct W8LevelFileLight {
    short version_00;
    unsigned char create;    /* != 0 -> created light path over static */
    unsigned char visible;   /* bake requires != 0 to emit the light */
    unsigned short flags_04; /* bit 0x2 -> pExtra serialized */
    unsigned char unknown_06[2];
    srVector3T<float> position; /* consumed by the vertex-lighting pass */
    srVector3T<float> colour;
    float intensity;
    float range;
    char name_28[0x14];            /* version_00 > 1; sun/moon/lightning classify it */
    W8LevelFileLightExtra* pExtra; /* flags_04 & 0x2 */
    W8LevelFilePathAI* pPathAI;    /* pExtra->flags_00 & 0x10 */
};

struct W8LevelFileAnimLight {
    char version_00;
    srVector3T<float> position;
    srVector3T<float> color;
    float intensity;
    float range;
    W8LevelFileLightExtra* pExtra; /* version_00 > 1: light definition */
};

/* Serialized monster: the 0x1e-byte head is the 0x14-byte name plus the
   inlined W8LevelFilePathAI header (version/scaled/position/unknown tail);
   MonPath is the path node array, always unscaled 0x1c-byte nodes. */
struct W8LevelFileMonster {
    char monster_name[0x14];
    unsigned char path_version;
    unsigned char path_scaled;
    int path_position;
    unsigned char unknown_1a[4];
    int num_mon_path;
    W8LevelFilePathNode* MonPath; /* num_mon_path records */
};

struct W8LevelFileTriggerPosition { /* 0x1c: placement_kind == 1 payload */
    srVector3T<float> position;
    float angle;
    srVector3T<float> direction;
};

struct W8LevelFileTriggerHotSpot { /* 0x85: has_hotspot != 0 payload */
    unsigned char unknown_00[0x85];
};

/* Serialized camera waypoint: the .pvl camera section
   (ReadWorldCameras) reads the same head before its PathAI; the
   0x14-byte span is the W8CameraPath name; the two leading ints stay
   unresolved. */
struct W8LevelFileCamera {
    int positional_00;
    int positional_04;
    char has_scale;
    char name[0x14];
    float scale; /* has_scale != 0; .pvl defaults to 15.0f */
    W8LevelFilePathAI pathAI;
};

/* Door trigger action data: the .pvl loader (ReadDoorTriggerActionData)
   reads the same byte stream: version, nine flag bytes, the item index, a
   position gate, an optional position and the linked trigger name. */
struct W8LevelFileDoor { /* 0x99 */
    unsigned char version_00;
    unsigned char flags[9];
    unsigned short item;
    unsigned char has_position;
    srVector3T<float> position; /* has_position != 0 */
    char linked_trigger[0x80];
};

/* Packed discriminator + payload pair used by both switch and super triggers.
   ReadDoorTriggerFile receives this pair at the discriminator address; retail
   stores the owned W8LevelFileDoor* in the following four bytes. */
struct W8LevelFileDoorRef { /* 0x05 */
    unsigned char kind_00;
    W8LevelFileDoor* door;
};

/* The 0x1bb record reachable from invisible and super triggers; appended to
   the +0x2609/+0x260d registry of the level workspace. */
struct W8LevelFileLinkedRecord {
    unsigned char kind_00;
    /* 0x1b0-byte payload: the linked region's 36 scaled vertex triples,
       forwarded as a unit to W8GameData's linked-record pass. */
    srVector3T<float> vertices[36];
    /* 0x1b1: the AddTriggerPlane overload at 0x00448C60 compares each of the
       twelve generated surfaces against this byte and links the matching one
       to a new environment record. */
    signed char linked_face;
    /* 0x1b2: second serialized flag byte (.pvl legacy_flags[1]); no
       consumer reads it. */
    unsigned char flag_1b2;
    /* 0x1b3: float factor handed to the environment-record builder
       (0x00448E60), which multiplies the surface normal by it. */
    float normal_scale;
    /* 0x1b7: float factor applied to the new environment record's
       forward_scale_34. */
    float forward_scale;
};

/* The 0x271-byte type-1 trigger record. The .pvl type-1 stream
   (Trigger.cpp case 1) reads the same fields into a W8Trigger, which names
   the serialized semantics. */
struct W8LevelFileSwitch { /* 0x271 */
    char version_00;
    int cycle_bounce;           /* -> Trigger::cycle_bounce */
    int state_count;            /* -> Trigger::state_count */
    float animate_states;       /* != 0 -> W8_TRIGGER_ANIMATE_STATES */
    int range;                  /* -> Trigger::range_maximum_0a8 (*500) */
    int action;                 /* -> Trigger::initial_action_22a */
    int value_15;               /* serialized; no reader consumer */
    int animate_action;         /* != 0 -> W8_TRIGGER_ANIMATE_ACTION */
    unsigned char packed_flags; /* bit0 FIRE_LINKED, bit1 LINK_ON_DEACTIVATE */
    unsigned char enabled;      /* != 0 -> W8_TRIGGER_ENABLED */
    char name[0x80];
    char recipients[0x100];
    char sound[0x80];               /* wave filename -> "data\sound\%s" */
    float minimum_range;            /* version_00 > 1 -> range_minimum_0a4 (*500) */
    char surface_id[0x40];          /* version_00 > 1: surface name/id string */
    unsigned char has_door_trigger; /* version_00 > 2 */
    W8LevelFileDoorRef door;        /* has_door_trigger != 0; kind 1 owns door */
    unsigned char padding_269[4];   /* never serialized */
    int action_value_26d;           /* version_00 > 3 -> Trigger::action_value */
};

struct W8LevelFilePlane { /* 0x30 */
    srVector3T<float> vertices[4];
};

/* The 0x241-byte type-2 (invisible) trigger record. The .pvl type-2 stream
   (Trigger.cpp case 2) reads the same fields into a W8Trigger. */
struct W8LevelFileInvisible { /* 0x241 */
    char version_00;
    float range;                /* -> range_maximum_0a8 (*500) */
    srVector3T<float> position; /* -> position_118 (*500) */
    int action;                 /* -> initial_action_22a */
    int searchable;             /* -> Trigger::searchable */
    unsigned char fire_linked;  /* != 0 -> W8_TRIGGER_FIRE_LINKED */
    unsigned char enabled;      /* != 0 -> W8_TRIGGER_ENABLED */
    char name[0x80];
    char recipients[0x100];
    unsigned char plane_flag_19b;      /* version_00 > 1: ==1 registers the
                                            vectors as a trigger plane */
    W8LevelFilePlane* pPlane;          /* version_00 > 1: 0x30 record ->
                                            representation_vectors_0cc (*500) */
    float angle;                       /* version_00 > 2 -> angle_0fc */
    srVector3T<float> direction;       /* version_00 > 2 -> direction_100 */
    unsigned char unused_1b0;          /* version_00 > 2: serialized, unread */
    char action_string[0x80];          /* version_00 > 2: action-17 payload */
    unsigned char flag_231;            /* version_00 > 3 -> flags_0a0 bit3 */
    int action_value_232;              /* version_00 > 3 -> action_value */
    unsigned char has_legacy_geometry; /* version_00 > 4: serialized gate */
    /* Retail zeroes the record, serializes geometry_kind, but tests this
       distinct byte for kind 2 in both the reader and writer. */
    /* Retail tests this distinct byte; the serialized kind is at +0x238. */
    unsigned char linked_record_kind_gate;
    unsigned char geometry_kind;      /* has_legacy_geometry != 0 */
    unsigned char padding_239[4];     /* never serialized */
    W8LevelFileLinkedRecord* pRecord; /* kind == 2 */
};

/* The 0x170-byte type-3 (ambient sound) trigger record. The .pvl type-3
   stream (Trigger.cpp case 3) reads the same fields and hands them to
   AddAmbientSound. */
struct W8LevelFileSound { /* 0x170 */
    char version_00;
    int volume_min;
    int volume_max;
    int speed_min;
    int speed_max;
    int time_min;
    int time_max;
    int unbounded; /* == 0 gates the bounded radius */
    float radius;
    srVector3T<float> position;
    srVector3T<float> region_u;
    srVector3T<float> region_v;
    char wave[0x80];                 /* wave filename -> "data\sound\%s" */
    unsigned char has_position;      /* version_00 > 1 */
    unsigned char looping;           /* version_00 > 1 */
    srVector3T<float> region_center; /* version_00 > 2 */
    float region_angle;              /* version_00 > 2 */
    srVector3T<float> region_min;    /* version_00 > 2 */
    srVector3T<float> region_max;    /* version_00 > 2 */
    char name[0x80];                 /* version_00 > 3 */
    unsigned char shared;            /* version_00 > 4 */
};

struct W8LevelFileSuperTrigger { /* 0x867 */
    char version_00;
    char name[0x80];
    unsigned char flags; /* bit0 skips the placement and hotspot records */
    unsigned char active;
    unsigned char kind;
    unsigned char when_active;
    unsigned char prop_index;
    unsigned char activation_count;
    unsigned char inactive_count;
    int trigger;
    int trigger_on;
    int trigger_off;
    char recipients[0x100];
    unsigned char ataxia_or_cure;
    char ps_events[0x100];
    unsigned char allow_save;
    int price;
    unsigned char door_kind;
    char animation[0x80];
    unsigned char padding_31b[0x180]; /* never serialized; the reader and
                                         writer jump straight to the
                                         version_00 > 1 block */
    float size[3];                    /* version_00 > 1 */
    float direction;                  /* version_00 > 1 */
    unsigned char wait_4ab;           /* version_00 > 1 */
    unsigned char wait_4ac;           /* version_00 > 1 */
    unsigned char wait_4ad;           /* version_00 > 1 */
    unsigned char loop;               /* version_00 > 1 */
    float speed;                      /* version_00 > 1 */
    float unknown_4b3[3];             /* version_00 > 1 */
    unsigned char ignore;
    unsigned char group;
    unsigned char set_group;
    char groups[0x100];
    char objects[0x100];
    unsigned char close_door;
    int wait_6c3;
    int field_6c7;
    char event[0x100];
    float normal_scale;                    /* copied to the linked record's normal_scale */
    char particle_system[0x80];            /* version_00 > 2 */
    unsigned char placement_kind;          /* !(flags & 1) */
    W8LevelFileTriggerPosition* pPosition; /* placement_kind == 1 */
    W8LevelFilePlane* pPlane;              /* placement_kind == 2 */
    unsigned char has_hotspot;             /* !(flags & 1) */
    W8LevelFileTriggerHotSpot* pHotSpot;   /* has_hotspot != 0 */
    unsigned char has_door;
    W8LevelFileDoorRef door;          /* has_door != 0; kind 1 owns door */
    W8LevelFileLinkedRecord* pRecord; /* door.kind_00 == 2 */
};

struct W8LevelFileTrigger {
    unsigned char version_00;
    char type;   /* 1 switch, 2 invisible, 3 sound, 4 super */
    void* pData; /* type selects the pointed-to record */
};

/* One LOD/morph frame: a flag byte, an embedded mesh record, and a texture
   table. Frame byte +0x0d (mesh.flags) bit0 marks LOD frames. */
struct W8LevelFileFrame {
    unsigned char flags_00;
    W8LevelFileMesh mesh; /* 0x5c */
    short num_textures;
    W8MaterialRecord* pTextures; /* num_textures * 0x12a */
};

struct W8LevelFileLODMesh {
    W8LevelFileFrame* pFrames;
};

/* One element of W8LevelFileAnimObj::pMorphs, of which there are
   num_anims. Not to be confused with the AnimObj record itself, which is
   0x5f bytes and holds these at +0x56. */
struct W8LevelFileMorph { /* 0x6 */
    unsigned char channel;
    unsigned char num_frames;
    W8LevelFileLODMesh LODMesh;
};

struct W8LevelFileTransform { /* 0x1c */
    unsigned char channel;
    unsigned char num_frames;
    W8LevelFileLODMesh LODMesh;
    W8LevelFilePathAI pathAI;
};

/* One serialized bounds pair: minimum then maximum corner. */
struct W8LevelFileBounds { /* 0x18 */
    srVector3T<float> minimum;
    srVector3T<float> maximum;
};

/* Serialized AnimObj embedded in a prop record; distinct from the runtime
   W8AnimObj (0x4c, AnimObj.h). */
struct W8LevelFileAnimObj { /* 0x5f */
    char version_00;
    /* The four byte counts below are not interchangeable. The retail reads
       each with a one-byte FileRead and then sign-extends two of them:
       num_anims with movsx at 0x004D3B5B and 0x004D3B88, num_anim_lights
       with movsx at 0x004D3C9E, and num_transforms with movsx at
       0x004D40CE, but num_bound_box is masked with and eax,0xff at
       0x004D3BCD and so is unsigned. */
    char num_anims; /* animation group count; also morph count */
    char animation_playing;
    char frame_method;
    char behaviour;
    char cycle;
    char path_lists; /* 0 -> morphs, else transforms */
    /* version_00 >= 3; default 15.0f */
    float playback_scale;
    char start_frame; /* version_00 >= 5 */
    /* version_00 >= 6 */
    char random_play_0c;
    float play_chance;                /* default 1.0f */
    unsigned char discarded_11[0x32]; /* serialized; never read back */
    char* abHowMany;                  /* 0x43: num_anims channel bytes */
    unsigned char num_bound_box;      /* version_00 > 6 */
    W8LevelFileBounds* pBoundBox;     /* 0x48: num_bound_box * 0x18 */
    /* version_00 > 7. ReadAnimObjFile sign-extends this count with
       movsx at 0x004D3C9E, 0x004D3CC4 and 0x004D3D0B, and tests it signed
       with jle at 0x004D3CE3; WriteAnimObjFile does the same with jle at
       0x004D46B3 and movsx at 0x004D46CF. By contrast num_bound_box is
       masked with and eax,0xff at 0x004D3BCD and num_transforms is a
       sign-extended plain char, so the three are not interchangeable. */
    signed char num_anim_lights;
    W8LevelFileAnimLight* pAnimLights; /* num_anim_lights * 0x25 */
    unsigned char has_path_ai;         /* version_00 > 8 && path_lists == 0 */
    W8LevelFilePathAI* pPathAI;
    W8LevelFileMorph* pMorphs;         /* path_lists == 0: num_anims * 6 */
    char num_transforms;               /* path_lists != 0 */
    W8LevelFileTransform* pTransforms; /* path_lists != 0: num_transforms * 0x1c */
};

struct W8LevelFileProp { /* 0xbf */
    char version_00;
    unsigned char bNumFrames;   /* 0x01: original name from the
       CreatePathProps assertion text (frame-count upper bound) */
    unsigned char option_02;    /* version_00 > 4: alignment option */
    srVector3T<float> position; /* version_00 > 4: serialized prop position */
    /* version_00 > 5; bit 0 marks the prop for stop-mesh record emission
       in OctPreTree. */
    unsigned int flags;
    char name[0x40]; /* version_00 > 6 */
    W8LevelFileAnimObj anim_obj;
    char has_trigger;
    W8LevelFileTrigger* pTrigger; /* 0xb3 */
    char num_frame_pos;           /* version_00 > 7 */
    /* 0xb8: num_frame_pos frame/tag records; CreatePathProps consumes
       the frame index, the tag feeds the .pvl prop's segment tag. */
    W8LevelFileFramePosition* usFrame_Pos;
    /* version_00 > 8: != 0 serializes footstep surface/material bytes. */
    char has_footsteps;
    unsigned char footstep_surface;
    unsigned char footstep_material;
};

/* The 0x225-byte particle body shared by the level file's particle systems
   and the world file's particle records. ReadWorldParticles reads it
   standalone after taking the version byte itself; the level file keeps it
   embedded behind the version byte. Its 0x225-byte extent and every named
   offset come directly from the version-sized reads and subsequent uses in
   0x004BD0D0. */
struct W8LevelParticleRecord {
    char name[64];                      /* 0x000 */
    srVector3T<float> location;         /* 0x040 */
    float rotation_angle;               /* 0x04c */
    srVector3T<float> rotation_axis;    /* 0x050 */
    unsigned char positional_05c[0x0c]; /* 0x05c */
    unsigned int particle_count;        /* 0x068 */
    /* 0x06c-0x074: emission spread extents; x/y bound both minimum_1d0
       and maximum_1dc symmetrically, z only the maximum. */
    float spread_x;
    float spread_y;
    float spread_z;
    int has_acceleration;            /* 0x078 */
    srVector3T<float> acceleration;  /* 0x07c */
    int expiry_mode;                 /* 0x088: nonzero enables particle expiry */
    int bounds_mode;                 /* 0x08c */
    srVector3T<float> bounds_origin; /* 0x090 */
    float bounds_radius;             /* 0x09c */
    srVector3T<float> bounds_extent; /* 0x0a0 */
    unsigned int lifetime;           /* 0x0ac */
    int velocity_mode;               /* 0x0b0 */
    unsigned int emission_interval;  /* 0x0b4 */
    int los_check;                   /* 0x0b8: nonzero enables the line-of-sight check */
    int placement_mode;              /* 0x0bc */
    float placement_0c0;             /* 0x0c0 */
    float placement_0c4;             /* 0x0c4 */
    float placement_0c8;             /* 0x0c8 */
    float particle_size;             /* 0x0cc: billboard quad scale */
    int flutter_mode;                /* 0x0d0 */
    float flutter_value;             /* 0x0d4 */
    float flutter_period;            /* 0x0d8 */
    int direction_mode;              /* 0x0dc */
    float direction_0e0;             /* 0x0e0 */
    float direction_0e4;             /* 0x0e4 */
    int initially_active;            /* 0x0e8 */
    W8MaterialRecord material;       /* 0x0ec */
    /* 0x216, version >= 2: copied to the particle's attachment_key_260 when
       non-negative. */
    short attachment_key;
    /* 0x218, version >= 3: copied to the particle's emission_limit_184. */
    int emission_limit;
    /* 0x21c, version >= 3: copied to the particle's requires_sorted_renderer_138. */
    unsigned char requires_sorted_renderer;
    int start_frame; /* 0x21d, unaligned, version >= 4 */
    int end_frame;   /* 0x221, version >= 4 */
};

struct W8LevelFileParticleSystem { /* 0x226 */
    char version_00;
    W8LevelParticleRecord particle;
};

/* Serialized form of W8NamedPosition: one version byte precedes the runtime
   record's name, position and trailing scalar fields. */
struct W8LevelFileNamedPosition { /* 0x9d */
    unsigned char version_00;
    char name[0x80];
    srVector3T<float> position;
    int value_08d;
    float value_091;
    float value_095;
    float value_099;
};

/* The serialized environment block gated by has_block, between the
   camera table and the trigger table. The .pvl environment section
   (ReadWorldEnvironment) serializes the same fields: fog gate,
   environment colour/intensity/view distance, camera mode plus optional
   position/angle/axis, and the two 0x300-byte 256-entry RGB colour ramps
   consumed by ReadLightColourTable/ReadEnvironmentColourTable. */
struct W8LevelFileBlock { /* 0x634 */
    unsigned char fog_enabled;
    float environment_red;
    float environment_green;
    float environment_blue;
    float intensity;
    float view_distance;
    /* >= 1 -> camera_position serialized; >= 2 -> camera_angle and
       camera_axis too. */
    unsigned char camera_mode;
    srVector3T<float> camera_position;
    float camera_angle;
    srVector3T<float> camera_axis;
    unsigned char has_light_colours; /* != 0 -> light_colours serialized */
    unsigned char light_colours[0x300];
    unsigned char has_environment_colours;
    unsigned char environment_colours[0x300];
};

/* The level workspace ReadLevelFile builds. Assert-proven pointer members;
   unproven spans are kept as unknown byte arrays. */
struct W8LevelFile {
    unsigned int submesh_count;                      /* pModels element count; initialized 1 */
    int mesh_count;                                  /* the tree's mesh count; initialized 1 */
    W8LevelFileMesh* pMeshes;                        /* 0x08: one 0x5c record */
    OctMeshModel* pModels;                           /* 0x0c: written/freed, elements 0x48 */
    short nTextures;                                 /* 0x10 */
    W8MaterialRecord* pTextures;                     /* 0x12: nTextures * 0x12a */
    short nLights;                                   /* 0x16 */
    W8LevelFileLight* pLights;                       /* 0x18: nLights * 0x44 */
    int nMonsters;                                   /* 0x1c */
    W8LevelFileMonster* pMonsters;                   /* 0x20: nMonsters * 0x26 */
    int nItems;                                      /* 0x24 */
    W8LevelFileItemRecord* pItems;                   /* 0x28: nItems records */
    int missile_count;                               /* 0x2c: the .pvl offset-check names
                                            this section 'missiles' */
    int nProps;                                      /* 0x30 */
    W8LevelFileProp* pProps;                         /* 0x34: nProps * 0xbf */
    int nBitmaps;                                    /* 0x38 */
    W8LevelFileProp* pBitmaps;                       /* 0x3c: nBitmaps * 0xbf */
    int nCameras;                                    /* 0x40 */
    W8LevelFileCamera* pCameras;                     /* 0x44: nCameras * 0x37 */
    int has_block;                                   /* 0x48: gates block */
    W8LevelFileBlock block;                          /* 0x4c */
    int nTriggers;                                   /* 0x680 */
    W8LevelFileTrigger* pTriggers;                   /* 0x684: nTriggers * 6 */
    int camera_mode;                                 /* .pvl reads it as an int into
                                            SetCameraSwayMode */
    int nClippingPlanes;                             /* 0x68c */
    unsigned char clipping_plane_version;            /* nClippingPlanes != 0 */
    W8LevelFileClippingPlaneRecord* pClippingPlanes; /* 0x691: nClippingPlanes records */
    srVector3T<float> environment_offset;            /* .pvl environment
                                                      section tail */
    int nParticleSystems;                            /* 0x6a1 */
    W8LevelFileParticleSystem* pParticleSystems;     /* 0x6a5: nParticleSystems * 0x226 */
    int nNamedPositions;                             /* 0x6a9 */
    W8LevelFileNamedPosition* pNamedPositions;       /* 0x6ad: nNamedPositions * 0x9d */
    /* PrePathing::CreateAutomapNodes fills these with the sorted automap
       cell keys; LevelFile.cpp writes them after the named positions. */
    int num_automap_nodes;
    unsigned long* automap_nodes; /* num_automap_nodes * 4 */
    unsigned char unknown_6b9[4];
    int read_end_position; /* FileGetPos result on read */
    int num_switch_triggers;
    W8LevelFileSwitch* switch_triggers[1000];
    int num_invisible_planes_1665;
    W8LevelFilePlane* invisible_planes_1669[1000];
    int num_linked_records_2609;
    W8LevelFileLinkedRecord* linked_records_260d[100];
};

#pragma pack(pop)

W8LevelFile* ReadLevelFile(int hFile);
BOOLEAN WriteLevelFile(int hFile, int hFileIn, W8LevelFile* pLevel);
BOOLEAN ReadMeshFile(int hFile, W8LevelFileMesh* pMesh);
BOOLEAN WriteMeshFile(int hFile, W8LevelFileMesh* pMesh);
BOOLEAN ReadLightFile(int hFile, W8LevelFileLight* pLight);
BOOLEAN WriteLightFile(int hFile, W8LevelFileLight* pLight);
BOOLEAN ReadAnimLightFile(int hFile, W8LevelFileAnimLight* pLight);
BOOLEAN WriteAnimLightFile(int hFile, W8LevelFileAnimLight* pLight);
BOOLEAN ReadTriggerFile(int hFile, W8LevelFileTrigger* pTrigger);
BOOLEAN WriteTriggerFile(int hFile, W8LevelFileTrigger* pTrigger);
BOOLEAN ReadSuperTriggerFile(int hFile, W8LevelFileTrigger* pTrigger);
BOOLEAN WriteSuperTriggerFile(int hFile, W8LevelFileTrigger* pTrigger);
BOOLEAN ReadDoorTriggerFile(int hFile, W8LevelFileDoorRef* pDoor);
BOOLEAN WriteDoorTriggerFile(int hFile, W8LevelFileDoorRef* pDoor);
BOOLEAN ReadPathAIFile(int hFile, W8LevelFilePathAI* pPathAI);
BOOLEAN WritePathAIFile(int hFile, W8LevelFilePathAI* pPathAI);
BOOLEAN ReadAnimObjFile(int hFile, W8LevelFileAnimObj* pAnimObj);
BOOLEAN WriteAnimObjFile(int hFile, W8LevelFileAnimObj* pAnimObj);
W8LevelFileProp* ReadPropsFile(int hFile, int count);
BOOLEAN WritePropsFile(int hFile, int count, W8LevelFileProp* pProps);
BOOLEAN ReadParticleSystemFile(int hFile, W8LevelFileParticleSystem* pSystem);
BOOLEAN WriteParticleSystemFile(int hFile, W8LevelFileParticleSystem* pSystem);
BOOLEAN ReadLevelFileBlock(int hFile, W8LevelFileBlock* pBlock);
BOOLEAN WriteLevelFileBlock(int hFile, W8LevelFileBlock* pBlock);

#include "wiz8/evidence/LevelFile_layout.inc"

#endif
