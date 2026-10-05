#include "wiz8/engine_code/stTextureFile.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/stParticle.h"
#include "wiz8/engine_code/game_timer.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/stTextureAnim.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/float_constants.h"
#include "wiz8/geometry.h"
#include "wiz8/sr_api.h"
#include "wiz8/virtual_file.h"
#include "surrender/srCore.h"
#include "surrender/srGERD.h"
#include "surrender/srHeap.h"
#include "surrender/srNode.h"
#include "surrender/srTriMeshPipeline.h"
#include "FileMan.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/PolyPick.h"
// GLOBAL: WIZ8 0x005ebc60
const float g_float_005ebc60 = 0.0020000000949949026f;
// GLOBAL: WIZ8 0x005ec438
const float g_float_005ec438 = 3.0517578125e-05f;
// GLOBAL: WIZ8 0x005ec8d0
const double g_double_005ec8d0 = 0.001;
// GLOBAL: WIZ8 0x005ecc38
const float g_float_005ecc38 = -0.0010000000474974513f;
// GLOBAL: WIZ8 0x005ecc3c
const float g_float_005ecc3c = -1000.0f;
// GLOBAL: WIZ8 0x005ecc40
const float g_float_005ecc40 = 0.00019174758926965296f;

// STRING: WIZ8 0x0060BF6C
#define ST_PARTICLE_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\stParticle.cpp"

// VTABLE: WIZ8 0x005ECBD0
// class stParticle

// VTABLE: WIZ8 0x005ECC04
// class srClassSupport<stParticle,srNode,0,65545>

/* Return the renderer flags as a value. VC6 lowers the four-byte class return
   through its hidden result pointer. */
// FUNCTION: WIZ8 0x00498A10
srShader stParticle::GetRenderFlags() const
{
    return render_flags;
}

// FUNCTION: WIZ8 0x004925A0
void stParticle::SetRenderFlags(srShader flags)
{
    render_flags = flags;
}

// FUNCTION: WIZ8 0x0049ADB0
stParticle* FindRegisteredParticle(const char* name)
{
    stParticle* particle = 0;
    char* uppercase_name = static_cast<char*>(malloc(strlen(name) + 1));

    if (uppercase_name != 0) {
        strcpy(uppercase_name, name);
        _strupr(uppercase_name);
        particle = static_cast<stParticle*>(
            srCore.getRegistry()->find(stParticle::sGetClassNode(), uppercase_name, 0));
    }
    free(uppercase_name);
    return particle;
}

// FUNCTION: WIZ8 0x0049AE90
void stParticle::SetParticleScale(float scale)
{
    float inverse_scale = 1.0f / size_scale;
    // Retail uses the extent as its offset, not the midpoint of the bounds.
    srVector3T<float> offset = lifetime_maximum - lifetime_minimum;
    lifetime_minimum = (lifetime_minimum - offset) * inverse_scale + offset;
    lifetime_maximum = (lifetime_maximum - offset) * inverse_scale + offset;
    size_scale = scale;
    lifetime_minimum = (lifetime_minimum - offset) * scale + offset;
    lifetime_maximum = (lifetime_maximum - offset) * scale + offset;
}

// FUNCTION: WIZ8 0x0049B150
void SaveParticleStates(HWFILE handle)
{
    unsigned char version = 1;
    char name[0x80] = "";
    int count = 0;

    FileWrite(handle, &version, sizeof(version), 0);

    stParticle* particle = static_cast<stParticle*>(srCore.getRegistry()->find(
        stParticle::sGetClassNode(), static_cast<const srRuntimeClass*>(0)));
    while (particle != 0) {
        if (particle->persisted) {
            ++count;
        }
        particle = static_cast<stParticle*>(
            srCore.getRegistry()->find(stParticle::sGetClassNode(), particle));
    }

    FileWrite(handle, &count, sizeof(count), 0);

    particle = static_cast<stParticle*>(srCore.getRegistry()->find(
        stParticle::sGetClassNode(), static_cast<const srRuntimeClass*>(0)));
    while (particle != 0) {
        if (particle->persisted) {
            strcpy(name, particle->getName());
            FileWrite(handle, name, sizeof(name), 0);
            FileWrite(handle, &particle->emitting, sizeof(particle->emitting), 0);
        }
        particle = static_cast<stParticle*>(
            srCore.getRegistry()->find(stParticle::sGetClassNode(), particle));
    }
}

// FUNCTION: WIZ8 0x0049B3B0
void LoadParticleStates(int handle)
{
    unsigned char version;
    int count = 0;
    char name[0x80] = "";

    FileRead(handle, &version, sizeof(version), 0);
    FileRead(handle, &count, sizeof(count), 0);

    for (int index = 0; index < count; ++index) {
        unsigned char active;
        FileRead(handle, name, sizeof(name), 0);
        FileRead(handle, &active, sizeof(active), 0);

        stParticle* particle = static_cast<stParticle*>(
            srCore.getRegistry()->find(stParticle::sGetClassNode(), name, 0));
        if (particle != 0) {
            particle->SetActive(active);
        }
    }
}

