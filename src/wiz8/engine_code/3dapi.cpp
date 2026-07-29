#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"
#include "surrender/srColorSurface.h"
#include "surrender/srMaterial.h"
#include "surrender/srMeshModel.h"
#include "surrender/srNode.h"
#include "surrender/srScene.h"

/*
 * Engine Code\3dapi.cpp.
 *
 * The thin layer the rest of the engine calls the renderer through. Most of
 * what is here forwards straight on, which is what makes the file a layer
 * rather than an implementation.
 */

#define THREE_D_API_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\3dapi.cpp"

extern void SetRendererReady(void);
extern void Function421090(const float* location);
extern void Function44FAF0(int argument);
extern void Function44F5F0(int a, int b, int c, int d, int e);
extern void SetHeapFree(void* block);
extern unsigned char g_renderer_ready_00607d7c;

/* Note that the renderer is up. Eight bytes and no branch. */
// FUNCTION: WIZ8 0x00451010
void MarkRendererReady(void)
{
    g_renderer_ready_00607d7c = 1;
}

/* Two forwarders that pass their arguments through unchanged. */
// FUNCTION: WIZ8 0x00451140
void Forward44FAF0(int argument)
{
    Function44FAF0(argument);
}

// FUNCTION: WIZ8 0x00451110
void Forward44F5F0(int a, int b, int c, int d, int e)
{
    Function44F5F0(a, b, c, d, e);
}

/* Report a failed assertion with no message of its own, so the expression and
   the site are all the caller has to give. */
// FUNCTION: WIZ8 0x00450780
void ReportAssertion(const char* expression, const char* source_path, long line)
{
    srAssertFail(expression, source_path, line, 0);
}

/* The camera's rotation basis, read straight back out of the renderer node.
   Both assertions belong to this body: 3dapi.cpp:975 names the world and
   3dapi.cpp:976 names its camera member pWorld->psrCamera, which is what puts
   the camera at 0x44 rather than anywhere else. */
// FUNCTION: WIZ8 0x004503c0
void WorldGetCameraRotation(W8World* world, srMatrix3T<float>* rotation)
{
    if (!world) {
        srAssertFail("pWorld", THREE_D_API_CPP, 0x3cf, 0);
    }
    if (!world->camera) {
        srAssertFail("pWorld->psrCamera", THREE_D_API_CPP, 0x3d0, 0);
    }
    world->camera->getRotation(rotation);
}

/* Moves the camera and the sky together. Only the camera's move notifies the
   level, which is what separates the two nodes: the sky follows the view but
   nothing else depends on where it is. The pair of doubles is rebuilt for each
   call rather than shared, so the two moves are independent statements. */
// FUNCTION: WIZ8 0x00450420
void WorldSetCameraLocation(W8World* world, const float* location)
{
    srVector3T<double> position;

    if (!world) {
        srAssertFail("pWorld", THREE_D_API_CPP, 0x422, 0);
    }
    if (world->camera != 0) {
        position.x = location[0];
        position.y = location[1];
        ((srNode*)world->camera)->setLocation(position);
        Function421090(location);
    }
    if (world->sky_node != 0) {
        position.x = location[0];
        position.y = location[1];
        ((srNode*)world->sky_node)->setLocation(position);
    }
}

/* An srNode's scalar deleting destructor. It releases through the renderer's
   own heap rather than the CRT's, which is what makes it the renderer's node
   rather than one of ours. */
// FUNCTION: WIZ8 0x0044f3d0
srNode* srNode::scalar_deleting_destructor(unsigned char flags)
{
    this->~srNode();
    if ((flags & 1) != 0) {
        SetHeapFree(this);
    }
    return this;
}

/*
 * The same slot for five more renderer classes. Every one is byte for byte the
 * body above apart from the destructor it calls, and that import slot is what
 * names the class: 0x005EB508, 0x005EB50C, 0x005EB510, 0x005EB518 and
 * 0x005EB584 resolve to srMaterial, srCamera, srScene, srColorSurface and
 * srMeshModel's decorated destructors, which is `original-export` evidence
 * rather than a reading of the body. All five free through the renderer's heap
 * on the same slot, which is what puts them in this file beside srNode's.
 */

// FUNCTION: WIZ8 0x00423e50
srMaterial* srMaterial::scalar_deleting_destructor(unsigned char flags)
{
    this->~srMaterial();
    if ((flags & 1) != 0) {
        SetHeapFree(this);
    }
    return this;
}

// FUNCTION: WIZ8 0x00423e80
srCamera* srCamera::scalar_deleting_destructor(unsigned char flags)
{
    this->~srCamera();
    if ((flags & 1) != 0) {
        SetHeapFree(this);
    }
    return this;
}

// FUNCTION: WIZ8 0x00423eb0
srScene* srScene::scalar_deleting_destructor(unsigned char flags)
{
    this->~srScene();
    if ((flags & 1) != 0) {
        SetHeapFree(this);
    }
    return this;
}

// FUNCTION: WIZ8 0x00423f00
srColorSurface* srColorSurface::scalar_deleting_destructor(unsigned char flags)
{
    this->~srColorSurface();
    if ((flags & 1) != 0) {
        SetHeapFree(this);
    }
    return this;
}

// FUNCTION: WIZ8 0x00424a50
srMeshModel* srMeshModel::scalar_deleting_destructor(unsigned char flags)
{
    this->~srMeshModel();
    if ((flags & 1) != 0) {
        SetHeapFree(this);
    }
    return this;
}

/* The second world, read straight out of the global with no guard. Its type is
   settled by the viewport, which reads a camera member through the same
   object. */
// FUNCTION: WIZ8 0x004512a0
W8World* GetWorld659AB8(void)
{
    return g_world_659ab8;
}
