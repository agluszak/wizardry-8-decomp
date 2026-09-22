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
float g_float_005ebc60 = 0.0020000000949949026f;
// GLOBAL: WIZ8 0x005ec438
float g_float_005ec438 = 3.0517578125e-05f;
// GLOBAL: WIZ8 0x005ec8d0
double g_double_005ec8d0 = 0.001;
// GLOBAL: WIZ8 0x005ecc38
float g_float_005ecc38 = -0.0010000000474974513f;
// GLOBAL: WIZ8 0x005ecc3c
float g_float_005ecc3c = -1000.0f;
// GLOBAL: WIZ8 0x005ecc40
float g_float_005ecc40 = 0.00019174758926965296f;

// GLOBAL: WIZ8 0x0060BF6C
static const char ST_PARTICLE_CPP[] = "C:\\Projects\\Wizardry 8\\Engine Code\\stParticle.cpp";

// VTABLE: WIZ8 0x005ECBD0
// class stParticle

// VTABLE: WIZ8 0x005ECC04
// class srClassSupport<stParticle,srNode,0,65545>

// TEMPLATE: WIZ8 0x0049B540
// srClassSupport<stParticle,srNode,0,65545>::getClassID

// TEMPLATE: WIZ8 0x0049B550
// srClassSupport<stParticle,srNode,0,65545>::getClassName

// TEMPLATE: WIZ8 0x0049B560
// srClassSupport<stParticle,srNode,0,65545>::getClassNode

// TEMPLATE: WIZ8 0x0049B5D0
// srClassSupport<stParticle,srNode,0,65545>::clone

// TEMPLATE: WIZ8 0x0049B990
// srClassSupport<stParticle,srNode,0,65545>::~srClassSupport<stParticle,srNode,0,65545>

// SYNTHETIC: WIZ8 0x0049BA50
// srClassSupport<stParticle,srNode,0,65545>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x00498150
// stParticle::`scalar deleting destructor'

/* Return the renderer flags as a value. VC6 lowers the four-byte class return
   through its hidden result pointer. */
// FUNCTION: WIZ8 0x00498A10
srShader stParticle::GetRenderFlags00498A10() const
{
    return render_flags_150;
}

// FUNCTION: WIZ8 0x004925A0
void stParticle::SetRenderFlags004925A0(srShader flags)
{
    render_flags_150 = flags;
}

// FUNCTION: WIZ8 0x0049ADB0
stParticle* FindRegisteredParticle0049ADB0(const char* name)
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
    float inverse_scale = 1.0f / size_scale_278;
    // Retail uses the extent as its offset, not the midpoint of the bounds.
    srVector3T<float> offset = maximum_228 - minimum_21c;
    minimum_21c = (minimum_21c - offset) * inverse_scale + offset;
    maximum_228 = (maximum_228 - offset) * inverse_scale + offset;
    size_scale_278 = scale;
    minimum_21c = (minimum_21c - offset) * scale + offset;
    maximum_228 = (maximum_228 - offset) * scale + offset;
}

// FUNCTION: WIZ8 0x0049B150
void SaveParticleStates0049B150(HWFILE handle)
{
    unsigned char version = 1;
    char name[0x80] = "";
    int count = 0;

    FileWrite(handle, &version, sizeof(version), 0);

    stParticle* particle = static_cast<stParticle*>(srCore.getRegistry()->find(
        stParticle::sGetClassNode(), static_cast<const srRuntimeClass*>(0)));
    while (particle != 0) {
        if (particle->persisted_192 != 0) {
            ++count;
        }
        particle = static_cast<stParticle*>(
            srCore.getRegistry()->find(stParticle::sGetClassNode(), particle));
    }

    FileWrite(handle, &count, sizeof(count), 0);

    particle = static_cast<stParticle*>(srCore.getRegistry()->find(
        stParticle::sGetClassNode(), static_cast<const srRuntimeClass*>(0)));
    while (particle != 0) {
        if (particle->persisted_192 != 0) {
            strcpy(name, particle->getName());
            FileWrite(handle, name, sizeof(name), 0);
            FileWrite(handle, &particle->emitting_1a0, sizeof(particle->emitting_1a0), 0);
        }
        particle = static_cast<stParticle*>(
            srCore.getRegistry()->find(stParticle::sGetClassNode(), particle));
    }
}

// FUNCTION: WIZ8 0x0049B3B0
void LoadParticleStates0049B3B0(int handle)
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

