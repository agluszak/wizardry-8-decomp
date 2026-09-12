#include "wiz8/engine_code/3d.h"
#include "wiz8/engine_code/Item.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/stHash.hpp"
#include "wiz8/engine_code/stModelInstance.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/Environment.h"
#include "wiz8/engine_code/stLight.hpp"
#include "wiz8/engine_code/stMeshModel.h"
#include "wiz8/float_constants.h"
#include "wiz8/sr_api.h"
#include "surrender/srCamera.h"
#include "surrender/srIlluminator.h"
#include "surrender/srScene.h"

#include <new>

#define THREE_D_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\3d.cpp"

// FUNCTION: WIZ8 0x0046DD70
void UpdateWorldMonsters0046DD70(W8World* world)
{
    if (world == 0) {
        srAssertFail("pWorld", THREE_D_CPP, 0x10d, 0);
    }
    if (world->plsMonsters == 0) {
        srAssertFail("pWorld->plsMonsters", THREE_D_CPP, 0x10e, 0);
    }

    srVector3T<double> camera_location = world->camera->getLocation();
    srVector3T<float> position;
    position = camera_location;

    unsigned int count = PLLength(world->plsMonsters);
    UpdateNearestMonsterGroupMembers004CA570();
    for (int index = 0; index < static_cast<int>(count); ++index) {
        W8Monster* monster = static_cast<W8Monster*>(PLGet(world->plsMonsters, index));
        if (monster != 0) {
            monster->DetachRepresentation004A7A70(world);
            monster->SelectLOD004A7BE0(&position);
            monster->Update();
            monster->UpdateRepresentation(world);
        }
    }
}

/* Detach every item mesh in one world, refresh its animation, and reattach it
   to the world's dynamic scene before the renderer transition. */
// FUNCTION: WIZ8 0x0046de40
void DetachWorldItemMeshes0046DE40(W8World* world)
{
    if (world == 0) {
        srAssertFail("pWorld", THREE_D_CPP, 0x135, 0);
    }
    if (world->plsItems == 0) {
        srAssertFail("pWorld->plsItems", THREE_D_CPP, 0x136, 0);
    }
    unsigned int count = PLLength(world->plsItems);
    for (int index = 0; index < static_cast<int>(count); ++index) {
        W8Item* item = static_cast<W8Item*>(PLGet(world->plsItems, index));
        if (item != 0) {
            item->DetachMesh0049FA30(world);
            item->UpdateAnimation0049F730();
            item->AttachMesh0049F900(world);
        }
    }
}

/* Remove every light held in one world's list, drop it from the render update
   set, and release it. */
// FUNCTION: WIZ8 0x0046e4a0
void DestroyWorldLights0046E4A0(W8World* world)
{
    W8PList* lights = &world->m_lights_0a8;

    while (PLLength(lights) != 0) {
        stLight* light = static_cast<stLight*>(PLGet(lights, 0));
        if (world == 0) {
            srAssertFail("pWorld", THREE_D_CPP, 0x278, 0);
        }
        if (light == 0) {
            srAssertFail("pLight", THREE_D_CPP, 0x279, 0);
        }
        PListRemove(lights, light);
        world->lights_to_update->RemoveAt(world->lights_to_update->IndexOf(light));
        if (light != 0) {
            light->release();
        }
    }
}

/* Put static-scene illuminators in group one and the world's camera light in
   group two.  The scene graph access is the ordinary srNode hierarchy API. */
// FUNCTION: WIZ8 0x0046F3A0
void FinalizeStaticScene0046F3A0(srScene* scene)
{
    srNode* node;

    for (node = scene->firstChild(); node != 0; node = node->nextSibling()) {
        if (node->getClassID() == 0x1220) {
            static_cast<srIlluminator*>(node)->setGroupMask(1);
        }
    }
    if (g_world != 0 && g_world->camera_light != 0) {
        g_world->camera_light->setGroupMask(2);
    }
}

