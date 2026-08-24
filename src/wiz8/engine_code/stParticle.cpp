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

static const char ST_PARTICLE_CPP[] =
    "C:\\Projects\\Wizardry 8\\Engine Code\\stParticle.cpp";

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

// FUNCTION: WIZ8 0x0049ADB0
stParticle* FindRegisteredParticle0049ADB0(const char* name)
{
    stParticle* particle = 0;
    char* uppercase_name = static_cast<char*>(malloc(strlen(name) + 1));

    if (uppercase_name != 0) {
        strcpy(uppercase_name, name);
        _strupr(uppercase_name);
        particle = static_cast<stParticle*>(srCore.getRegistry()->find(
            stParticle::sGetClassNode(), uppercase_name, 0));
    }
    free(uppercase_name);
    return particle;
}

// FUNCTION: WIZ8 0x0049B150
void SaveParticleStates0049B150(HWFILE handle)
{
    unsigned char version = 1;
    char name[0x80] = "";
    int count = 0;

    FileWrite(handle, &version, sizeof(version), 0);

    stParticle* particle = static_cast<stParticle*>(
        srCore.getRegistry()->find(
            stParticle::sGetClassNode(),
            static_cast<const srRuntimeClass*>(0)));
    while (particle != 0) {
        if (particle->trigger_flag_192 != 0) {
            ++count;
        }
        particle = static_cast<stParticle*>(srCore.getRegistry()->find(
            stParticle::sGetClassNode(), particle));
    }

    FileWrite(handle, &count, sizeof(count), 0);

    particle = static_cast<stParticle*>(
        srCore.getRegistry()->find(
            stParticle::sGetClassNode(),
            static_cast<const srRuntimeClass*>(0)));
    while (particle != 0) {
        if (particle->trigger_flag_192 != 0) {
            strcpy(name, particle->getName());
            FileWrite(handle, name, sizeof(name), 0);
            FileWrite(handle, &particle->active_1a0,
                      sizeof(particle->active_1a0), 0);
        }
        particle = static_cast<stParticle*>(srCore.getRegistry()->find(
            stParticle::sGetClassNode(), particle));
    }
}

// FUNCTION: WIZ8 0x0049B3B0
void LoadParticleStates0049B3B0(int handle)
{
    unsigned char version;
    int count = 0;
    char name[0x80] = "";

    ReadVirtualFile(handle, &version, sizeof(version), 0);
    ReadVirtualFile(handle, &count, sizeof(count), 0);

    for (int index = 0; index < count; ++index) {
        unsigned char active;
        ReadVirtualFile(handle, name, sizeof(name), 0);
        ReadVirtualFile(handle, &active, sizeof(active), 0);

        stParticle* particle = static_cast<stParticle*>(
            srCore.getRegistry()->find(
                stParticle::sGetClassNode(), name, 0));
        if (particle != 0) {
            particle->SetActive(active);
        }
    }
}