// FUNCTION: WIZ8 0x00497AF0
stParticle::stParticle(srNode* parent, unsigned int count)
    : srClassSupport<stParticle, srNode, 0, 0x10009>(static_cast<srNode*>(0))
{
    persisted_192 = 0;
    update_flags_250 = 0;
    attachment_key_260 = -1;
    start_frame_264 = -1;
    end_frame_268 = -1;
    callback_26c = 0;
    size_scale_278 = 1.0f;

    setParent(parent, 1);

    particle_count_180 = count;
    particle_positions_148 = 0;
    texcoords_164 = 0;
    vertex_positions_160 = 0;
    vertex_extras_170 = 0;
    triangles_168 = 0;
    colors_16c = 0;
    texture_154 = 0;
    requires_positional_138 = 0;
    particle_size_140 = 1.0;

    if (count == 0) {
        return;
    }

    if (count >= 10000) {
        srAssertFail("cnt < 10000", ST_PARTICLE_CPP, 0x41, 0);
    }

    particle_positions_148 =
        static_cast<srVector3T<float>*>(srHeap.allocate(count * sizeof(srVector3T<float>)));
    unsigned int i;
    for (i = 0; i < count; ++i) {
        particle_positions_148[i] = 0.0f;
    }

    vertex_count_158 = count * 4;
    texture_frame_count_15c = count * 2;
    texcoords_164 = static_cast<srVector2T<float>*>(
        srHeap.allocate(vertex_count_158 * sizeof(srVector2T<float>)));
    vertex_positions_160 = static_cast<srVector3T<float>*>(
        srHeap.allocate(vertex_count_158 * sizeof(srVector3T<float>)));
    triangles_168 = static_cast<srVector3i*>(srHeap.allocate(count * 2 * sizeof(srVector3i)));
    alphas_174 = new float[vertex_count_158];
    texture_frames_178 = 0;

    for (i = 0; i < count; ++i) {
        unsigned int vertex = i * 4;
        unsigned int triangle = i * 2;
        triangles_168[triangle].x = vertex;
        triangles_168[triangle].y = vertex + 1;
        triangles_168[triangle].z = vertex + 2;
        triangles_168[triangle + 1].x = vertex + 2;
        triangles_168[triangle + 1].y = vertex + 3;
        triangles_168[triangle + 1].z = vertex;

        particle_positions_148[i] = 0.0f;

        texcoords_164[vertex].x = 0.0f;
        texcoords_164[vertex].y = 0.0f;
        texcoords_164[vertex + 1].x = 1.0f;
        texcoords_164[vertex + 1].y = 0.0f;
        texcoords_164[vertex + 2].x = 1.0f;
        texcoords_164[vertex + 2].y = 1.0f;
        texcoords_164[vertex + 3].x = 0.0f;
        texcoords_164[vertex + 3].y = 1.0f;
    }

    for (i = 0; i < vertex_count_158; ++i) {
        alphas_174[i] = 1.0f;
    }

    emitting_1a0 = 1;
    traversal_enabled_1a1 = 1;
    retained_14c = 0;
    emission_limit_184 = 0;
    release_when_done_190 = false;
    replace_when_full_191 = 0;
    emission_count_188 = 0;
    active_triangles_254 = new unsigned long[texture_frame_count_15c];
    active_particle_count_18c = 0;
    velocities_198 =
        static_cast<srVector3T<float>*>(srHeap.allocate(count * sizeof(srVector3T<float>)));
    birth_ticks_19c = new unsigned int[count];
    particle_active_194 = new unsigned char[count];
    memset(particle_active_194, 0, count);

    has_acceleration_1a8 = 0;
    expiry_mode_1ac = 0;
    emission_mode_1b0 = 0;
    los_check_enabled_1b4 = 0;
    direction_mode_1b8 = 3;
    camera_relative_1c4 = 0;
    emission_interval_1c8 = 50;
    lifetime_ms_1cc = 1500;
    bounds_mode_1a4 = 2;
    placement_mode_1bc = 2;
    minimum_1d0 = -250.0f;
    maximum_1dc = 250.0f;
    direction_1e8.Set(0.0f, -1.0f, 0.0f);
    initial_speed_210 = 500.0f;
    acceleration_1f4.Set(0.0f, -4905.0f, 0.0f);
    flutter_mode_1c0 = 0;
    m_pflFlutterAngle = 0;
    flutter_amplitude_200 = 0.0f;
    flutter_period_204 = 0;
    speed_max_218 = 4000.0f;
    cone_yaw_208 = 0.39269906f;
    cone_pitch_20c = 0.39269906f;
    speed_min_214 = 1000.0f;
    minimum_21c = -1000.0f;
    maximum_228 = 1000.0f;
    bounds_origin_234.SetZero();
    bounds_radius_240 = 2000.0f;
    update_flags_250 = 0;
    activated_at_258 = g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT);
    updated_at_25c = activated_at_258;
    emission_gap_270 = 25;
    last_emitted_at_274 = 0;
}

