#include "wiz8/float_constants.h"
#include "wiz8/engine_code/SoundEvent.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/combat_state.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/engine_code/AniMesh.h"
#include "wiz8/engine_code/AnimObj.h"
#include "wiz8/engine_code/Item.h"
#include "wiz8/engine_code/Missile.h"
#include "wiz8/engine_code/materials.h"
#include "wiz8/engine_code/MonsterLight.h"
#include "wiz8/engine_code/registry_classes.h"
#include "wiz8/engine_code/game_timer.h"
#include "wiz8/engine_code/ReadLevel.h"
#include "wiz8/engine_code/Trigger.h"
#include "wiz8/engine_code/stTextureAnim.h"
#include "wiz8/engine_code/stScript.h"
#include "wiz8/engine_code/stSound3D.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/PathAI.h"
#include "wiz8/grcycle.h"
#include "wiz8/magic.h"
#include "wiz8/mesh_model.h"
#include "wiz8/monster_runtime.h"
#include "wiz8/sr_api.h"
#include "wiz8/targeting.h"
#include "wiz8/utility.h"
#include "wiz8/vector_005ec294.h"
#include "wiz8/virtual_file.h"
#include "surrender/srTimer.h"
#include "surrender/srScene.h"
#include "surrender/srModelInstance.h"
#include "surrender/srCore.h"
#include "Random.h"
#include "FileMan.h"
#include "soundman.h"
#include <windows.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

extern srTimer* g_shared_timer_base;
extern unsigned char GetFlag68F105(void);
extern unsigned char FindEntityByName(
    const char* name,
    srVector3T<float>* position,
    int* value,
    srVector3T<float>* direction);
extern void SetTriggerVariableByName00444030(const char* name, int value);
extern void Function401920(const char* message);
extern char* FormatString(const char* format, ...);
extern const float g_monster_rotation_offset_005ec04c;
extern float g_float_005ebb34;
extern const float g_float_005ebcf8;
extern const double g_monster_death_rotation_pi_005ed1f0;
extern float g_light_scale_0060bfe0;
extern float g_monster_scale_transition_step_005ebcf4;
extern unsigned char g_monster_model_value_enabled_00685111;
unsigned char g_monster_shadow_updates_enabled_0065970c;
extern unsigned char g_monster_combat_timer_enabled_006f0531;
extern unsigned char g_flag_00683fce;
extern const float g_monster_attachment_distance_scale_005ed2a8;
extern const float g_monster_attachment_vertical_scale_005eca84;
extern const double g_monster_attachment_group_spacing_005ed2a0;
extern const float g_monster_linked_vertical_scale_005ed29c;
extern const float g_monster_poster_vertical_rate_005ed298;
extern const double g_monster_poster_max_distance_005ec3d8;
extern srVector3T<float> g_monster_attachment_offsets_0060e618[][8];
extern float g_monster_attachment_scales_0060e914[];
extern void SetChainValue15C(char* node, int value);
extern void GetCameraPosition(srVector3T<float>* position);
extern W8World* GetWorld(void);
extern unsigned char Function525DF0(int value);
extern unsigned char g_flag_00683f97;
extern W8Navigator* g_startup_world_659c0c;
extern float g_startup_depth_603ac8;
extern const float g_monster_script_facing_tolerance_005ebc84;
extern const float g_world_scale_005ebc40;
extern unsigned char g_force_encounter_culling; /* 0x00687500 */
extern W8WideChar* GetMonsterName(
    W8MonsterInfo* monster_info,
    W8MonsterRecord* record,
    unsigned char name_form);
extern int FindItemRecordByName(const char* name);
extern void CreateItemIntoHandOrPool(int item_id, unsigned char quality);
extern int Function50A440(unsigned int monster_list_index);
extern void Function56C590(
    int npc_record, int value, int line, unsigned char suppress);
extern void Function547570(
    W8MonsterGroup* monster_group, unsigned char disposition, int value);
extern Trigger* FindTriggerByName(const char* name);
extern void Function48F650(
    W8MonsterInfo* monster_info, unsigned char value_1, unsigned char value_2);
extern unsigned char RemoveMonster(
    unsigned int monster_list_index, unsigned char destroy_monster);
extern const float g_monster_script_time_scale_005ec128;
extern const double g_monster_script_direction_step_005ed2b8;
extern const float g_monster_script_direction_scale_005ec150;
extern const double g_monster_facing_tolerance_005ec2b0;
extern const double g_monster_group_nearest_range_005ed2c0;
extern const char g_warning_missing_spell_vertex_0060f684[];
extern float BearingBetween(
    const srVector3T<float>* from, const srVector3T<float>* to);
extern unsigned char HasLineOfSightToBounds0046FD70(
    const srVector3T<float>* origin,
    srVector3T<float>* minimum,
    srVector3T<float>* maximum);
extern void Function577540();
extern void Function50F720(W8MonsterGroup* monster_group);
extern void* GetNPCItemListByID(int npc_record_id);
extern void Function56C5E0(
    void* item_list, int value_1, int value_2, int value_3, int value_4);
extern void ResetTargetSource(W8TargetSource* source);
extern void Function523C00(
    int monster_id, int value_2, int value_3, int value_4,
    W8TargetSource* source, int value_6);
extern srTextureIFace* LoadTexture004B9460(
    const char* path, unsigned char cached, unsigned char required);
extern unsigned char GetRenderOptionState(int option);
extern int NormalizeAttackMode(int attack_mode);
// GLOBAL: WIZ8 0x0060e614
unsigned char g_monster_gib_option_0060e614 = 1;
// GLOBAL: WIZ8 0x005ed280
extern const double g_monster_light_color_scale_005ed280 =
    0.00392156862745098;

static W8GrowableVector<stModelInstance005EC7D0*>
    g_monster_model_instances_682fd0;

#define MONSTER_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\Monster.cpp"

enum W8MonsterScriptCommand {
    MONSCR_GOTO,
    MONSCR_WALKTO,
    MONSCR_FACE,
    MONSCR_SAY,
    MONSCR_CYCLE,
    MONSCR_SHOOT,
    MONSCR_GIVE,
    MONSCR_TELEPORT,
    MONSCR_CAST,
    MONSCR_DIE,
    MONSCR_END,
    MONSCR_IF,
    MONSCR_ELSE,
    MONSCR_ENDIF,
    MONSCR_NPCINTERACTION,
    MONSCR_DISPOSITION,
    MONSCR_DISAPPEAR,
    MONSCR_LOOKHERE,
    MONSCR_NPCNUMBER,
    MONSCR_FOLLOW,
    MONSCR_PATROL,
    MONSCR_STOPPATROL,
    MONSCR_TRIGGER,
    MONSCR_DELAY,
    MONSCR_BEGINORDERS,
    MONSCR_ENDORDERS,
    MONSCR_GUARD,
    MONSCR_POINTPATROL,
    MONSCR_RANDOMPOINTPATROL,
    MONSCR_DEAF,
    MONSCR_DOACTION,
    MONSCR_EAST,
    MONSCR_NORTHEAST,
    MONSCR_NORTH,
    MONSCR_NORTHWEST,
    MONSCR_WEST,
    MONSCR_SOUTHWEST,
    MONSCR_SOUTH,
    MONSCR_SOUTHEAST,
    MONSCR_TURNTOFACEPARTY,
    MONSCR_LOOKABOUT,
    MONSCR_PLAY,
    MONSCR_FADEOUT,
    MONSCR_STAYHOME,
    MONSCR_COUNT
};

// GLOBAL: WIZ8 0x0060e958
const char* g_monster_script_commands[MONSCR_COUNT] = {
    "GOTO", "WALKTO", "FACE", "SAY", "CYCLE", "SHOOT", "GIVE",
    "TELEPORT", "CAST", "DIE", "END", "IF", "ELSE", "ENDIF",
    "NPCINTERACTION", "DISPOSITION", "DISAPPEAR", "LOOKHERE",
    "NPCNUMBER", "FOLLOW", "PATROL", "STOPPATROL", "TRIGGER", "DELAY",
    "BEGINORDERS", "ENDORDERS", "GUARD", "POINTPATROL",
    "RANDOMPOINTPATROL", "DEAF", "DOACTION", "EAST", "NORTHEAST",
    "NORTH", "NORTHWEST", "WEST", "SOUTHWEST", "SOUTH", "SOUTHEAST",
    "TURNTOFACEPARTY", "LOOKABOUT", "PLAY", "FADEOUT", "STAYHOME"
};

// VTABLE: WIZ8 0x005ecdac
// class W8GrowableVector<float>

// VTABLE: WIZ8 0x005ecdc8
// class W8GrowableVector<srVector3T<float> >

// TEMPLATE: WIZ8 0x004a2080
// W8GrowableVector<srVector3T<float> >::W8GrowableVector

// SYNTHETIC: WIZ8 0x004a2110
// W8GrowableVector<srVector3T<float> >::`scalar deleting destructor'

// VTABLE: WIZ8 0x005ed200
// class W8MonsterRep

/* Read the text-side monster representation. A named representation is shared
   by cloning its first live cycle; otherwise the MLS file supplies scalar
   movement settings, visual flags, sound/shake events, skin stages, lights,
   and the list of binary .mon cycles. */
// FUNCTION: WIZ8 0x004c0300
unsigned char MonsterReadAllCycles004C0300(
    const W8GrCycleLoadContext* context,
    const char* monster_name,
    W8Monster** monster,
    int load_value,
    int location_id)
{
    W8Monster* shared = static_cast<W8Monster*>(
        FindFirstGrCycleByName(monster_name));
    if (shared != 0) {
        *monster = new W8Monster(*shared);
        if (*monster == 0) {
            srAssertFail("*ppMonster", MONSTER_CPP, 0x4d3, 0);
        }
        (*monster)->RandomizeAppearanceAndMotion004C1D20();
        RegisterGrCycle(monster_name, *monster);
        return 1;
    }

    Function439BC0();

    unsigned char more = 1;
    unsigned char success = 1;
    unsigned char flies = 0;
    unsigned char swims = 0;
    unsigned char crawls = 0;
    unsigned char quadruped = 0;
    unsigned char full_transition = 0;
    unsigned char spice_monster = 0;
    int left_handed = 45;
    unsigned char has_light = 0;
    unsigned char light_pulsing = 0;
    unsigned char random_idle_range = 0;
    unsigned char has_lod_range = 0;
    float movement_rate = 3.0f;
    float rotation_rate = 0.7f;
    float scale_factor = 1.0f;
    float walk_radius = 0.0f;
    float fight_radius = 0.0f;
    float target_height = 0.0f;
    float camera_height = 0.0f;
    float scale_range_start = -1.0f;
    float scale_range_end = -1.0f;
    float hover_range_start = 0.0f;
    float hover_range_end = 0.0f;
    float bob_range_start = 0.0f;
    float bob_range_end = 0.0f;
    float idle_fps_start = 0.0f;
    float idle_fps_end = 0.0f;
    float lod_range_start = 0.0f;
    float lod_range_end = 0.0f;
    float opacity = -1.0f;
    float glow = -1.0f;
    float death_scale = 1.0f;
    float sound_falloff = -1.0f;
    float shadow_width = -1.0f;
    float shadow_depth = 0.0f;
    int missile_start = -1;
    int spell_start = -1;
    int footstep_volume = 0;
    int footstep_falloff = 0;
    srVector3T<float> light_first;
    srVector3T<float> light_second;
    W8VectorElement005ED094* last_sound = 0;
    int damage_stage = -1;
    int skin_stage = 0;

    char path[256];
    sprintf(path, "data\\Monsters\\%s.mls", monster_name);
    int handle = FileOpen(path, FILE_ACCESS_READ | FILE_OPEN_EXISTING, 0);
    if (handle == 0) {
        srAssertFail(
            "hFile", MONSTER_CPP, 0x50f,
            reinterpret_cast<char*>(String("Couldn't open %s", path)));
    }
    *monster = 0;

    if (handle == 0) {
        success = LoadGrCycle004A67E0(
            context, monster_name,
            reinterpret_cast<W8GrCycle**>(monster),
            -1, load_value, "data\\monsters", 0);
        if ((*monster)->m_plsParticles != 0) {
            for (int index = 0;
                 index < (*monster)->m_plsParticles->GetCount();
                 ++index) {
                W8GrCycleShakeEvent* event =
                    *(*monster)->m_plsParticles->GetAt(index);
                if (event->cycle_00 == -1 && event->subcycle_04 == -1) {
                    event->subcycle_04 =
                        (*monster)->m_pRep->selection.monster.current_subcycle;
                }
            }
        }
    }
    else {
        char line[250];
        while (more != 0) {
            ReadTextLine004CEE40(handle, line, sizeof(line), &more);
            while (line[0] == '\0' && more != 0) {
                ReadTextLine004CEE40(handle, line, sizeof(line), &more);
            }
            if (line[0] == '\0' || line[0] == '#') {
                continue;
            }

            char command[256];
            char argument[256];
            command[0] = '\0';
            argument[0] = '\0';
            sscanf(line, "%s %s", command, argument);

            if (_stricmp(command, "movementrate") == 0) {
                sscanf(line, "%s %f", command, &movement_rate);
            }
            else if (_stricmp(command, "RotationRate") == 0) {
                sscanf(line, "%s %f", command, &rotation_rate);
            }
            else if (_stricmp(command, "scalefactor") == 0) {
                sscanf(line, "%s %f", command, &scale_factor);
            }
            else if (_stricmp(command, "walkradius") == 0) {
                sscanf(line, "%s %f", command, &walk_radius);
            }
            else if (_stricmp(command, "fightradius") == 0) {
                sscanf(line, "%s %f", command, &fight_radius);
            }
            else if (_stricmp(command, "targetheight") == 0) {
                sscanf(line, "%s %f", command, &target_height);
            }
            else if (_stricmp(command, "cameraheight") == 0) {
                sscanf(line, "%s %f", command, &camera_height);
            }
            else if (_stricmp(command, "deathscale") == 0) {
                sscanf(line, "%s %f", command, &death_scale);
            }
            else if (_stricmp(command, "scalerangestart") == 0) {
                sscanf(line, "%s %f", command, &scale_range_start);
            }
            else if (_stricmp(command, "scalerangeend") == 0) {
                sscanf(line, "%s %f", command, &scale_range_end);
            }
            else if (_stricmp(command, "hoverrangestart") == 0) {
                sscanf(line, "%s %f", command, &hover_range_start);
            }
            else if (_stricmp(command, "hoverrangeend") == 0) {
                sscanf(line, "%s %f", command, &hover_range_end);
            }
            else if (_stricmp(command, "bobrangestart") == 0) {
                sscanf(line, "%s %f", command, &bob_range_start);
            }
            else if (_stricmp(command, "bobrangeend") == 0) {
                sscanf(line, "%s %f", command, &bob_range_end);
            }
            else if (_stricmp(command, "missilestart") == 0) {
                sscanf(line, "%s %d", command, &missile_start);
            }
            else if (_stricmp(command, "spellstart") == 0) {
                sscanf(line, "%s %d", command, &spell_start);
            }
            else if (_stricmp(command, "flies") == 0) {
                flies = 1;
            }
            else if (_stricmp(command, "swims") == 0) {
                swims = 1;
            }
            else if (_stricmp(command, "crawls") == 0) {
                crawls = 1;
            }
            else if (_stricmp(command, "quadruped") == 0) {
                quadruped = 1;
            }
            else if (_stricmp(command, "spicemonster") == 0) {
                spice_monster = 1;
            }
            else if (_stricmp(command, "randomidlefps") == 0) {
                sscanf(
                    line, "%s %f %f", command,
                    &idle_fps_start, &idle_fps_end);
                random_idle_range = 1;
            }
            else if (_stricmp(command, "loddistance") == 0) {
                sscanf(
                    line, "%s %f %f", command,
                    &lod_range_start, &lod_range_end);
                has_lod_range = 1;
            }
            else if (_stricmp(command, "pitch") == 0) {
                int pitch;
                if (last_sound == 0) {
                    srAssertFail(
                        "pSndEvent", MONSTER_CPP, 0x58e,
                        "mls pitch: Must specify a sound before pitch");
                }
                sscanf(line, "%s %d", command, &pitch);
                last_sound->value_014 = pitch;
            }
            else if (_stricmp(command, "volume") == 0) {
                if (last_sound == 0) {
                    srAssertFail(
                        "pSndEvent", MONSTER_CPP, 0x595,
                        "mls volume: Must specify a sound before volume");
                }
                sscanf(
                    line, "%s %d %d", command,
                    &last_sound->value_018, &last_sound->value_01c);
            }
            else if (_stricmp(command, "sound_falloff") == 0) {
                sscanf(line, "%s %f", command, &sound_falloff);
            }
            else if (_stricmp(command, "footstep_vol") == 0) {
                sscanf(
                    line, "%s %d %d", command,
                    &footstep_volume, &footstep_falloff);
            }
            else if (_stricmp(command, "probability") == 0) {
                int probability;
                if (last_sound == 0) {
                    srAssertFail(
                        "pSndEvent", MONSTER_CPP, 0x5a5,
                        "mls frequency: Must specify a sound before frequency");
                }
                sscanf(line, "%s %d", command, &probability);
                last_sound->value_024 = (unsigned char)probability;
            }
            else if (_stricmp(command, "animscript") == 0) {
            }
            else if (_stricmp(command, "script") == 0) {
                sscanf(line, "%s %s", command, argument);
                (*monster)->SetScript004C7F10(argument, 1);
            }
            else if (_stricmp(command, "opacity") == 0) {
                sscanf(line, "%s %f", command, &opacity);
            }
            else if (_stricmp(command, "glow") == 0) {
                sscanf(line, "%s %f", command, &glow);
            }
            else if (_stricmp(command, "shadow") == 0) {
                sscanf(
                    line, "%s %f %f", command,
                    &shadow_width, &shadow_depth);
            }
            else if (_stricmp(command, "lefthanded") == 0) {
                sscanf(line, "%s %d", command, &left_handed);
            }
            else if (_stricmp(command, "fulltransition") == 0) {
                full_transition = 1;
            }
            else if (_stricmp(command, "addlight") == 0) {
                char light_mode[256];
                sscanf(
                    line, "%*s %s ( %f %f %f ) ( %f %f %f )",
                    light_mode,
                    &light_first.x, &light_first.y, &light_first.z,
                    &light_second.x, &light_second.y, &light_second.z);
                light_first.x *= (float)g_monster_light_color_scale_005ed280;
                light_first.y *= (float)g_monster_light_color_scale_005ed280;
                light_first.z *= (float)g_monster_light_color_scale_005ed280;
                light_second.x *= (float)g_monster_light_color_scale_005ed280;
                light_second.y *= (float)g_monster_light_color_scale_005ed280;
                light_second.z *= (float)g_monster_light_color_scale_005ed280;
                light_pulsing = _stricmp(light_mode, "pulsing") == 0;
                has_light = 1;
            }
            else if (_stricmp(command, "skin") == 0) {
                if (damage_stage == -1) {
                    damage_stage = (*monster)->AddDamageStage004C6880(
                        monster_name, 0);
                    W8GrowableVector<stModelInstance005EC7D0*> instances;
                    (*monster)->CollectModelInstances004C6350(&instances);
                    for (int index = 0; index < instances.GetCount(); ++index) {
                        (*instances.GetAt(index))->damage_stage_184 =
                            damage_stage;
                    }
                    skin_stage = 0;
                }
                if (_stricmp(argument, "default") != 0) {
                    ++skin_stage;
                    damage_stage = (*monster)->AddDamageStage004C6880(
                        monster_name, skin_stage);
                }
            }
            else if (_stricmp(command, "skinswap") == 0) {
                char old_name[64];
                char new_name[64];
                sscanf(line, "%s %s %s", command, old_name, new_name);
                if (damage_stage != -1 &&
                    (*monster)->ReplaceSkinTexture004C6700(
                        damage_stage, old_name, new_name) == 0) {
                    Function401920(reinterpret_cast<const char*>(String(
                        "The skin texture %s not found in %s",
                        old_name, monster_name)));
                }
            }
            else {
                if (strlen(argument) <= 2) {
                    continue;
                }
                signed char subcycle;
                int cycle = ParseMonsterCycleName004C2010(command, &subcycle);
                if (cycle != -1) {
                    if (_strnicmp(argument, "gib", 3) != 0 ||
                        g_monster_gib_option_0060e614 != 0) {
                        if (GetRenderOptionState(0xe) == 0) {
                            cycle = NormalizeAttackMode(cycle);
                        }
                        if (*monster == 0 ||
                            (*monster)->IsCycleSupported((signed char)cycle) == 0 ||
                            GetRenderOptionState(0xe) != 0) {
                            float animation_scale = -1.0f;
                            sscanf(
                                line, "%s %s %f",
                                command, argument, &animation_scale);
                            success = LoadGrCycle004A67E0(
                                context, argument,
                                reinterpret_cast<W8GrCycle**>(monster),
                                cycle, load_value,
                                "data\\monsters", 0);
                            if ((*monster)->m_plsParticles != 0) {
                                for (int index = 0;
                                     index < (*monster)->m_plsParticles->GetCount();
                                     ++index) {
                                    W8GrCycleShakeEvent* event =
                                        *(*monster)->m_plsParticles->GetAt(index);
                                    if (event->cycle_00 == cycle &&
                                        event->subcycle_04 == -1) {
                                        event->subcycle_04 =
                                            (*monster)->m_pRep->selection.monster.current_subcycle;
                                    }
                                }
                            }
                            if (animation_scale > 0.0f) {
                                int current =
                                    (*monster)->m_pRep->selection.monster.current_subcycle;
                                W8AnimObj* animation =
                                    *(*monster)->m_pRep->animations[cycle].GetAt(current);
                                animation->playback_scale_08 = animation_scale;
                                *(*monster)->m_pRep->animation_scales[cycle].GetAt(current) =
                                    animation_scale;
                            }
                        }
                    }
                }
                else {
                    int sound_type = -1;
                    if (_stricmp(command, "SOUND_FRAME") == 0) sound_type = 1;
                    else if (_stricmp(command, "SOUND_CYCLE") == 0) sound_type = 2;
                    else if (_stricmp(command, "SOUND_FOOTSTEP") == 0) sound_type = 0x100;

                    if (sound_type != -1) {
                        char cycle_name[256];
                        char wave_name[256];
                        char loop_name[64];
                        int frame = 0;
                        cycle_name[0] = wave_name[0] = loop_name[0] = '\0';
                        if (sound_type == 0x100) {
                            sscanf(
                                line, "%s %s %d",
                                command, cycle_name, &frame);
                        }
                        else {
                            sscanf(
                                line, "%s %s %d %s %s",
                                command, cycle_name, &frame,
                                wave_name, loop_name);
                        }
                        int sound_cycle = ParseMonsterCycleName004C2010(
                            cycle_name, &subcycle);
                        char wave_path[256];
                        wave_path[0] = '\0';
                        if (sound_type != 0x100) {
                            sprintf(
                                wave_path,
                                "Data\\Sound\\Monsters\\%s.WAV",
                                wave_name);
                        }
                        last_sound = CreateSoundEvent004D57A0(
                            sound_type, sound_cycle, frame,
                            subcycle - 1, wave_path,
                            _stricmp(loop_name, "LOOP") == 0);
                        if (last_sound != 0) {
                            (*monster)->AddSoundEvent(last_sound);
                            last_sound->value_028 = location_id;
                            last_sound->value_018 =
                                sound_type == 0x100 ? 0x23 : 0x7f;
                            last_sound->value_01c = last_sound->value_018;
                            if (sound_falloff > 0.0f) {
                                last_sound->value_02c =
                                    (unsigned int)(sound_falloff * g_world_scale_005ebc40);
                            }
                            if (footstep_volume != 0 || footstep_falloff != 0) {
                                last_sound->value_030 = footstep_volume;
                                last_sound->value_034 = footstep_falloff;
                            }
                        }
                    }
                    else if (_stricmp(command, "SHAKE_FRAME") == 0) {
                        char cycle_name[256];
                        int frame;
                        float duration = 1.0f;
                        float intensity = 1.0f;
                        float value = 10.0f;
                        sscanf(
                            line, "%s %s %d %f %f %f",
                            command, cycle_name, &frame,
                            &duration, &intensity, &value);
                        int shake_cycle = ParseMonsterCycleName004C2010(
                            cycle_name, &subcycle);
                        W8CameraShakeEffect* effect = new W8CameraShakeEffect(
                            duration, 1, intensity,
                            (int)(value * g_world_scale_005ebc40), 0);
                        if (effect != 0) {
                            effect->frame_40 = frame;
                            effect->cycle_3c = shake_cycle;
                            effect->subcycle_44 = subcycle - 1;
                            (*monster)->AddShakeEffect004A8530(effect);
                        }
                    }
                    else {
                        srAssertFail(
                            "FALSE", MONSTER_CPP, 0x636,
                            FormatString(
                                "Monster::ReadAllCycles: ERROR - Unknown command %s in %s",
                                command, path));
                    }
                }
            }
        }
        CloseVirtualFile(handle);
    }

    W8MonsterRep* representation = (*monster)->m_pRep;
    delete[] representation->name_5c0;
    representation->name_5c0 = 0;
    if (monster_name != 0) {
        representation->name_5c0 = new char[strlen(monster_name) + 1];
        if (representation->name_5c0 != 0) {
            strcpy(representation->name_5c0, monster_name);
        }
    }
    if (scale_range_start != -1.0f && scale_range_end != -1.0f) {
        representation->minimum_scale_5f4 = scale_range_start;
        representation->maximum_scale_5f8 = scale_range_end;
        scale_factor =
            (scale_range_end - scale_range_start) *
                ((float)Random(1000) * 0.001f) +
            scale_range_start;
    }
    representation->scale_5f0 = scale_factor;
    representation->value_5fc = death_scale;

    if (walk_radius != 0.0f) {
        walk_radius *= g_world_scale_005ebc40;
        (*monster)->fields.movement_0c0.value_0b0 = walk_radius;
    }
    if (fight_radius != 0.0f) {
        fight_radius *= g_world_scale_005ebc40;
        (*monster)->fields.movement_0c0.alternate_radius_0b4 = fight_radius;
    }
    if (target_height != 0.0f) {
        target_height *= g_world_scale_005ebc40;
        if (target_height < 250.0f) target_height = 250.0f;
        (*monster)->fields.movement_0c0.height_offset_0b8 = target_height;
    }
    if (camera_height != 0.0f) {
        (*monster)->fields.movement_0c0.secondary_height_offset_0bc =
            camera_height * g_world_scale_005ebc40;
    }

    if (random_idle_range == 0) {
        idle_fps_start = -3.0f;
        idle_fps_end = 3.0f;
    }
    if (representation->animations[1].GetCount() < 1) {
        Function401920(reinterpret_cast<const char*>(String(
            "Monster %s: Missing CYCLE %s sub %d",
            representation->name_5c0, "IDLE", 0)));
    }
    W8AnimObj* idle = *representation->animations[1].GetAt(0);
    if (idle != 0) {
        representation->flag_600 = 1;
        representation->value_604 = idle->playback_scale_08;
        representation->value_608 = idle_fps_start;
        representation->value_60c = idle_fps_end;
    }
    if (missile_start > 0) (*monster)->value_1f4 = missile_start;
    if (spell_start > 0) (*monster)->value_1f8 = spell_start;
    if (has_lod_range != 0) {
        representation->lod_range_09c =
            lod_range_start * g_world_scale_005ebc40;
        representation->lod_range_0a0 =
            lod_range_end * g_world_scale_005ebc40;
    }
    if (opacity >= 0.0f && opacity < 1.0f) {
        (*monster)->scale_1cc = opacity;
    }
    if (glow > 0.0f) {
        W8GrowableVector<stModelInstance005EC7D0*> instances;
        (*monster)->CollectModelInstances004C6350(&instances);
        for (int index = 0; index < instances.GetCount(); ++index) {
            stModelInstance005EC7D0* instance = *instances.GetAt(index);
            instance->flag_1a1 = 1;
            instance->value_1a8 = glow;
        }
    }

    (*monster)->value_21c = hover_range_start;
    (*monster)->value_220 = hover_range_end;
    (*monster)->value_224 = bob_range_start;
    (*monster)->value_228 = bob_range_end;
    if (shadow_width != 0.0f) {
        if (shadow_width < 0.0f) {
            shadow_width =
                (*monster)->fields.movement_0c0.value_0b0 * 0.75f;
        }
        if (shadow_depth == 0.0f) shadow_depth = shadow_width;
        (*monster)->CreateGroundShadow(
            (int)(shadow_width * g_world_scale_005ebc40),
            (int)(shadow_depth * g_world_scale_005ebc40));
    }
    representation->value_610 = left_handed;
    representation->flag_601 =
        (flies != 0 || swims != 0 || full_transition != 0) ? 1 : 0;

    if (has_light != 0 && representation->monster_light_624 == 0) {
        representation->monster_light_624 = new MonsterLight(
            g_world->dynamic_scene,
            light_pulsing,
            (*monster)->fields.movement_0c0.value_0b0 * 0.1f,
            &light_first,
            &light_second);
        representation->monster_light_624->m_vertical_offset_228 =
            (*monster)->fields.movement_0c0.height_offset_0b8;
    }

    representation->selection.monster.current_subcycle = 0;
    (*monster)->SetCycle(1);
    (*monster)->SetSubCycle(0);
    srVector3T<float> minimum;
    srVector3T<float> maximum;
    (*monster)->GetAnimationBounds(&minimum, &maximum);
    (*monster)->SetBounds(&minimum, &maximum);
    if (movement_rate < 2.0f) movement_rate = 2.0f;
    (*monster)->SetValue120(movement_rate);
    (*monster)->SetTurnRate(
        rotation_rate * (float)g_double_005ec318);

    int navigation_mode = 1;
    if (flies != 0) navigation_mode = 2;
    else if (swims != 0) navigation_mode = 3;
    else if (quadruped != 0) navigation_mode = 5;
    else if (crawls != 0) navigation_mode = 6;
    (*monster)->SetNavigationMode(navigation_mode);
    if (spice_monster != 0) {
        (*monster)->flags_330.copied_flag_02 = 1;
        (*monster)->fields.owned_object_0a0 = 0;
        (*monster)->fields.movement_0c0.pitch_enabled_074 = 0;
    }

    (*monster)->RandomizeAppearanceAndMotion004C1D20();
    RegisterGrCycle(monster_name, *monster);
    Function439CA0();
    ReleaseReadMeshScratch004881D0();
    return success;
}