// FUNCTION: WIZ8 0x00497AF0
stParticle::stParticle(srNode* parent, unsigned int count)
    : srClassSupport<stParticle, srNode, 0, 0x10009>(
          static_cast<srNode*>(0))
{
    trigger_flag_192 = 0;
    update_flags_250 = 0;
    value_260 = -1;
    start_frame_264 = -1;
    end_frame_268 = -1;
    callback_26c = 0;
    value_278 = 1.0f;

    setParent(parent, 1);

    particle_count_180 = count;
    allocation_148 = 0;
    allocation_164 = 0;
    allocation_160 = 0;
    allocation_170 = 0;
    allocation_168 = 0;
    allocation_16c = 0;
    texture_154 = 0;
    value_138 = 0;
    value_140 = 1.0;

    if (count == 0) {
        return;
    }

    if (count >= 10000) {
        srAssertFail("cnt < 10000", ST_PARTICLE_CPP, 0x41, 0);
    }

    allocation_148 = static_cast<srVector3T<float>*>(
        srHeap.allocate(count * sizeof(srVector3T<float>)));
    unsigned int i;
    for (i = 0; i < count; ++i) {
        allocation_148[i].x = 0.0f;
        allocation_148[i].y = 0.0f;
        allocation_148[i].z = 0.0f;
    }

    vertex_count_158 = count * 4;
    texture_frame_count_15c = count * 2;
    allocation_164 = static_cast<srVector2T<float>*>(
        srHeap.allocate(vertex_count_158 * sizeof(srVector2T<float>)));
    allocation_160 = static_cast<srVector3T<float>*>(
        srHeap.allocate(vertex_count_158 * sizeof(srVector3T<float>)));
    allocation_168 = static_cast<srVector3i*>(
        srHeap.allocate(count * 2 * sizeof(srVector3i)));
    allocation_174 = new float[vertex_count_158];
    texture_frames_178 = 0;

    for (i = 0; i < count; ++i) {
        unsigned int vertex = i * 4;
        unsigned int triangle = i * 2;
        allocation_168[triangle].x = vertex;
        allocation_168[triangle].y = vertex + 1;
        allocation_168[triangle].z = vertex + 2;
        allocation_168[triangle + 1].x = vertex + 2;
        allocation_168[triangle + 1].y = vertex + 3;
        allocation_168[triangle + 1].z = vertex;

        allocation_148[i].x = 0.0f;
        allocation_148[i].y = 0.0f;
        allocation_148[i].z = 0.0f;

        allocation_164[vertex].x = 0.0f;
        allocation_164[vertex].y = 0.0f;
        allocation_164[vertex + 1].x = 1.0f;
        allocation_164[vertex + 1].y = 0.0f;
        allocation_164[vertex + 2].x = 1.0f;
        allocation_164[vertex + 2].y = 1.0f;
        allocation_164[vertex + 3].x = 0.0f;
        allocation_164[vertex + 3].y = 1.0f;
    }

    for (i = 0; i < vertex_count_158; ++i) {
        allocation_174[i] = 1.0f;
    }

    active_1a0 = 1;
    flag_1a1 = 1;
    retained_14c = 0;
    state_184 = 0;
    active_190 = 0;
    unknown_191 = 0;
    value_188 = 0;
    allocation_254 = new unsigned long[texture_frame_count_15c];
    active_particle_count_18c = 0;
    allocation_198 = static_cast<srVector3T<float>*>(
        srHeap.allocate(count * sizeof(srVector3T<float>)));
    allocation_19c = static_cast<unsigned int*>(
        ::operator new(count * sizeof(unsigned int)));
    allocation_194 = new unsigned char[count];
    memset(allocation_194, 0, count);

    value_1a8 = 0;
    value_1ac = 0;
    value_1b0 = 0;
    value_1b4 = 0;
    value_1b8 = 3;
    value_1c4 = 0;
    value_1c8 = 50;
    value_1cc = 1500;
    value_1a4 = 2;
    value_1bc = 2;
    minimum_1d0.x = -250.0f;
    minimum_1d0.y = -250.0f;
    minimum_1d0.z = -250.0f;
    maximum_1dc.x = 250.0f;
    maximum_1dc.y = 250.0f;
    maximum_1dc.z = 250.0f;
    direction_1e8.x = 0.0f;
    direction_1e8.y = -1.0f;
    direction_1e8.z = 0.0f;
    value_210 = 500.0f;
    acceleration_1f4.x = 0.0f;
    acceleration_1f4.y = -4905.0f;
    acceleration_1f4.z = 0.0f;
    value_1c0 = 0;
    m_pflFlutterAngle = 0;
    value_200 = 0.0f;
    value_204 = 0;
    value_218 = 4000.0f;
    value_208 = 0.39269906f;
    value_20c = 0.39269906f;
    value_214 = 1000.0f;
    minimum_21c.x = -1000.0f;
    minimum_21c.y = -1000.0f;
    minimum_21c.z = -1000.0f;
    maximum_228.x = 1000.0f;
    maximum_228.y = 1000.0f;
    maximum_228.z = 1000.0f;
    value_234.x = 0.0f;
    value_234.y = 0.0f;
    value_234.z = 0.0f;
    value_240 = 2000.0f;
    update_flags_250 = 0;
    activated_at_258 = g_shared_timer_base->getMsTime(
        srTimer::TIMER_READ_DEFAULT);
    updated_at_25c = activated_at_258;
    value_270 = 25;
    value_274 = 0;
}

