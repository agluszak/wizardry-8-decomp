/* Layout evidence for include/wiz8/engine_code/LevelFile.h. Included after its declarations. */

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
static_assert(offsetof(W8LevelFileSwitch, door) == 0x264, "W8LevelFileSwitch_door_264");
static_assert(offsetof(W8LevelFileSuperTrigger, door) == 0x85e, "W8LevelFileSuperTrigger_door_85e");
static_assert(sizeof(W8LevelFileLightExtra) == 0x3c, "W8LevelFileLightExtra_must_be_0x3c");
static_assert(sizeof(W8LevelFileItemRecord) == 0x44, "W8LevelFileItemRecord_must_be_0x44");
static_assert(sizeof(W8LevelFileClippingPlaneRecord) == 0x50,
              "W8LevelFileClippingPlaneRecord_must_be_0x50");
static_assert(sizeof(W8LevelFileFramePosition) == 4, "W8LevelFileFramePosition_must_be_4");
static_assert(sizeof(W8LevelFileTriggerPosition) == 0x1c,
              "W8LevelFileTriggerPosition_must_be_0x1c");
static_assert(offsetof(W8LevelFileTriggerPosition, angle) == 0x0c,
              "Level trigger placement angle offset");
static_assert(offsetof(W8LevelFileTriggerPosition, direction) == 0x10,
              "Level trigger placement direction offset");
static_assert(offsetof(W8LevelFileCamera, name) == 0x09, "Level camera name offset");
static_assert(sizeof(W8LevelFileTriggerHotSpot) == 0x85, "W8LevelFileTriggerHotSpot_must_be_0x85");
static_assert(sizeof(W8LevelFileTrigger) == 6, "W8LevelFileTrigger_must_be_6");
static_assert(sizeof(W8LevelFileFrame) == 0x63, "W8LevelFileFrame_must_be_0x63");
static_assert(sizeof(W8LevelFileMorph) == 6, "W8LevelFileMorph_must_be_6");
static_assert(sizeof(W8LevelFileTransform) == 0x1c, "W8LevelFileTransform_must_be_0x1c");
static_assert(sizeof(W8LevelFileAnimObj) == 0x5f, "W8LevelFileAnimObj_must_be_0x5f");
static_assert(sizeof(W8LevelFileProp) == 0xbf, "W8LevelFileProp_must_be_0xbf");
static_assert(offsetof(W8LevelFileProp, version) == 0x00, "W8LevelFileProp_version");
static_assert(offsetof(W8LevelFileProp, bNumFrames) == 0x01, "W8LevelFileProp_bNumFrames");
static_assert(offsetof(W8LevelFileProp, option) == 0x02, "W8LevelFileProp_option");
static_assert(offsetof(W8LevelFileProp, position) == 0x03, "W8LevelFileProp_position_03");
static_assert(offsetof(W8LevelFileProp, flags) == 0x0f, "W8LevelFileProp_flags_0f");
static_assert(offsetof(W8LevelFileProp, name) == 0x13, "W8LevelFileProp_name_13");
static_assert(offsetof(W8LevelFileProp, anim_obj) == 0x53, "W8LevelFileProp_anim_obj_53");
static_assert(offsetof(W8LevelFileProp, has_trigger) == 0xb2, "W8LevelFileProp_has_trigger_b2");
static_assert(offsetof(W8LevelFileProp, pTrigger) == 0xb3, "W8LevelFileProp_pTrigger");
static_assert(offsetof(W8LevelFileProp, num_frame_pos) == 0xb7, "W8LevelFileProp_num_frame_pos_b7");
static_assert(offsetof(W8LevelFileProp, usFrame_Pos) == 0xb8, "W8LevelFileProp_usFrame_Pos");
static_assert(offsetof(W8LevelFileProp, has_footsteps) == 0xbc, "W8LevelFileProp_has_footsteps_bc");
static_assert(offsetof(W8LevelFileProp, footstep_surface) == 0xbd,
              "W8LevelFileProp_footstep_surface_bd");
static_assert(offsetof(W8LevelFileProp, footstep_material) == 0xbe,
              "W8LevelFileProp_footstep_material_be");
static_assert(sizeof(W8LevelParticleRecord) == 0x225, "W8LevelParticleRecord_must_be_0x225");
static_assert(sizeof(W8LevelFileParticleSystem) == 0x226,
              "W8LevelFileParticleSystem_must_be_0x226");
static_assert(offsetof(W8LevelFileParticleSystem, particle.location) == 0x41,
              "W8LevelFileParticleSystem_position_41");
static_assert(offsetof(W8LevelFileParticleSystem, particle.attachment_key) == 0x217,
              "W8LevelFileParticleSystem_attachment_key_217");
static_assert(sizeof(W8LevelFileNamedPosition) == 0x9d, "W8LevelFileNamedPosition_must_be_0x9d");
static_assert(offsetof(W8LevelFileNamedPosition, position) == 0x81,
              "W8LevelFileNamedPosition_position_81");
static_assert(sizeof(W8LevelFileBlock) == 0x634, "W8LevelFileBlock_must_be_0x634");
static_assert(sizeof(W8LevelFile) == 0x279d, "W8LevelFile_must_be_0x279d");
static_assert(offsetof(W8LevelFile, pClippingPlanes) == 0x691, "W8LevelFile_pClippingPlanes");
