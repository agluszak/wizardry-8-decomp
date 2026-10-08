#include "wiz8/engine_code/AnimRep.hpp"
#include "wiz8/engine_code/Camera.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/CombatHostility.h"
#include "wiz8/engine_code/3d.h"
#include "wiz8/engine_code/3dapi.h"
#include "wiz8/engine_code/PolyPick.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/Spells.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/NPCInteractionSubscreen.h"
#include "wiz8/engine_code/Navigator.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/engine_code/Environment.h"
#include "wiz8/engine_code/Levels.h"
#include "wiz8/engine_code/Quality.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/float_constants.h"
#include "wiz8/startup_world.h"
#include "wiz8/engine_code/SoundEvent.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_screens/AutomapScreen.h"
#include "wiz8/local_screens/mipe.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/CombatRange.h"
#include "wiz8/xstatus.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/engine_code/AniMesh.h"
#include "wiz8/engine_code/AnimObj.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/Item.h"
#include "wiz8/engine_code/Missile.h"
#include "wiz8/engine_code/materials.h"
#include "wiz8/engine_code/MonsterLight.h"
#include "wiz8/engine_code/OctBuildPreTree.h"
#include "wiz8/ground_shadow.h"
#include "wiz8/engine_code/stLight.hpp"
#include "wiz8/engine_code/stModelInstance.h"
#include "wiz8/engine_code/stParticle.h"
#include "wiz8/engine_code/game_timer.h"
#include "wiz8/location_variables.h"
#include "wiz8/engine_code/ReadLevel.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/engine_code/stTextureAnim.h"
#include "wiz8/engine_code/stTextureFile.h"
#include "wiz8/engine_code/stScript.h"
#include "wiz8/engine_code/stSound3D.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/local_code/FormationAndFacing.h"
#include "wiz8/regions.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/PathAI.h"
#include "wiz8/item_spawning.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/engine_code/stMeshModel.h"
#include "wiz8/engine_code/stTextureFile.h"
#include "wiz8/monster_runtime.h"
#include "wiz8/local_code/NPCManager.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/npc_interaction.h"
#include "wiz8/sr_api.h"
#include "wiz8/local_code/Targeting.h"
#include "wiz8/utility.h"
#include "wiz8/virtual_file.h"
#include "wiz8/fonts.h"
#include "surrender/srCamera.h"
#include "surrender/srTimer.h"
#include "surrender/srScene.h"
#include "surrender/srModelInstance.h"
#include "surrender/srCore.h"
#include "surrender/srColorSurface.h"
#include "surrender/srPixelConvert.h"
#include "surrender/srMaterial.h"
#include "surrender/srShader.h"
#include "Random.h"
#include "Font.h"
#include "FileMan.h"
#include "soundman.h"
#include "wiz8/music_playlist.h"
#include "wiz8/layouts/npc_state.h"
#include "wiz8/local_code/NPCScripting.h"
#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/GrCycle.h"
#include "input.h"
// GLOBAL: WIZ8 0x00659c14
int g_combat_round_counter;

// GLOBAL: WIZ8 0x005ebcf8
const float g_float_inverse_half_turn_degrees = 0.0055555556900799274f;
// GLOBAL: WIZ8 0x0060bfe0
float g_monster_light_scale = 1.0f;

// GLOBAL: WIZ8 0x0065970C
unsigned char g_monster_shadow_updates_enabled;
/* Layout-compatible with srVector3T<float> but POD so VC6 emits static .data
   instead of a dynamic initializer into .bss. */
struct W8AttachmentOffset {
    float x;
    float y;
    float z;
};

// GLOBAL: WIZ8 0x0060e618
static W8AttachmentOffset g_monster_attachment_offsets[8][8] = {{{0.0f, 0.0f, 0.0f},
                                                                 {0.0f, 0.0f, 0.0f},
                                                                 {0.0f, 0.0f, 0.0f},
                                                                 {0.0f, 0.0f, 0.0f},
                                                                 {0.0f, 0.0f, 0.0f},
                                                                 {0.0f, 0.0f, 0.0f},
                                                                 {0.0f, 0.0f, 0.0f},
                                                                 {0.0f, 0.0f, 0.0f}},
                                                                {{-75.0f, 0.0f, 0.0f},
                                                                 {75.0f, 0.0f, 0.0f},
                                                                 {0.0f, 0.0f, 0.0f},
                                                                 {0.0f, 0.0f, 0.0f},
                                                                 {0.0f, 0.0f, 0.0f},
                                                                 {0.0f, 0.0f, 0.0f},
                                                                 {0.0f, 0.0f, 0.0f},
                                                                 {0.0f, 0.0f, 0.0f}},
                                                                {{-75.0f, 0.0f, 0.0f},
                                                                 {0.0f, 0.0f, 0.0f},
                                                                 {75.0f, 0.0f, 0.0f},
                                                                 {0.0f, 0.0f, 0.0f},
                                                                 {0.0f, 0.0f, 0.0f},
                                                                 {0.0f, 0.0f, 0.0f},
                                                                 {0.0f, 0.0f, 0.0f},
                                                                 {0.0f, 0.0f, 0.0f}},
                                                                {{-112.5f, 0.0f, 0.0f},
                                                                 {-37.5f, 0.0f, 0.0f},
                                                                 {37.5f, 0.0f, 0.0f},
                                                                 {112.5f, 0.0f, 0.0f},
                                                                 {0.0f, 0.0f, 0.0f},
                                                                 {0.0f, 0.0f, 0.0f},
                                                                 {0.0f, 0.0f, 0.0f},
                                                                 {0.0f, 0.0f, 0.0f}},
                                                                {{-112.5f, 0.0f, 0.0f},
                                                                 {-37.5f, 0.0f, 0.0f},
                                                                 {37.5f, 0.0f, 0.0f},
                                                                 {112.5f, 0.0f, 0.0f},
                                                                 {0.0f, 75.0f, 0.0f},
                                                                 {0.0f, 0.0f, 0.0f},
                                                                 {0.0f, 0.0f, 0.0f},
                                                                 {0.0f, 0.0f, 0.0f}},
                                                                {{-112.5f, 0.0f, 0.0f},
                                                                 {-37.5f, 0.0f, 0.0f},
                                                                 {37.5f, 0.0f, 0.0f},
                                                                 {112.5f, 0.0f, 0.0f},
                                                                 {-37.5f, 75.0f, 0.0f},
                                                                 {37.5f, 75.0f, 0.0f},
                                                                 {0.0f, 0.0f, 0.0f},
                                                                 {0.0f, 0.0f, 0.0f}},
                                                                {{-112.5f, 0.0f, 0.0f},
                                                                 {-37.5f, 0.0f, 0.0f},
                                                                 {37.5f, 0.0f, 0.0f},
                                                                 {112.5f, 0.0f, 0.0f},
                                                                 {-75.0f, 75.0f, 0.0f},
                                                                 {0.0f, 75.0f, 0.0f},
                                                                 {75.0f, 75.0f, 0.0f},
                                                                 {0.0f, 0.0f, 0.0f}},
                                                                {{-112.5f, 0.0f, 0.0f},
                                                                 {-37.5f, 0.0f, 0.0f},
                                                                 {37.5f, 0.0f, 0.0f},
                                                                 {112.5f, 0.0f, 0.0f},
                                                                 {-112.5f, 75.0f, 0.0f},
                                                                 {-37.5f, 75.0f, 0.0f},
                                                                 {37.5f, 75.0f, 0.0f},
                                                                 {112.5f, 75.0f, 0.0f}}};
// GLOBAL: WIZ8 0x0060e918
static float g_monster_attachment_scales[8] = {0.3f,  0.2f,  0.15f, 0.15f,
                                               0.15f, 0.15f, 0.15f, 0.15f};

// GLOBAL: WIZ8 0x005ec04c
const float g_monster_rotation_offset = 3.141592502593994f;
// GLOBAL: WIZ8 0x005ed1f0
const double g_monster_death_rotation_pi = 3.141592653589793;
// GLOBAL: WIZ8 0x005ed2a8
const float g_monster_attachment_distance_scale = 0.00039999998989515007f;
// GLOBAL: WIZ8 0x005eca84
const float g_monster_attachment_vertical_scale = 30.0f;
// GLOBAL: WIZ8 0x005ed2a0
const double g_monster_attachment_group_spacing = 15.0;
// GLOBAL: WIZ8 0x005ed29c
const float g_monster_linked_vertical_scale = 255.0f;
// GLOBAL: WIZ8 0x005ed298
const float g_monster_poster_vertical_rate = 0.20000000298023224f;
// GLOBAL: WIZ8 0x005ec3d8
const double g_monster_poster_max_distance = 1000.0;
// GLOBAL: WIZ8 0x005ed2b8
const double g_monster_script_direction_step = 0.8975978857142857;
// GLOBAL: WIZ8 0x005ec2b0
const double g_monster_facing_tolerance = 0.78539815;
// GLOBAL: WIZ8 0x005ed2c0
const double g_monster_group_nearest_range = 12500.0;
// GLOBAL: WIZ8 0x0060f684
static char g_warning_missing_spell_vertex[] =
    "WARNING: %ls does not have a SPELL vertex marked! --> Lee";
// GLOBAL: WIZ8 0x0060EA08
W8CycleNameRow g_cycle_names[W8_MONSTER_CYCLE_COUNT] = {
    {"BIRTH", 5},
    {"IDLE", 4},
    {"SPICE", 5},
    {"TRANSITION", 10},
    {"WALK", 4},
    {"DRAW", 4},
    {"ATTACK_CLOSE", 12},
    {"ATTACK_RANGED", 13},
    {"unused", 6},
    {"ATTACK_SWING", 12},
    {"ATTACK_THRUST", 13},
    {"ATTACK_BASH", 11},
    {"ATTACK_MELEE", 12},
    {"ATTACK_THROW", 12},
    {"ATTACK_PUNCH", 12},
    {"ATTACK_KICK", 11},
    {"ATTACK_LASH", 11},
    {"ATTACK_SHOOT", 12},
    {"ATTACK_SPECIAL", 14},
    {"DODGE", 5},
    {"GET_HIT", 7},
    {"DIE", 3},
    {"TURN", 4},
    {"TALK_SPICE", 10},
    {"TALK", 4},
    {"SPELL", 5},
    {"SPECIAL", 7},
};

// GLOBAL: WIZ8 0x0060e614
static bool g_monster_gib_option = true;
// GLOBAL: WIZ8 0x005ed280
extern const double g_monster_light_color_scale = 0.00392156862745098;

// GLOBAL: WIZ8 0x00682FD0
static W8Vector<stModelInstance*> g_monster_model_instances;

#define MONSTER_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\Monster.cpp"

/* Highlight triangle bitmaps indexed by the slot's marching-order position:
   the marker SetMonsterPartySlotMarker hangs on a monster. */
// GLOBAL: WIZ8 0x0060e938
const char* g_party_target_marker_bitmaps[8] = {"TriRed.tga",  "TriGreen.tga",  "TriPurple.tga",
                                                "TriBlue.tga", "TriOrange.tga", "TriYellow.tga",
                                                "TriPink.tga", "TriBrown.tga"};

// GLOBAL: WIZ8 0x0060f510
static char g_monster_bitmap_path_format[] = "Data\\Monsters\\Bitmaps\\%s";

// GLOBAL: WIZ8 0x0060e958
const char* g_monster_script_commands[MONSCR_COUNT] = {"GOTO",
                                                       "WALKTO",
                                                       "FACE",
                                                       "SAY",
                                                       "CYCLE",
                                                       "SHOOT",
                                                       "GIVE",
                                                       "TELEPORT",
                                                       "CAST",
                                                       "DIE",
                                                       "END",
                                                       "IF",
                                                       "ELSE",
                                                       "ENDIF",
                                                       "NPCINTERACTION",
                                                       "DISPOSITION",
                                                       "DISAPPEAR",
                                                       "LOOKHERE",
                                                       "NPCNUMBER",
                                                       "FOLLOW",
                                                       "PATROL",
                                                       "STOPPATROL",
                                                       "TRIGGER",
                                                       "DELAY",
                                                       "BEGINORDERS",
                                                       "ENDORDERS",
                                                       "GUARD",
                                                       "POINTPATROL",
                                                       "RANDOMPOINTPATROL",
                                                       "DEAF",
                                                       "DOACTION",
                                                       "EAST",
                                                       "NORTHEAST",
                                                       "NORTH",
                                                       "NORTHWEST",
                                                       "WEST",
                                                       "SOUTHWEST",
                                                       "SOUTH",
                                                       "SOUTHEAST",
                                                       "TURNTOFACEPARTY",
                                                       "LOOKABOUT",
                                                       "PLAY",
                                                       "FADEOUT",
                                                       "STAYHOME"};

// VTABLE: WIZ8 0x005ecdac
// class W8GrowableVector<float>
// VTABLE: WIZ8 0x005ecdc8
// class W8GrowableVector<srVector3T<float> >
// VTABLE: WIZ8 0x005ed200
// class W8MonsterRep
/* Read the text-side monster representation. A named representation is shared
   by cloning its first live cycle; otherwise the MLS file supplies scalar
   movement settings, visual flags, sound/shake events, skin stages, lights,
   and the list of binary .mon cycles. */
// FUNCTION: WIZ8 0x004c0300
unsigned char ReadOrCloneMonsterCycles(const W8GrCycleLoadContext* context,
                                       const char* monster_name, W8Monster** monster,
                                       int load_value, int location_id)
{
    W8Monster* shared = static_cast<W8Monster*>(FindFirstGrCycleByName(monster_name));
    if (shared != 0) {
        *monster = new W8Monster(*shared);
        if (*monster == 0) {
            srAssertFail("*ppMonster", MONSTER_CPP, 0x4d3, 0);
        }
        (*monster)->RandomizeAppearanceAndMotion();
        RegisterGrCycle(monster_name, *monster);
        return 1;
    }

    PauseSharedGameTimers();

    unsigned char more = 1;
    bool success = true;
    bool flies = false;
    bool swims = false;
    bool crawls = false;
    bool quadruped = false;
    bool full_transition = false;
    bool spice_monster = false;
    int left_handed = 45;
    bool has_light = false;
    bool light_pulsing = false;
    bool random_idle_range = false;
    bool has_lod_range = false;
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
    int footstep_combat_volume = 0;
    srVector3T<float> light_first;
    srVector3T<float> light_second;
    W8SoundEvent* last_sound = 0;
    int damage_stage = -1;
    int skin_stage = 0;
    bool skins_started = false;

    char path[256];
    sprintf(path, "data\\Monsters\\%s.mls", monster_name);
    int handle = FileOpen(path, FILE_ACCESS_READ | FILE_OPEN_EXISTING, 0);
    if (handle == 0) {
        srAssertFail("hFile", MONSTER_CPP, 0x50f,
                     reinterpret_cast<char*>(String("Couldn't open %s", path)));
    }
    *monster = 0;

    if (handle == 0) {
        W8GrCycle* loaded = 0;
        success = LoadGrCycle(context, monster_name, &loaded, -1, load_value, "data\\monsters", 0);
        *monster = static_cast<W8Monster*>(loaded);
        if ((*monster)->m_plsParticles != 0) {
            for (int index = 0; index < (*monster)->m_plsParticles->GetCount(); ++index) {
                W8GrCycleParticleAttachment* event = *(*monster)->m_plsParticles->GetAt(index);
                if (event->cycle == -1 && event->subcycle == -1) {
                    event->subcycle = (*monster)->m_pRep->current_subcycle;
                }
            }
        }
    } else {
        char line[250];
        while (more != 0) {
            ReadTextLine(handle, line, sizeof(line), &more);
            while (line[0] == '\0' && more != 0) {
                ReadTextLine(handle, line, sizeof(line), &more);
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
            } else if (_stricmp(command, "RotationRate") == 0) {
                sscanf(line, "%s %f", command, &rotation_rate);
            } else if (_stricmp(command, "scalefactor") == 0) {
                sscanf(line, "%s %f", command, &scale_factor);
            } else if (_stricmp(command, "walkradius") == 0) {
                sscanf(line, "%s %f", command, &walk_radius);
            } else if (_stricmp(command, "fightradius") == 0) {
                sscanf(line, "%s %f", command, &fight_radius);
            } else if (_stricmp(command, "targetheight") == 0) {
                sscanf(line, "%s %f", command, &target_height);
            } else if (_stricmp(command, "cameraheight") == 0) {
                sscanf(line, "%s %f", command, &camera_height);
            } else if (_stricmp(command, "deathscale") == 0) {
                sscanf(line, "%s %f", command, &death_scale);
            } else if (_stricmp(command, "scalerangestart") == 0) {
                sscanf(line, "%s %f", command, &scale_range_start);
            } else if (_stricmp(command, "scalerangeend") == 0) {
                sscanf(line, "%s %f", command, &scale_range_end);
            } else if (_stricmp(command, "hoverrangestart") == 0) {
                sscanf(line, "%s %f", command, &hover_range_start);
            } else if (_stricmp(command, "hoverrangeend") == 0) {
                sscanf(line, "%s %f", command, &hover_range_end);
            } else if (_stricmp(command, "bobrangestart") == 0) {
                sscanf(line, "%s %f", command, &bob_range_start);
            } else if (_stricmp(command, "bobrangeend") == 0) {
                sscanf(line, "%s %f", command, &bob_range_end);
            } else if (_stricmp(command, "missilestart") == 0) {
                sscanf(line, "%s %d", command, &missile_start);
            } else if (_stricmp(command, "spellstart") == 0) {
                sscanf(line, "%s %d", command, &spell_start);
            } else if (_stricmp(command, "flies") == 0) {
                flies = true;
            } else if (_stricmp(command, "swims") == 0) {
                swims = true;
            } else if (_stricmp(command, "crawls") == 0) {
                crawls = true;
            } else if (_stricmp(command, "quadruped") == 0) {
                quadruped = true;
            } else if (_stricmp(command, "spicemonster") == 0) {
                spice_monster = true;
            } else if (_stricmp(command, "randomidlefps") == 0) {
                sscanf(line, "%s %f %f", command, &idle_fps_start, &idle_fps_end);
                random_idle_range = true;
            } else if (_stricmp(command, "loddistance") == 0) {
                sscanf(line, "%s %f %f", command, &lod_range_start, &lod_range_end);
                has_lod_range = true;
            } else if (_stricmp(command, "pitch") == 0) {
                int pitch;
                if (last_sound == 0) {
                    srAssertFail("pSndEvent", MONSTER_CPP, 0x58e,
                                 "mls pitch: Must specify a sound before specifying parameters");
                }
                sscanf(line, "%s %d", command, &pitch);
                last_sound->pitch = pitch;
            } else if (_stricmp(command, "volume") == 0) {
                if (last_sound == 0) {
                    srAssertFail("pSndEvent", MONSTER_CPP, 0x595,
                                 "mls volume: Must specify a sound before specifying parameters");
                }
                sscanf(line, "%s %d %d", command, &last_sound->volume_min, &last_sound->volume_max);
            } else if (_stricmp(command, "sound_falloff") == 0) {
                sscanf(line, "%s %f", command, &sound_falloff);
            } else if (_stricmp(command, "footstep_vol") == 0) {
                sscanf(line, "%s %d %d", command, &footstep_volume, &footstep_combat_volume);
            } else if (_stricmp(command, "probability") == 0) {
                int probability;
                if (last_sound == 0) {
                    srAssertFail(
                        "pSndEvent", MONSTER_CPP, 0x5a5,
                        "mls frequency: Must specify a sound before specifying parameters");
                }
                sscanf(line, "%s %d", command, &probability);
                last_sound->probability = static_cast<unsigned char>(probability);
            } else if (_stricmp(command, "animscript") == 0) {
            } else if (_stricmp(command, "script") == 0) {
                sscanf(line, "%s %s", command, argument);
                (*monster)->SetScript(argument, true);
            } else if (_stricmp(command, "opacity") == 0) {
                sscanf(line, "%s %f", command, &opacity);
            } else if (_stricmp(command, "glow") == 0) {
                sscanf(line, "%s %f", command, &glow);
            } else if (_stricmp(command, "shadow") == 0) {
                sscanf(line, "%s %f %f", command, &shadow_width, &shadow_depth);
            } else if (_stricmp(command, "lefthanded") == 0) {
                sscanf(line, "%s %d", command, &left_handed);
            } else if (_stricmp(command, "fulltransition") == 0) {
                full_transition = true;
            } else if (_stricmp(command, "addlight") == 0) {
                char light_mode[256];
                sscanf(line, "%*s %s ( %f %f %f ) ( %f %f %f )", light_mode, &light_first.x,
                       &light_first.y, &light_first.z, &light_second.x, &light_second.y,
                       &light_second.z);
                light_first *= static_cast<float>(g_monster_light_color_scale);
                light_second *= static_cast<float>(g_monster_light_color_scale);
                light_pulsing = _stricmp(light_mode, "pulsing") == 0;
                has_light = true;
            } else if (_stricmp(command, "skin") == 0) {
                if (!skins_started) {
                    skin_stage = 0;
                    damage_stage = (*monster)->AddDamageStage(monster_name, 0);
                    W8Vector<stModelInstance*> instances;
                    (*monster)->CollectModelInstances(&instances);
                    for (int index = 0; index < instances.GetCount(); ++index) {
                        (*instances.GetAt(index))->damage_stage = damage_stage;
                    }
                    skins_started = true;
                }
                if (_stricmp(argument, "default") != 0) {
                    ++skin_stage;
                    damage_stage = (*monster)->AddDamageStage(monster_name, skin_stage);
                }
            } else if (_stricmp(command, "skinswap") == 0) {
                char old_name[64];
                char new_name[64];
                /* 0x004C0FA8..0x004C1070: a SKINSWAP with no SKIN before it
                   creates the base stage first, then swaps into that stage. */
                if (!skins_started) {
                    skin_stage = 0;
                    damage_stage = (*monster)->AddDamageStage(monster_name, 0);
                    W8Vector<stModelInstance*> instances;
                    (*monster)->CollectModelInstances(&instances);
                    for (int index = 0; index < instances.GetCount(); ++index) {
                        (*instances.GetAt(index))->damage_stage = damage_stage;
                    }
                    skins_started = true;
                }
                sscanf(line, "%s %s %s", command, old_name, new_name);
                if (damage_stage != -1 &&
                    !(*monster)->ReplaceSkinTexture(damage_stage, old_name, new_name)) {
                    ShutdownWithErrorBox(reinterpret_cast<const char*>(
                        String( // reinterpret-ok: String returns a logging buffer
                            "The skin texture %s not found in monster %s!", old_name,
                            monster_name)));
                }
            } else {
                /* 0x004C10FB: the length test is on the whole line, not the argument. */
                if (strlen(line) <= 2) {
                    continue;
                }
                signed char subcycle;
                W8MonsterCycle cycle = ParseMonsterCycleName(command, &subcycle);
                if (cycle != W8_MONSTER_CYCLE_NONE) {
                    if (_strnicmp(argument, "gib", 3) != 0 || g_monster_gib_option) {
                        if (GetRenderOptionState(W8_RENDER_OPTION_ADDITIONAL_ANIMATIONS) == 0) {
                            cycle = NormalizeMonsterCycle(cycle);
                        }
                        if (*monster == 0 ||
                            (*monster)->IsCycleSupported(static_cast<signed char>(cycle)) == 0 ||
                            GetRenderOptionState(W8_RENDER_OPTION_ADDITIONAL_ANIMATIONS) != 0) {
                            float animation_scale = -1.0f;
                            sscanf(line, "%s %s %f", command, argument, &animation_scale);
                            W8GrCycle* loaded = *monster;
                            success = LoadGrCycle(context, argument, &loaded, cycle, load_value,
                                                  "data\\monsters", 0);
                            *monster = static_cast<W8Monster*>(loaded);
                            if ((*monster)->m_plsParticles != 0) {
                                for (int index = 0; index < (*monster)->m_plsParticles->GetCount();
                                     ++index) {
                                    W8GrCycleParticleAttachment* event =
                                        *(*monster)->m_plsParticles->GetAt(index);
                                    if (event->cycle == cycle && event->subcycle == -1) {
                                        event->subcycle = (*monster)->m_pRep->current_subcycle;
                                    }
                                }
                            }
                            if (animation_scale > 0.0f) {
                                W8MonsterRep* rep = (*monster)->m_pRep;
                                int current = rep->current_subcycle;
                                /* 0x004C128A: a missing subcycle is fatal, as in
                                   every other retail cycle lookup. */
                                if (rep->animations[cycle].GetCount() <= current) {
                                    ShutdownWithErrorBox(reinterpret_cast<const char*>(
                                        String( // reinterpret-ok: String returns a logging buffer
                                            "Monster %s: Missing CYCLE_%s, sub-cycle %d", rep->name,
                                            g_cycle_names[cycle].name, current)));
                                }
                                W8AnimObj* animation =
                                    *(*monster)->m_pRep->animations[cycle].GetAt(current);
                                animation->playback_scale = animation_scale;
                                *(*monster)->m_pRep->animation_scales[cycle].GetAt(current) =
                                    animation_scale;
                            }
                        }
                    }
                } else {
                    W8SoundEventKind sound_type = W8_SOUND_EVENT_UNRECOGNIZED;
                    if (_stricmp(command, "SOUND_FRAME") == 0)
                        sound_type = W8_SOUND_EVENT_FRAME;
                    else if (_stricmp(command, "SOUND_CYCLE") == 0)
                        sound_type = W8_SOUND_EVENT_CYCLE;
                    else if (_stricmp(command, "SOUND_FOOTSTEP") == 0)
                        sound_type = W8_SOUND_EVENT_FOOTSTEP;

                    if (sound_type != W8_SOUND_EVENT_UNRECOGNIZED) {
                        char cycle_name[256];
                        char wave_name[256];
                        char loop_name[64];
                        int frame = 0;
                        cycle_name[0] = wave_name[0] = loop_name[0] = '\0';
                        if (sound_type == W8_SOUND_EVENT_FOOTSTEP) {
                            sscanf(line, "%s %s %d", command, cycle_name, &frame);
                        } else {
                            sscanf(line, "%s %s %d %s %s", command, cycle_name, &frame, wave_name,
                                   loop_name);
                        }
                        W8MonsterCycle sound_cycle = ParseMonsterCycleName(cycle_name, &subcycle);
                        char wave_path[256];
                        wave_path[0] = '\0';
                        if (sound_type != W8_SOUND_EVENT_FOOTSTEP) {
                            sprintf(wave_path, "Data\\Sound\\Monsters\\%s.WAV", wave_name);
                        }
                        last_sound = CreateSoundEvent(sound_type, sound_cycle, frame, subcycle - 1,
                                                      wave_path, _stricmp(loop_name, "LOOP") == 0);
                        if (last_sound != 0) {
                            (*monster)->AddSoundEvent(last_sound);
                            last_sound->location_id = location_id;
                            last_sound->volume_min =
                                sound_type == W8_SOUND_EVENT_FOOTSTEP ? 0x23 : 0x7f;
                            last_sound->volume_max = last_sound->volume_min;
                            if (sound_falloff > 0.0f) {
                                last_sound->falloff = sound_falloff * g_world_scale;
                            }
                            if (footstep_volume != 0 || footstep_combat_volume != 0) {
                                last_sound->footstep_volume = footstep_volume;
                                last_sound->footstep_combat_volume = footstep_combat_volume;
                            }
                        }
                    } else if (_stricmp(command, "SHAKE_FRAME") == 0) {
                        char cycle_name[256];
                        int frame;
                        float duration = 1.0f;
                        float intensity = 1.0f;
                        float value = 10.0f;
                        /* 0x004C1569..0x004C1621: the first float is the intensity and
                           the second the duration, as in the spell and missile tables. */
                        sscanf(line, "%s %s %d %f %f %f", command, cycle_name, &frame, &intensity,
                               &duration, &value);
                        W8MonsterCycle shake_cycle = ParseMonsterCycleName(cycle_name, &subcycle);
                        W8CameraShakeEffect* effect = new W8CameraShakeEffect(
                            duration, true, intensity, value * g_world_scale, 0);
                        if (effect != 0) {
                            effect->frame = frame;
                            effect->cycle = shake_cycle;
                            effect->subcycle = subcycle - 1;
                            (*monster)->AddShakeEffect(effect);
                        }
                    } else {
                        srAssertFail("FALSE", MONSTER_CPP, 0x636,
                                     FormatString("Monster::ReadAllCycles: ERROR - Unrecognized "
                                                  "MLS string \"%s\" in %s",
                                                  command, path));
                    }
                }
            }
        }
        FileClose(handle);
    }

    W8MonsterRep* representation = (*monster)->m_pRep;
    delete[] representation->name;
    representation->name = 0;
    if (monster_name != 0) {
        representation->name = new char[strlen(monster_name) + 1];
        if (representation->name != 0) {
            strcpy(representation->name, monster_name);
        }
    }
    if (scale_range_start != -1.0f && scale_range_end != -1.0f) {
        representation->minimum_scale = scale_range_start;
        representation->maximum_scale = scale_range_end;
        scale_factor = (scale_range_end - scale_range_start) * 0.5f + scale_range_start;
    }
    representation->scale = scale_factor;
    representation->death_scale = death_scale;

    /* The world scale follows the largest radius any monster reaches. */
    if (walk_radius != 0.0f) {
        walk_radius *= g_world_scale;
        (*monster)->movement.collision_radius = walk_radius;
        if (g_runtime_world_scale < walk_radius) {
            g_runtime_world_scale = walk_radius;
        }
    }
    if (fight_radius != 0.0f) {
        fight_radius *= g_world_scale;
        (*monster)->movement.alternate_radius = fight_radius;
        if (g_runtime_world_scale < fight_radius) {
            g_runtime_world_scale = fight_radius;
        }
    }
    if (target_height != 0.0f) {
        target_height *= g_world_scale;
        if (target_height < 250.0f)
            target_height = 250.0f;
        (*monster)->movement.height_offset = target_height;
    }
    if (camera_height != 0.0f) {
        (*monster)->movement.secondary_height_offset = camera_height * g_world_scale;
    }

    if (!random_idle_range) {
        idle_fps_start = -3.0f;
        idle_fps_end = 3.0f;
    }
    if (representation->animations[1].GetCount() < 1) {
        ShutdownWithErrorBox(
            reinterpret_cast<const char*>(String( // reinterpret-ok: String returns a logging buffer
                "Monster %s: Missing CYCLE_%s, sub-cycle %d", representation->name,
                g_cycle_names[1].name, 0)));
    }
    W8AnimObj* idle = *representation->animations[1].GetAt(0);
    if (idle != 0) {
        representation->random_idle = true;
        representation->idle_playback_scale = idle->playback_scale;
        representation->random_idle_fps_min = idle_fps_start;
        representation->random_idle_fps_max = idle_fps_end;
    }
    if (missile_start > 0)
        (*monster)->missile_frame = missile_start;
    if (spell_start > 0)
        (*monster)->spell_frame = spell_start;
    if (has_lod_range) {
        representation->lod_near = lod_range_start * g_world_scale;
        representation->lod_far = lod_range_end * g_world_scale;
    }
    if (opacity >= 0.0f && opacity < 1.0f) {
        (*monster)->scale = opacity;
    }
    if (glow > 0.0f) {
        W8Vector<stModelInstance*> instances;
        (*monster)->CollectModelInstances(&instances);
        for (int index = 0; index < instances.GetCount(); ++index) {
            stModelInstance* instance = *instances.GetAt(index);
            instance->emissive_override_enabled = true;
            instance->emissive_override = glow;
        }
    }

    (*monster)->hover_base_min = hover_range_start;
    (*monster)->hover_base_max = hover_range_end;
    (*monster)->bob_amplitude_min = bob_range_start;
    (*monster)->bob_amplitude_max = bob_range_end;
    if (shadow_width != 0.0f) {
        if (shadow_width < 0.0f) {
            shadow_width = (*monster)->movement.collision_radius * 0.002f;
        }
        if (shadow_depth == 0.0f)
            shadow_depth = shadow_width;
        (*monster)->CreateGroundShadow(shadow_width * g_world_scale, shadow_depth * g_world_scale);
    }
    representation->left_handed = left_handed;
    representation->special_movement = (flies || swims || full_transition) ? 1 : 0;

    if (has_light && representation->monster_light == 0) {
        representation->monster_light = new MonsterLight(
            g_world->dynamic_scene, light_pulsing, (*monster)->movement.collision_radius * 3.0f,
            &light_first, &light_second);
        representation->monster_light->m_vertical_offset = (*monster)->movement.height_offset;
    }

    representation->current_subcycle = 0;
    (*monster)->SetCycle(1);
    (*monster)->SetSubCycle(0);
    srVector3T<float> minimum;
    srVector3T<float> maximum;
    (*monster)->GetAnimationBounds(&minimum, &maximum);
    (*monster)->SetBounds(&minimum, &maximum);
    if (movement_rate < 0.1f)
        movement_rate = 2.0f;
    (*monster)->SetMovementScale(movement_rate);
    (*monster)->SetTurnRate(rotation_rate * static_cast<float>(g_motion_full_turn_radians));

    int navigation_mode = 1;
    if (flies)
        navigation_mode = 2;
    else if (swims)
        navigation_mode = 3;
    else if (quadruped)
        navigation_mode = 5;
    else if (crawls)
        navigation_mode = 6;
    (*monster)->SetNavigationMode(navigation_mode);
    if (spice_monster) {
        (*monster)->hostility_preserved = true;
        (*monster)->owned_object = 0;
        (*monster)->movement.pitch_enabled = false;
    }

    (*monster)->RandomizeAppearanceAndMotion();
    RegisterGrCycle(monster_name, *monster);
    ResumeSharedGameTimers();
    ReleaseReadMeshScratch();
    return success;
}