// FUNCTION: WIZ8 0x00498180
stParticle::stParticle(const stParticle& other)
    : srClassSupport<stParticle, srNode, 0, 0x10009>(static_cast<srNode*>(0))
{
    persisted_192 = other.persisted_192;
    update_flags_250 = 0;
    start_frame_264 = other.start_frame_264;
    end_frame_268 = other.end_frame_268;
    emission_gap_270 = other.emission_gap_270;
    size_scale_278 = other.size_scale_278;

    unsigned int count = other.particle_count_180;
    if (count == 0) {
        return;
    }

    setParent(other.getParent(), 1);
    setName(other.getName());
    particle_count_180 = count;
    particle_positions_148 = 0;
    texcoords_164 = 0;
    vertex_positions_160 = 0;
    vertex_extras_170 = 0;
    triangles_168 = 0;
    colors_16c = 0;
    texture_154 = 0;
    requires_positional_138 = other.requires_positional_138;
    particle_size_140 = other.particle_size_140;
    texture_frames_178 = 0;
    retained_14c = other.retained_14c;
    retained_14c->addReference();
    SetRenderFlags004925A0(other.GetRenderFlags00498A10());

    particle_positions_148 =
        static_cast<srVector3T<float>*>(srHeap.allocate(count * sizeof(srVector3T<float>)));
    if (particle_positions_148 == 0) {
        srAssertFail("pParticle", ST_PARTICLE_CPP, 0xda, 0);
    }
    unsigned int i;
    for (i = 0; i < count; ++i) {
        particle_positions_148[i] = 0.0f;
    }

    vertex_count_158 = count * 4;
    texture_frame_count_15c = count * 2;
    SetTexture0049AB00(other.texture_154);
    texcoords_164 = static_cast<srVector2T<float>*>(
        srHeap.allocate(vertex_count_158 * sizeof(srVector2T<float>)));
    if (texcoords_164 == 0) {
        srAssertFail("pTexCoord", ST_PARTICLE_CPP, 0xe4, 0);
    }
    vertex_positions_160 = static_cast<srVector3T<float>*>(
        srHeap.allocate(vertex_count_158 * sizeof(srVector3T<float>)));
    if (vertex_positions_160 == 0) {
        srAssertFail("pVertex", ST_PARTICLE_CPP, 0xe5, 0);
    }
    triangles_168 =
        static_cast<srVector3i*>(srHeap.allocate(texture_frame_count_15c * sizeof(srVector3i)));
    if (triangles_168 == 0) {
        srAssertFail("pVertex", ST_PARTICLE_CPP, 0xe7, 0);
    }
    alphas_174 = new float[vertex_count_158];

    for (i = 0; i < count; ++i) {
        unsigned int vertex = i * 4;
        unsigned int triangle = i * 2;
        triangles_168[triangle].x = vertex;
        triangles_168[triangle].y = vertex + 1;
        triangles_168[triangle].z = vertex + 2;
        triangles_168[triangle + 1].x = vertex + 2;
        triangles_168[triangle + 1].y = vertex + 3;
        triangles_168[triangle + 1].z = vertex;

        particle_positions_148[i] = 0.0f;
        texcoords_164[vertex].Set(0.0f, 0.0f);
        texcoords_164[vertex + 1].Set(1.0f, 0.0f);
        texcoords_164[vertex + 2].Set(1.0f, 1.0f);
        texcoords_164[vertex + 3].Set(0.0f, 1.0f);
    }
    for (i = 0; i < vertex_count_158; ++i) {
        alphas_174[i] = 1.0f;
    }

    emission_limit_184 = other.emission_limit_184;
    emission_count_188 = 0;
    active_particle_count_18c = 0;
    release_when_done_190 = other.release_when_done_190;
    replace_when_full_191 = other.replace_when_full_191;
    particle_active_194 = new unsigned char[count];
    memset(particle_active_194, 0, count);
    velocities_198 =
        static_cast<srVector3T<float>*>(srHeap.allocate(count * sizeof(srVector3T<float>)));
    birth_ticks_19c = new unsigned int[count];
    emitting_1a0 = other.emitting_1a0;
    traversal_enabled_1a1 = 1;
    bounds_mode_1a4 = other.bounds_mode_1a4;
    has_acceleration_1a8 = other.has_acceleration_1a8;
    expiry_mode_1ac = other.expiry_mode_1ac;
    emission_mode_1b0 = other.emission_mode_1b0;
    los_check_enabled_1b4 = other.los_check_enabled_1b4;
    direction_mode_1b8 = other.direction_mode_1b8;
    placement_mode_1bc = other.placement_mode_1bc;
    camera_relative_1c4 = other.camera_relative_1c4;
    m_pflFlutterAngle = 0;
    flutter_amplitude_200 = other.flutter_amplitude_200;
    flutter_period_204 = other.flutter_period_204;
    SetFlutter0049AD10(other.flutter_mode_1c0);
    emission_interval_1c8 = other.emission_interval_1c8;
    lifetime_ms_1cc = other.lifetime_ms_1cc;
    minimum_1d0 = other.minimum_1d0;
    maximum_1dc = other.maximum_1dc;
    direction_1e8 = other.direction_1e8;
    acceleration_1f4 = other.acceleration_1f4;
    cone_yaw_208 = other.cone_yaw_208;
    cone_pitch_20c = other.cone_pitch_20c;
    initial_speed_210 = other.initial_speed_210;
    speed_min_214 = other.speed_min_214;
    speed_max_218 = other.speed_max_218;
    minimum_21c = other.minimum_21c;
    maximum_228 = other.maximum_228;
    bounds_origin_234 = other.bounds_origin_234;
    bounds_radius_240 = other.bounds_radius_240;
    update_flags_250 = 2;
    active_triangles_254 = new unsigned long[texture_frame_count_15c];
    activated_at_258 = g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT);
    updated_at_25c = activated_at_258;
    attachment_key_260 = other.attachment_key_260;
    callback_26c = 0;

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
    last_emitted_at_274 = 0;
}

