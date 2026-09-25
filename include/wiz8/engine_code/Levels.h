#pragma once

#include "wiz8/layouts/levels.h"

extern W8LevelFolderRecord g_level_folders[W8_LEVEL_COUNT];

int GetLoadedLevelID(void);
const char* GetLevelFolderName(int level_id);
unsigned char GetLevelLocationCode(int level_id, char* location_code);
int FindLevelIdByLocationCode(const char* location_code);

extern unsigned short g_level_name_indices[W8_LEVEL_COUNT];

unsigned char LevelBuildInfoByID(int level_id, W8LevelInfo* info);
unsigned char LoadSkyWorld(int level, W8LevelInfo* info);
unsigned char LoadLevel(int requested_level, int entrance, unsigned char restoring_game);
unsigned char UnloadLevel(const char* save_directory);

bool IsLevelCdMissing(int level);
unsigned char FindGameDataPath(char* path, int cd_number);
int GetLevelCdNumber(int level);
void UnloadSkyWorld(void);
char GetLevelBand(int saved_level);

extern bool g_camera_path_active;
extern unsigned char g_level_status_loading;
extern float g_default_world_height;
extern float g_position_height_epsilon;
extern unsigned char g_environment_load_flag;
extern unsigned char g_mipe_trigger_display;

unsigned char ReloadLevelPreservingCamera(int level, int entrance);

/* 0x0042B770: start the saved level's music playlist, falling back to
   Adventure.MPL when the level-specific playlist does not exist. */
void StartLevelMusic(int fade, int replace_current);
class W8MaterialMapper;
extern W8MaterialMapper g_material_mapper;
