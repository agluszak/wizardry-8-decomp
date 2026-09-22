#ifndef WIZ8_LAYOUTS_WORLD_H
#define WIZ8_LAYOUTS_WORLD_H

#include "surrender/srMath.h"
#include "wiz8/environment_colour.h"
#include "wiz8/layouts/plist.h"
#include "wiz8/named_position.h"
#include "wiz8/vector.h"

class srCamera;
class srModelInstance;
class srNode;
class srScene;
class stLevel;
class W8Octree;
struct W8Quad;
class W8Missile;
class W8SpellVisual;
struct MonGen;
class stLight;
class W8Prop;
class Trigger;
class stParticle;
struct W8PathAI;
struct W8GameData;

struct W8WorldCameraEntry {
    unsigned char positional_00[0x14];
    unsigned char positional_14[4];
    W8PathAI* path;
};

/* The 0x3c-byte CamPos record GetWorldCameraState writes and
   RestoreWorldCameraState reads (3dapi.cpp, assertions pWorld / CamPos).
   Two six-float records follow the point: pitch at +0x0c and yaw/angle at
   +0x24. Recall stores this same object on the character at +0x17d7. */
/* Six-float CamPos angle record: [0] is the live angle, the tail is serialized
   padding. GetCameraOrientation/SetCameraOrientation take these; their array
   typedef decays to float* so callers with scalar floats keep their authored
   shape. */
typedef float W8CameraAngleRecord[6];

struct W8WorldCameraState {
    srVector3T<float> position;
    W8CameraAngleRecord pitch;
    W8CameraAngleRecord yaw;
};
static_assert(sizeof(W8WorldCameraState) == 0x3c, "W8WorldCameraState_size");

static_assert(sizeof(W8WorldCameraEntry) == 0x1c, "W8WorldCameraEntry_must_be_0x1c");

/* Engine Code\3dapi.cpp. CreateWorld allocates and zeroes exactly 0xdc bytes;
   the list/vector setup and teardown routines prove the owned fields below. */
struct W8World {
    W8PList* plsMonsters;
    W8PList* plsItems;
    W8PList* plsProps;
    W8PList* plsCameras;
    W8PList* plsAmbientSounds;
    float environment_range_start_014;
    float environment_range_end_018;
    /* Third serialized environment channel (loaded from
       environment_colour.blue like the start/end fractions); no recovered
       consumer reads it. */
    float environment_range_blue_01c;
    float view_distance_020;
    float environment_intensity_024;
    /* Snapshot of environment_intensity_024 taken when a fade-out starts; the
       lighting transition multiplies g_light_scale by this base. */
    float environment_base_intensity_028;
    EnvironmentColour environment_colour_02c;
    stLevel* level;
    srScene* static_scene;
    srNode* dynamic_scene;
    srCamera* camera;
    srModelInstance** psrMeshes;
    W8GameData* m_owned_04c;
    W8Octree* octree;
    stLight* camera_light;
    unsigned char m_padding_058[0x11];
    unsigned char m_loaded;
    unsigned char m_padding_06a[2];
    W8Quad* m_owned_06c;
    srModelInstance* update_mesh_source;
    float render_range_74;
    float render_range_78;
    unsigned char m_padding_07c[0x20];
    W8PList m_list_09c;
    W8PList m_lights_0a8;
    W8GrowableVector<W8SpellVisual*>* spell_visuals;
    W8GrowableVector<W8Missile*>* missiles;
    W8GrowableVector<stLight*>* lights_to_update;
    W8GrowableVector<W8Prop*>* collidable_props;
    W8GrowableVector<MonGen*>* monster_generators;
    W8GrowableVector<Trigger*>* triggers;
    W8GrowableVector<stParticle*>* particles;
    W8GrowableVector<W8NamedPosition*>* named_positions;
    unsigned char m_padding_0d4[8];
};

static_assert(sizeof(W8World) == 0xdc, "W8World_must_be_0xdc");

extern W8World* g_world;
extern W8World* g_world_659ab8;
extern unsigned char g_flag_6081e4;
extern int g_value_659c14;

#endif
