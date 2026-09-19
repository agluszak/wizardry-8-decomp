#ifndef WIZ8_LAYOUTS_LEVELS_H
#define WIZ8_LAYOUTS_LEVELS_H

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
    /* 0x03: counted by RecordMonsterKill for each in-combat kill credited
       while the party is on this level. */
    short monster_kill_count_03;
    unsigned int experience_gained; /* 0x05: accumulated when experience is awarded */
    int gold_collected;
    int sight_clock;
    unsigned char unknown_11[0x10];
};

#pragma pack(pop)

static_assert(sizeof(W8LevelFolderRecord) == 0x6b, "W8LevelFolderRecord_must_be_0x6b");
static_assert(sizeof(W8LevelProgressRow) == 0x21, "W8LevelProgressRow_must_be_0x21");

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

static_assert(sizeof(W8LevelInfo) == 0x458, "W8LevelInfo_must_be_0x458");

/* The forty-seven real levels. Level ids 47..56 are the test-level slots the
   LoadLevel assert (TEST_LEVEL_COUNT) still admits. */
enum { W8_LEVEL_COUNT = 47 };

#endif
