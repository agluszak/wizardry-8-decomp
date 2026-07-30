#pragma once

/* Engine Code\Levels.cpp. LevelBuildInfoByID fills eight consecutive path
   buffers. LoadLevel passes the first three to LoadWorld as the level folder,
   mutable level filename and bitmap folder; the remaining buffers are the
   complete level path and the corresponding four sky paths. */
struct W8LevelInfo {
    char level_folder[0x64];
    char level_file_name[0x64];
    char level_bitmap_folder[0x64];
    char level_path[0x100];
    char sky_folder[0x64];
    char sky_file_name[0x64];
    char sky_bitmap_folder[0x64];
    char sky_path[0x100];
};

static_assert(sizeof(W8LevelInfo) == 0x458,
              "W8LevelInfo_must_be_0x458");

unsigned char LevelBuildInfoByID(int level_id, W8LevelInfo* info);
unsigned char LoadLevel(
    int requested_level, int entrance, unsigned char restoring_game);
unsigned char UnloadLevel(const char* save_directory);