// FUNCTION: WIZ8 0x00499A50
unsigned char stParticle::ActivateParticle00499A50(
    unsigned int* out_index,
    unsigned char replace_when_full)
{
    if (state_184 != 0 && value_188 >= state_184) {
        return 0;
    }

    unsigned int index;
    for (index = 0; index < particle_count_180; ++index) {
        if (allocation_194[index] == 0) {
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
            if (allocation_19c[candidate] < allocation_19c[oldest] &&
                allocation_194[candidate] != 0) {
                oldest = candidate;
            }
        }
        DeactivateParticle00499F70(oldest);
        index = oldest;
    }

    *out_index = index;
    allocation_194[index] = 1;
    allocation_19c[index] = g_shared_timer_base->getMsTime(
        srTimer::TIMER_READ_DEFAULT);

    float magnitude = 0.0f;
    if (value_1bc == 1) {
        magnitude = value_210;
    }
    else if (value_1bc == 2) {
        magnitude = (value_218 - value_214) *
                (float)(rand() & 0x7fff) * g_float_005ec438 +
            value_214;
    }
    magnitude *= value_278;

    srVector3T<float>& velocity = allocation_198[index];
    switch (value_1b8) {
    case 1:
        velocity.x = direction_1e8.x * magnitude;
        velocity.y = direction_1e8.y * magnitude;
        velocity.z = direction_1e8.z * magnitude;
        break;

    case 2: {
        srVector3T<double> direction = getWorldSpaceDOF();
        velocity.x = (float)(magnitude * direction.x);
        velocity.y = (float)(magnitude * direction.y);
        velocity.z = (float)(magnitude * direction.z);
        break;
    }

    case 3: {
        srVector3T<float> direction;
        direction.x = g_float_005ebb34;
        direction.y = g_float_005ebb34;
        direction.z = magnitude;

        double angle = ((float)(rand() & 0x7fff) * g_float_005ec438 -
                        g_float_005ebc7c) *
            value_20c;
        direction.method_0049BA80(sin(angle), cos(angle));

        angle = ((float)(rand() & 0x7fff) * g_float_005ec438 -
                 g_float_005ebc7c) *
            value_208;
        direction.method_00451A10(sin(angle), cos(angle));

        srMatrix3T<float> rotation;
        getWorldSpaceRotation(rotation);
        velocity.x = rotation.vectors[0].x * direction.x +
            rotation.vectors[0].y * direction.y +
            rotation.vectors[0].z * direction.z;
        velocity.y = Function4218E0(rotation.vectors[1], direction);
        velocity.z = Function4218E0(rotation.vectors[2], direction);
        break;
    }

    case 4: {
        srVector3T<float> direction;
        direction.x = (float)(rand() & 0x7fff) * g_float_005ec438 -
            g_float_005ebc7c;
        direction.y = (float)(rand() & 0x7fff) * g_float_005ec438 -
            g_float_005ebc7c;
        direction.z = (float)(rand() & 0x7fff) * g_float_005ec438 -
            g_float_005ebc7c;

        float length_squared = direction.x * direction.x +
            direction.y * direction.y + direction.z * direction.z;
        if ((double)length_squared != g_zero_005ebb40) {
            float normalization = (float)(
                g_double_005ebc30 / sqrt(length_squared));
            direction.x *= normalization;
            direction.y *= normalization;
            direction.z *= normalization;
        }

        velocity.x = direction.x * magnitude;
        velocity.y = direction.y * magnitude;
        velocity.z = direction.z * magnitude;
        break;
    }

    default:
        velocity.x = 0.0f;
        velocity.y = 0.0f;
        velocity.z = 0.0f;
        break;
    }

    srVector3T<double> location = getLocation();
    allocation_148[index].x = (float)location.x;
    allocation_148[index].y = (float)location.y;
    allocation_148[index].z = (float)location.z;

    if (m_pflFlutterAngle != 0) {
        m_pflFlutterAngle[index] =
            (float)(rand() & 0x7fff) * g_float_005ecc40;
    }
    if (texture_frames_178 != 0) {
        texture_frames_178[index * 2]->SetFrame00485400(0);
    }

    unsigned int vertex = index * 4;
    unsigned int end = vertex + 4;
    for (; vertex < end; ++vertex) {
        allocation_174[vertex] = 1.0f;
    }

    update_flags_250 |= 2;
    ++value_188;
    ++active_particle_count_18c;
    return 1;
}

// FUNCTION: WIZ8 0x00499F70
void stParticle::DeactivateParticle00499F70(unsigned int index)
{
    unsigned char* active = allocation_194 + index;
    if (*active != 0) {
        *active = 0;
        update_flags_250 |= 2;
        --active_particle_count_18c;
    }
}

