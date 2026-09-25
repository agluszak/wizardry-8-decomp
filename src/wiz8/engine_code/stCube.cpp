#include "wiz8/xstatus.h"
#include "wiz8/world_cursor.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/engine_code/Environment.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/mipe.h"
#include "wiz8/engine_code/stParticle.h"
#include "wiz8/engine_code/stCube.h"
#include "wiz8/engine_code/stMeshModel.h"
#include "wiz8/engine_code/stModelInstance.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/fonts.h"
#include "wiz8/local_code/Controls.h"
#include "wiz8/sr_api.h"
#include "wiz8/utility.h"
#include "wiz8/vector.h"
#include "wiz8/geometry.h"
#include "surrender/srNode.h"
#include "surrender/srCamera.h"
#include "surrender/srModeler.h"
#include "surrender/srMaterial.h"
#include "surrender/srColorSurface.h"
#include "surrender/srTextureMap.h"
#include "surrender/srPixelConvert.h"
#include "surrender/srFilter.h"
#include "Font.h"

#include <math.h>
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/PolyPick.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "FileMan.h"

#define ST_CUBE_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\stCube.cpp"

/* Engine Code\stCube.cpp. The cursor's node table and the node selection
   pass; the cursor state itself lives in Cursor3d.cpp. ReleaseWorldCursorNodes
   at 0x0048DB30 lies inside the assertion-backed 0x0048D080-0x0048E7B0 hull;
   0x0048ED00-0x0048EFC0 are the following attribution gap, so no assertion
   names their unit. */

/* Copy the node's world location out; the searchable position resolver treats
   a missing node chain as unresolvable. */
// FUNCTION: WIZ8 0x0048d050
unsigned char W8WorldCursorNode::GetLocation0048D050(srVector3T<float>* position)
{
    if (node_04 != 0) {
        node_04->getLocation(*position);
        return 1;
    }
    return 0;
}

/* The cursor's node table is a real W8GrowableVector object: its static
   initializer at 0x0048D020 constructs it with capacity five and its
   destructor is run through atexit. */
// GLOBAL: WIZ8 0x0065ba58
W8GrowableVector<W8WorldCursorNode*> g_world_cursor_nodes_65ba58(5);

/* The double selection range at 0x005ECAC8: node distances below it select the
   node. Read as 75000.0, not the zero a float view would give. */
// GLOBAL: WIZ8 0x005ecac8
const double g_double_005ecac8 = 75000.0;

// GLOBAL: WIZ8 0x0060a9b0
int g_cursor_node_index_0060a9b0 = -1;

// GLOBAL: WIZ8 0x005ebf50
double g_world_cursor_scale_005ebf50 = 0.002;

// SYNTHETIC: WIZ8 0x0048F260
// W8WorldCursorNode::`scalar deleting destructor'

/* 0x0048D070 is a bare JMP to CreateWorldCursorCube: a tail-jump thunk
   with no distinct source entity. */
// SYNTHETIC: WIZ8 0x0048D070
// CreateWorldCursorCube (tail-jump thunk)

/* Build the numbered cube the world cursor table holds: a 500-unit modeller
   cube, a translucent white material, and a 32x32 texture the label painter
   later fills. */

