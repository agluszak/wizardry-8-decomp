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