/* Select one of the four directional states without immediately repeating the
   caller's current state. Random values four and five fold back to zero in the
   original table. */
// FUNCTION: WIZ8 0x004c2e00
unsigned short ChooseDifferentMonsterDirection004C2E00(
    unsigned short previous_direction)
{
    unsigned short direction;

    for (;;) {
        direction = (unsigned short)Random(6);
        if (direction > 3) {
            direction = 0;
        }
        switch (previous_direction) {
        case 0:
            if (direction == 0) {
                continue;
            }
            break;
        case 1:
            if (direction == 1) {
                continue;
            }
            break;
        case 2:
            if (direction == 2) {
                continue;
            }
            break;
        case 3:
            if (direction == 3) {
                continue;
            }
            break;
        default:
            continue;
        }
        return direction;
    }
}

/* A loaded Monster receives independent appearance, animation-speed, and
   vertical-motion variation. The cycle-one animation and scale vectors are
   updated together so the representation and its cached scalar values remain
   synchronized. */
// FUNCTION: WIZ8 0x004c1d20
void W8Monster::RandomizeAppearanceAndMotion004C1D20()
{
    unsigned int random_value;

    unknown_1be = Random(100) < m_pRep->value_610;

    if (m_pRep->minimum_scale_5f4 != g_float_005ebb34 &&
        m_pRep->maximum_scale_5f8 != g_float_005ebb34) {
        float minimum = m_pRep->minimum_scale_5f4;
        float maximum = m_pRep->maximum_scale_5f8;

        if (minimum > maximum) {
            srAssertFail("flStart <= flEnd", MONSTER_CPP, 0x2f5, 0);
        }
        random_value = Random(1000);
        m_pRep->scale_5f0 =
            (maximum - minimum) * (float)random_value *
                g_monster_script_time_scale_005ec128 + minimum;
    }

    if (m_pRep->flag_600 != 0) {
        int subcycle;
        float playback_scale =
            (m_pRep->value_60c - m_pRep->value_608) *
                (float)Random(1000) * g_monster_script_time_scale_005ec128 +
            m_pRep->value_608;

        for (subcycle = 0;
             subcycle < (signed char)m_pRep->animations[1].GetCount();
             ++subcycle) {
            int animation_index = (signed char)subcycle;
            W8AnimObj* animation;
            float scale = playback_scale + m_pRep->value_604;

            if (animation_index == -1) {
                animation_index = m_pRep->selection.monster.current_subcycle;
            }
            if (animation_index >= m_pRep->animations[1].GetCount()) {
                Function401920(FormatString(
                    "Monster %s: Missing CYCLE %s subcycle %d",
                    m_pRep->name_5c0,
                    g_cycle_names[1].name,
                    animation_index));
            }
            if (animation_index < m_pRep->animations[1].GetCount()) {
                animation = *m_pRep->animations[1].GetAt(animation_index);
            }
            else {
                animation = *m_pRep->animations[1].GetAt(0);
            }
            if (scale < g_float_005ebb38) {
                scale = g_float_005ebb38;
            }
            animation->playback_scale_08 = scale;
            if (subcycle < m_pRep->animation_scales[1].GetCount()) {
                *m_pRep->animation_scales[1].GetAt(subcycle) = scale;
            }
        }
    }

    fields.movement_0c0.vertical_base_07c =
        ((value_220 - value_21c) * (float)Random(1000) *
             g_monster_script_time_scale_005ec128 +
         value_21c) *
        g_world_scale_005ebc40;
    fields.movement_0c0.vertical_amplitude_080 =
        ((value_228 - value_224) * (float)Random(1000) *
             g_monster_script_time_scale_005ec128 +
         value_224) *
        g_world_scale_005ebc40;
    fields.movement_0c0.vertical_phase_084 =
        (float)Random(1000) * g_monster_script_time_scale_005ec128;
    fields.movement_0c0.vertical_offset_0c0 =
        (float)sin(
            (double)fields.movement_0c0.vertical_phase_084 *
            g_double_005ec318) *
            fields.movement_0c0.vertical_amplitude_080 +
        fields.movement_0c0.vertical_base_07c;
    fields.movement_0c0.height_offset_0b8 +=
        fields.movement_0c0.vertical_base_07c;
    fields.movement_0c0.secondary_height_offset_0bc +=
        fields.movement_0c0.vertical_base_07c;

    if (scale_1cc < g_float_005ebb38) {
        m_pRep->value_05c = scale_1cc;
        m_pRep->flag_061 = 1;
    }
}

// FUNCTION: WIZ8 0x004C2010
int ParseMonsterCycleName004C2010(const char* name, signed char* subcycle)
{
    int cycle;

    if (name == 0) {
        srAssertFail("pacName", MONSTER_CPP, 1937, 0);
    }

    cycle = -1;
    for (int index = 0; index < W8_MONSTER_CYCLE_COUNT; ++index) {
        if (strncmp(
                name,
                g_cycle_names[index].name,
                g_cycle_names[index].prefix_length) == 0) {
            cycle = index;
            break;
        }
    }

    /* The model format retains these older names for the first two cycles. */
    if (cycle == -1) {
        if (strncmp(name, "FLY", 3) == 0) {
            cycle = 0;
        }
        else if (strncmp(name, "EXPLODE", 7) == 0) {
            cycle = 1;
        }
    }

    if (subcycle != 0) {
        *subcycle = 1;
        int suffix = g_cycle_names[cycle].prefix_length;
        if ((int)strlen(name) > suffix &&
            name[suffix] >= '0' && name[suffix] <= '9') {
            *subcycle = (signed char)atoi(name + suffix);
        }
    }
    return cycle;
}

// SYNTHETIC: WIZ8 0x004beba0
// W8MonsterRep::`scalar deleting destructor'

// FUNCTION: WIZ8 0x004bea20
W8MonsterRep::W8MonsterRep()
    : flag_5bc(0),
      name_5c0(0),
      linked_objects_5e8(0),
      value_5ec(0),
      scale_5f0(1.0f),
      minimum_scale_5f4(0.0f),
      maximum_scale_5f8(0.0f),
      value_5fc(1.0f),
      flag_600(0),
      flag_601(0),
      value_604(10.0f),
      value_608(0),
      value_60c(0),
      value_610(0),
      monster_light_624(0)
{
    int index;

    value_5c4 = 0;
    for (index = 0; index < 8; ++index) {
        objects_5c8[index] = 0;
    }
}

/* Read one animation/subcycle into the Monster representation.  The current
   subcycle is the newly appended animation slot; its playback scale and light
   list occupy the two parallel vectors for the same cycle. */
// FUNCTION: WIZ8 0x004BF520
unsigned char W8MonsterRep::ReadCycleData004BF520(
    W8GrCycleReadInfo004A6970* info,
    W8Monster* monster,
    int cycle_index,
    int value)
{
    W8LightVector* lights = new W8LightVector;
    W8AnimObj* animation;
    unsigned char success;
    signed char cycle;
    int subcycle;

    if (info == 0 || info->handle_04 == 0 || monster == 0) {
        srAssertFail(
            "pInfo && pInfo->hFile && pMonster",
            "C:\\Projects\\Wizardry 8\\Engine Code\\Monster.cpp",
            0x22d,
            0);
    }

    animation = CreateAnimObj004A01A0();
    success = AnimObjReadFromFile004A05C0(
        reinterpret_cast<W8ReadLevelInfo*>(info),
        animation,
        value,
        lights,
        0);
    if (cycle_index == -1) {
        cycle_index = static_cast<signed char>(animation->unknown_03[1]);
    }
    cycle = static_cast<signed char>(cycle_index);
    if (cycle < 0 || cycle >= W8_MONSTER_CYCLE_COUNT) {
        srAssertFail(
            "bCycle>=CYCLE_FIRST && bCycle<=CYCLE_LAST",
            "C:\\Projects\\Wizardry 8\\Engine Code\\Monster.cpp",
            0xc35,
            0);
    }

    animations[cycle].Add(0);
    selection.monster.current_subcycle =
        static_cast<signed char>(animations[cycle].GetCount() - 1);
    subcycle = selection.monster.current_subcycle;
    if (subcycle < animation_scales[cycle].GetCount()) {
        *animation_scales[cycle].GetAt(subcycle) = animation->playback_scale_08;
    }
    else {
        animation_scales[cycle].Add(animation->playback_scale_08);
    }

    active = 1;
    flag_06e = 1;
    m_bLOD = 2;
    timer_068 = g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT);
    flag_070 = animation->unknown_03[0];
    flag_06f = animation->value_02;
    flag_06d = animation->unknown_00[1];
    if (selection.monster.current_cycle == -1) {
        selection.monster.current_cycle = cycle;
    }
    if (subcycle < animations[cycle].GetCount()) {
        *animations[cycle].GetAt(subcycle) = animation;
    }

    if (AnimationIsRunning(animation) == 1) {
        signed char list;

        for (list = 0; list < 3; ++list) {
            signed char entry;
            signed char count = static_cast<signed char>(
                AnimObjListCount004A1620(animation, list));

            for (entry = 0; entry < count; ++entry) {
                W8PathAI* path = static_cast<W8PathAI*>(
                    AnimObjListEntry004A16C0(animation, list, entry));
                if (path != 0) {
                    PathAISetFlag38004AA9D0(path, 1);
                    PathAISetFlag1C004AAA10(path, 1);
                    PathAISetScale004AA9C0(path, animation->playback_scale_08);
                }
            }
        }
    }

    if (lights->GetCount() == 0) {
        delete lights;
        lights = 0;
    }
    else {
        monster->SetLights(lights);
    }
    light_lists[cycle].Add(lights);
    return success;
}

// FUNCTION: WIZ8 0x004bebd0
W8MonsterRep::W8MonsterRep(const W8MonsterRep& other)
    : W8EmitterHost(other),
      flag_5bc(other.flag_5bc),
      linked_objects_5e8(0),
      value_5ec(other.value_5ec),
      scale_5f0(other.scale_5f0),
      minimum_scale_5f4(other.minimum_scale_5f4),
      maximum_scale_5f8(other.maximum_scale_5f8),
      value_5fc(other.value_5fc),
      flag_600(other.flag_600),
      flag_601(other.flag_601),
      value_604(other.value_604),
      value_608(other.value_608),
      value_60c(other.value_60c),
      value_610(other.value_610),
      monster_light_624(0)
{
    signed char cycle;
    int index;

    value_5c4 = 0;
    for (index = 0; index < 8; ++index) {
        objects_5c8[index] = 0;
    }
    for (cycle = 0; cycle < W8_MONSTER_CYCLE_COUNT; ++cycle) {
        Method004BF0F0(cycle, &other, cycle);
    }
    if (other.monster_light_624 != 0) {
        monster_light_624 = new MonsterLight(*other.monster_light_624);
    }
    name_5c0 = 0;
    if (other.name_5c0 != 0) {
        name_5c0 = new char[strlen(other.name_5c0) + 1];
        if (name_5c0 != 0) {
            strcpy(name_5c0, other.name_5c0);
        }
    }
}

extern void DestroyAnimObj004A01E0(W8AnimObj* animation);
extern W8AnimObj* CloneAnimObj004A0320(const W8AnimObj* animation);

// FUNCTION: WIZ8 0x004bee50
W8MonsterRep::~W8MonsterRep()
{
    int cycle;
    int index;

    for (cycle = 0; cycle < W8_MONSTER_CYCLE_COUNT; ++cycle) {
        int count = animations[cycle].GetCount();
        for (index = 0; index < count; ++index) {
            W8AnimObj* animation = *animations[cycle].GetAt(index);
            if (animation != 0) {
                DestroyAnimObj004A01E0(animation);
            }
        }
    }
    for (index = 0; index < 8; ++index) {
        delete objects_5c8[index];
    }
    for (cycle = 0; cycle < W8_MONSTER_CYCLE_COUNT; ++cycle) {
        int count = light_lists[cycle].GetCount();
        for (index = 0; index < count; ++index) {
            DestroyLightVector(*light_lists[cycle].GetAt(index));
        }
        light_lists[cycle].Clear();
    }
    while (linked_runtime_objects_614.GetCount() != 0) {
        delete linked_runtime_objects_614.RemoveAt(0);
    }
    delete monster_light_624;
    delete[] name_5c0;
}

/* Deep-copy one cycle's animation objects and render lights while retaining
   its per-subcycle scalar values.  The light copies are new scene objects:
   they are registered with the world's light list and detached until the
   owning GrCycle selects this cycle. */
// FUNCTION: WIZ8 0x004bf0f0
void W8MonsterRep::Method004BF0F0(
    signed char cycle,
    const W8MonsterRep* other,
    signed char other_cycle)
{
    int index;

    for (index = 0; index < other->animations[other_cycle].GetCount(); ++index) {
        animations[cycle].Add(
            CloneAnimObj004A0320(*other->animations[other_cycle].GetAt(index)));
        animation_scales[cycle].Add(
            *other->animation_scales[other_cycle].GetAt(index));
    }

    for (index = 0; index < other->light_lists[other_cycle].GetCount(); ++index) {
        W8LightVector* source_lights =
            *other->light_lists[other_cycle].GetAt(index);
        W8LightVector* copied_lights = 0;

        if (source_lights != 0) {
            int light_index;

            copied_lights = new W8LightVector;
            if (copied_lights == 0) {
                srAssertFail(
                    "plsNewLights",
                    MONSTER_CPP,
                    0x1e5,
                    "Out of memory creating monster light list");
            }
            for (light_index = 0;
                 light_index < source_lights->GetCount();
                 ++light_index) {
                stLight* source_light = *source_lights->GetAt(light_index);
                float x = source_light->positionalX();
                float y = source_light->positionalY();
                float z = source_light->positionalZ();
                stLight* copied_light = new stLight;

                if (copied_light != 0) {
                    *copied_light = *source_light;
                }
                if (copied_light == 0) {
                    srAssertFail(
                        "pstNewLight",
                        MONSTER_CPP,
                        0x1ed,
                        "Out of memory creating monster light");
                }
                copied_light->ConfigureMonsterCopy();
                copied_light->setLocation(x, y, z);
                copied_light->setParent(0, 0);
                PLAdoptAppend(&g_world->m_list_0a8, copied_light);
                copied_lights->Add(copied_light);
            }
        }
        light_lists[cycle].Add(copied_lights);
    }
}

/* `new stLight` above is what forces this emission: VC6 inlines stLight's own
   empty default constructor at the allocation site but leaves the registry
   base's constructor out of line here. */
// TEMPLATE: WIZ8 0x004CA8B0
// srClassSupport<stLight,srLight,0,65542>::srClassSupport

/* The representation clone slot is an ordinary virtual copy operation.  The
   allocation size and call to the copy constructor are both visible in the
   emitted body; there is no separate representation wrapper involved. */
// FUNCTION: WIZ8 0x004ca9e0
W8AnimRepBase005EC1D8* W8MonsterRep::Clone()
{
    return new W8MonsterRep(*this);
}

extern "C" {
extern void Function4C4EF0(void);
extern void Function4A7A70(int value);
extern unsigned char g_flag_6081e4;
extern int g_value_659c14;
}

// VTABLE: WIZ8 0x005ed22c W8Monster
// VTABLE: WIZ8 0x005ed218 W8Navigator
// class W8Monster

// SYNTHETIC: WIZ8 0x004bfde0
// W8Monster::`scalar deleting destructor'

/* cvdump preserves a terminal space in this generated thunk's demangled name;
   the explicit name reference must preserve it too. */
// SYNTHETIC: WIZ8 0x004cae30
// W8Monster::`vector deleting destructor'`adjustor{24}' 

extern int g_monster_cycle_registry_weight_0065ba4c;
extern void PrepareMonsterCycleForDestruction004ACF90(
    W8Monster* cycle);
extern unsigned char Function420E10(void);
extern unsigned char g_flag_00689b32;

// FUNCTION: WIZ8 0x004bfb00
W8Monster::W8Monster()
{
    memset(&value_1e0, 0, 0x58);
    memset(&state_28c, 0, sizeof(state_28c));
    memset(&state_2ac, 0, sizeof(state_2ac));
    memset(&state_2fc, 0, sizeof(state_2fc));
    memset(&flags_330, 0, sizeof(flags_330));

    flags_1dc = 0;
    value_1e0 = -1;
    propagated_value_1e4 = -1;
    value_1e8 = 1.0f;
    value_1ec = 1.0f;
    value_1f0 = 1.0f;
    value_210 = -1;
    script_238 = 0;
    script_line_23c = 0;
    script_wait_240 = -1;
    trigger_278 = 0;
    registry_weight_27c = 0;
    formation.x = 0.0f;
    formation.y = 0.0f;
    formation.z = 0.0f;
    sound_334 = 0;

    m_pRep = new W8MonsterRep;
    m_pRep->linked_objects_5e8 = PLCreate();
}

// FUNCTION: WIZ8 0x004bfe00
W8Monster::W8Monster(const W8Monster& rhs)
    : W8GrCycle(rhs),
      flags_1dc(rhs.flags_1dc),
      value_1e0(rhs.value_1e0),
      propagated_value_1e4(rhs.propagated_value_1e4),
      value_1e8(1.0f),
      value_1ec(1.0f),
      value_1f0(1.0f),
      value_1f4(rhs.value_1f4),
      value_1f8(rhs.value_1f8),
      flag_1fc(0),
      flag_1fd(0),
      value_200(0),
      value_204(0),
      value_208(0),
      value_210(-1),
      flag_215(rhs.flag_215),
      flag_216(1),
      flag_217(0),
      flag_218(0),
      value_21c(rhs.value_21c),
      value_220(rhs.value_220),
      value_224(rhs.value_224),
      value_228(rhs.value_228),
      flag_22c(0),
      flag_22d(0),
      script_238(0),
      script_wait_240(-1),
      trigger_278(0),
      registry_weight_27c(rhs.registry_weight_27c),
      sound_334(0)
{
    formation.x = 0.0f;
    formation.y = 0.0f;
    formation.z = 0.0f;
    flags_330.flag_00 = 0;
    flags_330.flag_01 = 0;
    flags_330.copied_flag_02 = rhs.flags_330.copied_flag_02;

    if (rhs.m_pRep == 0) {
        srAssertFail(
            "rhs.m_pRep",
            "C:\\Projects\\Wizardry 8\\Engine Code\\Monster.cpp",
            1099,
            0);
    }
    m_pRep = static_cast<W8MonsterRep*>(rhs.m_pRep->Clone());
    m_pRep->linked_objects_5e8 = PLCreate();

    state_28c.defining_orders = 0;
    state_28c.orders_finished = 0;
    state_28c.order_mode = -1;
    state_28c.deaf = 0;
    state_28c.face_party = 0;
    state_28c.stay_home = 0;
    state_2ac.flag_00 = 0;
    state_2ac.direction_x = 0;
    state_2ac.direction_y = 0;
    state_2ac.direction_z = 0;
    state_2ac.look_frequency = 0;
    state_2ac.look_duration = 0;
    state_2ac.value_24 = -1;
    state_2ac.flag_28 = 1;
    state_2fc.scale_00 = 1.0f;
    state_2fc.scale_04 = 1.0f;
    flags_1dc &= 0xfffffcb6;
    state_22e = 0;
    cycle_callback_230 = 0;
    if (flags_330.copied_flag_02 != 0) {
        fields.state_088 = 0;
    }
}

// FUNCTION: WIZ8 0x004c0170
W8Monster::~W8Monster()
{
    SetLights(0);
    if (m_pRep->linked_objects_5e8 != 0) {
        PrepareMonsterCycleForDestruction004ACF90(this);
        PLDestroy(m_pRep->linked_objects_5e8);
    }
    if (IsSoleGrCycleForName(this)) {
        g_monster_cycle_registry_weight_0065ba4c -= registry_weight_27c;
        RemoveCycleSkinTables004C6B10();
    }
    UnregisterGrCycle(this);
    delete m_pRep;
    if (script_238 != 0) {
        script_238->release();
        script_238 = 0;
    }
    flags_1dc &= ~0x20;
    script_line_23c = 0;
    script_wait_240 = -1;
    if (sound_334 != 0) {
        sound_334->release();
        sound_334 = 0;
    }
}