// FUNCTION: WIZ8 0x0048d080
W8WorldCursorNode* CreateWorldCursorCube(void)
{
    srModeler modeller;
    srModeler::Polygon polygon(4);
    stMeshModel* model = new stMeshModel(0, 0);
    stModelInstance* instance = new stModelInstance(0);
    W8WorldCursorNode* entry = new W8WorldCursorNode;
    /* Polygon(4) already heap-allocates the modelled vertices; these four
       stack Vertices are constructed and unused, matching retail. */
    srModeler::Vertex unused[4];
    (void)unused;
    srModeler::Vertex* vertices = polygon.vertices_30;

    vertices[0].uv_c0[0].x = 1.0f;
    vertices[0].uv_c0[0].y = 1.0f;
    vertices[1].uv_c0[0].x = 0.0f;
    vertices[1].uv_c0[0].y = 1.0f;
    vertices[2].uv_c0[0].x = 0.0f;
    vertices[2].uv_c0[0].y = 0.0f;
    vertices[3].uv_c0[0].x = 1.0f;
    vertices[3].uv_c0[0].y = 0.0f;

    srVector3T<float> npp(-0.5f, 1.0f, 0.5f);
    srVector3T<float> ppp(0.5f, 1.0f, 0.5f);
    srVector3T<float> ppn(0.5f, 1.0f, -0.5f);
    srVector3T<float> npn(-0.5f, 1.0f, -0.5f);
    srVector3T<float> nnn(-0.5f, 0.0f, -0.5f);
    srVector3T<float> pnp(0.5f, 0.0f, 0.5f);
    srVector3T<float> nnp(-0.5f, 0.0f, 0.5f);
    srVector3T<float> pnn(0.5f, 0.0f, -0.5f);

    vertices[0].position_00 = npp;
    vertices[1].position_00 = ppp;
    vertices[2].position_00 = ppn;
    vertices[3].position_00 = npn;
    modeller.addPolygon(polygon);

    vertices[0].position_00 = nnn;
    vertices[1].position_00 = nnp;
    vertices[2].position_00 = npp;
    vertices[3].position_00 = npn;
    modeller.addPolygon(polygon);

    vertices[0].position_00 = nnp;
    vertices[1].position_00 = pnp;
    vertices[2].position_00 = ppp;
    vertices[3].position_00 = npp;
    modeller.addPolygon(polygon);

    vertices[0].position_00 = pnp;
    vertices[1].position_00 = pnn;
    vertices[2].position_00 = ppn;
    vertices[3].position_00 = ppp;
    modeller.addPolygon(polygon);

    vertices[0].position_00 = pnn;
    vertices[1].position_00 = nnn;
    vertices[2].position_00 = npn;
    vertices[3].position_00 = ppn;
    modeller.addPolygon(polygon);

    vertices[0].position_00 = pnn;
    vertices[1].position_00 = pnp;
    vertices[2].position_00 = nnp;
    vertices[3].position_00 = nnn;
    modeller.addPolygon(polygon);

    srMaterial* material = SR_NEW(srMaterial);
    material->setName("Cube Number Material");
    material->autoRelease();

    srVector4T<float> colour;
    colour = 1.0f;
    material->setAmbient(colour);
    colour = 1.0f;
    material->setEmissive(colour);
    colour = 1.0f;
    material->setDiffuse(colour);
    colour = 0.0f;
    material->setSpecular(colour);
    material->parms.diffuse.w = 0.35f;
    material->parms.shininess = 1.0f;
    material->dirty_74 = 1;
    modeller.setMaterial(material, 0, static_cast<srMeshModel::e_side>(0));

    srShader shader;
    shader.value = 0x0100c5b3; /* packed srShader: LEQUAL, color write, dst 1-srcA, fog, modulate */
    modeller.setShader(shader, 0);

    W8ColorSurface* surface =
        SR_NEW(W8ColorSurface)(srPixelConvert::SURFACE_ARGB1555, 0x20UL, 0x20UL);
    if (surface == 0) {
        srAssertFail("psrSurface", ST_CUBE_CPP, 0xac, 0);
    }
    surface->autoRelease();

    unsigned long pixel;
    unsigned char* bytes =
        reinterpret_cast<unsigned char*>(&pixel); // reinterpret-ok: packed colour storage
    bytes[3] = (unsigned char)255.0;
    bytes[2] = (unsigned char)0.0;
    bytes[1] = (unsigned char)0.0;
    bytes[0] = (unsigned char)127.5;
    surface->fill(pixel);
    surface->setFilter(&srBoxFilter);

    srTextureMap* texture = SR_NEW(srTextureMap)(static_cast<srColorSurfaceIFace*>(0));
    texture->autoRelease();
    texture->setSurfacePtr(surface);
    texture->setMipmap(srTextureIFace::MIPMAP_NONE);

    srVector3T<float> scale(500.0f, 500.0f, 500.0f);
    modeller.scale(scale);
    modeller.convert(*model, 1);
    model->setTexture(texture, 0, 0);

    if ((model->control_state_390 & 8) == 0) {
        unsigned long state = model->control_state_390 | 8;
        model->control_state_390 = state;
        model->control_state_390 = state | 8;
    }
    if ((model->control_state_390 & 1) == 0) {
        unsigned long state = model->control_state_390 | 1;
        model->control_state_390 = state;
        model->control_state_390 = state | 8;
        model->updateAllClients(static_cast<srModel::Client::e_update>(0));
    }

    srVector3T<float> minimum;
    srVector3T<float> maximum;
    model->getBoundingBox(minimum, maximum);
    model->autoRelease();
    model->setName("stCube");
    instance->assignModel(model);
    entry->node_04 = instance;

    for (int index = 0; index < 3; ++index) {
        if (entry != 0) {
            entry->numbers_0c[index] = 0;
            if (entry->pUserdata != 0) {
                free(entry->pUserdata);
                entry->pUserdata = 0;
            }
            entry->size_1c = 0;
        }
    }

    unsigned long packed;
    PackColour00433FB0(&packed, 1.0, 0.0, 0.0, 0.5);
    SetWorldCursorNodeColor(entry, packed);
    DrawWorldCursorNodeLabel(entry);
    entry->pUserdata = 0;
    entry->size_1c = 0;
    entry->name_24[0] = 0;

    g_world_cursor_nodes_65ba58.Add(entry);
    return entry;
}

