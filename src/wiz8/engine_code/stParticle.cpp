#include "wiz8/gameplay_boundaries.h"
#include "wiz8/engine_code/registry_classes.h"
#include "wiz8/engine_code/game_timer.h"
#include "wiz8/engine_code/stTextureAnim.h"
#include "wiz8/sr_api.h"
#include "wiz8/virtual_file.h"
#include "surrender/srCore.h"
#include "surrender/srHeap.h"
#include "surrender/srNode.h"
#include "FileMan.h"

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
srFlags<stParticle::e_renderFlag> stParticle::GetRenderFlags00498A10() const
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
    allocation_168 = static_cast<unsigned int*>(
        srHeap.allocate(count * 6 * sizeof(unsigned int)));
    allocation_174 = new float[vertex_count_158];
    texture_frames_178 = 0;

    for (i = 0; i < count; ++i) {
        unsigned int vertex = i * 4;
        unsigned int index = i * 6;
        allocation_168[index] = vertex;
        allocation_168[index + 1] = vertex + 1;
        allocation_168[index + 2] = vertex + 2;
        allocation_168[index + 3] = vertex + 2;
        allocation_168[index + 4] = vertex + 3;
        allocation_168[index + 5] = vertex;

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
    allocation_254 = new float[texture_frame_count_15c];
    active_particle_count_18c = 0;
    allocation_198 = static_cast<srVector3T<float>*>(
        srHeap.allocate(count * sizeof(srVector3T<float>)));
    allocation_19c = ::operator new(count * sizeof(void*));
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
    value_1e8 = 0.0f;
    value_1ec = -1.0f;
    value_1f0 = 0.0f;
    value_210 = 500.0f;
    value_1f4 = 0.0f;
    value_1f8 = -4905.0f;
    value_1fc = 0.0f;
    value_1c0 = 0;
    m_pflFlutterAngle = 0;
    value_200 = 0.0f;
    value_204 = 0.0f;
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
            if (info.entries.capacity <= info.entry_count) {
                info.entries.setCapacity(
                    info.entries.capacity + 8 + info.entry_count);
            }
            info.entries.data[info.entry_count].node = this;
            info.entries.data[info.entry_count].value = 0;
            ++info.entry_count;
        }
    }

    if (!testFlag(FLAG_POSITIONAL_1) && firstChild() != 0) {
        firstChild()->traverse(info);
    }
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
void stParticle::SetRetainedObject0049ACA0(srClass* object)
{
    if (retained_14c != 0) {
        retained_14c->release();
    }
    retained_14c = object;
    if (object != 0) {
        object->addReference();
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
