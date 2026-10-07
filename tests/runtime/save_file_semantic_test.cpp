#include "save_file_semantic_test.h"
#include "runtime_case.h"
#include "wiz8/chunk.h"
#include "wiz8/engine_code/GrCycle.h"
#include "wiz8/layouts/character.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/local_code/LoadSaveGame.h"
#include "FileMan.h"

#include <new>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

namespace {

const char kFixture[] = "save-status-fixture.bin";

bool CheckEffectFlags(void*)
{
    for (unsigned int preset = 0; preset != 2; ++preset) {
        void* storage = malloc(sizeof(W8CameraShakeEffect));
        if (storage == 0) {
            return false;
        }
        memset(storage, 0xcc, sizeof(W8CameraShakeEffect));
        W8CameraShakeEffect* effect =
            new (storage) W8CameraShakeEffect(2.0f, preset != 0, 1.0f, 2.0f, 0);
        unsigned int flags;
        memcpy(&flags, &effect->flags, sizeof(flags));
        bool ok = flags == (preset ? 0x1cU : 0U);
        effect->~W8CameraShakeEffect();
        free(storage);
        if (!ok) {
            fprintf(stderr, "camera-shake flags preset=%u got=%08x\n", preset, flags);
            return false;
        }
    }
    return true;
}

bool CheckSaveVersions(void*)
{
    static W8GlobalStatus input;
    static W8GlobalStatus output;
    static W8Character input_characters[8];
    static W8Character output_characters[8];
    static W8PartySlotRow input_rows[8];
    static W8PartySlotRow output_rows[8];

    for (unsigned int status_version = 0; status_version != 2; ++status_version) {
        for (unsigned int character_version = 1; character_version <= 2; ++character_version) {
            memset(&input, 0, sizeof(input));
            memset(input_characters, 0, sizeof(input_characters));
            memset(input_rows, 0, sizeof(input_rows));
            input.buffers.save_version = status_version ? 1.1f : 1.0f;
            input.buffers.Char = input_characters;
            input.buffers.XChar = input_rows;
            input.party_gold = 0x12345678;
            for (unsigned int slot = 0; slot != 8; ++slot) {
                input.formation.positions[slot].bQuadrant = -1;
                input.formation.positions[slot].bQuadrantSlot = -1;
                input_characters[slot].record_version = character_version;
                input_characters[slot].iProfession = W8_PROFESSION_MAGE;
                input_characters[slot].original_profession = W8_PROFESSION_FIGHTER;
                input_rows[slot].item_slot = static_cast<unsigned short>(slot + 10);
            }
            for (unsigned int line = 0; line != 4; ++line) {
                input.text_box_lines_used[line] = line + 20;
                input.text_box_lines_shown[line] = line + 30;
                if (line < 3) {
                    input.legacy_text_box_lines[0][line] = line + 40;
                    input.legacy_text_box_lines[1][line] = line + 50;
                }
            }
            FileDelete(const_cast<char*>(kFixture));
            W8Chunk chunks;
            bool ok = chunks.OpenWrite(const_cast<char*>(kFixture)) &&
                      chunks.OpenChunk(0x41545347, 0); // GSTA
            unsigned int size = sizeof(input);
            ok = ok && chunks.Write(&size, 4, 0) && chunks.Write(&input, size, 0);
            for (unsigned int character = 0; character != 8 && ok; ++character) {
                size = sizeof(W8Character);
                ok = chunks.Write(&size, 4, 0) &&
                     chunks.Write(&input_characters[character], size, 0);
            }
            for (unsigned int row = 0; row != 8 && ok; ++row) {
                size = sizeof(W8PartySlotRow);
                ok = chunks.Write(&size, 4, 0) && chunks.Write(&input_rows[row], size, 0);
            }
            ok = ok && chunks.ReleaseCurrentChunk();
            chunks.Close();
            memset(&output, 0, sizeof(output));
            output.buffers.Char = output_characters;
            output.buffers.XChar = output_rows;
            ok = ok && chunks.OpenRead(const_cast<char*>(kFixture)) && chunks.OpenChunk(0, 0) &&
                 chunks.CurrentChunkId() == 0x41545347;
            if (ok) {
                LoadGameStatus(&chunks, &output);
                ok =
                    FileGetPos(chunks.m_hFile) == static_cast<INT32>(FileGetSize(chunks.m_hFile)) &&
                    output.buffers.Char == output_characters &&
                    output.buffers.XChar == output_rows && output.party_gold == 0x12345678;
                for (unsigned int loaded = 0; loaded != 8 && ok; ++loaded) {
                    ok = output_characters[loaded].original_profession ==
                             (character_version < 2 ? W8_PROFESSION_MAGE : W8_PROFESSION_FIGHTER) &&
                         output_rows[loaded].item_slot == loaded + 10;
                }
                for (unsigned int migrated = 0; migrated != 4 && ok; ++migrated) {
                    unsigned int used = status_version ? migrated + 20
                                        : migrated < 3 ? migrated + 40
                                                       : 0;
                    unsigned int shown = status_version ? migrated + 30
                                         : migrated < 3 ? migrated + 50
                                                        : 0;
                    ok = output.text_box_lines_used[migrated] == used &&
                         output.text_box_lines_shown[migrated] == shown;
                }
            }
            chunks.Close();
            if (!ok) {
                fprintf(stderr, "save-file status=%u character=%u failed\n", status_version,
                        character_version);
                return false;
            }
        }
    }
    return true;
}

} // namespace

bool RunSaveFileSemanticTests(RuntimeCase& test)
{
    test.expected("GSTA 1.0/1.1 migration, character 1/2 migration and retained process pointers");
    return test.run_invariant("save-file-versions", CheckSaveVersions, 0) &&
           test.run_invariant("camera-shake-flags", CheckEffectFlags, 0);
}