// FUNCTION: WIZ8 0x00499A50
unsigned char stParticle::ActivateParticle00499A50(unsigned int* out_index,
                                                   unsigned char replace_when_full)
{
    if (emission_limit_184 != 0 && emission_count_188 >= emission_limit_184) {
        return 0;
    }

    unsigned int index;
    for (index = 0; index < particle_count_180; ++index) {
        if (particle_active_194[index] == 0) {
            break;
        }
    }

    if (index == particle_count_180) {
        if (replace_when_full == 0) {
            return 0;
        }

        unsigned int oldest = 0;
        unsigned int candidate;
        for (candidate = 0; candidate < particle_count_180; ++candidate) {
            if (birth_ticks_19c[candidate] < birth_ticks_19c[oldest] &&
                particle_active_194[candidate] != 0) {
                oldest = candidate;
            }
        }
        DeactivateParticle00499F70(oldest);
        index = oldest;
    }

    *out_index = index;
    particle_active_194[index] = 1;
    birth_ticks_19c[index] = g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT);

    float magnitude = 0.0f;
    if (placement_mode_1bc == 1) {
        magnitude = initial_speed_210;
    } else if (placement_mode_1bc == 2) {
        magnitude =
            (speed_max_218 - speed_min_214) * (rand() & 0x7fff) * g_float_005ec438 + speed_min_214;
    }
    magnitude *= size_scale_278;

    srVector3T<float>& velocity = velocities_198[index];
    switch (direction_mode_1b8) {
    case 1:
        velocity = direction_1e8 * magnitude;
        break;

    case 2: {
        srVector3T<double> direction = getWorldSpaceDOF();
        velocity = direction * magnitude;
        break;
    }

    case 3: {
        srVector3T<float> direction;
        direction.Set(g_float_005ebb34, g_float_005ebb34, magnitude);

        double angle = ((rand() & 0x7fff) * g_float_005ec438 - g_float_005ebc7c) * cone_pitch_20c;
        direction.RotateAboutX(sin(angle), cos(angle));

        angle = ((rand() & 0x7fff) * g_float_005ec438 - g_float_005ebc7c) * cone_yaw_208;
        direction.RotateAboutY(sin(angle), cos(angle));

        srMatrix3T<float> rotation;
        getWorldSpaceRotation(rotation);
        velocity = rotation.Transform(direction);
        break;
    }

    case 4: {
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
    particle_positions_148[index] = location;

    if (m_pflFlutterAngle != 0) {
        m_pflFlutterAngle[index] = (rand() & 0x7fff) * g_float_005ecc40;
    }
    if (texture_frames_178 != 0) {
        texture_frames_178[index * 2]->SetFrame00485400(0);
    }

    unsigned int vertex = index * 4;
    unsigned int end = vertex + 4;
    for (; vertex < end; ++vertex) {
        alphas_174[vertex] = 1.0f;
    }

    update_flags_250 |= 2;
    ++emission_count_188;
    ++active_particle_count_18c;
    return 1;
}

// FUNCTION: WIZ8 0x00499F70
void stParticle::DeactivateParticle00499F70(unsigned int index)
{
    unsigned char* active = particle_active_194 + index;
    if (*active != 0) {
        *active = 0;
        update_flags_250 |= 2;
        --active_particle_count_18c;
    }
}

// FUNCTION: WIZ8 0x00499FA0
void stParticle::Update00499FA0()
{
    unsigned int now = g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT);
    if (now - last_emitted_at_274 < emission_gap_270) {
        return;
    }

    last_emitted_at_274 = now;
    if (active_particle_count_18c != 0) {
        unsigned int elapsed_ticks = now - activated_at_258;

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

        double elapsed = (double)elapsed_ticks;
        srVector3T<float> acceleration_step = acceleration_1f4 * (elapsed * g_double_005ec8d0);

        unsigned int index;
        for (index = 0; index < particle_count_180; ++index) {
            if (particle_active_194[index] == 0) {
                continue;
            }

            unsigned int vertex = index * 4;
            if (expiry_mode_1ac == 0) {
                unsigned int expires_at = birth_ticks_19c[index] + lifetime_ms_1cc;
                if (expires_at < now) {
                    particle_active_194[index] = 0;
                    update_flags_250 |= 2;
                    --active_particle_count_18c;
                    continue;
                }
                if (expires_at - 500 < now) {
                    float alpha = (expires_at - now) * g_float_005ebc60;
                    unsigned int alpha_end = vertex + 4;
                    unsigned int alpha_index;
                    for (alpha_index = vertex; alpha_index < alpha_end; ++alpha_index) {
                        alphas_174[alpha_index] = alpha;
                    }
                }
            } else if (expiry_mode_1ac == 1) {
                if (texture_frames_178 == 0) {
                    expiry_mode_1ac = 0;
                    if (lifetime_ms_1cc == 0) {
                        lifetime_ms_1cc = 1000;
                    }
                } else {
                    stTextureAnim* animation = texture_frames_178[index * 2];
                    animation->UpdateFrame004854B0();
                    if (animation->IsFinished00485730() != 0) {
                        particle_active_194[index] = 0;
                        update_flags_250 |= 2;
                        --active_particle_count_18c;
                        continue;
                    }
                }
            }

            if (has_acceleration_1a8 == 1) {
                velocities_198[index] += acceleration_step;
            }

            srVector3T<float> movement = velocities_198[index] * (elapsed * g_double_005ec8d0);

            srVector3T<float> candidate;
            candidate = particle_positions_148[index] + movement;

            if (bounds_mode_1a4 == 2) {
                double distance;
                if (bounds_origin_234.x == g_float_005ebb34 &&
                    bounds_origin_234.y == g_float_005ebb34 &&
                    bounds_origin_234.z == g_float_005ebb34) {
                    distance = (candidate - node_location).Length();
                } else {
                    srVector3T<float> center =
                        rotation.Transform(bounds_origin_234) + node_location;
                    srVector3T<float> difference = candidate - center;
                    distance = difference.Length();
                }

                if (size_scale_278 * bounds_radius_240 < distance) {
                    particle_active_194[index] = 0;
                    update_flags_250 |= 2;
                    --active_particle_count_18c;
                    continue;
                }
            } else if (bounds_mode_1a4 == 1) {
                srVector3T<float> local = candidate - node_location;
                srVector4T<float> transformed = transform.Transform(local);
                srVector3T<float> local_point;
                local_point.Set(transformed.x, transformed.y, transformed.z);
                if (PointInsideBounds004BE870(&local_point, &minimum_21c, &maximum_228) == 0) {
                    particle_active_194[index] = 0;
                    update_flags_250 |= 2;
                    --active_particle_count_18c;
                    continue;
                }
            }

            if (los_check_enabled_1b4 == 1 &&
                (g_world->octree == 0 ||
                 !g_world->octree->HasLineOfSight(&particle_positions_148[index], &candidate, 1))) {
                particle_active_194[index] = 0;
                update_flags_250 |= 2;
                --active_particle_count_18c;
                continue;
            }

            particle_positions_148[index] = candidate;
        }
    }

    activated_at_258 = now;

    if (emitting_1a0 == 0 || emission_mode_1b0 == 0) {
        return;
    }

    unsigned int emission_elapsed = now - updated_at_25c;
    if (emission_elapsed < emission_interval_1c8) {
        return;
    }

    if (emission_mode_1b0 == 1) {
        unsigned int particle_index;
        ActivateParticle00499A50(&particle_index, replace_when_full_191);
        updated_at_25c = now;
        return;
    }
    if (emission_mode_1b0 != 2 || emission_elapsed <= emission_interval_1c8) {
        return;
    }

    for (;;) {
        unsigned int lag = now - emission_interval_1c8 - updated_at_25c;
        unsigned int particle_index;
        if (ActivateParticle00499A50(&particle_index, replace_when_full_191) == 0) {
            updated_at_25c = now;
            return;
        }

        if (has_acceleration_1a8 == 1) {
            srVector3T<float> acceleration = (acceleration_1f4 * (double)lag) / 1000.0;
            velocities_198[particle_index] += acceleration;
        }

        srVector3T<float> displacement = velocities_198[particle_index];
        displacement *= (double)lag;
        displacement /= 1000.0;
        InitializeParticlePosition0049A990(&particle_positions_148[particle_index]);
        particle_positions_148[particle_index] += displacement;

        updated_at_25c += emission_interval_1c8;
        if (now - updated_at_25c <= emission_interval_1c8) {
            return;
        }
    }
}

