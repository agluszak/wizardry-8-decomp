#include "wiz8/gameplay_boundaries.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/Object0043A910.h"
#include "wiz8/sr_api.h"

/*
 * Engine Code\GameData.cpp.
 *
 * The bits of the level the party is currently standing in. One global points
 * at that record, and the accessors below read and write single bits of the
 * flag word that leads it. Nothing here establishes what the bits mean, so
 * each is named for the bit it touches; three of them are read together by
 * bodies that do say something about the record.
 */

enum {
    W8_LEVEL_FLAG_0 = 0x001,
    W8_LEVEL_FLAG_4 = 0x010,
    W8_LEVEL_FLAG_5_TO_7 = 0x0e0,
    W8_LEVEL_FLAG_6 = 0x040,
    W8_LEVEL_FLAG_8 = 0x100,
    W8_LEVEL_FLAG_9 = 0x200
};

extern unsigned char g_level_override_00652dba;
extern unsigned char g_flag_00652dce;
extern W8Object0043A910* g_object_6598bc;
extern void Function439CA0();
extern void Function43AAD0();

// FUNCTION: WIZ8 0x0041F260
void Function41F260()
{
    if (g_gd_camera_65a0f8 == 0) {
        g_gd_camera_65a0f8 = new GDCamera;
        if (g_gd_camera_65a0f8 == 0) {
            srAssertFail(
                "gpGDCamera",
                "C:\\Projects\\Wizardry 8\\Engine Code\\GameData.cpp",
                2739,
                0);
        }
    }
    if (g_object_6598bc == 0) {
        g_object_6598bc = new W8Object0043A910;
        if (g_object_6598bc == 0) {
            Function43AAD0();
            return;
        }
    }
    if (g_flag_00652dce != 0) {
        Function439CA0();
        g_flag_00652dce = 0;
    }
    Function43AAD0();
}
/* 0x005EBB34: one float constant with two independent readings - the level
   vector's "no value" here, and Controls.cpp's own range start. Neither is
   proven, so it keeps its address. */
/* Copy one four-byte handle over another. */
// FUNCTION: WIZ8 0x0041cf80
void CopyLevelDataHandle(int* destination, const int* source)
{
    *destination = *source;
}

/* Call one function a fixed number of times. The guard is written against the
   count less one, so a count of zero runs nothing. */
// FUNCTION: WIZ8 0x0041e880
void RepeatLevelDataCallback(int arg_1, int arg_2, int count, void (*callback)(void))
{
    if (count - 1 >= 0) {
        do {
            callback();
            --count;
        } while (count != 0);
    }
}

/* Bit eight: read, cleared and set by three neighbouring bodies. */
// FUNCTION: WIZ8 0x0041efb0
unsigned int GetLevelDataFlag8(void)
{
    if (g_level_data_00652dac != 0) {
        return (g_level_data_00652dac->flags >> 8) & 1;
    }
    return 0;
}

// FUNCTION: WIZ8 0x0041efd0
void ClearLevelDataFlag8(void)
{
    if (g_level_data_00652dac != 0) {
        g_level_data_00652dac->flags &= ~W8_LEVEL_FLAG_8;
    }
}

// FUNCTION: WIZ8 0x0041efe0
void SetLevelDataFlag8(void)
{
    if (g_level_data_00652dac != 0) {
        g_level_data_00652dac->flags |= W8_LEVEL_FLAG_8;
    }
}

// FUNCTION: WIZ8 0x0041eff0
unsigned int GetLevelDataFlag9(void)
{
    if (g_level_data_00652dac != 0) {
        return (g_level_data_00652dac->flags >> 9) & 1;
    }
    return 0;
}

/* Bit four, read out of the low byte rather than the whole word. */
// FUNCTION: WIZ8 0x0041f070
unsigned int GetLevelDataFlag4(void)
{
    if (g_level_data_00652dac != 0) {
        return ((unsigned char)g_level_data_00652dac->flags >> 4) & 1;
    }
    return 0;
}

/* Bits five through seven together, cleared as a group. */
// FUNCTION: WIZ8 0x0041f0c0
void ClearLevelDataFlags5To7(void)
{
    if (g_level_data_00652dac != 0) {
        g_level_data_00652dac->flags &= ~W8_LEVEL_FLAG_5_TO_7;
    }
}

// FUNCTION: WIZ8 0x0041f140
unsigned int GetLevelDataFlag6(void)
{
    if (g_level_data_00652dac != 0) {
        return ((unsigned char)g_level_data_00652dac->flags >> 6) & 1;
    }
    return 0;
}

// FUNCTION: WIZ8 0x0041f160
void ClearLevelDataFlag6(void)
{
    if (g_level_data_00652dac != 0) {
        g_level_data_00652dac->flags &= ~W8_LEVEL_FLAG_6;
    }
}

/* Bit four again, but with a global override: with the bit down, the override
   being set is what withholds the answer. */
// FUNCTION: WIZ8 0x0041f090
int IsLevelDataFlag4EffectivelySet(void)
{
    if (g_level_data_00652dac == 0) {
        return 0;
    }
    if ((g_level_data_00652dac->flags & W8_LEVEL_FLAG_4) == 0 &&
        g_level_override_00652dba != 0) {
        return 0;
    }
    return 1;
}

/* Whether the level has a live vector at 0x88: bit zero has to be up and at
   least one of the three floats has to differ from the default. */
// FUNCTION: WIZ8 0x0041f010
unsigned char HasLevelDataVector(void)
{
    if (g_level_data_00652dac == 0) {
        return 0;
    }
    if ((g_level_data_00652dac->flags & W8_LEVEL_FLAG_0) != 0 &&
        (g_level_data_00652dac->vector_88[0] != g_float_005ebb34 ||
         g_level_data_00652dac->vector_88[1] != g_float_005ebb34 ||
         g_level_data_00652dac->vector_88[2] != g_float_005ebb34)) {
        return 1;
    }
    return 0;
}