// FUNCTION: WIZ8 0x0048da80
void DestroyWorldCursorCube(W8WorldCursorNode* entry)
{
    if (entry != 0) {
        if (entry->pUserdata != 0) {
            free(entry->pUserdata);
            entry->pUserdata = 0;
        }
        entry->size_1c = 0;
        entry->node_04->setParent(0, 1);
        entry->node_04->release();
        g_world_cursor_nodes_65ba58.Remove(entry);
        delete entry;
    }
}

// FUNCTION: WIZ8 0x0048dbf0
void MoveWorldCursorNode(W8WorldCursorNode* entry, srVector3T<float>* position)
{
    if (position == 0) {
        srAssertFail("vPos", ST_CUBE_CPP, 0x101, 0);
    }
    if (entry == 0) {
        srAssertFail("pCube", ST_CUBE_CPP, 0x102, 0);
    }
    srVector3T<float> location = *position;
    if (entry == 0) {
        srAssertFail("pCube", ST_CUBE_CPP, 0x10b, 0);
    }
    if (entry->node_04 != 0) {
        entry->node_04->setLocation(srVector3T<double>(static_cast<double>(location.x),
                                                       static_cast<double>(location.y),
                                                       static_cast<double>(location.z)));
    }
}

// FUNCTION: WIZ8 0x0048dca0
void RefreshWorldCursorNodeLabel(W8WorldCursorNode* entry)
{
    DrawWorldCursorNodeLabel(entry);
}

/* Paint the three cube numbers onto the model's first texture using the menu
   small font. */
// FUNCTION: WIZ8 0x0048dcb0
void DrawWorldCursorNodeLabel(W8WorldCursorNode* entry)
{
    if (entry != 0) {
        stModelInstance* instance = static_cast<stModelInstance*>(entry->node_04);
        if (instance == 0) {
            srAssertFail("pstModelInstance", ST_CUBE_CPP, 0x124, 0);
        }
        stMeshModel* mesh = static_cast<stMeshModel*>(instance->model());
        if (mesh == 0) {
            srAssertFail("pstMeshModel", ST_CUBE_CPP, 0x127, 0);
        }
        srTextureMap* texture = static_cast<srTextureMap*>(mesh->getTexture(0, 0));
        if (texture == 0) {
            srAssertFail("psrTexture", ST_CUBE_CPP, 0x12a, 0);
        }
        srColorSurfaceIFace* surface = texture->getSurfacePtr();
        surface->fill(entry->color_20);
        unsigned char* data = static_cast<unsigned char*>(surface->getDataPtr());
        if (data == 0) {
            srAssertFail("pBuffer", ST_CUBE_CPP, 0x130, 0);
        }
        SaveFontSettings();
        SetFontDestBuffer(FontDestBuffer, 0, 0, surface->getWidth(), surface->getHeight(),
                          static_cast<unsigned char>(FontDestWrap));
        SetFont(g_smfnt_font_683694);
        SetFontObjectPalette16BPP(g_smfnt_font_683694, g_font_palette_smfnt_68ee10);
        for (int index = 0; index < 3; ++index) {
            wchar_t text[20];
            swprintf(text, g_format_d_0060aa20, entry->numbers_0c[index]);
            gprintf_buffer(data, surface->getPitch(), g_smfnt_font_683694, 0,
                           GetFontHeight(g_smfnt_font_683694) * index, text);
        }
        texture->invalidate();
        RestoreFontSettings();
    }
}

// FUNCTION: WIZ8 0x0048de40
void ScaleWorldCursorNodeX(W8WorldCursorNode* entry, double scale)
{
    srVector3T<float> factors(static_cast<float>(scale), 1.0f, 1.0f);
    if (entry != 0) {
        stMeshModel* model =
            static_cast<stMeshModel*>(static_cast<stModelInstance*>(entry->node_04)->model());
        if (model != 0) {
            model->scale(factors);
        }
    }
}

