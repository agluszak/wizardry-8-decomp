#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"

#include <wchar.h>

/*
 * Local Screens\MainMenuScreen.cpp.
 *
 * The unit is named by the assertion this body embeds at line 135. Nothing here
 * is named beyond that: the callees and globals carry address-derived names
 * because no evidence assigns them meaning yet, and inventing one would be a
 * guess dressed as a recovery.
 */

extern "C" {

extern unsigned int g_monster_record_count;


/* The screen's own state. */
extern unsigned char g_flag_68517c;
extern unsigned char g_flag_69c4ba;
extern unsigned char g_flag_69c4b6;
extern unsigned char g_selected_item_69c4b4;
extern unsigned char g_flag_69c4c4;
extern int g_dword_69c4bc;
extern int g_dword_69c4c0;
extern int g_dword_647bc0;
extern int g_dword_68c09c;
extern void* g_font_683660;
extern void* g_colour_68ee3c;
extern void* g_colour_68ee08;

extern void Function422B10(void);
extern void Function40B290(void);
extern unsigned char Function512FB0(void);
extern void Function421FF0(void);
extern void* Function4104B0(int packed_colour);
extern void Function402FA0(int id, int x, int y, int width, int height, void* colour);
extern void SetViewport(int left, int top, int right, int bottom);
extern void Function548F90(int id, int a, int b, int c, int d, int e, int f, int g);
extern void Function422D50(int left, int top, int right, int bottom, int a);
extern void Function4E3620(char* text, int a, int b, int c);
extern void Function407210(void* font);
extern void Function406DC0(void* font, void* colour);
extern short Function407010(const wchar_t* text, void* font, int a, const wchar_t* measured);
extern void Function407650(int x);
extern void Function4F1240(void);
extern void Function48FC10(const char* playlist, int a);
extern void Function55EF90(void);
extern void* Function5CF300(int a);
extern void Function5D2CB0(int a, int b);
extern void Function5D2800(int a, int b, int c, int d, int e, int f, int g, int h, int i);
extern void Function5CF580(void* a, int b);
extern unsigned char Function4298F0(void);
extern wchar_t* ConvertStringToWide(const char* text);

/* Draws one of the six menu items. The first switch turns the item index into
   its sprite slot and its top and bottom rows; the second turns the requested
   state into a sprite id. Item two is forced to state three whenever the flag
   0x005BC810 stores from 0x00512FB0 is clear, which is the only item whose
   state the screen overrides.

   The sprite call is written in every case rather than assigning the id and
   calling once: the original pushes each id as a literal and lets VC6 cross-jump
   the four identical calls together. An unrecognised state draws no sprite but
   still redraws the row. */
// FUNCTION: WIZ8 0x005BCAB0
unsigned char Function5BCAB0(short item, short state)
{
    int slot;
    int top;
    int bottom;
    int sprite;

    switch (item) {
    case 0: slot = 0; top = 0x8a;  bottom = 0xb1;  break;
    case 1: slot = 1; top = 0xbb;  bottom = 0xe1;  break;
    case 2:
        slot = 2;
        top = 0xeb;
        bottom = 0x112;
        if (!g_flag_69c4ba) {
            state = 3;
        }
        break;
    case 3: slot = 3; top = 0x11c; bottom = 0x145; break;
    case 4: slot = 4; top = 0x14f; bottom = 0x193; break;
    case 5: slot = 5; top = 0x1a7; bottom = 0x1d3; break;
    default:
        return 0;
    }

    switch (state) {
    case 0: Function548F90(-14, 0xea, 0, slot, 0x98, top, 2, 0); break;
    case 1: Function548F90(-14, 0xec, 0, slot, 0x98, top, 2, 0); break;
    case 2: Function548F90(-14, 0xeb, 0, slot, 0x98, top, 2, 0); break;
    case 3: Function548F90(-14, 0xed, 0, slot, 0x98, top, 2, 0); break;
    }

    Function422D50(0x98, top, 0x1f2, bottom, 0);
    return 1;
}

// FUNCTION: WIZ8 0x005BC810
unsigned char MainMenuScreenFunction005BC810(void)
{
    char text[64];
    wchar_t wide[64];
    void* colour;
    void* dialog;
    int pending;
    short measured;

    Function422B10();
    Function40B290();
    g_flag_68517c = 0;
    g_flag_69c4ba = Function512FB0();
    g_flag_69c4b6 = 1;
    Function421FF0();
    colour = Function4104B0(0x10101);
    Function402FA0(-14, 0, 0, 0x280, 0x1e0, colour);
    SetViewport(0, 0, 0x280, 0x1e0);
    g_selected_item_69c4b4 = 0;
    Function548F90(-14, 0xe8, 0, 0, 0, 0, 2, 0);

    /* Six items cleared then the selected one set, written out rather than
       looped: the original repeats the call with a literal index each time. */
    Function5BCAB0(0, 0);
    Function5BCAB0(1, 0);
    Function5BCAB0(2, 0);
    Function5BCAB0(3, 0);
    Function5BCAB0(4, 0);
    Function5BCAB0(5, 0);
    Function5BCAB0(g_selected_item_69c4b4, 1);

    Function4E3620(text, 0, 0, 0);
    wcscpy(wide, ConvertStringToWide(text));
    Function407210(g_font_683660);
    Function406DC0(g_font_683660, g_colour_68ee3c);
    measured = Function407010(wide, g_font_683660, 5, wide);
    Function407650(0x27b - measured);
    Function406DC0(g_font_683660, g_colour_68ee08);
    Function4F1240();
    RegionSetEnable(1);

    if (g_monster_record_count > 1000) {
        srAssertFail(
            "gXStatus.uiMonstersInDatabase <= MAX_MONSTERS_IN_DATABASE",
            "C:\\Projects\\Wizardry 8\\Local Screens\\MainMenuScreen.cpp",
            0x87,
            0);
    }
    if (g_dword_647bc0 != 10) {
        Function48FC10("MainMenu.MPL", 0);
    }
    Function55EF90();

    pending = g_dword_69c4bc;
    if (pending != 0) {
        dialog = Function5CF300(1);
        Function5D2CB0(0xfa, 200);
        Function5D2800(pending, 1, 0x32, 1, 0, 1, 1, 0, 0x15e);
        Function5CF580(dialog, 0);
        g_dword_69c4c0 = (int)dialog;
        ::operator delete((void*)g_dword_69c4bc);
        g_dword_69c4bc = 0;
        return 1;
    }
    if (!Function4298F0() && !g_flag_69c4c4) {
        int message = *(int*)(g_dword_68c09c + 0x1fb8);

        dialog = Function5CF300(1);
        Function5D2CB0(0xfa, 200);
        Function5D2800(message, 1, 0x32, 1, 0, 1, 1, 0, 0x15e);
        Function5CF580(dialog, 0);
        g_flag_69c4c4 = 1;
        g_dword_69c4c0 = (int)dialog;
    }
    return 1;
}

}