// FUNCTION: WIZ8 0x00499FA0
void stParticle::Update00499FA0()
{
    unsigned int now = g_shared_timer_base->getMsTime(
        srTimer::TIMER_READ_DEFAULT);
    if (now - value_274 < value_270) {
        return;
    }

    value_274 = now;
    if (active_particle_count_18c != 0) {
        unsigned int elapsed_ticks = now - activated_at_258;

        srMatrix3T<float> rotation;
        getRotation(rotation);

        srMatrix4T<float> transform;
        transform.vectors[0].x = rotation.vectors[0].x;
        transform.vectors[0].y = rotation.vectors[0].y;
        transform.vectors[0].z = rotation.vectors[0].z;
        transform.vectors[0].w = 0.0f;
        transform.vectors[1].x = rotation.vectors[1].x;
        transform.vectors[1].y = rotation.vectors[1].y;
        transform.vectors[1].z = rotation.vectors[1].z;
        transform.vectors[1].w = 0.0f;
        transform.vectors[2].x = rotation.vectors[2].x;
        transform.vectors[2].y = rotation.vectors[2].y;
        transform.vectors[2].z = rotation.vectors[2].z;
        transform.vectors[2].w = 0.0f;
        transform.vectors[3].x = 0.0f;
        transform.vectors[3].y = 0.0f;
        transform.vectors[3].z = 0.0f;
        transform.vectors[3].w = 1.0f;

        srMatrix4T<float> inverse;
        inverse.AdjugateFrom0049BF20(&transform.vectors[0].x);
        float determinant = transform.Det0049BDF0();
        if (determinant != g_double_005ebc30) {
            inverse.Scale0049BD50(g_double_005ebc30 / determinant);
        }
        transform = inverse;

        srVector3T<float> node_location;
        getLocation(node_location);

        double elapsed = (double)elapsed_ticks;
        srVector3T<float> acceleration_step;
        acceleration_step.method_00421680(
            acceleration_1f4.x * elapsed * g_double_005ec8d0,
            acceleration_1f4.y * elapsed * g_double_005ec8d0,
            acceleration_1f4.z * elapsed * g_double_005ec8d0);

        unsigned int index;
        for (index = 0; index < particle_count_180; ++index) {
            if (allocation_194[index] == 0) {
                continue;
            }

            unsigned int vertex = index * 4;
            if (value_1ac == 0) {
                unsigned int expires_at = allocation_19c[index] + value_1cc;
                if (expires_at < now) {
                    allocation_194[index] = 0;
                    update_flags_250 |= 2;
                    --active_particle_count_18c;
                    continue;
                }
                if (expires_at - 500 < now) {
                    float alpha = (float)(expires_at - now) *
                        g_float_005ebc60;
                    unsigned int alpha_end = vertex + 4;
                    unsigned int alpha_index;
                    for (alpha_index = vertex;
                         alpha_index < alpha_end;
                         ++alpha_index) {
                        allocation_174[alpha_index] = alpha;
                    }
                }
            }
            else if (value_1ac == 1) {
                if (texture_frames_178 == 0) {
                    value_1ac = 0;
                    if (value_1cc == 0) {
                        value_1cc = 1000;
                    }
                }
                else {
                    stTextureAnim* animation = texture_frames_178[index * 2];
                    animation->UpdateFrame004854B0();
                    if (animation->IsFinished00485730() != 0) {
                        allocation_194[index] = 0;
                        update_flags_250 |= 2;
                        --active_particle_count_18c;
                        continue;
                    }
                }
            }

            if (value_1a8 == 1) {
                allocation_198[index].x += acceleration_step.x;
                allocation_198[index].y += acceleration_step.y;
                allocation_198[index].z += acceleration_step.z;
            }

            srVector3T<float> movement;
            movement.method_00421680(
                allocation_198[index].x * elapsed * g_double_005ec8d0,
                allocation_198[index].y * elapsed * g_double_005ec8d0,
                allocation_198[index].z * elapsed * g_double_005ec8d0);

            srVector3T<float> candidate;
            candidate.x = allocation_148[index].x + movement.x;
            candidate.y = allocation_148[index].y + movement.y;
            candidate.z = allocation_148[index].z + movement.z;

            if (value_1a4 == 2) {
                double distance;
                if (value_234.x == g_float_005ebb34 &&
                    value_234.y == g_float_005ebb34 &&
                    value_234.z == g_float_005ebb34) {
                    float x = candidate.x - node_location.x;
                    float y = candidate.y - node_location.y;
                    float z = candidate.z - node_location.z;
                    distance = sqrt(x * x + y * y + z * z);
                }
                else {
                    float center_x =
                        Function4218E0(rotation.vectors[0], value_234);
                    float center_y =
                        Function4218E0(rotation.vectors[1], value_234);
                    float center_z =
                        Function4218E0(rotation.vectors[2], value_234);
                    srVector3T<float> center(
                        center_x + node_location.x,
                        center_y + node_location.y,
                        center_z + node_location.z);
                    srVector3T<float> difference(
                        candidate.x - center.x,
                        candidate.y - center.y,
                        candidate.z - center.z);
                    distance = sqrt(
                        difference.y * difference.y +
                        difference.z * difference.z +
                        difference.x * difference.x);
                }

                if (value_278 * value_240 < distance) {
                    allocation_194[index] = 0;
                    update_flags_250 |= 2;
                    --active_particle_count_18c;
                    continue;
                }
            }
            else if (value_1a4 == 1) {
                srVector3T<float> local(
                    candidate.x - node_location.x,
                    candidate.y - node_location.y,
                    candidate.z - node_location.z);
                float* matrix = &transform.vectors[0].x;
                srVector4T<float> transformed;
                transformed.method_004D6B30(
                    local.x * matrix[0] + local.y * matrix[1] +
                        local.z * matrix[2] + matrix[3],
                    local.x * matrix[4] + local.y * matrix[5] +
                        local.z * matrix[6] + matrix[7],
                    local.x * matrix[8] + local.y * matrix[9] +
                        local.z * matrix[10] + matrix[11],
                    local.x * matrix[12] + local.y * matrix[13] +
                        local.z * matrix[14] + matrix[15]);
                srVector3T<float> local_point;
                local_point.x = transformed.x;
                local_point.y = transformed.y;
                local_point.z = transformed.z;
                if (PointInsideBounds004BE870(
                        &local_point, &minimum_21c, &maximum_228) == 0) {
                    allocation_194[index] = 0;
                    update_flags_250 |= 2;
                    --active_particle_count_18c;
                    continue;
                }
            }

            if (value_1b4 == 1 &&
                (g_world->octree == 0 ||
                 !g_world->octree->HasLineOfSight(
                     reinterpret_cast<const srVector3T<float>*>(&allocation_148[index]),
                     reinterpret_cast<srVector3T<float>*>(&candidate), 1))) {
                allocation_194[index] = 0;
                update_flags_250 |= 2;
                --active_particle_count_18c;
                continue;
            }

            allocation_148[index] = candidate;
        }
    }

    activated_at_258 = now;

    if (active_1a0 == 0 || value_1b0 == 0) {
        return;
    }

    unsigned int emission_elapsed = now - updated_at_25c;
    if (emission_elapsed < value_1c8) {
        return;
    }

    if (value_1b0 == 1) {
        unsigned int particle_index;
        ActivateParticle00499A50(&particle_index, unknown_191);
        updated_at_25c = now;
        return;
    }
    if (value_1b0 != 2 || emission_elapsed <= value_1c8) {
        return;
    }

    for (;;) {
        unsigned int lag = now - value_1c8 - updated_at_25c;
        unsigned int particle_index;
        if (ActivateParticle00499A50(
                &particle_index, unknown_191) == 0) {
            updated_at_25c = now;
            return;
        }

        if (value_1a8 == 1) {
            srVector3T<float> acceleration =
                (acceleration_1f4 * (double)lag) / 1000.0;
            allocation_198[particle_index] += acceleration;
        }

        srVector3T<float> displacement = allocation_198[particle_index];
        displacement *= (double)lag;
        displacement /= 1000.0;
        InitializeParticlePosition0049A990(&allocation_148[particle_index]);
        allocation_148[particle_index] += displacement;

        updated_at_25c += value_1c8;
        if (now - updated_at_25c <= value_1c8) {
            return;
        }
    }
}