// FUNCTION: WIZ8 0x0048de90
void ScaleWorldCursorNodeY(W8WorldCursorNode* entry, double scale)
{
    srVector3T<float> factors(1.0f, static_cast<float>(scale), 1.0f);
    if (entry != 0) {
        stMeshModel* model =
            static_cast<stMeshModel*>(static_cast<stModelInstance*>(entry->node_04)->model());
        if (model != 0) {
            model->scale(factors);
        }
    }
}

// FUNCTION: WIZ8 0x0048dee0
void ScaleWorldCursorNodeZ(W8WorldCursorNode* entry, double scale)
{
    srVector3T<float> factors(1.0f, 1.0f, static_cast<float>(scale));
    if (entry != 0) {
        stMeshModel* model =
            static_cast<stMeshModel*>(static_cast<stModelInstance*>(entry->node_04)->model());
        if (model != 0) {
            model->scale(factors);
        }
    }
}

/* Debug wireframe box: project the eight corners of a world-aligned bounding
   box through the world camera, bail if any corner is off-screen, then paint
   the twelve edges into the primary GERD's back buffer. */
// FUNCTION: WIZ8 0x0048DF30
void DrawWorldBox(W8World* world, srVector3T<float> minimum, srVector3T<float> maximum,
                  unsigned long color)
{
    if (world == 0) {
        return;
    }

    srVector3T<float> corners[8];
    corners[0].Set(minimum.x, minimum.y, minimum.z);
    corners[1].Set(minimum.x, minimum.y, maximum.z);
    corners[2].Set(maximum.x, minimum.y, maximum.z);
    corners[3].Set(maximum.x, minimum.y, minimum.z);
    corners[4].Set(minimum.x, maximum.y, minimum.z);
    corners[5].Set(minimum.x, maximum.y, maximum.z);
    corners[6].Set(maximum.x, maximum.y, maximum.z);
    corners[7].Set(maximum.x, maximum.y, minimum.z);

    float viewport[4];
    GetScaledViewportBounds(viewport, viewport + 2);
    float viewport_width = viewport[2] - viewport[0];
    float viewport_height = viewport[3] - viewport[1];

    long screen[8][2];
    int index = 0;
    const srVector3T<float>* corner = corners;
    long (*pixel)[2] = screen;
    do {
        srVector3T<float> projected;
        srVector3T<double> position(corner->x, corner->y, corner->z);
        if (world->camera->project(projected, position) ==
            static_cast<srCamera::e_projectionResult>(-1)) {
            return;
        }
        (*pixel)[0] = static_cast<long>((projected.x * viewport_width + viewport[0]) * 640.0f);
        (*pixel)[1] = static_cast<long>((projected.y * viewport_height + viewport[1]) * 480.0f);
        if ((*pixel)[0] < 0 || (*pixel)[0] > 640 || (*pixel)[1] < 0 || (*pixel)[1] > 480) {
            return;
        }
        ++index;
        ++corner;
        ++pixel;
    } while (index < 8);

    DrawBufferLine(screen[0][0], screen[0][1], screen[1][0], screen[1][1], &color);
    DrawBufferLine(screen[1][0], screen[1][1], screen[2][0], screen[2][1], &color);
    DrawBufferLine(screen[2][0], screen[2][1], screen[3][0], screen[3][1], &color);
    DrawBufferLine(screen[3][0], screen[3][1], screen[0][0], screen[0][1], &color);
    DrawBufferLine(screen[4][0], screen[4][1], screen[5][0], screen[5][1], &color);
    DrawBufferLine(screen[5][0], screen[5][1], screen[6][0], screen[6][1], &color);
    DrawBufferLine(screen[6][0], screen[6][1], screen[7][0], screen[7][1], &color);
    DrawBufferLine(screen[7][0], screen[7][1], screen[4][0], screen[4][1], &color);
    DrawBufferLine(screen[0][0], screen[0][1], screen[4][0], screen[4][1], &color);
    DrawBufferLine(screen[1][0], screen[1][1], screen[5][0], screen[5][1], &color);
    DrawBufferLine(screen[2][0], screen[2][1], screen[6][0], screen[6][1], &color);
    DrawBufferLine(screen[3][0], screen[3][1], screen[7][0], screen[7][1], &color);
}

/* The three label numbers double as generic per-node parameters; the world
   cursor and the master-function table index into them by slot. */