// FUNCTION: WIZ8 0x0046F510
void ExpandBounds0046F510(srVector3T<float>* minimum, srVector3T<float>* maximum,
                          const srVector3T<float>* candidate_minimum,
                          const srVector3T<float>* candidate_maximum)
{
    minimum->x = minimum->x < candidate_minimum->x ? minimum->x : candidate_minimum->x;
    minimum->y = minimum->y < candidate_minimum->y ? minimum->y : candidate_minimum->y;
    minimum->z = minimum->z < candidate_minimum->z ? minimum->z : candidate_minimum->z;
    maximum->x = maximum->x > candidate_maximum->x ? maximum->x : candidate_maximum->x;
    maximum->y = maximum->y > candidate_maximum->y ? maximum->y : candidate_maximum->y;
    maximum->z = maximum->z > candidate_maximum->z ? maximum->z : candidate_maximum->z;
}

/* Build the concrete first-party instance used for an already-created mesh.
   Its model assignment goes through srModel::Client, exactly as the ordinary
   SurRender ownership interface requires. */
// FUNCTION: WIZ8 0x0046F5C0
stModelInstance* CreateModelInstance0046F5C0(stMeshModel* model)
{
    stModelInstance* instance;

    if (model == 0) {
        srAssertFail("pstMeshModel", THREE_D_CPP, 0x5bf, 0);
    }
    instance = new stModelInstance(0);
    if (instance == 0) {
        srAssertFail("pstHeadInstance", THREE_D_CPP, 0x5c2, 0);
    }
    instance->setName("ST CreateInstance");
    instance->assignModel(model);
    return instance;
}

// FUNCTION: WIZ8 0x0046F680
stModelInstance* DuplicateModelInstance0046F680(stModelInstance* instance)
{
    stModelInstance* copy;

    if (instance == 0) {
        srAssertFail("pstModelInstance", THREE_D_CPP, 0x5ce, 0);
    }
    copy = new stModelInstance(0);
    if (copy == 0) {
        srAssertFail("pstHeadInstance", THREE_D_CPP, 0x5d1, 0);
    }
    *copy = *instance;
    copy->setName("ST CreateInstance");
    copy->assignModel(instance->model());
    return copy;
}

// FUNCTION: WIZ8 0x0046DF90
stLight* CreateLight0046DF90(srNode* parent, const char* name)
{
    stLight* light = new stLight(parent);

    if (light != 0) {
        light->setName(name);
        light->attenuation_model_150 = srLight::ATTENUATION_3DSTUDIO_MAX;
        light->enable_flags_194 |= 0x10; /* ENABLE_RANGE_FAR */
        light->enable_flags_194 |= 4;    /* ENABLE_BOUNDING_SPHERE */
    }
    return light;
}

// FUNCTION: WIZ8 0x0046E030
stLight* CreateWorldLight0046E030(W8World* world, const char* name)
{
    stLight* light;

    if (name == 0) {
        srAssertFail("name", THREE_D_CPP, 570, 0);
    }

    if (world != 0) {
        light = new stLight(world->dynamic_scene);
    } else {
        light = new stLight(0);
    }

    if (light == 0) {
        srAssertFail("pLight", THREE_D_CPP, 579, 0);
    }
    light->setName(name);
    light->attenuation_model_150 = srLight::ATTENUATION_3DSTUDIO_MAX;
    light->enable_flags_194 |= 0x10; /* ENABLE_RANGE_FAR */
    light->enable_flags_194 |= 4;    /* ENABLE_BOUNDING_SPHERE */

    if (world != 0) {
        PLAdoptAppend(&world->m_lights_0a8, light);
    }
    return light;
}

// FUNCTION: WIZ8 0x0046E140
stLight* CreateWorldLight0046E140(W8World* world, const char* name)
{
    stLight* light;

    if (name == 0) {
        srAssertFail("name", THREE_D_CPP, 603, 0);
    }
    if (world == 0) {
        srAssertFail("pWorld", THREE_D_CPP, 604, 0);
    }

    light = CreateLight0046DF90(world->dynamic_scene, name);
    if (light == 0) {
        srAssertFail("pLight", THREE_D_CPP, 608, 0);
    }

    light->attenuation_model_150 = srLight::ATTENUATION_3DSTUDIO_MAX;
    light->enable_flags_194 |= 0x10; /* ENABLE_RANGE_FAR */
    light->enable_flags_194 |= 4;    /* ENABLE_BOUNDING_SPHERE */
    light->setName(name);
    light->near_start_158 = 0.0;
    light->near_end_160 = 0.0;
    light->far_start_168 = 0.0;
    light->far_end_170 = 1500.0;
    light->safe_range_1d4 = 5000.0f;
    light->setLinearAttenuation(1500.0f, 0.0019569471f);
    PLAdoptAppend(&world->m_lights_0a8, light);
    return light;
}

