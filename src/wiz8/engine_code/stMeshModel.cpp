#include "wiz8/gameplay_boundaries.h"
#include "wiz8/mesh_model.h"
#include "wiz8/sr_api.h"
#include "surrender/srCore.h"
#include "surrender/srTypeRegistry.h"

#include <string.h>
#include <stdlib.h>

/*
 * Engine Code\stMeshModel.cpp.
 *
 * A mesh model and the sibling chain it can be linked into.
 */

extern void Function4729F0(void* model);

// FUNCTION: WIZ8 0x004712d0
int stMeshModel::FindMappedIndex(short key)
{
    if (key < 0) {
        return -1;
    }
    int index = mapped_keys.IndexOf(key);
    if (index != -1) {
        return *mapped_values.GetAt(index);
    }
    return -1;
}

/* Link one model onto another, setting both ends - so the two pointers are one
   link rather than two independent fields. Unlinking passes nothing. */
// FUNCTION: WIZ8 0x00471d60
void stMeshModel::LinkTo(stMeshModel* other)
{
    next = other;
    if (other != 0) {
        other->previous = this;
    }
}

/* One vertex, bounds-checked against the model's own count and refused
   outright when there is no table at all. */
// FUNCTION: WIZ8 0x00471aa0
void* stMeshModel::GetVertex(unsigned int index)
{
    if (vertices != 0 && index < vertex_count) {
        return vertices[index];
    }
    return 0;
}

// FUNCTION: WIZ8 0x004736d0
int stMeshModel::FindSkinTable004736D0(const char* name)
{
    for (int index = 0; index < skin_table_names.GetCount(); ++index) {
        if (_stricmp(name, *skin_table_names.GetAt(index)) == 0) {
            return index;
        }
    }
    return -1;
}

// FUNCTION: WIZ8 0x00473720
srPtr<srTextureIFace>* stMeshModel::GetTextureTable00473720(int table)
{
    int index = skin_table_ids.IndexOf(table);

    if (index != -1) {
        return *skin_texture_tables.GetAt(index);
    }
    return 0;
}

/* Clone one polygon-texture table under a new name and allocate the parallel
   skin-blanking state used by the renderer. Table ids are the lowest free
   non-negative integer and remain stable independently of vector position. */
// FUNCTION: WIZ8 0x00473260
int stMeshModel::CreateSkinTable00473260(
    const char* name, int base_table)
{
    int base_index = skin_table_ids.IndexOf(base_table);

    if (FindSkinTable004736D0(name) != -1) {
        return -1;
    }

    srPtr<srTextureIFace>* source;
    if (base_index == -1) {
        source = getPolyTexture(0, 0, 0);
    }
    else {
        source = *skin_texture_tables.GetAt(base_index);
    }
    if (source == 0) {
        return -1;
    }

    int table = 0;
    while (skin_table_ids.IndexOf(table) != -1) {
        ++table;
    }

    srPtr<srTextureIFace>* textures =
        new srPtr<srTextureIFace>[polygon_count_230];
    for (int polygon = 0; polygon < polygon_count_230; ++polygon) {
        textures[polygon] = source[polygon];
    }
    skin_texture_tables.Add(textures);
    skin_table_ids.Add(table);

    char* copied_name = static_cast<char*>(malloc(strlen(name) + 1));
    strcpy(copied_name, name);
    skin_table_names.Add(copied_name);

    if (skin_blanking_apt_458 == 0) {
        skin_blanking_apt_458 = new W8GrowableVector<int*>;
        if (skin_blanking_apt_458 == 0) {
            srAssertFail(
                "m_plsSkinBlankingAPT",
                "C:\\Projects\\Wizardry 8\\Engine Code\\stMeshModel.cpp",
                0x5cd,
                0);
        }
    }
    skin_blanking_apt_458->Add(0);

    if (skin_blanking_apt_number_45c == 0) {
        skin_blanking_apt_number_45c = new W8GrowableVector<int>;
        if (skin_blanking_apt_number_45c == 0) {
            srAssertFail(
                "m_plsSkinBlankingAPTNum",
                "C:\\Projects\\Wizardry 8\\Engine Code\\stMeshModel.cpp",
                0x5d6,
                0);
        }
    }
    skin_blanking_apt_number_45c->Add(0);

    if (skin_blanking_checked_460 == 0) {
        skin_blanking_checked_460 = new W8GrowableVector<unsigned char>;
        if (skin_blanking_checked_460 == 0) {
            srAssertFail(
                "m_plsSkinBlankingChecked",
                "C:\\Projects\\Wizardry 8\\Engine Code\\stMeshModel.cpp",
                0x5df,
                0);
        }
    }
    skin_blanking_checked_460->Add(0);
    return table;
}

