#include "level_file_semantic_test.h"
#include "runtime_case.h"
#include "wiz8/engine_code/LevelFile.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/3d.h"
#include "wiz8/float_constants.h"
#include "FileMan.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace {

const char kFixture[] = "level-file-fixture.bin";
const char kWritten[] = "level-file-written.bin";

bool WriteFixture(const unsigned char* bytes, unsigned int size)
{
    // Retail FileOpen uses OPEN_ALWAYS for an existing physical file,
    // including FILE_CREATE_ALWAYS. Each fixture needs a genuinely new file.
    FileDelete(const_cast<char*>(kFixture));
    HWFILE file = FileOpen(const_cast<char*>(kFixture), FILE_ACCESS_WRITE | FILE_CREATE_ALWAYS, 0);
    if (!file) {
        return false;
    }
    unsigned int written = 0;
    bool ok = FileWrite(file, const_cast<unsigned char*>(bytes), size, &written) && written == size;
    FileClose(file);
    return ok;
}

HWFILE OpenOutput()
{
    FileDelete(const_cast<char*>(kWritten));
    return FileOpen(const_cast<char*>(kWritten), FILE_ACCESS_WRITE | FILE_CREATE_ALWAYS, 0);
}

bool MatchesFixture(const unsigned char* expected, unsigned int size)
{
    HWFILE file = FileOpen(const_cast<char*>(kWritten), FILE_ACCESS_READ, 0);
    if (!file) {
        return false;
    }
    unsigned char actual[256];
    unsigned int read = 0;
    bool ok = size <= sizeof(actual) && FileGetSize(file) == size &&
              FileRead(file, actual, size, &read) && read == size &&
              memcmp(actual, expected, size) == 0;
    FileClose(file);
    return ok;
}

void AppendWord(unsigned char* bytes, unsigned int& size, unsigned int value)
{
    for (unsigned int i = 0; i < 4; ++i) {
        bytes[size++] = static_cast<unsigned char>(value >> (i * 8));
    }
}

// Little-endian IEEE-754 encodings, independent of the recovered structs.
const unsigned int kFloats[] = {0x3f800000, 0x40000000, 0x40400000, 0x40800000, 0x40a00000,
                                0x40c00000, 0x40e00000, 0x41000000, 0x41100000, 0x41200000};

void AppendFloats(unsigned char* bytes, unsigned int& size, unsigned int count)
{
    for (unsigned int i = 0; i < count; ++i) {
        AppendWord(bytes, size, kFloats[i]);
    }
}

bool CheckPathRecords()
{
    for (unsigned int mode = 0; mode <= 2; ++mode) {
        unsigned char bytes[128];
        unsigned int size = 0;
        bytes[size++] = 1;
        bytes[size++] = static_cast<unsigned char>(mode);
        AppendWord(bytes, size, 0xfffffffe); // signed position -2
        AppendWord(bytes, size, 0x12345678); // opaque serialized word
        AppendWord(bytes, size, 2);
        AppendFloats(bytes, size, mode == 2 ? 10 : 7);
        AppendFloats(bytes, size, mode == 2 ? 10 : 7);
        if (!WriteFixture(bytes, size)) {
            return false;
        }
        HWFILE file = FileOpen(const_cast<char*>(kFixture), FILE_ACCESS_READ, 0);
        W8LevelFilePathAI path;
        memset(&path, 0, sizeof(path));
        bool ok = file && ReadPathAIFile(file, &path) &&
                  FileGetPos(file) == static_cast<int>(size) && path.position == -2 &&
                  path.path_count == 2;
        if (file) {
            FileClose(file);
        }
        if (ok && mode == 2) {
            ok = path.pScaledPaths && !path.pPaths &&
                 path.pScaledPaths[1].path.position.x == 1.0f &&
                 path.pScaledPaths[1].path.axis.z == 7.0f && path.pScaledPaths[1].scale.x == 8.0f &&
                 path.pScaledPaths[1].scale.z == 10.0f;
        } else if (ok) {
            ok = path.pPaths && !path.pScaledPaths && path.pPaths[1].position.x == 1.0f &&
                 path.pPaths[1].angle == 4.0f && path.pPaths[1].axis.z == 7.0f;
        }
        file = OpenOutput();
        ok = file && ok && WritePathAIFile(file, &path);
        if (file) {
            FileClose(file);
        }
        // The writer consumes these malloc-backed arrays and clears the pointers.
        ok = ok && !path.pPaths && !path.pScaledPaths && MatchesFixture(bytes, size);
        if (!ok) {
            fprintf(stderr, "level-file path mode=%u failed\n", mode);
            return false;
        }
    }
    return true;
}