/* Select one of the four directional states without immediately repeating the
   caller's current state. Random values four and five fold back to zero in the
   original table. */
// FUNCTION: WIZ8 0x004c2e00
unsigned short ChooseDifferentMonsterDirection(unsigned short previous_direction)
{
    unsigned short direction;

    for (;;) {
        direction = static_cast<unsigned short>(Random(6));
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
void W8Monster::RandomizeAppearanceAndMotion()
{
    unsigned int random_value;

    /* 0x004C1D3D: cmp/setl - a signed test against the LEFTHANDED percent. */
    mirror_x = static_cast<int>(Random(100)) < m_pRep->left_handed;

    if (m_pRep->minimum_scale != g_float_zero && m_pRep->maximum_scale != g_float_zero) {
        float minimum = m_pRep->minimum_scale;
        float maximum = m_pRep->maximum_scale;

        if (minimum > maximum) {
            srAssertFail("flStart <= flEnd", MONSTER_CPP, 0x2f5, 0);
        }
        random_value = Random(1000);
        m_pRep->scale = (maximum - minimum) * random_value * g_float_one_thousandth + minimum;
    }

    if (m_pRep->random_idle) {
        int subcycle;
        float playback_scale = (m_pRep->random_idle_fps_max - m_pRep->random_idle_fps_min) *
                                   Random(1000) * g_float_one_thousandth +
                               m_pRep->random_idle_fps_min;

        for (subcycle = 0; subcycle < static_cast<signed char>(m_pRep->animations[1].GetCount());
             ++subcycle) {
            int animation_index = static_cast<signed char>(subcycle);
            W8AnimObj* animation;
            float scale = playback_scale + m_pRep->idle_playback_scale;

            if (animation_index == -1) {
                animation_index = m_pRep->current_subcycle;
            }
            if (animation_index >= m_pRep->animations[1].GetCount()) {
                ShutdownWithErrorBox(FormatString("Monster %s: Missing CYCLE_%s, sub-cycle %d",
                                                  m_pRep->name, g_cycle_names[1].name,
                                                  animation_index));
            }
            animation = *m_pRep->animations[1].GetAt(animation_index);
            if (scale < g_float_one) {
                scale = g_float_one;
            }
            animation->playback_scale = scale;
            if (subcycle < m_pRep->animation_scales[1].GetCount()) {
                *m_pRep->animation_scales[1].GetAt(subcycle) = scale;
            }
        }
    }

    movement.vertical_base = ((hover_base_max - hover_base_min) * static_cast<float>(Random(1000)) *
                                  g_float_one_thousandth +
                              hover_base_min) *
                             g_world_scale;
    movement.vertical_amplitude = ((bob_amplitude_max - bob_amplitude_min) *
                                       static_cast<float>(Random(1000)) * g_float_one_thousandth +
                                   bob_amplitude_min) *
                                  g_world_scale;
    movement.vertical_phase = Random(1000) * g_float_one_thousandth;
    movement.vertical_offset =
        static_cast<float>(sin(movement.vertical_phase * g_motion_full_turn_radians)) *
            movement.vertical_amplitude +
        movement.vertical_base;
    movement.height_offset += movement.vertical_base;
    movement.secondary_height_offset += movement.vertical_base;

    if (this->scale < g_float_one) {
        m_pRep->instance_scale = this->scale;
        m_pRep->apply_instance_scale = true;
    }
}

// FUNCTION: WIZ8 0x004C2010
W8MonsterCycle ParseMonsterCycleName(const char* name, signed char* subcycle)
{
    W8MonsterCycle cycle;
    int index;

    if (name == 0) {
        srAssertFail("pacName", MONSTER_CPP, 1937, 0);
    }

    cycle = W8_MONSTER_CYCLE_NONE;
    for (index = 0; index < W8_MONSTER_CYCLE_COUNT; ++index) {
        if (strncmp(name, g_cycle_names[index].name, g_cycle_names[index].prefix_length) == 0) {
            cycle = static_cast<W8MonsterCycle>(index);
            break;
        }
    }

    /* The model format retains these older names for the first two cycles. */
    if (cycle == W8_MONSTER_CYCLE_NONE) {
        if (strncmp(name, "FLY", 3) == 0) {
            cycle = W8_MONSTER_CYCLE_BIRTH;
        } else if (strncmp(name, "EXPLODE", 7) == 0) {
            cycle = W8_MONSTER_CYCLE_IDLE;
        }
    }

    if (subcycle != 0) {
        *subcycle = 1;
        int suffix = g_cycle_names[cycle].prefix_length;
        if (static_cast<int>(strlen(name)) > suffix && name[suffix] >= '0' && name[suffix] <= '9') {
            /* The retail atoi offset uses the search index even after a fallback. */
            *subcycle = static_cast<signed char>(atoi(name + g_cycle_names[index].prefix_length));
        }
    }
    return cycle;
}

// FUNCTION: WIZ8 0x004bea20
W8MonsterRep::W8MonsterRep()
    : highlight_mask(0), name(0), spell_icons(0), standing_height(0), scale(1.0f),
      minimum_scale(0.0f), maximum_scale(0.0f), death_scale(1.0f), random_idle(0),
      special_movement(0), idle_playback_scale(10.0f), random_idle_fps_min(0),
      random_idle_fps_max(0), left_handed(0), monster_light(0)
{
    for (int index = 0; index < 8; ++index) {
        objects[index] = 0;
    }
    icon_count = 0;
}

/* Read one animation/subcycle into the Monster representation.  The current
   subcycle is the newly appended animation slot; its playback scale and light
   list occupy the two parallel vectors for the same cycle. */
// FUNCTION: WIZ8 0x004BF520
unsigned char W8MonsterRep::ReadCycleData(W8ReadLevelInfo* info, W8Monster* monster,
                                          int cycle_index, int load_all)
{
    W8GrowableVector<stLight*>* lights = new W8Vector<stLight*>;
    W8AnimObj* animation;
    unsigned char success;
    signed char cycle;
    int subcycle;

    if (info == 0 || info->hFile == 0 || monster == 0) {
        srAssertFail("pInfo && pInfo->hFile && pMonster",
                     "C:\\Projects\\Wizardry 8\\Engine Code\\Monster.cpp", 0x22d, 0);
    }

    animation = CreateAnimObj();
    success = AnimObjReadFromFile(info, animation, load_all, lights, 0);
    if (cycle_index == W8_MONSTER_CYCLE_NONE) {
        cycle_index = static_cast<signed char>(animation->cycle);
    }
    cycle = static_cast<signed char>(cycle_index);
    if (cycle < W8_MONSTER_CYCLE_BIRTH || cycle >= W8_MONSTER_CYCLE_COUNT) {
        srAssertFail("bCycle>=CYCLE_FIRST && bCycle<=CYCLE_LAST",
                     "C:\\Projects\\Wizardry 8\\Engine Code\\Monster.cpp", 0xc35, 0);
    }

    animations[cycle].Add(0);
    current_subcycle = static_cast<signed char>(animations[cycle].GetCount() - 1);
    subcycle = current_subcycle;
    if (subcycle < animation_scales[cycle].GetCount()) {
        *animation_scales[cycle].GetAt(subcycle) = animation->playback_scale;
    } else {
        animation_scales[cycle].Add(animation->playback_scale);
    }

    active = 1;
    frame_direction = W8_ANIMATION_FORWARD;
    m_bLOD = 2;
    timer = g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT);
    animation_behaviour = animation->behaviour;
    frame_method = animation->frame_method;
    animation_playing = animation->animation_playing;
    if (current_cycle == -1) {
        current_cycle = cycle;
    }
    if (subcycle < animations[cycle].GetCount()) {
        *animations[cycle].GetAt(subcycle) = animation;
    }

    if (AnimationIsRunning(animation) == 1) {
        signed char list;

        for (list = 0; list < 3; ++list) {
            signed char entry;
            signed char count = static_cast<signed char>(AnimObjListCount(animation, list));

            for (entry = 0; entry < count; ++entry) {
                W8PathAI* path = AnimObjListEntry(animation, list, entry);
                if (path != 0) {
                    PathAISetLooping(path, 1);
                    PathAISetDiscreteMode(path, 1);
                    PathAISetScale(path, animation->playback_scale);
                }
            }
        }
    }

    if (lights->GetCount() == 0) {
        delete lights;
        lights = 0;
    } else {
        monster->SetLights(lights);
    }
    light_lists[cycle].Add(lights);
    return success;
}

// FUNCTION: WIZ8 0x004bebd0
W8MonsterRep::W8MonsterRep(const W8MonsterRep& other)
    : W8EmitterHost(other), highlight_mask(other.highlight_mask), spell_icons(0),
      standing_height(other.standing_height), scale(other.scale),
      minimum_scale(other.minimum_scale), maximum_scale(other.maximum_scale),
      death_scale(other.death_scale), random_idle(other.random_idle),
      special_movement(other.special_movement), idle_playback_scale(other.idle_playback_scale),
      random_idle_fps_min(other.random_idle_fps_min),
      random_idle_fps_max(other.random_idle_fps_max), left_handed(other.left_handed),
      monster_light(0)
{
    signed char cycle;

    for (int index = 0; index < 8; ++index) {
        objects[index] = 0;
    }
    icon_count = 0;
    for (cycle = W8_MONSTER_CYCLE_BIRTH; cycle < W8_MONSTER_CYCLE_COUNT; ++cycle) {
        CopyCycle(cycle, &other, cycle);
    }
    if (other.monster_light != 0) {
        monster_light = new MonsterLight(*other.monster_light);
    }
    name = 0;
    if (other.name != 0) {
        name = new char[strlen(other.name) + 1];
        if (name != 0) {
            strcpy(name, other.name);
        }
    }
}

// FUNCTION: WIZ8 0x004bee50
W8MonsterRep::~W8MonsterRep()
{
    int cycle;
    int index;

    for (cycle = W8_MONSTER_CYCLE_BIRTH; cycle < W8_MONSTER_CYCLE_COUNT; ++cycle) {
        int count = animations[cycle].GetCount();
        for (index = 0; index < count; ++index) {
            W8AnimObj* animation = *animations[cycle].GetAt(index);
            if (animation != 0) {
                DestroyAnimObj(animation);
            }
        }
    }
    for (index = 0; index < 8; ++index) {
        delete objects[index];
    }
    for (cycle = W8_MONSTER_CYCLE_BIRTH; cycle < W8_MONSTER_CYCLE_COUNT; ++cycle) {
        int count = light_lists[cycle].GetCount();
        for (index = 0; index < count; ++index) {
            DestroyLightVector(*light_lists[cycle].GetAt(index));
        }
        light_lists[cycle].Clear();
    }
    while (linked_runtime_objects.GetCount() != 0) {
        linked_runtime_objects.RemoveAtAndDelete(0);
    }
    delete monster_light;
    delete[] name;
}

/* Deep-copy one cycle's animation objects and render lights while retaining
   its per-subcycle scalar values.  The light copies are new scene objects:
   they are registered with the world's light list and detached until the
   owning GrCycle selects this cycle. */
// FUNCTION: WIZ8 0x004bf0f0
void W8MonsterRep::CopyCycle(signed char cycle, const W8MonsterRep* other, signed char other_cycle)
{
    int index;

    for (index = 0; index < other->animations[other_cycle].GetCount(); ++index) {
        animations[cycle].Add(CloneAnimObj(*other->animations[other_cycle].GetAt(index)));
        animation_scales[cycle].Add(*other->animation_scales[other_cycle].GetAt(index));
    }

    for (index = 0; index < other->light_lists[other_cycle].GetCount(); ++index) {
        W8GrowableVector<stLight*>* source_lights = *other->light_lists[other_cycle].GetAt(index);
        W8GrowableVector<stLight*>* copied_lights =
            CloneAnimationLightList(source_lights, MONSTER_CPP, 0x1e5, 0x1ed);
        light_lists[cycle].Add(copied_lights);
    }
}

/* `new stLight` above is what forces this emission: VC6 inlines stLight's own
   empty default constructor at the allocation site but leaves the registry
   base's constructor out of line here. */

/* The representation clone slot is an ordinary virtual copy operation.  The
   allocation size and call to the copy constructor are both visible in the
   emitted body; there is no separate representation wrapper involved. */
// FUNCTION: WIZ8 0x004ca9e0
W8AnimRepBase* W8MonsterRep::Clone()
{
    return new W8MonsterRep(*this);
}

// VTABLE: WIZ8 0x005ed22c W8GrObject
// VTABLE: WIZ8 0x005ed218 W8Navigator
// class W8Monster
/* cvdump preserves a terminal space in this generated thunk's demangled name;
   the explicit name reference must preserve it too. */

// GLOBAL: WIZ8 0x0065ba4c
int g_monster_cycle_registry_weight;

// FUNCTION: WIZ8 0x004bfb00
W8Monster::W8Monster()
    : runtime_flags(0), value_1e0(-1), location_id(-1), scale_x(1.0f), scale_y(1.0f), scale_z(1.0f),
      missile_frame(0), spell_frame(0), talking(0), animate_mouth(false), mouth_frame_clock(0),
      mouth_frame(0), talk_start(0), talk_state(-1), inactive(false), pending_finalize(1),
      disabled(0), nearest_to_party(false), spell_vertex_warned(0), missile_point_warned(0),
      script(0), script_wait(MONSCR_NONE), trigger(0), registry_weight(0), spell_effect_armed(0),
      sector_mesh(0), fade_state(W8_MONSTER_FADE_IDLE), target_highlighted(false),
      hostility_preserved(false), sound(0)
{
    formation.SetZero();
    kind = 1;

    m_pRep = new W8MonsterRep;
    m_pRep->spell_icons = PLCreate();

    trace_mask = 2;
    direction_x = 0;
    direction_y = 0;
    direction_z = 0;
    target_scale = 1.0f;
    current_scale = 1.0f;
    sunlit_state = W8_MONSTER_SUNLIGHT_UNKNOWN;
    orders_finished = false;
    defining_orders = false;
    deaf = false;
    face_party = false;
    stay_home = false;
    patrol_index = 0;
    look_frequency = 0;
    look_duration = 0;
    removal_state = W8_MONSTER_REMOVAL_NONE;
    cycle_callback = 0;
    move_dirty = true;
    order_mode = W8_MONSTER_ORDER_NONE;
}

// FUNCTION: WIZ8 0x004bfe00
W8Monster::W8Monster(const W8Monster& rhs)
    : W8GrCycle(rhs), runtime_flags(rhs.runtime_flags), value_1e0(rhs.value_1e0),
      location_id(rhs.location_id), scale_x(1.0f), scale_y(1.0f), scale_z(1.0f),
      missile_frame(rhs.missile_frame), spell_frame(rhs.spell_frame), talking(0),
      animate_mouth(false), mouth_frame_clock(0), mouth_frame(0), talk_start(0), talk_state(-1),
      inactive(rhs.inactive), pending_finalize(1), disabled(0), nearest_to_party(false),
      hover_base_min(rhs.hover_base_min), hover_base_max(rhs.hover_base_max),
      bob_amplitude_min(rhs.bob_amplitude_min), bob_amplitude_max(rhs.bob_amplitude_max),
      spell_vertex_warned(0), missile_point_warned(0), script(0), script_wait(MONSCR_NONE),
      trigger(0), registry_weight(rhs.registry_weight), spell_effect_armed(0), sector_mesh(0),
      sound(0)
{
    formation.SetZero();
    fade_state = W8_MONSTER_FADE_IDLE;
    target_highlighted = false;
    hostility_preserved = rhs.hostility_preserved;

    if (rhs.m_pRep == 0) {
        srAssertFail("rhs.m_pRep", "C:\\Projects\\Wizardry 8\\Engine Code\\Monster.cpp", 1099, 0);
    }
    m_pRep = static_cast<W8MonsterRep*>(rhs.m_pRep->Clone());
    m_pRep->spell_icons = PLCreate();

    defining_orders = false;
    orders_finished = false;
    order_mode = W8_MONSTER_ORDER_NONE;
    deaf = false;
    face_party = false;
    stay_home = false;
    patrol_index = 0;
    direction_x = 0;
    direction_y = 0;
    direction_z = 0;
    look_frequency = 0;
    look_duration = 0;
    sunlit_state = W8_MONSTER_SUNLIGHT_UNKNOWN;
    move_dirty = true;
    target_scale = 1.0f;
    current_scale = 1.0f;
    runtime_flags &= ~(W8_MONSTER_KEEP_FRAME_DIRECTION | W8_MONSTER_SCALING_Y | W8_MONSTER_PARKED |
                       W8_MONSTER_REMOVE_AFTER_FADE | W8_MONSTER_REMOVE_NOW);
    removal_state = W8_MONSTER_REMOVAL_NONE;
    cycle_callback = 0;
    if (hostility_preserved) {
        active = false;
    }
}

// FUNCTION: WIZ8 0x004c0170
W8Monster::~W8Monster()
{
    SetLights(0);
    if (m_pRep->spell_icons != 0) {
        ClearMonsterSpellIcons(this);
        PLDestroy(m_pRep->spell_icons);
    }
    if (IsSoleRegisteredCycleForName()) {
        g_monster_cycle_registry_weight -= registry_weight;
        RemoveCycleSkinTables();
    }
    UnregisterGrCycle(this);
    delete m_pRep;
    ReleaseRendererObject(script);
    runtime_flags &= ~W8_MONSTER_SCRIPT_WAIT;
    script_line = 0;
    script_wait = MONSCR_NONE;
    ReleaseRendererObject(sound);
}

/* Navigator is W8Monster's second base at +0x18. VC6 places this override in
   that secondary table and emits the adjusted entry form at 0x004CA840. */
// FUNCTION: WIZ8 0x004ca840
void W8Monster::SetPosition(const srVector3T<float>* position)
{
    GetRepresentation()->SetLocation(position);
    m_pRep->SetLocation(position);
    SetPositionInternal(position);
    position_dirty = true;
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
    float distance;
    int cycle;
    int index;

    if (m_pRep == 0) {
        srAssertFail("m_pRep", MONSTER_CPP, 0x7f9, 0);
    }

    GetCameraPosition(&party_position);
    {
        srVector3T<float> position = GetPosition();
        monster_position = position;
    }
    distance = (monster_position - party_position).Length();

    if (sunlit_state == W8_MONSTER_SUNLIGHT_UNKNOWN || g_monster_light_scale < g_float_one) {
        current_scale = 0.75f;
        g_monster_model_instances.Clear();
        CollectModelInstances(&g_monster_model_instances);
        for (index = 0; index < g_monster_model_instances.GetCount(); ++index) {
            stModelInstance* model = *g_monster_model_instances.GetAt(index);
            model->light_scale = 0.75f;
        }
        target_scale = 0.75f;
        light_scale_timer.SetDuration(0.025f);
        light_scale_timer.Restart();
        sunlit_state = W8_MONSTER_SUNLIGHT_LIT;
        move_dirty = true;
    }

    if (g_monster_shadow_updates_enabled != 0 && (move_dirty || UpdateTrackedPosition())) {
        move_dirty = false;
        if (distance < WorldGetRenderRange(g_world)) {
            srNode* sun = static_cast<srNode*>(
                srCore.getRegistry()->find(g_world->dynamic_scene->getClassNode(), "SUN", 0));
            if (sun != 0) {
                srVector3T<float> mapped_position;
                srVector3T<double> sun_location = sun->getLocation();
                srVector3T<float> sun_position;

                GetMappedPosition(&mapped_position);
                sun_position = sun_location;
                if (g_octree->HasLineOfSight(&mapped_position, &sun_position, true)) {
                    if (sunlit_state == W8_MONSTER_SUNLIGHT_SHADOWED) {
                        target_scale = 0.75f;
                        light_scale_timer.SetDuration(0.025f);
                        light_scale_timer.Restart();
                        sunlit_state = W8_MONSTER_SUNLIGHT_LIT;
                    }
                } else if (sunlit_state == W8_MONSTER_SUNLIGHT_LIT) {
                    target_scale = 0.0f;
                    light_scale_timer.SetDuration(0.025f);
                    light_scale_timer.Restart();
                    sunlit_state = W8_MONSTER_SUNLIGHT_SHADOWED;
                }
            }
        }
    }

    if (current_scale != target_scale && light_scale_timer.GetProgress() >= g_float_one) {
        if (target_scale <= current_scale) {
            current_scale -= g_facing_tolerance0;
            if (current_scale < target_scale) {
                current_scale = target_scale;
            }
        } else {
            current_scale += g_facing_tolerance0;
            if (current_scale > target_scale) {
                current_scale = target_scale;
            }
        }
        g_monster_model_instances.Clear();
        CollectModelInstances(&g_monster_model_instances);
        for (index = 0; index < g_monster_model_instances.GetCount(); ++index) {
            stModelInstance* model = *g_monster_model_instances.GetAt(index);
            model->light_scale = current_scale;
        }
        light_scale_timer.Restart();
    }

    if (fade_state != W8_MONSTER_FADE_IDLE) {
        float progress = fade_timer.GetProgress();
        if (progress > g_float_one) {
            progress = g_float_one;
        }
        if (fade_state == W8_MONSTER_FADE_DELAYED_REMOVAL) {
            if (progress == g_float_one) {
                fade_state = W8_MONSTER_FADE_IDLE;
                fade_timer.SetDuration(3.0f);
                fade_timer.Restart();
                if (fade_state < W8_MONSTER_FADE_IN) {
                    m_pRep->instance_scale = g_float_one;
                    m_pRep->apply_instance_scale = true;
                    fade_state = W8_MONSTER_FADE_OUT;
                } else {
                    fade_timer.SetProgress(g_float_one - m_pRep->instance_scale);
                    fade_state = W8_MONSTER_FADE_OUT;
                }
            }
        } else {
            if (fade_state < W8_MONSTER_FADE_IN) {
                m_pRep->instance_scale = g_float_one - progress;
            } else {
                m_pRep->instance_scale = progress;
            }
            m_pRep->apply_instance_scale = true;
            if (progress == g_float_one) {
                if (fade_state < W8_MONSTER_FADE_IDLE) {
                    runtime_flags |= W8_MONSTER_FADED_OUT;
                }
                fade_state = W8_MONSTER_FADE_IDLE;
            }
        }
    }

    cycle = Query(W8_MONSTER_QUERY_CYCLE);
    SetGroundShadowVisible(cycle != W8_MONSTER_CYCLE_BIRTH && cycle != W8_MONSTER_CYCLE_DIE &&
                           fade_state == W8_MONSTER_FADE_IDLE &&
                           (runtime_flags & W8_MONSTER_FADED_OUT) == 0);

    {
        unsigned int monster_index =
            MonsterGetIndexByLocationID(0x86a, MONSTER_CPP, location_id, false);
        if (monster_index != 0xffffffff) {
            monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
        }
    }

    if (!IsMipeActive()) {
        if ((cycle == W8_MONSTER_CYCLE_IDLE || cycle == W8_MONSTER_CYCLE_SPICE) &&
            m_pRep->pending_cycle == -1 && !movement_stopped && !halted) {
            flags |= 0x100000;
        }

        if (monster_info == 0) {
            UpdateNavigation(0, false);
        } else {
            UpdateNavigation(monster_info->highest_condition >= W8_CONDITION_WEBBED,
                             monster_info->uiCondition[W8_CONDITION_SLOWED] != 0);
        }

        if (cycle != W8_MONSTER_CYCLE_DIE && script != 0 && !gXStatus.fCombatMode) {
            ProcessScript();
            cycle = Query(W8_MONSTER_QUERY_CYCLE);
        }

        if ((runtime_flags & W8_MONSTER_SCRIPT_WAIT) == 0 && m_pRep->pending_cycle == -1) {
            switch (cycle) {
            case W8_MONSTER_CYCLE_TALK:
                if (Query(W8_MONSTER_QUERY_CYCLE_COMPLETE) != 0) {
                    if (talk_duration < GetTickCount() - talk_start && IsCycleSupported(0x17)) {
                        m_pRep->pending_cycle = 0x17;
                    } else {
                        m_pRep->pending_cycle = 0x18;
                    }
                    m_pRep->frame_direction = W8_ANIMATION_FORWARD;
                    m_pRep->pending_behaviour = W8_ANIMATION_PLAY_ONCE;
                    m_pRep->animation_playing = 1;
                    m_pRep->timer = g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT);
                    m_pRep->pending_subcycle = 0;
                }
                break;
            case W8_MONSTER_CYCLE_TALK_SPICE:
                if (Query(W8_MONSTER_QUERY_CYCLE_COMPLETE) != 0) {
                    if (talking) {
                        /* 0x004C276F..0x004C27D7: the talking path never stores
                           pending_subcycle, so TALK keeps the current frame
                           (clamped by ApplyPendingCycle) instead of frame 0. */
                        talk_state = 0x17;
                        talk_start = GetTickCount();
                        talk_duration = Random(2000) + 2000;
                        m_pRep->pending_cycle = 0x18;
                        m_pRep->frame_direction = W8_ANIMATION_FORWARD;
                        m_pRep->pending_behaviour = W8_ANIMATION_PLAY_ONCE;
                        m_pRep->animation_playing = 1;
                        m_pRep->timer = g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT);
                    } else {
                        m_pRep->pending_cycle = 1;
                        m_pRep->animation_playing = 1;
                        m_pRep->timer = g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT);
                        m_pRep->pending_subcycle = 0;
                    }
                }
                break;
            case W8_MONSTER_CYCLE_SPELL:
                if (Query(W8_MONSTER_QUERY_CYCLE_COMPLETE) != 0) {
                    m_pRep->pending_cycle = 1;
                    m_pRep->frame_direction = W8_ANIMATION_FORWARD;
                    m_pRep->animation_playing = 1;
                    m_pRep->timer = g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT);
                    m_pRep->pending_behaviour = W8_ANIMATION_NEVER_STOP;
                    m_pRep->pending_subcycle = 0;
                }
                break;
            case W8_MONSTER_CYCLE_WALK:
                if (movement_stopped || halted) {
                    bool transition = Query(W8_MONSTER_QUERY_AT_PLAYBACK_END) != 0 || wrapped;
                    if (!transition && !m_pRep->special_movement) {
                        transition =
                            Query(W8_MONSTER_QUERY_FRAME) < Query(W8_MONSTER_QUERY_FRAME_COUNT) / 2;
                    }
                    if (transition) {
                        while (values.GetCount() != 0) {
                            SoundStop(*values.GetAt(0));
                            values.RemoveAt(0);
                        }
                        if (!IsCycleSupported(3)) {
                            m_pRep->pending_behaviour = W8_ANIMATION_NEVER_STOP;
                            m_pRep->pending_cycle = 1;
                        } else {
                            m_pRep->pending_cycle = 3;
                            m_pRep->frame_direction = W8_ANIMATION_REVERSE;
                            m_pRep->pending_behaviour = W8_ANIMATION_PLAY_ONCE;
                            m_pRep->pending_subcycle = static_cast<unsigned short>(
                                Query(W8_MONSTER_QUERY_FRAME_COUNT) - 1);
                            runtime_flags |= W8_MONSTER_KEEP_FRAME_DIRECTION;
                        }
                        m_pRep->animation_playing = 1;
                        m_pRep->timer = g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT);
                    }
                }
                break;
            case W8_MONSTER_CYCLE_IDLE:
            case W8_MONSTER_CYCLE_SPICE:
                if (!movement_stopped && !halted &&
                    (Query(W8_MONSTER_QUERY_AT_PLAYBACK_END) != 0 || wrapped)) {
                    flags &= ~0x100000;
                    if (!IsCycleSupported(3)) {
                        m_pRep->pending_behaviour = W8_ANIMATION_NEVER_STOP;
                        m_pRep->pending_cycle = 4;
                    } else {
                        m_pRep->pending_cycle = 3;
                        m_pRep->frame_direction = W8_ANIMATION_FORWARD;
                        m_pRep->pending_behaviour = W8_ANIMATION_PLAY_ONCE;
                        m_pRep->pending_subcycle = 0;
                    }
                    m_pRep->animation_playing = 1;
                    m_pRep->timer = g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT);
                }
                break;
            case W8_MONSTER_CYCLE_TRANSITION:
                if (Query(W8_MONSTER_QUERY_CYCLE_COMPLETE) != 0) {
                    if (m_pRep->frame_direction == W8_ANIMATION_REVERSE) {
                        m_pRep->frame_direction = W8_ANIMATION_FORWARD;
                        m_pRep->pending_cycle = 1;
                    } else {
                        m_pRep->pending_cycle = 4;
                    }
                    m_pRep->animation_playing = 1;
                    m_pRep->timer = g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT);
                    m_pRep->pending_behaviour = W8_ANIMATION_NEVER_STOP;
                    m_pRep->pending_subcycle = 0;
                }
                break;
            case W8_MONSTER_CYCLE_BIRTH:
                if (Query(W8_MONSTER_QUERY_CYCLE_COMPLETE) != 0) {
                    m_pRep->pending_cycle = 1;
                    m_pRep->animation_playing = 1;
                    m_pRep->timer = g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT);
                    m_pRep->pending_behaviour = W8_ANIMATION_NEVER_STOP;
                    m_pRep->pending_subcycle = 0;
                }
                break;
            }
        }
    }

    if (m_pRep->active == 0) {
        return;
    }

    UpdateAttachedObjects();
    cycle = Query(W8_MONSTER_QUERY_CYCLE);
    if (gfKeyState[VK_CONTROL] && g_combat_state != 0 &&
        (g_combat_state->round_active || gXStatus.fPartyMovementMode) &&
        (cycle == W8_MONSTER_CYCLE_IDLE || cycle == W8_MONSTER_CYCLE_SPICE) &&
        (m_pRep->pending_cycle == -1 || m_pRep->pending_cycle == 1 || m_pRep->pending_cycle == 2) &&
        (g_combat_state->eCombatActionStatus != 2 || g_combat_state->pActionMonsterInfo == 0 ||
         g_combat_state->pActionMonsterInfo->location_id != location_id)) {
        m_pRep->timer = g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT);
    }

    if (monster_info != 0 && monster_info->uiCondition[W8_CONDITION_SLOWED] != 0) {
        TickAnimation(Query(W8_MONSTER_QUERY_CYCLE) == W8_MONSTER_CYCLE_WALK
                          ? movement.movement_speed * g_float_half
                          : 0.5f);
    } else {
        TickAnimation(Query(W8_MONSTER_QUERY_CYCLE) == W8_MONSTER_CYCLE_WALK
                          ? movement.movement_speed
                          : 1.0f);
    }
    InitializeAnimatedTexture();

    if (sound != 0) {
        srVector3T<float> position = GetPosition();
        srVector3T<double> location;
        location.SetFromFloat(&position);
        sound->setLocation(location);
    }
}

