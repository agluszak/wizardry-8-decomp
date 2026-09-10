#include "wiz8/regions.h"
#include "wiz8/bringup_gates.h"
#include "wiz8/local_screens/MGSSpellCasting.h"
#include "wiz8/local_screens/MainMenuScreen.h"
#include "wiz8/cursor.h"
#include "wiz8/screen_state.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/local_code/ButtonSound.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/utility.h"
#include "wiz8/sr_api.h"
#include "input.h"
#include "timer.h"

#include <new>
#include <wchar.h>


enum { W8_SCREEN_WIDTH = 640, W8_SCREEN_HEIGHT = 480, W8_HELP_MARGIN = 2 };
enum { W8_REGION_MODE_MASK = 0xf };

/* The retail catalog contains 51 statically declared sets and 313 statically
   declared regions.  Dynamically constructed controls append after that
   catalog; region zero is the template copied into each appended record. */
// GLOBAL: WIZ8 0x00617b18
unsigned int g_region_set_count = 51;
// GLOBAL: WIZ8 0x0061f238
W8RegionSet g_region_sets[300] = {
    { 0, 0, 0 },
    { 0, 1, 6 }
};
// GLOBAL: WIZ8 0x00617b1c
unsigned int g_region_count = 313;
// GLOBAL: WIZ8 0x00620048
W8Region g_regions[1500] = {

    { 0x00000001, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 },
    { 0x00000001, 174, 138, 467, 182, MainMenuIntroduction, 0, 0, 0, -1, 0 },
    { 0x00000001, 140, 187, 501, 231, MainMenuNewGame, 0, 0, 0, -1, 0 },

    { 0x00000001, 204, 235, 436, 279, MainMenuLoadGame, 0, 0, 0, -1, 0 },
    { 0x00000001, 239, 284, 403, 328, MainMenuCredits, 0, 0, 0, -1, 0 },
    { 0x00000001, 234, 335, 408, 379, MainMenuOptions, 0, 0, 0, -1, 0 },

    { 0x00000001, 279, 423, 364, 467, MainMenuExit, 0, 0, 0, -1, 0 },
    { 0x00000001, 286, 382, 351, 402, IntroScreenRegionEvent, 0, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 639, 479, Function005BC7A0, 0, 0, 0, 0, 0 },

    { 0x00000001, 0, 0, 0, 0, Function0055E690, 0, 0, 0, -1, 0 },
    { 0x00000001, 24, 68, 42, 85, Function005673B0, 0, 0, 0, -1, 0 },
    { 0x00000001, 595, 68, 613, 85, Function005673B0, 1, 0, 0, -1, 0 },

    { 0x00000001, 24, 153, 42, 170, Function005673B0, 2, 0, 0, -1, 0 },
    { 0x00000001, 595, 153, 613, 170, Function005673B0, 3, 0, 0, -1, 0 },
    { 0x00000001, 24, 238, 42, 255, Function005673B0, 4, 0, 0, -1, 0 },

    { 0x00000001, 595, 238, 613, 255, Function005673B0, 5, 0, 0, -1, 0 },
    { 0x00000001, 24, 323, 42, 340, Function005673B0, 6, 0, 0, -1, 0 },
    { 0x00000001, 595, 323, 613, 340, Function005673B0, 7, 0, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function0059BD20, 0, 1, 0, 1984, 0 },
    { 0x00000001, 0, 0, 0, 0, Function0059BD20, 1, 1, 0, 1984, 0 },
    { 0x00000001, 0, 0, 0, 0, Function0059BD20, 2, 1, 0, 1984, 0 },

    { 0x00000001, 0, 0, 0, 0, Function0059BD20, 3, 1, 0, 1984, 0 },
    { 0x00000001, 0, 0, 0, 0, Function0059BD20, 4, 1, 0, 1984, 0 },
    { 0x00000001, 0, 0, 0, 0, Function0059BD20, 5, 1, 0, 1984, 0 },

    { 0x00000001, 0, 0, 0, 0, Function0059BD20, 6, 1, 0, 1984, 0 },
    { 0x00000001, 0, 0, 0, 0, Function0059BD20, 7, 1, 0, 1984, 0 },
    { 0x00000001, 0, 0, 0, 0, Function0059C260, 0, 1, 0, 1985, 0 },

    { 0x00000001, 0, 0, 0, 0, Function0059C260, 1, 1, 0, 1985, 0 },
    { 0x00000001, 0, 0, 0, 0, Function0059C260, 2, 1, 0, 1985, 0 },
    { 0x00000001, 0, 0, 0, 0, Function0059C260, 3, 1, 0, 1985, 0 },

    { 0x00000001, 0, 0, 0, 0, Function0059C260, 4, 1, 0, 1985, 0 },
    { 0x00000001, 0, 0, 0, 0, Function0059C260, 5, 1, 0, 1985, 0 },
    { 0x00000001, 0, 0, 0, 0, Function0059C260, 6, 1, 0, 1985, 0 },

    { 0x00000001, 0, 0, 0, 0, Function0059C260, 7, 1, 0, 1985, 0 },
    { 0x00000002, 32, 27, 8, 0, Function005667A0, 0, 1, 0, 25, 0 },
    { 0x00000002, 95, 27, 8, 0, Function00566AE0, 0, 1, 0, 26, 0 },

    { 0x00000001, 23, 18, 104, 89, Function00565990, 0, 0, 0, -1, 0 },
    { 0x00000001, 21, 91, 105, 101, Function0058F240, 0, 0, 0, -1, 0 },
    { 0x00000001, 4, 42, 19, 90, Function00566E20, 0, 1, 0, 28, 0 },

    { 0x00000001, 109, 42, 123, 90, Function005670C0, 0, 1, 0, 24, 0 },
    { 0x00000002, 608, 27, 8, 0, Function005667A0, 1, 1, 0, 25, 0 },
    { 0x00000002, 544, 27, 8, 0, Function00566AE0, 1, 1, 0, 26, 0 },

    { 0x00000001, 535, 18, 616, 89, Function00565990, 1, 0, 0, -1, 0 },
    { 0x00000001, 539, 91, 612, 101, Function0058F240, 1, 0, 0, -1, 0 },
    { 0x00000001, 621, 42, 635, 90, Function00566E20, 1, 1, 0, 28, 0 },

    { 0x00000001, 517, 42, 531, 90, Function005670C0, 1, 1, 0, 24, 0 },
    { 0x00000002, 32, 112, 8, 0, Function005667A0, 2, 1, 0, 25, 0 },
    { 0x00000002, 95, 112, 8, 0, Function00566AE0, 2, 1, 0, 26, 0 },

    { 0x00000001, 23, 103, 104, 174, Function00565990, 2, 0, 0, -1, 0 },
    { 0x00000001, 21, 176, 105, 186, Function0058F240, 2, 0, 0, -1, 0 },
    { 0x00000001, 4, 125, 19, 173, Function00566E20, 2, 1, 0, 28, 0 },

    { 0x00000001, 109, 125, 123, 173, Function005670C0, 2, 1, 0, 24, 0 },
    { 0x00000002, 608, 112, 8, 0, Function005667A0, 3, 1, 0, 25, 0 },
    { 0x00000002, 544, 112, 8, 0, Function00566AE0, 3, 1, 0, 26, 0 },

    { 0x00000001, 535, 103, 616, 174, Function00565990, 3, 0, 0, -1, 0 },
    { 0x00000001, 539, 176, 612, 186, Function0058F240, 3, 0, 0, -1, 0 },
    { 0x00000001, 621, 125, 635, 173, Function00566E20, 3, 1, 0, 28, 0 },

    { 0x00000001, 517, 125, 531, 173, Function005670C0, 3, 1, 0, 24, 0 },
    { 0x00000002, 32, 198, 8, 0, Function005667A0, 4, 1, 0, 25, 0 },
    { 0x00000002, 95, 198, 8, 0, Function00566AE0, 4, 1, 0, 26, 0 },

    { 0x00000001, 23, 189, 104, 255, Function00565990, 4, 0, 0, -1, 0 },
    { 0x00000001, 21, 261, 105, 271, Function0058F240, 4, 0, 0, -1, 0 },
    { 0x00000001, 4, 210, 19, 258, Function00566E20, 4, 1, 0, 28, 0 },

    { 0x00000001, 109, 210, 123, 258, Function005670C0, 4, 1, 0, 24, 0 },
    { 0x00000002, 608, 198, 8, 0, Function005667A0, 5, 1, 0, 25, 0 },
    { 0x00000002, 544, 198, 8, 0, Function00566AE0, 5, 1, 0, 26, 0 },

    { 0x00000001, 535, 189, 616, 255, Function00565990, 5, 0, 0, -1, 0 },
    { 0x00000001, 539, 261, 612, 271, Function0058F240, 5, 0, 0, -1, 0 },
    { 0x00000001, 621, 210, 635, 258, Function00566E20, 5, 1, 0, 28, 0 },

    { 0x00000001, 517, 210, 531, 258, Function005670C0, 5, 1, 0, 24, 0 },
    { 0x00000002, 32, 283, 8, 0, Function005667A0, 6, 1, 0, 25, 0 },
    { 0x00000002, 95, 283, 8, 0, Function00566AE0, 6, 1, 0, 26, 0 },

    { 0x00000001, 23, 274, 104, 344, Function00565990, 6, 0, 0, -1, 0 },
    { 0x00000001, 21, 346, 105, 356, Function0058F240, 6, 0, 0, -1, 0 },
    { 0x00000001, 4, 295, 19, 343, Function00566E20, 6, 1, 0, 28, 0 },

    { 0x00000001, 109, 295, 123, 343, Function005670C0, 6, 1, 0, 24, 0 },
    { 0x00000002, 608, 283, 8, 0, Function005667A0, 7, 1, 0, 25, 0 },
    { 0x00000002, 544, 283, 8, 0, Function00566AE0, 7, 1, 0, 26, 0 },

    { 0x00000001, 535, 274, 616, 344, Function00565990, 7, 0, 0, -1, 0 },
    { 0x00000001, 539, 346, 612, 356, Function0058F240, 7, 0, 0, -1, 0 },
    { 0x00000001, 621, 295, 635, 343, Function00566E20, 7, 1, 0, 28, 0 },

    { 0x00000001, 517, 295, 531, 343, Function005670C0, 7, 1, 0, 24, 0 },
    { 0x00000001, 457, 363, 472, 378, Function0058E2A0, 0, 0, 0, -1, 0 },
    { 0x00000001, 457, 429, 472, 444, Function0058E650, 0, 0, 0, -1, 0 },

    { 0x00000001, 461, 379, 468, 428, Function0058E9F0, 0, 0, 0, -1, 0 },
    { 0x00000001, 166, 362, 456, 445, Function0058ED90, 0, 0, 0, -1, 0 },
    { 0x00000001, 140, 362, 163, 377, Function0058EFD0, 0, 1, 0, 33, 0 },

    { 0x00000001, 140, 383, 163, 398, Function0058EFD0, 1, 1, 0, 34, 0 },
    { 0x00000001, 140, 404, 163, 419, Function0058EFD0, 3, 1, 0, 35, 0 },
    { 0x00000001, 128, 358, 511, 450, Function0058F240, 0, 0, 0, -1, 0 },

    { 0x00000001, 0, 18, 21, 102, Function00565990, 0, 1, 0, -1, 0 },
    { 0x00000001, 617, 18, 639, 102, Function00565990, 1, 1, 0, -1, 0 },
    { 0x00000001, 0, 103, 21, 187, Function00565990, 2, 1, 0, -1, 0 },

    { 0x00000001, 617, 103, 639, 187, Function00565990, 3, 1, 0, -1, 0 },
    { 0x00000001, 0, 188, 21, 272, Function00565990, 4, 1, 0, -1, 0 },
    { 0x00000001, 617, 188, 639, 272, Function00565990, 5, 1, 0, -1, 0 },

    { 0x00000001, 0, 273, 21, 357, Function00565990, 6, 1, 0, -1, 0 },
    { 0x00000001, 617, 273, 639, 357, Function00565990, 7, 1, 0, -1, 0 },
    { 0x00000002, 75, 402, 46, 0, Function00567600, 0, 1, 0, 31, 0 },

    { 0x00000002, 564, 402, 40, 0, Function005B2020, 0, 1, 0, 32, 0 },
    { 0x00000001, 512, 358, 617, 450, Function0058F240, 0, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function0056F020, 9, 0, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function0056F020, 10, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function0056F020, 11, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function0056F020, 12, 0, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function0056F020, 13, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function0056F020, 14, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function0056F020, 15, 0, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function0056F020, 16, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function0056F020, 17, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function0056F020, 18, 0, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function0056F020, 19, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function0056F020, 20, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function0056F020, 21, 0, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function0056F020, 22, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function0056F020, 23, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function0056F020, 24, 0, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function0056F020, 25, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function0056F020, 26, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function0056F020, 27, 0, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function0056F020, 28, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function0056F020, 29, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function0056F020, 30, 0, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function0056F020, 31, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function0056F020, 32, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function0056F020, 33, 0, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function0056F020, 34, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function0056F020, 35, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function0056F020, 36, 0, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function0056F020, 37, 0, 0, 0, 0 },
    { 0x00000001, 0, 0, 0, 0, Function0056F020, 1, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function0056F020, 2, 0, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function0056F020, 3, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function0056F020, 4, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function0056F020, 5, 0, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function0056F020, 6, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function0056F020, 7, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function0056F020, 8, 0, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function0056F020, 39, 0, 0, 0, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005A0C80, 0, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005A0C80, 1, 1, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function005A0C80, 2, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005A0C80, 3, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005A0C80, 4, 1, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function005A0C80, 5, 1, 0, -1, 0 },
    { 0x00000002, 0, 0, 0, 0, Function005A0E50, 0, 1, 0, 46, 0 },
    { 0x00000002, 0, 0, 0, 0, Function005A0E50, 1, 1, 0, 47, 0 },

    { 0x00000002, 0, 0, 0, 0, Function005A0E50, 2, 1, 0, 48, 0 },
    { 0x00000002, 0, 0, 0, 0, Function005A0E50, 3, 1, 0, 49, 0 },
    { 0x00000002, 0, 0, 0, 0, Function005A0E50, 4, 1, 0, 50, 0 },

    { 0x00000002, 0, 0, 0, 0, Function005A0E50, 5, 1, 0, 51, 0 },
    { 0x00000002, 0, 0, 0, 0, Function005A0E50, 6, 1, 0, 52, 0 },
    { 0x00000002, 0, 0, 0, 0, Function005A0E50, 7, 1, 0, 53, 0 },

    { 0x00000002, 0, 0, 0, 0, Function005A0E50, 8, 1, 0, 54, 0 },
    { 0x00000001, 0, 0, 0, 0, reinterpret_cast<W8RegionCallback>(Function5A1140), /* reinterpret-ok: the region catalog stores the raw callback ABI */ 0, 1, 0, 55, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005A0E50, 10, 1, 0, 17, 0 },

    { 0x00000001, 0, 0, 0, 0, Function0059D970, 0, 1, 0, 96, 0 },
    { 0x00000001, 0, 0, 0, 0, Function0059D970, 1, 1, 0, 97, 0 },
    { 0x00000001, 0, 0, 0, 0, Function0059DA30, 0, 1, 0, 98, 0 },

    { 0x00000001, 0, 0, 0, 0, Function0059DA30, 3, 1, 0, 99, 0 },
    { 0x00000001, 0, 0, 0, 0, Function0059DA30, 8, 1, 0, 17, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005B29D0, 0, 0, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function005B29D0, 1, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005B29D0, 2, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005B29D0, 3, 0, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function005B29D0, 4, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005B29D0, 5, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005B29D0, 6, 0, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function005B29D0, 7, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005B29D0, 8, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005B29D0, 9, 0, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function005B29D0, 10, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005B29D0, 11, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005B29D0, 12, 0, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function005B29D0, 13, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005B29D0, 14, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005B2CB0, 0, 1, 0, 1981, 0 },

    { 0x00000001, 0, 0, 0, 0, Function005B2CB0, 1, 1, 0, 1982, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005B2CB0, 2, 1, 0, 1983, 0 },
    { 0x00000001, 214, 60, 427, 303, Function005B2D70, 0, 0, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function00594760, 0, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function00594760, 1, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function00594760, 2, 1, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function00594760, 3, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function00594760, 4, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function00594760, 5, 1, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function00594760, 6, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function00594760, 7, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function00594760, 8, 1, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function00594760, 9, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function00594760, 10, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function00594760, 11, 1, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function00594760, 12, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005949A0, 0, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function00598DB0, 0, 1, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function00598DB0, 1, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function00598DB0, 2, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function00598DB0, 3, 1, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function00598DB0, 4, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function00598CD0, 0, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005A1DE0, 0, 1, 0, 37, 0 },

    { 0x00000001, 0, 0, 0, 0, Function005A1DE0, 1, 1, 0, 38, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005AEEA0, 11, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005AEEA0, 10, 1, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function005AEEA0, 9, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005AEEA0, 8, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005AEEA0, 7, 1, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function005AEEA0, 6, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005AEEA0, 5, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005AEEA0, 4, 1, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function005AEEA0, 3, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005AEEA0, 2, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005AEEA0, 1, 1, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function005AEEA0, 0, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005AF530, 0, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005AF530, 1, 1, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function005AF530, 2, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005AF530, 3, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005AF530, 4, 1, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function005AF530, 5, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005AF530, 6, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005AF530, 7, 1, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function005AF530, 8, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005AF5E0, 5, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005AF5E0, 4, 1, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function005AF5E0, 3, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005AF5E0, 2, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005AF5E0, 1, 1, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function005AF5E0, 0, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function00568100, 0, 1, 0, 36, 0 },
    { 0x00000001, 0, 0, 0, 0, Function00567800, 0, 0, 0, 23, 0 },

    { 0x00000001, 0, 0, 0, 0, Function005B66B0, 0, 1, 0, 1984, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005B6AA0, 0, 1, 0, 2387, 0 },
    { 0x00000001, 164, 12, 254, 84, Function005B5E90, 0, 1, 0, 2368, 0 },

    { 0x00000001, 0, 0, 0, 0, Function005B5F10, 0, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005B5F10, 1, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005B5F10, 2, 0, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function005B5F10, 3, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005B5F10, 4, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005B5F10, 5, 0, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function005B5F10, 6, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005B5F10, 7, 0, 0, -1, 0 },
    { 0x00000001, 106, 96, 302, 109, Function005B61A0, 0, 1, 0, 2362, 0 },

    { 0x00000001, 106, 124, 302, 133, Function005B6220, 0, 1, 0, 2363, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005BB350, 0, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005BB350, 1, 1, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function005BB350, 2, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005BB350, 3, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005BB350, 4, 1, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function005BB350, 5, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005BB350, 6, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005BB350, 7, 1, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function005BB560, 0, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005BB560, 1, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005BB560, 2, 1, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function005BB560, 3, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005BB560, 4, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005BB560, 5, 1, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function005BB560, 6, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005BB560, 7, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005BB560, 8, 1, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function005BB560, 9, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005BB560, 10, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005BB560, 11, 1, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function005BB900, 0, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005BB900, 1, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005BB900, 2, 1, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function005BB900, 3, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005BB900, 4, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005BB900, 5, 1, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function005BB900, 6, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005BB900, 7, 1, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005BBBB0, 0, 1, 0, 2378, 0 },

    { 0x00000001, 0, 0, 0, 0, Function005BBBB0, 1, 1, 0, 2379, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005BBBB0, 2, 1, 0, 2380, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005BBBB0, 3, 1, 0, 2381, 0 },

    { 0x00000001, 0, 0, 0, 0, Function005BBBB0, 4, 1, 0, 2382, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005BBBB0, 5, 1, 0, 2383, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005BBBB0, 6, 1, 0, 2384, 0 },

    { 0x00000001, 0, 0, 0, 0, Function005BBC70, 0, 1, 0, 2385, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005BBC70, 1, 1, 0, 2386, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005B79F0, 0, 0, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function005B79F0, 1, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005B79F0, 2, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005B79F0, 3, 0, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function005B79F0, 4, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005B79F0, 5, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005B62C0, 0, 1, 0, 2364, 0 },

    { 0x00000001, 0, 0, 0, 0, Function005B62C0, 1, 1, 0, 2367, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005B62C0, 2, 1, 0, 2366, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005B62C0, 3, 1, 0, 2365, 0 },

    { 0x00000001, 0, 0, 0, 0, Function005B62C0, 4, 1, 0, 2368, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005B6360, 0, 1, 0, 2369, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005B6360, 1, 1, 0, 2370, 0 },

    { 0x00000001, 0, 0, 0, 0, Function005B6360, 2, 1, 0, 2371, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005B6360, 3, 1, 0, 2372, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005B6360, 4, 1, 0, 2373, 0 },

    { 0x00000001, 0, 0, 0, 0, Function005B6360, 5, 1, 0, 2374, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005B6360, 6, 1, 0, 2375, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005B6360, 7, 1, 0, 2376, 0 },

    { 0x00000001, 0, 0, 639, 479, Function00581790, 0, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 639, 479, IntroScreenRegionEvent, 0, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function0052FD80, 0, 0, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function0052FD80, 1, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function0052FD80, 2, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function0052FD80, 3, 0, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function0052FD80, 4, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function0052FD80, 5, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function0052FD80, 6, 0, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, Function0052FD80, 7, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function00576650, 0, 0, 0, -1, 0 },
    { 0x00000001, 0, 0, 0, 0, Function005699D0, 0, 0, 0, -1, 0 },

    { 0x00000001, 0, 0, 0, 0, reinterpret_cast<W8RegionCallback>(ScreenLifecycleSuccess), /* reinterpret-ok: the region catalog stores the raw callback ABI */ 0, 0, 0, -1, 0 },
};
unsigned int g_current_region_index;
wchar_t* g_default_help_text;
unsigned int g_captured_region_index;
unsigned int g_hover_region_index;
unsigned int g_region_help_force_enabled;