// FUNCTION: WIZ8 0x0049A990
void stParticle::InitializeParticlePosition0049A990(srVector3T<float>* output)
{
    output->x =
        (maximum_1dc.x - minimum_1d0.x) * (rand() & 0x7fff) * g_float_005ec438 + minimum_1d0.x;
    output->y =
        (maximum_1dc.y - minimum_1d0.y) * (rand() & 0x7fff) * g_float_005ec438 + minimum_1d0.y;
    output->z =
        (maximum_1dc.z - minimum_1d0.z) * (rand() & 0x7fff) * g_float_005ec438 + minimum_1d0.z;

    *output *= size_scale_278;

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
    if (nextSibling() != 0) {
        nextSibling()->traverse(info);
    }

    if (!testFlag(FLAG_DISABLE)) {
        if ((emitting_1a0 != 0 || active_particle_count_18c != 0) && traversal_enabled_1a1 != 0) {
            srNode::TraverseInfo::Entry& entry = info.entries[info.entry_count];
            entry.node = this;
            entry.value = 0;
            ++info.entry_count;
        }
    }

    if (!testFlag(FLAG_TERMINATE) && firstChild() != 0) {
        firstChild()->traverse(info);
    }
}

/* Traversal is gated separately from particle activity.  Starting a new
   enabled interval resets the update timestamp; repeated enables do not. */