/* Script IF expressions are deliberately small: a name followed by an
   optional comma-separated float list. The retail interpreter supports the
   two predicates below and treats every other name as false. */
// FUNCTION: WIZ8 0x004C9DC0
bool W8Monster::EvaluateScriptCondition(const char* expression)
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
            parameters.Add(static_cast<float>(atof(argument)));
            argument = strtok(0, ",)");
        }
        *argument_list = 0;
    }

    if (_strnicmp(buffer, "PARTYNEAR", 9) == 0) {
        srVector3T<float> party_position;
        srVector3T<float> monster_position;

        GetCameraPosition(&party_position);
        if (parameters.GetCount() == 0) {
            srAssertFail("lsParmList.Length()", MONSTER_CPP, 7526,
                         FormatString("Monscr %s line %d: PARTYNEAR expects 1 parameter",
                                      script->getName(), script_line));
        }

        srVector3T<float> current_position = GetPosition();
        monster_position = current_position;
        if (!g_status.world_suspended &&
            (party_position - monster_position).Length() < *parameters.GetAt(0) * g_world_scale &&
            MonsterInfoFromID(7533, MONSTER_CPP, location_id, true)
                    ->player_visibility.line_of_sight != 0) {
            return true;
        }
    } else if (_strnicmp(buffer, "RANDOM", 6) == 0) {
        if (parameters.GetCount() == 0) {
            srAssertFail("lsParmList.Length()", MONSTER_CPP, 7542,
                         FormatString("Monscr %s line %d: RANDOM expects 1 parameter",
                                      script->getName(), script_line));
        }
        if (Chance(static_cast<unsigned int>(*parameters.GetAt(0))) != 0) {
            return true;
        }
    }
    return false;
}

/* Replace the current Monster script with an existing named runtime object or
   load it from the Monster script directory on first use. Registry ownership
   is balanced the same way on both paths: a newly loaded script is marked for
   automatic release, then the Monster takes its own reference. */
// FUNCTION: WIZ8 0x004C7F10
bool W8Monster::SetScript(const char* script_name, bool reset_orders)
{
    srRegistry* registry;

    ReleaseRendererObject(script);
    runtime_flags &= ~W8_MONSTER_SCRIPT_WAIT;
    script_line = 0;
    script_wait = MONSCR_NONE;
    ReleaseRendererObject(sound);
    if (reset_orders) {
        orders_finished = false;
    }

    registry = srCore.getRegistry();
    script = static_cast<stScript*>(registry->find(stScript::sGetClassNode(), script_name, 0));
    if (script == 0) {
        /* Retail builds the path only on a registry miss, in a MAX_PATH buffer. */
        char path[MAX_PATH] = "Data\\Monsters\\Scripts\\";
        strcat(path, script_name);
        script = new stScript;
        if (script != 0) {
            if (script->Load(path) != 0) {
                script->setName(script_name);
                script->autoRelease();
            } else {
                script->release();
                script = 0;
            }
        }
    }
    if (script == 0) {
        return false;
    }
    script->addReference();
    return true;
}

// FUNCTION: WIZ8 0x004C77F0
bool W8Monster::GetProjectilePosition(srVector3T<float>* position)
{
    signed char cycle;
    bool result;

    if (position == 0) {
        return false;
    }
    if (IsCycleSupported(0x11) && IsCycleSupported(0x0d)) {
        srAssertFail("!(IsCycleSupported(CYCLE_ATTACK_SHOOT) && "
                     "IsCycleSupported(CYCLE_ATTACK_THROW))",
                     MONSTER_CPP, 0x18b1, 0);
    }
    if (IsCycleSupported(0x0d)) {
        cycle = W8_MONSTER_CYCLE_ATTACK_THROW;
    } else if (IsCycleSupported(0x11)) {
        cycle = W8_MONSTER_CYCLE_ATTACK_SHOOT;
    } else if (IsCycleSupported(7)) {
        cycle = W8_MONSTER_CYCLE_ATTACK_RANGED;
    } else {
        return false;
    }

    result = GetCycleMappedPosition(cycle, 5, position);
    if (!result && !missile_point_warned) {
        if (g_dev_mode) {
            W8MonsterInfo* info = MonsterGetScriptPartByLocationIndex(
                MonsterGetIndexByLocationID(0x18c3, MONSTER_CPP, location_id, true));
            FormatDebugMessage(0, "WARNING: %ls does not have a MISSILE vertex marked --> Lee!",
                               GetMonsterDataForInfo(info));
        }
        missile_point_warned = true;
    }
    return result;
}

/* Cycle 25 mapping six is the spell launch vertex. Missing mappings warn at
   most once per Monster, while the result reports whether a launch position was found. */
// FUNCTION: WIZ8 0x004c78e0
bool W8Monster::GetSpellPosition(srVector3T<float>* position)
{
    bool found;

    if (position == 0) {
        return false;
    }
    found = GetCycleMappedPosition(0x19, 6, position);
    if (!found && !spell_vertex_warned) {
        if (g_dev_mode) {
            W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(
                MonsterGetIndexByLocationID(0x18e4, MONSTER_CPP, location_id, true));
            W8MonsterRecord* record = GetMonsterDataForInfo(monster_info);
            FormatDebugMessage(0, g_warning_missing_spell_vertex, record);
        }
        spell_vertex_warned = true;
    }
    return found;
}

