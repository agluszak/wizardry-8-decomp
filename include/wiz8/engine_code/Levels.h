#pragma once

#pragma pack(push, 1)

struct W8LevelFolderRecord {
    char folder_name[50];
    char level_name[50];
    char location_code[4];
    signed char sky_index;
    signed char cd_number;
    signed char unknown_6a;
};

struct W8LevelProgressRow {
    unsigned char visited;
    /* 0x01: incremented by the combat teardown for each finished fight while
       characters are still active. */
    unsigned short combat_end_count_01;
    unsigned char unknown_03[6];
    int gold_collected;
    int sight_clock;
    unsigned char unknown_11[0x10];
};

#pragma pack(pop)

static_assert(sizeof(W8LevelFolderRecord) == 0x6b,
              "W8LevelFolderRecord_must_be_0x6b");
static_assert(sizeof(W8LevelProgressRow) == 0x21,
              "W8LevelProgressRow_must_be_0x21");

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

extern "C" {
extern W8LevelFolderRecord g_level_folders[47];

int GetLoadedLevelID(void);
const char* GetLevelFolderName(int level_id);
unsigned char GetLevelLocationCode(int level_id, char* location_code);
int FindLevelIdByLocationCode(const char* location_code);
}

extern unsigned short g_level_name_indices_605820[47];

unsigned char LevelBuildInfoByID(int level_id, W8LevelInfo* info);
unsigned char LoadSkyWorld0042B020(int level, W8LevelInfo* info);
unsigned char LoadLevel(
    int requested_level, int entrance, unsigned char restoring_game);
unsigned char UnloadLevel(const char* save_directory);

unsigned char IsLevelCdMissing0042B6F0(int level);
unsigned char FindGameDataPath0042B590(char* path, int cd_number);
int GetLevelCdNumber0042B720(int level);
void Function42B3E0(void);
char Function42B740(int saved_level);

extern unsigned char g_level_runtime_flag_0065ba70;
extern unsigned char g_flag_00659756;
extern float g_default_world_height_00603ac8;
extern float g_position_height_epsilon_005ebfdc;
extern unsigned char g_environment_load_flag_00603ad0;
extern unsigned char g_value_0068f0fd;
extern unsigned char g_flag_00687607;

unsigned char Function42AF60(int level, int entrance);


void Function42B770(int, int);

