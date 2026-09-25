#include "wiz8/float_constants.h"
#include "wiz8/engine_code/AmbientSound.h"
#include "wiz8/engine_code/stLight.hpp"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/PathAI.h"
#include "wiz8/engine_code/OctPreTree.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/engine_code/materials.h"
#include "wiz8/engine_code/GrCycle.h"
#include "wiz8/engine_code/stModelInstance.h"
#include "wiz8/geometry.h"
#include "surrender/srModelInstance.h"
#include "surrender/srMeshModel.h"
#include "surrender/srCore.h"

#include "FileMan.h"

#include <windows.h>
#include <new>
#include <stdlib.h>
#include <string.h>

/* The previous 00497af0-004adb20 range spanned eight units that carry their
   own assertion-backed intervals -
   stParticle, OctSubMesh, Item, AnimObj, Missile, GrCycle, PathAI and Spells -
   and every body actually in this file belongs to the single unnamed unit that
   sits between them. The upper bound is OctSubMesh.cpp's interval lower bound
   less one; the lower bound is one past stParticle.cpp's last emission at
   0x0049BA50, since that unit runs past its own assertion interval. No CPP path
   string in the image names this unit; the class ownership is nevertheless
   established by its definitions. */

// GLOBAL: WIZ8 0x0060bfdc
unsigned int g_light_update_flags = 1;

/* rand() normalization to a 0..1 flicker probability; only Update0049C960
   uses it. */
// GLOBAL: WIZ8 0x005ec1e4
const float g_float_005ec1e4 = 3.0518509447574615e-05f;

/* The parent-taking constructor never forwards the parent to srLight: the base
   runs with its own defaults and the node is linked afterwards, which is why
   the emitted body carries an explicit null test around setParent. Everything
   below +0x228 is stLight's own state, and both timestamps start from the same
   converted tick. */
// FUNCTION: WIZ8 0x0049C2C0
stLight::stLight(srNode* parent)
{
    m_save_marked_23a = 0;
    m_prop_254 = 0;
    if (parent != 0) {
        setParent(parent, 0);
    }
    attenuation_model_150 = srLight::ATTENUATION_3DSTUDIO_MAX;
    m_owned_244 = 0;
    m_direction_239 = 1;
    m_path_index_248 = 0;
    m_path_direction_250 = 1;
    m_position_228.SetZero();
    m_level_240 = 0;
    m_definition_234 = 0;
    m_padding_238 = 0;
    m_level_time_23c = m_path_time_24c = GetTickCount() * 0.0025f;
}

/* Exactly two owned members. The definition is released through its own
   virtual destructor, so ownership of every stLightDefinition form is
   stLight's; the path is released through the kind-guarded PathAI helper
   rather than the general one. Nothing else here is owned: the prop at +0x254
   and the parent link are both left to their owners. */
// FUNCTION: WIZ8 0x0049C430
stLight::~stLight()
{
    delete m_definition_234;
    if (m_owned_244 != 0) {
        DestroyOwnedPathAI(m_owned_244);
    }
}

/* Assignment deep-copies both owned members - the definition through its
   virtual Clone slot and the path through the PathAI clone - so a copied light
   shares nothing with its source except the prop reference and the parent it
   is re-linked under. The two timestamps restart from the current tick instead
   of being copied. */
// FUNCTION: WIZ8 0x0049C690
stLight& stLight::operator=(const stLight& other)
{
    srLight::operator=(other);
    setParent(other.parentNode(), 0);
    m_position_228 = other.m_position_228;
    if (other.m_definition_234 != 0) {
        m_definition_234 = other.m_definition_234->Clone();
    } else {
        m_definition_234 = 0;
    }
    m_padding_238 = other.m_padding_238;
    m_direction_239 = other.m_direction_239;
    if (other.m_owned_244 != 0) {
        m_owned_244 = ClonePathAI(other.m_owned_244);
    } else {
        m_owned_244 = 0;
    }
    m_path_index_248 = other.m_path_index_248;
    m_path_direction_250 = other.m_path_direction_250;
    m_level_240 = other.m_level_240;
    m_level_time_23c = m_path_time_24c = GetTickCount() * 0.0025f;
    m_prop_254 = other.m_prop_254;
    m_save_marked_23a = other.m_save_marked_23a;
    return *this;
}