static void SetParticleQuadTriangles(srVector3i* triangles, unsigned int vertex)
{
    triangles[0].x = vertex;
    triangles[0].y = vertex + 1;
    triangles[0].z = vertex + 2;
    triangles[1].x = vertex + 2;
    triangles[1].y = vertex + 3;
    triangles[1].z = vertex;
}

// FUNCTION: WIZ8 0x00497AF0
stParticle::stParticle(srNode* parent, int count)
    : srClassSupport<stParticle, srNode, 0, 0x10009>(static_cast<srNode*>(0))
{
    persisted = false;
    update_flags = 0;
    attachment_key = -1;
    start_frame = -1;
    end_frame = -1;
    callback = 0;
    size_scale = 1.0f;

    setParent(parent, 1);

    particle_count = count;
    particle_positions = 0;
    texcoords = 0;
    vertex_positions = 0;
    vertex_extras = 0;
    triangles = 0;
    colors = 0;
    texture = 0;
    requires_sorted_renderer = 0;
    particle_size = 1.0;

    if (count == 0) {
        return;
    }

    if (count >= 10000) {
        srAssertFail("cnt < 10000", ST_PARTICLE_CPP, 0x41, 0);
    }

    particle_positions =
        static_cast<srVector3T<float>*>(srHeap.allocate(count * sizeof(srVector3T<float>)));
    /* 0x00497C57 and 0x00498360 test the count parameter signed, while every
       comparison against the stored particle_count (0x00497D6C, 0x00497E74)
       and vertex_count (0x00497EA8) is unsigned. */
    int i;
    for (i = 0; i < count; ++i) {
        particle_positions[i] = 0.0f;
    }

    vertex_count = count * 4;
    texture_frame_count = count * 2;
    texcoords = static_cast<srVector2T<float>*>(
        srHeap.allocate(vertex_count * sizeof(srVector2T<float>)));
    vertex_positions = static_cast<srVector3T<float>*>(
        srHeap.allocate(vertex_count * sizeof(srVector3T<float>)));
    triangles = static_cast<srVector3i*>(srHeap.allocate(count * 2 * sizeof(srVector3i)));
    alphas = new float[vertex_count];
    texture_frames = 0;

    for (i = 0; i < count; ++i) {
        unsigned int vertex = i * 4;
        unsigned int triangle = i * 2;
        SetParticleQuadTriangles(triangles + triangle, vertex);

        particle_positions[i] = 0.0f;

        texcoords[vertex].SetZero();
        texcoords[vertex + 1].Set(1.0f, 0.0f);
        texcoords[vertex + 2].Set(1.0f, 1.0f);
        texcoords[vertex + 3].Set(0.0f, 1.0f);
    }

    for (unsigned int v = 0; v < vertex_count; ++v) {
        alphas[v] = 1.0f;
    }

    emitting = 1;
    traversal_enabled = true;
    material = 0;
    emission_limit = 0;
    release_when_done = false;
    replace_when_full = false;
    emission_count = 0;
    active_triangles = new unsigned long[texture_frame_count];
    active_particle_count = 0;
    velocities =
        static_cast<srVector3T<float>*>(srHeap.allocate(count * sizeof(srVector3T<float>)));
    birth_ticks = new unsigned int[count];
    particle_active = new bool[count];
    memset(particle_active, 0, count);

    has_acceleration = 0;
    expiry_mode = W8_PARTICLE_EXPIRY_TIMED;
    emission_mode = W8_PARTICLE_EMISSION_NONE;
    los_check_enabled = 0;
    direction_mode = W8_PARTICLE_DIRECTION_CONE;
    camera_relative = 0;
    emission_interval = 50;
    lifetime_ms = 1500;
    bounds_mode = W8_PARTICLE_BOUNDS_SPHERE;
    speed_mode = W8_PARTICLE_SPEED_RANDOM;
    emission_minimum = -250.0f;
    emission_maximum = 250.0f;
    direction.Set(0.0f, -1.0f, 0.0f);
    initial_speed = 500.0f;
    acceleration.Set(0.0f, -4905.0f, 0.0f);
    flutter_mode = W8_PARTICLE_FLUTTER_NONE;
    m_pflFlutterAngle = 0;
    flutter_amplitude = 0.0f;
    flutter_period = 0;
    speed_max = 4000.0f;
    cone_yaw = 0.39269906f;
    cone_pitch = 0.39269906f;
    speed_min = 1000.0f;
    lifetime_minimum = -1000.0f;
    lifetime_maximum = 1000.0f;
    bounds_origin.SetZero();
    bounds_radius = 2000.0f;
    update_flags = 0;
    last_integration_tick = g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT);
    last_emission_tick = last_integration_tick;
    emission_gap = 25;
    last_emitted_at = 0;
}