/* Navigator is W8Monster's second base at +0x18. VC6 places this override in
   that secondary table and emits the adjusted entry form at 0x004CA840. */
// FUNCTION: WIZ8 0x004ca840
void W8Monster::SetPosition(const srVector3T<float>* position)
{
    GetRepresentation()->SetLocation004B8850(position);
    m_pRep->SetLocation004B8850(position);
    SetPositionInternal00453590(position);
    fields.position_dirty_09c = 1;
}

/* Advance the non-rendering half of one live Monster. This is the main
   Monster.cpp update slot: it maintains distance-driven model scale, the
   sunlight/ground-shadow transition, Navigator state, pending animation
   cycles, attached objects, and the optional scene node. Rendering remains in
   UpdateRepresentation. */
// FUNCTION: WIZ8 0x004c2100
void W8Monster::Update()
{
    srVector3T<float> party_position;
    srVector3T<float> monster_position;
    W8MonsterInfo* monster_info = 0;
    float dx;
    float dy;
    float dz;
    float distance;
    int cycle;
    int index;

    if (m_pRep == 0) {
        srAssertFail("m_pRep", MONSTER_CPP, 0x7f9, 0);
    }

    GetCameraPosition(&party_position);
    {
        srVector3T<float> position = GetPosition();
        monster_position.x = position.x;
        monster_position.y = position.y;
        monster_position.z = position.z;
    }
    dx = monster_position.x - party_position.x;
    dy = monster_position.y - party_position.y;
    dz = monster_position.z - party_position.z;
    distance = (float)sqrt(dx * dx + dy * dy + dz * dz);

    if (state_2ac.value_24 == -1 ||
        g_light_scale_0060bfe0 < g_float_005ebb38) {
        state_2fc.scale_04 = 0.75f;
        g_monster_model_instances_682fd0.Clear();
        CollectModelInstances004C6350(&g_monster_model_instances_682fd0);
        for (index = 0;
             index < g_monster_model_instances_682fd0.GetCount();
             ++index) {
            stModelInstance005EC7D0* model =
                *g_monster_model_instances_682fd0.GetAt(index);
            model->scale_194.x = 0.75f;
            model->scale_194.y = 0.75f;
            model->scale_194.z = 0.75f;
        }
        state_2fc.scale_00 = 0.75f;
        timer_2d8.SetDuration(0.025f);
        timer_2d8.Restart();
        state_2ac.value_24 = 1;
        state_2ac.flag_28 = 1;
    }

    if (g_monster_shadow_updates_enabled_0065970c != 0 &&
        (state_2ac.flag_28 != 0 || UpdateTrackedPosition00454950() != 0)) {
        state_2ac.flag_28 = 0;
        if (distance < WorldGetValue78(g_world)) {
            srNode* sun = static_cast<srNode*>(
                srCore.getRegistry()->find(
                    g_world->dynamic_scene->getClassNode(), "SUN", 0));
            if (sun != 0) {
                srVector3T<float> mapped_position;
                srVector3T<double> sun_location = sun->getLocation();
                srVector3T<float> sun_position;

                GetMappedPosition004C72A0(&mapped_position);
                sun_position.x = (float)sun_location.x;
                sun_position.y = (float)sun_location.y;
                sun_position.z = (float)sun_location.z;
                if (g_octree_6598a4->HasLineOfSight(
                        &mapped_position, &sun_position, 1)) {
                    if (state_2ac.value_24 == 0) {
                        state_2fc.scale_00 = 0.75f;
                        timer_2d8.SetDuration(0.025f);
                        timer_2d8.Restart();
                        state_2ac.value_24 = 1;
                    }
                }
                else if (state_2ac.value_24 == 1) {
                    state_2fc.scale_00 = 0.0f;
                    timer_2d8.SetDuration(0.025f);
                    timer_2d8.Restart();
                    state_2ac.value_24 = 0;
                }
            }
        }
    }

    if (state_2fc.scale_04 != state_2fc.scale_00 &&
        timer_2d8.GetProgress() >= g_float_005ebb38) {
        if (state_2fc.scale_00 <= state_2fc.scale_04) {
            state_2fc.scale_04 -= g_monster_scale_transition_step_005ebcf4;
            if (state_2fc.scale_04 < state_2fc.scale_00) {
                state_2fc.scale_04 = state_2fc.scale_00;
            }
        }
        else {
            state_2fc.scale_04 += g_monster_scale_transition_step_005ebcf4;
            if (state_2fc.scale_04 > state_2fc.scale_00) {
                state_2fc.scale_04 = state_2fc.scale_00;
            }
        }
        g_monster_model_instances_682fd0.Clear();
        CollectModelInstances004C6350(&g_monster_model_instances_682fd0);
        for (index = 0;
             index < g_monster_model_instances_682fd0.GetCount();
             ++index) {
            stModelInstance005EC7D0* model =
                *g_monster_model_instances_682fd0.GetAt(index);
            model->scale_194.x = state_2fc.scale_04;
            model->scale_194.y = state_2fc.scale_04;
            model->scale_194.z = state_2fc.scale_04;
        }
        timer_2d8.Restart();
    }

    if (flags_330.flag_00 != 0) {
        float progress = timer_30c.GetProgress();
        if (progress > g_float_005ebb38) {
            progress = g_float_005ebb38;
        }
        if (flags_330.flag_00 == -2) {
            if (progress == g_float_005ebb38) {
                flags_330.flag_00 = 0;
                timer_30c.SetDuration(3.0f);
                timer_30c.Restart();
                if ((signed char)flags_330.flag_00 < 1) {
                    m_pRep->value_05c = g_float_005ebb38;
                    m_pRep->flag_061 = 1;
                    flags_330.flag_00 = -1;
                }
                else {
                    timer_30c.SetProgress(
                        g_float_005ebb38 - m_pRep->value_05c);
                    flags_330.flag_00 = -1;
                }
            }
        }
        else {
            if ((signed char)flags_330.flag_00 < 1) {
                m_pRep->value_05c =
                    g_float_005ebb38 - progress;
            }
            else {
                m_pRep->value_05c = progress;
            }
            m_pRep->flag_061 = 1;
            if (progress == g_float_005ebb38) {
                if ((signed char)flags_330.flag_00 < 0) {
                    flags_1dc |= 0x400;
                }
                flags_330.flag_00 = 0;
            }
        }
    }

    cycle = Query(6);
    SetGroundShadowVisible(
        cycle != 0 && cycle != 0x15 && flags_330.flag_00 == 0 &&
        (flags_1dc & 0x400) == 0);

    {
        unsigned int monster_index = MonsterGetIndexByLocationID(
            0x86a, MONSTER_CPP, propagated_value_1e4, 0);
        if (monster_index != 0xffffffff) {
            monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
        }
    }

    if (GetFlag68F105() == 0) {
        if ((cycle == 1 || cycle == 2) &&
            m_pRep->selection.monster.pending_cycle == -1 &&
            fields.flag_024 == 0 && fields.flag_025 == 0) {
            fields.flags_00c |= 0x100000;
        }

        if (monster_info == 0) {
            UpdateNavigation004553A0(0, 0);
        }
        else {
            UpdateNavigation004553A0(
                monster_info->value_107 >= 0x0e,
                monster_info->condition_turns[5] != 0);
        }

        if (cycle != 0x15 && script_238 != 0 &&
            g_in_combat_00683f94 == 0) {
            ProcessScript004C80E0();
            cycle = Query(6);
        }

        if ((flags_1dc & 0x20) == 0 &&
            m_pRep->selection.monster.pending_cycle == -1) {
            switch (cycle) {
            case 0:
                if (Query(7) != 0) {
                    m_pRep->selection.monster.pending_cycle = 1;
                    m_pRep->active = 1;
                    m_pRep->timer_068 = g_shared_timer_base->getUTime(
                        srTimer::TIMER_READ_DEFAULT);
                    m_pRep->behaviour_071 = 3;
                    m_pRep->value_066 = 0;
                }
                break;
            case 1:
            case 2:
                if (fields.flag_024 == 0 && fields.flag_025 == 0 &&
                    (Query(2) != 0 || unknown_1bc != 0)) {
                    fields.flags_00c &= ~0x100000;
                    if (IsCycleSupported(3) == 0) {
                        m_pRep->behaviour_071 = 3;
                        m_pRep->selection.monster.pending_cycle = 4;
                    }
                    else {
                        m_pRep->selection.monster.pending_cycle = 3;
                        m_pRep->flag_06e = 1;
                        m_pRep->behaviour_071 = 1;
                        m_pRep->value_066 = 0;
                    }
                    m_pRep->active = 1;
                    m_pRep->timer_068 = g_shared_timer_base->getUTime(
                        srTimer::TIMER_READ_DEFAULT);
                }
                break;
            case 3:
                if (Query(7) != 0) {
                    if (m_pRep->flag_06e == 3) {
                        m_pRep->flag_06e = 1;
                        m_pRep->selection.monster.pending_cycle = 1;
                    }
                    else {
                        m_pRep->selection.monster.pending_cycle = 4;
                    }
                    m_pRep->active = 1;
                    m_pRep->timer_068 = g_shared_timer_base->getUTime(
                        srTimer::TIMER_READ_DEFAULT);
                    m_pRep->behaviour_071 = 3;
                    m_pRep->value_066 = 0;
                }
                break;
            case 4:
                if (fields.flag_024 != 0 || fields.flag_025 != 0) {
                    bool transition = Query(2) != 0 || unknown_1bc != 0;
                    if (!transition && m_pRep->flag_601 == 0) {
                        transition = Query(4) < Query(0) / 2;
                    }
                    if (transition) {
                        while (values_338.GetCount() != 0) {
                            SoundStop(*values_338.GetAt(0));
                            values_338.RemoveAt(0);
                        }
                        if (IsCycleSupported(3) == 0) {
                            m_pRep->behaviour_071 = 3;
                            m_pRep->selection.monster.pending_cycle = 1;
                        }
                        else {
                            m_pRep->selection.monster.pending_cycle = 3;
                            m_pRep->flag_06e = 3;
                            m_pRep->behaviour_071 = 1;
                            m_pRep->value_066 = (unsigned short)(Query(0) - 1);
                            flags_1dc |= 1;
                        }
                        m_pRep->active = 1;
                        m_pRep->timer_068 = g_shared_timer_base->getUTime(
                            srTimer::TIMER_READ_DEFAULT);
                    }
                }
                break;
            case 0x17:
                if (Query(7) != 0) {
                    if ((signed char)fields.flags_00c != 0) {
                        *reinterpret_cast<unsigned int*>(
                            &fields.movement_target_018.z) = 0x17;
                        *reinterpret_cast<unsigned int*>(
                            &fields.movement_target_018.x) = GetTickCount();
                        *reinterpret_cast<unsigned int*>(&fields.movement_target_018.y) = Random(2000) + 2000;
                        m_pRep->selection.monster.pending_cycle = 0x18;
                        m_pRep->flag_06e = 1;
                        m_pRep->behaviour_071 = 1;
                    }
                    else {
                        m_pRep->selection.monster.pending_cycle = 1;
                    }
                    m_pRep->active = 1;
                    m_pRep->timer_068 = g_shared_timer_base->getUTime(
                        srTimer::TIMER_READ_DEFAULT);
                    m_pRep->value_066 = 0;
                }
                break;
            case 0x18:
                if (Query(7) != 0) {
                    if (*reinterpret_cast<unsigned int*>(&fields.movement_target_018.y) <
                            GetTickCount() - *reinterpret_cast<unsigned int*>(&fields.movement_target_018.x) &&
                        IsCycleSupported(0x17) != 0) {
                        m_pRep->selection.monster.pending_cycle = 0x17;
                    }
                    else {
                        m_pRep->selection.monster.pending_cycle = 0x18;
                    }
                    m_pRep->flag_06e = 1;
                    m_pRep->behaviour_071 = 1;
                    m_pRep->active = 1;
                    m_pRep->timer_068 = g_shared_timer_base->getUTime(
                        srTimer::TIMER_READ_DEFAULT);
                    m_pRep->value_066 = 0;
                }
                break;
            case 0x19:
                if (Query(7) != 0) {
                    m_pRep->selection.monster.pending_cycle = 1;
                    m_pRep->flag_06e = 1;
                    m_pRep->active = 1;
                    m_pRep->timer_068 = g_shared_timer_base->getUTime(
                        srTimer::TIMER_READ_DEFAULT);
                    m_pRep->behaviour_071 = 3;
                    m_pRep->value_066 = 0;
                }
                break;
            }
        }
    }

    if (m_pRep->active == 0) {
        return;
    }

    UpdateAttachedObjects004C3F70();
    cycle = Query(6);
    if (g_monster_combat_timer_enabled_006f0531 != 0 &&
        g_combat_state != 0 &&
        (g_combat_state->flag_001 != 0 || g_flag_00683fce != 0) &&
        (cycle == 1 || cycle == 2) &&
        (m_pRep->selection.monster.pending_cycle == -1 ||
         m_pRep->selection.monster.pending_cycle == 1 ||
         m_pRep->selection.monster.pending_cycle == 2) &&
        (g_combat_state->selected_slot != 2 ||
         g_combat_state->selected_monster == 0 ||
         g_combat_state->selected_monster->location_id != propagated_value_1e4)) {
        m_pRep->timer_068 = g_shared_timer_base->getUTime(
            srTimer::TIMER_READ_DEFAULT);
    }

    if (monster_info != 0 && monster_info->condition_turns[5] != 0) {
        TickAnimation(
            Query(6) == 4
                ? fields.movement_0c0.movement_speed_064 *
                      g_float_005ebc7c
                : 0.5f);
    }
    else {
        TickAnimation(
            Query(6) == 4 ? fields.movement_0c0.movement_speed_064 : 1.0f);
    }
    InitializeAnimatedTexture004C51D0();

    if (sound_334 != 0) {
        srVector3T<float> position = GetPosition();
        srVector3T<double> location;
        location.x = position.x;
        location.y = position.y;
        location.z = position.z;
        sound_334->setLocation(location);
    }
}

/* Script IF expressions are deliberately small: a name followed by an
   optional comma-separated float list. The retail interpreter supports the
   two predicates below and treats every other name as false. */
// FUNCTION: WIZ8 0x004C9DC0
unsigned char W8Monster::EvaluateScriptCondition004C9DC0(
    const char* expression)
{
    W8GrowableVector<float> parameters;
    char buffer[512] = {0};
    char* argument_list;
    char* argument;

    if (expression == 0) {
        srAssertFail("pEvalVar", MONSTER_CPP, 7503, 0);
    }
    if (strlen(expression) >= sizeof(buffer)) {
        srAssertFail("strlen(pEvalVar) < 512", MONSTER_CPP, 7504, 0);
    }
    strcpy(buffer, expression);

    argument_list = strchr(buffer, '(');
    if (argument_list != 0) {
        argument = strtok(argument_list + 1, ",)");
        while (argument != 0) {
            parameters.Add((float)atof(argument));
            argument = strtok(0, ",)");
        }
        *argument_list = 0;
    }

    if (_strnicmp(buffer, "PARTYNEAR", 9) == 0) {
        srVector3T<float> party_position;
        srVector3T<float> monster_position;

        GetCameraPosition(&party_position);
        if (parameters.GetCount() == 0) {
            srAssertFail(
                "lsParmList.Length()",
                MONSTER_CPP,
                7526,
                FormatString(
                    "Monscr %s line %d: PARTYNEAR expects a parameter",
                    script_238->getName(),
                    script_line_23c));
        }

        srVector3T<float> current_position = GetPosition();
        monster_position.x = current_position.x;
        monster_position.y = current_position.y;
        monster_position.z = current_position.z;
        float dx = party_position.x - monster_position.x;
        float dy = party_position.y - monster_position.y;
        float dz = party_position.z - monster_position.z;
        if (g_force_encounter_culling == 0 &&
            sqrt(dx * dx + dy * dy + dz * dz) <
                *parameters.GetAt(0) *
                    g_world_scale_005ebc40 &&
            MonsterInfoFromID(
                7533, MONSTER_CPP, propagated_value_1e4, 1)
                    ->unknown_358[0x18] != 0) {
            return 1;
        }
    }
    else if (_strnicmp(buffer, "RANDOM", 6) == 0) {
        if (parameters.GetCount() == 0) {
            srAssertFail(
                "lsParmList.Length()",
                MONSTER_CPP,
                7542,
                FormatString(
                    "Monscr %s line %d: RANDOM expects a parameter",
                    script_238->getName(),
                    script_line_23c));
        }
        if (Chance((unsigned int)*parameters.GetAt(0)) != 0) {
            return 1;
        }
    }
    return 0;
}

/* Replace the current Monster script with an existing named runtime object or
   load it from the Monster script directory on first use. Registry ownership
   is balanced the same way on both paths: a newly loaded script is marked for
   automatic release, then the Monster takes its own reference. */
// FUNCTION: WIZ8 0x004C7F10
unsigned char W8Monster::SetScript004C7F10(
    const char* script_name, unsigned char reset_orders)
{
    srRegistry* registry;
    char path[256] = "Data\\Monsters\\Scripts\\";

    if (script_238 != 0) {
        script_238->release();
        script_238 = 0;
    }
    flags_1dc &= ~0x20;
    script_line_23c = 0;
    script_wait_240 = -1;
    if (sound_334 != 0) {
        sound_334->release();
        sound_334 = 0;
    }
    if (reset_orders != 0) {
        state_28c.orders_finished = 0;
    }

    registry = srCore.getRegistry();
    script_238 = static_cast<stScript*>(registry->find(
        stScript::sGetClassNode(), script_name, 0));
    if (script_238 == 0) {
        strcat(path, script_name);
        script_238 = new stScript;
        if (script_238 != 0) {
            if (script_238->Load004CF3B0(path) != 0) {
                script_238->setName(script_name);
                script_238->autoRelease();
            }
            else {
                script_238->release();
                script_238 = 0;
            }
        }
    }
    if (script_238 == 0) {
        return 0;
    }
    script_238->addReference();
    return 1;
}

// FUNCTION: WIZ8 0x004C77F0
unsigned char W8Monster::GetProjectilePosition004C77F0(
    srVector3T<float>* position)
{
    signed char cycle;
    unsigned char result;

    if (position == 0) {
        return 0;
    }
    if (IsCycleSupported(0x11) != 0 && IsCycleSupported(0x0d) != 0) {
        srAssertFail(
            "!(IsCycleSupported(CYCLE_ATTACK_1) && "
            "IsCycleSupported(CYCLE_ATTACK_2))",
            MONSTER_CPP, 0x18b1, 0);
    }
    if (IsCycleSupported(0x0d) != 0) {
        cycle = 0x0d;
    }
    else if (IsCycleSupported(0x11) != 0) {
        cycle = 0x11;
    }
    else if (IsCycleSupported(7) != 0) {
        cycle = 7;
    }
    else {
        return 0;
    }

    result = GetCycleMappedPosition004C7960(cycle, 5, position);
    if (result == 0 && flag_22d == 0) {
        if (g_flag_00689b32 != 0) {
            W8MonsterInfo* info = MonsterGetScriptPartByLocationIndex(
                MonsterGetIndexByLocationID(
                    0x18c3, MONSTER_CPP, propagated_value_1e4, 1));
            FormatDebugMessage(
                0,
                "WARNING: %ls does not have a MISSILE_START_POINT defined",
                GetMonsterDataForInfo(info));
        }
        flag_22d = 1;
    }
    return result;
}

/* Cycle 25 mapping six is the spell launch vertex. Missing mappings warn at
   most once per Monster, while the returned byte remains the mapping result. */
// FUNCTION: WIZ8 0x004c78e0
unsigned char W8Monster::GetSpellPosition004C78E0(srVector3T<float>* position)
{
    unsigned char found;

    if (position == 0) {
        return 0;
    }
    found = GetCycleMappedPosition004C7960(0x19, 6, position);
    if (found == 0 && flag_22c == 0) {
        if (g_flag_00689b32 != 0) {
            W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(
                MonsterGetIndexByLocationID(
                    0x18e4, MONSTER_CPP, propagated_value_1e4, 1));
            W8MonsterRecord* record = GetMonsterDataForInfo(monster_info);
            FormatDebugMessage(
                0, g_warning_missing_spell_vertex_0060f684, record);
        }
        flag_22c = 1;
    }
    return found;
}

// FUNCTION: WIZ8 0x004C7960
unsigned char W8Monster::GetCycleMappedPosition004C7960(
    signed char cycle, int mapped_index, srVector3T<float>* position)
{
    srModelInstance* current_model = GetCurrentModelInstance004A8250();
    W8MonsterAnimationVector* animations;
    W8AnimObj* animation;
    int subcycle;
    int dispatch_value;

    if (cycle == -1) {
        cycle = m_pRep->selection.monster.current_cycle;
        subcycle = m_pRep->selection.monster.current_subcycle;
    }
    else {
        subcycle = 0;
    }
    animations = &m_pRep->animations[cycle];
    if (animations->GetCount() <= subcycle) {
        Function401920(FormatString(
            "Monster %s: Missing CYCLE %s subcycle %d",
            m_pRep->name_5c0, g_cycle_names[cycle].name, subcycle));
    }
    animation = *animations->GetAt(subcycle);

    if (mapped_index == 5) {
        dispatch_value = (int)flags_1dc;
    }
    else if (mapped_index == 6) {
        dispatch_value = fields.navigation_mode_008;
    }
    else {
        dispatch_value = 0;
    }
    if (animation == 0) {
        return 0;
    }
    if (AnimationIsRunning(animation) != 0) {
        return GetAnimationCenter(position);
    }

    srModelInstance* model =
        AnimObjDispatch004A14D0(animation, 2, dispatch_value);
    if (model != 0) {
        stMeshModel* mesh = static_cast<stMeshModel*>(model->model());
        int vertex = FindMappedIndexInMeshChain(&mesh, mapped_index);
        if (mesh != 0 && vertex != -1) {
            srVector3T<float>* vertices =
                mesh->GetVertexLocations00471AD0(dispatch_value, 1, 0.0f);
            if (vertices != 0) {
                srMatrix4T<float> matrix;
                srVector3T<float> owner_position = GetPosition();
                float x = vertices[vertex].x * m_pRep->scale_5f0;
                float y = vertices[vertex].y * m_pRep->scale_5f0;
                float z = vertices[vertex].z * m_pRep->scale_5f0;

                current_model->getWorldSpaceMatrix(matrix);
                position->x = matrix.vectors[0].x * x +
                              matrix.vectors[0].y * y +
                              matrix.vectors[0].z * z + owner_position.x;
                position->y = matrix.vectors[1].x * x +
                              matrix.vectors[1].y * y +
                              matrix.vectors[1].z * z + owner_position.y +
                              fields.movement_0c0.vertical_base_07c;
                position->z = matrix.vectors[2].x * x +
                              matrix.vectors[2].y * y +
                              matrix.vectors[2].z * z + owner_position.z;
                return 1;
            }
        }
    }
    return 0;
}

/* Execute source lines until a command starts an asynchronous operation, ends
   the script, or the runaway-command guard trips. The two command modes share
   the original table: normal mode performs actions, while BEGINORDERS records
   the persistent movement policy that the Navigator update consumes. */