// FUNCTION: WIZ8 0x0049C7A0
void stLight::traverse(srNode::TraverseInfo& info)
{
    if (nextSibling() != 0) {
        nextSibling()->traverse(info);
    }

    if (!testFlag(FLAG_TERMINATE)) {
        if (testFlag(FLAG_DISABLE) || fabs(intensity_1d0) <= g_double_005ebc70 ||
            (g_light_update_flags & 1) == 0) {
            if (firstChild() != 0) {
                firstChild()->traverse(info);
            }
        } else if (m_definition_234 != 0) {
            if (!testFlag(FLAG_GLOBAL)) {
                srNode::TraverseInfo::Entry& entry = info.entries[info.entry_count];
                entry.node = this;
                entry.value = 1;
                ++info.entry_count;
            } else {
                info.nodes[info.node_count] = this;
                ++info.node_count;
            }

            if (firstChild() != 0) {
                firstChild()->traverse(info);
            }

            if (!testFlag(FLAG_GLOBAL)) {
                srNode::TraverseInfo::Entry& entry = info.entries[info.entry_count];
                entry.node = this;
                entry.value = 2;
                ++info.entry_count;
            }
        }
    }
}

// FUNCTION: WIZ8 0x0049C8D0
void stLight::process(const srNode::ProcessInfo& info, srNode::e_processType type)
{
    /* The recovered e_processType currently names only 0; retail still
       compares this override against 1 and 3. Enumerator names remain
       unknown, so keep the integer tests rather than inventing them. */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-compare"
    if ((type == 1 || type == 3) && g_light_scale_0060bfe0 != g_float_005ebb38) {
#pragma clang diagnostic pop
        float saved_scale = intensity_1d0;
        intensity_1d0 = saved_scale * g_light_scale_0060bfe0;
        srLight::process(info, type);
        intensity_1d0 = saved_scale;
        return;
    }
    srLight::process(info, type);
}

// FUNCTION: WIZ8 0x0049C940
void stLight::SetDefinitionTime(float time)
{
    if (m_definition_234 != 0 && m_definition_234->type_04 == 2) {
        static_cast<stKeyframedLightDefinition*>(m_definition_234)->time_4c = time;
    }
}

/* Advance the light's definition-driven state by one update. A keyframed
   (type 2) definition walks keyframe_index_48 through the time table and lerps
   intensity and diffuse color between the surrounding keys; a flags-driven
   definition either oscillates intensity between intensity_28 and
   intensity_to_2c (optionally lerping color toward color_to_*), ramps it
   one way, or flickers the node disable flag and its prop's animated
   texture at a per-update probability. Any owned path advances once the
   elapsed seconds times the path rate exceed one whole step, wrapping or
   ping-ponging at the ends. */