// FUNCTION: WIZ8 0x00498180
stParticle::stParticle(const stParticle& other)
    : srClassSupport<stParticle, srNode, 0, 0x10009>(static_cast<srNode*>(0))
{
    persisted = other.persisted;
    update_flags = 0;
    start_frame = other.start_frame;
    end_frame = other.end_frame;
    emission_gap = other.emission_gap;
    size_scale = other.size_scale;

    /* The copied count is compared signed at 0x00498360 even though the member
       it comes from is stored and compared unsigned. */
    int count = other.particle_count;
    if (count == 0) {
        return;
    }

    setParent(other.getParent(), 1);
    setName(other.getName());
    particle_count = count;
    particle_positions = 0;
    texcoords = 0;
    vertex_positions = 0;
    vertex_extras = 0;
    triangles = 0;
    colors = 0;
    texture = 0;
    requires_sorted_renderer = other.requires_sorted_renderer;
    particle_size = other.particle_size;
    texture_frames = 0;
    material = other.material;
    material->addReference();
    SetRenderFlags(other.GetRenderFlags());

    particle_positions =
        static_cast<srVector3T<float>*>(srHeap.allocate(count * sizeof(srVector3T<float>)));
    if (particle_positions == 0) {
        srAssertFail("pLoc", ST_PARTICLE_CPP, 0xda, 0);
    }
    /* 0x00497C57 and 0x00498360 test the count parameter signed, while every
       comparison against the stored particle_count (0x00497D6C, 0x00497E74)
       and vertex_count (0x00497EA8) is unsigned. */
    int i;
    for (i = 0; i < count; ++i) {
        particle_positions[i] = 0.0f;
    }

    vertex_count = count * 4;
    texture_frame_count = count * 2;
    SetTexture(other.texture);
    texcoords = static_cast<srVector2T<float>*>(
        srHeap.allocate(vertex_count * sizeof(srVector2T<float>)));
    if (texcoords == 0) {
        srAssertFail("vUV", ST_PARTICLE_CPP, 0xe4, 0);
    }
    vertex_positions = static_cast<srVector3T<float>*>(
        srHeap.allocate(vertex_count * sizeof(srVector3T<float>)));
    if (vertex_positions == 0) {
        srAssertFail("vLoc", ST_PARTICLE_CPP, 0xe5, 0);
    }
    triangles =
        static_cast<srVector3i*>(srHeap.allocate(texture_frame_count * sizeof(srVector3i)));
    if (triangles == 0) {
        srAssertFail("pVertex", ST_PARTICLE_CPP, 0xe7, 0);
    }
    alphas = new float[vertex_count];

    for (i = 0; i < count; ++i) {
        unsigned int vertex = i * 4;
        unsigned int triangle = i * 2;
        SetParticleQuadTriangles(triangles + triangle, vertex);

        particle_positions[i] = 0.0f;
        texcoords[vertex].Set(0.0f, 0.0f);
        texcoords[vertex + 1].Set(1.0f, 0.0f);
        texcoords[vertex + 2].Set(1.0f, 1.0f);
        texcoords[vertex + 3].Set(0.0f, 1.0f);
    }
    for (unsigned int v = 0; v < vertex_count; ++v) {
        alphas[v] = 1.0f;
    }

    emission_limit = other.emission_limit;
    emission_count = 0;
    active_particle_count = 0;
    release_when_done = other.release_when_done;
    replace_when_full = other.replace_when_full;
    particle_active = new bool[count];
    memset(particle_active, 0, count);
    velocities =
        static_cast<srVector3T<float>*>(srHeap.allocate(count * sizeof(srVector3T<float>)));
    birth_ticks = new unsigned int[count];
    emitting = other.emitting;
    traversal_enabled = true;
    bounds_mode = other.bounds_mode;
    has_acceleration = other.has_acceleration;
    expiry_mode = other.expiry_mode;
    emission_mode = other.emission_mode;
    los_check_enabled = other.los_check_enabled;
    direction_mode = other.direction_mode;
    speed_mode = other.speed_mode;
    camera_relative = other.camera_relative;
    m_pflFlutterAngle = 0;
    flutter_amplitude = other.flutter_amplitude;
    flutter_period = other.flutter_period;
    SetFlutter(other.flutter_mode);
    emission_interval = other.emission_interval;
    lifetime_ms = other.lifetime_ms;
    emission_minimum = other.emission_minimum;
    emission_maximum = other.emission_maximum;
    direction = other.direction;
    acceleration = other.acceleration;
    cone_yaw = other.cone_yaw;
    cone_pitch = other.cone_pitch;
    initial_speed = other.initial_speed;
    speed_min = other.speed_min;
    speed_max = other.speed_max;
    lifetime_minimum = other.lifetime_minimum;
    lifetime_maximum = other.lifetime_maximum;
    bounds_origin = other.bounds_origin;
    bounds_radius = other.bounds_radius;
    update_flags = W8_PARTICLE_ACTIVE_TRIANGLES_DIRTY;
    active_triangles = new unsigned long[texture_frame_count];
    last_integration_tick = g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT);
    last_emission_tick = last_integration_tick;
    attachment_key = other.attachment_key;
    callback = 0;

    setLocation(other.getLocation());
    srMatrix3T<float> rotation;
    other.getRotation(rotation);
    setRotation(rotation);

    srMatrix4T<float> source_world;
    other.getWorldSpaceMatrix(source_world);
    srMatrix4T<double> world;
    for (i = 0; i < 4; ++i) {
        world.vectors[i].Set(source_world.vectors[i].x, source_world.vectors[i].y,
                             source_world.vectors[i].z, source_world.vectors[i].w);
    }
    setWorldSpaceMatrix(world);
    last_emitted_at = 0;
}