// FUNCTION: WIZ8 0x0049A990
void stParticle::InitializeParticlePosition0049A990(
    srVector3T<float>* output)
{
    output->x = (maximum_1dc.x - minimum_1d0.x) *
            (float)(rand() & 0x7fff) * g_float_005ec438 +
        minimum_1d0.x;
    output->y = (maximum_1dc.y - minimum_1d0.y) *
            (float)(rand() & 0x7fff) * g_float_005ec438 +
        minimum_1d0.y;
    output->z = (maximum_1dc.z - minimum_1d0.z) *
            (float)(rand() & 0x7fff) * g_float_005ec438 +
        minimum_1d0.z;

    output->x *= value_278;
    output->y *= value_278;
    output->z *= value_278;

    srMatrix3T<float> rotation;
    getRotation(rotation);
    srVector3T<float> original = *output;
    output->x = rotation.vectors[0].x * original.x +
        rotation.vectors[0].z * original.z +
        rotation.vectors[0].y * original.y;
    output->y = rotation.vectors[1].z * original.z +
        rotation.vectors[1].y * original.y +
        rotation.vectors[1].x * original.x;
    output->z = rotation.vectors[2].z * original.z +
        rotation.vectors[2].y * original.y +
        rotation.vectors[2].x * original.x;

    srVector3T<double> location = getLocation();
    output->x += (float)location.x;
    output->y += (float)location.y;
    output->z += (float)location.z;
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

    if (!testFlag(FLAG_POSITIONAL_0)) {
        if ((active_1a0 != 0 || active_particle_count_18c != 0) && flag_1a1 != 0) {
            srNode::TraverseInfo::Entry& entry =
                info.entries[info.entry_count];
            entry.node = this;
            entry.value = 0;
            ++info.entry_count;
        }
    }

    if (!testFlag(FLAG_POSITIONAL_1) && firstChild() != 0) {
        firstChild()->traverse(info);
    }
}

/* Traversal is gated separately from particle activity.  Starting a new
   enabled interval resets the update timestamp; repeated enables do not. */
// FUNCTION: WIZ8 0x00498D90
void stParticle::SetTraversalEnabled00498D90(unsigned char enabled)
{
    if (enabled != 0 && flag_1a1 == 0) {
        updated_at_25c = g_shared_timer_base->getMsTime(
            srTimer::TIMER_READ_DEFAULT);
    }
    flag_1a1 = enabled;
}

// FUNCTION: WIZ8 0x00498D60
void stParticle::process(const ProcessInfo& info, e_processType)
{
    info.renderer->pushMatrix();
    Function4994D0(info.renderer);
    info.renderer->popMatrix();
}

/* Build the four camera-facing offsets once per call, then expand every
   particle center into a quad. */