// FUNCTION: WIZ8 0x0049C960
void stLight::Update0049C960()
{
    if (m_definition_234 != 0 && m_definition_234->type_04 == 2) {
        stKeyframedLightDefinition* definition =
            static_cast<stKeyframedLightDefinition*>(m_definition_234);
        float time = definition->time_4c;
        int count = definition->values_18.count;
        int last = count - 1;
        int* slot = definition->values_18.GetAt(last);
        if (time <= *slot) {
            definition->keyframe_index_48 = 0;
            if (definition->values_18.count != 1 && -1 < definition->values_18.count - 1) {
                do {
                    int next = definition->keyframe_index_48 + 1;
                    slot = definition->values_18.GetAt(next);
                    if (*slot <= time) {
                        definition->keyframe_index_48 = next;
                    } else {
                        break;
                    }
                } while (definition->keyframe_index_48 < count - 1);
            }
        } else {
            slot = definition->values_18.GetAt(last);
            time = *slot;
            definition->keyframe_index_48 = count - 2;
        }
        if (*definition->values_18.data <= time) {
            int index = definition->keyframe_index_48;
            if (count - 2 <= index) {
                return;
            }
            int* from = definition->values_18.GetAt(index);
            int* to = definition->values_18.GetAt(index + 1);
            float span = *to - *from;
            float blend = g_float_005ebb38;
            if (span != g_float_005ebb34) {
                blend = (time - *from) / span;
            }
            float* key_from = definition->values_28.GetAt(index);
            float* key_to = definition->values_28.GetAt(index + 1);
            float inverse = g_float_005ebb38 - blend;
            intensity_1d0 = inverse * *key_from + blend * *key_to;
            index = definition->keyframe_index_48;
            srVector3T<float>* color_to = definition->values_38.GetAt(index + 1);
            srVector3T<float>* color_from = definition->values_38.GetAt(index);
            float red = color_from->x * inverse + color_to->x * blend;
            float green = color_from->y * inverse + color_to->y * blend;
            float blue = color_from->z * inverse + color_to->z * blend;
            if (g_float_005ebb38 < red) {
                red = 1.0f;
            }
            if (g_float_005ebb38 < green) {
                green = 1.0f;
            }
            if (g_float_005ebb38 < blue) {
                blue = 1.0f;
            }
            diffuse_1a4.x = red;
            diffuse_1a4.y = green;
            diffuse_1a4.z = blue;
            return;
        }
        intensity_1d0 = g_float_005ebb34;
        return;
    }

    unsigned long ticks = GetTickCount();
    W8PathAI* path = m_owned_244;
    float seconds = ticks * g_float_005ec128;
    stParametricLightDefinition* definition =
        static_cast<stParametricLightDefinition*>(m_definition_234);
    if (definition->period_30 < g_float_005ebc90) {
        definition->period_30 = 1.0f;
    }
    unsigned int mode = definition->flags_08 & 3;
    if (mode == 0) {
        float step = (seconds - m_level_time_23c) * definition->rate_34;
        if (g_float_005ebb34 < step) {
            float level = m_level_240;
            step = (g_float_005ebb38 / definition->period_30) * step;
            if (m_direction_239 == '\0') {
                level = level - step;
                if (level < g_float_005ebb34) {
                    m_direction_239 = 1;
                    level = step + step + level;
                }
            } else {
                level = step + level;
                if (g_float_005ebb38 < level) {
                    m_direction_239 = 0;
                    level = level - (step + step);
                }
            }
            float blend = g_float_005ebb34;
            if (g_float_005ebb34 < level) {
                blend = level;
                if (g_float_005ebb38 <= level) {
                    blend = g_float_005ebb38;
                }
            }
            intensity_1d0 = (definition->intensity_to_2c - definition->intensity_28) * blend +
                            definition->intensity_28;
            m_level_240 = blend;
            if ((definition->flags_08 & 8) == 0) {
                m_level_time_23c = seconds;
            } else {
                float inverse = g_float_005ebb38 - blend;
                float red = blend * definition->color_to_1c.x + inverse * definition->color_10.x;
                float green = blend * definition->color_to_1c.y + inverse * definition->color_10.y;
                float blue = blend * definition->color_to_1c.z + inverse * definition->color_10.z;
                if (g_float_005ebb38 < red) {
                    red = 1.0f;
                }
                if (g_float_005ebb38 < green) {
                    green = 1.0f;
                }
                if (g_float_005ebb38 < blue) {
                    blue = 1.0f;
                }
                diffuse_1a4.x = red;
                diffuse_1a4.y = green;
                diffuse_1a4.z = blue;
                m_level_time_23c = seconds;
            }
        }
    } else if (mode == 3) {
        float step = (seconds - m_level_time_23c) * definition->rate_34;
        if (g_float_005ebb34 < step) {
            float blend = (g_float_005ebb38 / definition->period_30) * step + m_level_240;
            if (blend <= g_float_005ebb38) {
                float level = blend;
                intensity_1d0 = (definition->intensity_to_2c - definition->intensity_28) * level +
                                definition->intensity_28;
                m_level_240 = level;
                if ((definition->flags_08 & 8) != 0) {
                    float inverse = g_float_005ebb38 - blend;
                    float red =
                        blend * definition->color_to_1c.x + inverse * definition->color_10.x;
                    float green =
                        blend * definition->color_to_1c.y + inverse * definition->color_10.y;
                    float blue =
                        blend * definition->color_to_1c.z + inverse * definition->color_10.z;
                    if (g_float_005ebb38 < red) {
                        red = 1.0f;
                    }
                    if (g_float_005ebb38 < green) {
                        green = 1.0f;
                    }
                    if (g_float_005ebb38 < blue) {
                        blue = 1.0f;
                    }
                    diffuse_1a4.x = red;
                    diffuse_1a4.y = green;
                    diffuse_1a4.z = blue;
                }
                m_level_time_23c = seconds;
            }
        }
    } else if ((definition->flags_08 & 1) == 1) {
        W8Prop* prop = m_prop_254;
        srModelInstance* instance = 0;
        if (prop != 0) {
            instance = prop->ToggleRepAnimationDefault();
            srMeshModel* model = static_cast<srMeshModel*>(instance->model());
            if (MeshHasAnimatedTexture(model) == 0) {
                m_prop_254 = 0;
                instance = 0;
            }
        }
        if ((definition->flags_08 & 8) != 0) {
            diffuse_1a4 = definition->color_10;
        }
        if (testFlag(FLAG_DISABLE) == 0) {
            setFlag(FLAG_DISABLE);
            if (instance != 0) {
                SetModelAnimatedTextureFrame(instance, 1);
            }
        } else {
            int roll = rand();
            if (roll * g_float_005ec1e4 < definition->flicker_chance_0c) {
                if (testFlag(FLAG_DISABLE) == 0) {
                    setFlag(FLAG_DISABLE);
                    if (instance != 0) {
                        SetModelAnimatedTextureFrame(instance, 1);
                    }
                } else {
                    clearFlag(FLAG_DISABLE);
                    if (instance != 0) {
                        SetModelAnimatedTextureFrame(instance, 0);
                    }
                }
            }
        }
    }
    if ((path == 0) || (PathAIEntryCount(path) == 0) ||
        ((seconds - m_path_time_24c) * definition->path_speed_38 < g_float_005ebb38)) {
        return;
    }
    int index = static_cast<int>((seconds - m_path_time_24c) * definition->path_speed_38);
    index = index * m_path_direction_250 + m_path_index_248;
    if (index < static_cast<int>(PathAIEntryCount(path))) {
        if (index < 0) {
            m_path_direction_250 = 1;
            index = 0;
        }
    } else if ((definition->flags_08 & 0x20) != 0) {
        m_path_direction_250 = -1;
        index = static_cast<int>(PathAIEntryCount(path)) - 2;
    } else {
        index = 0;
    }
    PathAISetValue(path, static_cast<float>(index));
    PathAIApply(path, this);
    m_path_index_248 = index;
    m_path_time_24c = seconds;
}