// FUNCTION: WIZ8 0x00498D90
void stParticle::SetTraversalEnabled00498D90(unsigned char enabled)
{
    if (enabled != 0 && traversal_enabled_1a1 == 0) {
        updated_at_25c = g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT);
    }
    traversal_enabled_1a1 = enabled;
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
void stParticle::PrepareRenderer00498DD0(srMatrix4T<float>& view)
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

        float scale = static_cast<float>(particle_size_140) * size_scale_278;
        offsets[index] =
            srVector3T<float>(transformed.x, transformed.y, transformed.z) * (double)scale;
    }

    if (flutter_mode_1c0 == 0) {
        for (unsigned int direct_index = 0; direct_index < particle_count_180; ++direct_index) {
            unsigned int vertex = direct_index * 4;
            const srVector3T<float>& position = particle_positions_148[direct_index];
            vertex_positions_160[vertex] = position + offsets[0];
            vertex_positions_160[vertex + 1] = position + offsets[1];
            vertex_positions_160[vertex + 2] = position + offsets[2];
            vertex_positions_160[vertex + 3] = position + offsets[3];
        }
        return;
    }

    float phase = g_float_005ebb34;
    if (flutter_period_204 != 0) {
        phase = static_cast<float>(g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT) %
                                   flutter_period_204) /
                static_cast<int>(flutter_period_204) * g_camera_angle_period_005ec014;
    }
    float flutter = static_cast<float>(sin(phase)) * flutter_amplitude_200 * size_scale_278;

    for (unsigned int particle_index = 0; particle_index < particle_count_180; ++particle_index) {
        srVector3T<float> position;

        if (velocities_198[particle_index].y >= g_float_005ebb34) {
            position = particle_positions_148[particle_index];
        } else {
            position.Set(flutter, 0.0f, 0.0f);

            if (flutter_mode_1c0 == 2) {
                float scale = g_float_005ecc3c;
                if (g_float_005ecc3c < velocities_198[particle_index].y) {
                    scale = velocities_198[particle_index].y;
                }
                position.x = scale * g_float_005ecc38 * flutter;
            }

            double angle = m_pflFlutterAngle[particle_index];
            position.RotateAboutY(sin(angle), cos(angle));
            position += particle_positions_148[particle_index];
        }

        unsigned int vertex = particle_index * 4;
        vertex_positions_160[vertex] = position + offsets[0];
        vertex_positions_160[vertex + 1] = position + offsets[1];
        vertex_positions_160[vertex + 2] = position + offsets[2];
        vertex_positions_160[vertex + 3] = position + offsets[3];
    }
}

/* One particle system's complete submission. The system is placed (either
   relative to the camera or at its own node location), aged, stopped after
   its configured total activation count, culled against the renderer, and
   finally handed to the shared triangle-mesh pipeline as a single slot.

   The retired path is the only one that can drop the system: a particle whose
   activity has run out notifies its shake callback and, when release_when_done_190 marks
   it as self-owned, releases itself. */