/* Take a light out of both of the world's light collections and release it.
   The update list's Remove is the inlined growable-vector search-and-shift. */
// FUNCTION: WIZ8 0x0046e250
void WorldRemoveLight(W8World* world, stLight* light)
{
    if (world == 0) {
        srAssertFail("pWorld", THREE_D_CPP, 0x278, 0);
    }
    if (light == 0) {
        srAssertFail("pLight", THREE_D_CPP, 0x279, 0);
    }
    PListRemove(&world->m_lights_0a8, light);
    world->lights_to_update->Remove(light);
    if (light != 0) {
        light->release();
    }
}

// FUNCTION: WIZ8 0x0046E300
void ConfigureWorldLight0046E300(srLight* light, float range)
{
    light->far_end_170 = (double)range;
    light->near_start_158 = 0.0;
    light->near_end_160 = 0.0;
    light->far_start_168 = 0.0;
    light->safe_range_1d4 = 5000.0f;
    light->setLinearAttenuation(range, 0.0019569471f);
}

// Source unit is Engine Code\3d.cpp; the assertion at line 344 is what names
// and types World::plsProps.
// FUNCTION: WIZ8 0x0046ded0
void WorldUpdateProps(W8World* world)
{
    int count;
    int index;
    W8Prop* prop;

    if (!world || !world->plsProps) {
        srAssertFail("pWorld && pWorld->plsProps", "C:\\Projects\\Wizardry 8\\Engine Code\\3d.cpp",
                     0x158, 0);
    }
    count = (int)PLLength(world->plsProps);
    for (index = 0; index < count; index++) {
        prop = (W8Prop*)PLGet(world->plsProps, index);
        if (prop) {
            prop->Method44D360(world);
            prop->Method44C030();
            prop->Method44C830(world);
        }
    }
}

/* Advance each light queued by this world. */
// FUNCTION: WIZ8 0x0046df50
void WorldUpdateLights(W8World* world)
{
    int count = world->lights_to_update->GetCount();

    for (int index = 0; index < count; ++index) {
        stLight** light = world->lights_to_update->GetAt(index);
        (*light)->Update0049C960();
    }
}

/* 0x00659AB4: the world being rendered, which the list wrappers below reach
   through. Every one of them ignores the caller's own first argument and uses
   this global instead. */
extern void SetHeapFree(void* block);

/* Add a monster to the world's monster list, or add/remove an item on the
   item list. Each wrapper still takes the caller's W8World* even though the
   body uses g_world. */
// FUNCTION: WIZ8 0x0046e580
void AddMonsterToWorld0046E580(W8World* unused, W8Monster* monster)
{
    PLAdoptAppend(g_world->plsMonsters, monster);
}

// FUNCTION: WIZ8 0x0046e5c0
void AddItemToWorld0046E5C0(W8World* unused, W8Item* item)
{
    PLAdoptAppend(g_world->plsItems, item);
}

// FUNCTION: WIZ8 0x0046e5e0
void RemoveItemFromWorld0046E5E0(W8World* unused, W8Item* item)
{
    PListRemove(g_world->plsItems, item);
}

/* How many props the world holds, and the one at a position - both answers
   discarded by the wrapper itself, which is what makes these thin forwarders
   rather than accessors. */
// FUNCTION: WIZ8 0x0046e600
void WorldGetPropCount(void)
{
    PLLength(g_world->plsProps);
}

// FUNCTION: WIZ8 0x0046e620
void WorldGetPropAt(W8World* unused, int index)
{
    PLGet(g_world->plsProps, index);
}

/* Two wrappers that reach the world's static scene along before forwarding. */
// FUNCTION: WIZ8 0x0046e860
void ForwardThroughMember3C_46E750(W8World* owner, int argument)
{
    SetSceneMeshShaderLowBits0046E750(owner->static_scene, argument);
}

// FUNCTION: WIZ8 0x0046e880
void ForwardThroughMember3C_46E640(W8World* owner, int argument)
{
    SetSceneMeshShaderBit3_0046E640(owner->static_scene, argument);
}

/* Walk one scene subtree and toggle shader DEPTH_WRITE on every mesh model of
   every model instance; the instance's flags_3a0 bit zero selects the off
   state. */
