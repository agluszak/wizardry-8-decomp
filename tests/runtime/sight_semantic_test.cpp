#include "wiz8/engine_code/World.h"
#include "wiz8/float_constants.h"
#include "wiz8/local_code/Sight.h"

#include "surrender/srCamera.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

struct SightSemanticResult {
    unsigned char blind_is_zero;
    unsigned char facing_away_reduces_range;
    unsigned char skip_fov_restores_range;
    unsigned char penalty_source_reduces_range;
    unsigned char attribute_scales_range;
    unsigned char same_primitive_party_and_monster;
};

static W8World g_sight_test_world;
static srCamera g_sight_test_camera;

static void SetupSightTestWorld(float far_clip)
{
    memset(&g_sight_test_world, 0, sizeof(g_sight_test_world));
    g_sight_test_camera.setClipRange(1.0, far_clip);
    g_sight_test_world.camera = &g_sight_test_camera;
    g_world = &g_sight_test_world;
}

static float ThresholdFacingTarget(float distance)
{
    return ComputeSightThreshold(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, distance, 0.0f, 50, 0, 0, 0, 0, 0, 0,
                                 0, distance);
}

static float ThresholdFacingAway(float distance)
{
    return ComputeSightThreshold(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, distance, 3.1415927f, 50, 0, 0, 0, 0,
                                 0, 0, 0, distance);
}

bool RunSightSemanticTests(SightSemanticResult* result)
{
    float facing;
    float away;
    float blind;
    float skip_fov;
    float penalized;
    float attributed;
    float monster_to_player;
    float player_to_monster;

    memset(result, 0, sizeof(*result));
    SetupSightTestWorld(10000.0f);

    blind = ComputeSightThreshold(0.0f, 0.0f, 0.0f, 100.0f, 0.0f, 100.0f, 0.0f, 50, 0, 1, 0, 0, 0,
                                  0, 0, 500.0f);
    result->blind_is_zero = blind == g_float_005ebb34;

    facing = ThresholdFacingTarget(1000.0f);
    away = ThresholdFacingAway(1000.0f);
    result->facing_away_reduces_range = away > facing;

    skip_fov = ComputeSightThreshold(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1000.0f, 3.1415927f, 50, 0, 0, 0,
                                     0, 0, 1, 0, 1000.0f);
    result->skip_fov_restores_range = fabs(skip_fov - facing) < 1.0f;

    penalized = ComputeSightThreshold(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1000.0f, 0.0f, 50, 0, 0, 0, 5,
                                      0, 0, 0, 1000.0f);
    result->penalty_source_reduces_range = penalized < facing;

    attributed = ComputeSightThreshold(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1000.0f, 0.0f, 25, 0, 0, 0, 0,
                                       0, 0, 0, 1000.0f);
    result->attribute_scales_range = attributed < facing;

    monster_to_player = ComputeSightThreshold(0.0f, 0.0f, 0.0f, 100.0f, 0.0f, 0.0f, 0.0f, 50, 0, 0,
                                              0, 10, 3, 0, 0, 500.0f);
    player_to_monster = ComputeSightThreshold(100.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 50, 15, 0,
                                              0, 20, 1, 1, 4, 500.0f);
    result->same_primitive_party_and_monster =
        monster_to_player > g_float_005ebb34 && player_to_monster > g_float_005ebb34;

    g_world = 0;
    return result->blind_is_zero && result->facing_away_reduces_range &&
           result->skip_fov_restores_range && result->penalty_source_reduces_range &&
           result->attribute_scales_range && result->same_primitive_party_and_monster;
}

void PrintSightSemanticResults(const SightSemanticResult* result)
{
    printf("WIZ8_SIGHT_SEMANTIC blind_is_zero=%u facing_away_reduces_range=%u "
           "skip_fov_restores_range=%u penalty_source_reduces_range=%u "
           "attribute_scales_range=%u same_primitive_party_and_monster=%u\n",
           result->blind_is_zero, result->facing_away_reduces_range,
           result->skip_fov_restores_range, result->penalty_source_reduces_range,
           result->attribute_scales_range, result->same_primitive_party_and_monster);
}