// FUNCTION: WIZ8 0x004C7960
bool W8Monster::GetCycleMappedPosition(signed char cycle, int mapped_index,
                                       srVector3T<float>* position)
{
    srModelInstance* current_model = GetCurrentModelInstance();
    W8GrowableVector<W8AnimObj*>* animations;
    W8AnimObj* animation;
    int subcycle;
    int dispatch_value;

    if (cycle == W8_MONSTER_CYCLE_NONE) {
        cycle = m_pRep->current_cycle;
        subcycle = m_pRep->current_subcycle;
    } else {
        subcycle = 0;
    }
    animations = &m_pRep->animations[cycle];
    if (animations->GetCount() <= subcycle) {
        ShutdownWithErrorBox(
            reinterpret_cast<const char*>( // reinterpret-ok: String returns a logging buffer
                String("Monster %s: Missing CYCLE_%s, sub-cycle %d", m_pRep->name,
                       g_cycle_names[cycle].name, subcycle)));
    }
    animation = *animations->GetAt(subcycle);

    /* 0x004C7A3A..0x004C7A50: the missile vertex (5) is sampled on
       missile_frame (0x1f4) and the spell vertex (6) on spell_frame (0x1f8). */
    if (mapped_index == 5) {
        dispatch_value = missile_frame;
    } else if (mapped_index == 6) {
        dispatch_value = spell_frame;
    } else {
        dispatch_value = 0;
    }
    if (animation == 0) {
        return false;
    }
    if (AnimationIsRunning(animation) != 0) {
        return GetAnimationCenter(position);
    }

    srModelInstance* model = AnimObjDispatch(animation, 2, dispatch_value);
    if (model != 0) {
        stMeshModel* mesh = static_cast<stMeshModel*>(model->getModel());
        int vertex = FindMappedIndexInMeshChain(&mesh, mapped_index);
        if (mesh != 0 && vertex != -1) {
            srVector3T<float>* vertices = mesh->GetVertexLocations(dispatch_value, true, 0.0f);
            if (vertices != 0 && current_model != 0) {
                srMatrix3T<float> rotation;
                current_model->getWorldSpaceRotation(rotation);
                srVector3T<float> local = vertices[vertex] * m_pRep->scale;
                srVector3T<float> rotated = rotation.Transform(local);
                srVector3T<float> owner_position = GetPosition();
                position->Set(rotated.x + owner_position.x,
                              rotated.y + owner_position.y + movement.vertical_base,
                              rotated.z + owner_position.z);
                return true;
            }
        }
    }
    return false;
}

bool W8Monster::ResolveScriptPosition(const char* name, srVector3T<float>* position)
{
    if (_stricmp(name, "home") == 0) {
        *position = formation;
    } else if (_stricmp(name, "off_camera") == 0) {
        position->x = position->y = position->z = -10000000.0f;
    } else if (!FindEntityByName(name, position, 0, 0)) {
        return false;
    }
    return true;
}

/* Execute source lines until a command starts an asynchronous operation, ends
   the script, or the runaway-command guard trips. The two command modes share
   the original table: normal mode performs actions, while BEGINORDERS records
   the persistent movement policy that the Navigator update consumes. */
// FUNCTION: WIZ8 0x004C80E0
void W8Monster::ProcessScript()
{
    W8MonsterInfo* monster_info;
    unsigned int monster_index;
    int command_count;
    bool stop;
    /* 0x004C8116 clears this byte once per call, not per command: every
       POINTPATROL/RANDOMPOINTPATROL resolved point in the same pass counts,
       so a later patrol line appends to the earlier one instead of clearing
       the list, and the signed byte test at 0x004C9855/0x004C9A49 decides
       whether the mode is set. */
    signed char patrol_points = 0;

    if (script == 0) {
        return;
    }

    monster_index = MonsterGetIndexByLocationID(0x1a4a, MONSTER_CPP, location_id, true);
    monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
    if (orders_finished) {
        if (monster_info == 0 || (monster_info->ai_mode & W8_MONSTER_AI_RESTORE_SCRIPT) == 0) {
            return;
        }
        monster_info->ai_mode &= ~W8_MONSTER_AI_RESTORE_SCRIPT;
        return;
    }

    if (monster_info != 0 && (monster_info->ai_mode & W8_MONSTER_AI_RESTORE_SCRIPT) != 0) {
        monster_info->ai_mode &= ~W8_MONSTER_AI_RESTORE_SCRIPT;
        if (script_wait == MONSCR_WALKTO && movement_stopped) {
            if (script_line > 0) {
                --script_line;
            }
        } else if (!CanContinueScript()) {
            return;
        }
    } else if (!CanContinueScript()) {
        return;
    }

    script_wait = MONSCR_NONE;
    stop = false;
    command_count = 0;
    while (script != 0 && script_wait == MONSCR_NONE && !stop) {
        char line[256];
        char* token;
        W8MonsterScriptCommand command;
        stScriptLine* source_line;

        if (script_line >= script->lines.GetCount()) {
            break;
        }
        source_line = *script->lines.GetAt(script_line++);
        if (source_line == 0 || source_line->text == 0) {
            continue;
        }
        strcpy(line, source_line->text);
        token = strtok(line, " \t");
        if (token == 0) {
            continue;
        }

        command = MONSCR_NONE;
        for (int index = 0; index < MONSCR_COUNT; ++index) {
            if (_stricmp(token, g_monster_script_commands[index]) == 0) {
                command = static_cast<W8MonsterScriptCommand>(index);
                break;
            }
        }

        if (script_conditions.GetCount() != 0 && !(*script_conditions.GetAt(0)) &&
            command != MONSCR_ELSE && command != MONSCR_ENDIF) {
            if (++command_count > 50) {
                stop = true;
            }
            continue;
        }

        if (!defining_orders) {
            switch (command) {
            case MONSCR_GOTO: {
                token = strtok(0, " \t");
                if (token != 0) {
                    int line_number = script->FindLabelLine(token);
                    if (line_number != -1) {
                        script_conditions.Clear();
                        script_line = line_number;
                    }
                }
                stop = true;
                break;
            }
            case MONSCR_WALKTO: {
                srVector3T<float> position;
                token = strtok(0, " \t");
                if (token == 0) {
                    break;
                }
                if (!ResolveScriptPosition(token, &position)) {
                    ShutdownWithErrorBox(
                        FormatString("MonScript %s Line %d: Unknown location %s", script->getName(),
                                     script->GetSourceLine(script_line - 1), token));
                    break;
                }
                if (_stricmp(token, "PARTY") == 0) {
                    SetMovementTargetToNavigator(g_startup_world, 5.0);
                } else {
                    ConfigureMovementToPosition(&position);
                }
                token = strtok(0, " \t");
                if (token == 0 || _stricmp(token, "NOBLOCK") != 0) {
                    script_wait = MONSCR_WALKTO;
                } else {
                    stop = true;
                }
                break;
            }
            case MONSCR_FACE: {
                srVector3T<float> position;
                token = strtok(0, " \t");
                if (token == 0) {
                    break;
                }
                if (!ResolveScriptPosition(token, &position)) {
                    ShutdownWithErrorBox(
                        FormatString("MonScript %s Line %d: Unknown location %s", script->getName(),
                                     script->GetSourceLine(script_line - 1), token));
                    break;
                }
                AimAtPosition(&position);
                token = strtok(0, " \t");
                if (token == 0 || _stricmp(token, "NOBLOCK") != 0) {
                    script_wait = MONSCR_FACE;
                } else {
                    stop = true;
                }
                break;
            }
            case MONSCR_SAY:
            case MONSCR_NPCINTERACTION: {
                int line_number = -1;
                bool suppress = false;
                token = strtok(0, " \t");
                if (token != 0) {
                    line_number = atoi(token);
                    token = strtok(0, " \t");
                    if (token != 0 && _strnicmp(token, "SUPPRESS", 8) == 0) {
                        suppress = true;
                    }
                }
                ForwardNpcScriptNotice(
                    FindNpcBindingForMonster(MonsterGetIndexByLocationID(
                        command == MONSCR_SAY ? 0x1ac9 : 0x1b77, MONSTER_CPP, location_id, true)),
                    0, line_number, command == MONSCR_SAY ? 1 : suppress);
                if (command == MONSCR_NPCINTERACTION) {
                    script_wait = MONSCR_NPCINTERACTION;
                } else if (token == 0 || _stricmp(token, "NOBLOCK") != 0) {
                    script_wait = MONSCR_SAY;
                } else {
                    stop = true;
                }
                break;
            }
            case MONSCR_CYCLE: {
                signed char subcycle;
                token = strtok(0, " \t");
                if (token != 0) {
                    W8MonsterCycle cycle = ParseMonsterCycleName(token, &subcycle);
                    if (cycle != W8_MONSTER_CYCLE_NONE) {
                        m_pRep->pending_cycle = static_cast<signed char>(cycle);
                        m_pRep->animation_playing = 1;
                        m_pRep->timer = g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT);
                        SetSubCycle(0);
                        m_pRep->forced_subcycle = subcycle - 1;
                        if (m_pRep->pending_cycle == -1) {
                            m_pRep->pending_cycle = m_pRep->current_cycle;
                        }
                        runtime_flags |= W8_MONSTER_KEEP_SUBCYCLE;
                        token = strtok(0, " \t");
                        if (token == 0 || _stricmp(token, "NOBLOCK") != 0) {
                            m_pRep->pending_behaviour = W8_ANIMATION_PLAY_ONCE;
                            runtime_flags |= W8_MONSTER_SCRIPT_WAIT;
                            script_wait = MONSCR_CYCLE;
                        } else {
                            stop = true;
                        }
                    } else {
                        ShutdownWithErrorBox(FormatString(
                            "MonScript %s Line %d: Unknown cycle %s", script->getName(),
                            script->GetSourceLine(script_line - 1), token));
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
                unsigned int owner = static_cast<unsigned int>(atoi(token));
                token = strtok(0, " \t");
                if (token == 0) {
                    break;
                }
                if (!ResolveScriptPosition(token, &target)) {
                    break;
                }
                if (!GetProjectilePosition(&source)) {
                    GetMappedPosition(&source);
                }
                FireMissile(owner, &source, &target, 0, 0, 1, 50000.0f);
                break;
            }
            case MONSCR_GIVE:
                token = strtok(0, " \t");
                if (token != 0) {
                    int item = FindItemRecordByName(token);
                    if (item != -1) {
                        CreateItemIntoHandOrPool(item, true);
                    }
                }
                break;
            case MONSCR_TELEPORT: {
                srVector3T<float> position;
                token = strtok(0, " \t");
                if (token != 0) {
                    if (!ResolveScriptPosition(token, &position)) {
                        ShutdownWithErrorBox(FormatString(
                            "MonScript %s Line %d: Unknown location %s", script->getName(),
                            script->GetSourceLine(script_line - 1), token));
                        break;
                    }
                    SetPositionInternal(&position);
                }
                break;
            }
            case MONSCR_DIE:
                m_pRep->pending_behaviour = W8_ANIMATION_PLAY_ONCE;
                m_pRep->pending_cycle = 0x15;
                m_pRep->animation_playing = 1;
                m_pRep->timer = g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT);
                SetSubCycle(0);
                break;
            case MONSCR_END:
                stop = true;
                script_line = script->lines.GetCount();
                break;
            case MONSCR_IF: {
                bool invert = false;
                bool value;
                /* 0x004C8A52: a bare IF is skipped without pushing a condition.
                   After NOT, a missing name is still passed through
                   (0x004C8A86 keeps %edi = 0). */
                token = strtok(0, " \t");
                if (token == 0) {
                    break;
                }
                if (_strnicmp(token, "NOT", 3) == 0) {
                    invert = true;
                    token = strtok(0, " \t");
                }
                value = EvaluateScriptCondition(token);
                script_conditions.InsertAt(0, invert ? !value : value);
                break;
            }
            case MONSCR_ELSE:
                if (script_conditions.GetCount() != 0) {
                    *script_conditions.GetAt(0) = !(*script_conditions.GetAt(0));
                } else {
                    ShutdownWithErrorBox(
                        FormatString("MonScript %s Line %d: ELSE without matching IF",
                                     script->getName(), script->GetSourceLine(script_line - 1)));
                }
                break;
            case MONSCR_ENDIF:
                if (script_conditions.GetCount() != 0) {
                    script_conditions.RemoveAt(0);
                } else {
                    ShutdownWithErrorBox(
                        FormatString("MonScript %s Line %d: ENDIF without matching IF",
                                     script->getName(), script->GetSourceLine(script_line - 1)));
                }
                break;
            case MONSCR_DISPOSITION: {
                unsigned char disposition = 0xff;
                token = strtok(0, " \t");
                if (token != 0) {
                    if (_stricmp(token, "DISP_NEUTRAL") == 0)
                        disposition = 0;
                    else if (_stricmp(token, "DISP_HOSTILE") == 0)
                        disposition = 1;
                    else if (_stricmp(token, "DISP_FRIENDLY") == 0)
                        disposition = 2;
                    monster_info = MonsterGetScriptPartByLocationIndex(
                        MonsterGetIndexByLocationID(0x1b8b, MONSTER_CPP, location_id, true));
                    if (monster_info != 0) {
                        unsigned int group_index = GetMonsterGroupIndexByID(
                            0x1b90, MONSTER_CPP, monster_info->monster_group_id, false);
                        if (group_index != static_cast<unsigned int>(-1)) {
                            W8MonsterGroup* group = GetMonsterGroupByListIndex(group_index);
                            if (group != 0) {
                                SetMonsterGroupHostility(group, disposition, false);
                            }
                        }
                    }
                }
                break;
            }
            case MONSCR_DISAPPEAR:
                runtime_flags |= W8_MONSTER_PARKED;
                break;
            case MONSCR_LOOKHERE:
                PointCameraAtMonster(
                    MonsterGetScriptPartByLocationIndex(
                        MonsterGetIndexByLocationID(0x1ba0, MONSTER_CPP, location_id, true)),
                    true, true);
                token = strtok(0, " \t");
                if (token == 0 || _stricmp(token, "NOBLOCK") != 0) {
                    script_wait = MONSCR_LOOKHERE;
                } else {
                    stop = true;
                }
                break;
            case MONSCR_PATROL: {
                srVector3T<float> home;
                float distance;
                float variation;
                token = strtok(0, " \t");
                if (token == 0)
                    break;
                distance = static_cast<float>(atof(token)) * g_world_scale;
                token = strtok(0, " \t");
                if (token == 0)
                    break;
                variation = static_cast<float>(atof(token)) * g_world_scale;
                home = formation;
                if (IsZeroVector(&home) != 0) {
                    srVector3T<float> current = GetPosition();
                    home = current;
                    formation = home;
                }
                StartPatrol(&home, distance, variation);
                break;
            }
            case MONSCR_STOPPATROL:
                flags &= 0xdfffffff;
                break;
            case MONSCR_TRIGGER:
                token = strtok(0, " \t");
                if (token != 0) {
                    this->trigger = FindTriggerByName(token);
                    if (this->trigger != 0) {
                        this->trigger->Run(-1);
                        script_wait = MONSCR_TRIGGER;
                    }
                }
                break;
            case MONSCR_DELAY:
                token = strtok(0, " \t");
                if (token != 0 && static_cast<float>(atof(token)) != 0.0f) {
                    script_delay_timer.SetDuration(static_cast<float>(atof(token)) *
                                                   g_float_one_thousandth);
                    script_delay_timer.Restart();
                    script_wait = MONSCR_DELAY;
                }
                break;
            case MONSCR_BEGINORDERS:
                defining_orders = true;
                break;
            case MONSCR_DEAF:
                deaf = true;
                break;
            case MONSCR_DOACTION:
                token = strtok(0, " \t");
                if (token != 0) {
                    if (_stricmp(token, "STARTGOLEMATTACK") == 0) {
                        ClearMainGameTargetState();
                        W8MonsterGroup* group = FindFirstMonsterByID(0x68);
                        if (group != 0)
                            SetMonsterGroupHostility(group, 1, false);
                        group = FindFirstMonsterByID(0x13e);
                        if (group != 0) {
                            SetMonsterGroupHostility(group, 1, false);
                            MonsterGroupEnterCombat(group);
                        }
                    } else if (_stricmp(token, "ENDSAVANTWALK") == 0) {
                        runtime_flags |= W8_MONSTER_PARKED;
                        Trigger* trigger = FindTriggerByName("Path3Trigger");
                        if (trigger != 0)
                            trigger->Run(-1);
                    } else if (_stricmp(token, "UNLOCKUI") == 0 ||
                               _stricmp(token, "ENDGARIWALK") == 0) {
                        ClearMainGameTargetState();
                    } else if (_stricmp(token, "ENDHOGARWALK") == 0) {
                        ClearMainGameTargetState();
                        SetScript("ClosePatrol.msf", true);
                    } else if (_stricmp(token, "ENDHOGARWALKANDPUTTOSLEEP") == 0) {
                        W8TargetSource source;
                        ClearMainGameTargetState();
                        SetScript("ClosePatrol.msf", true);
                        monster_info = MonsterGetScriptPartByLocationIndex(
                            MonsterGetIndexByLocationID(0x1c3a, MONSTER_CPP, location_id, true));
                        ResetTargetSource(&source);
                        SetMonsterCondition(monster_info->location_id, W8_CONDITION_ASLEEP, 6, 0,
                                            &source, true);
                    } else if (_stricmp(token, "ENDBELAWALK") == 0) {
                        runtime_flags |= W8_MONSTER_PARKED;
                        ClearMainGameTargetState();
                    } else if (_stricmp(token, "BELA_END_CC_WALK") == 0) {
                        W8NpcState* npc = GetNpcStateByKind(0x8d);
                        if (npc != 0)
                            QueueNpcScriptNotice(npc, 0, 6, false, 0);
                        monster_info = MonsterGetScriptPartByLocationIndex(
                            MonsterGetIndexByLocationID(0x14b3, MONSTER_CPP, location_id, true));
                        if (monster_info->control_state != W8_MONSTER_CONTROL_LURED) {
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
                if (token == 0)
                    break;
                value = atoi(token);
                if (value != 0)
                    token = strtok(0, " \t");
                if (token == 0)
                    break;
                distance = static_cast<float>(atof(token)) * g_world_scale;
                if (distance != 0.0f)
                    token = strtok(0, " \t");
                if (token == 0)
                    break;
                sound = new stSound3D(token, 0);
                if (sound != 0) {
                    srVector3T<float> position = GetPosition();
                    sound->volume = value;
                    sound->setLocation(static_cast<double>(position.x),
                                       static_cast<double>(position.y),
                                       static_cast<double>(position.z));
                    if (distance != 0.0f)
                        sound->falloff = distance;
                    if (sound->Play(false, false) == 0) {
                        sound->release();
                        sound = 0;
                    } else {
                        script_wait = MONSCR_PLAY;
                    }
                }
                break;
            }
            case MONSCR_FADEOUT:
                /* 0x004C88F1..0x004C8993 is BeginFadeOutAndRemove(NONE) inlined,
                   including the removal_state store at 0x004C8993. */
                BeginFadeOutAndRemove(W8_MONSTER_REMOVAL_NONE);
                stop = true;
                script_line = script->lines.GetCount();
                break;
            default:
                break;
            }
        } else {
            switch (command) {
            case MONSCR_FACE:
                token = strtok(0, " \t");
                if (token != 0 && _stricmp(token, "PARTY") == 0) {
                    face_party = true;
                } else if (token != 0) {
                    int direction = -1;
                    for (int index = MONSCR_EAST; index <= MONSCR_SOUTHEAST; ++index) {
                        if (_stricmp(token, g_monster_script_commands[index]) == 0) {
                            direction = index;
                            break;
                        }
                    }
                    if (direction != -1) {
                        double angle = (direction - MONSCR_EAST) * g_monster_script_direction_step;
                        order_mode = W8_MONSTER_ORDER_FACE_DIRECTION;
                        direction_x = static_cast<float>(cos(angle) * g_double_five_hundred);
                        direction_y = 0.0f;
                        direction_z = static_cast<float>(sin(angle) * g_double_five_hundred);
                    } else {
                        ShutdownWithErrorBox(FormatString(
                            "MonScript %s Line %d: Unknown direction %s", script->getName(),
                            script->GetSourceLine(script_line - 1), token));
                    }
                }
                break;
            case MONSCR_PATROL:
                /* 0x004C9463: both values are parsed first and stored together
                   (0x004C94B6/0x004C94BC) only when the second token exists. */
                token = strtok(0, " \t");
                if (token != 0) {
                    float distance = static_cast<float>(atof(token)) * g_world_scale;
                    token = strtok(0, " \t");
                    if (token != 0) {
                        patrol_variation = static_cast<float>(atof(token)) * g_world_scale;
                        patrol_distance = distance;
                        order_mode = W8_MONSTER_ORDER_PATROL;
                    }
                }
                break;
            case MONSCR_ENDORDERS:
                defining_orders = false;
                orders_finished = true;
                script_wait = MONSCR_ENDORDERS;
                break;
            case MONSCR_GUARD:
                /* 0x004C94CE: one location; the list is cleared and the point
                   added only after it resolves (0x004C9568), the mode is
                   written even if the Add fails (0x004C95D4/0x004C965E), and
                   orders_finished is left alone. No token: nothing changes. */
                token = strtok(0, " \t");
                if (token != 0) {
                    srVector3T<float> position;
                    if (!ResolveScriptPosition(token, &position)) {
                        ShutdownWithErrorBox(FormatString(
                            "MonScript %s Line %d: Unknown location %s", script->getName(),
                            script->GetSourceLine(script_line - 1), token));
                        break;
                    }
                    vector.Clear();
                    vector.Add(position);
                    order_mode = W8_MONSTER_ORDER_GUARD;
                }
                break;
            case MONSCR_POINTPATROL:
            case MONSCR_RANDOMPOINTPATROL: {
                while ((token = strtok(0, " \t")) != 0) {
                    srVector3T<float> position;
                    if (!ResolveScriptPosition(token, &position)) {
                        ShutdownWithErrorBox(FormatString(
                            "MonScript %s Line %d: Unknown location %s", script->getName(),
                            script->GetSourceLine(script_line - 1), token));
                        continue;
                    }
                    if (patrol_points == 0) {
                        vector.Clear();
                    }
                    ++patrol_points;
                    vector.Add(position);
                }
                if (patrol_points > 0) {
                    order_mode = command == MONSCR_POINTPATROL
                                     ? W8_MONSTER_ORDER_POINT_PATROL
                                     : W8_MONSTER_ORDER_RANDOM_POINT_PATROL;
                }
                break;
            }
            case MONSCR_DEAF:
                deaf = true;
                break;
            case MONSCR_TURNTOFACEPARTY:
                face_party = true;
                break;
            case MONSCR_LOOKABOUT:
                token = strtok(0, " \t");
                if (token != 0) {
                    look_frequency = atoi(token);
                    monster_info->look_time = static_cast<unsigned char>(look_frequency);
                    token = strtok(0, " \t");
                    if (token != 0) {
                        look_duration = atoi(token);
                    } else {
                        ShutdownWithErrorBox(FormatString(
                            "MonScript %s Line %d: Missing lookabout duration", script->getName(),
                            script->GetSourceLine(script_line - 1)));
                    }
                } else {
                    ShutdownWithErrorBox(
                        FormatString("MonScript %s Line %d: Missing lookabout frequency",
                                     script->getName(), script->GetSourceLine(script_line - 1)));
                }
                break;
            case MONSCR_STAYHOME:
                stay_home = true;
                break;
            default:
                break;
            }
        }

        if (++command_count > 50) {
            stop = true;
        }
    }

    if (script != 0 && script_line >= script->lines.GetCount() && script_wait == MONSCR_NONE) {
        script->release();
        script = 0;
        runtime_flags &= ~W8_MONSTER_SCRIPT_WAIT;
        script_line = 0;
        script_wait = MONSCR_NONE;
        ReleaseRendererObject(sound);
    }
}

/* A script command that requested blocking leaves its command number here.
   Each command family has one concrete completion condition; commands without
   a condition are immediately ready. */
// FUNCTION: WIZ8 0x004CA0F0
bool W8Monster::CanContinueScript()
{
    switch (script_wait) {
    case MONSCR_WALKTO:
        if (!movement_stopped) {
            return false;
        }
        break;
    case MONSCR_FACE:
        if (static_cast<float>(fabs(movement.target_yaw - movement.yaw)) >=
            g_camera_transition_epsilon) {
            return false;
        }
        break;
    case MONSCR_SAY:
        if (ShouldDeferCharacterEventForNpcScript(false)) {
            return false;
        }
        break;
    case MONSCR_CYCLE:
        if (Query(W8_MONSTER_QUERY_AT_PLAYBACK_END) == 0) {
            return false;
        }
        runtime_flags &= ~W8_MONSTER_SCRIPT_WAIT;
        return true;
    case MONSCR_NPCINTERACTION:
        if (gXStatus.fNpcDialogueMode) {
            return false;
        }
        break;
    case MONSCR_TRIGGER:
        if (trigger != 0 && (trigger->flags & W8_TRIGGER_RUNNING) != 0) {
            return false;
        }
        trigger = 0;
        return true;
    case MONSCR_DELAY:
        if (script_delay_timer.GetProgress() < g_float_one) {
            return false;
        }
        break;
    case MONSCR_PLAY:
        if (sound->IsPlaying()) {
            return false;
        }
        sound->release();
        sound = 0;
        break;
    default:
        break;
    }
    return true;
}

// FUNCTION: WIZ8 0x004CA260
bool W8Monster::SetScriptLabel(const char* label)
{
    int line;

    if (script != 0) {
        line = script->FindLabelLine(label);
        if (line >= 0) {
            script_line = line;
            return true;
        }
    }
    return false;
}

// FUNCTION: WIZ8 0x004CA290
bool W8Monster::IsPendingFinalize() const
{
    return pending_finalize;
}

// FUNCTION: WIZ8 0x004CA2A0
bool W8Monster::IsWithinWorldRange()
{
    if (sector_mesh != 0) {
        return sector_mesh->testFlag(srNode::FLAG_DISABLE) == 0;
    } else {
        double far_clip = WorldGetFarClip(GetWorld());
        srVector3T<float> position = GetPosition();
        srVector3T<float> reference = g_startup_world->GetPosition();
        srVector3T<float> delta = reference - position;

        return delta.LengthSquared() <= static_cast<float>(far_clip) * static_cast<float>(far_clip);
    }
}

/* Exercise the inexpensive elevated-origin sight query from this Monster to
   the player, and answer with the trace's own result. */
// FUNCTION: WIZ8 0x004c4810
bool W8Monster::CheckLineOfSightToPlayer()
{
    srVector3T<float> monster_position;
    srVector3T<float> player_position;

    monster_position = movement.position;
    monster_position.y += movement.height_offset;
    GetCameraPosition(&player_position);
    return g_octree->HasLineOfSight(&monster_position, &player_position, true);
}

/* The sight code keeps the engine trace's three outcomes as two independent
   flags. A clear trace sets both false, the special -1 result sets only the
   secondary flag, and every other obstructed result sets both. */
// FUNCTION: WIZ8 0x004c4870
void W8Monster::GetPlayerSightFlags(bool* primary, bool* secondary)
{
    srVector3T<float> monster_position;
    srVector3T<float> player_position;
    short result;

    monster_position = movement.position;
    monster_position.y += movement.height_offset;
    GetCameraPosition(&player_position);
    result = g_octree->TraceLineOfSight(&monster_position, &player_position, true, location_id, -1,
                                        true, 0);
    if (result == -1) {
        *secondary = true;
        *primary = false;
    } else if (result != 1) {
        *secondary = true;
        *primary = true;
    } else {
        *secondary = false;
        *primary = false;
    }
}

/* The inexpensive visibility path tests the Monster's elevated origin. The
   detailed path tests the translated animation bounds at their centre and
   corners. */
// FUNCTION: WIZ8 0x004c4920
bool W8Monster::IsVisibleToPlayer(bool use_bounds)
{
    srVector3T<float> player_position;

    GetCameraPosition(&player_position);
    if (use_bounds) {
        srVector3T<float> minimum;
        srVector3T<float> maximum;

        /* VC6 member functions can be invoked with a null this. */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-compare"
        if (this != 0) {
#pragma clang diagnostic pop
            srVector3T<float> position;

            GetAnimationBounds(&minimum, &maximum);
            position = GetPosition();
            minimum += position;
            position = GetPosition();
            maximum += position;
        }
        return HasLineOfSightToBounds(&player_position, &minimum, &maximum);
    }

    srVector3T<float> monster_position = movement.position;
    monster_position.y += movement.height_offset;
    return g_octree->HasLineOfSight(&player_position, &monster_position, true);
}

// FUNCTION: WIZ8 0x004c4a20
void W8Monster::GetPlayerToMonsterSightFlags(bool* primary, bool* secondary,
                                             const srVector3T<float>* source)
{
    srVector3T<float> monster_position;
    srVector3T<float> player_position;
    short result;

    monster_position = movement.position;
    monster_position.y += movement.height_offset;
    if (source == 0) {
        GetCameraPosition(&player_position);
    } else {
        player_position = *source;
    }
    result = g_octree->TraceLineOfSight(&player_position, &monster_position, true, -1, location_id,
                                        true, 0);
    if (result == -1) {
        *secondary = true;
        *primary = false;
    } else if (result != 1) {
        *secondary = true;
        *primary = true;
    } else {
        *secondary = false;
        *primary = false;
    }
}

// FUNCTION: WIZ8 0x004c4af0
bool W8Monster::HasLineOfSightToMonster(W8Monster* monster)
{
    srVector3T<float> from;
    srVector3T<float> to;

    from = movement.position;
    from.y += movement.height_offset;
    to = monster->movement.position;
    to.y += monster->movement.height_offset;
    return g_octree->HasLineOfSight(&from, &to, true);
}

// FUNCTION: WIZ8 0x004c4b70
void W8Monster::GetMonsterSightFlags(W8Monster* monster, bool* primary, bool* secondary)
{
    srVector3T<float> from;
    srVector3T<float> to;
    short result;

    from = movement.position;
    from.y += movement.height_offset;
    to = monster->movement.position;
    to.y += monster->movement.height_offset;
    result =
        g_octree->TraceLineOfSight(&from, &to, true, location_id, monster->location_id, true, 0);
    if (result == -1) {
        *secondary = true;
        *primary = false;
    } else if (result != 1) {
        *secondary = true;
        *primary = true;
    } else {
        *secondary = false;
        *primary = false;
    }
}

// FUNCTION: WIZ8 0x004c4c40
bool W8Monster::HasLineOfSightFromPoint(srVector3T<float> point)
{
    srVector3T<float> monster_position;

    monster_position = movement.position;
    monster_position.y += movement.height_offset;
    return g_octree->TraceLineOfSight(&point, &monster_position, true, -3, -3, true, 0) != 1;
}

// FUNCTION: WIZ8 0x004c4ca0
int W8Monster::IsFacingMonster(W8Monster* monster)
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
    bearing = NormalizeAngle(GetHeadingAngle(&from, &to));
    facing = NormalizeAngle(GetYaw());
    return fabs(bearing - facing) <= g_monster_facing_tolerance;
}

// FUNCTION: WIZ8 0x004c4d40
int W8Monster::IsFacingPlayer()
{
    float bearing;
    float facing;
    srVector3T<float> from;
    srVector3T<float> to;

    if (g_startup_world == 0) {
        srAssertFail("pPlayer", MONSTER_CPP, 3838, 0);
    }
    to = g_startup_world->GetPosition();
    from = GetPosition();
    bearing = NormalizeAngle(GetHeadingAngle(&from, &to));
    facing = NormalizeAngle(GetYaw());
    return fabs(bearing - facing) <= g_monster_facing_tolerance;
}

/* Attach or remove the party slot's target-marker triangle on a monster's
   representation: on set it hangs the slot's colored Tri*.tga object on
   attachment index `party_slot`, on clear it detaches and deletes it. The
   rep's object count tracks the live markers. */
// FUNCTION: WIZ8 0x004c4de0
void SetMonsterPartySlotMarker(int party_slot, int location_id, char on)
{
    W8MonsterRep* rep;
    W8MonsterInfo* info;
    W8Item* item;
    char path[260];

    info = MonsterGetScriptPartByLocationIndex(
        MonsterGetIndexByLocationID(0xf1b, MONSTER_CPP, location_id, true));
    rep = info->p3D->m_pRep;
    if (on == 0) {
        item = rep->objects[party_slot];
        if (item != 0) {
            item->DetachMesh(g_world);
            PListRemove(g_world->plsItems, item);
            delete item;
            rep->objects[party_slot] = 0;
            --rep->icon_count;
        }
    } else {
        if (rep->objects[party_slot] == 0) {
            sprintf(path, g_monster_bitmap_path_format,
                    g_party_target_marker_bitmaps[g_status.buffers.XChar[party_slot]
                                                      .party_order_index]);
            rep->objects[party_slot] = CreateMonsterIconItem(g_world, path, 1);
            ++rep->icon_count;
        }
    }
    info->p3D->UpdateAttachedObjects();
}

/* Start making the representation visible.  Reversing an active fade-out
   preserves its current scale by seeding the opposite timer at that progress;
   an idle monster starts from zero instead. */
// FUNCTION: WIZ8 0x004c4f80
void W8Monster::BeginFadeIn(float duration)
{
    if (fade_state <= W8_MONSTER_FADE_IDLE) {
        fade_timer.SetDuration(duration);
        fade_timer.Restart();
        if (fade_state < W8_MONSTER_FADE_IDLE) {
            W8MonsterRep* rep = m_pRep;
            fade_timer.SetProgress(rep->instance_scale);
        } else {
            W8MonsterRep* rep = m_pRep;
            rep->instance_scale = 0.0f;
            rep->apply_instance_scale = true;
        }
        fade_state = W8_MONSTER_FADE_IN;
        runtime_flags &= ~W8_MONSTER_FADED_OUT;
    }
}

// FUNCTION: WIZ8 0x004c5000
void W8Monster::BeginDelayedRemoval()
{
    runtime_flags |= W8_MONSTER_REMOVE_AFTER_FADE;
    fade_state = W8_MONSTER_FADE_DELAYED_REMOVAL;
    fade_timer.SetDuration(5.0f);
    fade_timer.Restart();
}

/* Begin the three-second disappearance transition, remove the live monster
   from the manager without destroying it, and retain the requested terminal
   state for the transition's completion. */
// FUNCTION: WIZ8 0x004c5040
void W8Monster::BeginFadeOutAndRemove(W8MonsterRemovalState state)
{
    runtime_flags |= W8_MONSTER_REMOVE_AFTER_FADE;
    if (fade_state >= W8_MONSTER_FADE_IDLE) {
        fade_timer.SetDuration(3.0f);
        fade_timer.Restart();
        W8MonsterRep* rep = m_pRep;
        if (fade_state > W8_MONSTER_FADE_IDLE) {
            fade_timer.SetProgress(1.0f - rep->instance_scale);
        } else {
            rep->instance_scale = 1.0f;
            rep->apply_instance_scale = true;
        }
        fade_state = W8_MONSTER_FADE_OUT;
    }
    RemoveMonster(MonsterGetIndexByLocationID(0x1021, MONSTER_CPP, location_id, true), false);
    removal_state = state;
}

// FUNCTION: WIZ8 0x004c5150
void W8Monster::BeginFadeOut(float duration)
{
    if (fade_state < W8_MONSTER_FADE_IDLE) {
        return;
    }
    fade_timer.SetDuration(duration);
    fade_timer.Restart();
    if (fade_state > W8_MONSTER_FADE_IDLE) {
        W8MonsterRep* rep = m_pRep;
        fade_timer.SetProgress(1.0f - rep->instance_scale);
        fade_state = W8_MONSTER_FADE_OUT;
        return;
    }
    W8MonsterRep* rep = m_pRep;
    rep->instance_scale = 1.0f;
    rep->apply_instance_scale = true;
    fade_state = W8_MONSTER_FADE_OUT;
}

// FUNCTION: WIZ8 0x004c73f0
void W8Monster::StartTalking(bool animate_mouth)
{
    if (m_pRep != 0) {
        talking = true;
        this->animate_mouth = animate_mouth;
        mouth_frame_clock = GetTickCount();
        mouth_open = 0;
        talk_state = -1;
        talk_start = GetTickCount();
        talk_duration = Random(2000) + 2000;
        m_pRep->pending_cycle = 0x18;
        m_pRep->pending_behaviour = W8_ANIMATION_PLAY_ONCE;
    }
}

// FUNCTION: WIZ8 0x004c7470
void W8Monster::StopTalking()
{
    if (m_pRep != 0) {
        srModelInstance* model;
        stTextureAnim* mouth;

        talking = false;
        model = GetCurrentModelInstance();
        if (model != 0) {
            mouth = static_cast<stModelInstance*>(model)->FindMouthTexture();
            if (mouth != 0) {
                mouth->animation_mode = W8_TEXTURE_ANIM_MANUAL;
                mouth->SetFrame(0);
            }
        }
        if (m_pRep->current_cycle != 0x15) {
            m_pRep->pending_behaviour = W8_ANIMATION_NEVER_STOP;
            m_pRep->pending_cycle = 1;
        }
    }
}

// FUNCTION: WIZ8 0x004ca340
void W8Monster::SetCycleCallback(int cycle, CycleCallback callback)
{
    cycle_callback = callback;
    callback_cycle = cycle;
}

// FUNCTION: WIZ8 0x004ca360
bool W8Monster::GetPatrolPoint(srVector3T<float>* point)
{
    srVector3T<float>* patrol_point;

    if (!orders_finished || patrol_index < 0) {
        return false;
    }
    if (vector.GetCount() == 0) {
        if (IsZeroVector(&formation) != 0) {
            srVector3T<float> position = GetPosition();
            formation = position;
        }
        vector.Add(formation);
    }

    if (order_mode == W8_MONSTER_ORDER_GUARD) {
        patrol_point = vector.GetAt(0);
    } else if (order_mode > W8_MONSTER_ORDER_PATROL &&
               order_mode < W8_MONSTER_ORDER_FACE_DIRECTION) {
        patrol_point = vector.GetAt(patrol_index);
    } else {
        return false;
    }

    *point = *patrol_point;
    return true;
}

// FUNCTION: WIZ8 0x004ca4f0
bool MonsterGetWorldAnimationBounds(W8Monster* monster, srVector3T<float>* minimum,
                                    srVector3T<float>* maximum)
{
    if (monster != 0) {
        srVector3T<float> position;

        monster->GetAnimationBounds(minimum, maximum);
        position = monster->GetPosition();
        *minimum += position;
        position = monster->GetPosition();
        *maximum += position;
        return true;
    }
    return false;
}

/* Keep only the short live-sound queue owned by the monster.  The growable
   vector's normal Add/GetAt/RemoveAt methods reproduce the original inline
   template operations; no address-shaped container wrapper is involved. */
// FUNCTION: WIZ8 0x004ca6e0
void W8Monster::TrackSoundHandle(int handle)
{
    int count;
    int index;

    if (values.GetCount() > 6) {
        while (values.GetCount() != 0) {
            SoundStop(*values.GetAt(0));
            values.RemoveAt(0);
        }
    }
    values.Add(handle);
    count = values.GetCount();
    for (index = 0; index < count; ++index) {
        if (SoundIsPlaying(*values.GetAt(index)) == 0) {
            values.RemoveAt(index);
            --count;
        }
    }
}

/* Mark the closest live member of each loaded group inside the selection
   range. Members that do not improve the current candidate are unmarked. */
// FUNCTION: WIZ8 0x004ca570
void UpdateNearestMonsterGroupMembers()
{
    srVector3T<float> player_position;
    unsigned int group_index;

    GetCameraPosition(&player_position);
    for (group_index = 0; group_index < PLLength(gXStatus.plsMonsterGroupList); ++group_index) {
        W8MonsterGroup* group = GetMonsterGroupByListIndex(group_index);

        if (group != 0) {
            double nearest_distance = 1e11;
            W8MonsterInfo* nearest = 0;
            unsigned int member_index;

            for (member_index = 0; member_index < ILLength(group->monsters); ++member_index) {
                W8MonsterInfo* member =
                    MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
                        0x1efe, MONSTER_CPP, IListGetAt(group->monsters, member_index), true));

                if (member != 0 && member->p3D != 0) {
                    srVector3T<float> position = member->p3D->GetPosition();
                    float distance = (position - player_position).Length();

                    if (distance < g_monster_group_nearest_range && distance < nearest_distance) {
                        nearest_distance = distance;
                        nearest = member;
                    } else {
                        member->p3D->nearest_to_party = false;
                    }
                }
            }
            if (nearest != 0) {
                nearest->p3D->nearest_to_party = true;
            }
        }
    }
}

