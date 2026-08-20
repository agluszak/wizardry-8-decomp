#include "wiz8/unattributed/quarantine_common.h"
#include "wiz8/cursor.h"

/* Address quarantine 00424041-0042a36f; bounds come from adjacent
   assertion-backed original translation-unit intervals. */

// FUNCTION: WIZ8 0x00425820
void ClearNodeFlag(srNode* node)
{
    if (node) {
        node->setFlag(srNode::FLAG_POSITIONAL_0);
    }
}
// FUNCTION: WIZ8 0x00427810
srModelInstance* GetValue65962C(void)
{
    return g_current_model_instance_65962c;
}
// FUNCTION: WIZ8 0x00427820
void SetValue65962C(srModelInstance* value)
{
    g_current_model_instance_65962c = value;
}
// FUNCTION: WIZ8 0x00428010
unsigned char ClearFlag603C60(void)
{
    g_flag_603c60 = 0;
    return 1;
}
// FUNCTION: WIZ8 0x00428020
unsigned char SetFlag603C60(void)
{
    g_flag_603c60 = 1;
    return 1;
}
// FUNCTION: WIZ8 0x00428A90
void SetRendererMode6596EC(void)
{
    g_dword_6596ec = 2;
}
// FUNCTION: WIZ8 0x00428AA0
void SetRendererModePair(void)
{
    g_dword_6596f0 = 2;
    g_dword_6596ec = 2;
}
// FUNCTION: WIZ8 0x004291C0
unsigned char GetRendererModeByte(void)
{
    return (unsigned char)g_renderer_mode_603d74;
}
// FUNCTION: WIZ8 0x00429200
void SetValue659668(int value)
{
    g_value_659668 = value;
}
// FUNCTION: WIZ8 0x004297D0
bool HasScreenTransitionObjects(void)
{
    return g_screen_transition_object_count_654aac != 0;
}
// FUNCTION: WIZ8 0x004298E0
void SetFlag603C4C(unsigned char value)
{
    g_flag_603c4c = value;
}
extern "C" {
// FUNCTION: WIZ8 0x004298F0
unsigned char Function4298F0(void)
{
    FARPROC extended;
    LARGE_INTEGER available;
    LARGE_INTEGER capacity;
    LARGE_INTEGER free_bytes;
    DWORD sectors_per_cluster;
    DWORD bytes_per_sector;
    DWORD free_clusters;
    DWORD total_clusters;
    unsigned int megabytes;
    unsigned char enough;

    /* One nested expression, not two statements: the original pushes the
       procedure name before it calls GetModuleHandleA, which is what
       right-to-left argument evaluation of a single call gives. */
    extended = GetProcAddress(GetModuleHandleA("kernel32.dll"),
                              "GetDiskFreeSpaceExA");
    if (extended != NULL) {
        GetDiskFreeSpaceExA(NULL, (PULARGE_INTEGER)&available,
                            (PULARGE_INTEGER)&capacity, (PULARGE_INTEGER)&free_bytes);
        megabytes = (unsigned int)(free_bytes.QuadPart / 0x100000);
        enough = megabytes >= 0x100;
        return enough;
    }
    GetDiskFreeSpaceA(NULL, &sectors_per_cluster, &bytes_per_sector,
                      &free_clusters, &total_clusters);
    megabytes = (unsigned int)((__int64)sectors_per_cluster * bytes_per_sector
                               * free_clusters / 0x400 / 0x400);
    enough = megabytes >= 0x100;
    return enough;
}
}
// FUNCTION: WIZ8 0x00429AF0
void __fastcall ReleaseOwnedClass(srClass** owner)
{
    if (*owner) {
        (*owner)->release();
    }
}

/* Fill a caller-supplied point with the sum of the two global pairs, after the
   refresh that recomputes them. Fifty callers reach this; what the two pairs
   mean is not established beyond the sum, so both keep address-qualified names. */
// FUNCTION: WIZ8 0x004284f0
void GetScreenPoint004284F0(W8ScreenPoint* point)
{
    if (point != 0) {
        Function00428340();
        point->x = g_cursor_hotspot_x_6596bc + g_cursor_width_654ad0;
        point->y = g_cursor_hotspot_y_6596c0 + g_cursor_height_654ad4;
    }
}