// FUNCTION: WIZ8 0x0048E2B0
int GetWorldCursorNodeParameter(W8WorldCursorNode* entry, int index)
{
    if (entry != 0) {
        return entry->numbers_0c[index];
    }
    return -1;
}

/* Set a parameter and retire the userdata scratch - the handlers lazily
   allocate it again on the next visit. */
// FUNCTION: WIZ8 0x0048E2D0
void SetWorldCursorNodeParameter(W8WorldCursorNode* entry, int index, int value)
{
    if (entry != 0) {
        entry->numbers_0c[index] = value;
        if (entry->pUserdata != 0) {
            free(entry->pUserdata);
            entry->pUserdata = 0;
        }
        entry->size_1c = 0;
    }
}

/* Nearest cursor node to the world camera that is bound to the current pick
   model instance. The screen-point parameters are carried but unused - the
   pick is purely camera-distance based. */
// FUNCTION: WIZ8 0x0048e310
static W8WorldCursorNode* FindNearestWorldCursorNode(int x, int y)
{
    float nearest = 999999.0f;
    W8WorldCursorNode* result = 0;
    int count = g_world_cursor_nodes_65ba58.count;

    for (int index = 0; index < count; ++index) {
        W8WorldCursorNode* entry = *g_world_cursor_nodes_65ba58.GetAt(index);
        if (entry != 0 && entry->node_04 != 0 && entry->node_04 == GetPickedModelInstance()) {
            srVector3T<double> camera;
            g_world->camera->getLocation(camera);
            srVector3T<double> node;
            entry->node_04->getLocation(node);
            double dx = node.x - camera.x;
            double dy = node.y - camera.y;
            double dz = node.z - camera.z;
            float distance = static_cast<float>(sqrt(dx * dx + dy * dy + dz * dz));
            if (distance < nearest) {
                nearest = distance;
                result = entry;
            }
        }
    }
    return result;
}

// FUNCTION: WIZ8 0x0048e3e0
W8WorldCursorNode* PickWorldCursorNodeAtScreenPoint(int x, int y)
{
    return FindNearestWorldCursorNode(x, y);
}

/* Store the packed fill colour and repaint the cube numbers. */
// FUNCTION: WIZ8 0x0048e400
void SetWorldCursorNodeColor(W8WorldCursorNode* entry, unsigned long color)
{
    entry->color_20 = color;
    DrawWorldCursorNodeLabel(entry);
}

/* Pack the RGB components at full alpha and repaint. */
// FUNCTION: WIZ8 0x0048E420
void SetWorldCursorNodeColorComponents(W8WorldCursorNode* entry, float red, float green, float blue)
{
    unsigned long packed;
    SetWorldCursorNodeColor(entry, *PackColour00433FB0(&packed, 1.0, red, green, blue));
}

// FUNCTION: WIZ8 0x0048e470
unsigned int LoadWorldCursorNodeStates(int handle)
{
    int version;
    unsigned int count;
    unsigned int index;
    bool success = true;

    if (!FileRead(handle, &version, 4, 0)) {
        return 0;
    }
    if (version == 0x21122112) {
        version = 1;
    }
    if (!FileRead(handle, &count, 4, 0)) {
        return 0;
    }

    for (index = 0; index < count; ++index) {
        W8WorldCursorNode* cube = 0;
        bool temporary = false;

        if (version < 2) {
            cube = GetWorldCursorNode(index);
        } else {
            char name[0x20];
            int cube_index;

            FileRead(handle, name, sizeof(name), 0);
            for (cube_index = 0; cube_index < g_world_cursor_nodes_65ba58.GetCount();
                 ++cube_index) {
                W8WorldCursorNode* candidate = *g_world_cursor_nodes_65ba58.GetAt(cube_index);
                if (strcmp(candidate->name_24, name) == 0) {
                    cube = candidate;
                    break;
                }
            }
            if (cube == 0) {
                cube = new W8WorldCursorNode;
                temporary = true;
            }
        }

        if (success && FileRead(handle, &cube->size_1c, 4, 0)) {
            success = true;
        } else {
            success = false;
        }
        if (cube->size_1c != 0) {
            cube->pUserdata = malloc(cube->size_1c);
            if (cube->pUserdata == 0) {
                srAssertFail("pCube->pUserdata", ST_CUBE_CPP, 0x3c8, 0);
            }
            memset(cube->pUserdata, 0, cube->size_1c);
            if (success && FileRead(handle, cube->pUserdata, cube->size_1c, 0)) {
                success = true;
            } else {
                success = false;
            }
        }
        if (temporary && cube != 0) {
            if (cube->pUserdata != 0) {
                free(cube->pUserdata);
                cube->pUserdata = 0;
            }
            cube->size_1c = 0;
            delete cube;
        }
    }
    return count;
}