// FUNCTION: WIZ8 0x004c7cb0
float W8Monster::GetDistanceToPlayer()
{
    srVector3T<float> position = GetPosition();
    return GetPointDistanceToPlayer(position);
}

// FUNCTION: WIZ8 0x004c7d50
float W8Monster::GetPointDistanceToPlayer(srVector3T<float> point)
{
    srVector3T<float> player_position;
    float distance;

    GetCameraPosition(&player_position);
    player_position.y -= g_default_world_height;
    distance = (point - player_position).Length() - movement.alternate_radius -
               g_startup_world->movement.alternate_radius;
    if (distance < g_float_zero) {
        distance = g_float_zero;
    }
    return distance;
}

// FUNCTION: WIZ8 0x004c7dd0
float W8Monster::GetDistanceToMonster(W8Monster* monster)
{
    srVector3T<float> position = GetPosition();
    srVector3T<float> other_position = monster->GetPosition();
    float distance = (position - other_position).Length() - movement.alternate_radius -
                     monster->movement.alternate_radius;

    if (distance < g_float_zero) {
        distance = g_float_zero;
    }
    return distance;
}

// FUNCTION: WIZ8 0x004c7e80
float W8Monster::GetPointDistanceToMonster(W8Monster* monster, srVector3T<float> point)
{
    srVector3T<float> position = monster->GetPosition();
    float distance = (point - position).Length() - movement.alternate_radius -
                     monster->movement.alternate_radius;

    if (distance < g_float_zero) {
        distance = g_float_zero;
    }
    return distance;
}

/* Decide whether a requested cycle can replace the current one. Monster adds
   combat, motionless, and death rules to W8GrCycle's primary-table operation;
   W8Navigator's distinct slot 3 remains inherited in the secondary table. */
// FUNCTION: WIZ8 0x004c2bf0
unsigned char W8Monster::CanEnterCycle(signed char cycle)
{
    unsigned int monster_index = MonsterGetIndexByLocationID(0x969, MONSTER_CPP, location_id, true);
    W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_index);

    if (gXStatus.fCombatMode && IsCameraTransitionActive()) {
        return 0;
    }
    if (m_pRep->animation_playing == 0) {
        if (cycle != W8_MONSTER_CYCLE_GET_HIT && cycle != W8_MONSTER_CYCLE_DIE &&
            cycle != W8_MONSTER_CYCLE_BIRTH && monster_info->fMotionless) {
            if (!g_dev_mode) {
                return 0;
            }
            srAssertFail("FALSE", MONSTER_CPP, 0x97f, 0);
            return 0;
        }
    } else {
        if (!IsCycleInterruptable(static_cast<signed char>(Query(W8_MONSTER_QUERY_CYCLE))) &&
            Query(W8_MONSTER_QUERY_CYCLE_COMPLETE) == 0) {
            return 0;
        }
        if (cycle == W8_MONSTER_CYCLE_DIE && monster_info->monster_species == 0x199 &&
            Query(W8_MONSTER_QUERY_AT_PLAYBACK_END) == 0) {
            return 0;
        }
    }
    return 1;
}

/* Report whether a cycle may be interrupted. The original diagnostic names
   this operation `Monster::CycleInterruptable`; its spelling is retained here
   because it is the only source-level name available. */
// FUNCTION: WIZ8 0x004c2cf0
bool W8Monster::IsCycleInterruptable(signed char cycle)
{
    signed char current_cycle;
    signed char pending_cycle;
    const char* pending_name;
    const char* current_name;
    const char* requested_name;

    if (IsMipeActive()) {
        return true;
    }
    if (m_pRep->animation_playing == 0) {
        current_cycle = static_cast<signed char>(Query(W8_MONSTER_QUERY_CYCLE));
        pending_cycle = m_pRep->pending_cycle;
        if (pending_cycle != -1 && CanEnterCycle(pending_cycle) != 0) {
            pending_name = g_cycle_names[pending_cycle].name;
            if (current_cycle != -1) {
                current_name = g_cycle_names[current_cycle].name;
            } else {
                current_name = "";
            }
            if (cycle != W8_MONSTER_CYCLE_NONE) {
                requested_name = g_cycle_names[cycle].name;
            } else {
                requested_name = "";
            }
            FormatDebugMessage(1,
                               "Monster::CycleInterruptable (ID %d) - WARNING: Monster is "
                               "not animating - Cycle %d(%s), current %d(%s), pending "
                               "%d(%s)",
                               location_id, static_cast<int>(cycle), requested_name,
                               static_cast<int>(current_cycle), current_name,
                               static_cast<int>(pending_cycle), pending_name);
        }
        return true;
    }

    switch (cycle) {
    case W8_MONSTER_CYCLE_NONE:
    case W8_MONSTER_CYCLE_IDLE:
    case W8_MONSTER_CYCLE_SPICE:
    case W8_MONSTER_CYCLE_TRANSITION:
    case W8_MONSTER_CYCLE_WALK:
    case W8_MONSTER_CYCLE_TURN:
        return true;
    }
    return false;
}

/* Apply the two script-specific side effects selected before an ungrouped
   monster is removed: state two returns it to Balbrak's home marker, while
   state three clears the ScregActive trigger variable. */
// FUNCTION: WIZ8 0x004c50f0
void W8Monster::ApplyRemovalStateEffects()
{
    srVector3T<float> position;

    switch (removal_state) {
    case W8_MONSTER_REMOVAL_RETURN_BALBRAK_HOME:
        if (FindEntityByName("NP_Balbrakhome", &position, 0, 0)) {
            SetPosition(&position);
        }
        break;
    case W8_MONSTER_REMOVAL_CLEAR_SCREG_ACTIVE:
        SetTriggerVariableByName("ScregActive", 0);
        break;
    }
}

// FUNCTION: WIZ8 0x004bfab0
unsigned char W8MonsterRep::GetNumSubsPerCycle(signed char bCycle)
{
    if (bCycle >= W8_MONSTER_CYCLE_COUNT) {
        srAssertFail("bCycle < CYCLE_NUM_UNIQUE", MONSTER_CPP, 0x3c0,
                     "GetNumSubsPerCycle() -> Invalid cycle num.");
    }
    if (bCycle == -1) {
        bCycle = current_cycle;
    }
    return static_cast<unsigned char>(animations[bCycle].GetCount());
}

/* Select the active AnimObj for a cycle and dispatch the requested LOD/frame.
   This is the concrete implementation behind AnimRep's third vtable slot. */
// FUNCTION: WIZ8 0x004bf8c0
srModelInstance* W8MonsterRep::SetCycleFrameLod(signed char cycle, signed char frame,
                                                signed char lod)
{
    int subcycle = current_subcycle;
    W8GrowableVector<W8AnimObj*>* selected_cycle = &animations[cycle];
    W8AnimObj** animation_slot;
    W8AnimObj* animation;

    animation_slot = selected_cycle->GetAt(subcycle);
    animation = *animation_slot;
    if (animation->path_lists == 0) {
        return AnimObjDispatch(animation, lod, frame);
    }
    return AnimObjDispatchList(animation, lod, 0);
}