// GLOBAL: WIZ8 0x00689B32
unsigned char g_flag_689b32;


// FUNCTION: WIZ8 0x004f27a0
void SetRegionHelpDelay(int delay_ms)
{
    if (delay_ms == 0) {
        delay_ms = g_settings_6850c8.tooltip_delay_ms;
    }
    g_region_help_delay = delay_ms;
}

// FUNCTION: WIZ8 0x004f1220
void ReleasePointer689B40(void)
{
    if (g_default_help_text != 0) {
        delete[] g_default_help_text;
    }
}

/* Dispatch one mouse-position event through the enabled region sets. A forced
   modal region bypasses hit testing; otherwise the first containing region
   receives leave/enter transitions, hover help timing, and the ordinary
   position callback as one transaction. */
// FUNCTION: WIZ8 0x004f1360
unsigned int UpdateRegionMousePosition(int x, int y)
{
    W8RegionMouseEvent event;
    unsigned int set_index;
    unsigned int region_index;

    event.event.time = GetClock();
    event.event.modifiers = gfAltState | gfCtrlState | gfShiftState;
    event.event.reason = MOUSE_POS;
    event.mouse_position =
        (static_cast<unsigned int>(static_cast<unsigned short>(y)) << 16) |
        static_cast<unsigned short>(x);

    if (g_captured_region_index != 0) {
        W8Region* forced = &g_regions[g_captured_region_index];
        forced->callback(&event.event, forced);
        return g_current_region_index;
    }

    for (set_index = 0; set_index < g_region_set_count; ++set_index) {
        W8RegionSet* set = &g_region_sets[set_index];
        if (set->enabled != 1 || set->first_region > set->last_region) {
            continue;
        }
        for (region_index = set->first_region;
             region_index <= set->last_region; ++region_index) {
            if (!RegionContainsPoint(region_index,
                                     static_cast<unsigned short>(x),
                                     static_cast<unsigned short>(y))) {
                continue;
            }

            W8Region* region = &g_regions[region_index];
            unsigned int previous_index = g_hover_region_index;
            g_current_region_index = region_index;
            if (previous_index != 0 && previous_index != region_index) {
                W8Region* previous = &g_regions[previous_index];
                previous->flags = (previous->flags & 0xff0f) | W8_REGION_MOUSE_LEAVE;
                previous->callback(&event.event, previous);
                if ((previous->flags & W8_REGION_HELP_SHOWN) != 0) {
                    VideoRemoveToolTip();
                    previous->flags &= ~W8_REGION_HELP_SHOWN;
                }
                PlayButtonSound(1);
                g_region_help_delay = (unsigned short)g_settings_6850c8.tooltip_delay_ms;
                previous->flags &= ~W8_REGION_MOUSE_STATE_MASK;
                g_region_help_force_enabled = 0;
            }
            if (previous_index != region_index) {
                region->flags |= W8_REGION_MOUSE_ENTER;
                SetRegionHelpText(
                    FormatWideString(L"Region %d", region_index));
            }
            region->callback(&event.event, region);
            if (g_current_region_index != previous_index) {
                if (region->help_enabled != 0 &&
                    (g_settings_6850c8.tooltips_enabled != 0 ||
                     g_region_help_force_enabled != 0)) {
                    g_region_help_clock =
                        SetCountdownClock(g_region_help_delay);
                }
                PlayButtonSound(0);
            }
            region->flags &= ~W8_REGION_MOUSE_TRANSITION_MASK;
            g_hover_region_index = g_current_region_index;
            return g_current_region_index;
        }
    }

    g_current_region_index = 0;
    if (g_hover_region_index != 0) {
        unsigned int previous_index = g_hover_region_index;
        W8Region* previous = &g_regions[previous_index];
        previous->flags = (previous->flags & 0xff0f) | W8_REGION_MOUSE_LEAVE;
        previous->callback(&event.event, previous);
        if ((previous->flags & W8_REGION_HELP_SHOWN) != 0) {
            VideoRemoveToolTip();
            previous->flags &= ~W8_REGION_HELP_SHOWN;
        }
        PlayButtonSound(1);
        g_region_help_delay = (unsigned short)g_settings_6850c8.tooltip_delay_ms;
        g_region_help_force_enabled = 0;
        previous->flags &= ~W8_REGION_MOUSE_STATE_MASK;
    }
    g_hover_region_index = g_current_region_index;
    return g_current_region_index;
}

