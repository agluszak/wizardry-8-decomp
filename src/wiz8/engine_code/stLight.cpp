#include "wiz8/float_constants.h"
#include "wiz8/engine_code/3d.h"
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

/* rand() normalization to a 0..1 flicker probability; only Update
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
    m_save_marked = 0;
    m_prop = 0;
    if (parent != 0) {
        setParent(parent, 0);
    }
    attenuation_model = srLight::ATTENUATION_3DSTUDIO_MAX;
    path_ai = 0;
    m_direction = 1;
    m_path_index = 0;
    m_path_direction = 1;
    m_position_228.SetZero();
    m_level = 0;
    m_definition = 0;
    m_unknown_238 = 0;
    m_level_time = m_path_time = GetTickCount() * 0.001f;
}

/* Exactly two owned members. The definition is released through its own
   virtual destructor, so ownership of every stLightDefinition form is
   stLight's; the path is released through the kind-guarded PathAI helper
   rather than the general one. Nothing else here is owned: the prop at +0x254
   and the parent link are both left to their owners. */
// FUNCTION: WIZ8 0x0049C430
stLight::~stLight()
{
    delete m_definition;
    if (path_ai != 0) {
        DestroyOwnedPathAI(path_ai);
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
    setParent(other.parent_, 0);
    m_position_228 = other.m_position_228;
    if (other.m_definition != 0) {
        m_definition = other.m_definition->Clone();
    } else {
        m_definition = 0;
    }
    m_unknown_238 = other.m_unknown_238;
    m_direction = other.m_direction;
    if (other.path_ai != 0) {
        path_ai = ClonePathAI(other.path_ai);
    } else {
        path_ai = 0;
    }
    m_path_index = other.m_path_index;
    m_path_direction = other.m_path_direction;
    m_level = other.m_level;
    m_level_time = m_path_time = GetTickCount() * 0.001f;
    m_prop = other.m_prop;
    m_save_marked = other.m_save_marked;
    return *this;
}

// FUNCTION: WIZ8 0x0049C7A0
void stLight::traverse(srNode::TraverseInfo& info)
{
    if (next_sibling_ != 0) {
        next_sibling_->traverse(info);
    }

    if (!testFlag(FLAG_TERMINATE)) {
        if (testFlag(FLAG_DISABLE) || fabs(intensity_1d0) <= g_double_005ebc70 ||
            (g_light_update_flags & 1) == 0) {
            if (first_child_ != 0) {
                first_child_->traverse(info);
            }
        } else if (m_definition != 0) {
            if (!testFlag(FLAG_GLOBAL)) {
                srNode::TraverseInfo::Entry& entry = info.entries[info.entry_count];
                entry.node = this;
                entry.value = 1;
                ++info.entry_count;
            } else {
                info.nodes[info.node_count] = this;
                ++info.node_count;
            }

            if (first_child_ != 0) {
                first_child_->traverse(info);
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
    if ((type == PROCESS_PUSH || type == PROCESS_PUSH_GLOBAL) &&
        g_monster_light_scale != g_float_one) {
        float saved_scale = intensity_1d0;
        intensity_1d0 = saved_scale * g_monster_light_scale;
        srLight::process(info, type);
        intensity_1d0 = saved_scale;
        return;
    }
    srLight::process(info, type);
}

// FUNCTION: WIZ8 0x0049C940
void stLight::SetDefinitionTime(float time)
{
    if (m_definition != 0 && m_definition->kind == W8_LIGHT_DEFINITION_KEYFRAMED) {
        static_cast<stKeyframedLightDefinition*>(m_definition)->time_4c = time;
    }
}

/* Advance the light's definition-driven state by one update. A keyframed
   (type 2) definition walks keyframe_index through the time table and lerps
   intensity and diffuse color between the surrounding keys; a flags-driven
   definition either oscillates intensity between intensity and
   intensity_to (optionally lerping color toward color_to_*), ramps it
   one way, or flickers the node disable flag and its prop's animated
   texture at a per-update probability. Any owned path advances once the
   elapsed seconds times the path rate exceed one whole step, wrapping or
   ping-ponging at the ends. */
// FUNCTION: WIZ8 0x0049C960
void stLight::Update()
{
    if (m_definition != 0 && m_definition->kind == W8_LIGHT_DEFINITION_KEYFRAMED) {
        stKeyframedLightDefinition* definition =
            static_cast<stKeyframedLightDefinition*>(m_definition);
        float time = definition->time_4c;
        int count = definition->key_frames.count;
        int last = count - 1;
        int* slot = definition->key_frames.GetAt(last);
        if (time <= *slot) {
            definition->keyframe_index = 0;
            if (definition->key_frames.count != 1 && -1 < definition->key_frames.count - 1) {
                do {
                    int next = definition->keyframe_index + 1;
                    slot = definition->key_frames.GetAt(next);
                    if (*slot <= time) {
                        definition->keyframe_index = next;
                    } else {
                        break;
                    }
                } while (definition->keyframe_index < count - 1);
            }
        } else {
            slot = definition->key_frames.GetAt(last);
            time = *slot;
            definition->keyframe_index = count - 2;
        }
        if (*definition->key_frames.data <= time) {
            int index = definition->keyframe_index;
            if (count - 2 <= index) {
                return;
            }
            int* from = definition->key_frames.GetAt(index);
            int* to = definition->key_frames.GetAt(index + 1);
            float span = *to - *from;
            float blend = g_float_one;
            if (span != g_float_zero) {
                blend = (time - *from) / span;
            }
            float* key_from = definition->key_intensities.GetAt(index);
            float* key_to = definition->key_intensities.GetAt(index + 1);
            float inverse = g_float_one - blend;
            intensity_1d0 = inverse * *key_from + blend * *key_to;
            index = definition->keyframe_index;
            srVector3T<float>* color_to = definition->key_colors.GetAt(index + 1);
            srVector3T<float>* color_from = definition->key_colors.GetAt(index);
            float red = color_from->x * inverse + color_to->x * blend;
            float green = color_from->y * inverse + color_to->y * blend;
            float blue = color_from->z * inverse + color_to->z * blend;
            if (g_float_one < red) {
                red = 1.0f;
            }
            if (g_float_one < green) {
                green = 1.0f;
            }
            if (g_float_one < blue) {
                blue = 1.0f;
            }
            diffuse_1a4.Set(red, green, blue);
            return;
        }
        intensity_1d0 = g_float_zero;
        return;
    }

    unsigned long ticks = GetTickCount();
    W8PathAI* path = path_ai;
    float seconds = ticks * g_float_005ec128;
    stParametricLightDefinition* definition =
        static_cast<stParametricLightDefinition*>(m_definition);
    if (definition->period < g_float_005ebc90) {
        definition->period = 1.0f;
    }
    unsigned int mode = definition->flags & 3;
    if (mode == 0) {
        float step = (seconds - m_level_time) * definition->rate;
        if (g_float_zero < step) {
            float level = m_level;
            step = (g_float_one / definition->period) * step;
            if (m_direction == 0) {
                level -= step;
                if (level < g_float_zero) {
                    m_direction = 1;
                    level = step + step + level;
                }
            } else {
                level = step + level;
                if (g_float_one < level) {
                    m_direction = 0;
                    level -= step + step;
                }
            }
            float blend = g_float_zero;
            if (g_float_zero < level) {
                blend = level;
                if (g_float_one <= level) {
                    blend = g_float_one;
                }
            }
            intensity_1d0 = (definition->intensity_to - definition->intensity) * blend +
                            definition->intensity;
            m_level = blend;
            if ((definition->flags & 8) == 0) {
                m_level_time = seconds;
            } else {
                float inverse = g_float_one - blend;
                float red = blend * definition->color_to.x + inverse * definition->color.x;
                float green = blend * definition->color_to.y + inverse * definition->color.y;
                float blue = blend * definition->color_to.z + inverse * definition->color.z;
                if (g_float_one < red) {
                    red = 1.0f;
                }
                if (g_float_one < green) {
                    green = 1.0f;
                }
                if (g_float_one < blue) {
                    blue = 1.0f;
                }
                diffuse_1a4.Set(red, green, blue);
                m_level_time = seconds;
            }
        }
    } else if (mode == 3) {
        float step = (seconds - m_level_time) * definition->rate;
        if (g_float_zero < step) {
            float blend = (g_float_one / definition->period) * step + m_level;
            if (blend <= g_float_one) {
                float level = blend;
                intensity_1d0 = (definition->intensity_to - definition->intensity) * level +
                                definition->intensity;
                m_level = level;
                if ((definition->flags & 8) != 0) {
                    float inverse = g_float_one - blend;
                    float red =
                        blend * definition->color_to.x + inverse * definition->color.x;
                    float green =
                        blend * definition->color_to.y + inverse * definition->color.y;
                    float blue =
                        blend * definition->color_to.z + inverse * definition->color.z;
                    if (g_float_one < red) {
                        red = 1.0f;
                    }
                    if (g_float_one < green) {
                        green = 1.0f;
                    }
                    if (g_float_one < blue) {
                        blue = 1.0f;
                    }
                    diffuse_1a4.Set(red, green, blue);
                }
                m_level_time = seconds;
            }
        }
    } else if ((definition->flags & 1) == 1) {
        W8Prop* prop = m_prop;
        srModelInstance* instance = 0;
        if (prop != 0) {
            instance = prop->ToggleRepAnimationDefault();
            srMeshModel* model = static_cast<srMeshModel*>(instance->getModel());
            if (MeshHasAnimatedTexture(model) == 0) {
                m_prop = 0;
                instance = 0;
            }
        }
        if ((definition->flags & 8) != 0) {
            diffuse_1a4 = definition->color;
        }
        if (testFlag(FLAG_DISABLE) == 0) {
            setFlag(FLAG_DISABLE);
            if (instance != 0) {
                SetModelAnimatedTextureFrame(instance, 1);
            }
        } else {
            int roll = rand();
            if (roll * g_float_005ec1e4 < definition->flicker_chance) {
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
        ((seconds - m_path_time) * definition->path_speed < g_float_one)) {
        return;
    }
    int index = static_cast<int>((seconds - m_path_time) * definition->path_speed);
    index = index * m_path_direction + m_path_index;
    if (index < static_cast<int>(PathAIEntryCount(path))) {
        if (index < 0) {
            m_path_direction = 1;
            index = 0;
        }
    } else if ((definition->flags & 0x20) != 0) {
        m_path_direction = -1;
        index = static_cast<int>(PathAIEntryCount(path)) - 2;
    } else {
        index = 0;
    }
    PathAISetValue(path, static_cast<float>(index));
    PathAIApply(path, this);
    m_path_index = index;
    m_path_time = seconds;
}

/* Restart the light-definition driven state when a Monster switches cycles.
   The definition at +0x234 is owned by stLight; type two resets its own pair
   of counters, while the other forms restore intensity and an optional
   colour. */
// FUNCTION: WIZ8 0x0049D070
void stLight::Reset()
{
    if (m_definition != 0) {
        if (m_definition->kind == W8_LIGHT_DEFINITION_KEYFRAMED) {
            stKeyframedLightDefinition* definition =
                static_cast<stKeyframedLightDefinition*>(m_definition);
            definition->time_4c = 0.0f;
            definition->keyframe_index = 0;
            m_path_index = 0;
            m_path_direction = 1;
        } else {
            stParametricLightDefinition* definition =
                static_cast<stParametricLightDefinition*>(m_definition);
            intensity_1d0 = definition->intensity;
            if ((definition->flags & 8) != 0) {
                diffuse_1a4 = definition->color;
            }
        }
    }

    m_level = 0;
    m_path_time = GetTickCount() * 0.001f;
    m_level_time = m_path_time;
}

/* Serialize the registered positional lights: a version byte and count
   followed by each light's 0x80-byte name and its disable flag. */
// FUNCTION: WIZ8 0x0049D120
void SaveLightStates(int handle)
{
    char name[0x80] = {0};
    memcpy(name, &g_empty_ambient_name, sizeof(g_empty_ambient_name));
    unsigned char version = 1;
    int count = 0;

    FileWrite(handle, &version, sizeof(version), 0);

    stLight* light = static_cast<stLight*>(srCore.getRegistry()->find(
        stLight::sGetClassNode(), static_cast<const srRuntimeClass*>(0)));
    while (light != 0) {
        if (light->m_save_marked != 0) {
            ++count;
        }
        light = static_cast<stLight*>(srCore.getRegistry()->find(stLight::sGetClassNode(), light));
    }

    FileWrite(handle, &count, sizeof(count), 0);

    light = static_cast<stLight*>(srCore.getRegistry()->find(
        stLight::sGetClassNode(), static_cast<const srRuntimeClass*>(0)));
    while (light != 0) {
        if (light->m_save_marked != 0) {
            strcpy(name, light->getName());
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
    char name[0x80] = {0};
    memcpy(name, &g_empty_ambient_name, sizeof(g_empty_ambient_name));
    unsigned char version;
    int count = 0;

    FileRead(handle, &version, sizeof(version), 0);
    FileRead(handle, &count, sizeof(count), 0);

    for (int index = 0; index < count; ++index) {
        unsigned char enabled;
        FileRead(handle, name, sizeof(name), 0);
        FileRead(handle, &enabled, sizeof(enabled), 0);

        stLight* light =
            static_cast<stLight*>(srCore.getRegistry()->find(stLight::sGetClassNode(), name, 0));
        if (light != 0) {
            if (enabled != 0) {
                light->clearFlag(srNode::FLAG_DISABLE);
            } else {
                light->setFlag(srNode::FLAG_DISABLE);
            }
        }
    }
}

/* The registry base's own destructor, emitted out of line here rather than
   inlined the way 0x0049C430 expands it. */

// FUNCTION: WIZ8 0x0049E3A0
srClass* stLight::vInstance()
{
    return new stLight(0);
}

/* Test a point against the six inward-facing planes of one region volume. */
// FUNCTION: WIZ8 0x0049e460
unsigned char W8OctRegionVolume::ContainsPoint(const srVector3T<float>* point) const
{
    return PointInsideFrustum(point, m_planes);
}