bool CheckWGDRecords()
{
    if (g_environ || g_level_data) {
        return false;
    }
    unsigned char bytes[256];
    unsigned int size = 0;
    AppendWord(bytes, size, 3); // vertex count differs from face count
    AppendWord(bytes, size, 1);
    const unsigned int vertices[] = {0, 0, 0, 0x40000000, 0, 0, 0, 0, 0x40000000};
    for (unsigned int vertex_word = 0; vertex_word < 9; ++vertex_word) {
        AppendWord(bytes, size, vertices[vertex_word]);
    }
    AppendWord(bytes, size, 0);
    AppendWord(bytes, size, 1);
    AppendWord(bytes, size, 2);
    AppendWord(bytes, size, 0);
    AppendWord(bytes, size, 0x3f800000); // source normal (0,1,0)
    AppendWord(bytes, size, 0);
    AppendWord(bytes, size, 2);          // face version
    AppendWord(bytes, size, 1);          // walkable classification
    AppendWord(bytes, size, 0x3f000000); // slope .5
    AppendWord(bytes, size, 0x3e800000); // contact margin .25
    AppendWord(bytes, size, 0x00000403); // material 3, surface 4, padding
    AppendWord(bytes, size, 73);         // trace chance
    AppendWord(bytes, size, 99);         // retail replaces the stored trigger index
    const unsigned int bounds[] = {0, 0, 0, 0x40000000, 0, 0x40000000};
    for (unsigned int bound_word = 0; bound_word < 6; ++bound_word) {
        AppendWord(bytes, size, bounds[bound_word]);
    }
    if (!WriteFixture(bytes, size)) {
        return false;
    }
    HANDLE file =
        CreateFileA(kFixture, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }
    W8GameData* saved_game_data = g_octree_game_data;
    W8GameData* game_data = new W8GameData(0, true);
    bool ok = game_data->ReadWGDList(file, 0) && SetFilePointer(file, 0, 0, FILE_CURRENT) == size &&
              game_data->m_iNumVertices == 3 && game_data->m_iNumSurfaces == 1 &&
              game_data->m_pVertices[1].x == 2.0f * g_world_scale &&
              game_data->m_pVertices[2].z == 2.0f * g_world_scale &&
              game_data->m_pSurfaces[0].vertex_indices[2] == 2 &&
              game_data->m_pSurfaces[0].footstep_material == 3 &&
              game_data->m_pSurfaces[0].footstep_surface == 4 &&
              game_data->m_pSurfaces[0].chance == 73 &&
              game_data->m_pSurfaces[0].trigger_index == 0;
    CloseHandle(file);
    // Like oct-file, retain the constructed fixture for process teardown;
    // restore the constructor's publications before the next invariant.
    g_octree_game_data = saved_game_data;
    g_environ = 0;
    return ok;
}

bool CheckAnimationVersions()
{
    for (unsigned int version = 1; version <= 9; ++version) {
        for (unsigned int transforms = 0; transforms <= 1; ++transforms) {
            unsigned char bytes[256];
            unsigned int size = 0;
            bytes[size++] = static_cast<unsigned char>(version);
            bytes[size++] = 0; // no morph channels
            bytes[size++] = 2; // animation playing
            bytes[size++] = 3; // frame method
            bytes[size++] = 4; // behaviour
            bytes[size++] = 5; // cycle
            bytes[size++] = static_cast<unsigned char>(transforms);
            if (version >= 3) {
                AppendWord(bytes, size, 0x40000000); // playback scale 2
            }
            if (version >= 5) {
                bytes[size++] = 6; // start frame
            }
            if (version >= 6) {
                bytes[size++] = 1;                   // random play
                AppendWord(bytes, size, 0x3f000000); // chance 0.5
            }
            for (unsigned int i = 0; i < 50; ++i) {
                bytes[size++] = static_cast<unsigned char>(i + 1);
            }
            if (version >= 7) {
                bytes[size++] = 1; // bounds count
                AppendFloats(bytes, size, 6);
            }
            if (version >= 8) {
                bytes[size++] = 1; // animated light count
                bytes[size++] = 1; // light version, without extra payload
                AppendFloats(bytes, size, 8);
            }
            if (transforms) {
                bytes[size++] = 1; // one transform, no mesh frames
                bytes[size++] = 7; // channel
                bytes[size++] = 0; // frame count
            } else if (version >= 9) {
                bytes[size++] = 1; // optional PathAI present
            }
            if (transforms || version >= 9) {
                bytes[size++] = 1;                   // PathAI version
                bytes[size++] = 0;                   // unscaled
                AppendWord(bytes, size, 0xffffffff); // position -1
                AppendWord(bytes, size, 0x12345678);
                AppendWord(bytes, size, 0); // no keyframes
            }
            if (!WriteFixture(bytes, size)) {
                return false;
            }
            HWFILE file = FileOpen(const_cast<char*>(kFixture), FILE_ACCESS_READ, 0);
            W8LevelFileAnimObj anim;
            memset(&anim, 0, sizeof(anim));
            bool ok = file && ReadAnimObjFile(file, &anim) &&
                      FileGetPos(file) == static_cast<int>(size) &&
                      anim.playback_scale == (version >= 3 ? 2.0f : 15.0f) &&
                      anim.start_frame == (version >= 5 ? 6 : 0) &&
                      anim.random_play == (version >= 6 ? 1 : 0) &&
                      anim.play_chance == (version >= 6 ? 0.5f : 1.0f);
            if (file) {
                FileClose(file);
            }
            if (ok && version >= 7) {
                ok = anim.num_bound_box == 1 && anim.pBoundBox &&
                     anim.pBoundBox[0].minimum.z == 3.0f && anim.pBoundBox[0].maximum.z == 6.0f;
            }
            if (ok && version >= 8) {
                ok = anim.num_anim_lights == 1 && anim.pAnimLights &&
                     anim.pAnimLights[0].color.z == 6.0f && anim.pAnimLights[0].intensity == 7.0f &&
                     anim.pAnimLights[0].range == 8.0f;
            }
            if (ok && transforms) {
                ok = anim.num_transforms == 1 && anim.pTransforms &&
                     anim.pTransforms[0].channel == 7 && anim.pTransforms[0].pathAI.position == -1;
            } else if (ok && version >= 9) {
                ok = anim.has_path_ai == 1 && anim.pPathAI && anim.pPathAI->position == -1;
            }
            file = OpenOutput();
            ok = file && ok && WriteAnimObjFile(file, &anim);
            if (file) {
                FileClose(file);
            }
            if (!ok || !MatchesFixture(bytes, size)) {
                fprintf(stderr, "level-file animation version=%u transforms=%u failed\n", version,
                        transforms);
                return false;
            }
        }
    }
    return true;
}

