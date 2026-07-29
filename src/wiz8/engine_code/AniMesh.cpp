#include "wiz8/engine_code/AniMesh.h"
#include "wiz8/sr_api.h"

#include <stdlib.h>

#define ANI_MESH_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\AniMesh.cpp"

extern unsigned char Function4B5D00(int value, W8AniMesh* mesh, int load);
extern void Function4B63F0(W8AniMesh* mesh, int value);
extern int g_storage_state_65be80;

// FUNCTION: WIZ8 0x004b5880
void DestroyAniMesh004B5880(W8AniMesh* mesh)
{
    if (mesh == 0) {
        srAssertFail("pAniMesh", ANI_MESH_CPP, 0x91, 0);
    }
    if ((mesh->flags_00 & 1) != 0) {
        Function4B63F0(mesh, 1);
    }
    free(mesh->allocation_2c);
    free(mesh->allocation_30);
    free(mesh);
}

// FUNCTION: WIZ8 0x004b64f0
unsigned char AniMeshValue004B64F0(W8AniMesh* mesh)
{
    if (mesh == 0) {
        srAssertFail("pAniMesh", ANI_MESH_CPP, 0x2be, 0);
        return 0xff;
    }
    if ((mesh->flags_00 & 2) == 0) {
        if (Function4B5D00(0, mesh, 1) == 0) {
            srAssertFail("0", ANI_MESH_CPP, 0x2c6, 0);
            return 0xff;
        }
    }
    return mesh->value_01;
}

// FUNCTION: WIZ8 0x004b66e0
unsigned char AniMeshRadius004B66E0(W8AniMesh* mesh, float* radius)
{
    if (mesh == 0 || radius == 0) {
        srAssertFail("pAniMesh&&pflRadius", ANI_MESH_CPP, 0x348, 0);
    }
    if (mesh != 0 && radius != 0) {
        if ((mesh->flags_00 & 4) == 0) {
            if (Function4B5D00(0, mesh, 1) == 0) {
                srAssertFail("0", ANI_MESH_CPP, 0x350, 0);
                return 0;
            }
        }
        *radius = mesh->radius_20;
        mesh->counter_3c = g_storage_state_65be80;
        ++g_storage_state_65be80;
        return 1;
    }
    return 0;
}

// FUNCTION: WIZ8 0x004b6860
void AniMeshSetFlag10004B6860(W8AniMesh* mesh, int, signed char enabled)
{
    if (mesh == 0) {
        srAssertFail("pAniMesh", ANI_MESH_CPP, 0x3b7, 0);
    }
    if (enabled != 0) {
        mesh->flags_00 |= 0x10;
        return;
    }
    mesh->flags_00 &= ~0x10;
}