/* Restart the light-definition driven state when a Monster switches cycles.
   The definition at +0x234 is owned by stLight; type two resets its own pair
   of counters, while the other forms restore intensity and an optional
   colour. */
// FUNCTION: WIZ8 0x0049D070
void stLight::Reset0049D070()
{
    if (m_definition_234 != 0) {
        if (m_definition_234->type_04 == 2) {
            stKeyframedLightDefinition* definition =
                static_cast<stKeyframedLightDefinition*>(m_definition_234);
            definition->time_4c = 0.0f;
            definition->keyframe_index_48 = 0;
            m_path_index_248 = 0;
            m_path_direction_250 = 1;
        } else {
            stParametricLightDefinition* definition =
                static_cast<stParametricLightDefinition*>(m_definition_234);
            intensity_1d0 = definition->intensity_28;
            if ((definition->flags_08 & 8) != 0) {
                diffuse_1a4 = definition->color_10;
            }
        }
    }

    m_level_240 = 0;
    m_path_time_24c = GetTickCount() * 0.0025f;
    m_level_time_23c = m_path_time_24c;
}

/* Serialize the registered positional lights: a version byte and count
   followed by each light's 0x80-byte name and its disable flag. */
// FUNCTION: WIZ8 0x0049D120
void SaveLightStates(int handle)
{
    unsigned short name[0x40] = {g_empty_ambient_name};
    unsigned char version = 1;
    int count = 0;

    FileWrite(handle, &version, sizeof(version), 0);

    stLight* light = static_cast<stLight*>(srCore.getRegistry()->find(
        stLight::sGetClassNode(), static_cast<const srRuntimeClass*>(0)));
    while (light != 0) {
        if (light->m_save_marked_23a != 0) {
            ++count;
        }
        light = static_cast<stLight*>(srCore.getRegistry()->find(stLight::sGetClassNode(), light));
    }

    FileWrite(handle, &count, sizeof(count), 0);

    light = static_cast<stLight*>(srCore.getRegistry()->find(
        stLight::sGetClassNode(), static_cast<const srRuntimeClass*>(0)));
    while (light != 0) {
        if (light->m_save_marked_23a != 0) {
            strcpy(reinterpret_cast<char*>(name), // reinterpret-ok: the
                   // 0x80-byte save field stores the narrow name packed as
                   // bytes
                   light->getName());
            FileWrite(handle, name, sizeof(name), 0);
            unsigned char enabled = light->testFlag(srNode::FLAG_DISABLE) == 0;
            FileWrite(handle, &enabled, sizeof(enabled), 0);
        }
        light = static_cast<stLight*>(srCore.getRegistry()->find(stLight::sGetClassNode(), light));
    }
}