/* Find the first enabled region containing the mouse position.  Moving to a
   different region also sends the old region its leave transition and drops
   any help box it still owns. */
// FUNCTION: WIZ8 0x004f16f0
unsigned int FindRegionAtPoint(unsigned short x, unsigned short y)
{
    W8RegionMouseEvent event;
    unsigned int set_index;
    unsigned int region_index;

    event.event.time = GetClock();
    event.event.modifiers = gfAltState | gfCtrlState | gfShiftState;
    event.event.reason = MOUSE_POS;
    event.mouse_position =
        (static_cast<unsigned int>(y) << 16) | x;

    if (g_captured_region_index != 0) {
        return g_captured_region_index;
    }

    for (set_index = 0; set_index < g_region_set_count; ++set_index) {
        W8RegionSet* set = &g_region_sets[set_index];
        if (set->enabled != 1 || set->first_region > set->last_region) {
            continue;
        }
        for (region_index = set->first_region;
             region_index <= set->last_region; ++region_index) {
            if (!RegionContainsPoint(region_index, x, y)) {
                continue;
            }
            if (g_hover_region_index != 0 &&
                g_hover_region_index != region_index) {
                W8Region* previous = &g_regions[g_hover_region_index];
                previous->flags = (previous->flags & 0xff0f) | W8_REGION_MOUSE_LEAVE;
                previous->callback(&event.event, previous);
                if ((previous->flags & W8_REGION_HELP_SHOWN) != 0) {
                    VideoRemoveToolTip();
                    previous->flags &= ~W8_REGION_HELP_SHOWN;
                }
                g_region_help_delay = (unsigned short)g_settings_6850c8.tooltip_delay_ms;
                previous->flags &= ~W8_REGION_MOUSE_STATE_MASK;
                g_region_help_force_enabled = 0;
                g_hover_region_index = 0;
                g_current_region_index = 0;
            }
            return region_index;
        }
    }

    if (g_hover_region_index != 0 &&
        (g_regions[g_hover_region_index].flags & W8_REGION_HELP_SHOWN) != 0) {
        unsigned int previous_index = g_hover_region_index;
        VideoRemoveToolTip();
        g_regions[previous_index].flags &= ~W8_REGION_HELP_SHOWN;
    }
    return 0;
}

