#include "wiz8/world_cursor.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/engine_code/Environment.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/local_screens/MainGameScreen.h"
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
#include "surrender/srNode.h"
#include "surrender/srModeler.h"
#include "surrender/srMaterial.h"
#include "surrender/srColorSurface.h"
#include "surrender/srTextureMap.h"
#include "surrender/srPixelConvert.h"
#include "surrender/srFilter.h"
#include "Font.h"

#include <math.h>
#include <new>
#include <stdlib.h>
#include <wchar.h>
#include "wiz8/engine_code/GameData.h"
#include "wiz8/local_screens/CharacterScreen.h"

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

/* Build the numbered cube the world cursor table holds: a 500-unit modeller
   cube, a translucent white material, and a 32x32 texture the label painter
   later fills. */
// FUNCTION: WIZ8 0x0048d080
W8WorldCursorNode* CreateWorldCursorCube0048D080(void)
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
    colour.x = 1.0f;
    colour.y = 1.0f;
    colour.z = 1.0f;
    colour.w = 1.0f;
    material->setAmbient(colour);
    colour.x = 1.0f;
    colour.y = 1.0f;
    colour.z = 1.0f;
    colour.w = 1.0f;
    material->setEmissive(colour);
    colour.x = 1.0f;
    colour.y = 1.0f;
    colour.z = 1.0f;
    colour.w = 1.0f;
    material->setDiffuse(colour);
    colour.x = 0.0f;
    colour.y = 0.0f;
    colour.z = 0.0f;
    colour.w = 0.0f;
    material->setSpecular(colour);
    material->parms_18.diffuse.w = 0.35f;
    material->parms_18.shininess = 1.0f;
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
            if (entry->buffer_18 != 0) {
                free(entry->buffer_18);
                entry->buffer_18 = 0;
            }
            entry->size_1c = 0;
        }
    }

    unsigned long packed;
    PackColour00433FB0(&packed, 1.0, 0.0, 0.0, 0.5);
    SetWorldCursorNodeColor0048E400(entry, packed);
    DrawWorldCursorNodeLabel0048DCB0(entry);
    entry->buffer_18 = 0;
    entry->size_1c = 0;
    entry->flag_24 = 0;

    g_world_cursor_nodes_65ba58.Add(entry);
    return entry;
}

/* Paint the three cube numbers onto the model's first texture using the menu
   small font. */
// FUNCTION: WIZ8 0x0048dcb0
void DrawWorldCursorNodeLabel0048DCB0(W8WorldCursorNode* entry)
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

/* Store the packed fill colour and repaint the cube numbers. */
// FUNCTION: WIZ8 0x0048e400
void SetWorldCursorNodeColor0048E400(W8WorldCursorNode* entry, unsigned long color)
{
    entry->color_20 = color;
    DrawWorldCursorNodeLabel0048DCB0(entry);
}

// FUNCTION: WIZ8 0x0048ED00
int GetWorldCursorNodeCount0048ED00(void)
{
    return g_world_cursor_nodes_65ba58.count;
}

/* Reparent the world's cursor-attached nodes onto the dynamic scene, or
   detach them when hidden. Levels.cpp drives this from the world-cursor
   flag. */
// FUNCTION: WIZ8 0x0048ED70
void SetWorldCursorNodesVisible0048ED70(unsigned char visible)
{
    unsigned int count = g_world_cursor_nodes_65ba58.count;

    for (unsigned int index = 0; index < count; ++index) {
        W8WorldCursorNode* entry = g_world_cursor_nodes_65ba58.data[index];

        if (entry != 0) {
            srNode* parent = 0;
            if (visible != 0) {
                parent = g_world->dynamic_scene;
            }
            entry->node_04->setParent(parent, 1);
        }
    }
}

/* Select the cursor node nearest the camera within the selection distance,
   remembering it for the next call. Answers whether one was close enough.
   Retail loads entry->node_04->getLocation() before TEST ESI,ESI on both the
   cached-index path and the table scan; there is no separate node_04 null
   check. Keep that load order. */
// FUNCTION: WIZ8 0x0048EFC0
bool SelectWorldCursorNode0048EFC0(void)
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
            if (entry->buffer_18 != 0) {
                free(entry->buffer_18);
                entry->buffer_18 = 0;
            }
            entry->size_1c = 0;
            entry->node_04->setParent(0, 1);
            entry->node_04->release();
            g_world_cursor_nodes_65ba58.Remove(entry);
            delete entry;
        }
    }
}