/* Reload the serialized positional-light states: a version byte and count
   followed by each light's 0x80-byte name and its enable flag, applied to the
   light found by name in the registry. */
// FUNCTION: WIZ8 0x0049D390
void LoadLightStates(int handle)
{
    unsigned short name[0x40] = {g_empty_ambient_name};
    unsigned char version;
    int count = 0;

    FileRead(handle, &version, sizeof(version), 0);
    FileRead(handle, &count, sizeof(count), 0);

    for (int index = 0; index < count; ++index) {
        unsigned char enabled;
        FileRead(handle, name, sizeof(name), 0);
        FileRead(handle, &enabled, sizeof(enabled), 0);

        stLight* light = static_cast<stLight*>(srCore.getRegistry()->find(
            stLight::sGetClassNode(),
            reinterpret_cast<char*>(name), /* reinterpret-ok: the 0x80-byte save
                field stores the narrow name packed as bytes */
            0));
        if (light != 0) {
            if (enabled != 0) {
                light->clearFlag(srNode::FLAG_DISABLE);
            } else {
                light->setFlag(srNode::FLAG_DISABLE);
            }
        }
    }
}

// TEMPLATE: WIZ8 0x0049DB10
// srClassSupport<srIlluminator,srNode,0,4608>::getClassID

// TEMPLATE: WIZ8 0x0049DB30
// srClassSupport<srIlluminator,srNode,0,4608>::getClassNode

// TEMPLATE: WIZ8 0x0049DBA0
// srClassSupport<srIlluminator,srNode,0,4608>::clone

// TEMPLATE: WIZ8 0x0049DFE0
// srClassSupport<srIlluminator,srNode,0,4608>::~srClassSupport

// SYNTHETIC: WIZ8 0x0049DFB0
// srClassSupport<srIlluminator,srNode,0,4608>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x0049DC60
// srClassSupport<stLight,srLight,0,65542>::getClassID

// TEMPLATE: WIZ8 0x0049DC70
// srClassSupport<stLight,srLight,0,65542>::getClassName

// TEMPLATE: WIZ8 0x0049DC80
// srClassSupport<stLight,srLight,0,65542>::getClassNode

// TEMPLATE: WIZ8 0x0049DD60
// srClassSupport<stLight,srLight,0,65542>::clone

/* The registry base's own destructor, emitted out of line here rather than
   inlined the way 0x0049C430 expands it. */
// TEMPLATE: WIZ8 0x0049DD80
// srClassSupport<stLight,srLight,0,65542>::~srClassSupport

// SYNTHETIC: WIZ8 0x0049E260
// srClassSupport<stLight,srLight,0,65542>::`scalar deleting destructor'

// FUNCTION: WIZ8 0x0049E3A0
srClass* stLight::vInstance()
{
    return new stLight(0);
}

// SYNTHETIC: WIZ8 0x0049E400
// stLight::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x0049E450
// stLight::`scalar deleting destructor'`adjustor{312}'

/* Test a point against the six inward-facing planes of one region volume. */
// FUNCTION: WIZ8 0x0049e460
unsigned char W8OctRegionVolume::ContainsPoint0049E460(const srVector3T<float>* point) const
{
    for (short plane = 0; plane < 6; ++plane) {
        float distance = SignedPlaneDistance(planes_88[plane], *point);
        if (distance < g_float_005ebb34) {
            return 0;
        }
    }
    return 1;
}

// SYNTHETIC: WIZ8 0x004A2200
// stParametricLightDefinition::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x0049E290
// srArray<srNode*>::setCapacity