/* Route one queued input atom to the forced region, the current hot region,
   or the first enabled region under the event's mouse position. */
// FUNCTION: WIZ8 0x004f1910
unsigned char DispatchRegionInput(const InputAtom* event)
{
    unsigned int region_index = g_captured_region_index;
    unsigned int set_index;
    int sound_id = -1;
    unsigned short x = static_cast<unsigned short>(event->uiParam) +
                       g_cursor_hotspot_x_6596bc;
    unsigned short y = static_cast<unsigned short>(event->uiParam >> 16) +
                       g_cursor_hotspot_y_6596c0;

    if (region_index != 0) {
        goto dispatch;
    }

    region_index = g_current_region_index;
    if (region_index != 0 && RegionContainsPoint(region_index, x, y)) {
        goto dispatch;
    }

    for (set_index = 0; set_index < g_region_set_count; ++set_index) {
        W8RegionSet* set = &g_region_sets[set_index];
        if (set->enabled != 1 || set->first_region > set->last_region) {
            continue;
        }
        for (region_index = set->first_region;
             region_index <= set->last_region; ++region_index) {
            if (RegionContainsPoint(region_index, x, y)) {
                goto dispatch;
            }
        }
    }
    return 0;

dispatch:
    W8Region* region = &g_regions[region_index];
    if (region->help_enabled != 0 &&
        (g_settings_6850c8.tooltips_enabled != 0 ||
         g_region_help_force_enabled != 0) &&
        event->usEvent != MOUSE_POS) {
        if ((region->flags & W8_REGION_HELP_SHOWN) != 0) {
            VideoRemoveToolTip();
            region->flags &= ~W8_REGION_HELP_SHOWN;
        }
        if (region->help_enabled != 0 &&
            (g_settings_6850c8.tooltips_enabled != 0 ||
             g_region_help_force_enabled != 0)) {
            g_region_help_clock = SetCountdownClock(g_region_help_delay);
        }
    }

    switch (event->usEvent) {
    case LEFT_BUTTON_DOWN:
    case RIGHT_BUTTON_DOWN:
        sound_id = 2;
        break;
    case LEFT_BUTTON_UP:
        if ((g_regions[g_current_region_index].flags & W8_REGION_LEFT_BUTTON_HELD) != 0) {
            sound_id = 3;
        }
        break;
    case RIGHT_BUTTON_UP:
        if ((g_regions[g_current_region_index].flags & W8_REGION_RIGHT_BUTTON_HELD) != 0) {
            sound_id = 3;
        }
        break;
    }

    unsigned char handled = region->callback(
        reinterpret_cast<const W8RegionEvent*>(event), region);
    if (sound_id != -1) {
        PlayButtonSound(sound_id);
    }
    return handled;
}