// FUNCTION: WIZ8 0x004C80E0
void W8Monster::ProcessScript004C80E0()
{
    W8MonsterInfo* monster_info;
    unsigned int monster_index;
    int command_count;
    unsigned char stop;

    if (script_238 == 0) {
        return;
    }

    monster_index = MonsterGetIndexByLocationID(
        0x1a4a, MONSTER_CPP, propagated_value_1e4, 1);
    monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
    if (state_28c.orders_finished != 0) {
        if (monster_info == 0 || (monster_info->flag_255 & 0x10) == 0) {
            return;
        }
        monster_info->flag_255 &= ~0x10;
        return;
    }

    if (monster_info != 0 && (monster_info->flag_255 & 0x10) != 0) {
        monster_info->flag_255 &= ~0x10;
        if (script_wait_240 == MONSCR_WALKTO && fields.flag_024 != 0) {
            if (script_line_23c > 0) {
                --script_line_23c;
            }
        }
        else if (CanContinueScript004CA0F0() == 0) {
            return;
        }
    }
    else if (CanContinueScript004CA0F0() == 0) {
        return;
    }

    script_wait_240 = -1;
    stop = 0;
    command_count = 0;
    while (script_238 != 0 && script_wait_240 == -1 && stop == 0) {
        char line[256];
        char* token;
        int command;
        stScriptLine* source_line;

        if (script_line_23c >= script_238->lines.GetCount()) {
            break;
        }
        source_line = *script_238->lines.GetAt(script_line_23c++);
        if (source_line == 0 || source_line->text == 0) {
            continue;
        }
        strcpy(line, source_line->text);
        token = strtok(line, " \t");
        if (token == 0) {
            continue;
        }

        command = -1;
        for (int index = 0; index < MONSCR_COUNT; ++index) {
            if (_stricmp(token, g_monster_script_commands[index]) == 0) {
                command = index;
                break;
            }
        }

        if (script_conditions_244.GetCount() != 0 &&
            *script_conditions_244.GetAt(0) == 0 &&
            command != MONSCR_ELSE && command != MONSCR_ENDIF) {
            if (++command_count > 50) {
                stop = 1;
            }
            continue;
        }

        if (state_28c.defining_orders == 0) {
            switch (command) {
            case MONSCR_GOTO: {
                token = strtok(0, " \t");
                if (token != 0) {
                    int line_number = script_238->FindLabelLine004CF730(token);
                    if (line_number != -1) {
                        script_conditions_244.Clear();
                        script_line_23c = line_number;
                    }
                }
                stop = 1;
                break;
            }
            case MONSCR_WALKTO: {
                srVector3T<float> position;
                token = strtok(0, " \t");
                if (token == 0) {
                    break;
                }
                if (_stricmp(token, "home") == 0) {
                    position = formation;
                }
                else if (_stricmp(token, "off_camera") == 0) {
                    position.x = position.y = position.z = -10000000.0f;
                }
                else if (FindEntityByName(token, &position, 0, 0) == 0) {
                    break;
                }
                if (_stricmp(token, "PARTY") == 0) {
                    Function4526C0(g_startup_world_659c0c, 5.0);
                }
                else {
                    Function452630(&position);
                }
                token = strtok(0, " \t");
                if (token == 0 || _stricmp(token, "NOBLOCK") != 0) {
                    script_wait_240 = MONSCR_WALKTO;
                }
                else {
                    stop = 1;
                }
                break;
            }
            case MONSCR_FACE: {
                srVector3T<float> position;
                token = strtok(0, " \t");
                if (token == 0) {
                    break;
                }
                if (_stricmp(token, "home") == 0) {
                    position = formation;
                }
                else if (_stricmp(token, "off_camera") == 0) {
                    position.x = position.y = position.z = -10000000.0f;
                }
                else if (FindEntityByName(token, &position, 0, 0) == 0) {
                    break;
                }
                AimAtPosition(&position);
                token = strtok(0, " \t");
                if (token == 0 || _stricmp(token, "NOBLOCK") != 0) {
                    script_wait_240 = MONSCR_FACE;
                }
                else {
                    stop = 1;
                }
                break;
            }
            case MONSCR_SAY:
            case MONSCR_NPCINTERACTION: {
                int line_number = -1;
                unsigned char suppress = 0;
                token = strtok(0, " \t");
                if (token != 0) {
                    line_number = atoi(token);
                    token = strtok(0, " \t");
                    if (token != 0 && _strnicmp(token, "SUPPRESS", 8) == 0) {
                        suppress = 1;
                    }
                }
                Function56C590(
                    Function50A440(MonsterGetIndexByLocationID(
                        command == MONSCR_SAY ? 0x1ac9 : 0x1b77,
                        MONSTER_CPP, propagated_value_1e4, 1)),
                    0, line_number,
                    command == MONSCR_SAY ? 1 : suppress);
                if (command == MONSCR_NPCINTERACTION) {
                    script_wait_240 = MONSCR_NPCINTERACTION;
                }
                else if (token == 0 || _stricmp(token, "NOBLOCK") != 0) {
                    script_wait_240 = MONSCR_SAY;
                }
                else {
                    stop = 1;
                }
                break;
            }
            case MONSCR_CYCLE: {
                signed char subcycle;
                token = strtok(0, " \t");
                if (token != 0) {
                    int cycle = ParseMonsterCycleName004C2010(token, &subcycle);
                    if (cycle != -1) {
                        m_pRep->selection.monster.pending_cycle = (signed char)cycle;
                        m_pRep->active = 1;
                        m_pRep->timer_068 = g_shared_timer_base->getUTime(
                            srTimer::TIMER_READ_DEFAULT);
                        SetSubCycle(0);
                        m_pRep->selection.monster.runtime_value_a6 = subcycle - 1;
                        if (m_pRep->selection.monster.pending_cycle == -1) {
                            m_pRep->selection.monster.pending_cycle =
                                m_pRep->selection.monster.current_cycle;
                        }
                        flags_1dc |= 0x10;
                        token = strtok(0, " \t");
                        if (token == 0 || _stricmp(token, "NOBLOCK") != 0) {
                            m_pRep->behaviour_071 = 1;
                            flags_1dc |= 0x20;
                            script_wait_240 = MONSCR_CYCLE;
                        }
                        else {
                            stop = 1;
                        }
                    }
                }
                break;
            }
            case MONSCR_SHOOT: {
                srVector3T<float> source;
                srVector3T<float> target;
                token = strtok(0, " \t");
                if (token == 0) {
                    break;
                }
                unsigned int owner = (unsigned int)atoi(token);
                token = strtok(0, " \t");
                if (token == 0) {
                    break;
                }
                if (_stricmp(token, "home") == 0) {
                    target = formation;
                }
                else if (_stricmp(token, "off_camera") == 0) {
                    target.x = target.y = target.z = -10000000.0f;
                }
                else if (FindEntityByName(token, &target, 0, 0) == 0) {
                    break;
                }
                if (GetProjectilePosition004C77F0(&source) == 0) {
                    GetMappedPosition004C72A0(&source);
                }
                Function4A2D30(
                    owner, &source, &target,
                    0, 0, 1, 0x47435000);
                break;
            }
            case MONSCR_GIVE:
                token = strtok(0, " \t");
                if (token != 0) {
                    int item = FindItemRecordByName(token);
                    if (item != -1) {
                        CreateItemIntoHandOrPool(item, 1);
                    }
                }
                break;
            case MONSCR_TELEPORT: {
                srVector3T<float> position;
                token = strtok(0, " \t");
                if (token != 0) {
                    if (_stricmp(token, "home") == 0) {
                        position = formation;
                    }
                    else if (_stricmp(token, "off_camera") == 0) {
                        position.x = position.y = position.z = -10000000.0f;
                    }
                    else if (FindEntityByName(token, &position, 0, 0) == 0) {
                        break;
                    }
                    SetPositionInternal00453590(&position);
                }
                break;
            }
            case MONSCR_DIE:
                m_pRep->behaviour_071 = 1;
                m_pRep->selection.monster.pending_cycle = 0x15;
                m_pRep->active = 1;
                m_pRep->timer_068 = g_shared_timer_base->getUTime(
                    srTimer::TIMER_READ_DEFAULT);
                SetSubCycle(0);
                break;
            case MONSCR_END:
                stop = 1;
                script_line_23c = script_238->lines.GetCount();
                break;
            case MONSCR_IF: {
                unsigned char invert = 0;
                unsigned char value;
                token = strtok(0, " \t");
                if (token != 0 && _strnicmp(token, "NOT", 3) == 0) {
                    invert = 1;
                    token = strtok(0, " \t");
                }
                value = EvaluateScriptCondition004C9DC0(token);
                script_conditions_244.InsertAt(
                    0, invert != 0 ? value == 0 : value != 0);
                break;
            }
            case MONSCR_ELSE:
                if (script_conditions_244.GetCount() != 0) {
                    *script_conditions_244.GetAt(0) =
                        *script_conditions_244.GetAt(0) == 0;
                }
                break;
            case MONSCR_ENDIF:
                if (script_conditions_244.GetCount() != 0) {
                    script_conditions_244.RemoveAt(0);
                }
                break;
            case MONSCR_DISPOSITION: {
                unsigned char disposition = 0xff;
                token = strtok(0, " \t");
                if (token != 0) {
                    if (_stricmp(token, "DISP_NEUTRAL") == 0) disposition = 0;
                    else if (_stricmp(token, "DISP_HOSTILE") == 0) disposition = 1;
                    else if (_stricmp(token, "DISP_FRIENDLY") == 0) disposition = 2;
                    monster_info = MonsterGetScriptPartByLocationIndex(
                        MonsterGetIndexByLocationID(
                            0x1b8b, MONSTER_CPP, propagated_value_1e4, 1));
                    if (monster_info != 0) {
                        unsigned int group_index = GetMonsterGroupIndexByID(
                            0x1b90, MONSTER_CPP,
                            monster_info->monster_group_id, 0);
                        if (group_index != (unsigned int)-1) {
                            W8MonsterGroup* group =
                                GetMonsterGroupByListIndex(group_index);
                            if (group != 0) {
                                Function547570(group, disposition, 0);
                            }
                        }
                    }
                }
                break;
            }
            case MONSCR_DISAPPEAR:
                flags_1dc |= 0x40;
                break;
            case MONSCR_LOOKHERE:
                Function48F650(
                    MonsterGetScriptPartByLocationIndex(
                        MonsterGetIndexByLocationID(
                            0x1ba0, MONSTER_CPP, propagated_value_1e4, 1)),
                    1, 1);
                token = strtok(0, " \t");
                if (token == 0 || _stricmp(token, "NOBLOCK") != 0) {
                    script_wait_240 = MONSCR_LOOKHERE;
                }
                else {
                    stop = 1;
                }
                break;
            case MONSCR_PATROL: {
                srVector3T<float> home;
                float distance;
                float variation;
                token = strtok(0, " \t");
                if (token == 0) break;
                distance = (float)atof(token) * g_world_scale_005ebc40;
                token = strtok(0, " \t");
                if (token == 0) break;
                variation = (float)atof(token) * g_world_scale_005ebc40;
                home = formation;
                if (home.x == 0.0f && home.y == 0.0f && home.z == 0.0f) {
                    srVector3T<float> current = GetPosition();
                    home.x = current.x;
                    home.y = current.y;
                    home.z = current.z;
                    formation = home;
                }
                StartPatrol(&home, distance, variation);
                break;
            }
            case MONSCR_STOPPATROL:
                fields.flags_00c &= 0xdfffffff;
                break;
            case MONSCR_TRIGGER:
                token = strtok(0, " \t");
                if (token != 0) {
                    trigger_278 = FindTriggerByName(token);
                    if (trigger_278 != 0) {
                        trigger_278->Run(-1);
                        script_wait_240 = MONSCR_TRIGGER;
                    }
                }
                break;
            case MONSCR_DELAY:
                token = strtok(0, " \t");
                if (token != 0 && (float)atof(token) != 0.0f) {
                    timer_254.SetDuration(
                        (float)atof(token) * g_monster_script_time_scale_005ec128);
                    timer_254.Restart();
                    script_wait_240 = MONSCR_DELAY;
                }
                break;
            case MONSCR_BEGINORDERS:
                state_28c.defining_orders = 1;
                break;
            case MONSCR_DEAF:
                state_28c.deaf = 1;
                break;
            case MONSCR_DOACTION:
                token = strtok(0, " \t");
                if (token != 0) {
                    if (_stricmp(token, "STARTGOLEMATTACK") == 0) {
                        Function577540();
                        W8MonsterGroup* group = FindFirstMonsterByID(0x68);
                        if (group != 0) Function547570(group, 1, 0);
                        group = FindFirstMonsterByID(0x13e);
                        if (group != 0) {
                            Function547570(group, 1, 0);
                            Function50F720(group);
                        }
                    }
                    else if (_stricmp(token, "ENDSAVANTWALK") == 0) {
                        flags_1dc |= 0x40;
                        Trigger* trigger = FindTriggerByName("Path3Trigger");
                        if (trigger != 0) trigger->Run(-1);
                    }
                    else if (_stricmp(token, "UNLOCKUI") == 0 ||
                             _stricmp(token, "ENDGARIWALK") == 0) {
                        Function577540();
                    }
                    else if (_stricmp(token, "ENDHOGARWALK") == 0) {
                        Function577540();
                        SetScript004C7F10("ClosePatrol.msf", 1);
                    }
                    else if (_stricmp(token, "ENDHOGARWALKANDPUTTOSLEEP") == 0) {
                        W8TargetSource source;
                        Function577540();
                        SetScript004C7F10("ClosePatrol.msf", 1);
                        monster_info = MonsterGetScriptPartByLocationIndex(
                            MonsterGetIndexByLocationID(
                                0x1c3a, MONSTER_CPP, propagated_value_1e4, 1));
                        ResetTargetSource(&source);
                        Function523C00(
                            monster_info->location_id, 0xf, 6, 0, &source, 1);
                    }
                    else if (_stricmp(token, "ENDBELAWALK") == 0) {
                        flags_1dc |= 0x40;
                        Function577540();
                    }
                    else if (_stricmp(token, "BELA_END_CC_WALK") == 0) {
                        void* list = GetNPCItemListByID(0x8d);
                        if (list != 0) Function56C5E0(list, 0, 6, 0, 0);
                        monster_info = MonsterGetScriptPartByLocationIndex(
                            MonsterGetIndexByLocationID(
                                0x14b3, MONSTER_CPP,
                                propagated_value_1e4, 1));
                        if (monster_info->control_state != 1) {
                            srVector3T<float> party;
                            GetCameraPosition(&party);
                            AimAtPosition(&party);
                        }
                    }
                }
                break;
            case MONSCR_PLAY: {
                int value = 0;
                float distance = 0.0f;
                token = strtok(0, " \t");
                if (token == 0) break;
                value = atoi(token);
                if (value != 0) token = strtok(0, " \t");
                if (token == 0) break;
                distance = (float)atof(token) * g_world_scale_005ebc40;
                if (distance != 0.0f) token = strtok(0, " \t");
                if (token == 0) break;
                sound_334 = new stSound3D(token, 0);
                if (sound_334 != 0) {
                    srVector3T<float> position = GetPosition();
                    sound_334->value_140 = value;
                    sound_334->setLocation(
                        (double)position.x, (double)position.y, (double)position.z);
                    if (distance != 0.0f) sound_334->value_144 = distance;
                    if (sound_334->Play004AEBF0(0, 0) == 0) {
                        sound_334->release();
                        sound_334 = 0;
                    }
                    else {
                        script_wait_240 = MONSCR_PLAY;
                    }
                }
                break;
            }
            case MONSCR_FADEOUT:
                flags_1dc |= 0x100;
                if ((signed char)flags_330.flag_00 >= 0) {
                    timer_30c.SetDuration(3.0f);
                    timer_30c.Restart();
                    if ((signed char)flags_330.flag_00 < 1) {
                        m_pRep->value_05c = 1.0f;
                        m_pRep->flag_061 = 1;
                    }
                    else {
                        timer_30c.SetProgress(1.0f - m_pRep->value_05c);
                    }
                    flags_330.flag_00 = -1;
                }
                RemoveMonster(
                    MonsterGetIndexByLocationID(
                        0x1021, MONSTER_CPP, propagated_value_1e4, 1),
                    0);
                stop = 1;
                script_line_23c = script_238->lines.GetCount();
                break;
            default:
                break;
            }
        }
        else {
            switch (command) {
            case MONSCR_FACE:
                token = strtok(0, " \t");
                if (token != 0 && _stricmp(token, "PARTY") == 0) {
                    state_28c.face_party = 1;
                }
                else if (token != 0) {
                    int direction = -1;
                    for (int index = MONSCR_EAST;
                         index <= MONSCR_SOUTHEAST; ++index) {
                        if (_stricmp(token, g_monster_script_commands[index]) == 0) {
                            direction = index;
                            break;
                        }
                    }
                    if (direction != -1) {
                        double angle =
                            (direction - MONSCR_EAST) *
                            g_monster_script_direction_step_005ed2b8;
                        state_28c.order_mode = 4;
                        state_2ac.direction_x =
                            (float)cos(angle) * g_monster_script_direction_scale_005ec150;
                        state_2ac.direction_y = 0.0f;
                        state_2ac.direction_z =
                            (float)sin(angle) * g_monster_script_direction_scale_005ec150;
                    }
                }
                break;
            case MONSCR_PATROL:
                token = strtok(0, " \t");
                if (token != 0) {
                    state_28c.patrol_distance =
                        (float)atof(token) * g_world_scale_005ebc40;
                    token = strtok(0, " \t");
                    if (token != 0) {
                        state_28c.patrol_variation =
                            (float)atof(token) * g_world_scale_005ebc40;
                        state_28c.order_mode = 1;
                    }
                }
                break;
            case MONSCR_ENDORDERS:
                state_28c.defining_orders = 0;
                state_28c.orders_finished = 1;
                script_wait_240 = MONSCR_ENDORDERS;
                break;
            case MONSCR_GUARD:
            case MONSCR_POINTPATROL:
            case MONSCR_RANDOMPOINTPATROL: {
                int added = 0;
                if (command == MONSCR_GUARD) vector_29c.Clear();
                while ((token = strtok(0, " \t")) != 0) {
                    srVector3T<float> position;
                    if (_stricmp(token, "home") == 0) {
                        position = formation;
                    }
                    else if (_stricmp(token, "off_camera") == 0) {
                        position.x = position.y = position.z = -10000000.0f;
                    }
                    else if (FindEntityByName(token, &position, 0, 0) == 0) {
                        continue;
                    }
                    if ((command == MONSCR_POINTPATROL ||
                         command == MONSCR_RANDOMPOINTPATROL) && added == 0) {
                        vector_29c.Clear();
                    }
                    if (vector_29c.Add(position) != -1) ++added;
                    if (command == MONSCR_GUARD) break;
                }
                if (command == MONSCR_GUARD && added != 0) {
                    state_28c.orders_finished = 0;
                    state_28c.order_mode = 0;
                }
                else if (added != 0) {
                    state_28c.order_mode =
                        command == MONSCR_POINTPATROL ? 2 : 3;
                }
                break;
            }
            case MONSCR_DEAF:
                state_28c.deaf = 1;
                break;
            case MONSCR_TURNTOFACEPARTY:
                state_28c.face_party = 1;
                break;
            case MONSCR_LOOKABOUT:
                token = strtok(0, " \t");
                if (token != 0) {
                    state_2ac.look_frequency = (float)atoi(token);
                    if (monster_info != 0) {
                        monster_info->unknown_301[1] = (unsigned char)atoi(token);
                    }
                    token = strtok(0, " \t");
                    if (token != 0) {
                        state_2ac.look_duration = (float)atoi(token);
                    }
                }
                break;
            case MONSCR_STAYHOME:
                state_28c.stay_home = 1;
                break;
            default:
                break;
            }
        }

        if (++command_count > 50) {
            stop = 1;
        }
    }

    if (script_238 != 0 &&
        script_line_23c >= script_238->lines.GetCount() &&
        script_wait_240 == -1) {
        script_238->release();
        script_238 = 0;
        flags_1dc &= ~0x20;
        script_line_23c = 0;
        script_wait_240 = -1;
        if (sound_334 != 0) {
            sound_334->release();
            sound_334 = 0;
        }
    }
}

/* A script command that requested blocking leaves its command number here.
   Each command family has one concrete completion condition; commands without
   a condition are immediately ready. */
// FUNCTION: WIZ8 0x004CA0F0
unsigned char W8Monster::CanContinueScript004CA0F0()
{
    switch (script_wait_240) {
    case 1:
        if (fields.flag_024 == 0) {
            return 0;
        }
        break;
    case 2:
        if ((float)fabs(fields.movement_0c0.target_yaw -
                        fields.movement_0c0.yaw) >=
            g_monster_script_facing_tolerance_005ebc84) {
            return 0;
        }
        break;
    case 3:
        if (Function525DF0(0) != 0) {
            return 0;
        }
        break;
    case 4:
        if (Query(2) == 0) {
            return 0;
        }
        flags_1dc &= ~0x20;
        return 1;
    case 0x0e:
        if (g_flag_00683f97 == 1) {
            return 0;
        }
        break;
    case 0x16:
        if (trigger_278 != 0 && (trigger_278->flags_0a0 & 0x40) != 0) {
            return 0;
        }
        trigger_278 = 0;
        return 1;
    case 0x17:
        if (timer_254.GetProgress() < g_float_005ebb38) {
            return 0;
        }
        break;
    case 0x29:
        if (sound_334->IsPlaying004AEC70() != 0) {
            return 0;
        }
        sound_334->release();
        sound_334 = 0;
        break;
    }
    return 1;
}