// FUNCTION: WIZ8 0x00499A50
unsigned char stParticle::ActivateParticle(unsigned int* out_index, bool replace_when_full)
{
    if (emission_limit != 0 && emission_count >= emission_limit) {
        return 0;
    }

    unsigned int index;
    for (index = 0; index < particle_count; ++index) {
        if (particle_active[index] == 0) {
            break;
        }
    }

    if (index == particle_count) {
        if (!replace_when_full) {
            return 0;
        }

        unsigned int oldest = 0;
        unsigned int candidate;
        for (candidate = 0; candidate < particle_count; ++candidate) {
            if (birth_ticks[candidate] < birth_ticks[oldest] &&
                particle_active[candidate] != 0) {
                oldest = candidate;
            }
        }
        DeactivateParticle(oldest);
        index = oldest;
    }

    *out_index = index;
    particle_active[index] = 1;
    birth_ticks[index] = g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT);

    float magnitude = 0.0f;
    if (speed_mode == W8_PARTICLE_SPEED_FIXED) {
        magnitude = initial_speed;
    } else if (speed_mode == W8_PARTICLE_SPEED_RANDOM) {
        magnitude =
            (speed_max - speed_min) * (rand() & 0x7fff) * g_float_005ec438 + speed_min;
    }
    magnitude *= size_scale;

    srVector3T<float>& velocity = velocities[index];
    switch (direction_mode) {
    case W8_PARTICLE_DIRECTION_NODE_FORWARD: {
        srVector3T<double> direction = getWorldSpaceDOF();
        velocity = direction * magnitude;
        break;
    }

    case W8_PARTICLE_DIRECTION_FIXED:
        velocity = this->direction * magnitude;
        break;

    case W8_PARTICLE_DIRECTION_CONE: {
        srVector3T<float> direction;
        direction.Set(g_float_zero, g_float_zero, magnitude);

        double angle = ((rand() & 0x7fff) * g_float_005ec438 - g_float_005ebc7c) * cone_pitch;
        direction.RotateAboutX(sin(angle), cos(angle));

        angle = ((rand() & 0x7fff) * g_float_005ec438 - g_float_005ebc7c) * cone_yaw;
        direction.RotateAboutY(sin(angle), cos(angle));

        srMatrix3T<float> rotation;
        getWorldSpaceRotation(rotation);
        velocity = rotation.Transform(direction);
        break;
    }

    case W8_PARTICLE_DIRECTION_RANDOM: {
        srVector3T<float> direction;
        direction.x = (rand() & 0x7fff) * g_float_005ec438 - g_float_005ebc7c;
        direction.y = (rand() & 0x7fff) * g_float_005ec438 - g_float_005ebc7c;
        direction.z = (rand() & 0x7fff) * g_float_005ec438 - g_float_005ebc7c;

        direction.Normalize();

        velocity = direction * magnitude;
        break;
    }

    default:
        velocity = 0.0f;
        break;
    }

    srVector3T<double> location = getLocation();
    particle_positions[index] = location;

    if (m_pflFlutterAngle != 0) {
        m_pflFlutterAngle[index] = (rand() & 0x7fff) * g_float_005ecc40;
    }
    if (texture_frames != 0) {
        texture_frames[index * 2]->SetFrame(0);
    }

    unsigned int vertex = index * 4;
    unsigned int end = vertex + 4;
    for (; vertex < end; ++vertex) {
        alphas[vertex] = 1.0f;
    }

    update_flags |= W8_PARTICLE_ACTIVE_TRIANGLES_DIRTY;
    ++emission_count;
    ++active_particle_count;
    return 1;
}

// FUNCTION: WIZ8 0x00499F70
void stParticle::DeactivateParticle(unsigned int index)
{
    bool* active = particle_active + index;
    if (*active != 0) {
        *active = 0;
        update_flags |= W8_PARTICLE_ACTIVE_TRIANGLES_DIRTY;
        --active_particle_count;
    }
}