/* Raises the help box for one region, taking a stale one down first. The
   selected text comes either from the region's indexed notice entry or the
   shared fallback, and the final position is clamped inside the 640x480
   screen before the region records ownership of the box. */
// FUNCTION: WIZ8 0x004f2650
void ShowRegionHelp(unsigned int region_index)
{
    W8Region* region;
    unsigned int mode;
    wchar_t* text;
    POINT anchor;
    int width;
    int height;

    if (g_settings_6850c8.tooltips_enabled == 0 && g_region_help_force_enabled == 0) {
        return;
    }
    region = &g_regions[region_index];
    mode = region->flags & W8_REGION_MODE_MASK;
    if (mode != 1 && mode != 2 && (region->flags & W8_REGION_HELP_SHOWN) != 0) {
        VideoRemoveToolTip();
        region->flags &= ~W8_REGION_HELP_SHOWN;
    }
    if ((region->flags & W8_REGION_HELP_SHOWN) != 0) {
        return;
    }
    if (region->help_text_id == -1) {
        text = g_default_help_text;
        if (text == 0) {
            return;
        }
    } else {
        text = gppStringList[region->help_text_id];
    }
    VideoToolTip(text);
    width = g_help_box_width + W8_HELP_MARGIN;
    height = g_help_box_height + W8_HELP_MARGIN;
    SGPMouseGetPos(&anchor);
    anchor.y -= height;
    if (anchor.x < 0) {
        anchor.x = W8_HELP_MARGIN;
    }
    if (anchor.x + width > W8_SCREEN_WIDTH - 1) {
        anchor.x = W8_SCREEN_WIDTH - width;
    }
    if (anchor.y < 0) {
        anchor.y = W8_HELP_MARGIN;
    }
    if (anchor.y + height > W8_SCREEN_HEIGHT - 1) {
        anchor.y = W8_SCREEN_HEIGHT - height;
    }
    VideoPositionToolTip(anchor.x, anchor.y);
    region->flags |= W8_REGION_HELP_SHOWN;
}