bool CheckPropVersions()
{
    for (unsigned int version = 1; version <= 9; ++version) {
        unsigned char bytes[256];
        unsigned int size = 0;
        bytes[size++] = static_cast<unsigned char>(version);
        bytes[size++] = 0; // no mesh frames
        if (version >= 5) {
            bytes[size++] = 3; // option
            AppendFloats(bytes, size, 3);
        }
        if (version >= 6) {
            AppendWord(bytes, size, 0x12345678);
        }
        if (version >= 7) {
            for (unsigned int i = 0; i < 64; ++i) {
                bytes[size++] = i == 0 ? 'P' : 0;
            }
        }
        if (version >= 8) {
            bytes[size++] = 2;
            AppendWord(bytes, size, 0x00020001); // frame 1, tag 2
            AppendWord(bytes, size, 0x00040003); // frame 3, tag 4
        }
        // Version-one AnimObj with no morphs or transforms.
        bytes[size++] = 1;
        for (unsigned int header_byte = 0; header_byte < 6; ++header_byte) {
            bytes[size++] = 0;
        }
        for (unsigned int discarded_byte = 0; discarded_byte < 50; ++discarded_byte) {
            bytes[size++] = static_cast<unsigned char>(discarded_byte + 1);
        }
        bytes[size++] = 0; // no trigger
        if (version >= 9) {
            bytes[size++] = 1;
            bytes[size++] = 7;  // footstep surface
            bytes[size++] = 11; // footstep material
        }
        if (!WriteFixture(bytes, size)) {
            return false;
        }
        HWFILE file = FileOpen(const_cast<char*>(kFixture), FILE_ACCESS_READ, 0);
        W8LevelFileProp* prop = file ? ReadPropsFile(file, 1) : 0;
        bool ok = prop && FileGetPos(file) == static_cast<int>(size);
        if (file) {
            FileClose(file);
        }
        if (ok && version >= 5) {
            ok = prop->option == 3 && prop->position.z == 3.0f;
        }
        if (ok && version >= 8) {
            ok = prop->num_frame_pos == 2 && prop->usFrame_Pos && prop->usFrame_Pos[1].frame == 3 &&
                 prop->usFrame_Pos[1].tag == 4;
        }
        if (ok && version >= 9) {
            ok =
                prop->has_footsteps && prop->footstep_surface == 7 && prop->footstep_material == 11;
        }
        file = OpenOutput();
        ok = file && ok && WritePropsFile(file, 1, prop);
        if (file) {
            FileClose(file);
        }
        if (!ok || !MatchesFixture(bytes, size)) {
            fprintf(stderr, "level-file prop version=%u failed\n", version);
            return false;
        }
    }
    return true;
}

bool CheckLevelFiles(void*)
{
    return CheckPathRecords() && CheckAnimationVersions() && CheckPropVersions() &&
           CheckWGDRecords();
}

} // namespace

bool RunLevelFileSemanticTests(RuntimeCase& test)
{
    test.expected("byte-exact PathAI modes 0..2, AnimObj/Prop versions 1..9 and WGD face records");
    return test.run_invariant("level-file-layouts", CheckLevelFiles, 0);
}