// FUNCTION: WIZ8 0x004994D0
void stParticle::SubmitToRenderer(srGERD* renderer)
{
    srVector3T<float> position;

    if (camera_relative_1c4 == 1) {
        srVector3T<float> camera_position;
        GetCameraPosition(&camera_position);
        position = camera_position + camera_offset_244;

        srVector3T<double> placed;
        placed.SetFromFloat(&position);
        setLocation(placed);
    } else {
        /* Bound rather than copied: the three conversions read through the
           returned buffer instead of through a named local's own address. */
        const srVector3T<double>& located = getLocation();
        position = located;
    }

    Update00499FA0();

    if (emission_limit_184 != 0 && emission_count_188 >= emission_limit_184) {
        emitting_1a0 = 0;
    }

    if (emitting_1a0 == 0 && active_particle_count_18c == 0) {
        if (callback_26c != 0) {
            callback_26c->RestoreAnimation();
        }
        if (release_when_done_190) {
            release();
        }
        return;
    }

    srMatrix3T<float> rotation;

    if (bounds_mode_1a4 == 2) {
        srGERD::e_visibility visibility;

        /* Bound once: the retail body keeps the extent address in a register
           across the three comparisons and the three projections. */
        const srVector3T<float>& extent = bounds_origin_234;

        if (extent.x == g_float_005ebb34 && extent.y == g_float_005ebb34 &&
            extent.z == g_float_005ebb34) {
            visibility = renderer->testBoundingSphere(position, size_scale_278 * bounds_radius_240);
        } else {
            getRotation(rotation);
            srVector3T<float> center = rotation.Transform(extent) + position;
            visibility = renderer->testBoundingSphere(center, size_scale_278 * bounds_radius_240);
        }
        if (visibility == srGERD::VISIBILITY_POSITIONAL_0) {
            return;
        }
    }

    if (bounds_mode_1a4 == 1) {
        getRotation(rotation);
        srVector3T<float> minimum = rotation.Transform(minimum_21c) + position;
        srVector3T<float> maximum = rotation.Transform(maximum_228) + position;

        srGERD::e_visibility visibility = renderer->testBoundingBox(minimum, maximum);
        if (visibility == srGERD::VISIBILITY_POSITIONAL_0) {
            return;
        }
    }

    /* Two indices per surviving particle, rebuilt only after a deactivation
       has marked the pairs stale. */
    if ((update_flags_250 & 2) != 0) {
        unsigned int written = 0;
        for (unsigned int index = 0; index < particle_count_180; ++index) {
            if (particle_active_194[index] != 0) {
                active_triangles_254[written++] = index * 2;
                active_triangles_254[written++] = index * 2 + 1;
            }
        }
        update_flags_250 &= ~2u;
    }

    renderer->pushEnable();
    renderer->matrixMode(srGERD::MATRIX_MODELVIEW);

    srMatrix4T<float> view;
    renderer->getMatrix(srGERD::MATRIX_MODELVIEW, view);
    view.Invert();
    PrepareRenderer00498DD0(view);

    if (requires_positional_138 != 0 && !renderer->isEnabled(srGERD::ENABLE_POSITIONAL_1)) {
        renderer->toggle(srGERD::ENABLE_POSITIONAL_1);
    }
    renderer->setCullMode(srGERD::CULL_FRONT);
    renderer->setPickKey(0);

    srTriMeshPipeline* pipeline = srTriMeshPipeline::Get004750A0(renderer);

    /* Three array/count pairs: the index pairs rebuilt above, the polygon
       index list, and the transformed vertex positions. */
    pipeline->active_triangles_2c = active_triangles_254;
    pipeline->active_triangle_count_24 = active_particle_count_18c * 2;
    pipeline->triangles_34 = triangles_168;
    pipeline->triangle_count_1c = texture_frame_count_15c;
    pipeline->positions_38 = vertex_positions_160;
    pipeline->vertex_count_20 = vertex_count_158;
    if (vertex_extras_170 != 0) {
        pipeline->vertex_extras_3c = vertex_extras_170;
    }

    pipeline->current_record_14->flags_00 = 0;
    pipeline->current_pass_18->shader_14 = 0;
    pipeline->current_pass_18->texture_array_0c = 0;
    pipeline->current_pass_18->value_10 = 0;

    if (colors_16c != 0) {
        pipeline->current_record_14->colors_0c = colors_16c;
        pipeline->current_record_14->color_format_10 = 1;
        pipeline->current_record_14->flags_00 |= 1;
    }
    if (alphas_174 != 0) {
        pipeline->current_record_14->alphas_1c = alphas_174;
        pipeline->current_record_14->flags_00 |= 8;
    }

    /* The retained object is the batch's material: the same pointer reaches
       both the pipeline and the record it is about to submit. */
    pipeline->material_80 = retained_14c;
    pipeline->current_record_14->material_08 = retained_14c;

    pipeline->SetFlags004752C0(render_flags_150);

    if (texcoords_164 != 0) {
        pipeline->current_record_14->st0_20 = texcoords_164;
        pipeline->current_record_14->flags_00 |= 0x10;
    }

    if (texture_frames_178 != 0) {
        pipeline->current_pass_18->texture_array_0c = texture_frames_178;
    } else {
        srTextureIFace* texture = texture_154;

        if (texture != 0) {
            pipeline->texture_78 = texture;
            pipeline->current_pass_18->texture_00 = texture;
        }
    }

    ++pipeline->slot_count_84;
    pipeline->PrepareSlot00475540();

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
    if (particle_positions_148 != 0) {
        srHeap.free(particle_positions_148);
    }
    if (vertex_extras_170 != 0) {
        srHeap.free(vertex_extras_170);
    }
    if (texcoords_164 != 0) {
        srHeap.free(texcoords_164);
    }
    if (vertex_positions_160 != 0) {
        srHeap.free(vertex_positions_160);
    }
    if (triangles_168 != 0) {
        srHeap.free(triangles_168);
    }
    if (colors_16c != 0) {
        srHeap.free(colors_16c);
    }
    if (alphas_174 != 0) {
        delete[] alphas_174;
    }
    if (retained_14c != 0) {
        retained_14c->release();
    }
    if (velocities_198 != 0) {
        srHeap.free(velocities_198);
    }
    if (birth_ticks_19c != 0) {
        delete[] birth_ticks_19c;
    }
    if (active_triangles_254 != 0) {
        delete[] active_triangles_254;
    }
    if (particle_active_194 != 0) {
        delete[] particle_active_194;
    }
    if (texture_frames_178 != 0) {
        for (unsigned int i = 0; i < texture_frame_count_15c; i += 2) {
            texture_frames_178[i]->release();
        }
        delete[] texture_frames_178;
    }
    if (m_pflFlutterAngle != 0) {
        delete[] m_pflFlutterAngle;
    }
    texture_154->release();
    setParent(0, 1);
}