/* Force one region to the front of modal dispatch. If another region owned
   that role, send its ordinary mouse-leave callback first and release any
   transition object it still owns. */
// FUNCTION: WIZ8 0x004f2040
void ActivateDialogRegion(unsigned int region_index)
{
    if (region_index >= g_region_count) {
        srAssertFail(
            "uiRegionIndex < guiRegionCount",
            "C:\\Projects\\Wizardry 8\\Local Code\\RegionManager.cpp",
            0x1d6,
            0);
    }

    g_captured_region_index = region_index;
    g_current_region_index = region_index;
    if (g_hover_region_index == region_index) {
        return;
    }

    if (g_hover_region_index != 0) {
        W8RegionEvent event;
        event.time = GetClock();
        event.modifiers = gfAltState | gfCtrlState | gfShiftState;
        event.reason = MOUSE_POS;

        W8Region* previous = &g_regions[g_hover_region_index];
        previous->flags = (previous->flags & 0xff0f) | W8_REGION_MOUSE_LEAVE;
        previous->callback(&event, previous);
        unsigned int previous_index = g_hover_region_index;
        if ((g_regions[previous_index].flags & W8_REGION_HELP_SHOWN) != 0) {
            VideoRemoveToolTip();
            g_regions[previous_index].flags &= ~W8_REGION_HELP_SHOWN;
        }
        g_region_help_delay = (unsigned short)g_settings_6850c8.tooltip_delay_ms;
        g_regions[g_hover_region_index].flags &= ~W8_REGION_MOUSE_STATE_MASK;
        g_region_help_force_enabled = 0;
        g_hover_region_index = 0;
    }
    g_regions[g_captured_region_index].flags &= ~W8_REGION_MOUSE_STATE_MASK;
}

// FUNCTION: WIZ8 0x004f21b0
unsigned char ClearActiveRegionIfMatches(unsigned int region_index)
{
    if (g_captured_region_index == region_index) {
        g_captured_region_index = 0;
        g_current_region_index = 0;
        return 1;
    }
    return 0;
}

// FUNCTION: WIZ8 0x004f21d0
unsigned int GetForcedRegion(void)
{
    return g_captured_region_index;
}

// FUNCTION: WIZ8 0x004f21e0
void RegionSetEnable(unsigned int region_set_index)
{
    if (region_set_index >= g_region_set_count) {
        srAssertFail(
            "uiRegionSetIndex < guiRegsetCount",
            "C:\\Projects\\Wizardry 8\\Local Code\\RegionManager.cpp",
            0x22b,
            0);
    }
    g_region_sets[region_set_index].enabled = 1;
}

// FUNCTION: WIZ8 0x004f2220
void RegionSetDisable(unsigned int region_set_index)
{
    if (region_set_index >= g_region_set_count) {
        srAssertFail(
            "uiRegionSetIndex < guiRegsetCount",
            "C:\\Projects\\Wizardry 8\\Local Code\\RegionManager.cpp",
            0x234,
            0);
    }
    g_region_sets[region_set_index].enabled = 0;
}

// FUNCTION: WIZ8 0x004f2260
void EnableRegionSetInput(unsigned int region_set_index)
{
    unsigned int region_index;
    W8Region* region;

    if (region_set_index >= g_region_set_count) {
        srAssertFail(
            "uiRegionSetIndex < guiRegsetCount",
            "C:\\Projects\\Wizardry 8\\Local Code\\RegionManager.cpp",
            0x23f,
            0);
    }
    region_index = g_region_sets[region_set_index].first_region;
    if (region_index <= g_region_sets[region_set_index].last_region) {
        region = &g_regions[region_index];
        do {
            if (region_index >= g_region_count) {
                srAssertFail(
                    "uiRegionIndex < guiRegionCount",
                    "C:\\Projects\\Wizardry 8\\Local Code\\RegionManager.cpp",
                    0x259,
                    0);
            }
            region->flags &= ~W8_REGION_INPUT_MODE_MASK;
            ++region_index;
            ++region;
        } while (region_index <= g_region_sets[region_set_index].last_region);
    }
}

// FUNCTION: WIZ8 0x004f22f0
void DisableRegionSetInput(unsigned int region_set_index)
{
    unsigned int region_index;
    unsigned int last_region;

    if (region_set_index >= g_region_set_count) {
        srAssertFail(
            "uiRegionSetIndex < guiRegsetCount",
            "C:\\Projects\\Wizardry 8\\Local Code\\RegionManager.cpp",
            0x24d,
            0);
    }
    region_index = g_region_sets[region_set_index].first_region;
    if (region_index <= g_region_sets[region_set_index].last_region) {
        do {
            if (region_index >= g_region_count) {
                srAssertFail(
                    "uiRegionIndex < guiRegionCount",
                    "C:\\Projects\\Wizardry 8\\Local Code\\RegionManager.cpp",
                    0x262,
                    0);
            }
            last_region = g_region_sets[region_set_index].last_region;
            g_regions[region_index].flags =
                (g_regions[region_index].flags & ~W8_REGION_INPUT_MODE_MASK) |
                W8_REGION_INPUT_DISABLED;
            ++region_index;
        } while (region_index <= last_region);
    }
}