// FUNCTION: WIZ8 0x00499FA0
void stParticle::Update()
{
    unsigned int now = g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT);
    if (now - last_emitted_at < emission_gap) {
        return;
    }

    last_emitted_at = now;
    if (active_particle_count != 0) {
        unsigned int elapsed_ticks = now - last_integration_tick;

        srMatrix3T<float> rotation;
        getRotation(rotation);

        srMatrix4T<float> transform;
        transform.vectors[0].Set(rotation.vectors[0].x, rotation.vectors[0].y,
                                 rotation.vectors[0].z, 0.0f);
        transform.vectors[1].Set(rotation.vectors[1].x, rotation.vectors[1].y,
                                 rotation.vectors[1].z, 0.0f);
        transform.vectors[2].Set(rotation.vectors[2].x, rotation.vectors[2].y,
                                 rotation.vectors[2].z, 0.0f);
        transform.vectors[3].Set(0.0f, 0.0f, 0.0f, 1.0f);

        transform.Invert();

        srVector3T<float> node_location;
        getLocation(node_location);

        double elapsed = static_cast<double>(elapsed_ticks);
        srVector3T<float> acceleration_step = this->acceleration * (elapsed * g_double_005ec8d0);

        unsigned int index;
        for (index = 0; index < particle_count; ++index) {
            if (particle_active[index] == 0) {
                continue;
            }

            unsigned int vertex = index * 4;
            if (expiry_mode == W8_PARTICLE_EXPIRY_TIMED) {
                unsigned int expires_at = birth_ticks[index] + lifetime_ms;
                if (expires_at < now) {
                    particle_active[index] = 0;
                    update_flags |= W8_PARTICLE_ACTIVE_TRIANGLES_DIRTY;
                    --active_particle_count;
                    continue;
                }
                if (expires_at - 500 < now) {
                    float alpha = (expires_at - now) * g_float_005ebc60;
                    unsigned int alpha_end = vertex + 4;
                    unsigned int alpha_index;
                    for (alpha_index = vertex; alpha_index < alpha_end; ++alpha_index) {
                        alphas[alpha_index] = alpha;
                    }
                }
            } else if (expiry_mode == W8_PARTICLE_EXPIRY_TEXTURE) {
                if (texture_frames == 0) {
                    expiry_mode = W8_PARTICLE_EXPIRY_TIMED;
                    if (lifetime_ms == 0) {
                        lifetime_ms = 1000;
                    }
                } else {
                    stTextureAnim* animation = texture_frames[index * 2];
                    animation->UpdateFrame();
                    if (animation->IsFinished() != 0) {
                        particle_active[index] = 0;
                        update_flags |= W8_PARTICLE_ACTIVE_TRIANGLES_DIRTY;
                        --active_particle_count;
                        continue;
                    }
                }
            }

            if (has_acceleration == 1) {
                velocities[index] += acceleration_step;
            }

            srVector3T<float> movement = velocities[index] * (elapsed * g_double_005ec8d0);

            srVector3T<float> candidate;
            candidate = particle_positions[index] + movement;

            if (bounds_mode == W8_PARTICLE_BOUNDS_SPHERE) {
                double distance;
                if (bounds_origin.x == g_float_zero &&
                    bounds_origin.y == g_float_zero &&
                    bounds_origin.z == g_float_zero) {
                    distance = (candidate - node_location).Length();
                } else {
                    srVector3T<float> center =
                        rotation.Transform(bounds_origin) + node_location;
                    srVector3T<float> difference = candidate - center;
                    distance = difference.Length();
                }

                if (size_scale * bounds_radius < distance) {
                    particle_active[index] = 0;
                    update_flags |= W8_PARTICLE_ACTIVE_TRIANGLES_DIRTY;
                    --active_particle_count;
                    continue;
                }
            } else if (bounds_mode == W8_PARTICLE_BOUNDS_BOX) {
                srVector3T<float> local = candidate - node_location;
                srVector4T<float> transformed = transform.Transform(local);
                srVector3T<float> local_point;
                local_point = transformed.xyz();
                if (!PointInsideBounds(&local_point, &lifetime_minimum, &lifetime_maximum)) {
                    particle_active[index] = 0;
                    update_flags |= W8_PARTICLE_ACTIVE_TRIANGLES_DIRTY;
                    --active_particle_count;
                    continue;
                }
            }

            if (los_check_enabled == 1 &&
                (g_world->octree == 0 ||
                 !g_world->octree->HasLineOfSight(&particle_positions[index], &candidate, true))) {
                particle_active[index] = 0;
                update_flags |= W8_PARTICLE_ACTIVE_TRIANGLES_DIRTY;
                --active_particle_count;
                continue;
            }

            particle_positions[index] = candidate;
        }
    }

    last_integration_tick = now;

    if (emitting == 0 || emission_mode == W8_PARTICLE_EMISSION_NONE) {
        return;
    }

    unsigned int emission_elapsed = now - last_emission_tick;
    if (emission_elapsed < emission_interval) {
        return;
    }

    if (emission_mode == W8_PARTICLE_EMISSION_SINGLE) {
        unsigned int particle_index;
        ActivateParticle(&particle_index, replace_when_full);
        last_emission_tick = now;
        return;
    }
    if (emission_mode != W8_PARTICLE_EMISSION_CATCH_UP ||
        emission_elapsed <= emission_interval) {
        return;
    }

    for (;;) {
        unsigned int lag = now - emission_interval - last_emission_tick;
        unsigned int particle_index;
        if (ActivateParticle(&particle_index, replace_when_full) == 0) {
            last_emission_tick = now;
            return;
        }

        if (has_acceleration == 1) {
            srVector3T<float> acceleration = (this->acceleration * static_cast<double>(lag)) / 1000.0;
            velocities[particle_index] += acceleration;
        }

        srVector3T<float> displacement = velocities[particle_index];
        displacement *= static_cast<double>(lag);
        displacement /= 1000.0;
        InitializeParticlePosition(&particle_positions[particle_index]);
        particle_positions[particle_index] += displacement;

        last_emission_tick += emission_interval;
        if (now - last_emission_tick <= emission_interval) {
            return;
        }
    }
}