// FUNCTION: WIZ8 0x0048e6d0
unsigned char SaveWorldCursorNodeStates(int handle)
{
    bool ok = true;
    int version = 2;
    unsigned int count;
    unsigned int index;
    W8WorldCursorNode* node;

    if (!FileWrite(handle, &version, 4, 0)) {
        return 0;
    }
    count = g_world_cursor_nodes_65ba58.count;
    if (!FileWrite(handle, &count, 4, 0)) {
        return 0;
    }
    for (index = 0; index < count && ok; ++index) {
        node = *g_world_cursor_nodes_65ba58.GetAt(index);
        ok = FileWrite(handle, node->name_24, sizeof(node->name_24), 0) &&
             FileWrite(handle, &node->size_1c, 4, 0);
        if (node->size_1c != 0) {
            ok = ok && FileWrite(handle, node->pUserdata, node->size_1c, 0);
        }
    }
    return ok;
}

// FUNCTION: WIZ8 0x0048e7b0
unsigned int LoadWorldCursorNodes(int handle)
{
    int version;
    unsigned int count;
    unsigned int index;
    bool success = true;

    if (!FileRead(handle, &version, 4, 0)) {
        return 0;
    }
    if (static_cast<unsigned int>(version) == 0xdeadd00d) {
        version = 1;
    } else if (version > 2) {
        FileRead(handle, &gXStatus.mipe_cube_serial, 4, 0);
    } else {
        gXStatus.mipe_cube_serial = 100;
    }
    if (!FileRead(handle, &count, 4, 0)) {
        return 0;
    }

    for (index = 0; index < count; ++index) {
        W8WorldCursorNode* cube = CreateWorldCursorCube();
        float minimum[3];
        float maximum[3];
        float location[3];
        int component;

        if (version < 2) {
            sprintf(cube->name_24, "Cube%d", index);
        } else {
            FileRead(handle, cube->name_24, 0x20, 0);
            if (cube->name_24[0] == 0) {
                sprintf(cube->name_24, "Cube%3.3d", gXStatus.mipe_cube_serial++);
            }
        }
        for (component = 0; component < 3; ++component) {
            if (success && FileRead(handle, &cube->numbers_0c[component], 4, 0)) {
                success = true;
            } else {
                success = false;
            }
        }
        for (component = 0; component < 3; ++component) {
            if (success && FileRead(handle, &minimum[component], 4, 0)) {
                success = true;
            } else {
                success = false;
            }
        }
        for (component = 0; component < 3; ++component) {
            if (success && FileRead(handle, &maximum[component], 4, 0)) {
                success = true;
            } else {
                success = false;
            }
        }

        srVector3T<float> scale(
            (maximum[0] - minimum[0]) * static_cast<float>(g_world_cursor_scale_005ebf50),
            (maximum[1] - minimum[1]) * static_cast<float>(g_world_cursor_scale_005ebf50),
            (maximum[2] - minimum[2]) * static_cast<float>(g_world_cursor_scale_005ebf50));
        if (cube != 0) {
            stModelInstance* instance = static_cast<stModelInstance*>(cube->node_04);
            if (instance != 0) {
                stMeshModel* model = static_cast<stMeshModel*>(instance->model());
                if (model != 0) {
                    model->scale(scale);
                }
            }
        }

        for (component = 0; component < 3; ++component) {
            if (success && FileRead(handle, &location[component], 4, 0)) {
                success = true;
            } else {
                success = false;
            }
        }
        if (cube == 0) {
            srAssertFail("pCube", ST_CUBE_CPP, 0x10b, 0);
        }
        if (cube->node_04 != 0) {
            srVector3T<double> node_location(static_cast<double>(location[0]),
                                             static_cast<double>(location[1]),
                                             static_cast<double>(location[2]));
            cube->node_04->setLocation(node_location);
        }
        if (success && FileRead(handle, &cube->value_08, 4, 0)) {
            success = true;
        } else {
            success = false;
        }
        DrawWorldCursorNodeLabel(cube);
    }
    return count;
}