// FUNCTION: WIZ8 0x004f2380
void EnableRegionInput(unsigned int region_index)
{
    if (region_index >= g_region_count) {
        srAssertFail(
            "uiRegionIndex < guiRegionCount",
            "C:\\Projects\\Wizardry 8\\Local Code\\RegionManager.cpp",
            0x259,
            0);
    }
    g_regions[region_index].flags &= ~W8_REGION_INPUT_MODE_MASK;
}

// FUNCTION: WIZ8 0x004f23d0
void DisableRegionInput(unsigned int region_index)
{
    if (region_index >= g_region_count) {
        srAssertFail(
            "uiRegionIndex < guiRegionCount",
            "C:\\Projects\\Wizardry 8\\Local Code\\RegionManager.cpp",
            0x262,
            0);
    }
    unsigned int flags = g_regions[region_index].flags;
    flags &= ~W8_REGION_INPUT_MODE_MASK;
    flags |= W8_REGION_INPUT_DISABLED;
    g_regions[region_index].flags = flags;
}

// FUNCTION: WIZ8 0x004f2420
void SetRegionBounds(unsigned int region_index, unsigned short x1, unsigned short y1,
                     unsigned short x2, unsigned short y2)
{
    if (region_index >= g_region_count) {
        srAssertFail(
            "uiRegionIndex < guiRegionCount",
            "C:\\Projects\\Wizardry 8\\Local Code\\RegionManager.cpp",
            0x27d,
            0);
    }
    g_regions[region_index].x1 = x1;
    g_regions[region_index].y1 = y1;
    g_regions[region_index].x2 = x2;
    g_regions[region_index].y2 = y2;
}

// FUNCTION: WIZ8 0x004f2490
bool RegionContainsPoint(unsigned int region_index, unsigned short x, unsigned short y)
{
    W8Region* region;

    if (region_index >= g_region_count) {
        srAssertFail(
            "uiRegionIndex < guiRegionCount",
            "C:\\Projects\\Wizardry 8\\Local Code\\RegionManager.cpp",
            0x2a4,
            0);
    }
    region = &g_regions[region_index];
    switch (region->flags & W8_REGION_MODE_MASK) {
    case W8_REGION_RECTANGLE:
        if (x >= region->x1 && x <= region->x2 &&
            y >= region->y1 && y <= region->y2) {
            return true;
        }
        break;
    case W8_REGION_CIRCLE: {
        short delta_x = x - region->x1;
        short delta_y = y - region->y1;
        if (delta_x * delta_x + delta_y * delta_y <= region->x2 * region->x2) {
            return true;
        }
        break;
    }
    }
    return false;
}

// FUNCTION: WIZ8 0x004f2550
bool RegionHasFlags(unsigned int region_index, unsigned int flags)
{
    bool has_flags;

    if (region_index >= g_region_count) {
        srAssertFail(
            "uiRegionIndex < guiRegionCount",
            "C:\\Projects\\Wizardry 8\\Local Code\\RegionManager.cpp",
            0x2c6,
            0);
    }
    has_flags = (g_regions[region_index].flags & flags) != 0;
    return has_flags;
}

// FUNCTION: WIZ8 0x004f25a0
void UpdateRegionHelp(void)
{
    if (g_captured_region_index == 0) {
        if (g_current_region_index != 0 &&
            g_regions[g_current_region_index].help_enabled != 0 &&
            (g_settings_6850c8.tooltips_enabled != 0 || g_region_help_force_enabled != 0) &&
            ClockIsTicking(g_region_help_clock) == 0) {
            ShowRegionHelp(g_current_region_index);
        }
    } else if (g_regions[g_captured_region_index].help_enabled != 0 &&
               (g_settings_6850c8.tooltips_enabled != 0 || g_region_help_force_enabled != 0) &&
               ClockIsTicking(g_region_help_clock) == 0) {
        ShowRegionHelp(g_captured_region_index);
    }
}


// FUNCTION: WIZ8 0x004f2750
void SetRegionHelpText(const wchar_t* text)
{
    if (g_default_help_text != 0) {
        delete[] g_default_help_text;
    }
    if (text != 0) {
        g_default_help_text = new wchar_t[wcslen(text) + 1];
        wcscpy(g_default_help_text, text);
    } else {
        g_default_help_text = 0;
    }
}

// FUNCTION: WIZ8 0x004f27f0
void ResetRegionHelp(unsigned char delayed)
{
    unsigned int region_index = g_current_region_index;

    VideoRemoveToolTip();
    g_regions[region_index].flags &= 0xfffffdff;
    if (delayed == 0) {
        ShowRegionHelp(g_current_region_index);
    } else if (g_regions[g_current_region_index].help_enabled != 0 &&
               (g_settings_6850c8.tooltips_enabled != 0 || g_region_help_force_enabled != 0)) {
        g_region_help_clock = SetCountdownClock(g_region_help_delay);
    }
}


// FUNCTION: WIZ8 0x004f2880
unsigned int CreateRegionSet(void)
{
    unsigned int region_set_index = g_region_set_count++;

    if (g_region_set_count > 300) {
        srAssertFail(
            "guiRegsetCount <= REGSET_LIMIT",
            "C:\\Projects\\Wizardry 8\\Local Code\\RegionManager.cpp",
            0x487,
            0);
    }
    g_region_sets[region_set_index].enabled = 0;
    g_region_sets[region_set_index].first_region = g_region_count;
    g_region_sets[region_set_index].last_region = 0;
    return region_set_index;
}

// FUNCTION: WIZ8 0x004f28e0
void ResetRegionSet(unsigned int region_set_index)
{
    if (region_set_index >= g_region_set_count) {
        srAssertFail(
            "uiRegionSet < guiRegsetCount",
            "C:\\Projects\\Wizardry 8\\Local Code\\RegionManager.cpp",
            0x4a0,
            0);
    }
    g_region_sets[region_set_index].last_region = 0;
}