// FUNCTION: WIZ8 0x0049A990
void stParticle::InitializeParticlePosition(srVector3T<float>* output)
{
    output->x = (emission_maximum.x - emission_minimum.x) * (rand() & 0x7fff) * g_float_005ec438 +
                emission_minimum.x;
    output->y = (emission_maximum.y - emission_minimum.y) * (rand() & 0x7fff) * g_float_005ec438 +
                emission_minimum.y;
    output->z = (emission_maximum.z - emission_minimum.z) * (rand() & 0x7fff) * g_float_005ec438 +
                emission_minimum.z;

    *output *= size_scale;

    srMatrix3T<float> rotation;
    getRotation(rotation);
    output->Transform(rotation);

    srVector3T<double> location = getLocation();
    output->x += static_cast<float>(location.x);
    output->y += static_cast<float>(location.y);
    output->z += static_cast<float>(location.z);
}

// FUNCTION: WIZ8 0x004980E0
srClass* stParticle::vInstance()
{
    return new stParticle(0, 0);
}

// FUNCTION: WIZ8 0x00498C40
void stParticle::traverse(srNode::TraverseInfo& info)
{
    if (next_sibling_ != 0) {
        next_sibling_->traverse(info);
    }

    if (!testFlag(FLAG_DISABLE)) {
        if ((emitting != 0 || active_particle_count != 0) && traversal_enabled) {
            srNode::TraverseInfo::Entry& entry = info.entries[info.entry_count];
            entry.node = this;
            entry.value = 0;
            ++info.entry_count;
        }
    }

    if (!testFlag(FLAG_TERMINATE) && first_child_ != 0) {
        first_child_->traverse(info);
    }
}

/* Traversal is gated separately from particle activity.  Starting a new
   enabled interval resets the update timestamp; repeated enables do not. */
// FUNCTION: WIZ8 0x00498D90
void stParticle::SetTraversalEnabled(bool enabled)
{
    if (enabled && !traversal_enabled) {
        last_emission_tick = g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT);
    }
    traversal_enabled = enabled;
}

// FUNCTION: WIZ8 0x00498D60
void stParticle::process(const ProcessInfo& info, e_processType)
{
    info.renderer->pushMatrix();
    SubmitToRenderer(info.renderer);
    info.renderer->popMatrix();
}

/* Build the four camera-facing offsets once per call, then expand every
   particle center into a quad. */
// FUNCTION: WIZ8 0x00498DD0
void stParticle::PrepareRenderer(srMatrix4T<float>& view)
{
    static srVector3T<float> corners[4] = {
        srVector3T<float>(-0.5f, 0.5f, 0.0f), srVector3T<float>(0.5f, 0.5f, 0.0f),
        srVector3T<float>(0.5f, -0.5f, 0.0f), srVector3T<float>(-0.5f, -0.5f, 0.0f)};
    static srVector3T<float> offsets[4];

    view.vectors[0].w = 0.0f;
    view.vectors[1].w = 0.0f;
    view.vectors[2].w = 0.0f;

    float normalization = static_cast<float>(g_double_005ebc30 / view.vectors[0].Length());
    view.vectors[0] *= normalization;
    view.vectors[1] *= normalization;
    view.vectors[2] *= normalization;

    for (unsigned int index = 0; index < 4; ++index) {
        srVector4T<float> transformed = view.Transform(corners[index]);

        float scale = static_cast<float>(particle_size) * size_scale;
        offsets[index] = srVector3T<float>(transformed.x, transformed.y, transformed.z) *
                         static_cast<double>(scale);
    }

    if (flutter_mode == W8_PARTICLE_FLUTTER_NONE) {
        for (unsigned int direct_index = 0; direct_index < particle_count; ++direct_index) {
            unsigned int vertex = direct_index * 4;
            const srVector3T<float>& position = particle_positions[direct_index];
            vertex_positions[vertex] = position + offsets[0];
            vertex_positions[vertex + 1] = position + offsets[1];
            vertex_positions[vertex + 2] = position + offsets[2];
            vertex_positions[vertex + 3] = position + offsets[3];
        }
        return;
    }

    float phase = g_float_zero;
    if (flutter_period != 0) {
        phase = static_cast<float>(g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT) %
                                   flutter_period) /
                static_cast<int>(flutter_period) * g_camera_angle_period;
    }
    float flutter = static_cast<float>(sin(phase)) * flutter_amplitude * size_scale;

    for (unsigned int particle_index = 0; particle_index < particle_count; ++particle_index) {
        srVector3T<float> position;

        if (velocities[particle_index].y >= g_float_zero) {
            position = particle_positions[particle_index];
        } else {
            position.Set(flutter, 0.0f, 0.0f);

            if (flutter_mode == W8_PARTICLE_FLUTTER_VELOCITY_SCALED) {
                float scale = g_float_005ecc3c;
                if (g_float_005ecc3c < velocities[particle_index].y) {
                    scale = velocities[particle_index].y;
                }
                position.x = scale * g_float_005ecc38 * flutter;
            }

            double angle = m_pflFlutterAngle[particle_index];
            position.RotateAboutY(sin(angle), cos(angle));
            position += particle_positions[particle_index];
        }

        unsigned int vertex = particle_index * 4;
        vertex_positions[vertex] = position + offsets[0];
        vertex_positions[vertex + 1] = position + offsets[1];
        vertex_positions[vertex + 2] = position + offsets[2];
        vertex_positions[vertex + 3] = position + offsets[3];
    }
}