// FUNCTION: WIZ8 0x0048ead0
unsigned char SaveWorldCursorNodes(int handle)
{
    bool ok = true;
    int version = 3;
    unsigned int count;
    unsigned int index;
    int component;
    srVector3T<float> location;
    W8WorldCursorNode* node;
    srNode::BoundInfo bounds;

    if (!FileWrite(handle, &version, 4, 0)) {
        return 0;
    }
    FileWrite(handle, &gXStatus.mipe_cube_serial, 4, 0);
    count = g_world_cursor_nodes_65ba58.count;
    if (!FileWrite(handle, &count, 4, 0)) {
        return 0;
    }
    for (index = 0; index < count && ok; ++index) {
        node = *g_world_cursor_nodes_65ba58.GetAt(index);
        FileWrite(handle, node->name_24, sizeof(node->name_24), 0);
        for (component = 0; component < 3 && ok; ++component) {
            ok = FileWrite(handle, &node->numbers_0c[component], 4, 0);
        }
        node->node_04->getLocalBounds(bounds);
        location = node->node_04->getLocation();
        ok = ok && FileWrite(handle, &bounds.minimum.x, 4, 0) &&
             FileWrite(handle, &bounds.minimum.y, 4, 0) &&
             FileWrite(handle, &bounds.minimum.z, 4, 0) &&
             FileWrite(handle, &bounds.maximum.x, 4, 0) &&
             FileWrite(handle, &bounds.maximum.y, 4, 0) &&
             FileWrite(handle, &bounds.maximum.z, 4, 0) && FileWrite(handle, &location.x, 4, 0) &&
             FileWrite(handle, &location.y, 4, 0) && FileWrite(handle, &location.z, 4, 0) &&
             FileWrite(handle, &node->value_08, 4, 0);
    }
    return ok;
}

// FUNCTION: WIZ8 0x0048ED00
int GetWorldCursorNodeCount(void)
{
    return g_world_cursor_nodes_65ba58.count;
}

// FUNCTION: WIZ8 0x0048ED10
W8WorldCursorNode* GetWorldCursorNode(int index)
{
    return *g_world_cursor_nodes_65ba58.GetAt(index);
}

/* Attach or detach the node's scene node under the world's dynamic scene. */
// FUNCTION: WIZ8 0x0048ED30
void AttachWorldCursorNode(W8WorldCursorNode* entry, unsigned char attached)
{
    if (entry != 0) {
        if (attached != 0) {
            entry->node_04->setParent(g_world->dynamic_scene, 1);
            return;
        }
        entry->node_04->setParent(0, 1);
    }
}

/* Copy `name` into the node's fixed 0x20-byte label field, always leaving a
   terminator. */
// FUNCTION: WIZ8 0x0048F110
void SetWorldCursorNodeName(W8WorldCursorNode* entry, const char* name)
{
    strncpy(entry->name_24, name, 0x20);
    entry->name_24[0x1f] = 0;
}

/* Reparent the world's cursor-attached nodes onto the dynamic scene, or
   detach them when hidden. Levels.cpp drives this from the world-cursor
   flag. */
// FUNCTION: WIZ8 0x0048ED70
void SetWorldCursorNodesVisible(unsigned char visible)
{
    unsigned int count = g_world_cursor_nodes_65ba58.count;

    for (unsigned int index = 0; index < count; ++index) {
        W8WorldCursorNode* entry = *g_world_cursor_nodes_65ba58.GetAt(index);

        if (entry != 0) {
            srNode* parent = 0;
            if (visible != 0) {
                parent = g_world->dynamic_scene;
            }
            entry->node_04->setParent(parent, 1);
        }
    }
}

/* Answer the next table node after `after` whose world-space bounds contain
   `point`; a null `after` starts the walk at the head of the table. When
   `after` is the last entry the walk answers null immediately. */
// FUNCTION: WIZ8 0x0048EDD0
W8WorldCursorNode* FindWorldCursorNodeAtPoint(W8WorldCursorNode* after, srVector3T<float>* point)
{
    unsigned int index = 0;
    unsigned int count = g_world_cursor_nodes_65ba58.count;

    if (after != 0) {
        for (unsigned int i = 0; i < count; ++i) {
            if (*g_world_cursor_nodes_65ba58.GetAt(i) == after) {
                index = i + 1;
                if (i == count - 1) {
                    return 0;
                }
            }
        }
    }
    while (index < count) {
        W8WorldCursorNode* entry = *g_world_cursor_nodes_65ba58.GetAt(index);
        if (entry->node_04 != 0) {
            srNode::BoundInfo bounds;
            entry->node_04->getLocalBounds(bounds);
            srVector3T<double> location = entry->node_04->getLocation();
            bounds.minimum.x += static_cast<float>(location.x);
            bounds.minimum.y += static_cast<float>(location.y);
            bounds.minimum.z += static_cast<float>(location.z);
            bounds.maximum.x += static_cast<float>(location.x);
            bounds.maximum.y += static_cast<float>(location.y);
            bounds.maximum.z += static_cast<float>(location.z);
            if (PointInsideBounds004BE870(point, &bounds.minimum, &bounds.maximum) != 0) {
                return entry;
            }
        }
        ++index;
    }
    return 0;
}