// FUNCTION: WIZ8 0x004f2920
unsigned int AddRegionToSet(unsigned int region_set_index)
{
    unsigned int region_index;

    if (region_set_index >= g_region_set_count) {
        srAssertFail(
            "uiRegionSet < guiRegsetCount",
            "C:\\Projects\\Wizardry 8\\Local Code\\RegionManager.cpp",
            0x4b7,
            0);
    }
    if (g_region_sets[region_set_index].last_region == 0) {
        region_index = g_region_sets[region_set_index].first_region;
    } else {
        region_index = g_region_sets[region_set_index].last_region + 1;
    }
    g_region_sets[region_set_index].last_region = region_index;
    if (region_index == g_region_count) {
        ++g_region_count;
        if (g_region_count > 1500) {
            srAssertFail(
                "guiRegionCount <= REGION_LIMIT",
                "C:\\Projects\\Wizardry 8\\Local Code\\RegionManager.cpp",
                0x4c7,
                0);
        }
        g_regions[region_index] = g_regions[0];
    }
    return region_index;
}

// FUNCTION: WIZ8 0x004f29c0
void SetRegionCallback(unsigned int region_index, W8RegionCallback callback,
                       unsigned short callback_id)
{
    if (region_index > g_region_count) {
        srAssertFail(
            "uiRegionIndex <= guiRegionCount",
            "C:\\Projects\\Wizardry 8\\Local Code\\RegionManager.cpp",
            0x4df,
            0);
    }
    g_regions[region_index].callback = callback;
    g_regions[region_index].callback_id = callback_id;
}

// FUNCTION: WIZ8 0x004f2a10
void SetRegionOwner(unsigned int region_index, void* owner)
{
    g_regions[region_index].owner = owner;
}

// FUNCTION: WIZ8 0x004f2a30
void SetRegionHelp(unsigned int region_index, unsigned char enabled, int help_text_id)
{
    if (region_index > g_region_count) {
        srAssertFail(
            "uiRegionIndex <= guiRegionCount",
            "C:\\Projects\\Wizardry 8\\Local Code\\RegionManager.cpp",
            0x506,
            0);
    }
    g_regions[region_index].help_enabled = enabled;
    g_regions[region_index].help_text_id = help_text_id;
}

/* Send the current hot region a mouse-leave transition at the live cursor
   position, then relinquish its help and hover state. */
// FUNCTION: WIZ8 0x004f2a80
void ClearHotRegion004F2A80(void)
{
    POINT mouse;
    W8RegionMouseEvent event;

    SGPMouseGetPos(&mouse);
    event.event.time = GetClock();
    event.event.modifiers = gfAltState | gfCtrlState | gfShiftState;
    event.event.reason = MOUSE_POS;
    event.mouse_position =
        (static_cast<unsigned int>(mouse.y) << 16) |
        (static_cast<unsigned int>(mouse.x) & 0xffff);

    if (g_current_region_index != 0) {
        W8Region* region = &g_regions[g_current_region_index];
        unsigned int mode = region->flags & W8_REGION_MODE_MASK;
        if (mode == 1 || mode == 2) {
            region->flags = (region->flags & 0xff0f) | W8_REGION_MOUSE_LEAVE;
            region->callback(&event.event, region);
            unsigned int region_index = g_current_region_index;
            if ((g_regions[region_index].flags & W8_REGION_HELP_SHOWN) != 0) {
                VideoRemoveToolTip();
                g_regions[region_index].flags &= ~W8_REGION_HELP_SHOWN;
            }
            g_region_help_delay = (unsigned short)g_settings_6850c8.tooltip_delay_ms;
            g_region_help_force_enabled = 0;
            g_regions[g_current_region_index].flags &= ~W8_REGION_MOUSE_STATE_MASK;
            g_current_region_index = 0;
        }
    }
}

// FUNCTION: WIZ8 0x004f2bb0
void EnableRegionHelp(unsigned int region_index)
{
    if (region_index > g_region_count) {
        srAssertFail(
            "uiRegionIndex <= guiRegionCount",
            "C:\\Projects\\Wizardry 8\\Local Code\\RegionManager.cpp",
            0x558,
            0);
    }
    g_regions[region_index].help_enabled = 1;
}

// FUNCTION: WIZ8 0x004f2bf0
void DisableRegionHelp(unsigned int region_index)
{
    if (region_index > g_region_count) {
        srAssertFail(
            "uiRegionIndex <= guiRegionCount",
            "C:\\Projects\\Wizardry 8\\Local Code\\RegionManager.cpp",
            0x55f,
            0);
    }
    g_regions[region_index].help_enabled = 0;
}

/* Drops every region back to its resting state. The three tracked regions are
   released first - each only if it still carries bit 0x200 - then every region
   set is disabled and every region keeps only its low two flag bits. The three
   trackers are cleared last, and the fourth field is reseeded from the
   settings word rather than zeroed. */
// FUNCTION: WIZ8 0x004f1240
void ResetRegions(void)
{
    unsigned int index;
    W8RegionSet* set;
    W8Region* region;
    unsigned int remaining;

    index = g_current_region_index;
    if (g_current_region_index != 0 && (g_regions[g_current_region_index].flags & W8_REGION_HELP_SHOWN) != 0) {
        VideoRemoveToolTip();
        g_regions[index].flags &= ~W8_REGION_HELP_SHOWN;
    }
    index = g_hover_region_index;
    if (g_hover_region_index != 0 && (g_regions[g_hover_region_index].flags & W8_REGION_HELP_SHOWN) != 0) {
        VideoRemoveToolTip();
        g_regions[index].flags &= ~W8_REGION_HELP_SHOWN;
    }
    index = g_captured_region_index;
    if (g_captured_region_index != 0 && (g_regions[g_captured_region_index].flags & W8_REGION_HELP_SHOWN) != 0) {
        VideoRemoveToolTip();
        g_regions[index].flags &= ~W8_REGION_HELP_SHOWN;
    }
    if (g_region_set_count != 0) {
        set = g_region_sets;
        remaining = g_region_set_count;
        do {
            set->enabled = 0;
            set = set + 1;
            remaining = remaining - 1;
        } while (remaining != 0);
    }
    if (g_region_count != 0) {
        region = g_regions;
        remaining = g_region_count;
        do {
            region->flags &= 3;
            region = region + 1;
            remaining = remaining - 1;
        } while (remaining != 0);
    }
    g_current_region_index = 0;
    g_hover_region_index = 0;
    g_captured_region_index = 0;
    g_region_help_force_enabled = 0;
    g_region_help_delay = (unsigned short)g_settings_6850c8.tooltip_delay_ms;
}