// FUNCTION: WIZ8 0x00498DD0
void stParticle::PrepareRenderer00498DD0(srMatrix4T<float>& view)
{
    static srVector3T<float> corners[4] = {
        srVector3T<float>(-0.5f, 0.5f, 0.0f),
        srVector3T<float>(0.5f, 0.5f, 0.0f),
        srVector3T<float>(0.5f, -0.5f, 0.0f),
        srVector3T<float>(-0.5f, -0.5f, 0.0f)
    };
    static srVector3T<float> offsets[4];

    float* matrix = &view.vectors[0].x;
    matrix[3] = 0.0f;
    matrix[7] = 0.0f;
    matrix[11] = 0.0f;

    float normalization = (float)(
        g_double_005ebc30
        / sqrt(matrix[0] * matrix[0] + matrix[1] * matrix[1]
               + matrix[2] * matrix[2] + matrix[3] * matrix[3]));
    matrix[0] *= normalization;
    matrix[1] *= normalization;
    matrix[2] *= normalization;
    matrix[3] *= normalization;
    matrix[4] *= normalization;
    matrix[5] *= normalization;
    matrix[6] *= normalization;
    matrix[7] *= normalization;
    matrix[8] *= normalization;
    matrix[9] *= normalization;
    matrix[10] *= normalization;
    matrix[11] *= normalization;

    for (unsigned int index = 0; index < 4; ++index) {
        srVector4T<float> transformed;
        transformed.method_004D6B30(
            corners[index].y * matrix[1] + corners[index].x * matrix[0]
                + corners[index].z * matrix[2] + matrix[3],
            corners[index].y * matrix[5] + corners[index].z * matrix[6]
                + corners[index].x * matrix[4] + matrix[7],
            corners[index].y * matrix[9] + corners[index].x * matrix[8]
                + corners[index].z * matrix[10] + matrix[11],
            corners[index].y * matrix[13] + corners[index].x * matrix[12]
                + corners[index].z * matrix[14] + matrix[15]);

        float scale = (float)value_140 * value_278;
        offsets[index].x = transformed.x * scale;
        offsets[index].y = transformed.y * scale;
        offsets[index].z = transformed.z * scale;
    }

    if (value_1c0 == 0) {
        for (unsigned int direct_index = 0;
             direct_index < particle_count_180;
             ++direct_index) {
            unsigned int vertex = direct_index * 4;
            const srVector3T<float>& position = allocation_148[direct_index];
            allocation_160[vertex].x = position.x + offsets[0].x;
            allocation_160[vertex].y = position.y + offsets[0].y;
            allocation_160[vertex].z = position.z + offsets[0].z;
            allocation_160[vertex + 1].x = position.x + offsets[1].x;
            allocation_160[vertex + 1].y = position.y + offsets[1].y;
            allocation_160[vertex + 1].z = position.z + offsets[1].z;

            allocation_160[vertex + 2] = srVector3T<float>(
                position.x + offsets[2].x,
                position.y + offsets[2].y,
                position.z + offsets[2].z);
            allocation_160[vertex + 3] = srVector3T<float>(
                position.x + offsets[3].x,
                position.y + offsets[3].y,
                position.z + offsets[3].z);
        }
        return;
    }

    float phase = g_float_005ebb34;
    if (value_204 != 0) {
        phase = (float)(g_shared_timer_base->getMsTime(
                           srTimer::TIMER_READ_DEFAULT)
                       % value_204)
            / (int)value_204 * g_camera_angle_period_005ec014;
    }
    float flutter = (float)sin(phase) * value_200 * value_278;

    for (unsigned int particle_index = 0;
         particle_index < particle_count_180;
         ++particle_index) {
        srVector3T<float> position;

        if (allocation_198[particle_index].y >= g_float_005ebb34) {
            position = allocation_148[particle_index];
        }
        else {
            position.x = flutter;
            position.y = 0.0f;
            position.z = 0.0f;

            if (value_1c0 == 2) {
                float scale = g_float_005ecc3c;
                if (g_float_005ecc3c < allocation_198[particle_index].y) {
                    scale = allocation_198[particle_index].y;
                }
                position.x = scale * g_float_005ecc38 * flutter;
            }

            double angle = m_pflFlutterAngle[particle_index];
            position.method_00451A10(sin(angle), cos(angle));
            position.x += allocation_148[particle_index].x;
            position.y += allocation_148[particle_index].y;
            position.z += allocation_148[particle_index].z;
        }

        unsigned int vertex = particle_index * 4;
        allocation_160[vertex].x = position.x + offsets[0].x;
        allocation_160[vertex].y = position.y + offsets[0].y;
        allocation_160[vertex].z = position.z + offsets[0].z;
        allocation_160[vertex + 1].x = position.x + offsets[1].x;
        allocation_160[vertex + 1].y = position.y + offsets[1].y;
        allocation_160[vertex + 1].z = position.z + offsets[1].z;
        allocation_160[vertex + 2] = srVector3T<float>(
            position.x + offsets[2].x,
            position.y + offsets[2].y,
            position.z + offsets[2].z);
        allocation_160[vertex + 3] = position + offsets[3];
    }
}