// FUNCTION: WIZ8 0x0046e640
void SetSceneMeshShaderBit3_0046E640(srNode* node, int argument)
{
    unsigned int shader_default;
    Function00424A40(&shader_default);
    for (; node != 0; node = node->nextSibling()) {
        if (node->getClassID() == 0x10004) {
            stModelInstance* instance = static_cast<stModelInstance*>(node);
            for (stMeshModel* mesh = static_cast<stMeshModel*>(instance->model()); mesh != 0;
                 mesh = mesh->next) {
                srShader* polygon_shader = mesh->getPolyShader(0, 0);
                bool clear = argument == 0 || (mesh->flags_3a0 & 1) != 0;

                if (polygon_shader == 0) {
                    srShader shader = mesh->getShader(0);
                    if (clear) {
                        shader.value &= ~srShader::MASK_DEPTH_WRITE;
                    } else {
                        shader.value |= srShader::MASK_DEPTH_WRITE;
                    }
                    mesh->setShader(shader, 0);
                } else if (clear) {
                    for (long index = 0; index < mesh->polygon_count_230; ++index) {
                        polygon_shader[index].value &= ~srShader::MASK_DEPTH_WRITE;
                    }
                } else {
                    for (long index = 0; index < mesh->polygon_count_230; ++index) {
                        polygon_shader[index].value |= srShader::MASK_DEPTH_WRITE;
                    }
                }
            }
        }
        if (node->firstChild() != 0) {
            SetSceneMeshShaderBit3_0046E640(node->firstChild(), argument);
        }
    }
}

/* The sibling walker that leaves PASS bit 2 clear and writes the low three
   bits: PASS_ALWAYS when the argument is zero, otherwise PASS_LEQUAL. */
// FUNCTION: WIZ8 0x0046e750
void SetSceneMeshShaderLowBits0046E750(srNode* node, int argument)
{
    unsigned int shader_default;
    Function00424A40(&shader_default);
    for (; node != 0; node = node->nextSibling()) {
        if (node->getClassID() == 0x10004) {
            stModelInstance* instance = static_cast<stModelInstance*>(node);
            for (stMeshModel* mesh = static_cast<stMeshModel*>(instance->model()); mesh != 0;
                 mesh = mesh->next) {
                if ((mesh->flags_3a0 & 1) != 0) {
                    continue;
                }
                srShader* polygon_shader = mesh->getPolyShader(0, 0);
                if (polygon_shader == 0) {
                    srShader shader = mesh->getShader(0);
                    if (argument == 0) {
                        shader.value |= srShader::PASS_ALWAYS;
                    } else {
                        shader.value = (shader.value & 0xfffffffb) | srShader::PASS_LEQUAL;
                    }
                    mesh->setShader(shader, 0);
                } else if (argument == 0) {
                    for (long index = 0; index < mesh->polygon_count_230; ++index) {
                        polygon_shader[index].value |= srShader::PASS_ALWAYS;
                    }
                } else {
                    for (long index = 0; index < mesh->polygon_count_230; ++index) {
                        polygon_shader[index].value =
                            (polygon_shader[index].value & 0xfffffffb) | srShader::PASS_LEQUAL;
                    }
                }
            }
        }
        if (node->firstChild() != 0) {
            SetSceneMeshShaderLowBits0046E750(node->firstChild(), argument);
        }
    }
}

/* Release one block back to the renderer's heap rather than the CRT's. */
// FUNCTION: WIZ8 0x0046f3f0
void FreeThroughRenderHeap(void* block)
{
    SetHeapFree(block);
}

/* Walk a chain through its link at 0x134 and set the same field on every node
   of it. The link is srNode::first_child_ (+0x134), but the +0x15c store
   lies past the 0x138-byte plain srNode base, inside the srModelInstance
   tail (exclusion_mask_15c). Proven callers hand model instances, but the
   trace-model node from CreateTraceModel0041C930 is only established as an
   srNode, so the helper stays a raw walker until every chain member proves
   the tail. */
// FUNCTION: WIZ8 0x0046f4f0
void SetChainValue15C(char* node, int value)
{
    for (; node != 0; node = *(char**)(node + 0x134)) {
        *(int*)(node + 0x15c) = value;
    }
}