// FUNCTION: WIZ8 0x004CA260
unsigned char W8Monster::SetScriptLabel004CA260(const char* label)
{
    int line;

    if (script_238 != 0) {
        line = script_238->FindLabelLine004CF730(label);
        if (line >= 0) {
            script_line_23c = line;
            return 1;
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x004CA290
unsigned char W8Monster::GetFlag216004CA290() const
{
    return flag_216;
}

// FUNCTION: WIZ8 0x004CA2A0
unsigned char W8Monster::IsWithinWorldRange004CA2A0()
{
    if (state_2fc.node_0c != 0) {
        return state_2fc.node_0c->testFlag(srNode::FLAG_POSITIONAL_0) == 0;
    }
    else {
        double far_clip = WorldGetFarClip(GetWorld());
        srVector3T<float> position = GetPosition();
        srVector3T<float> reference = g_startup_world_659c0c->GetPosition();
        float dx = reference.x - position.x;
        float dy = reference.y - position.y;
        float dz = reference.z - position.z;

        return dx * dx + dy * dy + dz * dz <=
               (float)far_clip * (float)far_clip;
    }
}

/* Exercise the inexpensive elevated-origin sight query from this Monster to
   the player. The caller needs the trace side effects rather than its result. */
// FUNCTION: WIZ8 0x004c4810
void W8Monster::CheckLineOfSightToPlayer004C4810()
{
    srVector3T<float> monster_position;
    srVector3T<float> player_position;

    monster_position.x = fields.movement_0c0.position_040.x;
    monster_position.y = fields.movement_0c0.position_040.y +
                         fields.movement_0c0.height_offset_0b8;
    monster_position.z = fields.movement_0c0.position_040.z;
    GetCameraPosition(&player_position);
    g_octree_6598a4->HasLineOfSight(
        &monster_position, &player_position, 1);
}

/* The sight code keeps the engine trace's three outcomes as two independent
   flags. A clear trace sets both false, the special -1 result sets only the
   secondary flag, and every other obstructed result sets both. */
// FUNCTION: WIZ8 0x004c4870
void W8Monster::GetPlayerSightFlags004C4870(
    unsigned char* primary, unsigned char* secondary)
{
    srVector3T<float> monster_position;
    srVector3T<float> player_position;
    short result;

    monster_position.x = fields.movement_0c0.position_040.x;
    monster_position.y = fields.movement_0c0.position_040.y +
                         fields.movement_0c0.height_offset_0b8;
    monster_position.z = fields.movement_0c0.position_040.z;
    GetCameraPosition(&player_position);
    result = g_octree_6598a4->TraceLineOfSight(
        &monster_position, &player_position, 1,
        propagated_value_1e4, -1, 1, 0);
    if (result == -1) {
        *secondary = 1;
        *primary = 0;
    }
    else if (result != 1) {
        *secondary = 1;
        *primary = 1;
    }
    else {
        *secondary = 0;
        *primary = 0;
    }
}

/* The inexpensive visibility path tests the Monster's elevated origin. The
   detailed path tests the translated animation bounds at their centre and
   corners. */
// FUNCTION: WIZ8 0x004c4920
unsigned char W8Monster::IsVisibleToPlayer004C4920(unsigned char use_bounds)
{
    srVector3T<float> player_position;

    GetCameraPosition(&player_position);
    if (use_bounds != 0) {
        srVector3T<float> minimum;
        srVector3T<float> maximum;

        if (this != 0) {
            srVector3T<float> position;

            GetAnimationBounds(&minimum, &maximum);
            position = GetPosition();
            minimum.x += position.x;
            minimum.y += position.y;
            minimum.z += position.z;
            position = GetPosition();
            maximum.x += position.x;
            maximum.y += position.y;
            maximum.z += position.z;
        }
        return HasLineOfSightToBounds0046FD70(
            &player_position, &minimum, &maximum);
    }

    srVector3T<float> monster_position;
    monster_position.x = fields.movement_0c0.position_040.x;
    monster_position.y = fields.movement_0c0.position_040.y +
                         fields.movement_0c0.height_offset_0b8;
    monster_position.z = fields.movement_0c0.position_040.z;
    return g_octree_6598a4->HasLineOfSight(
        &player_position, &monster_position, 1);
}

// FUNCTION: WIZ8 0x004c4a20
void W8Monster::GetPlayerToMonsterSightFlags004C4A20(
    unsigned char* primary,
    unsigned char* secondary,
    const srVector3T<float>* source)
{
    srVector3T<float> monster_position;
    srVector3T<float> player_position;
    short result;

    monster_position.x = fields.movement_0c0.position_040.x;
    monster_position.y = fields.movement_0c0.position_040.y +
                         fields.movement_0c0.height_offset_0b8;
    monster_position.z = fields.movement_0c0.position_040.z;
    if (source == 0) {
        GetCameraPosition(&player_position);
    }
    else {
        player_position = *source;
    }
    result = g_octree_6598a4->TraceLineOfSight(
        &player_position, &monster_position, 1,
        -1, propagated_value_1e4, 1, 0);
    if (result == -1) {
        *secondary = 1;
        *primary = 0;
    }
    else if (result != 1) {
        *secondary = 1;
        *primary = 1;
    }
    else {
        *secondary = 0;
        *primary = 0;
    }
}

// FUNCTION: WIZ8 0x004c4af0
unsigned char W8Monster::HasLineOfSightToMonster004C4AF0(
    W8Monster* monster)
{
    srVector3T<float> from;
    srVector3T<float> to;

    from.x = fields.movement_0c0.position_040.x;
    from.y = fields.movement_0c0.position_040.y +
             fields.movement_0c0.height_offset_0b8;
    from.z = fields.movement_0c0.position_040.z;
    to.x = monster->fields.movement_0c0.position_040.x;
    to.y = monster->fields.movement_0c0.position_040.y +
           monster->fields.movement_0c0.height_offset_0b8;
    to.z = monster->fields.movement_0c0.position_040.z;
    return g_octree_6598a4->HasLineOfSight(&from, &to, 1);
}

// FUNCTION: WIZ8 0x004c4b70
void W8Monster::GetMonsterSightFlags004C4B70(
    W8Monster* monster,
    unsigned char* primary,
    unsigned char* secondary)
{
    srVector3T<float> from;
    srVector3T<float> to;
    short result;

    from.x = fields.movement_0c0.position_040.x;
    from.y = fields.movement_0c0.position_040.y +
             fields.movement_0c0.height_offset_0b8;
    from.z = fields.movement_0c0.position_040.z;
    to.x = monster->fields.movement_0c0.position_040.x;
    to.y = monster->fields.movement_0c0.position_040.y +
           monster->fields.movement_0c0.height_offset_0b8;
    to.z = monster->fields.movement_0c0.position_040.z;
    result = g_octree_6598a4->TraceLineOfSight(
        &from, &to, 1,
        propagated_value_1e4, monster->propagated_value_1e4, 1, 0);
    if (result == -1) {
        *secondary = 1;
        *primary = 0;
    }
    else if (result != 1) {
        *secondary = 1;
        *primary = 1;
    }
    else {
        *secondary = 0;
        *primary = 0;
    }
}

// FUNCTION: WIZ8 0x004c4c40
unsigned char W8Monster::HasLineOfSightFromPoint004C4C40(
    srVector3T<float> point)
{
    srVector3T<float> monster_position;

    monster_position.x = fields.movement_0c0.position_040.x;
    monster_position.y = fields.movement_0c0.position_040.y +
                         fields.movement_0c0.height_offset_0b8;
    monster_position.z = fields.movement_0c0.position_040.z;
    return g_octree_6598a4->TraceLineOfSight(
        &point, &monster_position, 1, -3, -3, 1, 0) != 1;
}

// FUNCTION: WIZ8 0x004c4ca0
int W8Monster::IsFacingMonster004C4CA0(W8Monster* monster)
{
    float bearing;
    float facing;
    srVector3T<float> from;
    srVector3T<float> to;

    if (monster == 0) {
        srAssertFail("pMonsterB", MONSTER_CPP, 3812, 0);
    }
    to = monster->GetPosition();
    from = GetPosition();
    bearing = NormalizeAngle(BearingBetween(&from, &to));
    facing = NormalizeAngle(GetYaw());
    return fabs(bearing - facing) <=
           g_monster_facing_tolerance_005ec2b0;
}

// FUNCTION: WIZ8 0x004c4d40
int W8Monster::IsFacingPlayer004C4D40()
{
    float bearing;
    float facing;
    srVector3T<float> from;
    srVector3T<float> to;

    if (g_startup_world_659c0c == 0) {
        srAssertFail("pPlayer", MONSTER_CPP, 3838, 0);
    }
    to = g_startup_world_659c0c->GetPosition();
    from = GetPosition();
    bearing = NormalizeAngle(BearingBetween(&from, &to));
    facing = NormalizeAngle(GetYaw());
    return fabs(bearing - facing) <=
           g_monster_facing_tolerance_005ec2b0;
}

/* Start making the representation visible.  Reversing an active fade-out
   preserves its current scale by seeding the opposite timer at that progress;
   an idle monster starts from zero instead. */
// FUNCTION: WIZ8 0x004c4f80
void W8Monster::BeginFadeIn004C4F80(float duration)
{
    if (flags_330.flag_00 <= 0) {
        timer_30c.SetDuration(duration);
        timer_30c.Restart();
        if (flags_330.flag_00 < 0) {
            W8MonsterRep* rep = m_pRep;
            timer_30c.SetProgress(rep->value_05c);
        }
        else {
            W8MonsterRep* rep = m_pRep;
            rep->value_05c = 0.0f;
            rep->flag_061 = 1;
        }
        flags_330.flag_00 = 1;
        flags_1dc &= ~0x400;
    }
}

// FUNCTION: WIZ8 0x004c5000
void W8Monster::BeginDelayedRemoval004C5000()
{
    flags_1dc |= 0x100;
    flags_330.flag_00 = -2;
    timer_30c.SetDuration(5.0f);
    timer_30c.Restart();
}

/* Begin the three-second disappearance transition, remove the live monster
   from the manager without destroying it, and retain the requested terminal
   state for the transition's completion. */
// FUNCTION: WIZ8 0x004c5040
void W8Monster::BeginFadeOutAndRemove004C5040(signed char state)
{
    flags_1dc |= 0x100;
    if (flags_330.flag_00 >= 0) {
        timer_30c.SetDuration(3.0f);
        timer_30c.Restart();
        W8MonsterRep* rep = m_pRep;
        if (flags_330.flag_00 > 0) {
            timer_30c.SetProgress(
                1.0f - rep->value_05c);
        }
        else {
            rep->value_05c = 1.0f;
            rep->flag_061 = 1;
        }
        flags_330.flag_00 = -1;
    }
    RemoveMonster(
        MonsterGetIndexByLocationID(
            0x1021, MONSTER_CPP, propagated_value_1e4, 1),
        0);
    state_22e = state;
}

// FUNCTION: WIZ8 0x004c5150
void W8Monster::BeginFadeOut004C5150(float duration)
{
    if (flags_330.flag_00 < 0) {
        return;
    }
    timer_30c.SetDuration(duration);
    timer_30c.Restart();
    if (flags_330.flag_00 > 0) {
        W8MonsterRep* rep = m_pRep;
        timer_30c.SetProgress(1.0f - rep->value_05c);
        flags_330.flag_00 = -1;
        return;
    }
    W8MonsterRep* rep = m_pRep;
    rep->value_05c = 1.0f;
    rep->flag_061 = 1;
    flags_330.flag_00 = -1;
}

// FUNCTION: WIZ8 0x004c73f0
void W8Monster::StartTalking004C73F0(unsigned char animate_mouth)
{
    if (m_pRep != 0) {
        flag_1fc = 1;
        flag_1fd = animate_mouth;
        value_200 = GetTickCount();
        unknown_214 = 0;
        value_210 = -1;
        value_208 = GetTickCount();
        value_20c = Random(2000) + 2000;
        m_pRep->selection.monster.pending_cycle = 0x18;
        m_pRep->behaviour_071 = 1;
    }
}

// FUNCTION: WIZ8 0x004c7470
void W8Monster::StopTalking004C7470()
{
    if (m_pRep != 0) {
        srModelInstance* model;
        stTextureAnim* mouth;

        flag_1fc = 0;
        model = GetCurrentModelInstance004A8250();
        if (model != 0) {
            mouth = static_cast<stModelInstance*>(model)
                        ->FindMouthTexture00481080();
            if (mouth != 0) {
                mouth->flag_60 = 3;
                mouth->SetFrame00485400(0);
            }
        }
        if (m_pRep->selection.monster.current_cycle != 0x15) {
            m_pRep->behaviour_071 = 3;
            m_pRep->selection.monster.pending_cycle = 1;
        }
    }
}

// FUNCTION: WIZ8 0x004ca340
void W8Monster::SetCycleCallback004CA340(
    int cycle, CycleCallback callback)
{
    cycle_callback_230 = callback;
    callback_cycle_234 = cycle;
}

// FUNCTION: WIZ8 0x004ca360
unsigned char W8Monster::GetPatrolPoint004CA360(srVector3T<float>* point)
{
    srVector3T<float>* patrol_point;

    if (state_28c.orders_finished == 0 || state_2ac.flag_00 < 0) {
        return 0;
    }
    if (vector_29c.GetCount() == 0) {
        if (formation.x == 0.0f &&
            formation.y == 0.0f &&
            formation.z == 0.0f) {
            srVector3T<float> position = GetPosition();
            formation.x = position.x;
            formation.y = position.y;
            formation.z = position.z;
        }
        vector_29c.Add(formation);
    }

    if (state_28c.order_mode == 0) {
        patrol_point = vector_29c.GetAt(0);
    }
    else if (state_28c.order_mode > 1 && state_28c.order_mode < 4) {
        if (state_2ac.flag_00 < vector_29c.GetCount()) {
            patrol_point = vector_29c.GetAt(state_2ac.flag_00);
        }
        else {
            patrol_point = vector_29c.GetAt(0);
        }
    }
    else {
        return 0;
    }

    *point = *patrol_point;
    return 1;
}

// FUNCTION: WIZ8 0x004ca4f0
unsigned char MonsterGetWorldAnimationBounds004CA4F0(
    W8Monster* monster, srVector3T<float>* minimum, srVector3T<float>* maximum)
{
    if (monster != 0) {
        srVector3T<float> position;

        monster->GetAnimationBounds(minimum, maximum);
        position = monster->GetPosition();
        minimum->x += position.x;
        minimum->y += position.y;
        minimum->z += position.z;
        position = monster->GetPosition();
        maximum->x += position.x;
        maximum->y += position.y;
        maximum->z += position.z;
        return 1;
    }
    return 0;
}

/* Keep only the short live-sound queue owned by the monster.  The growable
   vector's normal Add/GetAt/RemoveAt methods reproduce the original inline
   template operations; no address-shaped container wrapper is involved. */
// FUNCTION: WIZ8 0x004ca6e0
void W8Monster::TrackSoundHandle004CA6E0(int handle)
{
    int count;
    int index;

    if (values_338.GetCount() > 6) {
        while (values_338.GetCount() != 0) {
            SoundStop(*values_338.GetAt(0));
            values_338.RemoveAt(0);
        }
    }
    values_338.Add(handle);
    count = values_338.GetCount();
    index = 0;
    if (count > 0) {
        do {
            if (SoundIsPlaying(*values_338.GetAt(index)) == 0) {
                values_338.RemoveAt(index);
                --count;
            }
            ++index;
        } while (index < count);
    }
}

/* Mark the closest live member of each loaded group inside the selection
   range. Members that do not improve the current candidate are unmarked. */
// FUNCTION: WIZ8 0x004ca570
void UpdateNearestMonsterGroupMembers004CA570()
{
    srVector3T<float> player_position;
    unsigned int group_index;

    GetCameraPosition(&player_position);
    for (group_index = 0;
         group_index < PLLength(g_monster_group_list);
         ++group_index) {
        W8MonsterGroup* group = GetMonsterGroupByListIndex(group_index);

        if (group != 0) {
            double nearest_distance = 1e11;
            W8MonsterInfo* nearest = 0;
            unsigned int member_index;

            for (member_index = 0;
                 member_index < ILLength(group->monsters);
                 ++member_index) {
                W8MonsterInfo* member = MonsterGetScriptPartByLocationIndex(
                    MonsterGetIndexByLocationID(
                        0x1efe,
                        MONSTER_CPP,
                        IListGetAt(group->monsters, member_index),
                        1));

                if (member != 0 && member->monster != 0) {
                    srVector3T<float> position = member->monster->GetPosition();
                    float x = position.x - player_position.x;
                    float y = position.y - player_position.y;
                    float z = position.z - player_position.z;
                    float distance = (float)sqrt(x * x + y * y + z * z);

                    if (distance < g_monster_group_nearest_range_005ed2c0 &&
                        distance < nearest_distance) {
                        nearest_distance = distance;
                        nearest = member;
                    }
                    else {
                        member->monster->flag_218 = 0;
                    }
                }
            }
            if (nearest != 0) {
                nearest->monster->flag_218 = 1;
            }
        }
    }
}

// FUNCTION: WIZ8 0x004c7cb0
float W8Monster::GetDistanceToPlayer004C7CB0()
{
    srVector3T<float> position = GetPosition();
    srVector3T<float> player_position;
    float x;
    float y;
    float z;
    float distance;

    GetCameraPosition(&player_position);
    x = position.x - player_position.x;
    y = position.y - (player_position.y - g_startup_depth_603ac8);
    z = position.z - player_position.z;
    distance = (float)sqrt(x * x + y * y + z * z) -
        fields.movement_0c0.alternate_radius_0b4 -
        g_startup_world_659c0c->fields.movement_0c0.alternate_radius_0b4;
    if (distance < g_float_005ebb34) {
        distance = g_float_005ebb34;
    }
    return distance;
}

// FUNCTION: WIZ8 0x004c7d50
float W8Monster::GetPointDistanceToPlayer004C7D50(
    float x, float y, float z)
{
    srVector3T<float> player_position;
    float delta_x;
    float delta_y;
    float delta_z;
    float distance;

    GetCameraPosition(&player_position);
    delta_x = x - player_position.x;
    delta_y = y - (player_position.y - g_startup_depth_603ac8);
    delta_z = z - player_position.z;
    distance = (float)sqrt(
        delta_x * delta_x + delta_y * delta_y + delta_z * delta_z) -
        fields.movement_0c0.alternate_radius_0b4 -
        g_startup_world_659c0c->fields.movement_0c0.alternate_radius_0b4;
    if (distance < g_float_005ebb34) {
        distance = g_float_005ebb34;
    }
    return distance;
}

// FUNCTION: WIZ8 0x004c7dd0
float W8Monster::GetDistanceToMonster004C7DD0(W8Monster* monster)
{
    srVector3T<float> position = GetPosition();
    srVector3T<float> other_position = monster->GetPosition();
    float x = position.x - other_position.x;
    float y = position.y - other_position.y;
    float z = position.z - other_position.z;
    float distance = (float)sqrt(x * x + y * y + z * z) -
        fields.movement_0c0.alternate_radius_0b4 -
        monster->fields.movement_0c0.alternate_radius_0b4;

    if (distance < g_float_005ebb34) {
        distance = g_float_005ebb34;
    }
    return distance;
}

// FUNCTION: WIZ8 0x004c7e80
float W8Monster::GetPointDistanceToMonster004C7E80(
    W8Monster* monster, float x, float y, float z)
{
    srVector3T<float> position = monster->GetPosition();
    float delta_x = x - position.x;
    float delta_y = y - position.y;
    float delta_z = z - position.z;
    float distance = (float)sqrt(
        delta_x * delta_x + delta_y * delta_y + delta_z * delta_z) -
        fields.movement_0c0.alternate_radius_0b4 -
        monster->fields.movement_0c0.alternate_radius_0b4;

    if (distance < g_float_005ebb34) {
        distance = g_float_005ebb34;
    }
    return distance;
}

/* Decide whether a requested cycle can replace the current one. Monster adds
   combat, motionless, and death rules to W8GrCycle's primary-table operation;
   W8Navigator's distinct slot 3 remains inherited in the secondary table. */
// FUNCTION: WIZ8 0x004c2bf0
unsigned char W8Monster::CanEnterCycle(signed char cycle)
{
    unsigned int monster_index = MonsterGetIndexByLocationID(
        0x969, MONSTER_CPP, propagated_value_1e4, 1);
    W8MonsterInfo* monster_info =
        MonsterGetScriptPartByLocationIndex(monster_index);

    if (g_in_combat_00683f94 != 0 && Function420E10() != 0) {
        return 0;
    }
    if (m_pRep->flag_06d == 0) {
        if (cycle != 0x14 && cycle != 0x15 && cycle != 0 &&
            monster_info->motionless != 0) {
            if (g_flag_00689b32 == 0) {
                return 0;
            }
            srAssertFail("FALSE", MONSTER_CPP, 0x97f, 0);
            return 0;
        }
    } else {
        if (IsCycleInterruptable((signed char)Query(6)) == 0 && Query(7) == 0) {
            return 0;
        }
        if (cycle == 0x15 && monster_info->monster_species == 0x199 &&
            Query(2) == 0) {
            return 0;
        }
    }
    return 1;
}

/* Report whether a cycle may be interrupted. The original diagnostic names
   this operation `Monster::CycleInterruptable`; its spelling is retained here
   because it is the only source-level name available. */
// FUNCTION: WIZ8 0x004c2cf0
unsigned char W8Monster::IsCycleInterruptable(signed char cycle)
{
    signed char current_cycle;
    signed char pending_cycle;
    const char* pending_name;
    const char* current_name;
    const char* requested_name;

    if (GetFlag68F105() != 0) {
        return 1;
    }
    if (m_pRep->flag_06d == 0) {
        current_cycle = (signed char)Query(6);
        pending_cycle = m_pRep->selection.monster.pending_cycle;
        if (pending_cycle != -1 && CanEnterCycle(pending_cycle) != 0) {
            pending_name = g_cycle_names[pending_cycle].name;
            if (current_cycle != -1) {
                current_name = g_cycle_names[current_cycle].name;
            } else {
                current_name = "";
            }
            if (cycle != -1) {
                requested_name = g_cycle_names[cycle].name;
            } else {
                requested_name = "";
            }
            FormatDebugMessage(
                1,
                "Monster::CycleInterruptable (ID %d) - WARNING: Monster is "
                "not animating - Cycle %d(%s), current %d(%s), pending "
                "%d(%s)",
                propagated_value_1e4,
                (int)cycle,
                requested_name,
                (int)current_cycle,
                current_name,
                (int)pending_cycle,
                pending_name);
        }
        return 1;
    }

    switch (cycle) {
    case -1:
    case 1:
    case 2:
    case 3:
    case 4:
    case 0x16:
        return 1;
    }
    return 0;
}

/* Apply the two script-specific side effects selected before an ungrouped
   monster is removed: state two returns it to Balbrak's home marker, while
   state three clears the ScregActive trigger variable. */
// FUNCTION: WIZ8 0x004c50f0
void W8Monster::ApplyRemovalStateEffects()
{
    srVector3T<float> position;

    switch (state_22e) {
    case 2:
        if (FindEntityByName("NP_Balbrakhome", &position, 0, 0) != 0) {
            SetPosition(&position);
        }
        break;
    case 3:
        SetTriggerVariableByName00444030("ScregActive", 0);
        break;
    }
}

/* Engine Code\Monster.cpp. CYCLE_NUM_UNIQUE and the method name both come from
   the canonical assertion at line 960, whose message reads
   "GetNumSubsPerCycle() -> Invalid cycle num.". The element count and stride
   agree with the reviewed constructor: 27 entries of 0x10 bytes at 0xAC ends at
   0x25C, exactly where Monster's second vector array begins. */
// FUNCTION: WIZ8 0x004bfab0
unsigned char W8MonsterRep::GetNumSubsPerCycle(signed char bCycle)
{
    if (bCycle >= W8_MONSTER_CYCLE_COUNT) {
        srAssertFail(
            "bCycle < CYCLE_NUM_UNIQUE",
            MONSTER_CPP,
            0x3c0,
            "GetNumSubsPerCycle() -> Invalid cycle num.");
    }
    if (bCycle == -1) {
        bCycle = selection.monster.current_cycle;
    }
    return (unsigned char)animations[bCycle].GetCount();
}

/* Select the active AnimObj for a cycle and dispatch the requested LOD/frame.
   This is the concrete implementation behind AnimRep's third vtable slot. */
// FUNCTION: WIZ8 0x004bf8c0
srModelInstance* W8MonsterRep::SetCycleFrameLod(
    signed char cycle, signed char frame, signed char lod)
{
    int subcycle = selection.monster.current_subcycle;
    W8MonsterAnimationVector* selected_cycle = &animations[cycle];
    W8AnimObj** animation_slot;
    W8AnimObj* animation;

    if (subcycle < selected_cycle->GetCount()) {
        animation_slot = selected_cycle->data + subcycle;
    }
    else {
        animation_slot = selected_cycle->data;
    }
    animation = *animation_slot;
    if (animation->flag_05 == 0) {
        return AnimObjDispatch004A14D0(animation, (signed char)lod, frame);
    }
    return AnimObjDispatchList004A1560(animation, (signed char)lod, 0);
}

/* The selected subcycle's AniMesh for one animation cycle.  AnimObj's
   canonical body takes the three stack arguments emitted here; keep that
   call-site ABI local until the older four-parameter declaration is
   corrected as its own bundle. */
// FUNCTION: WIZ8 0x004bf920
W8AniMesh* W8MonsterRep::GetEmitterAniMesh(char cycle)
{
    typedef void* (__cdecl *LegacyAnimObjEntryCall)(
        W8AnimObj*, signed char, unsigned int);

    W8AnimObj* animation =
        *animations[cycle].GetAt(selection.monster.current_subcycle);

    if (animation == 0) {
        return 0;
    }
    return (W8AniMesh*)((LegacyAnimObjEntryCall)AnimObjEntry004A1660)(
        animation, m_bLOD, 0);
}

/* Synchronize the live world representation with the Navigator state, update
   transient mouth/scale effects, and attach or hide the cycle's light graph.
   This is Monster's primary slot four; the world is the ordinary stack
   argument also consumed by the inherited GrCycle implementation. */
// FUNCTION: WIZ8 0x004c2e60
void W8Monster::UpdateRepresentation(W8World* world)
{
    srVector3T<float> position;
    srMatrix3T<float> rotation;
    srModelInstance* model;
    stTextureAnim* mouth;
    W8MonsterLightVector* lights;
    int index;
    int count;

    position.x = fields.movement_0c0.position_040.x;
    position.y = fields.movement_0c0.position_040.y;
    position.z = fields.movement_0c0.position_040.z;
    position.y += fields.movement_0c0.vertical_offset_0c0;
    GetRepresentation()->SetLocation004B8850(&position);

    rotation.SetIdentity00467310();
    {
        float angle = NormalizeAngle(
            GetYaw() + g_monster_rotation_offset_005ec04c);
        if (angle != 0.0f) {
            rotation.method_00438F90(sin((double)angle), cos((double)angle));
        }
    }
    {
        float angle = GetPitch();
        if (angle != g_float_005ebb34) {
            rotation.method_004A5AB0((double)angle);
        }
    }
    if (fields.movement_0c0.roll_028 != g_float_005ebb34) {
        rotation.method_004CAB60((double)fields.movement_0c0.roll_028);
    }
    m_pRep->SetRotation004B88D0(&rotation);

    if ((flags_1dc & 8) != 0) {
        model = GetCurrentModelInstance004A8250();
        srVector3T<double> source_scale = model->getScale();
        float scale_y = (float)source_scale.y * value_1ec;
        float scale_z = (float)source_scale.z;
        srVector3T<double> scale;
        scale.x = source_scale.x;
        scale.y = (double)scale_y;
        scale.z = (double)scale_z;
        model->setScale(scale);
        value_1ec -= g_float_005ebc3c;
    }

    if (flag_1fc != 0 && flag_1fd != 0) {
        if (unknown_214 != 0) {
            model = GetCurrentModelInstance004A8250();
            if (model != 0 &&
                (mouth = static_cast<stModelInstance*>(model)
                    ->FindMouthTexture00481080()) != 0) {
                mouth->flag_60 = 3;
                mouth->SetFrame00485400(0);
            }
        }
        else if (GetTickCount() - value_200 > 120) {
            unsigned short frame;
            value_200 = GetTickCount();
            do {
                frame = (unsigned short)Random(6);
                if (frame > 3) {
                    frame = 0;
                }
            } while (frame == (unsigned short)value_204);
            value_204 = frame;
            model = GetCurrentModelInstance004A8250();
            if (model != 0 &&
                (mouth = static_cast<stModelInstance*>(model)
                    ->FindMouthTexture00481080()) != 0) {
                mouth->flag_60 = 3;
                mouth->SetFrame00485400(frame);
            }
        }
    }

    if (fields.position_dirty_09c != 0 ||
        fields.movement_0c0.position_adjusted_0c8 != 0) {
        g_octree_6598a4->UpdateMonsterLocation(
            (unsigned short)propagated_value_1e4, &position);
    }

    if ((state_2fc.node_0c == 0 ||
         state_2fc.node_0c->testFlag(srNode::FLAG_POSITIONAL_0) == 0) &&
        IsRenderable004C7C00(0) != 0) {
        W8GrCycle::UpdateRepresentation(world);
        model = GetCurrentModelInstance004A8250();
        if (model != 0) {
            static_cast<stModelInstance005EC7D0*>(model)->value_1ac =
                g_monster_model_value_enabled_00685111 != 0
                    ? unknown_1d4 : 0.0f;
            SetChainValue15C((char*)model, 4);
        }
        if (m_pRep->monster_light_624 != 0) {
            m_pRep->monster_light_624->Update0049D990(&position);
        }
        if (enabled_1bd == 0) {
            enabled_1bd = 1;
            SetShakeEventVisibility004BF9E0(
                m_pRep->selection.monster.current_cycle);
            lights = *m_pRep->light_lists[
                m_pRep->selection.monster.current_cycle].GetAt(
                    m_pRep->selection.monster.current_subcycle);
            if (lights != 0 && (count = lights->GetCount()) != 0) {
                for (index = 0; index < count; ++index) {
                    (*lights->GetAt(index))->clearFlag(
                        srNode::FLAG_POSITIONAL_0);
                }
            }
            if (m_pRep->monster_light_624 != 0) {
                m_pRep->monster_light_624->SetVisible0049D970(1);
            }
        }
    }
    else if (enabled_1bd != 0) {
        enabled_1bd = 0;
        SetShakeEventVisibility004BF9E0(
            m_pRep->selection.monster.current_cycle);
        lights = *m_pRep->light_lists[
            m_pRep->selection.monster.current_cycle].GetAt(
                m_pRep->selection.monster.current_subcycle);
        if (lights != 0 && (count = lights->GetCount()) != 0) {
            for (index = 0; index < count; ++index) {
                (*lights->GetAt(index))->setFlag(srNode::FLAG_POSITIONAL_0);
            }
        }
        if (m_pRep->monster_light_624 != 0) {
            m_pRep->monster_light_624->SetVisible0049D970(0);
        }
    }
}

/* Enable only the shake particles belonging to the requested cycle and the
   currently selected subcycle. A ranged particle is left to the frame-driven
   update path; this method only toggles particles without a distinct range. */
// FUNCTION: WIZ8 0x004bf9e0
void W8Monster::SetShakeEventVisibility004BF9E0(signed char cycle)
{
    int index;
    int count;

    if (m_plsParticles == 0 ||
        (count = m_plsParticles->GetCount()) == 0) {
        return;
    }

    for (index = 0; index < count; ++index) {
        W8GrCycleShakeEvent* event = *m_plsParticles->GetAt(index);

        if (event->cycle_00 == cycle &&
            event->subcycle_04 ==
                m_pRep->selection.monster.current_subcycle &&
            enabled_1bd != 0) {
            W8AnimObj* animation = *m_pRep->animations[
                m_pRep->selection.monster.current_cycle].GetAt(
                    m_pRep->selection.monster.current_subcycle);

            if (animation == 0 || animation->start_frame_14 == 0 ||
                animation->end_frame_15 < animation->start_frame_14) {
                stParticle* particle = event->particle_08;
                if (particle->start_frame_264 != -1 &&
                    particle->end_frame_268 != -1 &&
                    particle->start_frame_264 != particle->end_frame_268) {
                    continue;
                }
                particle->SetActive(1);
            }
        }
        else {
            event->particle_08->SetActive(0);
        }
    }
}

/* The Monster vtable's slot-three method selects the active subcycle's
   AnimObj (falling back to entry zero) and submits the Monster's current
   animation index.  The assertion's `pao` spelling establishes the pointee's
   AnimObj identity without supplying a name for this Monster method. */
// FUNCTION: WIZ8 0x004bf970
unsigned int W8MonsterRep::ApplyEmitterSetting(char cycle)
{
    W8MonsterAnimationVector* selected_cycle = &animations[cycle];
    W8AnimObj** animation_slot;
    W8AnimObj* animation;

    if (selection.monster.current_subcycle < selected_cycle->GetCount()) {
        animation_slot = selected_cycle->data +
            selection.monster.current_subcycle;
    } else {
        animation_slot = selected_cycle->data;
    }
    animation = *animation_slot;
    if (animation == 0) {
        srAssertFail(
            "pao",
            "C:\\Projects\\Wizardry 8\\Engine Code\\Monster.cpp",
            0x2de,
            0);
    }
    return AnimObjValue004A15D0(
        animation, m_bLOD);
}

// FUNCTION: WIZ8 0x004caa40
signed char W8Monster::GetNumSubCycles()
{
    W8MonsterRep* representation = m_pRep;
    W8MonsterAnimationVector* cycle = &representation->animations[
        representation->selection.monster.current_cycle];
    W8AnimObj** slot = cycle->data;
    int subcycle = representation->selection.monster.current_subcycle;

    if (subcycle < cycle->count) {
        slot += subcycle;
    }

    return (signed char)AnimObjValue004A15D0(
        *slot, representation->m_bLOD);
}

/* W8Monster stores its animation object immediately after the shared
   0x1d8-byte GrCycle base. */
// FUNCTION: WIZ8 0x004c3740
unsigned char W8Monster::IsCycleSupported(signed char cycle)
{
    if (cycle >= W8_MONSTER_CYCLE_COUNT) {
        srAssertFail(
            "bCycle < CYCLE_NUM_UNIQUE",
            "C:\\Projects\\Wizardry 8\\Engine Code\\Monster.cpp",
            0xafc,
            "IsCycleSupported() -> Invalid cycle num.");
    }
    return m_pRep->animations[cycle].GetCount() != 0;
}

/* Select a concrete animation subcycle and rebuild the renderer-facing light
   and model state for it.  The one stack argument and RET 4 establish the
   primary-vtable slot as an ordinary signed-byte cycle setter. */
// FUNCTION: WIZ8 0x004c3790
void W8Monster::SetCycle(signed char cycle)
{
    W8MonsterAnimationVector* animations;
    W8MonsterLightVector* lights;
    W8AnimObj* animation;
    signed char subcycle;
    int count;
    int index;

    if (cycle < 0 || cycle >= W8_MONSTER_CYCLE_COUNT) {
        srAssertFail(
            "bCycle >= CYCLE_FIRST && bCycle <= CYCLE_LAST",
            MONSTER_CPP,
            0xb14,
            0);
    }

    animations = &m_pRep->animations[cycle];
    count = animations->GetCount();
    if (count == 0) {
        if (m_pRep->animations[1].GetCount() < 1) {
            W8MonsterInfo* monster_info =
                MonsterGetScriptPartByLocationIndex(
                    MonsterGetIndexByLocationID(
                        0xb26, MONSTER_CPP, propagated_value_1e4, 1));
            srAssertFail(
                "FALSE",
                MONSTER_CPP,
                0xb26,
                FormatString(
                    "ERROR: Monster %ls has no IDLE cycle!",
                    GetMonsterName(monster_info, 0, 0)));
            return;
        }

        W8MonsterInfo* monster_info =
            MonsterGetScriptPartByLocationIndex(
                MonsterGetIndexByLocationID(
                    0xb1d, MONSTER_CPP, propagated_value_1e4, 1));
        FormatDebugMessage(
            0,
            "WARNING: Monster %ls is missing anim cycle %s",
            GetMonsterName(monster_info, 0, 0),
            g_cycle_names[cycle].name);
        m_pRep->Method004BF0F0(cycle, m_pRep, 1);
        count = animations->GetCount();
        if (count == 0) {
            return;
        }
    }

    if (count == 1) {
        subcycle = 0;
    }
    else if (count < 2) {
        srAssertFail("0", MONSTER_CPP, 0xb43, 0);
        subcycle = 0;
    }
    else if (m_pRep->selection.monster.runtime_value_a6 == -1 ||
             count <= m_pRep->selection.monster.runtime_value_a6) {
        if ((flags_1dc & 0x10) == 0) {
            subcycle = (signed char)(GetTickCount() % count);
        }
        else {
            flags_1dc &= ~0x10;
            subcycle = m_pRep->selection.monster.current_subcycle;
        }
    }
    else {
        flags_1dc &= ~0x10;
        subcycle = m_pRep->selection.monster.runtime_value_a6;
        m_pRep->selection.monster.runtime_value_a6 = -1;
    }

    if (cycle_callback_230 != 0 &&
        callback_cycle_234 == m_pRep->selection.monster.current_cycle) {
        cycle_callback_230(this);
        cycle_callback_230 = 0;
    }

    if (m_pRep->selection.monster.current_cycle != 0) {
        lights = *m_pRep->light_lists[
            m_pRep->selection.monster.current_cycle].GetAt(
                m_pRep->selection.monster.current_subcycle);
        if (lights != 0) {
            count = lights->GetCount();
            for (index = 0; index < count; ++index) {
                stLight* light = *lights->GetAt(index);

                light->setParent(0, 1);
                if (light->definition() != 0) {
                    int world_index =
                        g_world->lights_to_update->IndexOf(light);
                    if (world_index != -1) {
                        g_world->lights_to_update->RemoveAt(world_index);
                    }
                }
            }
        }
    }

    m_pRep->selection.monster.current_cycle = cycle;
    m_pRep->selection.monster.current_subcycle = subcycle;
    animation = *animations->GetAt(subcycle);
    if (animation == 0) {
        srAssertFail("pao", MONSTER_CPP, 0xb51, 0);
    }
    m_pRep->flag_06f = animation->value_02;

    {
        srVector3T<double> camera_location = g_world->camera->getLocation();
        srVector3T<float> listener;
        listener.x = (float)camera_location.x;
        listener.y = (float)camera_location.y;
        listener.z = (float)camera_location.z;
        SelectLOD004A7BE0(&listener.x);
    }

    GetAnimationRadius(&m_pRep->value_0a8);
    if ((flags_1dc & 1) != 0) {
        flags_1dc &= ~1;
    }
    else {
        m_pRep->flag_06e = 1;
    }

    lights = *m_pRep->light_lists[cycle].GetAt(subcycle);
    SetLights(lights);
    if (lights != 0) {
        count = lights->GetCount();
        for (index = 0; index < count; ++index) {
            stLight* light = *lights->GetAt(index);

            light->setParent(g_world->dynamic_scene, 1);
            light->Reset0049D070();
            if (light->definition() != 0) {
                g_world->lights_to_update->Add(light);
            }
        }
    }

    flags_1dc &= ~2;
    m_pRep->counter_094 = 0;
    m_pRep->counter_095 = GetNumSubCycles() - 1;
    SetShakeEventVisibility004BF9E0(cycle);

    if (lights != 0) {
        count = lights->GetCount();
        for (index = 0; index < count; ++index) {
            if (enabled_1bd != 0) {
                (*lights->GetAt(index))->clearFlag(
                    srNode::FLAG_POSITIONAL_0);
            }
            else {
                (*lights->GetAt(index))->setFlag(
                    srNode::FLAG_POSITIONAL_0);
            }
        }
    }
    if (m_pRep->monster_light_624 != 0) {
        m_pRep->monster_light_624->SetVisible0049D970(enabled_1bd);
    }

    g_monster_model_instances_682fd0.Clear();
    CollectModelInstances004C6350(&g_monster_model_instances_682fd0);
    for (index = 0;
         index < g_monster_model_instances_682fd0.GetCount();
         ++index) {
        stModelInstance005EC7D0* model =
            *g_monster_model_instances_682fd0.GetAt(index);
        model->scale_194.x = state_2fc.scale_04;
        model->scale_194.y = state_2fc.scale_04;
        model->scale_194.z = state_2fc.scale_04;
    }

    if (cycle == 0x15) {
        W8AnimRepValue4 empty = {0, 0, 0, 0};
        srModelInstance* instance;

        m_pRep->value_04c = empty;
        instance = SelectCycleFrameLod004A8360(
            m_pRep->selection.monster.current_cycle,
            0,
            m_pRep->m_bLOD);
        if (instance != 0 && instance->model() != 0 &&
            strstr(instance->model()->getName(), "gib") != 0) {
            SetAngles004538F0(
                (float)(g_monster_death_rotation_pi_005ed1f0 *
                        g_float_005ebcf8 * Random(0x168)));
        }
        if (m_pRep->monster_light_624 != 0) {
            m_pRep->monster_light_624->StartFadeOut0049DAF0();
        }
    }
}

// FUNCTION: WIZ8 0x004c3dd0
signed char W8Monster::GetTotalAnimationCount()
{
    signed char total = 0;
    int cycle;

    for (cycle = 0; cycle < W8_MONSTER_CYCLE_COUNT; ++cycle) {
        total += (signed char)m_pRep->animations[cycle].GetCount();
    }
    return total;
}

// FUNCTION: WIZ8 0x004caa90
float W8Monster::GetCurrentAnimationScale()
{
    W8MonsterRep* representation = m_pRep;

    return *representation->animation_scales[
        representation->selection.monster.current_cycle].GetAt(
            representation->selection.monster.current_subcycle);
}

// FUNCTION: WIZ8 0x004cab00
W8EmitterHost* W8Monster::GetRepresentation()
{
    return m_pRep;
}

// FUNCTION: WIZ8 0x004c3df0
unsigned char W8Monster::GetAnimationBounds(
    srVector3T<float>* minimum, srVector3T<float>* maximum)
{
    unsigned char result;
    float scale;

    result = W8GrCycle::GetAnimationBounds(minimum, maximum);
    scale = m_pRep->scale_5f0;
    minimum->x *= scale;
    minimum->y *= scale;
    minimum->z *= scale;
    scale = m_pRep->scale_5f0;
    maximum->x *= scale;
    maximum->y *= scale;
    maximum->z *= scale;
    return result;
}

// FUNCTION: WIZ8 0x004c3ed0
unsigned char W8Monster::GetAnimationRadius(float* radius)
{
    unsigned char result = W8GrCycle::GetAnimationRadius(radius);

    *radius *= m_pRep->scale_5f0;
    return result;
}

static const float g_monster_bounds_vertical_factor_005ecd88 = 0.66f;

// FUNCTION: WIZ8 0x004c3e60
unsigned char W8Monster::GetAnimationCenter(srVector3T<float>* center)
{
    srVector3T<float> minimum;
    srVector3T<float> maximum;

    if (GetAnimationBounds(&minimum, &maximum) != 0) {
        srVector3T<float> position = GetPosition();

        center->x = position.x;
        center->y = position.y;
        center->z = position.z;
        center->y += (maximum.y - minimum.y) *
            g_monster_bounds_vertical_factor_005ecd88;
        return 1;
    }
    return 0;
}

/* Keep equipped items, linked items, and temporary poster model instances in
   the camera-facing attachment layout selected by the representation. */
// FUNCTION: WIZ8 0x004c3f70
void W8Monster::UpdateAttachedObjects004C3F70()
{
    W8MonsterRep* representation = m_pRep;
    int attachment_layout = representation->value_5c4;
    unsigned int linked_count =
        PLLength(representation->linked_objects_5e8);
    int poster_count = representation->linked_runtime_objects_614.GetCount();
    srMatrix3T<float> camera_rotation;
    srVector3T<float> base_position;
    srVector3T<float> party_position;
    float distance_scale;
    unsigned int elapsed;
    int index;

    if (attachment_layout == 0 && linked_count == 0 && poster_count == 0) {
        return;
    }

    elapsed = g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT) -
        representation->timer_068;
    WorldGetCameraRotation(g_world, &camera_rotation);
    base_position.x = fields.movement_0c0.position_040.x;
    base_position.y = fields.movement_0c0.position_040.y +
        fields.movement_0c0.vertical_base_07c +
        fields.movement_0c0.vertical_amplitude_080;
    base_position.z = fields.movement_0c0.position_040.z;

    GetCameraPosition(&party_position);
    party_position.x -= base_position.x;
    party_position.y -= base_position.y;
    party_position.z -= base_position.z;
    distance_scale = (float)sqrt(
        party_position.x * party_position.x +
        party_position.y * party_position.y +
        party_position.z * party_position.z) *
        g_monster_attachment_distance_scale_005ed2a8;
    if (flags_330.flag_01 == 0 && distance_scale > g_float_005ebb38) {
        distance_scale = g_float_005ebb38;
    }

    if (attachment_layout != 0) {
        for (index = 0; index < 8; ++index) {
            W8Item* item = representation->objects_5c8[index];

            if (item != 0) {
                const srVector3T<float>& source =
                    g_monster_attachment_offsets_0060e618[
                        attachment_layout - 1][index];
                srVector3T<float> offset;
                srVector3T<float> location;
                srNode* mesh;
                float mesh_scale;
                srVector3T<double> widened_scale;

                offset.x = source.x * distance_scale;
                offset.y = source.y * distance_scale;
                offset.z = source.z * distance_scale;
                location.x = base_position.x +
                    camera_rotation.vectors[0].x * offset.x +
                    camera_rotation.vectors[0].y * offset.y +
                    camera_rotation.vectors[0].z * offset.z;
                location.y = base_position.y + representation->value_5ec +
                    distance_scale * g_monster_attachment_vertical_scale_005eca84 +
                    camera_rotation.vectors[1].x * offset.x +
                    camera_rotation.vectors[1].y * offset.y +
                    camera_rotation.vectors[1].z * offset.z;
                location.z = base_position.z +
                    Function4218E0(camera_rotation.vectors[2], offset);

                item->Function49F720(&location);
                mesh = item->GetMesh();
                mesh_scale = distance_scale *
                    g_monster_attachment_scales_0060e914[attachment_layout];
                widened_scale.x = mesh_scale;
                widened_scale.y = mesh_scale;
                widened_scale.z = mesh_scale;
                mesh->setScale(widened_scale);
                if ((flags_1dc & 0x400) == 0) {
                    mesh->clearFlag(srNode::FLAG_POSITIONAL_0);
                }
                else {
                    mesh->setFlag(srNode::FLAG_POSITIONAL_0);
                }
                item->ApplyRepTransform0049FAA0();
            }
        }
    }

    index = 0;
    {
        int group = 0;

        while (index < (int)linked_count) {
            int chunk_count = (int)linked_count - index;
            int chunk_index;
            float group_height;

            if (chunk_count > 4) {
                chunk_count = 4;
            }
            group_height = (float)(group *
                g_monster_attachment_group_spacing_005ed2a0);

            for (chunk_index = 0; chunk_index < chunk_count;
                 ++chunk_index, ++index) {
                W8MonsterLinkedItem005E8* entry =
                    static_cast<W8MonsterLinkedItem005E8*>(
                        PLGet(representation->linked_objects_5e8, index));
                W8Item* item = entry->item_04;
                const srVector3T<float>& source =
                    g_monster_attachment_offsets_0060e618[
                        chunk_count - 1][chunk_index];
                srVector3T<float> offset;
                srVector3T<float> location;
                srNode* mesh;
                float mesh_scale;
                srVector3T<double> widened;

                offset.x = source.x * distance_scale;
                offset.y = (source.y + group_height) * distance_scale;
                offset.z = source.z * distance_scale;
                location.x = base_position.x +
                    camera_rotation.vectors[0].x * offset.x +
                    camera_rotation.vectors[0].y * offset.y +
                    camera_rotation.vectors[0].z * offset.z;
                location.y = base_position.y + representation->value_5ec +
                    distance_scale * g_monster_linked_vertical_scale_005ed29c +
                    camera_rotation.vectors[1].x * offset.x +
                    camera_rotation.vectors[1].y * offset.y +
                    camera_rotation.vectors[1].z * offset.z;
                location.z = base_position.z +
                    Function4218E0(camera_rotation.vectors[2], offset);

                item->Function49F720(&location);
                mesh = item->GetMesh();
                mesh_scale = distance_scale *
                    g_monster_attachment_scales_0060e914[chunk_count];
                widened.x = mesh_scale;
                widened.y = mesh_scale;
                widened.z = mesh_scale;
                mesh->setScale(widened);
                widened.x = location.x;
                widened.y = location.y;
                widened.z = location.z;
                mesh->setLocation(widened);
                if ((flags_1dc & 0x400) == 0) {
                    mesh->clearFlag(srNode::FLAG_POSITIONAL_0);
                }
                else {
                    mesh->setFlag(srNode::FLAG_POSITIONAL_0);
                }
            }
            ++group;
        }
    }

    if (poster_count != 0) {
        srVector3T<float> mapped_position;
        float vertical_offset = elapsed * g_monster_poster_vertical_rate_005ed298;
        int poster_index = 0;

        GetMappedPosition004C72A0(&mapped_position);
        while (poster_index < poster_count) {
            stModelInstance005EC7D0* poster =
                *representation->linked_runtime_objects_614.GetAt(poster_index);
            srVector3T<double> location = poster->getLocation();
            float x = (float)location.x;
            float y = (float)location.y + vertical_offset;
            float z = (float)location.z;
            float dx = x - mapped_position.x;
            float dy = y - mapped_position.y;
            float dz = z - mapped_position.z;

            if ((float)sqrt(dx * dx + dy * dy + dz * dz) <=
                (float)g_monster_poster_max_distance_005ec3d8) {
                location.x = x;
                location.y = y;
                location.z = z;
                poster->setLocation(location);
                ++poster_index;
            }
            else {
                representation->linked_runtime_objects_614.RemoveAt(
                    representation->linked_runtime_objects_614.IndexOf(poster));
                delete poster;
                --poster_count;
            }
        }
    }
}

/* Query the current animation state. The selector is an internal ten-entry
   interface used by MonsterManager and the animation driver; selector eight is
   intentionally unsupported and returns -1 with out-of-range selectors. */
// FUNCTION: WIZ8 0x004c4660
int W8Monster::Query(int query)
{
    int result = -1;
    unsigned int animation_value;

    switch (query) {
    case 0:
        result = m_pRep->ApplyEmitterSetting(
            m_pRep->selection.monster.current_cycle);
        break;
    case 1:
        result = GetTotalAnimationCount();
        break;
    case 2:
        if (m_pRep->flag_06e != 1 && m_pRep->flag_06e != 2) {
            result = m_pRep->flag_064 == 0;
            break;
        }
        animation_value = m_pRep->ApplyEmitterSetting(
            m_pRep->selection.monster.current_cycle);
        result = m_pRep->flag_064 == animation_value - 1;
        break;
    case 3:
        if (m_pRep->flag_06e == 1 || m_pRep->flag_06e == 2) {
            result = m_pRep->flag_064 == 0;
            break;
        }
        animation_value = m_pRep->ApplyEmitterSetting(
            m_pRep->selection.monster.current_cycle);
        result = m_pRep->flag_064 == animation_value - 1;
        break;
    case 4:
        result = m_pRep->flag_064;
        break;
    case 5:
        result = m_pRep->ApplyEmitterSetting(
            m_pRep->selection.monster.current_cycle) != (unsigned int)-1;
        break;
    case 6:
        result = m_pRep->selection.monster.current_cycle;
        break;
    case 7:
        if (m_pRep->flag_070 == 3) {
            if (m_pRep->flag_06d == 0) {
                result = 1;
            }
            break;
        }

        animation_value = m_pRep->ApplyEmitterSetting(
            m_pRep->selection.monster.current_cycle);
        if ((m_pRep->flag_06e == 1 &&
             m_pRep->flag_064 == animation_value - 1) ||
            (m_pRep->flag_06e == 3 && m_pRep->flag_064 == 0) ||
            m_pRep->flag_06e == 4 || m_pRep->flag_06e == 2) {
            result = 1;
        }
        break;
    case 9:
        result = m_pRep->selection.monster.current_subcycle;
        break;
    }
    return result;
}

// FUNCTION: WIZ8 0x004c32e0
void W8Monster::AdvanceAnimationFrame(int value, int)
{
    unsigned char previous_frame = m_pRep->flag_064;

    W8GrCycle::AdvanceAnimationFrame(value, 0);
    if ((m_pRep->selection.monster.current_cycle == 7 ||
         m_pRep->selection.monster.current_cycle == 13 ||
         m_pRep->selection.monster.current_cycle == 17) &&
        ((value_1f4 > 0 && previous_frame < value_1f4 &&
          value_1f4 <= m_pRep->flag_064) ||
         (value_1f4 == 0 && m_pRep->flag_064 == 1))) {
        HandleAnimationThreshold004C75C0();
    }
    HandleAnimationFrame004C74D0(previous_frame);
    if (m_plsParticles != 0 && m_plsParticles->GetCount() != 0) {
        UpdateShakeEvents004C3380(previous_frame);
    }
}

extern unsigned int MonsterGetIndexByLocationID(
    int caller_line, const char* caller_file, int location_id,
    unsigned char assert_on_failure);
extern W8MonsterInfo* MonsterGetScriptPartByLocationIndex(unsigned int index);
extern unsigned int MonsterCastsSpell(
    W8MonsterInfo* monster_info, int spell_id, unsigned int power_level);
extern void FatigueMonster(
    W8MonsterInfo* monster_info, unsigned int amount, int report_to);
extern int g_spell_effect_frame_0064c158;
extern int g_spell_index_0069b7dc;
extern void* CreateSpellEffect004AD8A0(
    const char* mls_name, int frame, W8Monster* parent, int value, int flags);
extern void SetTargetSourceToMonster(
    const W8MonsterInfo* monster_info, W8TargetSource* source);
extern void ClearAttackBlock(void* block);
extern unsigned int ChooseAttackMode(unsigned int attack_modes);
extern int CalculateMonsterMissileAccuracy(
    W8MonsterInfo* monster_info, const W8MonsterAttack* attack,
    int attack_mode, int flags);
extern void CombatLog(const char* format, ...);
extern void FireMissileSourceToTarget(
    int missile_type, W8TargetSource* source, W8CombatSlot* target,
    void* attack_block, unsigned char use_default_accuracy,
    unsigned int range_category, int accuracy);
extern unsigned int g_missile_table_count_65bddc;

// VTABLE: WIZ8 0x005ed288
// class W8MonsterShakeCallback

// VTABLE: WIZ8 0x005ed290
// class W8MonsterShakeCallbackBase

// SYNTHETIC: WIZ8 0x004c3710
// W8MonsterShakeCallback::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x004c3730
// W8MonsterShakeCallback::~W8MonsterShakeCallback

// SYNTHETIC: WIZ8 0x004cab40
// W8MonsterShakeCallbackBase::`scalar deleting destructor'

/* Cycle 25 launches either the queued spell visual or the monster's pending
   spell action when its animation crosses the configured frame. The cast
   returns the stamina charge; passing that value straight to FatigueMonster
   is why MonsterCastsSpell cannot have the void return type previously used
   by Magic.cpp. */
// FUNCTION: WIZ8 0x004c74d0
void W8Monster::HandleAnimationFrame004C74D0(unsigned char previous_frame)
{
    W8MonsterInfo* monster_info;
    int action_kind;
    int action_detail;
    unsigned int power_level;
    unsigned int fatigue;

    if (m_pRep->selection.monster.current_cycle == 25 &&
        ((value_1f8 > 0 && previous_frame < value_1f8 &&
          value_1f8 <= m_pRep->flag_064) ||
         (value_1f8 == 0 && m_pRep->flag_064 == 1))) {
        if (state_2fc.unknown_08[0] != 0) {
            state_2fc.unknown_08[0] = 0;
            CreateSpellEffect004AD8A0(
                g_spell_records[g_spell_index_0069b7dc].resource_name,
                g_spell_effect_frame_0064c158, this, 0, 0);
            return;
        }

        monster_info = MonsterGetScriptPartByLocationIndex(
            MonsterGetIndexByLocationID(
                0x1804, MONSTER_CPP, propagated_value_1e4, 1));
        action_kind = monster_info->action_kind;
        action_detail = monster_info->action_detail;
        power_level = *(unsigned int*)monster_info->unknown_2e9;
        if (action_kind == 2 && action_detail != 0 && power_level != 0) {
            fatigue = MonsterCastsSpell(
                monster_info, action_detail, power_level);
            FatigueMonster(monster_info, fatigue, 0);
            monster_info->unknown_2df[1] = 1;
        }
    }
}

struct W8MonsterMissileAttackBlock {
    int unknown_00;
    int missile_value_04;
    unsigned char missile_values_08[0x10];
    int monster_value_18;
    int missile_value_1c;
    unsigned char unknown_20[0x10];
};

static_assert(
    sizeof(W8MonsterMissileAttackBlock) == 0x30,
    "W8MonsterMissileAttackBlock_size_must_be_0x30");

/* Launch the missile at the frame shared by attack cycles 7, 13 and 17. In
   combat an already-selected attack is reused; otherwise the monster picks a
   live character and the first database attack that permits a missile mode. */
// FUNCTION: WIZ8 0x004c75c0
void W8Monster::HandleAnimationThreshold004C75C0()
{
    W8MonsterInfo* monster_info;
    W8MonsterRecord* record;
    W8TargetSource source;
    W8MonsterMissileAttackBlock attack_block;
    const W8MonsterAttack* attack;
    unsigned int attack_index;
    unsigned int range_category;
    int missile_type;
    int accuracy;
    unsigned char selected_attack;
    unsigned char monster_value;

    selected_attack = 0;
    monster_info = MonsterGetScriptPartByLocationIndex(
        MonsterGetIndexByLocationID(
            0x1834, MONSTER_CPP, propagated_value_1e4, 1));
    record = GetMonsterDataForInfo(monster_info);
    SetTargetSourceToMonster(monster_info, &source);

    if (g_in_combat_00683f94 != 0 &&
        g_combat_state->selected_slot == 2 &&
        g_combat_state->selected_monster == monster_info) {
        attack_index = monster_info->pCombat->attack_index_11;
        selected_attack = 1;
        goto prepare_attack;
    }

    monster_info->combat_slot_2ba.iType = W8_TARGET_KIND_CHARACTER;
    monster_info->combat_slot_2ba.iChar = GetRandomCharacter(1, 1, -1, -1);
    monster_info->combat_slot_2ba.iMonsterID = -1;
    if (monster_info->combat_slot_2ba.iChar == -1) {
        return;
    }

    for (attack_index = 0; attack_index < W8_MAX_MONSTER_ATTACKS;
         ++attack_index) {
        attack = &record->attacks[attack_index];
        if (attack->fHasAttack != 0 &&
            (attack->attack_modes & 0x110) != 0) {
            monster_info->action_detail = ChooseAttackMode(attack->attack_modes);
            goto prepare_attack;
        }
    }

    missile_type = 0;
    range_category = 3;
    ClearAttackBlock(&attack_block);
    accuracy = 50;
    goto fire_missile;

prepare_attack:
    if (attack_index >= W8_MAX_MONSTER_ATTACKS) {
        srAssertFail(
            "uiAttack < MAX_MONSTER_ATTACKS", MONSTER_CPP, 0x1862, 0);
    }
    attack = &record->attacks[attack_index];
    missile_type = attack->missile_type;
    if ((unsigned int)missile_type >= g_missile_table_count_65bddc) {
        FormatDebugMessage(
            0, "WARNING: %ls has invalid missile type %d for attack %d",
            record, missile_type, attack_index);
        missile_type = 0;
    }

    range_category = attack->range_category;
    ClearAttackBlock(&attack_block);
    attack_block.missile_value_04 = attack->missile_value_17;
    memcpy(attack_block.missile_values_08, attack->missile_values_05, 0x10);
    monster_value = record->missile_value_24f;
    attack_block.monster_value_18 =
        monster_value + (monster_value < 15 ? monster_value : 15);
    attack_block.missile_value_1c = attack->missile_value_1b;

    if (selected_attack != 0) {
        accuracy = CalculateMonsterMissileAccuracy(
            monster_info, attack, monster_info->action_detail, 0);
        CombatLog("TO HIT: MISSILE ACCURACY = %d%%\n", accuracy);
    } else {
        accuracy = 50;
    }

fire_missile:
    monster_info->unknown_2df[0] = 1;
    FireMissileSourceToTarget(
        missile_type, &source, &monster_info->combat_slot_2ba, &attack_block,
        selected_attack == 0, range_category, accuracy);
}

// FUNCTION: WIZ8 0x004c3620
void W8MonsterShakeCallback::RestoreAnimation()
{
    W8MonsterRep* representation;

    if (m_pMonster == 0) {
        srAssertFail("m_pMonster", MONSTER_CPP, 0x102, 0);
    }
    if (m_pParticles == 0) {
        srAssertFail("m_pParticles", MONSTER_CPP, 0x103, 0);
    }

    m_pParticles->callback_26c = 0;
    m_pParticles->SetActive(0);
    representation = m_pMonster->m_pRep;
    if (saved_behaviour < 1 || saved_behaviour > 3) {
        srAssertFail(
            "bBehaviour >= BEHAVIOUR_FIRST && bBehaviour <= BEHAVIOUR_LAST",
            "..\\Engine Code\\Include\\AnimRep.hpp", 0x87, 0);
    }
    representation->behaviour_071 = saved_behaviour;
    representation->SetFrameMethod004B55C0(saved_frame_method);
    representation->flag_06e = 1;
    representation->counter_094 = 0;
    representation->counter_095 = m_pMonster->GetNumSubCycles() - 1;
    delete this;
}

/* Drive the particles attached to the active cycle/subcycle. A particle with
   no distinct frame range fires when the animation crosses its own start
   frame; a ranged particle is switched on and off at its explicit bounds. */
// FUNCTION: WIZ8 0x004c3380
void W8Monster::UpdateShakeEvents004C3380(unsigned char previous_frame)
{
    W8AnimObj* animation;
    W8GrCycleShakeEvent* event;
    stParticle* particle;
    W8MonsterShakeCallback* callback;
    int count;
    int index;
    unsigned char animation_has_range;

    count = m_plsParticles->GetCount();
    animation = *m_pRep->animations[
        m_pRep->selection.monster.current_cycle].GetAt(
            m_pRep->selection.monster.current_subcycle);
    animation_has_range =
        animation != 0 && animation->start_frame_14 != 0 &&
        animation->end_frame_15 >= animation->start_frame_14;

    for (index = 0; index < count; ++index) {
        event = *m_plsParticles->GetAt(index);
        if (event->cycle_00 != m_pRep->selection.monster.current_cycle ||
            event->subcycle_04 !=
                m_pRep->selection.monster.current_subcycle) {
            continue;
        }

        particle = event->particle_08;
        if (animation_has_range != 0 &&
            (particle->start_frame_264 == -1 ||
             particle->end_frame_268 == -1 ||
             particle->start_frame_264 == particle->end_frame_268) &&
            previous_frame < animation->start_frame_14 &&
            animation->start_frame_14 <= m_pRep->flag_064) {
            if (enabled_1bd != 0) {
                particle->SetActive(1);
                particle->value_188 = 0;
                if (index == 0) {
                    callback = new W8MonsterShakeCallback;
                    callback->m_pMonster = this;
                    callback->m_pParticles = particle;
                    callback->saved_behaviour = m_pRep->flag_070;
                    callback->saved_frame_method = m_pRep->flag_06f;
                    particle->callback_26c = callback;

                    m_pRep->behaviour_071 = 3;
                    if (animation->start_frame_14 == animation->end_frame_15) {
                        m_pRep->SetFrameMethod004B55C0(4);
                        m_pRep->flag_06e = 1;
                    } else {
                        m_pRep->SetFrameMethod004B55C0(2);
                        m_pRep->counter_094 = animation->start_frame_14;
                        if (animation->end_frame_15 < GetNumSubCycles()) {
                            m_pRep->counter_095 = animation->end_frame_15;
                        } else {
                            m_pRep->counter_095 = GetNumSubCycles() - 1;
                        }
                    }
                }
            }
            continue;
        }

        if (particle->start_frame_264 != -1 &&
            particle->end_frame_268 != -1 &&
            particle->start_frame_264 != particle->end_frame_268) {
            if ((unsigned int)previous_frame ==
                (unsigned int)particle->start_frame_264) {
                if (enabled_1bd != 0) {
                    particle->SetActive(1);
                    particle->value_188 = 0;
                }
            } else if ((unsigned int)m_pRep->flag_064 ==
                       (unsigned int)particle->end_frame_268) {
                particle->SetActive(0);
            }
        }
    }
}

// FUNCTION: WIZ8 0x004cab10
W8AnimObj* W8Monster::GetCurrentAnimation()
{
    W8MonsterRep* representation = m_pRep;

    return *representation->animations[
        representation->selection.monster.current_cycle].GetAt(
            representation->selection.monster.current_subcycle);
}

// FUNCTION: WIZ8 0x004caac0
void W8Monster::SetCurrentAnimationScale(float scale)
{
    W8MonsterRep* representation = m_pRep;

    *representation->animation_scales[
        representation->selection.monster.current_cycle].GetAt(
            representation->selection.monster.current_subcycle) = scale;
}

/* Resolve the active cycle/subcycle AnimObj and submit entry zero using the
   Monster's animation index. */
// FUNCTION: WIZ8 0x004c3f00
W8AniMesh* W8Monster::GetCurrentAniMesh()
{
    typedef void* (__cdecl *LegacyAnimObjEntryCall)(
        W8AnimObj*, signed char, unsigned int);

    int cycle_index =
        m_pRep->selection.monster.current_cycle;
    int subcycle_index =
        m_pRep->selection.monster.current_subcycle;
    W8MonsterAnimationVector* cycle = &m_pRep->animations[cycle_index];
    W8AnimObj** animation_slot;
    W8AnimObj* animation;

    if (subcycle_index < cycle->GetCount()) {
        animation_slot = cycle->data + subcycle_index;
    } else {
        animation_slot = cycle->data;
    }
    animation = *animation_slot;
    if (animation == 0) {
        srAssertFail(
            "pao",
            "C:\\Projects\\Wizardry 8\\Engine Code\\Monster.cpp",
            0xc4e,
            0);
    }
    /* This canonical caller passes the legacy three-argument call shape to
       0x004A1660 even though that callee's own body has a four-slot prototype.
       Preserve the observed caller ABI locally rather than weakening the
       callee's reviewed declaration. */
    return (W8AniMesh*)((LegacyAnimObjEntryCall)AnimObjEntry004A1660)(
        animation, animationIndex(), 0);
}

/* Store one value in the two cycle records used as its compact mirrors, then
   propagate it to every attached object's +0x28 field.  The body consumes two
   cdecl arguments; callers that reserve another stack slot clean it themselves. */
// FUNCTION: WIZ8 0x004c5870
void MonsterPropagateValue004C5870(W8Monster* monster, int value)
{
    int index;
    int count;

    if (monster == 0) {
        srAssertFail(
            "pMonster",
            "C:\\Projects\\Wizardry 8\\Engine Code\\Monster.cpp",
            0x125f,
            0);
    }
    monster->propagated_value_1e4 = value;
    monster->fields.movement_0c0.location_id_004 = (unsigned short)value;
    if (monster->m_plsSoundEvents != 0) {
        count = monster->m_plsSoundEvents->GetCount();
        index = 0;
        if (count > 0) {
            do {
                int propagated_value = monster->propagated_value_1e4;
                W8VectorElement005ED094* object =
                    *monster->m_plsSoundEvents->GetAt(index);
                ++index;
                object->value_028 = propagated_value;
            } while (index < count);
        }
    }
}

// FUNCTION: WIZ8 0x004c5710
bool MonsterHasPendingCycle(W8Monster* monster)
{
    return monster->m_pRep->selection.monster.pending_cycle != -1;
}

extern srModelInstance* GetValue65962C(void);

/* Compare the cycle's selected frame against the renderer's typed current-model
   slot.  Prop.cpp independently compares that slot with srModelInstance values. */
// FUNCTION: WIZ8 0x004c56f0
unsigned char MonsterUsesCurrentModelInstance(W8GrCycle* cycle)
{
    srModelInstance* current = cycle->GetCurrentModelInstance004A8250();
    return current == GetValue65962C();
}

// FUNCTION: WIZ8 0x004c5730
void MonsterGetLocation(
    W8Monster* monster, srVector3T<float>* location)
{
    monster->m_pRep->GetLocation004B8890(location);
}

// FUNCTION: WIZ8 0x004c5750
void MonsterGetLocalLocation(
    W8Monster* monster, srVector3T<float>* location)
{
    monster->m_pRep->GetLocalLocation004B88B0(location);
}

/* The wrapper is intentionally unguarded: every retail caller supplies a live
   Monster, and the original immediately dispatches through slot 16. */
// FUNCTION: WIZ8 0x004c59a0
void UpdateMonster(W8Monster* monster)
{
    monster->Update();
}

// FUNCTION: WIZ8 0x004c5a80
unsigned char MonsterIsCycleSupported(
    W8Monster* monster, signed char cycle)
{
    if (monster != 0) {
        return monster->IsCycleSupported(cycle);
    }
    return 0;
}

// FUNCTION: WIZ8 0x004c5b10
unsigned char MonsterReplacePath(W8Monster* monster, void* path)
{
    if (monster != 0) {
        return monster->ReplacePath004A8400(path);
    }
    return 0;
}

/* Reset pitch, update the Navigator's facing, and rebuild the representation's
   complete yaw/pitch/roll matrix.  The explicit vector and matrix operations
   reproduce the ordinary SurRender math calls retained in this large body. */
// FUNCTION: WIZ8 0x004c5b60
void MonsterSetFacing004C5B60(W8Monster* monster, float angle)
{
    srMatrix3T<float> rotation;
    srMatrix3T<float> adjustment;
    srVector3T<float> first;
    srVector3T<float> second;
    srVector3T<float> third;
    double sine;
    double cosine;

    if (monster == 0) {
        return;
    }

    monster->SetAngles004538F0(NormalizeAngle(angle));
    monster->SetPitch(0.0f);

    rotation.vectors[0].x = 1.0f;
    rotation.vectors[0].y = 0.0f;
    rotation.vectors[0].z = 0.0f;
    rotation.vectors[1].x = 0.0f;
    rotation.vectors[1].y = 1.0f;
    rotation.vectors[1].z = 0.0f;
    rotation.vectors[2].x = 0.0f;
    rotation.vectors[2].y = 0.0f;
    rotation.vectors[2].z = 1.0f;

    angle = NormalizeAngle(
        monster->GetYaw() +
        g_monster_rotation_offset_005ec04c);
    if ((double)angle != g_zero_005ebb40) {
        cosine = cos((double)angle);
        sine = sin((double)angle);
        third.method_00421680(-sine, 0.0, cosine);
        second.method_00421680(0.0, 1.0, 0.0);
        first.method_00421680(cosine, 0.0, sine);
        adjustment.method_004219F0(first, second, third);
        rotation.method_00421A40(adjustment);
    }

    angle = monster->GetPitch();
    if (angle != g_float_005ebb34 &&
        (double)angle != g_zero_005ebb40) {
        cosine = cos((double)angle);
        sine = sin((double)angle);
        third.method_00421680(0.0, sine, cosine);
        second.method_00421680(0.0, cosine, -sine);
        first.method_00421680(1.0, 0.0, 0.0);
        adjustment.method_004219F0(first, second, third);
        rotation.method_00421A40(adjustment);
    }

    angle = monster->fields.movement_0c0.roll_028;
    if (angle != g_float_005ebb34 &&
        (double)angle != g_zero_005ebb40) {
        cosine = cos((double)angle);
        sine = sin((double)angle);
        third.method_00421680(0.0, 0.0, 1.0);
        second.method_00421680(sine, cosine, 0.0);
        first.method_00421680(cosine, -sine, 0.0);
        adjustment.method_004219F0(first, second, third);
        rotation.method_00421A40(adjustment);
    }

    monster->m_pRep->SetRotation004B88D0(&rotation);
}

// FUNCTION: WIZ8 0x004c5e80
unsigned char MonsterGetAnimationRadius(W8Monster* monster, float* radius)
{
    if (monster != 0) {
        return monster->GetAnimationRadius(radius);
    }
    return 0;
}

// FUNCTION: WIZ8 0x004c6180
void MonsterSetCycle(W8Monster* monster, signed char cycle)
{
    if (monster != 0) {
        monster->SetCycle(cycle);
    }
}

/* Cycle 17's third state byte is preserved by ActivateMonster while the live
   engine object is rebuilt, then restored into the replacement. */
// FUNCTION: WIZ8 0x004c57f0
unsigned char MonsterGetCycle17State(W8Monster* monster)
{
    return monster->unknown_1be;
}

// FUNCTION: WIZ8 0x004c5800
void MonsterSetCycle17State(W8Monster* monster, unsigned char state)
{
    monster->unknown_1be = state;
}

// FUNCTION: WIZ8 0x004c5820
unsigned char MonsterGetRuntimeFlag5BC(W8Monster* monster)
{
    return monster->m_pRep->flag_5bc;
}

// FUNCTION: WIZ8 0x004c5840
void MonsterSetRuntimeFlag5BC(W8Monster* monster, unsigned char flag)
{
    monster->m_pRep->flag_5bc = flag;
}

/* Cycle 18's pointee carries the scale at +0x5f0. Both accessors reach it the
   same way - through the pointer at the cycle's +0x0c, which 0x004E60B0 also
   reads a byte from - so the pointee is a shared engine object rather than
   anything the cycle owns. It is not modelled: only this one field is known. */
// FUNCTION: WIZ8 0x004c5780
float MonsterGetScale(W8Monster* monster)
{
    return monster->m_pRep->scale_5f0;
}

// FUNCTION: WIZ8 0x004c57a0
void MonsterSetScale(W8Monster* monster, float scale)
{
    monster->m_pRep->scale_5f0 = scale;
}

// FUNCTION: WIZ8 0x004c57c0
void MonsterGetScaleRange(W8Monster* monster, float* minimum, float* maximum)
{
    W8MonsterRep* runtime = monster->m_pRep;

    *minimum = runtime->minimum_scale_5f4;
    *maximum = runtime->maximum_scale_5f8;
}

/* Returns the previous animation state and timestamps every update through the
   recovered shared SurRender timer. */
// FUNCTION: WIZ8 0x004c5a00
unsigned char MonsterSetAnimating(W8Monster* monster, unsigned char animating)
{
    if (monster != 0) {
        W8MonsterRep* runtime = monster->m_pRep;
        unsigned char previous = runtime->flag_06d;

        runtime->flag_06d = animating;
        runtime->timer_068 =
            g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT);
        return previous;
    }
    return 0;
}

// FUNCTION: WIZ8 0x004c59e0
unsigned char MonsterIsAnimating(W8Monster* monster)
{
    if (monster != 0) {
        return monster->m_pRep->flag_06d;
    }
    return 0;
}

/* Cycle 19 bit 5 blocks pending-cycle changes. Otherwise the request is stored
   as the signed low byte in cycle 18's runtime record. */
// FUNCTION: WIZ8 0x004c5aa0
void MonsterSetPendingCycle(W8Monster* monster, int cycle)
{
    if (monster != 0 && ((monster->flags_1dc >> 5) & 1) == 0) {
        monster->m_pRep->selection.monster.pending_cycle = (signed char)cycle;
    }
}

// FUNCTION: WIZ8 0x004c5e40
void MonsterSetRuntimeBehaviour(W8Monster* monster, signed char behaviour)
{
    if (monster != 0) {
        if (behaviour < 1 || behaviour > 3) {
            srAssertFail(
                "bBehaviour >= BEHAVIOUR_FIRST && bBehaviour <= BEHAVIOUR_LAST",
                "..\\Engine Code\\Include\\AnimRep.hpp",
                0x87,
                0);
        }
        monster->m_pRep->behaviour_071 = behaviour;
    }
}

// FUNCTION: WIZ8 0x004c5ee0
unsigned char MonsterHasCycle19Flag3(W8Monster* monster)
{
    if (monster != 0) {
        return (monster->flags_1dc >> 3) & 1;
    }
    return 0;
}

// FUNCTION: WIZ8 0x004c6160
void MonsterSetStateA0(W8Monster* monster, unsigned char state)
{
    if (monster != 0) {
        monster->fields.state_088 = state;
    }
}

/* Named by the MonsterManager assertions. A null monster answers -1 rather than
   forwarding, which is how the callers tell "no monster" from a real result. */
// FUNCTION: WIZ8 0x004c5b40
int MonsterQuery(W8Monster* monster, int query)
{
    if (monster != NULL) {
        return monster->Query(query);
    }
    return -1;
}

// FUNCTION: WIZ8 0x004ca4c0
unsigned char W8Monster::IsDying()
{
    unsigned char dying = Query(6) == 0x15 ||
                          m_pRep->selection.monster.pending_cycle == 0x15;

    return dying;
}

/* Resolve mapped vertex zero on the current model and transform it into world
   space. Models without that mapping use the Navigator position plus the
   Monster's vertical offset. */
// FUNCTION: WIZ8 0x004c72a0
void W8Monster::GetMappedPosition004C72A0(srVector3T<float>* position)
{
    srModelInstance* instance = GetCurrentModelInstance004A8250();

    if (instance != 0) {
        stMeshModel* mesh = static_cast<stMeshModel*>(instance->model());
        while (mesh != 0) {
            int index = mesh->FindMappedIndex(0);

            if (index >= 0) {
                srVector3T<float>* vertices;
                if ((mesh->flags_3a0 & 4) == 0) {
                    vertices = mesh->getVertexLoc();
                }
                else {
                    vertices = mesh->GetVertexLocations00471AD0(0, 1, 0.0f);
                }
                if (vertices != 0) {
                    srMatrix4T<float> matrix;
                    float x;
                    float y;
                    float z;

                    position->x = vertices[index].x;
                    position->y = vertices[index].y;
                    position->z = vertices[index].z;
                    instance->getWorldSpaceMatrix(matrix);
                    x = position->x;
                    y = position->y;
                    z = position->z;
                    position->y = matrix.vectors[1].x * x +
                                  matrix.vectors[1].y * y +
                                  matrix.vectors[1].z * z +
                                  matrix.vectors[1].w;
                    position->x = matrix.vectors[0].x * x +
                                  matrix.vectors[0].y * y +
                                  matrix.vectors[0].z * z +
                                  matrix.vectors[0].w;
                    position->z = matrix.vectors[2].x * x +
                                  matrix.vectors[2].y * y +
                                  matrix.vectors[2].z * z +
                                  matrix.vectors[2].w;
                    return;
                }
            }
            mesh = mesh->next;
        }
    }

    position->x = fields.movement_0c0.position_040.x;
    position->y = fields.movement_0c0.position_040.y;
    position->z = fields.movement_0c0.position_040.z;
    position->y += fields.movement_0c0.height_offset_0b8;
}

/* Six thin bodies over the live animation object. Each is a null check and a
   forward, or a single member read; nothing here says what the members and
   slots are for, so each is named for what it reaches. */

extern void Function4C4DE0(int arg_1, int arg_2, int arg_3);
/* Neither takes an argument nor reads ECX: both work entirely over the pair of
   globals at 0x00659B34 and 0x00659B3C, which is what makes them free
   functions rather than the Navigator methods their neighbours in the same
   address range are. */
extern void Function453160(void);
extern void Function4531A0(void);
/* Cleans its own argument - the caller at 0x004C5A40 pushes and never adjusts
   afterwards - so it is __stdcall and not the cdecl the decompiler assumes. */
/* The location-id lookup pair, spelled as Magic Effects.cpp already declares
   it: the index comes first and the script part is fetched from it. */
extern unsigned int MonsterGetIndexByLocationID(
    int caller_line, const char* caller_file, int location_id, unsigned char assert_on_failure);
extern W8MonsterInfo* MonsterGetScriptPartByLocationIndex(unsigned int index);
extern unsigned int MonsterCastsSpell(
    W8MonsterInfo* monster_info, int spell_id, unsigned int power_level);
extern void FatigueMonster(
    W8MonsterInfo* monster_info, unsigned int amount, int report_to);

/* The caller proves only the roles below: the first global selects a frame in
   the spell animation, and the second indexes g_spell_records. Their original
   descriptive names have not been recovered. */
extern int g_spell_effect_frame_0064c158;
extern int g_spell_index_0069b7dc;
extern void* CreateSpellEffect004AD8A0(
    const char* mls_name, int frame, W8Monster* parent, int value, int flags);

/* 0x00421070, owned by the 0041F261-0042403F quarantine: the shared reference
   position every consumer of the object at 0x0065A0F8 reads. */
extern void GetCameraPosition(srVector3T<float>* position);
extern void Function4A84A0(W8GrCycle* monster);
/* Spelled the way MonsterManager.cpp already declares it: the callee takes its
   receiver in ECX, which __fastcall is how a no-argument member call is
   reachable from a free declaration. The receiver is the monster's Navigator
   base at +0x18. */
extern void __fastcall Function4537E0(W8Navigator* navigator);

/* Copies a position into a local and hands the local on. The monster argument
   is dead beyond its own null check - the callee never receives it - which is
   the same shape the other guarded forwarders here take, except that what
   survives the guard is the copy rather than the object.
   The copy goes through the FPU one component at a time - `fld dword` then
   `fstp dword` per component - rather than as the three integer moves VC6
   emits for a plain three-float assignment, which is what this body still gets
   and the whole of its remaining difference. That shape is the signature of
   srVector3T<float>::method_00421680 expanded inline: its parameters are
   doubles, so each float round-trips through the FPU instead of being copied
   as bits. The image carries both an out-of-line COMDAT copy of that setter at
   0x00421680 and this inlined expansion, which is the multiple-translation-unit
   visibility the inlining policy asks for before a body moves into a header.

   That was measured rather than argued. Defining the setter in srMath.h and
   calling it here reproduces the copy exactly - the three fld/fstp pairs land
   instruction for instruction, leaving only a register choice and one
   scheduling swap - and takes this body from 0.375 to 0.8125. It also stops
   VC6 emitting the out-of-line copy at all, because this is the only call site
   in the tree and it inlines: 0x00421680 goes from exact to missing. The
   inlining policy requires the bundle to improve without regressing an exact
   boundary, so the trade is refused and the out-of-line definition stays.
   Hand-spelling the conversion does not work either - `(float)(double)f` is
   value-preserving, so VC6 folds it straight back to the integer copy.
   Reproducing both emissions needs a second call site that does not inline,
   which is not decidable from this one; the filed bead tracks it. */
// FUNCTION: WIZ8 0x004c5a40
void MonsterForward4A7BE0(W8Monster* monster, const srVector3T<float>* position)
{
    srVector3T<float> local;

    if (monster != 0) {
        local.x = position->x;
        local.y = position->y;
        local.z = position->z;
        monster->SelectLOD004A7BE0(&local.x);
    }
}

/* Records a value on the cycle runtime and, when nothing is pending, seeds the
   pending cycle from the runtime's own fallback at 0x0a4 rather than leaving it
   at -1. The runtime pointer is fetched twice rather than held in a local -
   the second `mov` reloads it from the cycle - which is what says the original
   spelled the two reaches out separately instead of naming the record once. */
// FUNCTION: WIZ8 0x004c6c00
void W8Monster::SetRuntimeValueA6(signed char value)
{
    m_pRep->selection.monster.runtime_value_a6 = value;
    if (m_pRep->selection.monster.pending_cycle == -1) {
        m_pRep->selection.monster.pending_cycle =
            m_pRep->selection.monster.current_cycle;
    }
}

/* Writes the sixteen-byte block the cycle runtime carries at 0x4c, but only for
   a monster that is neither absent nor already answering the dying cycle to
   query six - the same 0x15 the death test compares against, reached the same
   way. Both guards leave through one shared exit, which is why the body has a
   single epilogue despite testing two things. The block arrives by value and is
   stored as one assignment. */
// FUNCTION: WIZ8 0x004c5ad0
void MonsterSetRuntimeBlock4C(W8Monster* monster, W8MonsterRuntimeBlock4C block)
{
    if (monster != 0 && monster->Query(6) != 0x15) {
        monster->m_pRep->value_04c = block;
    }
}


/* The engine object a monster holds at 0x0c, or nothing when there is no
   monster to ask. */
// FUNCTION: WIZ8 0x004c5b30
void* MonsterGetObject0C(W8Monster* monster)
{
    if (monster != 0) {
        return monster->m_pAI;
    }
    return 0;
}

/* Expose the first Navigator angle through the enclosing Monster. */
// FUNCTION: WIZ8 0x004c5770
float MonsterGetAngleD4004C5770(W8Monster* monster)
{
    return monster->GetYaw();
}

/* Two null-checked forwards that share one shape: a monster that is not there
   is simply not acted on. */
// FUNCTION: WIZ8 0x004c5ea0
void MonsterForward4A84A0(W8Monster* monster)
{
    if (monster != 0) {
        Function4A84A0(monster);
    }
}

// FUNCTION: WIZ8 0x004c6140
void MonsterForward4537E0(W8Monster* monster)
{
    if (monster != 0) {
        Function4537E0(monster);
    }
}

/* Two more of the same null-checked shape, except that what they forward to is
   already recovered: both callees are W8GrCycle setters GrCycle.cpp owns, and
   both are reached as methods rather than as free functions - the receiver
   stays in ECX across the guard and only the value is pushed. That is what
   types the parameter as the cycle rather than as the opaque pointer the
   neighbouring forwarders take. */
// FUNCTION: WIZ8 0x004c61a0
void MonsterSetCycleBehaviour(W8GrCycle* cycle, signed char bBehaviour)
{
    if (cycle != 0) {
        cycle->SetBehaviour(bBehaviour);
    }
}

// FUNCTION: WIZ8 0x004c61c0
void MonsterSetCycleSubCycle(W8GrCycle* cycle, unsigned char subcycle)
{
    if (cycle != 0) {
        cycle->SetSubCycle(subcycle);
    }
}

/* An unguarded three-argument forward. The arguments are pushed back to front
   and handed straight on, so nothing here says what any of them mean. */
// FUNCTION: WIZ8 0x004c5eb0
void MonsterForward4C4DE0(int arg_1, int arg_2, int arg_3)
{
    Function4C4DE0(arg_1, arg_2, arg_3);
}

/* The public forwarding boundary preserves the loader's AL result. Both
   MonsterManager callers assert that result immediately after this call. */
// FUNCTION: WIZ8 0x004c58e0
unsigned char MonsterReadAllCycles004C58E0(
    const W8GrCycleLoadContext* context,
    const char* monster_name,
    W8Monster** monster,
    int load_value,
    int location_id)
{
    return MonsterReadAllCycles004C0300(
        context, monster_name, monster, load_value, location_id);
}

/* Load one monster cycle through GrCycle's polymorphic factory boundary, then
   fill in any particle event whose cycle matches but whose subcycle was left
   at the -1 sentinel.  The factory accepts the base-class output slot; object
   type zero is what proves the resulting object is a W8Monster here. */
// FUNCTION: WIZ8 0x004C5910
unsigned char LoadMonsterCycle004C5910(
    const W8GrCycleLoadContext* context,
    const char* mon_name,
    W8Monster** monster,
    int cycle,
    int value)
{
    unsigned char success = LoadGrCycle004A67E0(
        context,
        mon_name,
        reinterpret_cast<W8GrCycle**>(monster),
        cycle,
        value,
        "data\\monsters",
        0);

    if ((*monster)->m_plsParticles != 0) {
        int count = (*monster)->m_plsParticles->GetCount();
        if (count != 0) {
            for (int index = 0; index < count; ++index) {
                W8GrCycleShakeEvent* event =
                    *(*monster)->m_plsParticles->GetAt(index);
                if (event->cycle_00 == cycle && event->subcycle_04 == -1) {
                    event->subcycle_04 =
                        (*monster)->m_pRep->selection.monster.current_subcycle;
                }
            }
        }
    }
    return success;
}

/* Two whole-body tail calls. Neither wrapper takes an argument and neither
   callee touches ECX - both read only the pair of globals at 0x00659B34 and
   0x00659B3C - so the wrappers pass nothing on and VC6 lowers each to a bare
   jump. That is the whole difference from the cdecl pass-throughs above: with
   no stack arguments there is nothing left to clean up. */
// FUNCTION: WIZ8 0x004c61e0
void MonsterForward453160(void)
{
    Function453160();
}

// FUNCTION: WIZ8 0x004c61f0
void MonsterForward4531A0(void)
{
    Function4531A0();
}

/*
 * Six more null-guarded forwarders onto the Navigator base at +0x18, the same shape
 * MonsterForward4537E0 has: the guard tests the monster, the receiver is
 * derived from it with a `lea`, and a monster that is not there is simply not
 * acted on. What each one answers on the null path is the evidence for its
 * return type - a cleared AL for the byte-sized ones, a loaded 0.0f for the
 * float, and a bare return for the four that hand nothing back.
 */
// FUNCTION: WIZ8 0x004c5f50
void MonsterSetNavigatorValue120(W8Monster* monster, float value)
{
    if (monster != 0) {
        monster->SetValue120(value);
    }
}

// FUNCTION: WIZ8 0x004c5f70
float MonsterGetNavigatorValue120(W8Monster* monster)
{
    if (monster != 0) {
        float value = monster->GetValue120();
        return value;
    }
    return 0.0f;
}

// FUNCTION: WIZ8 0x004c5f90
unsigned char MonsterForward452630(W8Monster* monster, const srVector3T<float>* position)
{
    if (monster != 0) {
        return monster->Function452630(position);
    }
    return 0;
}

// FUNCTION: WIZ8 0x004c5fb0
void MonsterForward453690(W8Monster* monster, void* argument)
{
    if (monster != 0) {
        monster->Function453690(argument);
    }
}

// FUNCTION: WIZ8 0x004c5fd0
void MonsterSetNavigatorObjectFlag38(W8Monster* monster, char value)
{
    if (monster != 0) {
        monster->SetObject68Flag38(value);
    }
}

/* Pass a copied position through Navigator's collision adjustment and install
   the adjusted result on the Monster's ordinary Navigator base. */
// FUNCTION: WIZ8 0x004c5f00
void MonsterSetAdjustedPosition004C5F00(
    W8Monster* monster, const srVector3T<float>* position)
{
    srVector3T<float> current;
    srVector3T<float> adjusted;
    srVector3T<float>* adjusted_position;
    srVector3T<float> result;

    current.x = position->x;
    current.y = position->y;
    current.z = position->z;
    adjusted_position = monster->AdjustPosition00454440(
        &adjusted, &current, &current);
    result.x = adjusted_position->x;
    result.y = adjusted_position->y;
    result.z = adjusted_position->z;
    monster->SetPositionInternal00453590(&result);
}

// FUNCTION: WIZ8 0x004c5ff0
unsigned short MonsterApproachStartupNavigator004C5FF0(
    W8Monster* monster, double separation)
{
    unsigned short result;

    if (monster != 0) {
        result = monster->Function4526C0(
            g_startup_world_659c0c, separation);
        if (result != 0) {
            monster->fields.movement_0c0.unknown_076[0] = 0;
        }
        return result;
    }
    return 0;
}

// FUNCTION: WIZ8 0x004c6030
unsigned short MonsterLinkToStartupNavigator004C6030(W8Monster* monster)
{
    if (monster != 0) {
        W8Navigator* target = g_startup_world_659c0c;

        return monster->LinkToNavigator004527A0(
            target, WorldGetFarClip(GetWorld()) * 2.0);
    }
    return 0;
}

// FUNCTION: WIZ8 0x004c6070
unsigned short MonsterConfigureMovementToPlayer004C6070(
    W8Monster* monster,
    int value_1,
    int value_2,
    srVector3T<float> position,
    int value_3,
    int value_4)
{
    if (monster != 0) {
        W8Navigator* target = g_startup_world_659c0c;

        return monster->ConfigureMovementToNavigator004529A0(
            target,
            value_1,
            value_2,
            position,
            value_3,
            monster->GetYaw(),
            value_4);
    }
    return 0;
}

// FUNCTION: WIZ8 0x004c60d0
unsigned short MonsterConfigureMovementToMonster004C60D0(
    W8Monster* monster,
    W8Monster* target,
    int value_1,
    int value_2,
    srVector3T<float> position,
    int value_3,
    int value_4)
{
    if (monster != 0 && target != 0) {
        return monster->ConfigureMovementToNavigator004529A0(
            target,
            value_1,
            value_2,
            position,
            value_3,
            monster->GetYaw(),
            value_4);
    }
    return 0;
}

// FUNCTION: WIZ8 0x004c6200
void MonsterSetNavigatorFlag25(W8Monster* monster, char value)
{
    if (monster != 0) {
        monster->SetFlag25(value);
    }
}

/* Hands the shared reference position to one of Navigator's two position
   sinks. The monster's script part is found the long way round - the location
   id lives in the cycle array at 0x1e4, and the lookup pair turns it into the
   W8MonsterInfo whose control state gates the whole body - which is what puts
   this in Monster.cpp rather than in the engine: the assertion path the lookup
   carries names this file and line 5299.
   Control state one is the only value that suppresses the update; every other
   value falls through. The position is fetched before the flag is read, so it
   is read even on the path that turns out not to need one sink over the other,
   and the flag decides only which sink receives it. */
// FUNCTION: WIZ8 0x004c6240
void MonsterForwardReferencePosition(W8Monster* monster, char alternate)
{
    W8MonsterInfo* monster_info;
    srVector3T<float> position;

    if (monster != 0) {
        monster_info = MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
            0x14b3, MONSTER_CPP, monster->propagated_value_1e4, 1));
        if (monster_info->control_state != 1) {
            GetCameraPosition(&position);
            if (alternate != 0) {
                monster->Function454040(&position);
            } else {
                monster->AimAtPosition(&position);
            }
        }
    }
}

