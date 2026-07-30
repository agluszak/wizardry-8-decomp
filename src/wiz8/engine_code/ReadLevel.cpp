#include <cstdlib>
#include <cstring>

#include "wiz8/engine_code/PathAI.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/sr_api.h"
#include "wiz8/virtual_file.h"

#define READ_LEVEL_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\ReadLevel.cpp"

namespace {

struct W8ReadLevelInfo {
    W8World* world;
    int hFile;
    const char* bitmap_folder;
};

} // namespace

// FUNCTION: WIZ8 0x004BC850
unsigned char ReadWorldCameras004BC850(
    W8ReadLevelInfo* pInfo, W8World* pWorld)
{
    int count;
    int index;
    int positional_0;
    int positional_1;
    unsigned char has_scale;
    float scale;
    unsigned char success;
    W8WorldCameraEntry* entry;

    if (pInfo == 0 || (pInfo->hFile == 0 | pWorld == 0)) {
        return 0;
    }
    success = ReadVirtualFile(pInfo->hFile, &count, sizeof(count), 0);
    if (!success || count >= 100000) {
        return 0;
    }
    if (count == 0) {
        return 1;
    }

    for (index = 0; index < count; ++index) {
        entry = static_cast<W8WorldCameraEntry*>(
            malloc(sizeof(W8WorldCameraEntry)));
        if (entry == 0) {
            return 0;
        }
        memset(entry, 0, sizeof(W8WorldCameraEntry));
        ReadVirtualFile(pInfo->hFile, &positional_0,
                        sizeof(positional_0), 0);
        ReadVirtualFile(pInfo->hFile, &positional_1,
                        sizeof(positional_1), 0);
        ReadVirtualFile(pInfo->hFile, &has_scale, sizeof(has_scale), 0);
        ReadVirtualFile(pInfo->hFile, entry->positional_00,
                        sizeof(entry->positional_00), 0);
        if (has_scale > 0) {
            ReadVirtualFile(pInfo->hFile, &scale, sizeof(scale), 0);
        }
        else {
            scale = 15.0f;
        }

        entry->path = 0;
        success = success && LoadPathAI004A92A0(&entry->path, pInfo->hFile);
        PathAIEnableTimedMode004A9BA0(entry->path);
        PListAdd(pWorld->plsCameras, entry);
        entry->path->value_10 = index;
        PathAISetScale004AA9C0(entry->path, scale);
    }
    return 1;
}

// FUNCTION: WIZ8 0x004BDC90
unsigned char ReadNamedPositions004BDC90(
    W8ReadLevelInfo* pInfo,
    W8GrowableVector<W8NamedPosition*>* named_positions)
{
    int hFile;
    int count;
    int index;
    unsigned char version;
    W8NamedPosition* pNamedPos;

    hFile = pInfo->hFile;
    ReadVirtualFile(hFile, &count, sizeof(count), 0);
    for (index = 0; index < count; ++index) {
        pNamedPos = new W8NamedPosition;
        if (pNamedPos == 0) {
            srAssertFail("pNamedPos", READ_LEVEL_CPP, 0x6d3,
                         "out of memory creating NamedPos");
        }

        ReadVirtualFile(hFile, &version, sizeof(version), 0);
        if (version != 1) {
            srAssertFail("bVersion == 1", READ_LEVEL_CPP, 0x6d6,
                         "Unknown Named Position version");
        }

        ReadVirtualFile(hFile, pNamedPos->name,
                        sizeof(pNamedPos->name), 0);
        ReadVirtualFile(hFile, &pNamedPos->position.x,
                        sizeof(pNamedPos->position.x), 0);
        ReadVirtualFile(hFile, &pNamedPos->position.y,
                        sizeof(pNamedPos->position.y), 0);
        ReadVirtualFile(hFile, &pNamedPos->position.z,
                        sizeof(pNamedPos->position.z), 0);
        pNamedPos->position.x *= 500.0;
        pNamedPos->position.y *= 500.0;
        pNamedPos->position.z *= 500.0;
        ReadVirtualFile(hFile, &pNamedPos->value_08c,
                        sizeof(pNamedPos->value_08c), 0);
        ReadVirtualFile(hFile, &pNamedPos->value_090,
                        sizeof(pNamedPos->value_090), 0);
        ReadVirtualFile(hFile, &pNamedPos->value_094,
                        sizeof(pNamedPos->value_094), 0);
        ReadVirtualFile(hFile, &pNamedPos->value_098,
                        sizeof(pNamedPos->value_098), 0);
        named_positions->Add(pNamedPos);
    }
    return 1;
}