/* The world's own float pair at 0x74 and 0x78. Both are assigned from the one
   argument, and only zero is rejected, so the guard is a "leave it alone"
   rather than a range check. The assertion at 3d.cpp:651 is what names the
   receiver pWorld. */
// FUNCTION: WIZ8 0x0046e350
void WorldSetValue74(W8World* world, float value)
{
    if (!world) {
        srAssertFail("pWorld", THREE_D_CPP, 0x28b, 0);
    }
    if (value != 0.0f) {
        world->value_74 = value;
        world->value_78 = value;
        MarkRendererReady();
    }
}

/* Reads back only the second of the pair, which is what makes 0x78 the live
   copy and 0x74 the one nothing here consumes. */
// FUNCTION: WIZ8 0x0046e3a0
float WorldGetValue78(W8World* world)
{
    if (!world) {
        srAssertFail("pWorld", THREE_D_CPP, 0x297, 0);
    }
    return world->value_78;
}

/* Pushes the world's view distance into the camera as the far clip plane; the
   near plane is the fixed 62.5 the original materialises inline. The guard is
   against a denormal-scale epsilon rather than zero, so a distance that has
   collapsed leaves the camera as it was. */
// FUNCTION: WIZ8 0x0046e3d0
void WorldSetFarClip(W8World* world, float distance)
{
    if (!world) {
        srAssertFail("pWorld", THREE_D_CPP, 0x29e, 0);
    }
    if ((double)distance > 5.9604644775390625e-008) {
        world->camera->setClipRange(62.5, (double)distance);
        RefreshFogRanges004836A0();
        MarkRendererReady();
    }
}

/* The matching reader. Both planes come back, and only the far one is
   returned, which is the same asymmetry the setter has. A world without a
   camera answers zero rather than reading through it. */
// FUNCTION: WIZ8 0x0046e440
double WorldGetFarClip(W8World* world)
{
    double near_plane = 0;
    double far_plane = 0;

    if (world != 0 && world->camera != 0) {
        world->camera->getClipRange(near_plane, far_plane);
    }
    return far_plane;
}

// FUNCTION: WIZ8 0x0046E5A0
void RemoveMonsterFromWorldList(W8World* unused, W8Monster* monster)
{
    PListRemove(g_world->plsMonsters, monster);
}

// FUNCTION: WIZ8 0x0046DC90
void SetSceneAmbientLightWhite(srScene* scene)
{
    if (scene == 0) {
        srAssertFail("psrScene", THREE_D_CPP, 0xf1, 0);
    }
    scene->setAmbientLight(1.0f, 1.0f, 1.0f);
}

// GLOBAL: WIZ8 0x00652db0
W8GameData* g_octree_game_data_00652db0;

/* The octree builds read the level data through this slot; the recovered
   caller passes the W8GameData object it just read. */
// FUNCTION: WIZ8 0x0046D7D0
void __stdcall SetOctreeGameData0046D7D0(W8GameData* value)
{
    g_octree_game_data_00652db0 = value;
}

/* Test one point against all six frustum planes: outside if any signed
   distance is negative. */
// FUNCTION: WIZ8 0x0046d880
unsigned char PointInsideFrustum0046D880(const srVector3T<float>* point,
                                         const srVector4T<float>* planes)
{
    for (int index = 0; index < 6; ++index) {
        float distance = SignedPlaneDistance(planes[index], *point);

        if (distance < g_float_005ebb34) {
            return 0;
        }
    }
    return 1;
}

/* Header-visible SetPlaneFromThreePoints. This TU lowers the three-point
   copy as a component countdown; 0x00449A40 unrolls the same assignments. */
// FUNCTION: WIZ8 0x0046d660
void BuildPlaneFromPoints0046D660(srVector4T<float>* plane, const srVector3T<float>* first,
                                  const srVector3T<float>* second, const srVector3T<float>* third)
{
    SetPlaneFromThreePoints(&plane->x, first, second, third);
}

/* Pointer-plus-capacity cleanup used by BitArray::Save's Sampler and by later
   mesh helpers. Retail emits this out of line between 3d.cpp and
   stMeshModel.cpp rather than as a Sampler import. */
// FUNCTION: WIZ8 0x004701b0
W8OwnedPtr::~W8OwnedPtr()
{
    ::operator delete(data);
    data = 0;
    size = 0;
}