/* Copy the node's userdata pointer and size into the caller's slots; either
   out pointer may be null. */
// FUNCTION: WIZ8 0x0048EF00
void GetWorldCursorNodeUserdata(W8WorldCursorNode* entry, char** buffer, int* size)
{
    if (entry != 0) {
        if (buffer != 0) {
            *buffer = static_cast<char*>(entry->pUserdata);
        }
        if (size != 0) {
            *size = entry->size_1c;
        }
    } else {
        if (buffer != 0) {
            *buffer = 0;
        }
        if (size != 0) {
            *size = 0;
        }
    }
}

/* Allocate the node's userdata scratch, or release it when `size` is zero.
   Retail overwrites an existing allocation without freeing it first. */
// FUNCTION: WIZ8 0x0048EF40
void SetWorldCursorNodeUserdataSize(W8WorldCursorNode* entry, int size)
{
    if (entry != 0) {
        if (size != 0) {
            entry->pUserdata = malloc(size);
            if (entry->pUserdata == 0) {
                srAssertFail("pCube->pUserdata", ST_CUBE_CPP, 0x3c8, 0);
            }
            memset(entry->pUserdata, 0, size);
        } else {
            if (entry->pUserdata != 0) {
                free(entry->pUserdata);
                entry->pUserdata = 0;
            }
        }
        entry->size_1c = size;
    }
}

/* Select the cursor node nearest the camera within the selection distance,
   remembering it for the next call. Answers whether one was close enough.
   Retail loads entry->node_04->getLocation() before TEST ESI,ESI on both the
   cached-index path and the table scan; there is no separate node_04 null
   check. Keep that load order. */
// FUNCTION: WIZ8 0x0048EFC0
bool SelectWorldCursorNode(void)
{
    if (g_world != 0 && g_world->camera != 0) {
        srVector3T<float> camera_position;
        srVector3T<double> camera_location;

        GetCameraPosition(&camera_position);
        camera_location.SetFromFloat(&camera_position);
        int selected = g_cursor_node_index_0060a9b0;
        if (selected >= 0 && selected < g_world_cursor_nodes_65ba58.count) {
            W8WorldCursorNode* entry = g_world_cursor_nodes_65ba58.data[selected];
            srVector3T<double> target = entry->node_04->getLocation();

            if (entry != 0) {
                srVector3T<double> delta = target;

                delta -= camera_location;
                if (delta.Length() < g_double_005ecac8) {
                    return 1;
                }
            }
        }
        int count = g_world_cursor_nodes_65ba58.count;
        for (int index = 0; index < count; ++index) {
            W8WorldCursorNode* entry = g_world_cursor_nodes_65ba58.data[index];
            srVector3T<double> target = entry->node_04->getLocation();

            if (entry != 0) {
                srVector3T<double> delta = target;

                delta -= camera_location;
                if (delta.Length() < g_double_005ecac8) {
                    g_cursor_node_index_0060a9b0 = index;
                    return 1;
                }
            }
        }
    }
    return 0;
}

/* Release every node the cursor table still holds: free its scratch buffer,
   detach and release its scene node, drop it from the table and run its own
   destructor. A null head with a nonzero count spins, as in retail. */
// FUNCTION: WIZ8 0x0048DB30
void ReleaseWorldCursorNodes0048DB30(void)
{
    while (g_world_cursor_nodes_65ba58.count != 0) {
        W8WorldCursorNode* entry = g_world_cursor_nodes_65ba58.data[0];

        if (entry != 0) {
            if (entry->pUserdata != 0) {
                free(entry->pUserdata);
                entry->pUserdata = 0;
            }
            entry->size_1c = 0;
            entry->node_04->setParent(0, 1);
            entry->node_04->release();
            g_world_cursor_nodes_65ba58.Remove(entry);
            delete entry;
        }
    }
}