// FUNCTION: WIZ8 0x00473830
void stMeshModel::RemoveSkinTable00473830(int index)
{
    srPtr<srTextureIFace>* textures = *skin_texture_tables.GetAt(index);
    for (int polygon = 0; polygon < polygon_count_230; ++polygon) {
        textures[polygon] = static_cast<srTextureIFace*>(0);
    }
    delete[] textures;
    free(*skin_table_names.GetAt(index));

    skin_texture_tables.RemoveAt(index);
    skin_table_names.RemoveAt(index);
    skin_table_ids.RemoveAt(index);

    int* apt = *skin_blanking_apt_458->GetAt(index);
    if (apt != 0) {
        delete apt;
    }
    skin_blanking_apt_458->RemoveAt(index);
    skin_blanking_apt_number_45c->RemoveAt(index);
    skin_blanking_checked_460->RemoveAt(index);
}

/* Skin tables are named with the owning cycle plus a one-character suffix.
   Final teardown removes every table whose name has that cycle prefix. */
// FUNCTION: WIZ8 0x00473780
void stMeshModel::RemoveSkinTablesForCycle00473780(const char* cycle_name)
{
    if (cycle_name != 0) {
        for (int index = 0; index < skin_table_names.GetCount(); ++index) {
            char name[200];

            strncpy(name, *skin_table_names.GetAt(index), 199);
            name[199] = '\0';
            name[strlen(name) - 1] = '\0';
            if (strlen(name) != 0 && _stricmp(cycle_name, name) == 0) {
                RemoveSkinTable00473830(index);
                --index;
            }
        }
    }
}

/* Thirteen-byte forwarder onto the model release path. */
// FUNCTION: WIZ8 0x00473180
void ReleaseMeshModel(void* model)
{
    Function4729F0(model);
}

/* The two SurRender registry slots. The literal is the class's own original
   name and the id sits in the Wizardry-registered range at 0x10000 and up,
   which is what separates this class from SurRender's own srMeshModel at
   0x2010. */
// FUNCTION: WIZ8 0x004741f0
const char* stMeshModel::getClassName() const
{
    return "stMeshModel";
}

// FUNCTION: WIZ8 0x004741e0
unsigned long stMeshModel::getClassID() const
{
    return 0x10003;
}

/* Three-level registry builder that names every level by literal - no static
   name getter appears at all, which is what makes it the shortest of the
   three-level forms. The chain is stMeshModel under srMeshModel under srModel
   under srClass. */
// FUNCTION: WIZ8 0x00474820
srRegistry::ClassNode* stMeshModel::getClassNode() const
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x10003);

    if (!node) {
        srRegistry* mesh_registry = srCore.getRegistry();
        srRegistry::ClassNode* mesh = mesh_registry->getClassNode(0x2010);

        if (!mesh) {
            srRegistry* model_registry = srCore.getRegistry();
            srRegistry::ClassNode* model = model_registry->getClassNode(0x2000);

            if (!model) {
                model = model_registry->registerClass(
                    "srModel", srClass::sGetClassNode(), 0x2000, 1);
            }
            mesh = mesh_registry->registerClass("srMeshModel", model, 0x2010, 0);
        }
        node = registry->registerClass("stMeshModel", mesh, 0x10003, 0);
    }
    return node;
}