/* One particle system's complete submission. The system is placed (either
   relative to the camera or at its own node location), aged, stopped after
   its configured total activation count, culled against the renderer, and
   finally handed to the shared triangle-mesh pipeline as a single slot.

   The retired path is the only one that can drop the system: a particle whose
   activity has run out notifies its shake callback and, when active_190 marks
   it as self-owned, releases itself. */
// FUNCTION: WIZ8 0x004994D0
void stParticle::Function4994D0(srGERD* renderer)
{
    srVector3T<float> position;

    if (value_1c4 == 1) {
        srVector3T<float> camera_position;
        GetCameraPosition(&camera_position);
        position.x = camera_position.x + camera_offset_244.x;
        position.y = camera_position.y + camera_offset_244.y;
        position.z = camera_position.z + camera_offset_244.z;

        srVector3T<double> placed;
        placed.x = position.x;
        placed.y = position.y;
        placed.z = position.z;
        setLocation(placed);
    }
    else {
        /* Bound rather than copied: the three conversions read through the
           returned buffer instead of through a named local's own address. */
        const srVector3T<double>& located = getLocation();
        position.x = (float)located.x;
        position.y = (float)located.y;
        position.z = (float)located.z;
    }

    Update00499FA0();

    if (state_184 != 0 && value_188 >= state_184) {
        active_1a0 = 0;
    }

    if (active_1a0 == 0 && active_particle_count_18c == 0) {
        if (callback_26c != 0) {
            callback_26c->RestoreAnimation();
        }
        if (active_190 != 0) {
            release();
        }
        return;
    }

    srMatrix3T<float> rotation;

    if (value_1a4 == 2) {
        srGERD::e_visibility visibility;

        /* Bound once: the retail body keeps the extent address in a register
           across the three comparisons and the three projections. */
        const srVector3T<float>& extent = value_234;

        if (extent.x == g_float_005ebb34 && extent.y == g_float_005ebb34 &&
            extent.z == g_float_005ebb34) {
            visibility =
                renderer->testBoundingSphere(position, value_278 * value_240);
        }
        else {
            getRotation(rotation);
            float x = Function4218E0(rotation.vectors[0], extent);
            float y = Function4218E0(rotation.vectors[1], extent);
            float z = Function4218E0(rotation.vectors[2], extent);
            srVector3T<float> center(
                x + position.x, position.y + y, position.z + z);
            visibility =
                renderer->testBoundingSphere(center, value_278 * value_240);
        }
        if (visibility == srGERD::VISIBILITY_POSITIONAL_0) {
            return;
        }
    }

    if (value_1a4 == 1) {
        getRotation(rotation);
        float x = Function4218E0(rotation.vectors[0], minimum_21c);
        float y = Function4218E0(rotation.vectors[1], minimum_21c);
        float z = Function4218E0(rotation.vectors[2], minimum_21c);
        srVector3T<float> minimum(
            x + position.x, position.y + y, position.z + z);

        x = Function4218E0(rotation.vectors[0], maximum_228);
        y = Function4218E0(rotation.vectors[1], maximum_228);
        z = Function4218E0(rotation.vectors[2], maximum_228);
        srVector3T<float> maximum(
            x + position.x, position.y + y, position.z + z);

        srGERD::e_visibility visibility =
            renderer->testBoundingBox(minimum, maximum);
        if (visibility == srGERD::VISIBILITY_POSITIONAL_0) {
            return;
        }
    }

    /* Two indices per surviving particle, rebuilt only after a deactivation
       has marked the pairs stale. */
    if ((update_flags_250 & 2) != 0) {
        unsigned int written = 0;
        for (unsigned int index = 0; index < particle_count_180; ++index) {
            if (allocation_194[index] != 0) {
                allocation_254[written++] = index * 2;
                allocation_254[written++] = index * 2 + 1;
            }
        }
        update_flags_250 &= ~2u;
    }

    renderer->pushEnable();
    renderer->matrixMode(srGERD::MATRIX_MODE_POSITIONAL_0);

    srMatrix4T<float> view;
    renderer->getMatrix(srGERD::MATRIX_MODE_POSITIONAL_0, view);
    view.InvertMatrix0049BAB0();
    PrepareRenderer00498DD0(view);

    if (value_138 != 0 && !renderer->isEnabled(srGERD::ENABLE_POSITIONAL_1)) {
        renderer->toggle(srGERD::ENABLE_POSITIONAL_1);
    }
    renderer->setCullMode(srGERD::CULL_MODE_POSITIONAL_2);
    renderer->setPickKey(0);

    srTriMeshPipeline* pipeline = srTriMeshPipeline::Get004750A0(renderer);

    /* Three array/count pairs: the index pairs rebuilt above, the polygon
       index list, and the transformed vertex positions. */
    pipeline->value_2c = allocation_254;
    pipeline->value_24 = active_particle_count_18c * 2;
    pipeline->value_34 = allocation_168;
    pipeline->value_1c = texture_frame_count_15c;
    pipeline->value_38 = allocation_160;
    pipeline->value_20 = vertex_count_158;
    if (allocation_170 != 0) {
        pipeline->value_3c = allocation_170;
    }

    pipeline->current_record_14->flags_00 = 0;
    pipeline->current_pass_18->value_14 = 0;
    pipeline->current_pass_18->value_0c = 0;
    pipeline->current_pass_18->value_10 = 0;

    if (allocation_16c != 0) {
        pipeline->current_record_14->value_0c = allocation_16c;
        pipeline->current_record_14->value_10 = 1;
        pipeline->current_record_14->flags_00 |= 1;
    }
    if (allocation_174 != 0) {
        pipeline->current_record_14->value_1c = allocation_174;
        pipeline->current_record_14->flags_00 |= 8;
    }

    /* The retained object is the batch's material: the same pointer reaches
       both the pipeline and the record it is about to submit. */
    pipeline->material_80 = retained_14c;
    pipeline->current_record_14->material_08 = retained_14c;

    pipeline->SetFlags004752C0(render_flags_150);

    if (allocation_164 != 0) {
        pipeline->current_record_14->value_20 = allocation_164;
        pipeline->current_record_14->flags_00 |= 0x10;
    }

    if (texture_frames_178 != 0) {
        pipeline->current_pass_18->value_0c = texture_frames_178;
    }
    else {
        srTextureIFace* texture = texture_154;

        if (texture != 0) {
            pipeline->value_78 = texture;
            pipeline->current_pass_18->value_00 = texture;
        }
    }

    ++pipeline->slot_count_84;
    pipeline->PrepareSlot00475540();

    if (!renderer->isPickStackEmpty()) {
        srGERD::Pick pick;

        renderer->popPick(pick);
        pipeline->FlushIfCurrent();
        renderer->pushPick(pick);
    }
    else {
        pipeline->FlushIfCurrent();
    }
    renderer->popEnable();
}