/* The selected subcycle's AniMesh for one animation cycle. */
// FUNCTION: WIZ8 0x004bf920
W8AniMesh* W8MonsterRep::GetEmitterAniMesh(signed char cycle)
{
    W8AnimObj* animation = *animations[cycle].GetAt(current_subcycle);

    if (animation == 0) {
        return 0;
    }
    return AnimObjEntry(animation, m_bLOD, 0);
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
    W8GrowableVector<stLight*>* lights;
    int index;
    int count;

    position = movement.position;
    position.y += movement.vertical_offset;
    GetRepresentation()->SetLocation(&position);

    rotation.SetIdentity();
    {
        float angle = NormalizeAngle(GetYaw() + g_monster_rotation_offset);
        if (angle != 0.0f) {
            rotation.RotateAboutY(sin(angle), cos(angle));
        }
    }
    {
        float angle = GetPitch();
        if (angle != g_float_zero) {
            rotation.RotateAboutX(static_cast<double>(angle));
        }
    }
    if (movement.roll != g_float_zero) {
        rotation.RotateAboutZ(static_cast<double>(movement.roll));
    }
    m_pRep->SetRotation(&rotation);

    if ((runtime_flags & W8_MONSTER_SCALING_Y) != 0) {
        model = GetCurrentModelInstance();
        srVector3T<double> source_scale = model->getScale();
        float scale_y = static_cast<float>(source_scale.y) * this->scale_y;
        float scale_z = static_cast<float>(source_scale.z);
        srVector3T<double> scale;
        scale.x = source_scale.x;
        scale.y = static_cast<double>(scale_y);
        scale.z = static_cast<double>(scale_z);
        model->setScale(scale);
        this->scale_y -= g_float_one_tenth;
    }

    if (talking && animate_mouth) {
        if (mouth_open != 0) {
            model = GetCurrentModelInstance();
            if (model != 0 &&
                (mouth = static_cast<stModelInstance*>(model)->FindMouthTexture()) != 0) {
                mouth->animation_mode = W8_TEXTURE_ANIM_MANUAL;
                mouth->SetFrame(0);
            }
        } else if (GetTickCount() - mouth_frame_clock > 120) {
            unsigned short frame;
            mouth_frame_clock = GetTickCount();
            do {
                frame = static_cast<unsigned short>(Random(6));
                if (frame > 3) {
                    frame = 0;
                }
            } while (frame == static_cast<unsigned short>(mouth_frame));
            mouth_frame = frame;
            model = GetCurrentModelInstance();
            if (model != 0 &&
                (mouth = static_cast<stModelInstance*>(model)->FindMouthTexture()) != 0) {
                mouth->animation_mode = W8_TEXTURE_ANIM_MANUAL;
                mouth->SetFrame(frame);
            }
        }
    }

    if (position_dirty || movement.position_adjusted) {
        g_octree->UpdateMonsterLocation(static_cast<unsigned short>(location_id), &position);
    }

    /* Only a monster whose current world sector mesh is enabled (or that has
       none yet) is re-attached; this is the octree sector cache at 0x308, not
       the navigator's scene node. */
    if ((sector_mesh == 0 || sector_mesh->testFlag(srNode::FLAG_DISABLE) == 0) &&
        IsRenderable(false)) {
        W8GrCycle::UpdateRepresentation(world);
        model = GetCurrentModelInstance();
        if (model != 0) {
            static_cast<stModelInstance*>(model)->frame_interpolation =
                g_settings.smooth_monster_animations != 0 ? frame_fraction : 0.0f;
            SetModelInstanceChainExclusionMask(model, 4);
        }
        if (m_pRep->monster_light != 0) {
            m_pRep->monster_light->Update(&position);
        }
        if (!enabled) {
            enabled = true;
            SetShakeEventVisibility(m_pRep->current_cycle);
            lights = *m_pRep->light_lists[m_pRep->current_cycle].GetAt(m_pRep->current_subcycle);
            if (lights != 0 && (count = lights->GetCount()) != 0) {
                for (index = 0; index < count; ++index) {
                    (*lights->GetAt(index))->clearFlag(srNode::FLAG_DISABLE);
                }
            }
            if (m_pRep->monster_light != 0) {
                m_pRep->monster_light->SetVisible(true);
            }
        }
    } else if (enabled) {
        enabled = false;
        SetShakeEventVisibility(m_pRep->current_cycle);
        lights = *m_pRep->light_lists[m_pRep->current_cycle].GetAt(m_pRep->current_subcycle);
        if (lights != 0 && (count = lights->GetCount()) != 0) {
            for (index = 0; index < count; ++index) {
                (*lights->GetAt(index))->setFlag(srNode::FLAG_DISABLE);
            }
        }
        if (m_pRep->monster_light != 0) {
            m_pRep->monster_light->SetVisible(false);
        }
    }
}

/* Enable only the shake particles belonging to the requested cycle and the
   currently selected subcycle. A ranged particle is left to the frame-driven
   update path; this method only toggles particles without a distinct range. */
// FUNCTION: WIZ8 0x004bf9e0
void W8Monster::SetShakeEventVisibility(signed char cycle)
{
    int index;
    int count;

    if (m_plsParticles == 0 || (count = m_plsParticles->GetCount()) == 0) {
        return;
    }

    for (index = 0; index < count; ++index) {
        W8GrCycleParticleAttachment* event = *m_plsParticles->GetAt(index);

        if (event->cycle == cycle && event->subcycle == m_pRep->current_subcycle && enabled) {
            W8AnimObj* animation =
                *m_pRep->animations[m_pRep->current_cycle].GetAt(m_pRep->current_subcycle);

            if (animation == 0 || animation->start_frame == 0 ||
                animation->end_frame < animation->start_frame) {
                stParticle* particle = event->m_pstParticles;
                if (particle->start_frame != -1 && particle->end_frame != -1 &&
                    particle->start_frame != particle->end_frame) {
                    continue;
                }
                particle->SetActive(1);
            }
        } else {
            event->m_pstParticles->SetActive(0);
        }
    }
}

/* The Monster vtable's slot-three method selects the active subcycle's
   AnimObj (falling back to entry zero) and submits the Monster's current
   animation index.  The assertion's `pao` spelling establishes the pointee's
   AnimObj identity without supplying a name for this Monster method. */
// FUNCTION: WIZ8 0x004bf970
unsigned int W8MonsterRep::ApplyEmitterSetting(signed char cycle)
{
    W8GrowableVector<W8AnimObj*>* selected_cycle = &animations[cycle];
    W8AnimObj** animation_slot;
    W8AnimObj* animation;

    animation_slot = selected_cycle->GetAt(current_subcycle);
    animation = *animation_slot;
    if (animation == 0) {
        srAssertFail("pao", "C:\\Projects\\Wizardry 8\\Engine Code\\Monster.cpp", 0x2de, 0);
    }
    return AnimObjValue(animation, m_bLOD);
}

// FUNCTION: WIZ8 0x004caa40
signed char W8Monster::GetNumSubCycles()
{
    W8MonsterRep* representation = m_pRep;
    W8GrowableVector<W8AnimObj*>* cycle =
        &representation->animations[representation->current_cycle];
    int subcycle = representation->current_subcycle;
    W8AnimObj** slot = cycle->GetAt(subcycle);

    return static_cast<signed char>(AnimObjValue(*slot, representation->m_bLOD));
}

/* W8Monster stores its animation object immediately after the shared
   0x1d8-byte GrCycle base. */
