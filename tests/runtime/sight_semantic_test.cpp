#include "wiz8/engine_code/World.h"
#include "wiz8/float_constants.h"
#include "wiz8/local_code/Sight.h"
#include "wiz8/sr_api.h"

#include "sight_semantic_test.h"
#include "surrender/srCamera.h"

#include <direct.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static W8World g_sight_test_world;
static bool g_sight_sr_initialized = false;

/* Installs a private world with the camera the caller asked for — the same
   camera ComputeSightThreshold reads its far clip from through
   WorldGetFarClip — and restores the previous world and releases the camera
   on every exit path, including early-return failures. */
struct SightWorldScope {
    W8World* previous_world;
    srCamera* camera;

    SightWorldScope(srCamera* installed_camera, float far_clip)
    {
        previous_world = g_world;
        camera = installed_camera;
        if (!g_sight_sr_initialized) {
            _chdir("DLL");
            srInit();
            _chdir("..");
            g_sight_sr_initialized = true;
        }
        memset(&g_sight_test_world, 0, sizeof(g_sight_test_world));
        camera->setClipRange(1.0, far_clip);
        g_sight_test_world.camera = camera;
        g_world = &g_sight_test_world;
    }

    ~SightWorldScope()
    {
        g_world = previous_world;
        camera->release();
    }
};

static float ThresholdFacingTarget(float distance)
{
    srVector3T<float> observer;
    srVector3T<float> target;

    observer.Set(0.0f, 0.0f, 0.0f);
    target.Set(0.0f, 0.0f, distance);
    return ComputeSightThreshold(observer, target, 0.0f, 50, 0, 0, 0, 0, 0, 0, 0, distance);
}

static float ThresholdFacingAway(float distance)
{
    srVector3T<float> observer;
    srVector3T<float> target;

    observer.Set(0.0f, 0.0f, 0.0f);
    target.Set(0.0f, 0.0f, distance);
    return ComputeSightThreshold(observer, target, 3.1415927f, 50, 0, 0, 0, 0, 0, 0, 0, distance);
}

/* Full sight percent on a facing target: the threshold should scale with the
   camera's far clip, which is only true when the installed camera carries the
   clip range being measured. */
static float ThresholdFullSight(float distance)
{
    srVector3T<float> observer;
    srVector3T<float> target;

    observer.Set(0.0f, 0.0f, 0.0f);
    target.Set(0.0f, 0.0f, distance);
    return ComputeSightThreshold(observer, target, 0.0f, 100, 0, 0, 0, 0, 0, 0, 0, distance);
}

/* Leaves the scope via an early return, the same way a failed check exits —
   the destructor must still restore g_world and release the camera. */
static bool FailingSightBody()
{
    SightWorldScope scope(SR_NEW(srCamera)(static_cast<srNode*>(0)), 10000.0f);
    return false;
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
    srVector3T<float> observer;
    srVector3T<float> target;

    memset(result, 0, sizeof(*result));

    {
        SightWorldScope scope(SR_NEW(srCamera)(static_cast<srNode*>(0)), 10000.0f);

        observer.Set(0.0f, 0.0f, 0.0f);
        target.Set(100.0f, 0.0f, 100.0f);
        blind = ComputeSightThreshold(observer, target, 0.0f, 50, 0, 1, 0, 0, 0, 0, 0, 500.0f);
        result->blind_is_zero = blind == g_float_005ebb34;

        facing = ThresholdFacingTarget(1000.0f);
        away = ThresholdFacingAway(1000.0f);
        result->facing_away_reduces_range = away < facing;

        observer.Set(0.0f, 0.0f, 0.0f);
        target.Set(0.0f, 0.0f, 9000.0f);
        skip_fov =
            ComputeSightThreshold(observer, target, 3.1415927f, 50, 0, 0, 0, 0, 0, 1, 0, 9000.0f);
        result->skip_fov_restores_range = fabs(skip_fov - facing) < 1.0f;

        penalized =
            ComputeSightThreshold(observer, target, 3.1415927f, 50, 0, 0, 0, 5, 0, 0, 0, 9000.0f);
        result->penalty_source_reduces_range = penalized < away;

        attributed =
            ComputeSightThreshold(observer, target, 3.1415927f, 25, 0, 0, 0, 0, 0, 0, 0, 9000.0f);
        result->attribute_scales_range = attributed < away;

        observer.Set(0.0f, 0.0f, 0.0f);
        target.Set(100.0f, 0.0f, 0.0f);
        monster_to_player =
            ComputeSightThreshold(observer, target, 0.0f, 50, 0, 0, 0, 10, 3, 0, 0, 500.0f);
        observer.Set(100.0f, 0.0f, 0.0f);
        target.Set(0.0f, 0.0f, 0.0f);
        player_to_monster =
            ComputeSightThreshold(observer, target, 0.0f, 50, 15, 0, 0, 20, 1, 1, 4, 500.0f);
        result->same_primitive_party_and_monster =
            monster_to_player > g_float_005ebb34 && player_to_monster > g_float_005ebb34;
    }

    {
        float t10000;
        float t5000;
        {
            SightWorldScope scope(SR_NEW(srCamera)(static_cast<srNode*>(0)), 10000.0f);
            t10000 = ThresholdFullSight(1000.0f);
        }
        {
            SightWorldScope scope(SR_NEW(srCamera)(static_cast<srNode*>(0)), 5000.0f);
            t5000 = ThresholdFullSight(1000.0f);
        }
        result->far_clip_scales_threshold = fabs(t10000 - 2.0f * t5000) < 1.0f;
    }

    {
        W8World* before = g_world;
        FailingSightBody();
        result->world_restored_after_failure = g_world == before;
    }

    return result->blind_is_zero && result->facing_away_reduces_range &&
           result->skip_fov_restores_range && result->penalty_source_reduces_range &&
           result->attribute_scales_range && result->same_primitive_party_and_monster &&
           result->far_clip_scales_threshold && result->world_restored_after_failure;
}

void PrintSightSemanticResults(const SightSemanticResult* result)
{
    fprintf(stderr,
            "sight-threshold semantic: blind_is_zero=%u facing_away_reduces_range=%u "
            "skip_fov_restores_range=%u penalty_source_reduces_range=%u "
            "attribute_scales_range=%u same_primitive_party_and_monster=%u "
            "far_clip_scales_threshold=%u world_restored_after_failure=%u\n",
            result->blind_is_zero, result->facing_away_reduces_range,
            result->skip_fov_restores_range, result->penalty_source_reduces_range,
            result->attribute_scales_range, result->same_primitive_party_and_monster,
            result->far_clip_scales_threshold, result->world_restored_after_failure);
    fflush(stderr);
}
