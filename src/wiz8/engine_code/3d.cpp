#include "wiz8/3d_code/IList.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/engine_code/3d.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/Item.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/OctPreTree.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/stHash.hpp"
#include "wiz8/engine_code/stModelInstance.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/Environment.h"
#include "wiz8/engine_code/stLight.hpp"
#include "wiz8/engine_code/stMeshModel.h"
#include "wiz8/engine_code/PolyPick.h"
#include "wiz8/float_constants.h"
#include "wiz8/geometry.h"
#include "wiz8/sr_api.h"
#include "surrender/srCamera.h"
#include "surrender/srHeap.h"
#include "surrender/srIlluminator.h"
#include "surrender/srMaterial.h"
#include "surrender/srScene.h"
#include "surrender/srVectorProcessor.h"

#include <new>
#include <stdlib.h>
#include <string.h>

#define THREE_D_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\3d.cpp"

// GLOBAL: WIZ8 0x005EC428
double g_double_005ec428 = 0.9980430528375734;
// GLOBAL: WIZ8 0x005EC430
double g_double_005ec430 = 0.0019569471624266144;

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

/* Five-byte tail-jump thunk to GameData.cpp's GetCameraYawRadians. */
// SYNTHETIC: WIZ8 0x0046e490
// GetCameraYawRadians

/* Remove every light held in one world's list, drop it from the render update
   set, and release it. */