/* One particle system's complete submission. The system is placed (either
   relative to the camera or at its own node location), aged, stopped after
   its configured total activation count, culled against the renderer, and
   finally handed to the shared triangle-mesh pipeline as a single slot.

   The retired path is the only one that can drop the system: a particle whose
   activity has run out notifies its shake callback and, when release_when_done marks
   it as self-owned, releases itself. */
// FUNCTION: WIZ8 0x004994D0
void stParticle::SubmitToRenderer(srGERD* renderer)
{
    srVector3T<float> position;

    if (camera_relative == 1) {
        srVector3T<float> camera_position;
        GetCameraPosition(&camera_position);
        position = camera_position + camera_offset;

        srVector3T<double> placed;
        placed.SetFromFloat(&position);
        setLocation(placed);
    } else {
        /* Bound rather than copied: the three conversions read through the
           returned buffer instead of through a named local's own address. */
        const srVector3T<double>& located = getLocation();
        position = located;
    }

    Update();

    if (emission_limit != 0 && emission_count >= emission_limit) {
        emitting = 0;
    }

    if (emitting == 0 && active_particle_count == 0) {
        if (callback != 0) {
            callback->RestoreAnimation();
        }
        if (release_when_done) {
            release();
        }
        return;
    }

    srMatrix3T<float> rotation;

    if (bounds_mode == W8_PARTICLE_BOUNDS_SPHERE) {
        srGERD::e_visibility visibility;

        const srVector3T<float>& extent = bounds_origin;

        if (extent.x == g_float_zero && extent.y == g_float_zero &&
            extent.z == g_float_zero) {
            visibility = renderer->testBoundingSphere(position, size_scale * bounds_radius);
        } else {
            getRotation(rotation);
            srVector3T<float> center = rotation.Transform(extent) + position;
            visibility = renderer->testBoundingSphere(center, size_scale * bounds_radius);
        }
        if (visibility == srGERD::VISIBILITY_OUTSIDE) {
            return;
        }
    }

    if (bounds_mode == W8_PARTICLE_BOUNDS_BOX) {
        getRotation(rotation);
        srVector3T<float> minimum = rotation.Transform(lifetime_minimum) + position;
        srVector3T<float> maximum = rotation.Transform(lifetime_maximum) + position;

        srGERD::e_visibility visibility = renderer->testBoundingBox(minimum, maximum);
        if (visibility == srGERD::VISIBILITY_OUTSIDE) {
            return;
        }
    }

    /* Two indices per surviving particle, rebuilt only after a deactivation
       has marked the pairs stale. */
    if ((update_flags & W8_PARTICLE_ACTIVE_TRIANGLES_DIRTY) != 0) {
        unsigned int written = 0;
        for (unsigned int index = 0; index < particle_count; ++index) {
            if (particle_active[index] != 0) {
                active_triangles[written++] = index * 2;
                active_triangles[written++] = index * 2 + 1;
            }
        }
        update_flags &= ~static_cast<unsigned int>(W8_PARTICLE_ACTIVE_TRIANGLES_DIRTY);
    }

    renderer->pushEnable();
    renderer->matrixMode(srGERD::MATRIX_MODELVIEW);

    srMatrix4T<float> view;
    renderer->getMatrix(srGERD::MATRIX_MODELVIEW, view);
    view.Invert();
    PrepareRenderer(view);

    if (requires_sorted_renderer != 0 &&
        !renderer->isEnabled(srGERD::ENABLE_SORTED_RENDERING)) {
        renderer->toggle(srGERD::ENABLE_SORTED_RENDERING);
    }
    renderer->setCullMode(srGERD::CULL_NONE);
    renderer->setPickKey(0);

    srTriMeshPipeline* pipeline = srTriMeshPipeline::Get(renderer);

    /* Three array/count pairs: the index pairs rebuilt above, the polygon
       index list, and the transformed vertex positions. */
    pipeline->active_triangles = active_triangles;
    pipeline->active_triangle_count = active_particle_count * 2;
    pipeline->triangles = triangles;
    pipeline->triangle_count = texture_frame_count;
    pipeline->positions = vertex_positions;
    pipeline->vertex_count = vertex_count;
    if (vertex_extras != 0) {
        pipeline->vertex_extras = vertex_extras;
    }

    pipeline->current_record->flags = 0;
    pipeline->current_pass->shaders = 0;
    pipeline->current_pass->tex_table_0 = 0;
    pipeline->current_pass->tex_table_1 = 0;

    if (colors != 0) {
        pipeline->current_record->colors = colors;
        pipeline->current_record->color_format = srVertexPipe::Record::ColorSource::FORMAT_VECTOR3;
        pipeline->current_record->flags |= srVertexPipe::Record::HAS_COLORS;
    }
    if (alphas != 0) {
        pipeline->current_record->alphas = alphas;
        pipeline->current_record->flags |= srVertexPipe::Record::HAS_ALPHA;
    }

    /* The particle material reaches
       both the pipeline and the record it is about to submit. */
    pipeline->material = material;
    pipeline->current_record->material = material;

    pipeline->SetFlags(render_flags);

    if (texcoords != 0) {
        pipeline->current_record->st0 = texcoords;
        pipeline->current_record->flags |= srVertexPipe::Record::HAS_TEXCOORD0;
    }

    if (texture_frames != 0) {
        pipeline->current_pass->tex_table_0 = texture_frames;
    } else {
        srTextureIFace* texture = this->texture;

        if (texture != 0) {
            pipeline->texture0 = texture;
            pipeline->current_pass->texture0 = texture;
        }
    }

    ++pipeline->slot_count;
    pipeline->PrepareSlot();

    if (!renderer->isPickStackEmpty()) {
        srGERD::Pick pick;

        renderer->popPick(pick);
        pipeline->FlushIfCurrent();
        renderer->pushPick(pick);
    } else {
        pipeline->FlushIfCurrent();
    }
    renderer->popEnable();
}