// FUNCTION: WIZ8 0x00498A20
stParticle::~stParticle()
{
    if (allocation_148 != 0) {
        srHeap.free(allocation_148);
    }
    if (allocation_170 != 0) {
        srHeap.free(allocation_170);
    }
    if (allocation_164 != 0) {
        srHeap.free(allocation_164);
    }
    if (allocation_160 != 0) {
        srHeap.free(allocation_160);
    }
    if (allocation_168 != 0) {
        srHeap.free(allocation_168);
    }
    if (allocation_16c != 0) {
        srHeap.free(allocation_16c);
    }
    if (allocation_174 != 0) {
        ::operator delete(allocation_174);
    }
    if (retained_14c != 0) {
        retained_14c->release();
    }
    if (allocation_198 != 0) {
        srHeap.free(allocation_198);
    }
    if (allocation_19c != 0) {
        ::operator delete(allocation_19c);
    }
    if (allocation_254 != 0) {
        ::operator delete(allocation_254);
    }
    if (allocation_194 != 0) {
        ::operator delete(allocation_194);
    }
    if (texture_frames_178 != 0) {
        for (unsigned int i = 0; i < texture_frame_count_15c; i += 2) {
            texture_frames_178[i]->release();
        }
        ::operator delete(texture_frames_178);
    }
    if (m_pflFlutterAngle != 0) {
        ::operator delete(m_pflFlutterAngle);
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
            stTextureAnim* frame =
                new stTextureAnim(*static_cast<stTextureAnim*>(texture));
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
    if (active != 0 && active_1a0 == 0) {
        unsigned int now = g_shared_timer_base->getMsTime(
            srTimer::TIMER_READ_DEFAULT);
        activated_at_258 = now;
        updated_at_25c = now;
    }
    active_1a0 = active;
}

// FUNCTION: WIZ8 0x0049ac30
unsigned char stParticle::ReplaceTexture0049AC30(
    const char* old_name, srTextureIFace* replacement)
{
    if (texture_154 != 0 &&
        (texture_154->getClassID() == 0x10001 ||
         texture_154->getClassID() == 0x10000) &&
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

    value_1c0 = enabled;
    if (enabled == 0) {
        if (m_pflFlutterAngle != 0) {
            ::operator delete(m_pflFlutterAngle);
            m_pflFlutterAngle = 0;
        }
    }
    else if (m_pflFlutterAngle == 0) {
        m_pflFlutterAngle = new float[particle_count_180];
        if (m_pflFlutterAngle == 0) {
            srAssertFail("m_pflFlutterAngle", ST_PARTICLE_CPP, 1210, 0);
        }
        for (i = 0; i < particle_count_180; ++i) {
            m_pflFlutterAngle[i] = 0.0f;
        }
    }
}