// FUNCTION: WIZ8 0x0046e4a0
void DestroyWorldLights0046E4A0(W8World* world)
{
    W8PList* lights = &world->m_lights_0a8;

    /* Retail really does call the IList accessor here; W8IList and W8PList
       share the same three-field layout. */
    while (ILLength(reinterpret_cast<W8IList*>(lights)) !=
           0) { // reinterpret-ok: retail calls ILLength on a W8PList field
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

/* Bake the dynamic scene's non-"Sun" lights into every mesh of the instance's
   model chain. Each mesh gets a world-space vertex set (translated or fully
   transformed, or aliased when the transform is identity) and a direction
   array of vertex-minus-light vectors per overlapping light; a vertex that
   faces the light accumulates diffuse attenuated by range into its vertex
   light. "Sun"-prefixed lights bake into the sunlight array instead and are
   skipped here. `walk_chain` limits the light walk to the first sibling. */
// FUNCTION: WIZ8 0x0046E8A0
unsigned char BakeInstanceVertexLighting0046E8A0(stModelInstance* instance, srNode* lights,
                                                 char walk_chain)
{
    srVector3T<float>* directions = 0;
    unsigned char locations_allocated = 0;
    stMeshModel* mesh = static_cast<stMeshModel*>(instance->model());
    srVector3T<float> location;
    location = instance->getWorldSpaceLocation();
    srVector3T<float> minimum;
    srVector3T<float> maximum;
    mesh->getBoundingBox(minimum, maximum);
    maximum += location;
    minimum += location;
    srMatrix3T<float> rotation;
    instance->getWorldSpaceRotation(rotation);

    while (mesh != 0) {
        srVector3T<float>* vertex_lights = mesh->GetVertexLights(1, -1);
        mesh->GetVertexSunlight(1);
        srPtr<srMaterialIFace>* vertex_materials =
            mesh->getVertexMaterial(0, static_cast<srMeshModel::e_side>(0), 0);
        srMaterialIFace* material_iface = mesh->getMaterial(0, static_cast<srMeshModel::e_side>(0));
        srVector4T<float> material_diffuse;
        if (material_iface != 0) {
            material_diffuse = static_cast<srMaterial*>(material_iface)->parms.diffuse;
        }
        if (vertex_lights == 0) {
            srAssertFail("psrDIG", THREE_D_CPP, 0x453, 0);
        }

        srVector3T<float>* vertices;
        srVector3T<float>* normals;
        if ((mesh->flags_3a0 & 4) != 0) {
            vertices = mesh->GetVertexLocations00471AD0(0, 1, 0.0f);
            normals = mesh->GetVertexNormals00471CA0(0, 1);
        } else {
            vertices = mesh->getVertexLoc();
            normals = mesh->getVertexNormal();
        }
        unsigned long count = mesh->vertex_location_count_22c;

        srVector3T<float>* world_vertices = vertices;
        if (rotation.vectors[0].x != g_float_005ebb38 ||
            rotation.vectors[1].y != g_float_005ebb38 ||
            rotation.vectors[2].z != g_float_005ebb38 ||
            rotation.vectors[0].y != g_float_005ebb34 ||
            rotation.vectors[0].z != g_float_005ebb34 ||
            rotation.vectors[1].x != g_float_005ebb34 ||
            rotation.vectors[1].z != g_float_005ebb34 ||
            rotation.vectors[2].x != g_float_005ebb34 ||
            rotation.vectors[2].y != g_float_005ebb34 || location.x != g_float_005ebb34 ||
            location.y != g_float_005ebb34 || location.z != g_float_005ebb34) {
            locations_allocated = 1;
            world_vertices =
                static_cast<srVector3T<float>*>(malloc(count * sizeof(srVector3T<float>)));
            if (world_vertices == 0) {
                return 0;
            }
            if (count != 0 && world_vertices != vertices) {
                srVectorProcessor::memcopy(world_vertices, vertices,
                                           count * sizeof(srVector3T<float>));
            }
            if (rotation.vectors[0].x == g_float_005ebb38 &&
                rotation.vectors[1].y == g_float_005ebb38 &&
                rotation.vectors[2].z == g_float_005ebb38 &&
                rotation.vectors[0].y == g_float_005ebb34 &&
                rotation.vectors[0].z == g_float_005ebb34 &&
                rotation.vectors[1].x == g_float_005ebb34 &&
                rotation.vectors[1].z == g_float_005ebb34 &&
                rotation.vectors[2].x == g_float_005ebb34 &&
                rotation.vectors[2].y == g_float_005ebb34) {
                if (count != 0 &&
                    (location.x != g_float_005ebb34 || location.y != g_float_005ebb34 ||
                     location.z != g_float_005ebb34)) {
                    srVectorProcessor::add(world_vertices, location, world_vertices,
                                           static_cast<SRDWORD>(count));
                }
            } else if (count != 0) {
                srMatrix4T<float> transform;
                transform.Set(rotation, location);
                srVectorProcessor::transform(world_vertices, world_vertices, transform,
                                             static_cast<SRDWORD>(count));
            }
        }

        srVector3T<float>* world_normals;
        if (rotation.vectors[0].x == g_float_005ebb38 &&
            rotation.vectors[1].y == g_float_005ebb38 &&
            rotation.vectors[2].z == g_float_005ebb38 &&
            rotation.vectors[0].y == g_float_005ebb34 &&
            rotation.vectors[0].z == g_float_005ebb34 &&
            rotation.vectors[1].x == g_float_005ebb34 &&
            rotation.vectors[1].z == g_float_005ebb34 &&
            rotation.vectors[2].x == g_float_005ebb34 &&
            rotation.vectors[2].y == g_float_005ebb34) {
            world_normals = normals;
        } else {
            world_normals =
                static_cast<srVector3T<float>*>(malloc(count * sizeof(srVector3T<float>)));
            if (world_normals == 0) {
                return 0;
            }
            srVector3T<float> origin(0.0f, 0.0f, 0.0f);
            if (count != 0) {
                srMatrix4T<float> transform;
                transform.Set(rotation, origin);
                srVectorProcessor::transform(world_normals, normals, transform,
                                             static_cast<SRDWORD>(count));
            }
        }

        srNode* light_node = lights;
        while (light_node != 0) {
            if (light_node->getClassID() == 0x10006 &&
                _strnicmp(light_node->getName(), "Sun", 3) != 0) {
                stLight* light = static_cast<stLight*>(light_node);
                srVector3T<float> attenuation = light->opengl_attenuation_188;
                float range =
                    static_cast<float>(g_double_005ec428 / (attenuation.y * g_double_005ec430));
                srVector3T<float> light_position;
                light_position = light_node->getWorldSpaceLocation();
                srVector3T<float> light_min(light_position.x - range, light_position.y - range,
                                            light_position.z - range);
                srVector3T<float> light_max(light_position.x + range, light_position.y + range,
                                            light_position.z + range);
                if (BoundsOverlap004BE8D0(&light_min, &light_max, &minimum, &maximum)) {
                    if (directions == 0) {
                        directions = static_cast<srVector3T<float>*>(
                            malloc(count * sizeof(srVector3T<float>)));
                        if (directions == 0) {
                            return 0;
                        }
                    }
                    if (light_position.x == g_float_005ebb34 &&
                        light_position.y == g_float_005ebb34 &&
                        light_position.z == g_float_005ebb34) {
                        CopyDwordBuffer00470180(directions, world_vertices, count * 3);
                    } else {
                        srVector3T<float> offset(-light_position.x, -light_position.y,
                                                 -light_position.z);
                        OffsetVertices00470040(directions, world_vertices, &offset, count);
                    }
                    srVector3T<float> light_color = light->diffuse_1a4;
                    float intensity = light->intensity_1d0;
                    for (unsigned long index = 0; index < count; ++index) {
                        srVector3T<float> direction = directions[index];
                        float distance = direction.Length();
                        if (distance <= range) {
                            direction.Normalize();
                            float facing = DotProduct(direction, world_normals[index]);
                            if (facing < g_zero_005ebb40) {
                                double contribution =
                                    -facing * intensity * (g_double_005ebc30 - distance / range);
                                if (vertex_materials != 0) {
                                    srMaterialIFace* vertex_material = vertex_materials[index];
                                    if (vertex_material != 0) {
                                        material_diffuse = static_cast<srMaterial*>(vertex_material)
                                                               ->parms.diffuse;
                                    } else {
                                        srMaterialIFace* base_material = mesh->getMaterial(
                                            0, static_cast<srMeshModel::e_side>(0));
                                        if (base_material != 0) {
                                            material_diffuse =
                                                static_cast<srMaterial*>(base_material)
                                                    ->parms.diffuse;
                                        } else {
                                            material_diffuse.x = 1.0f;
                                            material_diffuse.y = 1.0f;
                                            material_diffuse.z = 1.0f;
                                        }
                                    }
                                }
                                vertex_lights[index].x +=
                                    material_diffuse.x * light_color.x * contribution;
                                vertex_lights[index].y +=
                                    material_diffuse.y * light_color.y * contribution;
                                vertex_lights[index].z +=
                                    material_diffuse.z * light_color.z * contribution;
                            }
                        }
                    }
                }
            }
            if (walk_chain == 0) {
                break;
            }
            light_node = light_node->nextSibling();
        }

        if (locations_allocated != 0) {
            free(world_vertices);
        }
        if (rotation.vectors[0].x != g_float_005ebb38 ||
            rotation.vectors[1].y != g_float_005ebb38 ||
            rotation.vectors[2].z != g_float_005ebb38 ||
            rotation.vectors[0].y != g_float_005ebb34 ||
            rotation.vectors[0].z != g_float_005ebb34 ||
            rotation.vectors[1].x != g_float_005ebb34 ||
            rotation.vectors[1].z != g_float_005ebb34 ||
            rotation.vectors[2].x != g_float_005ebb34 ||
            rotation.vectors[2].y != g_float_005ebb34) {
            free(world_normals);
        }
        if (directions != 0) {
            free(directions);
            directions = 0;
            mesh->flags_3a0 |= 2;
        }
        mesh = mesh->next;
    }
    return 1;
}

/* Bake the dynamic scene's light children into every not-yet-lit model
   instance under one static-scene subtree. render_flags_178 bit 1 is the instance's
   own lit marker; the first-child chain store is the same typed walk
   SetModelInstanceChainExclusionMask performs. */
// FUNCTION: WIZ8 0x0046F410
unsigned char FinalizeWorldScenes0046F410(srNode* node, srNode* dynamic_scene)
{
    for (; node != 0; node = node->nextSibling()) {
        if (node->firstChild() != 0) {
            FinalizeWorldScenes0046F410(node->firstChild(), dynamic_scene);
        }
        if (node->getClassID() == 0x10004) {
            stModelInstance* instance = static_cast<stModelInstance*>(node);
            srNode* lights = dynamic_scene->firstChild();
            if ((instance->render_flags_178 & 2) == 0) {
                instance->render_flags_178 |= 2;
                for (srModelInstance* chain = instance; chain != 0;
                     chain = static_cast<srModelInstance*>(chain->firstChild())) {
                    chain->setExclusionMask(1);
                }
                BakeInstanceVertexLighting0046E8A0(instance, lights, 1);
            }
        }
    }
    return 1;
}

/* Bake dynamic-scene lights into one not-yet-lit model instance. render_flags_178
   bit 1 is the lit marker; the first-child walk matches SetModelInstanceChainExclusionMask. */
// FUNCTION: WIZ8 0x0046F4A0
unsigned char BakeInstanceVertexLightingIfNeeded0046F4A0(stModelInstance* instance,
                                                         srNode* dynamic_scene)
{
    srNode* lights = dynamic_scene->firstChild();

    if ((instance->render_flags_178 & 2) == 0) {
        instance->render_flags_178 |= 2;
        for (srModelInstance* chain = instance; chain != 0;
             chain = static_cast<srModelInstance*>(chain->firstChild())) {
            chain->setExclusionMask(1);
        }
        BakeInstanceVertexLighting0046E8A0(instance, lights, 1);
    }
    return 1;
}

/* Walk every live world mesh (octree mesh table, or the update-mesh chain when
   there is no octree) and point its vertex-light table index at `table`, then
   raise flags_3a0 bit 1 so the next bake uses that table. */
// FUNCTION: WIZ8 0x0046F760
void SetWorldMeshVertexLightTable0046F760(W8World* world, int table)
{
    if (world->octree == 0) {
        for (stMeshModel* model = static_cast<stMeshModel*>(world->update_mesh_source->model());
             model != 0; model = model->next) {
            model->vertex_light_table_3b0 = table;
            model->flags_3a0 |= 2;
        }
    } else {
        for (unsigned int mesh = 0; mesh < world->octree->m_meshCount_1b4; ++mesh) {
            srModelInstance* instance = world->psrMeshes[mesh];
            if (instance != 0) {
                for (stMeshModel* model = static_cast<stMeshModel*>(instance->model()); model != 0;
                     model = model->next) {
                    model->vertex_light_table_3b0 = table;
                    model->flags_3a0 |= 2;
                }
            }
        }
    }
}

/* True when the camera projects the AABB midpoint or any of its eight corners
   and the octree has line of sight from eye to that point. */
// FUNCTION: WIZ8 0x0046F820
unsigned char ShowTargetMarker(const srVector3T<float>* eye, const srVector3T<float>* lower,
                               const srVector3T<float>* upper)
{
    srVector3T<float> point;

    point = (*lower + *upper) * g_double_005ebe80;
    if (ProjectPointThroughCamera004BE940(&point) != 0) {
        if (g_octree_6598a4->HasLineOfSight(eye, &point, 1) != 0) {
            return 1;
        }
    }
    point = *upper;
    if (ProjectPointThroughCamera004BE940(&point) != 0) {
        if (g_octree_6598a4->HasLineOfSight(eye, &point, 1) != 0) {
            return 1;
        }
    }
    point = *lower;
    if (ProjectPointThroughCamera004BE940(&point) != 0) {
        if (g_octree_6598a4->HasLineOfSight(eye, &point, 1) != 0) {
            return 1;
        }
    }
    point.Set(lower->x, upper->y, upper->z);
    if (ProjectPointThroughCamera004BE940(&point) != 0) {
        if (g_octree_6598a4->HasLineOfSight(eye, &point, 1) != 0) {
            return 1;
        }
    }
    point.Set(lower->x, upper->y, lower->z);
    if (ProjectPointThroughCamera004BE940(&point) != 0) {
        if (g_octree_6598a4->HasLineOfSight(eye, &point, 1) != 0) {
            return 1;
        }
    }
    point.Set(upper->x, upper->y, lower->z);
    if (ProjectPointThroughCamera004BE940(&point) != 0) {
        if (g_octree_6598a4->HasLineOfSight(eye, &point, 1) != 0) {
            return 1;
        }
    }
    point.Set(upper->x, lower->y, upper->z);
    if (ProjectPointThroughCamera004BE940(&point) != 0) {
        if (g_octree_6598a4->HasLineOfSight(eye, &point, 1) != 0) {
            return 1;
        }
    }
    point.Set(lower->x, lower->y, upper->z);
    if (ProjectPointThroughCamera004BE940(&point) != 0) {
        if (g_octree_6598a4->HasLineOfSight(eye, &point, 1) != 0) {
            return 1;
        }
    }
    point.Set(upper->x, lower->y, lower->z);
    if (ProjectPointThroughCamera004BE940(&point) != 0) {
        if (g_octree_6598a4->HasLineOfSight(eye, &point, 1) != 0) {
            return 1;
        }
    }
    return 0;
}

/* The world-trace variant of HasLineOfSightToBounds0046FD70: same midpoint and
   eight-corner probe, but through TraceLineOfSight, which reports a clear
   trace as zero. */
// FUNCTION: WIZ8 0x0046FAF0
char TraceLineOfSightToBounds0046FAF0(const srVector3T<float>* origin, srVector3T<float>* minimum,
                                      srVector3T<float>* maximum)
{
    srVector3T<float> point;

    point = (*minimum + *maximum) * g_double_005ebe80;
    if (g_octree_6598a4->TraceLineOfSight(origin, &point, 1, -3, -3, 1, 0) == 0) {
        return 1;
    }
    point = *maximum;
    if (g_octree_6598a4->TraceLineOfSight(origin, &point, 1, -3, -3, 1, 0) == 0) {
        return 1;
    }
    point = *minimum;
    if (g_octree_6598a4->TraceLineOfSight(origin, &point, 1, -3, -3, 1, 0) == 0) {
        return 1;
    }
    point.Set(minimum->x, maximum->y, maximum->z);
    if (g_octree_6598a4->TraceLineOfSight(origin, &point, 1, -3, -3, 1, 0) == 0) {
        return 1;
    }
    point.Set(minimum->x, maximum->y, minimum->z);
    if (g_octree_6598a4->TraceLineOfSight(origin, &point, 1, -3, -3, 1, 0) == 0) {
        return 1;
    }
    point.Set(maximum->x, maximum->y, minimum->z);
    if (g_octree_6598a4->TraceLineOfSight(origin, &point, 1, -3, -3, 1, 0) == 0) {
        return 1;
    }
    point.Set(maximum->x, minimum->y, maximum->z);
    if (g_octree_6598a4->TraceLineOfSight(origin, &point, 1, -3, -3, 1, 0) == 0) {
        return 1;
    }
    point.Set(minimum->x, minimum->y, maximum->z);
    if (g_octree_6598a4->TraceLineOfSight(origin, &point, 1, -3, -3, 1, 0) == 0) {
        return 1;
    }
    point.Set(maximum->x, minimum->y, minimum->z);
    return g_octree_6598a4->TraceLineOfSight(origin, &point, 1, -3, -3, 1, 0) == 0;
}

/* Report whether the eye sees a bounds box: true when the midpoint or any one
   of the eight min/max corner combinations has line of sight. */
// FUNCTION: WIZ8 0x0046FD70
bool HasLineOfSightToBounds0046FD70(const srVector3T<float>* origin, srVector3T<float>* minimum,
                                    srVector3T<float>* maximum)
{
    srVector3T<float> point;

    point = (*minimum + *maximum) * g_double_005ebe80;
    if (g_octree_6598a4->HasLineOfSight(origin, &point, 1)) {
        return true;
    }
    point = *maximum;
    if (g_octree_6598a4->HasLineOfSight(origin, &point, 1)) {
        return true;
    }
    point = *minimum;
    if (g_octree_6598a4->HasLineOfSight(origin, &point, 1)) {
        return true;
    }
    point.Set(minimum->x, maximum->y, maximum->z);
    if (g_octree_6598a4->HasLineOfSight(origin, &point, 1)) {
        return true;
    }
    point.Set(minimum->x, maximum->y, minimum->z);
    if (g_octree_6598a4->HasLineOfSight(origin, &point, 1)) {
        return true;
    }
    point.Set(maximum->x, maximum->y, minimum->z);
    if (g_octree_6598a4->HasLineOfSight(origin, &point, 1)) {
        return true;
    }
    point.Set(maximum->x, minimum->y, maximum->z);
    if (g_octree_6598a4->HasLineOfSight(origin, &point, 1)) {
        return true;
    }
    point.Set(minimum->x, minimum->y, maximum->z);
    if (g_octree_6598a4->HasLineOfSight(origin, &point, 1)) {
        return true;
    }
    point.Set(maximum->x, minimum->y, minimum->z);
    return g_octree_6598a4->HasLineOfSight(origin, &point, 1);
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
            prop->DetachAnimationInstances0044D360(world);
            prop->UpdatePropAnimation0044C030();
            prop->AttachAnimationInstances0044C830(world);
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
int WorldGetPropCount(W8World* unused)
{
    /* Retail really does call the IList accessor here; W8IList and W8PList
       share the same three-field layout. */
    return ILLength(reinterpret_cast<W8IList*>(
        g_world->plsProps)); // reinterpret-ok: retail calls ILLength on a W8PList field
}

// FUNCTION: WIZ8 0x0046e620
W8Prop* WorldGetPropAt(W8World* unused, int index)
{
    return (W8Prop*)PLGet(g_world->plsProps, index); // c-style-cast-ok: PList stores void* rows
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
    srShader shader;
    for (; node != 0; node = node->nextSibling()) {
        if (node->getClassID() == 0x10004) {
            stModelInstance* instance = static_cast<stModelInstance*>(node);
            for (stMeshModel* mesh = static_cast<stMeshModel*>(instance->model()); mesh != 0;
                 mesh = mesh->next) {
                srShader* polygon_shader = mesh->getPolyShader(0, 0);
                bool clear = argument == 0 || (mesh->flags_3a0 & 1) != 0;

                if (polygon_shader == 0) {
                    shader = mesh->getShader(0);
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
    srShader shader;
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
                    shader = mesh->getShader(0);
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
    srHeap.free(block);
}

/* Propagate the model-instance exclusion mask through the first-child chain. */
// FUNCTION: WIZ8 0x0046f4f0
void SetModelInstanceChainExclusionMask(srModelInstance* node, int value)
{
    for (; node != 0; node = static_cast<srModelInstance*>(node->firstChild())) {
        node->setExclusionMask(value);
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
bool PointInsideFrustum0046D880(const srVector3T<float>* point, const srVector4T<float>* planes)
{
    for (int index = 0; index < 6; ++index) {
        float distance = SignedPlaneDistance(planes[index], *point);

        if (distance < g_float_005ebb34) {
            return false;
        }
    }
    return true;
}

/* Header-visible SetPlaneFromThreePoints. This TU lowers the three-point
   copy as a component countdown; 0x00449A40 unrolls the same assignments. */
// FUNCTION: WIZ8 0x0046d660
void BuildPlaneFromPoints0046D660(srVector4T<float>* plane, const srVector3T<float>* first,
                                  const srVector3T<float>* second, const srVector3T<float>* third)
{
    SetPlaneFromThreePoints(plane, first, second, third);
}

/* Point-in-triangle test by even-odd crossing on the plane perpendicular to
   `axis`: the two remaining components (axis+1, axis+2 mod 3) project the
   triangle and query point, and each edge straddling the point's second
   coordinate toggles the inside flag when its interpolation crosses the
   first. */
// FUNCTION: WIZ8 0x0046d530
unsigned char PointInsideTriangle0046D530(const srVector3T<float>* vertices, short axis,
                                          const srVector3T<float>* point)
{
    const float* p = &point->x;
    short u = static_cast<short>(axis + 1) % 3;
    short v = static_cast<short>(axis + 2) % 3;
    char inside = 0;

    for (int i = 0; i < 3; ++i) {
        const float* first = &vertices[i].x;
        const float* second = &vertices[(i + 1) % 3].x;
        if ((first[v] < p[v] && p[v] < second[v]) || (second[v] < p[v] && p[v] < first[v])) {
            if (p[u] <=
                (p[v] - first[v]) * (second[u] - first[u]) / (second[v] - first[v]) + first[u]) {
                inside = inside == 0;
            }
        }
    }
    return inside;
}

/* Build the six face planes of the frustum described by `points` in the
   canonical corner order SortFrustumCorners produces. */
// FUNCTION: WIZ8 0x0046d7e0
void BuildFrustumPlanes0046D7E0(const srVector3T<float>* points, srVector4T<float>* planes)
{
    BuildPlaneFromPoints0046D660(&planes[0], &points[1], &points[5], &points[4]);
    BuildPlaneFromPoints0046D660(&planes[1], &points[6], &points[7], &points[3]);
    BuildPlaneFromPoints0046D660(&planes[2], &points[0], &points[2], &points[3]);
    BuildPlaneFromPoints0046D660(&planes[3], &points[4], &points[5], &points[7]);
    BuildPlaneFromPoints0046D660(&planes[4], &points[4], &points[6], &points[2]);
    BuildPlaneFromPoints0046D660(&planes[5], &points[1], &points[3], &points[7]);
}

/* Sort `points` in place into the canonical corner order: an index array is
   insertion-sorted by y, then each four-entry run is re-sorted by z and each
   two-entry run by x, and the points are permuted through a temporary copy. */
// FUNCTION: WIZ8 0x0046da20
void SortFrustumCorners0046DA20(srVector3T<float>* points)
{
    short order[8];
    srVector3T<float> sorted[8];

    for (short i = 0; i < 8; ++i) {
        short index = i;
        short slot = 0;
        if (i > 0) {
            for (short j = 0; j < i; ++j) {
                slot = order[j];
                if (points[index].y < points[slot].y) {
                    order[j] = index;
                    index = slot;
                }
            }
            slot = i;
        }
        order[slot] = index;
    }
    short axis = 2;
    short run = 4;
    short start = 0;
    short end = run;
    do {
        if (start < end) {
            for (short at = start; at < end; ++at) {
                short value = order[at];
                for (short k = start; k < at; ++k) {
                    if ((&points[value].x)[axis] < (&points[order[k]].x)[axis]) {
                        order[k] = value;
                        value = order[k];
                    }
                }
                order[at] = value;
            }
        }
        if (end < 8) {
            start += run;
            end += run;
        } else {
            run /= 2;
            axis = (axis + 1) % 3;
            start = 0;
            end = run;
        }
    } while (run > 1);
    short point;
    for (point = 0; point < 8; ++point) {
        sorted[point] = points[order[point]];
    }
    for (point = 0; point < 8; ++point) {
        points[point] = sorted[point];
    }
}

/* Sphere test generalised to a bounds box: accept when any box corner is
   inside every frustum plane or any volume corner sits inside the box. */
// FUNCTION: WIZ8 0x0046d8d0
unsigned char SphereInsideFrustum0046D8D0(const srVector3T<float>* point, float radius,
                                          const srVector4T<float>* planes)
{
    char inside = 1;

    for (short plane = 0; plane < 6 && inside != 0; ++plane) {
        if (point->x * planes[plane].x + point->y * planes[plane].y + point->z * planes[plane].z +
                planes[plane].w <
            -radius) {
            inside = 0;
        }
    }
    return inside;
}

// FUNCTION: WIZ8 0x0046d920
bool BoundsInsideFrustum0046D920(const W8OctRegionVolume* volume, const W8BoundingBox* bounds)
{
    for (short x = 0; x < 2; ++x) {
        for (short y = 0; y < 2; ++y) {
            for (short z = 0; z < 2; ++z) {
                short plane = 0;
                while (true) {
                    if ((&bounds->minimum)[x].x * volume->planes_88[plane].x +
                            (&bounds->minimum)[y].y * volume->planes_88[plane].y +
                            (&bounds->minimum)[z].z * volume->planes_88[plane].z +
                            volume->planes_88[plane].w <
                        g_float_005ebb34) {
                        break;
                    }
                    if (++plane > 5) {
                        return 1;
                    }
                }
            }
        }
    }
    for (short corner = 0; corner < 8; ++corner) {
        const srVector3T<float>* point = &volume->points_1c[corner + 1];
        if (bounds->minimum.x <= point->x && point->x < bounds->maximum.x &&
            bounds->minimum.y <= point->y && point->y < bounds->maximum.y &&
            bounds->minimum.z <= point->z && point->z < bounds->maximum.z) {
            return 1;
        }
    }
    return 0;
}

/* Scalar-delete array teardown shared by the Sampler symbol array and other
   folded array instantiations. The primary template lives in srArray.h. */
// TEMPLATE: WIZ8 0x004701b0
// srArray<T>::release

/* srMatrix4T<float>::Set emitted for this TU (BakeInstanceVertexLighting's
   transform builds); the primary template lives in srMath.h. */
// TEMPLATE: WIZ8 0x00470200
// srMatrix4T<float>::Set