// FUNCTION: WIZ8 0x00498A20
stParticle::~stParticle()
{
    if (particle_positions != 0) {
        srHeap.free(particle_positions);
    }
    if (vertex_extras != 0) {
        srHeap.free(vertex_extras);
    }
    if (texcoords != 0) {
        srHeap.free(texcoords);
    }
    if (vertex_positions != 0) {
        srHeap.free(vertex_positions);
    }
    if (triangles != 0) {
        srHeap.free(triangles);
    }
    if (colors != 0) {
        srHeap.free(colors);
    }
    if (alphas != 0) {
        delete[] alphas;
    }
    if (material != 0) {
        material->release();
    }
    if (velocities != 0) {
        srHeap.free(velocities);
    }
    if (birth_ticks != 0) {
        delete[] birth_ticks;
    }
    if (active_triangles != 0) {
        delete[] active_triangles;
    }
    if (particle_active != 0) {
        delete[] particle_active;
    }
    if (texture_frames != 0) {
        for (unsigned int i = 0; i < texture_frame_count; i += 2) {
            texture_frames[i]->release();
        }
        delete[] texture_frames;
    }
    if (m_pflFlutterAngle != 0) {
        delete[] m_pflFlutterAngle;
    }
    texture->release();
    setParent(0, 1);
}

// FUNCTION: WIZ8 0x0049AB00
void stParticle::SetTexture(srTextureIFace* texture)
{
    unsigned int i;

    if (this->texture != 0) {
        if (texture_frames != 0) {
            for (i = 0; i < texture_frame_count; i += 2) {
                texture_frames[i]->release();
            }
            delete[] texture_frames;
            texture_frames = 0;
        }
        this->texture->release();
    }

    if (texture != 0 && texture->getClassID() == stTextureAnim::CLASS_ID) {
        texture_frames = new stTextureAnim*[texture_frame_count];
        for (i = 0; i < texture_frame_count; i += 2) {
            stTextureAnim* frame = new stTextureAnim(*static_cast<stTextureAnim*>(texture));
            texture_frames[i] = frame;
            texture_frames[i + 1] = frame;
        }
    }

    this->texture = texture;
    texture->addReference();
}

// FUNCTION: WIZ8 0x0049acd0
void stParticle::SetActive(unsigned char active)
{
    if (active != 0 && emitting == 0) {
        unsigned int now = g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT);
        last_integration_tick = now;
        last_emission_tick = now;
    }
    emitting = active;
}

// FUNCTION: WIZ8 0x0049ac30
unsigned char stParticle::ReplaceTexture(const char* old_name, srTextureIFace* replacement)
{
    if (texture != 0 &&
        (texture->getClassID() == stTextureFile::CLASS_ID ||
         texture->getClassID() == stTextureAnim::CLASS_ID) &&
        _stricmp(texture->getName(), old_name) == 0) {
        SetTexture(replacement);
        return 1;
    }
    return 0;
}

// FUNCTION: WIZ8 0x0049ACA0
void stParticle::SetMaterial(srMaterialIFace* new_material)
{
    if (material != 0) {
        material->release();
    }
    material = new_material;
    if (new_material != 0) {
        new_material->addReference();
    }
}

// FUNCTION: WIZ8 0x0049AD10
void stParticle::SetFlutter(W8ParticleFlutterMode mode)
{
    unsigned int i;

    flutter_mode = mode;
    if (mode == W8_PARTICLE_FLUTTER_NONE) {
        if (m_pflFlutterAngle != 0) {
            delete[] m_pflFlutterAngle;
            m_pflFlutterAngle = 0;
        }
    } else if (m_pflFlutterAngle == 0) {
        m_pflFlutterAngle = new float[particle_count];
        if (m_pflFlutterAngle == 0) {
            srAssertFail("m_pflFlutterAngle", ST_PARTICLE_CPP, 1210, 0);
        }
        for (i = 0; i < particle_count; ++i) {
            m_pflFlutterAngle[i] = 0.0f;
        }
    }
}
