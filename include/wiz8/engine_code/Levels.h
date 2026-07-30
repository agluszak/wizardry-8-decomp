#pragma once

/* Engine Code\Levels.cpp. LevelBuildInfoByID fills three consecutive path
   buffers. LoadLevel passes them to LoadWorld as the level folder, mutable
   level filename, and asset folder respectively. Only the string prefix of
   the final 0x390-byte region is consumed by the recovered orchestration. */
struct W8LevelInfo {
    char level_folder[0x64];
    char level_file_name[0x64];
    char asset_folder[0x390];
};

static_assert(sizeof(W8LevelInfo) == 0x458,
              "W8LevelInfo_must_be_0x458");

unsigned char LevelBuildInfoByID(int level_id, W8LevelInfo* info);
unsigned char LoadLevel(
    int requested_level, int entrance, unsigned char restoring_game);
unsigned char UnloadLevel(const char* save_directory);