/* Aim one live Monster at another unless the source is controlled directly.
   The alternate path uses Navigator's second position sink, matching the
   corresponding player-position helper above. */
// FUNCTION: WIZ8 0x004c62c0
void MonsterAimAtMonster004C62C0(
    W8Monster* monster, W8Monster* target, char alternate)
{
    W8MonsterInfo* monster_info;
    srVector3T<float> target_position;
    srVector3T<float> position;

    if (monster != 0 && target != 0) {
        monster_info = MonsterGetScriptPartByLocationIndex(
            MonsterGetIndexByLocationID(
                0x14c7, MONSTER_CPP, monster->propagated_value_1e4, 1));
        if (monster_info->control_state != 1) {
            target_position = target->GetPosition();
            position.x = target_position.x;
            position.y = target_position.y;
            position.z = target_position.z;
            if (alternate != 0) {
                monster->Function454040(&position);
            }
            else {
                monster->AimAtPosition(&position);
            }
        }
    }
}

/* Flatten every model instance reachable from every cycle and subcycle. The
   temporary vectors used by the damage-appearance accessors below prove the
   element type: AniMesh's frame lookup returns stModelInstance objects and the
   consumers read their first-party fields beyond the srModelInstance base. */
// FUNCTION: WIZ8 0x004c6350
void W8Monster::CollectModelInstances004C6350(
    W8GrowableVector<stModelInstance005EC7D0*>* instances)
{
    int cycle;

    GetTotalAnimationCount();
    for (cycle = 0; cycle < W8_MONSTER_CYCLE_COUNT; ++cycle) {
        int subcycle;

        for (subcycle = 0;
             subcycle < m_pRep->GetNumSubsPerCycle((signed char)cycle);
             ++subcycle) {
            W8MonsterAnimationVector* cycle_animations =
                &m_pRep->animations[cycle];
            W8AnimObj* animation;

            if (subcycle >= cycle_animations->GetCount()) {
                Function401920(FormatString(
                    "Monster %s: Missing CYCLE %s subcycle %d",
                    m_pRep->name_5c0,
                    g_cycle_names[cycle].name,
                    subcycle));
            }
            animation = *cycle_animations->GetAt(subcycle);
            if (animation == 0) {
                continue;
            }

            if (AnimationIsRunning(animation) == 0) {
                int list_index;

                for (list_index = 0; list_index < 3; ++list_index) {
                    W8AniMesh* mesh =
                        static_cast<W8AniMesh*>(animation->entries_18[list_index]);
                    if (mesh != 0) {
                        int frame_count = AniMeshValue004B64F0(mesh);

                        if ((mesh->flags_00 & 0x20) != 0) {
                            instances->Add(GetAniMeshFrame004B6550(mesh, 0));
                        }
                        else {
                            int frame;

                            for (frame = 0; frame < frame_count; ++frame) {
                                instances->Add(
                                    GetAniMeshFrame004B6550(mesh, frame));
                            }
                        }
                    }
                }
            }
            else if (AnimationIsRunning(animation) == 1) {
                int list_index;

                for (list_index = 0; list_index < 3; ++list_index) {
                    W8PList* list = animation->meshes_28[list_index];
                    if (list != 0) {
                        int mesh_index;
                        int mesh_count = PLLength(list);

                        for (mesh_index = 0; mesh_index < mesh_count; ++mesh_index) {
                            W8AniMesh* mesh = static_cast<W8AniMesh*>(
                                PLGet(list, mesh_index));
                            if (mesh != 0) {
                                int frame;
                                int frame_count = AniMeshValue004B64F0(mesh);

                                for (frame = 0; frame < frame_count; ++frame) {
                                    instances->Add(
                                        GetAniMeshFrame004B6550(mesh, frame));
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

/* Replace one named texture in a damage stage. Model instances own the normal
   skin tables; shake particles are the fallback when no model uses the name. */
// FUNCTION: WIZ8 0x004c6700
unsigned char W8Monster::ReplaceSkinTexture004C6700(
    int stage, const char* old_name, const char* new_name)
{
    char path[200];
    unsigned char replaced = 0;

    sprintf(path, "Data\\Monsters\\Bitmaps\\%s", new_name);
    srTextureIFace* texture = LoadTexture004B9460(path, 0, 1);
    if (texture == 0) {
        Function401920(FormatString("Missing skin texture: %s", new_name));
        return 0;
    }

    W8GrowableVector<stModelInstance005EC7D0*> instances;
    CollectModelInstances004C6350(&instances);
    for (int index = 0; index < instances.GetCount(); ++index) {
        if ((*instances.GetAt(index))->ReplaceDamageStageTexture004807B0(
                stage, old_name, texture) != 0) {
            replaced = 1;
        }
    }

    if (replaced == 0 && m_plsParticles != 0) {
        for (int index = 0; index < m_plsParticles->GetCount(); ++index) {
            W8GrCycleShakeEvent* event = *m_plsParticles->GetAt(index);
            if (event->particle_08->ReplaceTexture0049AC30(
                    old_name, texture) != 0) {
                replaced = 1;
            }
        }
    }
    return replaced;
}

/* Damage table names are the parsed skin name followed by its numeric stage.
   A frame either creates the table or attaches the already-created table. */
// FUNCTION: WIZ8 0x004c6880
int W8Monster::AddDamageStage004C6880(const char* base_name, int stage)
{
    char name[128];
    int result = -1;
    W8GrowableVector<stModelInstance005EC7D0*> instances;

    sprintf(name, "%s%d", base_name, stage);
    CollectModelInstances004C6350(&instances);
    for (int index = 0; index < instances.GetCount(); ++index) {
        stModelInstance005EC7D0* instance = *instances.GetAt(index);
        if (instance->FindDamageStage00480790(name) == -1) {
            result = instance->AddDamageStage00480560(name);
        }
        else {
            result = instance->AddExistingDamageStage00480670(name);
        }
    }
    return result;
}

/* The final registered Monster for a cycle name owns removal of that name's
   per-mesh skin tables. */
// FUNCTION: WIZ8 0x004c6b10
void W8Monster::RemoveCycleSkinTables004C6B10()
{
    const char* cycle_name = GetGrCycleName(this);
    W8GrowableVector<stModelInstance005EC7D0*> instances;

    if (cycle_name != 0) {
        CollectModelInstances004C6350(&instances);
        for (int index = 0; index < instances.GetCount(); ++index) {
            stMeshModel* mesh = static_cast<stMeshModel*>(
                (*instances.GetAt(index))->model());
            for (; mesh != 0; mesh = mesh->next) {
                mesh->RemoveSkinTablesForCycle00473780(cycle_name);
            }
        }
    }
}

/* Select the damage-stage model on every frame instance owned by this
   Monster. UpdateMonsterDamageAppearance supplies the HP-derived stage. */
// FUNCTION: WIZ8 0x004c6990
void W8Monster::SetDamageStage004C6990(int stage)
{
    W8GrowableVector<stModelInstance005EC7D0*> instances;
    int index;

    CollectModelInstances004C6350(&instances);
    for (index = 0; index < instances.GetCount(); ++index) {
        (*instances.GetAt(index))->damage_stage_184 = stage;
    }
}

/* Every frame instance in one Monster carries the same number of available
   damage stages, so the first instance supplies the count. */
// FUNCTION: WIZ8 0x004c6a50
int W8Monster::GetDamageStageCount004C6A50()
{
    W8GrowableVector<stModelInstance005EC7D0*> instances;

    CollectModelInstances004C6350(&instances);
    if (instances.GetCount() != 0) {
        return (*instances.GetAt(0))->damage_stage_count_18c;
    }
    return 0;
}

/* Resolve the database-controlled render gate after the Monster's transient
   runtime overrides. The alternate argument selects the secondary live-info
   flag used by the world-update path. */
// FUNCTION: WIZ8 0x004c7c00
unsigned char W8Monster::IsRenderable004C7C00(char alternate)
{
    unsigned char disabled = flag_217;
    int location_id = propagated_value_1e4;
    W8MonsterInfo* monster_info;
    W8MonsterRecord* record;

    if (disabled != 0) {
        return 0;
    }
    if (flag_215 != 0) {
        return 1;
    }
    if (location_id == -1) {
        return 1;
    }
    if (((flags_1dc >> 8) & 1) != 0) {
        return 1;
    }
    if (flags_330.flag_00 != 0) {
        return 1;
    }

    monster_info = MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
        0x1977, MONSTER_CPP, location_id, 1));
    record = GetMonsterDataForInfo(monster_info);
    if (record->flag_248 > 0) {
        return monster_info->flag_28d;
    }
    if (alternate != 0) {
        return monster_info->flag_2ab;
    }
    return monster_info->flag_24d;
}

/* Discover animated material state once, cache it in flags_1dc, and restart
   the selected model's animated texture on frame zero when present. */
// FUNCTION: WIZ8 0x004c51d0
void W8Monster::InitializeAnimatedTexture004C51D0()
{
    srModelInstance* instance = 0;

    if (m_pRep->flag_064 == 0) {
        if ((flags_1dc & 2) == 0) {
            srMeshModel* model;

            instance = SelectCycleFrameLod004A8360(
                m_pRep->selection.monster.current_cycle,
                0,
                m_pRep->m_bLOD);
            model = static_cast<srMeshModel*>(instance->model());
            if (MeshHasAnimatedTexture004B9AA0(model) == 0) {
                flags_1dc &= ~4;
            }
            else {
                flags_1dc |= 4;
            }
            flags_1dc |= 2;
        }
        if ((flags_1dc & 4) != 0) {
            if (instance == 0) {
                instance = SelectCycleFrameLod004A8360(
                    m_pRep->selection.monster.current_cycle,
                    0,
                    m_pRep->m_bLOD);
            }
            SetModelAnimatedTextureFrame004B9B00(instance, 0);
        }
    }
}

/* Forward to the object's own vtable slot four. */
// FUNCTION: WIZ8 0x004c59b0
void MonsterCallSlot10(void* object, int argument)
{
    (*(void(**)(void*, int))(*(void***)object + 4))(object, argument);
}

extern "C" {
// FUNCTION: WIZ8 0x004C5810
void Function4C5810(W8Monster* target)
{
    target->Method4C5290();
}
}
extern "C" {
// FUNCTION: WIZ8 0x004C5860
void DeleteMonster004C5860(W8Monster* monster)
{
    if (monster != NULL) {
        delete monster;
    }
}
}
extern "C" {
// FUNCTION: WIZ8 0x004C59C0
void Function4C59C0(int enabled, int value)
{
    if (enabled != 0 && value != 0) {
        Function4A7A70(value);
    }
}
}
extern "C" {
// FUNCTION: WIZ8 0x004C5ED0
void Function4C5ED0(int enabled)
{
    if (enabled != 0) {
        Function4C4EF0();
    }
}
}
// FUNCTION: WIZ8 0x004C6220
void SetFlag6081E4(unsigned char value)
{
    g_flag_6081e4 = value;
    g_value_659c14 = 0;
}
