#include "mongen_semantic_test.h"
#include "wiz8/local_code/MonsterGenerator.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/NPCScripting.h"
#include "wiz8/monster_generators.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/MonGen.h"
#include "wiz8/local_screens/mipe.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/xstatus.h"
#include "FileMan.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned int ReadWord(const unsigned char* bytes, int offset, int size)
{
    unsigned int value = 0;
    for (int index = 0; index < size; ++index) {
        value |= static_cast<unsigned int>(bytes[offset + index]) << (index * 8);
    }
    return value;
}

static bool CheckSerialization(MonGen* generator)
{
    char path[] = "MonGenSemantic.bin";
    HWFILE file = FileOpen(path, FILE_ACCESS_READWRITE | FILE_CREATE_ALWAYS, 0);
    if (!file) {
        return false;
    }
    generator->SetName("deterministic");
    generator->flags = 4;
    generator->custom_spawn_chance = 17;
    generator->state_0c.x = 1.0f;
    generator->state_0c.y = 2.0f;
    generator->state_0c.z = 3.0f;
    generator->Save(file);
    unsigned char bytes[59];
    FileSeek(file, 0, FILE_SEEK_FROM_START);
    bool serialized =
        FileRead(file, bytes, sizeof(bytes), 0) && memcmp(bytes + 1, generator->name, 32) == 0 &&
        bytes[33] == 1 && ReadWord(bytes, 34, 4) == 4 && bytes[38] == 17 &&
        ReadWord(bytes, 39, 2) == 10 && ReadWord(bytes, 41, 2) == 0xffff &&
        ReadWord(bytes, 43, 4) == 0x3f800000 && ReadWord(bytes, 47, 4) == 0x40000000 &&
        ReadWord(bytes, 51, 4) == 0x40400000 && ReadWord(bytes, 55, 4) == 0xffffffff;
    /* Retail Save leaves byte zero uninitialized. Supply a known version to
       test Load's documented version-three branch, not a fictitious round trip. */
    unsigned char version = 3;
    FileSeek(file, 0, FILE_SEEK_FROM_START);
    FileWrite(file, &version, 1, 0);
    FileSeek(file, 0, FILE_SEEK_FROM_START);
    MonGen loaded;
    srand(1);
    bool restored = loaded.Load(file) && strcmp(loaded.name, "deterministic") == 0 &&
                    loaded.flags == 0 && loaded.generation_enabled == 1 &&
                    loaded.custom_spawn_chance == 17 && loaded.custom_interval_seconds == 10 &&
                    (unsigned short)loaded.unknown_08 == 0xffff && loaded.state_0c.x == 1.0f &&
                    loaded.state_0c.y == 2.0f && loaded.state_0c.z == 3.0f &&
                    loaded.encounter_table_index == -1;
    FileClose(file);
    FileDelete(path);
    return serialized && restored;
}

bool RunMonGenSemanticTest(void)
{
    GDCamera camera;
    GDCamera* saved_camera = g_gd_camera_65a0f8;
    g_gd_camera_65a0f8 = &camera;
    MonGen generator;
    generator.custom_interval_seconds = 10;
    /* The pinned MSVCRT's first rand after srand(1) is 41: Random(5) is zero,
       so the ten-second interval starts at the lower jitter bound, eight. */
    srand(1);
    generator.Reset();
    W8IntervalGate* timer = generator.m_pTimer;
    bool reset = timer != 0 && timer->m_duration_seconds == 8.0f && (timer->m_flags & 2) == 0;
    srand(1);
    generator.Reset();
    reset = reset && generator.m_pTimer == timer && timer->m_duration_seconds == 8.0f;
    srVector3T<float> position(0.0f, 0.0f, 0.0f);
    generator.flags = 1;
    bool disabled = generator.GenerateEncounter(&position) == 0;
    generator.flags = 0;
    disabled = disabled && generator.GenerateEncounter(&position) == 0;
    generator.generation_enabled = 0;
    disabled = disabled && generator.CanGenerateEncounter(1) == 0;
    generator.generation_enabled = 1;

    bool gates_clear = !g_generator_save_flag && !g_flag_006840bc && !gXStatus.fCombatMode &&
                       !gXStatus.fNpcDialogueMode && !GetFlag68F105() &&
                       !g_status_685170.value_2390;
    GetCameraPosition(&generator.state_0c);
    bool range = gates_clear && generator.CanGenerateEncounter(0) == 0;
    generator.state_0c.x += 200001.0f;
    range = range && generator.CanGenerateEncounter(0) == 0;

    int saved_time = g_status_685170.game_time_ms;
    g_status_685170.game_time_ms = 36000000;
    W8EncounterTableRuntime table;
    unsigned short species = 11;
    unsigned char rarity = 3;
    unsigned char day = 0;
    unsigned char night = 1;
    unsigned char always = 2;
    unsigned char level = 1;
    unsigned char too_high = 4;
    table.species_ids.Add(species);
    table.species_ids.Add(species);
    table.species_ids.Add(species);
    table.rarity_class.Add(rarity);
    table.rarity_class.Add(rarity);
    table.rarity_class.Add(rarity);
    table.time_condition.Add(day);
    table.time_condition.Add(night);
    table.time_condition.Add(always);
    table.challenge_level.Add(level);
    table.challenge_level.Add(level);
    table.challenge_level.Add(too_high);
    W8GrowableVector<int> candidates;
    srand(1);
    bool filtering =
        generator.SelectEncounterCandidates(&table, &candidates) == 1 && *candidates.GetAt(0) == 0;
    g_status_685170.game_time_ms = 0;
    srand(1);
    filtering = filtering && generator.SelectEncounterCandidates(&table, &candidates) == 1 &&
                *candidates.GetAt(0) == 1;
    g_status_685170.game_time_ms = saved_time;

    W8MonsterGroup group;
    memset(&group, 0, sizeof(group));
    group.flag_c3 = 1;
    group.spawn_time = g_status_685170.world_clock;
    int before = g_active_groups.GetCount();
    RegisterActiveEncounterGroup(&group);
    RegisterActiveEncounterGroup(&group);
    bool lifecycle = g_active_groups.GetCount() == before + 1;
    int saved_level = g_status_685170.current_level;
    g_status_685170.current_level = 0;
    CullExpiredEncounters();
    lifecycle = lifecycle && g_active_groups.IndexOf(&group) != -1;
    UnregisterActiveEncounterGroup(&group);
    UnregisterActiveEncounterGroup(&group);
    lifecycle = lifecycle && g_active_groups.GetCount() == before;
    g_status_685170.current_level = saved_level;
    bool serialization = CheckSerialization(&generator);
    g_gd_camera_65a0f8 = saved_camera;
    delete camera.m_manual_input_timer;
    fprintf(
        stderr,
        "WIZ8_MONGEN reset=%u disabled=%u range=%u filtering=%u lifecycle=%u serialization=%u\n",
        reset, disabled, range, filtering, lifecycle, serialization);
    return reset && disabled && range && filtering && lifecycle && serialization;
}