// FUNCTION: WIZ8 0x0049AB00
void stParticle::SetTexture0049AB00(srTextureIFace* texture)
{
    unsigned int i;

    if (texture_154 != 0) {
        if (texture_frames_178 != 0) {
            for (i = 0; i < texture_frame_count_15c; i += 2) {
                texture_frames_178[i]->release();
            }
            delete[] texture_frames_178;
            texture_frames_178 = 0;
        }
        texture_154->release();
    }

    if (texture != 0 && texture->getClassID() == stTextureAnim::CLASS_ID) {
        texture_frames_178 = new stTextureAnim*[texture_frame_count_15c];
        for (i = 0; i < texture_frame_count_15c; i += 2) {
            stTextureAnim* frame = new stTextureAnim(*static_cast<stTextureAnim*>(texture));
            texture_frames_178[i] = frame;
            texture_frames_178[i + 1] = frame;
        }
    }

    texture_154 = texture;
    texture->addReference();
}

// FUNCTION: WIZ8 0x0049acd0
void stParticle::SetActive(unsigned char active)
{
    if (active != 0 && emitting_1a0 == 0) {
        unsigned int now = g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT);
        activated_at_258 = now;
        updated_at_25c = now;
    }
    emitting_1a0 = active;
}

// FUNCTION: WIZ8 0x0049ac30
unsigned char stParticle::ReplaceTexture0049AC30(const char* old_name, srTextureIFace* replacement)
{
    if (texture_154 != 0 &&
        (texture_154->getClassID() == 0x10001 || texture_154->getClassID() == 0x10000) &&
        _stricmp(texture_154->getName(), old_name) == 0) {
        SetTexture0049AB00(replacement);
        return 1;
    }
    return 0;
}

// FUNCTION: WIZ8 0x0049ACA0
void stParticle::SetRetainedObject0049ACA0(srMaterialIFace* material)
{
    if (retained_14c != 0) {
        retained_14c->release();
    }
    retained_14c = material;
    if (material != 0) {
        material->addReference();
    }
}

// FUNCTION: WIZ8 0x0049AD10
void stParticle::SetFlutter0049AD10(int enabled)
{
    unsigned int i;

    flutter_mode_1c0 = enabled;
    if (enabled == 0) {
        if (m_pflFlutterAngle != 0) {
            delete[] m_pflFlutterAngle;
            m_pflFlutterAngle = 0;
        }
    } else if (m_pflFlutterAngle == 0) {
        m_pflFlutterAngle = new float[particle_count_180];
        if (m_pflFlutterAngle == 0) {
            srAssertFail("m_pflFlutterAngle", ST_PARTICLE_CPP, 1210, 0);
        }
        for (i = 0; i < particle_count_180; ++i) {
            m_pflFlutterAngle[i] = 0.0f;
        }
    }
}