// FUNCTION: WIZ8 0x004c3740
bool W8Monster::IsCycleSupported(signed char cycle)
{
    if (cycle >= W8_MONSTER_CYCLE_COUNT) {
        srAssertFail("bCycle < CYCLE_NUM_UNIQUE",
                     "C:\\Projects\\Wizardry 8\\Engine Code\\Monster.cpp", 0xafc,
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
    W8GrowableVector<W8AnimObj*>* animations;
    W8GrowableVector<stLight*>* lights;
    W8AnimObj* animation;
    signed char subcycle;
    int count;
    int index;

    if (cycle < W8_MONSTER_CYCLE_BIRTH || cycle >= W8_MONSTER_CYCLE_COUNT) {
        srAssertFail("bCycle >= CYCLE_FIRST && bCycle <= CYCLE_LAST", MONSTER_CPP, 0xb14, 0);
    }

    animations = &m_pRep->animations[cycle];
    count = animations->GetCount();
    if (count == 0) {
        if (m_pRep->animations[1].GetCount() < 1) {
            W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(
                MonsterGetIndexByLocationID(0xb26, MONSTER_CPP, location_id, true));
            srAssertFail("FALSE", MONSTER_CPP, 0xb26,
                         FormatString("ERROR: Monster %ls has no IDLE cycle!",
                                      GetMonsterName(monster_info, 0, 0)));
            return;
        }

        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(
            MonsterGetIndexByLocationID(0xb1d, MONSTER_CPP, location_id, true));
        FormatDebugMessage(0, "WARNING: Monster %ls is missing anim cycle %s",
                           GetMonsterName(monster_info, 0, 0), g_cycle_names[cycle].name);
        m_pRep->CopyCycle(cycle, m_pRep, 1);
        count = animations->GetCount();
        if (count == 0) {
            return;
        }
    }

    if (count == 1) {
        subcycle = 0;
    } else if (count < 2) {
        srAssertFail("0", MONSTER_CPP, 0xb43, 0);
        subcycle = 0;
    } else if (m_pRep->forced_subcycle == -1 || count <= m_pRep->forced_subcycle) {
        if ((runtime_flags & W8_MONSTER_KEEP_SUBCYCLE) == 0) {
            subcycle = static_cast<signed char>(GetTickCount() % count);
        } else {
            runtime_flags &= ~W8_MONSTER_KEEP_SUBCYCLE;
            subcycle = m_pRep->current_subcycle;
        }
    } else {
        runtime_flags &= ~W8_MONSTER_KEEP_SUBCYCLE;
        subcycle = m_pRep->forced_subcycle;
        m_pRep->forced_subcycle = -1;
    }

    if (cycle_callback != 0 && callback_cycle == m_pRep->current_cycle) {
        cycle_callback(this);
        cycle_callback = 0;
    }

    if (m_pRep->current_cycle != 0) {
        lights = *m_pRep->light_lists[m_pRep->current_cycle].GetAt(m_pRep->current_subcycle);
        if (lights != 0) {
            count = lights->GetCount();
            for (index = 0; index < count; ++index) {
                stLight* light = *lights->GetAt(index);

                light->setParent(0, 1);
                if (light->definition() != 0) {
                    g_world->lights_to_update->Remove(light);
                }
            }
        }
    }

    m_pRep->current_cycle = cycle;
    m_pRep->current_subcycle = subcycle;
    animation = *animations->GetAt(subcycle);
    if (animation == 0) {
        srAssertFail("pao", MONSTER_CPP, 0xb51, 0);
    }
    m_pRep->frame_method = animation->frame_method;

    {
        srVector3T<double> camera_location = g_world->camera->getLocation();
        srVector3T<float> listener;
        listener = camera_location;
        SelectLOD(&listener);
    }

    GetAnimationRadius(&m_pRep->animation_radius);
    if ((runtime_flags & W8_MONSTER_KEEP_FRAME_DIRECTION) != 0) {
        runtime_flags &= ~W8_MONSTER_KEEP_FRAME_DIRECTION;
    } else {
        m_pRep->frame_direction = W8_ANIMATION_FORWARD;
    }

    lights = *m_pRep->light_lists[cycle].GetAt(subcycle);
    SetLights(lights);
    if (lights != 0) {
        count = lights->GetCount();
        for (index = 0; index < count; ++index) {
            stLight* light = *lights->GetAt(index);

            light->setParent(g_world->dynamic_scene, 1);
            light->Reset();
            if (light->definition() != 0) {
                g_world->lights_to_update->Add(light);
            }
        }
    }

    runtime_flags &= ~W8_MONSTER_TEXTURE_CHECKED;
    m_pRep->first_frame = 0;
    m_pRep->last_frame = GetNumSubCycles() - 1;
    SetShakeEventVisibility(cycle);

    if (lights != 0) {
        count = lights->GetCount();
        for (index = 0; index < count; ++index) {
            if (enabled) {
                (*lights->GetAt(index))->clearFlag(srNode::FLAG_DISABLE);
            } else {
                (*lights->GetAt(index))->setFlag(srNode::FLAG_DISABLE);
            }
        }
    }
    if (m_pRep->monster_light != 0) {
        m_pRep->monster_light->SetVisible(enabled);
    }

    g_monster_model_instances.Clear();
    CollectModelInstances(&g_monster_model_instances);
    for (index = 0; index < g_monster_model_instances.GetCount(); ++index) {
        stModelInstance* model = *g_monster_model_instances.GetAt(index);
        model->light_scale = current_scale;
    }

    if (cycle == W8_MONSTER_CYCLE_DIE) {
        srVector4T<float> empty;
        empty = 0.0f;
        srModelInstance* instance;

        m_pRep->highlight_colour = empty;
        instance = SelectCycleFrameLod(m_pRep->current_cycle, 0, m_pRep->m_bLOD);
        if (instance != 0 && instance->getModel() != 0 &&
            strstr(instance->getModel()->getName(), "gib") != 0) {
            SetAngles(static_cast<float>(g_monster_death_rotation_pi *
                                         g_float_inverse_half_turn_degrees * Random(0x168)));
        }
        if (m_pRep->monster_light != 0) {
            m_pRep->monster_light->StartFadeOut();
        }
    }
}

// FUNCTION: WIZ8 0x004c3dd0
signed char W8Monster::GetTotalAnimationCount()
{
    signed char total = 0;
    int cycle;

    for (cycle = W8_MONSTER_CYCLE_BIRTH; cycle < W8_MONSTER_CYCLE_COUNT; ++cycle) {
        total += static_cast<signed char>(m_pRep->animations[cycle].GetCount());
    }
    return total;
}

// FUNCTION: WIZ8 0x004caa90
float W8Monster::GetCurrentAnimationScale()
{
    W8MonsterRep* representation = m_pRep;

    return *representation->animation_scales[representation->current_cycle].GetAt(
        representation->current_subcycle);
}

// FUNCTION: WIZ8 0x004cab00
W8EmitterHost* W8Monster::GetRepresentation()
{
    return m_pRep;
}

// FUNCTION: WIZ8 0x004c3df0
unsigned char W8Monster::GetAnimationBounds(srVector3T<float>* arg_minimum,
                                            srVector3T<float>* arg_maximum)
{
    unsigned char result;
    float scale;

    result = W8GrCycle::GetAnimationBounds(arg_minimum, arg_maximum);
    scale = m_pRep->scale;
    *arg_minimum *= scale;
    scale = m_pRep->scale;
    *arg_maximum *= scale;
    return result;
}

// FUNCTION: WIZ8 0x004c3ed0
unsigned char W8Monster::GetAnimationRadius(float* arg_radius)
{
    unsigned char result = W8GrCycle::GetAnimationRadius(arg_radius);

    *arg_radius *= m_pRep->scale;
    return result;
}

static const float g_monster_bounds_vertical_factor = 0.66f;

// FUNCTION: WIZ8 0x004c3e60
bool W8Monster::GetAnimationCenter(srVector3T<float>* center)
{
    srVector3T<float> minimum;
    srVector3T<float> maximum;

    if (GetAnimationBounds(&minimum, &maximum) != 0) {
        srVector3T<float> position = GetPosition();

        *center = position;
        center->y += (maximum.y - minimum.y) * g_monster_bounds_vertical_factor;
        return true;
    }
    return false;
}

/* Keep equipped items, spell icons, and temporary poster model instances in
   the camera-facing attachment layout selected by the representation. */
// FUNCTION: WIZ8 0x004c3f70
void W8Monster::UpdateAttachedObjects()
{
    W8MonsterRep* representation = m_pRep;
    int attachment_layout = representation->icon_count;
    unsigned int linked_count = PLLength(representation->spell_icons);
    int poster_count = representation->linked_runtime_objects.GetCount();
    srMatrix3T<float> camera_rotation;
    srVector3T<float> base_position;
    srVector3T<float> party_position;
    float distance_scale;
    unsigned int elapsed;
    int index;

    if (attachment_layout == 0 && linked_count == 0 && poster_count == 0) {
        return;
    }

    elapsed = g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT) - representation->timer;
    WorldGetCameraRotation(g_world, &camera_rotation);
    base_position = movement.position;
    base_position.y += movement.vertical_base + movement.vertical_amplitude;

    GetCameraPosition(&party_position);
    party_position -= base_position;
    distance_scale = party_position.Length() * g_monster_attachment_distance_scale;
    if (!target_highlighted && distance_scale > g_float_one) {
        distance_scale = g_float_one;
    }

    if (attachment_layout != 0) {
        /* 0x004C40C4 indexes the layout row with a count of the markers
           already placed (0x28(%esp), bumped at 0x004C4243 only for a present
           slot), so the live markers pack into the first row entries
           whatever party slots they belong to. */
        int placed = 0;

        for (index = 0; index < 8; ++index) {
            W8Item* item = representation->objects[index];

            if (item != 0) {
                const W8AttachmentOffset& raw =
                    g_monster_attachment_offsets[attachment_layout - 1][placed];
                srVector3T<float> source(raw.x, raw.y, raw.z);
                srVector3T<float> offset;
                srVector3T<float> location;
                srNode* mesh;
                float mesh_scale;
                srVector3T<double> widened_scale;

                offset = source * distance_scale;
                srVector3T<float> rotated = camera_rotation.Transform(offset);
                location = base_position + rotated;
                location.y += representation->standing_height +
                              distance_scale * g_monster_attachment_vertical_scale;

                item->SetLocation(&location);
                mesh = item->GetMesh();
                mesh_scale = distance_scale * g_monster_attachment_scales[attachment_layout - 1];
                widened_scale = mesh_scale;
                mesh->setScale(widened_scale);
                if ((runtime_flags & W8_MONSTER_FADED_OUT) == 0) {
                    mesh->clearFlag(srNode::FLAG_DISABLE);
                } else {
                    mesh->setFlag(srNode::FLAG_DISABLE);
                }
                item->ApplyRepTransform();
                ++placed;
            }
        }
    }

    index = 0;
    {
        int group = 0;

        while (index < static_cast<int>(linked_count)) {
            int chunk_count = static_cast<int>(linked_count) - index;
            int chunk_index;
            float group_height;

            if (chunk_count > 4) {
                chunk_count = 4;
            }
            group_height = static_cast<float>(group * g_monster_attachment_group_spacing);

            for (chunk_index = 0; chunk_index < chunk_count; ++chunk_index, ++index) {
                W8MonsterSpellIcon* entry =
                    static_cast<W8MonsterSpellIcon*>(PLGet(representation->spell_icons, index));
                W8Item* item = entry->psrBMO;
                const W8AttachmentOffset& raw =
                    g_monster_attachment_offsets[chunk_count - 1][chunk_index];
                srVector3T<float> source(raw.x, raw.y, raw.z);
                srVector3T<float> offset;
                srVector3T<float> location;
                srNode* mesh;
                float mesh_scale;
                srVector3T<double> widened;

                offset = source * distance_scale;
                offset.y += group_height * distance_scale;
                srVector3T<float> rotated = camera_rotation.Transform(offset);
                location = base_position + rotated;
                location.y += representation->standing_height +
                              distance_scale * g_monster_linked_vertical_scale;

                item->SetLocation(&location);
                mesh = item->GetMesh();
                mesh_scale = distance_scale * g_monster_attachment_scales[chunk_count - 1];
                widened = mesh_scale;
                mesh->setScale(widened);
                widened.SetFromFloat(&location);
                mesh->setLocation(widened);
                if ((runtime_flags & W8_MONSTER_FADED_OUT) == 0) {
                    mesh->clearFlag(srNode::FLAG_DISABLE);
                } else {
                    mesh->setFlag(srNode::FLAG_DISABLE);
                }
            }
            ++group;
        }
    }

    if (poster_count != 0) {
        srVector3T<float> mapped_position;
        float vertical_offset = elapsed * g_monster_poster_vertical_rate;
        int poster_index = 0;

        GetMappedPosition(&mapped_position);
        while (poster_index < poster_count) {
            stModelInstance* poster = *representation->linked_runtime_objects.GetAt(poster_index);
            srVector3T<double> location = poster->getLocation();
            srVector3T<float> poster_position(static_cast<float>(location.x),
                                              static_cast<float>(location.y) + vertical_offset,
                                              static_cast<float>(location.z));

            if ((poster_position - mapped_position).Length() <=
                static_cast<float>(g_monster_poster_max_distance)) {
                location.SetFromFloat(&poster_position);
                poster->setLocation(location);
                ++poster_index;
            } else {
                representation->linked_runtime_objects.Remove(poster);
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
int W8Monster::Query(W8MonsterQueryKind query)
{
    int result = -1;
    unsigned int animation_value;

    switch (query) {
    case W8_MONSTER_QUERY_FRAME_COUNT:
        result = m_pRep->ApplyEmitterSetting(m_pRep->current_cycle);
        break;
    case W8_MONSTER_QUERY_ANIMATION_COUNT:
        result = GetTotalAnimationCount();
        break;
    case W8_MONSTER_QUERY_AT_PLAYBACK_END:
        if (m_pRep->frame_direction != W8_ANIMATION_FORWARD &&
            m_pRep->frame_direction != W8_ANIMATION_FORWARD_COMPLETE) {
            result = m_pRep->subcycle == 0;
            break;
        }
        animation_value = m_pRep->ApplyEmitterSetting(m_pRep->current_cycle);
        result = m_pRep->subcycle == animation_value - 1;
        break;
    case W8_MONSTER_QUERY_AT_PLAYBACK_START:
        if (m_pRep->frame_direction == W8_ANIMATION_FORWARD ||
            m_pRep->frame_direction == W8_ANIMATION_FORWARD_COMPLETE) {
            result = m_pRep->subcycle == 0;
            break;
        }
        animation_value = m_pRep->ApplyEmitterSetting(m_pRep->current_cycle);
        result = m_pRep->subcycle == animation_value - 1;
        break;
    case W8_MONSTER_QUERY_FRAME:
        result = m_pRep->subcycle;
        break;
    case W8_MONSTER_QUERY_HAS_FRAMES:
        result =
            m_pRep->ApplyEmitterSetting(m_pRep->current_cycle) != static_cast<unsigned int>(-1);
        break;
    case W8_MONSTER_QUERY_CYCLE:
        result = m_pRep->current_cycle;
        break;
    case W8_MONSTER_QUERY_SUBCYCLE:
        result = m_pRep->current_subcycle;
        break;
    case W8_MONSTER_QUERY_CYCLE_COMPLETE:
        result = 0;
        if (m_pRep->animation_behaviour == W8_ANIMATION_NEVER_STOP) {
            if (m_pRep->animation_playing == 0) {
                result = 1;
            }
            break;
        }

        animation_value = m_pRep->ApplyEmitterSetting(m_pRep->current_cycle);
        if ((m_pRep->frame_direction == W8_ANIMATION_FORWARD &&
             m_pRep->subcycle == animation_value - 1) ||
            (m_pRep->frame_direction == W8_ANIMATION_REVERSE && m_pRep->subcycle == 0) ||
            m_pRep->frame_direction == W8_ANIMATION_REVERSE_COMPLETE ||
            m_pRep->frame_direction == W8_ANIMATION_FORWARD_COMPLETE) {
            result = 1;
        }
        break;
    default:
        break;
    }
    return result;
}

// FUNCTION: WIZ8 0x004c32e0
void W8Monster::AdvanceAnimationFrame(int value, int)
{
    unsigned char previous_frame = m_pRep->subcycle;

    W8GrCycle::AdvanceAnimationFrame(value, 0);
    if ((m_pRep->current_cycle == 7 || m_pRep->current_cycle == 13 ||
         m_pRep->current_cycle == 17) &&
        ((missile_frame > 0 && previous_frame < missile_frame &&
          missile_frame <= m_pRep->subcycle) ||
         (missile_frame == 0 && m_pRep->subcycle == 1))) {
        HandleAnimationThreshold();
    }
    HandleAnimationFrame(previous_frame);
    if (m_plsParticles != 0 && m_plsParticles->GetCount() != 0) {
        UpdateShakeEvents(previous_frame);
    }
}

// GLOBAL: WIZ8 0x0064c158
static int g_spell_effect_frame = 1;
// GLOBAL: WIZ8 0x0069b7dc
static int g_spell_index;

// VTABLE: WIZ8 0x005ed288
// class W8MonsterShakeCallback
// VTABLE: WIZ8 0x005ed290
// class W8MonsterShakeCallbackBase
/* Cycle 25 launches either the queued spell visual or the monster's pending
   spell action when its animation crosses the configured frame. The cast's
   return is the stamina charge passed directly to FatigueMonster, establishing
   MonsterCastsSpell's non-void return. */
// FUNCTION: WIZ8 0x004c74d0
void W8Monster::HandleAnimationFrame(unsigned char previous_frame)
{
    W8MonsterInfo* monster_info;
    W8MonsterActionKind action_kind;
    int action_detail;
    unsigned int power_level;
    unsigned int fatigue;

    if (m_pRep->current_cycle == 25 &&
        ((spell_frame > 0 && previous_frame < spell_frame && spell_frame <= m_pRep->subcycle) ||
         (spell_frame == 0 && m_pRep->subcycle == 1))) {
        if (spell_effect_armed) {
            spell_effect_armed = false;
            CreateAttachedSpellEffect(g_spell_records[g_spell_index].resource_name,
                                      g_spell_effect_frame, this, 0, 0);
            return;
        }

        monster_info = MonsterGetScriptPartByLocationIndex(
            MonsterGetIndexByLocationID(0x1804, MONSTER_CPP, location_id, true));
        action_kind = monster_info->action_kind;
        action_detail = monster_info->action_detail;
        power_level = monster_info->spell_power_level;
        if (action_kind == W8_MONSTER_ACTION_SPELL && action_detail != 0 && power_level != 0) {
            fatigue = MonsterCastsSpell(monster_info, action_detail, power_level);
            FatigueMonster(monster_info, fatigue, 0);
            monster_info->fSpellReleased = true;
        }
    }
}

/* Launch the missile at the frame shared by attack cycles 7, 13 and 17. In
   combat an already-selected attack is reused; otherwise the monster picks a
   live character and the first database attack that permits a missile mode. */
// FUNCTION: WIZ8 0x004c75c0
void W8Monster::HandleAnimationThreshold()
{
    W8MonsterInfo* monster_info;
    W8MonsterRecord* record;
    W8TargetSource source;
    W8SpellEffectDefinition attack_block;
    W8MonsterAttack* attack;
    unsigned int attack_index;
    W8RangeCategory range_category;
    int missile_type;
    int accuracy;
    bool selected_attack;
    unsigned char monster_value;

    selected_attack = false;
    monster_info = MonsterGetScriptPartByLocationIndex(
        MonsterGetIndexByLocationID(0x1834, MONSTER_CPP, location_id, true));
    record = GetMonsterDataForInfo(monster_info);
    SetTargetSourceToMonster(monster_info, &source);

    if (gXStatus.fCombatMode && g_combat_state->eCombatActionStatus == 2 &&
        g_combat_state->pActionMonsterInfo == monster_info) {
        attack_index = monster_info->pCombat->attack_index;
        selected_attack = true;
        goto prepare_attack;
    }

    monster_info->Target.iType = W8_TARGET_KIND_CHARACTER;
    monster_info->Target.iChar = GetRandomCharacter(1, 1, -1, -1);
    monster_info->Target.iMonsterID = -1;
    if (monster_info->Target.iChar == -1) {
        return;
    }

    for (attack_index = 0; attack_index < W8_MAX_MONSTER_ATTACKS; ++attack_index) {
        attack = &record->attacks[attack_index];
        if (attack->fHasAttack != 0 && (attack->attack_modes & 0x110) != 0) {
            monster_info->action_detail = ChooseAttackMode(attack->attack_modes);
            goto prepare_attack;
        }
    }

    missile_type = 0;
    range_category = W8_RANGE_EXTREME;
    ClearAttackBlock(&attack_block);
    accuracy = 50;
    goto fire_missile;

prepare_attack:
    if (attack_index >= W8_MAX_MONSTER_ATTACKS) {
        srAssertFail("uiAttack < MAX_MONSTER_ATTACKS", MONSTER_CPP, 0x1862, 0);
    }
    attack = &record->attacks[attack_index];
    missile_type = attack->missile_type;
    if (static_cast<unsigned int>(missile_type) >= g_missile_table_count) {
        FormatDebugMessage(0, "WARNING: %ls has invalid missile type %d for attack %d", record,
                           missile_type, attack_index);
        missile_type = 0;
    }

    range_category = static_cast<W8RangeCategory>(attack->range_category);
    ClearAttackBlock(&attack_block);
    attack_block.magnitude = attack->damage_dice;
    memcpy(attack_block.condition_chances, attack->missile_values, 0x10);
    monster_value = record->effective_level;
    attack_block.power_level = monster_value + (monster_value < 15 ? monster_value : 15);
    attack_block.magnitude_base = attack->missile_magnitude;

    if (selected_attack) {
        accuracy = GetMonsterAttackScore(monster_info, attack,
                                         static_cast<W8AttackMode>(monster_info->action_detail), 0);
        CombatLog("TO HIT (MISSILE ACCURACY): Chance %d", accuracy);
    } else {
        accuracy = 50;
    }

fire_missile:
    monster_info->fMissileReleased = true;
    FireMissileSourceToTarget(missile_type, &source, &monster_info->Target, &attack_block,
                              !selected_attack, range_category, accuracy);
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

    m_pParticles->callback = 0;
    m_pParticles->SetActive(0);
    representation = m_pMonster->m_pRep;
    if (saved_behaviour < 1 || saved_behaviour > 3) {
        srAssertFail("bBehaviour >= BEHAVIOUR_FIRST && bBehaviour <= BEHAVIOUR_LAST",
                     "..\\Engine Code\\Include\\AnimRep.hpp", 0x87, 0);
    }
    representation->pending_behaviour = saved_behaviour;
    representation->SetFrameMethod(saved_frame_method);
    representation->frame_direction = W8_ANIMATION_FORWARD;
    representation->first_frame = 0;
    representation->last_frame = m_pMonster->GetNumSubCycles() - 1;
    delete this;
}

/* Drive the particles attached to the active cycle/subcycle. A particle with
   no distinct frame range fires when the animation crosses its own start
   frame; a ranged particle is switched on and off at its explicit bounds. */
// FUNCTION: WIZ8 0x004c3380
void W8Monster::UpdateShakeEvents(unsigned char previous_frame)
{
    W8AnimObj* animation;
    W8GrCycleParticleAttachment* event;
    stParticle* particle;
    W8MonsterShakeCallback* callback;
    int count;
    int index;
    bool animation_has_range;

    count = m_plsParticles->GetCount();
    animation = *m_pRep->animations[m_pRep->current_cycle].GetAt(m_pRep->current_subcycle);
    animation_has_range = animation != 0 && animation->start_frame != 0 &&
                          animation->end_frame >= animation->start_frame;

    for (index = 0; index < count; ++index) {
        event = *m_plsParticles->GetAt(index);
        if (event->cycle != m_pRep->current_cycle || event->subcycle != m_pRep->current_subcycle) {
            continue;
        }

        particle = event->m_pstParticles;
        if (animation_has_range &&
            (particle->start_frame == -1 || particle->end_frame == -1 ||
             particle->start_frame == particle->end_frame) &&
            previous_frame < animation->start_frame && animation->start_frame <= m_pRep->subcycle) {
            if (enabled) {
                particle->SetActive(1);
                particle->emission_count = 0;
                if (index == 0) {
                    callback = new W8MonsterShakeCallback;
                    callback->m_pMonster = this;
                    callback->m_pParticles = particle;
                    callback->saved_behaviour = m_pRep->animation_behaviour;
                    callback->saved_frame_method = m_pRep->frame_method;
                    particle->callback = callback;

                    m_pRep->pending_behaviour = W8_ANIMATION_NEVER_STOP;
                    if (animation->start_frame == animation->end_frame) {
                        m_pRep->SetFrameMethod(4);
                        m_pRep->frame_direction = W8_ANIMATION_FORWARD;
                    } else {
                        m_pRep->SetFrameMethod(W8_ANIMATION_PING_PONG);
                        m_pRep->first_frame = animation->start_frame;
                        if (animation->end_frame < GetNumSubCycles()) {
                            m_pRep->last_frame = animation->end_frame;
                        } else {
                            m_pRep->last_frame = GetNumSubCycles() - 1;
                        }
                    }
                }
            }
            continue;
        }

        if (particle->start_frame != -1 && particle->end_frame != -1 &&
            particle->start_frame != particle->end_frame) {
            if (static_cast<unsigned int>(previous_frame) ==
                static_cast<unsigned int>(particle->start_frame)) {
                if (enabled) {
                    particle->SetActive(1);
                    particle->emission_count = 0;
                }
            } else if (static_cast<unsigned int>(m_pRep->subcycle) ==
                       static_cast<unsigned int>(particle->end_frame)) {
                particle->SetActive(0);
            }
        }
    }
}

// FUNCTION: WIZ8 0x004cab10
W8AnimObj* W8Monster::GetCurrentAnimation()
{
    W8MonsterRep* representation = m_pRep;

    return *representation->animations[representation->current_cycle].GetAt(
        representation->current_subcycle);
}

// FUNCTION: WIZ8 0x004caac0
void W8Monster::SetCurrentAnimationScale(float arg_scale)
{
    W8MonsterRep* representation = m_pRep;

    *representation->animation_scales[representation->current_cycle].GetAt(
        representation->current_subcycle) = arg_scale;
}

/* Resolve the active cycle/subcycle AnimObj at the representation's selected LOD. */
// FUNCTION: WIZ8 0x004c3f00
W8AniMesh* W8Monster::GetCurrentAniMesh()
{
    W8AnimObj* animation =
        *m_pRep->animations[m_pRep->current_cycle].GetAt(m_pRep->current_subcycle);
    if (animation == 0) {
        srAssertFail("pao", "C:\\Projects\\Wizardry 8\\Engine Code\\Monster.cpp", 0xc4e, 0);
    }
    return AnimObjEntry(animation, m_pRep->m_bLOD, 0);
}

/* Store one value in the two cycle records used as its compact mirrors, then
   propagate it to every attached object's +0x28 field.  The body consumes two
   cdecl arguments; callers that reserve another stack slot clean it themselves. */
// FUNCTION: WIZ8 0x004c5870
void MonsterSetLocationId(W8Monster* monster, int value)
{
    int index;
    int count;

    if (monster == 0) {
        srAssertFail("pMonster", "C:\\Projects\\Wizardry 8\\Engine Code\\Monster.cpp", 0x125f, 0);
    }
    monster->location_id = value;
    monster->movement.location_id = static_cast<unsigned short>(value);
    if (monster->m_plsSoundEvents != 0) {
        count = monster->m_plsSoundEvents->GetCount();
        index = 0;
        if (count > 0) {
            do {
                int propagated_value = monster->location_id;
                W8SoundEvent* event = *monster->m_plsSoundEvents->GetAt(index);
                ++index;
                event->location_id = propagated_value;
            } while (index < count);
        }
    }
}

// FUNCTION: WIZ8 0x004c5710
bool MonsterHasPendingCycle(W8Monster* monster)
{
    return monster->m_pRep->pending_cycle != -1;
}

/* Compare the cycle's selected frame against the renderer's typed current-model
   slot.  Prop.cpp independently compares that slot with srModelInstance values. */
// FUNCTION: WIZ8 0x004c56f0
bool MonsterUsesCurrentModelInstance(W8GrCycle* cycle)
{
    srModelInstance* current = cycle->GetCurrentModelInstance();
    return current == GetPickedModelInstance();
}

// FUNCTION: WIZ8 0x004c5730
void MonsterGetLocation(W8Monster* monster, srVector3T<float>* location)
{
    monster->m_pRep->GetLocation(location);
}

// FUNCTION: WIZ8 0x004c5750
void MonsterGetLocalLocation(W8Monster* monster, srVector3T<float>* location)
{
    monster->m_pRep->GetLocalLocation(location);
}

/* The wrapper is intentionally unguarded: every retail caller supplies a live
   Monster, and the original immediately dispatches through slot 16. */
// FUNCTION: WIZ8 0x004c59a0
void UpdateMonster(W8Monster* monster)
{
    monster->Update();
}

// FUNCTION: WIZ8 0x004c5a80
bool MonsterIsCycleSupported(W8Monster* monster, signed char cycle)
{
    if (monster != 0) {
        return monster->IsCycleSupported(cycle);
    }
    return false;
}

// FUNCTION: WIZ8 0x004c5b10
unsigned char MonsterReplacePath(W8Monster* monster, W8PathAI* path)
{
    if (monster != 0) {
        return monster->ReplacePath(path);
    }
    return 0;
}

/* Reset pitch, update the Navigator's facing, and rebuild the representation's
   complete yaw/pitch/roll matrix. */
// FUNCTION: WIZ8 0x004c5b60
void MonsterSetFacing(W8Monster* monster, float angle)
{
    srMatrix3T<float> rotation;

    if (monster == 0) {
        return;
    }

    monster->SetAngles(NormalizeAngle(angle));
    monster->SetPitch(0.0f);

    rotation.SetIdentity();

    angle = NormalizeAngle(monster->GetYaw() + g_monster_rotation_offset);
    if (angle != g_double_zero) {
        rotation.RotateAboutY(sin(angle), cos(angle));
    }

    angle = monster->GetPitch();
    if (angle != g_float_zero && angle != g_double_zero) {
        rotation.RotateAboutX(sin(angle), cos(angle));
    }

    angle = monster->movement.roll;
    if (angle != g_float_zero && angle != g_double_zero) {
        rotation.RotateAboutZ(sin(angle), cos(angle));
    }

    monster->m_pRep->SetRotation(&rotation);
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
bool MonsterGetMirrorX(W8Monster* monster)
{
    return monster->mirror_x;
}

// FUNCTION: WIZ8 0x004c5800
void MonsterSetMirrorX(W8Monster* monster, bool state)
{
    monster->mirror_x = state;
}

// FUNCTION: WIZ8 0x004c5820
unsigned char MonsterGetHighlightMask(W8Monster* monster)
{
    return monster->m_pRep->highlight_mask;
}

// FUNCTION: WIZ8 0x004c5840
void MonsterSetHighlightMask(W8Monster* monster, unsigned char flag)
{
    monster->m_pRep->highlight_mask = flag;
}

/* Cycle 18's pointee carries the scale at +0x5f0. Both accessors reach it the
   same way - through the pointer at the cycle's +0x0c, which 0x004E60B0 also
   reads a byte from - so the pointee is a shared engine object rather than
   anything the cycle owns. It is not modelled: only this one field is known. */
// FUNCTION: WIZ8 0x004c5780
float MonsterGetScale(W8Monster* monster)
{
    return monster->m_pRep->scale;
}

// FUNCTION: WIZ8 0x004c57a0
void MonsterSetScale(W8Monster* monster, float scale)
{
    monster->m_pRep->scale = scale;
}

// FUNCTION: WIZ8 0x004c57c0
void MonsterGetScaleRange(W8Monster* monster, float* minimum, float* maximum)
{
    W8MonsterRep* runtime = monster->m_pRep;

    *minimum = runtime->minimum_scale;
    *maximum = runtime->maximum_scale;
}

/* Returns the previous animation state and timestamps every update through the
   recovered shared SurRender timer. */
// FUNCTION: WIZ8 0x004c5a00
unsigned char MonsterSetAnimating(W8Monster* monster, bool animating)
{
    if (monster != 0) {
        W8MonsterRep* runtime = monster->m_pRep;
        unsigned char previous = runtime->animation_playing;

        runtime->animation_playing = animating;
        runtime->timer = g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT);
        return previous;
    }
    return 0;
}

// FUNCTION: WIZ8 0x004c59e0
unsigned char MonsterIsAnimating(W8Monster* monster)
{
    if (monster != 0) {
        return monster->m_pRep->animation_playing;
    }
    return 0;
}

/* A blocking script cycle keeps its pending-cycle slot; otherwise the request
   is stored as the representation's pending cycle. */
// FUNCTION: WIZ8 0x004c5aa0
void MonsterSetPendingCycle(W8Monster* monster, int cycle)
{
    if (monster != 0 && (monster->runtime_flags & W8_MONSTER_SCRIPT_WAIT) == 0) {
        monster->m_pRep->pending_cycle = static_cast<signed char>(cycle);
    }
}

// FUNCTION: WIZ8 0x004c5e40
void MonsterSetRuntimeBehaviour(W8Monster* monster, signed char behaviour)
{
    if (monster != 0) {
        if (behaviour < 1 || behaviour > 3) {
            srAssertFail("bBehaviour >= BEHAVIOUR_FIRST && bBehaviour <= BEHAVIOUR_LAST",
                         "..\\Engine Code\\Include\\AnimRep.hpp", 0x87, 0);
        }
        monster->m_pRep->pending_behaviour = behaviour;
    }
}

// FUNCTION: WIZ8 0x004c5ee0
bool MonsterIsScalingY(W8Monster* monster)
{
    if (monster != 0) {
        return (monster->runtime_flags & W8_MONSTER_SCALING_Y) != 0;
    }
    return false;
}

// FUNCTION: WIZ8 0x004c6160
void MonsterSetActive(W8Monster* monster, bool state)
{
    if (monster != 0) {
        monster->active = state;
    }
}

/* Named by the MonsterManager assertions. A null monster answers -1 rather than
   forwarding, which is how the callers tell "no monster" from a real result. */
// FUNCTION: WIZ8 0x004c5b40
int MonsterQuery(W8Monster* monster, W8MonsterQueryKind query)
{
    if (monster != NULL) {
        return monster->Query(query);
    }
    return -1;
}

// FUNCTION: WIZ8 0x004ca4c0
bool W8Monster::IsDying()
{
    bool dying =
        Query(W8_MONSTER_QUERY_CYCLE) == W8_MONSTER_CYCLE_DIE || m_pRep->pending_cycle == 0x15;

    return dying;
}

/* Resolve mapped vertex zero on the current model and transform it into world
   space. Models without that mapping use the Navigator position plus the
   Monster's vertical offset. */
// FUNCTION: WIZ8 0x004c72a0
void W8Monster::GetMappedPosition(srVector3T<float>* position)
{
    srModelInstance* instance = GetCurrentModelInstance();

    if (instance != 0) {
        stMeshModel* mesh = static_cast<stMeshModel*>(instance->getModel());
        while (mesh != 0) {
            int index = mesh->FindMappedIndex(0);

            if (index >= 0) {
                srVector3T<float>* vertices;
                if ((mesh->flags & W8_MESH_HAS_FRAME_STORAGE) == 0) {
                    vertices = mesh->getVertexLoc();
                } else {
                    vertices = mesh->GetVertexLocations(0, true, 0.0f);
                }
                if (vertices != 0) {
                    srMatrix4T<float> matrix;

                    *position = vertices[index];
                    instance->getWorldSpaceMatrix(matrix);
                    *position = matrix.TransformPoint(*position);
                    return;
                }
            }
            mesh = mesh->next;
        }
    }

    *position = movement.position;
    position->y += movement.height_offset;
}

/* Six thin bodies over the live animation object. Each is a null check and a
   forward, or a single member read; nothing here says what the members and
   slots are for, so each is named for what it reaches. */

/* Keep the position copy through the shared double-argument vector setter:
   retail 0x004C5A4F-0x004C5A63 round-trips each component through the FPU. */
// FUNCTION: WIZ8 0x004c5a40
void MonsterSelectLOD(W8Monster* monster, const srVector3T<float>* position)
{
    srVector3T<float> local;

    if (monster != 0) {
        local.Set(position->x, position->y, position->z);
        monster->SelectLOD(&local);
    }
}

/* Set the forced subcycle; if no cycle is pending, use the current cycle. */
// FUNCTION: WIZ8 0x004c6c00
void W8Monster::SetForcedSubcycle(signed char value)
{
    m_pRep->forced_subcycle = value;
    if (m_pRep->pending_cycle == -1) {
        m_pRep->pending_cycle = m_pRep->current_cycle;
    }
}

/* Writes the sixteen-byte block the cycle runtime carries at 0x4c, but only for
   a monster that is neither absent nor already answering the dying cycle to
   query six - the same 0x15 the death test compares against, reached the same
   way. Both guards leave through one shared exit, which is why the body has a
   single epilogue despite testing two things. The block arrives by value and is
   stored as one assignment. */
// FUNCTION: WIZ8 0x004c5ad0
void MonsterSetHighlightColour(W8Monster* monster, srVector4T<float> block)
{
    if (monster != 0 && monster->Query(W8_MONSTER_QUERY_CYCLE) != W8_MONSTER_CYCLE_DIE) {
        monster->m_pRep->highlight_colour = block;
    }
}

/* Return the monster's AI record, or nothing for an absent monster. */
// FUNCTION: WIZ8 0x004c5b30
W8AIRecord* MonsterGetAIRecord(W8Monster* monster)
{
    if (monster != 0) {
        return monster->m_pAI;
    }
    return 0;
}

/* Expose Navigator yaw through the enclosing Monster. */
// FUNCTION: WIZ8 0x004c5770
float MonsterGetYaw(W8Monster* monster)
{
    return monster->GetYaw();
}

/* Two null-checked forwards that share one shape: a monster that is not there
   is simply not acted on. */
// FUNCTION: WIZ8 0x004c5ea0
void MonsterSubmitTargetValue(W8Monster* monster)
{
    if (monster != 0) {
        monster->SubmitTargetValue();
    }
}

// FUNCTION: WIZ8 0x004c6140
void MonsterClearMovement(W8Monster* monster)
{
    if (monster != 0) {
        monster->ClearMovement();
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

/* Attach or detach the TriRed target marker for one party slot on a monster. */
// FUNCTION: WIZ8 0x004c5eb0
void NotifyMonsterHighlight(int party_slot, int location_id, int on)
{
    SetMonsterPartySlotMarker(party_slot, location_id, on);
}

/* The public forwarding boundary preserves the loader's AL result. Both
   MonsterManager callers assert that result immediately after this call. */
// FUNCTION: WIZ8 0x004c58e0
unsigned char MonsterReadAllCycles(const W8GrCycleLoadContext* context, const char* monster_name,
                                   W8Monster** monster, int load_value, int location_id)
{
    return ReadOrCloneMonsterCycles(context, monster_name, monster, load_value, location_id);
}

/* Load one monster cycle through GrCycle's polymorphic factory boundary, then
   fill in any particle event whose cycle matches but whose subcycle was left
   at the -1 sentinel.  The factory accepts the base-class output slot; object
   type zero is what proves the resulting object is a W8Monster here. */
// FUNCTION: WIZ8 0x004C5910
unsigned char LoadMonsterCycle(const W8GrCycleLoadContext* context, const char* mon_name,
                               W8Monster** monster, int cycle, int value)
{
    W8GrCycle* loaded = *monster;
    bool success = LoadGrCycle(context, mon_name, &loaded, cycle, value, "data\\monsters", 0);
    *monster = static_cast<W8Monster*>(loaded);

    if ((*monster)->m_plsParticles != 0) {
        int count = (*monster)->m_plsParticles->GetCount();
        if (count != 0) {
            for (int index = 0; index < count; ++index) {
                W8GrCycleParticleAttachment* event = *(*monster)->m_plsParticles->GetAt(index);
                if (event->cycle == cycle && event->subcycle == -1) {
                    event->subcycle = (*monster)->m_pRep->current_subcycle;
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
void MonsterStopAllNavigators(void)
{
    StopAllNavigators();
}

// FUNCTION: WIZ8 0x004c61f0
void MonsterResumeAllNavigators(void)
{
    ResumeAllNavigators();
}

/*
 * Six more null-guarded forwarders onto the Navigator base at +0x18, the same shape
 * MonsterClearMovement has: the guard tests the monster, the receiver is
 * derived from it with a `lea`, and a monster that is not there is simply not
 * acted on. What each one answers on the null path is the evidence for its
 * return type - a cleared AL for the byte-sized ones, a loaded 0.0f for the
 * float, and a bare return for the four that hand nothing back.
 */
// FUNCTION: WIZ8 0x004c5f50
void MonsterSetNavigatorMovementScale(W8Monster* monster, float value)
{
    if (monster != 0) {
        monster->SetMovementScale(value);
    }
}

// FUNCTION: WIZ8 0x004c5f70
float MonsterGetNavigatorMovementScale(W8Monster* monster)
{
    if (monster != 0) {
        float value = monster->GetMovementScale();
        return value;
    }
    return 0.0f;
}

// FUNCTION: WIZ8 0x004c5f90
unsigned char MonsterConfigureMovementToPosition(W8Monster* monster,
                                                 const srVector3T<float>* position)
{
    if (monster != 0) {
        return monster->ConfigureMovementToPosition(position);
    }
    return 0;
}

// FUNCTION: WIZ8 0x004c5fb0
void MonsterAddPathPoint(W8Monster* monster, const srVector3T<float>* argument)
{
    if (monster != 0) {
        monster->AddPathPoint(argument);
    }
}

// FUNCTION: WIZ8 0x004c5fd0
void MonsterSetPathLooping(W8Monster* monster, char value)
{
    if (monster != 0) {
        monster->SetPathLooping(value);
    }
}

/* Pass a copied position through Navigator's collision adjustment and install
   the adjusted result on the Monster's ordinary Navigator base. */
// FUNCTION: WIZ8 0x004c5f00
void MonsterSetAdjustedPosition(W8Monster* monster, const srVector3T<float>* position)
{
    srVector3T<float> current;
    srVector3T<float> adjusted;
    srVector3T<float>* adjusted_position;
    srVector3T<float> result;

    current = *position;
    adjusted_position = monster->AdjustPosition(&adjusted, &current, &current);
    result = *adjusted_position;
    monster->SetPositionInternal(&result);
}

// FUNCTION: WIZ8 0x004c5ff0
unsigned short MonsterApproachStartupNavigator(W8Monster* monster, double separation)
{
    unsigned short result;

    if (monster != 0) {
        result = monster->SetMovementTargetToNavigator(g_startup_world, separation);
        if (result != 0) {
            monster->movement.boundary_enabled = false;
        }
        return result;
    }
    return 0;
}

// FUNCTION: WIZ8 0x004c6030
unsigned char MonsterLinkToStartupNavigator(W8Monster* monster)
{
    if (monster != 0) {
        W8Navigator* target = g_startup_world;

        return monster->LinkToNavigator(target, WorldGetFarClip(GetWorld()) * 2.0);
    }
    return 0;
}

// FUNCTION: WIZ8 0x004c6070
unsigned short MonsterConfigureMovementToPlayer(W8Monster* monster, float separation,
                                                float maximum_distance, srVector3T<float> position,
                                                int trace_mode, unsigned char* probe_result)
{
    if (monster != 0) {
        W8Navigator* target = g_startup_world;

        return monster->ConfigureMovementToNavigator(target, separation, maximum_distance, position,
                                                     trace_mode, monster->GetYaw(), probe_result);
    }
    return 0;
}

// FUNCTION: WIZ8 0x004c60d0
unsigned short MonsterConfigureMovementToMonster(W8Monster* monster, W8Monster* target,
                                                 float separation, float maximum_distance,
                                                 srVector3T<float> position, int trace_mode,
                                                 unsigned char* probe_result)
{
    if (monster != 0 && target != 0) {
        return monster->ConfigureMovementToNavigator(target, separation, maximum_distance, position,
                                                     trace_mode, monster->GetYaw(), probe_result);
    }
    return 0;
}

// FUNCTION: WIZ8 0x004c6200
void MonsterSetNavigatorHalted(W8Monster* monster, bool value)
{
    if (monster != 0) {
        monster->SetHalted(value);
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
        monster_info = MonsterGetScriptPartByLocationIndex(
            MonsterGetIndexByLocationID(0x14b3, MONSTER_CPP, monster->location_id, true));
        if (monster_info->control_state != W8_MONSTER_CONTROL_LURED) {
            GetCameraPosition(&position);
            if (alternate != 0) {
                monster->SetFacingToward(&position);
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
void MonsterAimAtMonster(W8Monster* monster, W8Monster* target, bool alternate)
{
    W8MonsterInfo* monster_info;
    srVector3T<float> target_position;
    srVector3T<float> position;

    if (monster != 0 && target != 0) {
        monster_info = MonsterGetScriptPartByLocationIndex(
            MonsterGetIndexByLocationID(0x14c7, MONSTER_CPP, monster->location_id, true));
        if (monster_info->control_state != W8_MONSTER_CONTROL_LURED) {
            target_position = target->GetPosition();
            position = target_position;
            if (alternate) {
                monster->SetFacingToward(&position);
            } else {
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
void W8Monster::CollectModelInstances(W8GrowableVector<stModelInstance*>* instances)
{
    int cycle;

    GetTotalAnimationCount();
    for (cycle = W8_MONSTER_CYCLE_BIRTH; cycle < W8_MONSTER_CYCLE_COUNT; ++cycle) {
        int subcycle;

        for (subcycle = 0; subcycle < m_pRep->GetNumSubsPerCycle(static_cast<signed char>(cycle));
             ++subcycle) {
            W8GrowableVector<W8AnimObj*>* cycle_animations = &m_pRep->animations[cycle];
            W8AnimObj* animation;

            if (subcycle >= cycle_animations->GetCount()) {
                ShutdownWithErrorBox(
                    reinterpret_cast<
                        const char*>( // reinterpret-ok: String returns a logging buffer
                        String("Monster %s: Missing CYCLE_%s, sub-cycle %d", m_pRep->name,
                               g_cycle_names[cycle].name, subcycle)));
            }
            animation = *cycle_animations->GetAt(subcycle);
            if (animation == 0) {
                continue;
            }

            if (AnimationIsRunning(animation) == 0) {
                int list_index;

                for (list_index = 0; list_index < 3; ++list_index) {
                    W8AniMesh* mesh = animation->entries[list_index];
                    if (mesh != 0) {
                        int frame_count = AniMeshValue(mesh);

                        if (mesh->flags.single_instance) {
                            instances->Add(GetAniMeshFrame(mesh, 0));
                        } else {
                            int frame;

                            for (frame = 0; frame < frame_count; ++frame) {
                                instances->Add(GetAniMeshFrame(mesh, frame));
                            }
                        }
                    }
                }
            } else if (AnimationIsRunning(animation) == 1) {
                int list_index;

                for (list_index = 0; list_index < 3; ++list_index) {
                    W8PList* list = animation->meshes[list_index];
                    if (list != 0) {
                        int mesh_index;
                        int mesh_count = PLLength(list);

                        for (mesh_index = 0; mesh_index < mesh_count; ++mesh_index) {
                            W8AniMesh* mesh = static_cast<W8AniMesh*>(PLGet(list, mesh_index));
                            if (mesh != 0) {
                                int frame;
                                int frame_count = AniMeshValue(mesh);

                                for (frame = 0; frame < frame_count; ++frame) {
                                    instances->Add(GetAniMeshFrame(mesh, frame));
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
bool W8Monster::ReplaceSkinTexture(int stage, const char* old_name, const char* new_name)
{
    char path[200];
    bool replaced = false;

    sprintf(path, "Data\\Monsters\\Bitmaps\\%s", new_name);
    srTextureIFace* texture = LoadTextureFromPath(path, 0, true);
    if (texture == 0) {
        ShutdownWithErrorBox(FormatString("Missing skin texture: %s", new_name));
        return false;
    }

    W8Vector<stModelInstance*> instances;
    CollectModelInstances(&instances);
    for (int index = 0; index < instances.GetCount(); ++index) {
        if ((*instances.GetAt(index))->ReplaceDamageStageTexture(stage, old_name, texture) != 0) {
            replaced = true;
        }
    }

    if (!replaced && m_plsParticles != 0) {
        for (int index = 0; index < m_plsParticles->GetCount(); ++index) {
            W8GrCycleParticleAttachment* event = *m_plsParticles->GetAt(index);
            if (event->m_pstParticles->ReplaceTexture(old_name, texture) != 0) {
                replaced = true;
            }
        }
    }
    return replaced;
}

/* Damage table names are the parsed skin name followed by its numeric stage.
   A frame either creates the table or attaches the already-created table. */
// FUNCTION: WIZ8 0x004c6880
int W8Monster::AddDamageStage(const char* base_name, int stage)
{
    char name[128];
    int result = -1;
    W8Vector<stModelInstance*> instances;

    sprintf(name, "%s%d", base_name, stage);
    CollectModelInstances(&instances);
    for (int index = 0; index < instances.GetCount(); ++index) {
        stModelInstance* instance = *instances.GetAt(index);
        if (instance->FindDamageStage(name) == -1) {
            result = instance->AddDamageStage(name);
        } else {
            result = instance->AddExistingDamageStage(name);
        }
    }
    return result;
}

/* The final registered Monster for a cycle name owns removal of that name's
   per-mesh skin tables. */
// FUNCTION: WIZ8 0x004c6b10
void W8Monster::RemoveCycleSkinTables()
{
    const char* cycle_name = GetRegisteredName();
    W8Vector<stModelInstance*> instances;

    if (cycle_name != 0) {
        CollectModelInstances(&instances);
        for (int index = 0; index < instances.GetCount(); ++index) {
            stMeshModel* mesh = static_cast<stMeshModel*>((*instances.GetAt(index))->getModel());
            for (; mesh != 0; mesh = mesh->next) {
                mesh->RemoveSkinTablesForCycle(cycle_name);
            }
        }
    }
}

/* Select the damage-stage model on every frame instance owned by this
   Monster. UpdateMonsterDamageAppearance supplies the HP-derived stage. */
// FUNCTION: WIZ8 0x004c6990
void W8Monster::SetDamageStage(int stage)
{
    W8Vector<stModelInstance*> instances;
    int index;

    CollectModelInstances(&instances);
    for (index = 0; index < instances.GetCount(); ++index) {
        (*instances.GetAt(index))->damage_stage = stage;
    }
}

/* Every frame instance in one Monster carries the same number of available
   damage stages, so the first instance supplies the count. */
// FUNCTION: WIZ8 0x004c6a50
int W8Monster::GetDamageStageCount()
{
    W8Vector<stModelInstance*> instances;

    CollectModelInstances(&instances);
    if (instances.GetCount() != 0) {
        return (*instances.GetAt(0))->damage_stage_tables.capacity;
    }
    return 0;
}

/* Floating damage feedback: render the amount into a scratch ARGB1555
   surface, wrap it in a camera-facing poster ahead of the monster, and fire a
   ten-particle blood burst at the same spot. The poster scales with camera
   distance so the number stays legible. */
// FUNCTION: WIZ8 0x004C6C30
void W8Monster::SpawnDamageNumber(unsigned int amount)
{
    if ((runtime_flags & W8_MONSTER_FADED_OUT) != 0) {
        return;
    }

    srColorSurface* surface =
        SR_NEW(srColorSurface)(srPixelConvert::SURFACE_ARGB1555, 0x100, 0x100);
    surface->fill(0);
    unsigned char* data = static_cast<unsigned char*>(surface->getDataPtr());
    if (data != 0) {
        wchar_t text[20];
        srVector3T<float> position;
        srVector3T<float> camera_position;
        srVector3T<float> facing;
        srVector3T<double> location;
        srVector4T<float> colour;
        srMatrix3T<float> rotation;
        srMatrix3T<double> world;
        stModelInstance* poster;
        stParticle* particle;
        srMaterial* material;
        srShader shader;
        float distance;
        float scale;
        float pitch;
        float yaw;

        SaveFontSettings();
        SetFontDestBuffer(FontDestBuffer, 0, 0, surface->getWidth(), surface->getHeight(),
                          static_cast<unsigned char>(FontDestWrap));
        SetFont(g_monster_damage_font);
        swprintf(text, g_format_d, amount);
        gprintf_buffer(data, surface->getPitch(), g_monster_damage_font,
                       0x80 - StringPixLength(text, g_monster_damage_font) / 2,
                       0x80 - GetFontHeight(g_monster_damage_font) / 2, text);
        RestoreFontSettings();

        GetMappedPosition(&position);
        facing = GetPosition();
        pitch = ElevationToTargetCPP(&facing);
        facing = GetPosition();
        yaw = HeadingToTargetCPP(&facing);
        OffsetPositionByYawPitch(500.0f, &position, yaw, pitch);
        GetCameraPosition(&camera_position);
        distance = (position - camera_position).Length();
        if (distance > g_float_fifty_thousand) {
            distance = g_float_fifty_thousand;
        }
        scale = (distance * 0.0002f + g_float_one) * 375.0f;
        poster = static_cast<stModelInstance*>(VideoMakePoster(surface, scale, scale, true));
        if (poster != 0) {
            poster->alignment_flags.set(0, 1);
            location.SetFromFloat(&position);
            poster->setLocation(location);
            poster->setParent(g_world->dynamic_scene, 1);
            m_pRep->linked_runtime_objects.Add(poster);

            particle = new stParticle(g_world->dynamic_scene, 0xa);
            material = SR_NEW(srMaterial);
            colour.Set(0.0f, 0.0f, 0.0f, 1.0f);
            material->setEmissive(colour);
            material->setDiffuse(colour);
            particle->SetMaterial(material);
            particle->SetTexture(
                LoadTextureFromFolder("Data\\Monsters\\Bitmaps\\", "BloodParticle.tga", true));
            shader.value = 0x100c4b3;
            particle->SetRenderFlags(shader);
            particle->particle_size = 20.0;
            particle->expiry_mode = W8_PARTICLE_EXPIRY_TIMED;
            particle->bounds_origin.SetZero();
            particle->cone_yaw = 1.5707963f;
            particle->cone_pitch = 1.5707963f;
            particle->emission_interval = 1;
            particle->has_acceleration = 1;
            particle->speed_mode = W8_PARTICLE_SPEED_RANDOM;
            particle->emission_mode = W8_PARTICLE_EMISSION_SINGLE;
            particle->lifetime_ms = 10000;
            particle->direction_mode = W8_PARTICLE_DIRECTION_CONE;
            particle->speed_min = 1000.0f;
            particle->speed_max = 3000.0f;
            particle->emission_limit = 8;
            particle->release_when_done = true;
            particle->bounds_mode = W8_PARTICLE_BOUNDS_SPHERE;
            particle->bounds_radius = 1000.0f;
            location.SetFromFloat(&position);
            particle->setLocation(location);
            GetCurrentModelInstance()->getRotation(rotation);
            rotation.RotateAboutY(sin(g_camera_pi), cos(g_camera_pi));
            world.vectors[0].SetFromFloat(&rotation.vectors[0]);
            world.vectors[1].SetFromFloat(&rotation.vectors[1]);
            world.vectors[2].SetFromFloat(&rotation.vectors[2]);
            particle->rotate(world);
        }
    }
}

/* Resolve the database-controlled render gate after the Monster's transient
   runtime overrides. The alternate argument selects the secondary live-info
   flag used by the world-update path. */
// FUNCTION: WIZ8 0x004c7c00
bool W8Monster::IsRenderable(bool alternate)
{
    bool disabled = this->disabled;
    int location_id = this->location_id;
    W8MonsterInfo* monster_info;
    W8MonsterRecord* record;

    if (disabled) {
        return false;
    }
    if (inactive) {
        return true;
    }
    if (location_id == -1) {
        return true;
    }
    if ((runtime_flags & W8_MONSTER_REMOVE_AFTER_FADE) != 0) {
        return true;
    }
    if (fade_state != W8_MONSTER_FADE_IDLE) {
        return true;
    }

    monster_info = MonsterGetScriptPartByLocationIndex(
        MonsterGetIndexByLocationID(0x1977, MONSTER_CPP, location_id, true));
    record = GetMonsterDataForInfo(monster_info);
    if (record->camouflage0 > 0) {
        return monster_info->party_threat.party_detected;
    }
    if (alternate) {
        return monster_info->party_threat.visible_to_player;
    }
    return monster_info->within_viewing_distance;
}

/* Discover animated material state once, cache it in the monster flags, and restart
   the selected model's animated texture on frame zero when present. */
// FUNCTION: WIZ8 0x004c51d0
void W8Monster::InitializeAnimatedTexture()
{
    srModelInstance* instance = 0;

    if (m_pRep->subcycle == 0) {
        if ((runtime_flags & W8_MONSTER_TEXTURE_CHECKED) == 0) {
            srMeshModel* model;

            instance = SelectCycleFrameLod(m_pRep->current_cycle, 0, m_pRep->m_bLOD);
            model = static_cast<srMeshModel*>(instance->getModel());
            if (!MeshHasAnimatedTexture(model)) {
                runtime_flags &= ~W8_MONSTER_ANIMATED_TEXTURE;
            } else {
                runtime_flags |= W8_MONSTER_ANIMATED_TEXTURE;
            }
            runtime_flags |= W8_MONSTER_TEXTURE_CHECKED;
        }
        if ((runtime_flags & W8_MONSTER_ANIMATED_TEXTURE) != 0) {
            if (instance == 0) {
                instance = SelectCycleFrameLod(m_pRep->current_cycle, 0, m_pRep->m_bLOD);
            }
            SetModelAnimatedTextureFrame(instance, 0);
        }
    }
}

/* Advance a cycle's representation through its own virtual layout. */
// FUNCTION: WIZ8 0x004c59b0
void UpdateCycleRepresentation(W8GrCycle* cycle, W8World* world)
{
    cycle->UpdateRepresentation(world);
}

// FUNCTION: WIZ8 0x004C4EF0
void W8Monster::RefreshStandingHeight()
{
    W8MonsterRep* representation = m_pRep;
    signed char previous_cycle = representation->current_cycle;
    SetCycle(1);
    SetSubCycle(0);
    srVector3T<float> camera_location;
    camera_location = g_world->camera->getLocation();
    SelectLOD(&camera_location);
    srVector3T<float> minimum;
    srVector3T<float> maximum;
    GetAnimationBounds(&minimum, &maximum);
    representation->standing_height = maximum.y;
    if (previous_cycle != -1) {
        SetCycle(previous_cycle);
    }
}

// FUNCTION: WIZ8 0x004C5290
void W8Monster::ApplyRepresentationScale()
{
    float old_scale = movement.scale;
    SetScale(m_pRep->scale);
    for (int cycle = 0; cycle < W8_MONSTER_CYCLE_COUNT; ++cycle) {
        float scale = m_pRep->scale;
        if (cycle == W8_MONSTER_CYCLE_DIE) {
            scale = m_pRep->death_scale * m_pRep->scale;
        }
        float x_scale = scale;
        if (mirror_x) {
            x_scale = scale * -1.0f;
        }
        int animation_count = m_pRep->animations[cycle].GetCount();
        for (int index = 0; index < animation_count; ++index) {
            W8AnimObj* animation = *m_pRep->animations[cycle].GetAt(index);
            // The retail query precedes the null check.
            int frame_count = AnimObjValue(animation, 2);
            if (animation != 0 && frame_count != 0) {
                for (int frame = 0; frame < frame_count; ++frame) {
                    srModelInstance* instance =
                        AnimObjDispatch(animation, 2, static_cast<unsigned char>(frame));
                    instance->setScale(srVector3T<double>(x_scale, scale, scale));
                }
            }
        }
    }
    if (m_plsParticles != 0) {
        for (int index = 0; index < m_plsParticles->GetCount(); ++index) {
            stParticle* particle = (*m_plsParticles->GetAt(index))->m_pstParticles;
            particle->SetParticleScale(m_pRep->scale);
            if (mirror_x) {
                (*m_plsParticles->GetAt(index))->position.x *= -1.0f;
                particle->setScale(srVector3T<double>(-1.0, 1.0, 1.0));
            }
        }
    }
    if (m_ground_shadow != 0) {
        float ratio = m_pRep->scale / old_scale;
        m_ground_shadow->width *= ratio;
        m_ground_shadow->depth *= ratio;
    }
    if (m_pRep->monster_light != 0) {
        m_pRep->monster_light->SetRange(movement.collision_radius * g_float_three);
        m_pRep->monster_light->m_vertical_offset = movement.height_offset;
    }
}

/* Build a floating icon item from a bitmap path: clamp-wrapped texture, a
   500-unit-wide poster quad relocated 250 units up the Y axis with alignment
   enabled, wrapped in a W8Item whose mesh attachment and bounds are refreshed
   for the world. Returns 0 when the texture or quad could not be made. */
// FUNCTION: WIZ8 0x004C5500
W8Item* CreateMonsterIconItem(W8World* world, const char* path, int flag)
{
    stTextureFile* texture = new stTextureFile(path, 0);
    texture->setWrapS(srTextureIFace::WRAP_CLAMP);
    texture->setWrapT(srTextureIFace::WRAP_CLAMP);
    if (texture != 0) {
        texture->loadSurface();
        texture->autoRelease();
        stModelInstance* instance =
            static_cast<stModelInstance*>(MakePosterQuad(texture, 500.0f, 0.0f, true));
        if (instance != 0) {
            srVector3T<float> offset(0.0f, 250.0f, 0.0f);
            static_cast<srMeshModel*>(instance->getModel())->relocateVertices(offset);
            instance->setAlignment(1);
            W8Item* item = new W8Item();
            if (item != 0) {
                W8ItemRep* rep = static_cast<W8ItemRep*>(item->m_pRep);
                rep->flags |= 0x40;
                rep->m_psrMesh = instance;
                rep->RefreshBounds();
                item->AttachMesh(world);
                instance->light_scale.SetZero();
                return item;
            }
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x004C5810
void ApplyMonsterRepresentationScale(W8Monster* target)
{
    target->ApplyRepresentationScale();
}
// FUNCTION: WIZ8 0x004C5860
void DeleteMonster(W8Monster* monster)
{
    if (monster != NULL) {
        delete monster;
    }
}
// FUNCTION: WIZ8 0x004C59C0
void DetachMonsterRepresentation(W8Monster* monster, W8World* world)
{
    if (monster != 0 && world != 0) {
        monster->DetachRepresentation(world);
    }
}
// FUNCTION: WIZ8 0x004C5ED0
void RefreshMonsterStandingHeight(W8Monster* monster)
{
    if (monster != 0) {
        monster->RefreshStandingHeight();
    }
}
// FUNCTION: WIZ8 0x004C6220
void SetCombatInactiveFlag(unsigned char value)
{
    g_combat_inactive = value;
    g_combat_round_counter = 0;
}
