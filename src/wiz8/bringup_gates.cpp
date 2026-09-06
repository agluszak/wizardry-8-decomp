#include "wiz8/bringup_gates.h"
#include "wiz8/game_status.h"
#include "wiz8/wiz8_windows.h"

#include "wiz8/render_state.h"
#include "wiz8/sgp_vsurface_private.h"
#include "Button System.h"
#include "Font.h"
#include "FileMan.h"
#include "Mss.h"
#include "input.h"
#include "random.h"
#include "sgp.h"
#include "timer.h"
#include "vobject.h"
#include "vsurface.h"

#include <direct.h>
#include <process.h>
#include <stdlib.h>
#include <string.h>

/*
 * Gates called from BringUpEngine at 0x00401570, in the order the startup spine
 * records. Two of them the spine characterises from their imports and strings
 * and they are named accordingly; the rest it explicitly cannot, so they keep
 * address-derived names rather than invented meanings. Their globals are
 * likewise positional: the stores establish widths and initial values, nothing
 * establishes purpose.
 */

struct W8VideoManagerNode {
    unsigned int handle;
    int unused_04;
    int unused_08;
    W8VideoManagerNode* next;
};

W8VideoManagerNode* g_surface_list_650dbc;
/* Released and cleared together by 0x00402E60, which frees each through
   0x00403A50; only the third is rebuilt here. */
HVSURFACE g_surface_650dd4;
HVSURFACE g_surface_650dd8;
HVSURFACE g_primary_surface_view_650ddc;
HVSURFACE g_surface_650de0;
extern "C" HVSURFACE ghFrameBuffer;
extern "C" HVSURFACE ghMouseBuffer;
int g_dword_650dc0;
int g_cursor_clip_active_650db8;
int g_dword_650dc4;
int g_dword_650dc8;
int g_dword_650dcc;
int g_dword_6ef4c0;
int g_dword_650df4;
int g_dword_650df8;
int g_dword_650dfc;
int g_dword_650e00;
bool g_flag_650e04;
unsigned char g_flag_650e50;
unsigned char g_flag_5ff651;
extern unsigned char g_flag_65970f;
extern unsigned char g_flag_6598a8;
extern unsigned char g_flag_659711;
extern unsigned char g_fullscreen_603c39;
int g_dword_65a104;
int g_dword_5ff64c;
extern int g_dword_687595;
extern unsigned char g_byte_68de44;
extern unsigned char g_flag_65970f;
extern unsigned char g_flag_6598a8;
extern unsigned char g_flag_659711;
extern unsigned char g_fullscreen_603c39;
W8VideoManagerNode* g_video_object_list_650e24;
int g_dword_650e28;
int g_dword_650e2c;
int g_dword_650e30;
unsigned char g_video_objects_ready_650e20;
unsigned char g_flag_5ff5e8;

// FUNCTION: WIZ8 0x00402e60
void Function402E60(void)
{
    if (g_surface_650dd4) {
        DeleteVideoSurface(g_surface_650dd4);
        g_surface_650dd4 = 0;
    }
    if (g_surface_650dd8) {
        DeleteVideoSurface(g_surface_650dd8);
        g_surface_650dd8 = 0;
    }
    if (g_primary_surface_view_650ddc) {
        DeleteVideoSurface(g_primary_surface_view_650ddc);
        g_primary_surface_view_650ddc = 0;
    }
    if (g_surface_650de0) {
        DeleteVideoSurface(g_surface_650de0);
        g_surface_650de0 = 0;
    }
}

// FUNCTION: WIZ8 0x00421f20
IDirectDrawSurface2* Function421F20(void)
{
    return g_primary_surface_6596a8;
}

// FUNCTION: WIZ8 0x004044c0
HVSURFACE Function4044C0(IDirectDrawSurface2* surface)
{
    return CreateVideoSurfaceFromDDSurface(surface);
}


/* Named in the startup spine. The rest are gates it records as
   uncharacterisable, and the callees below it that nothing yet identifies. */
/* Three adjacent 0x34-byte SGP buffers. The helper copies the application name
   into the first and third and the key-class spelling into the middle one; it
   does not call RegisterClassA despite its old provisional name. */
char g_application_key_650eac[0x34];
char g_application_name_copy_650ee0[0x34];
char g_application_name_650f14[0x34];

// FUNCTION: WIZ8 0x0040f020
unsigned char RegisterWindowClass(const char* application_name, const char* key_class)
{
    if (!application_name || !key_class) {
        return 0;
    }
    strcpy(g_application_name_650f14, application_name);
    strcpy(g_application_key_650eac, key_class);
    strcpy(g_application_name_copy_650ee0, g_application_name_650f14);
    return 1;
}

extern void ProcessCommandLine(char* pCommandLine);
extern void GetRuntimeSettings(void);
extern unsigned char InitializeWiz8FontManager(
    unsigned short pixel_depth, FontTranslationTable* translation);

extern long __stdcall WindowProc4011E0(void* window, int message,
                                       unsigned int wparam, long lparam);


extern unsigned char InitializeRenderer(
    void* instance, unsigned short show_command, void* window_proc);



extern unsigned char InitializeGameData(void);
unsigned int g_mswheel_roll_message;
bool g_flag_6505a9;


/* Rebuilds the view of the primary DirectDraw surface. 0x00402E60 releases and
   clears the four views this module owns, 0x00421F20 hands back the primary
   surface, and 0x004044C0 wraps it in the 0x30-byte descriptor that carries its
   width, height and depth. Reports whether the wrapper was built; a missing
   surface is not an error the caller distinguishes from a failed wrap. */
// FUNCTION: WIZ8 0x00402e30
bool Function402E30(void)
{
    IDirectDrawSurface2* surface;
    bool built;

    Function402E60();
    surface = Function421F20();
    if (surface == NULL) {
        return false;
    }
    g_primary_surface_view_650ddc = Function4044C0(surface);
    built = g_primary_surface_view_650ddc != 0;
    if (built) {
        /* The released VSurface manager names these two reserved handles.
           Wizardry owns the actual DirectDraw wrapper, so publish that one
           instead of constructing a parallel surface database. */
        ghFrameBuffer = g_primary_surface_view_650ddc;
        ghMouseBuffer = g_primary_surface_view_650ddc;
    }
    return built;
}

/* Shared success stub. BringUpEngine uses it as a gate and the 62-entry frame
   dispatch table parks it in seventeen slots. */
// FUNCTION: WIZ8 0x005b1740
unsigned char Function5B1740(void)
{
    return 1;
}

// FUNCTION: WIZ8 0x00402970
bool InitializeWizardryVideoSurfaceManager(void)
{
    bool ready;

    g_dword_650dc0 = 0;
    g_surface_list_650dbc = 0;
    g_dword_6ef4c0 = 0;
    ready = Function402E30() != 0;
    return ready;
}

// FUNCTION: WIZ8 0x00402750
void Function402750(void)
{
    ClipCursor(0);
    g_cursor_clip_active_650db8 = 0;
}

// FUNCTION: WIZ8 0x00404ba0
bool InitializeVideoSurfaceState(void)
{
    g_dword_650e00 = 0;
    g_dword_650df4 = 0;
    g_dword_650df8 = 0;
    g_dword_650dfc = 0;
    g_flag_650e04 = true;
    return true;
}

// FUNCTION: WIZ8 0x00405e60
bool InitializeWizardryVideoObjectManager(void)
{
    g_dword_650e28 = 0;
    g_video_object_list_650e24 = 0;
    g_video_objects_ready_650e20 = 1;
    return true;
}

// FUNCTION: WIZ8 0x00402990
bool ShutdownWizardryVideoSurfaceManager(void)
{
    W8VideoManagerNode* node;

    Function402E60();
    while (g_surface_list_650dbc) {
        node = g_surface_list_650dbc;
        g_surface_list_650dbc = node->next;
        DeleteVideoSurface((HVSURFACE)node->handle);
        free(node);
    }
    g_dword_650dc0 = 0;
    g_dword_650dc4 = 0;
    g_dword_650dc8 = 0;
    g_dword_650dcc = 0;
    return true;
}

// FUNCTION: WIZ8 0x004029f0
bool Function4029F0(void)
{
    W8VideoManagerNode* node = g_surface_list_650dbc;

    while (node) {
        HVSURFACE surface = reinterpret_cast<HVSURFACE>(node->handle);
        if ((surface->fFlags & VSURFACE_VIDEO_MEM_USAGE) == 0 ||
            surface->pSavedSurfaceData1 == 0) {
            return false;
        }
        DDRestoreSurface(
            reinterpret_cast<IDirectDrawSurface2*>(surface->pSurfaceData));
        RECT rectangle;
        rectangle.left = 0;
        rectangle.top = 0;
        rectangle.right = surface->usWidth;
        rectangle.bottom = surface->usHeight;
        DDBltFastSurface(
            reinterpret_cast<IDirectDrawSurface2*>(surface->pSavedSurfaceData),
            0, 0,
            reinterpret_cast<IDirectDrawSurface2*>(surface->pSurfaceData),
            &rectangle, 0);
        node = node->next;
    }
    return true;
}

// FUNCTION: WIZ8 0x00405e80
void ShutdownWizardryVideoObjectManager(void)
{
    W8VideoManagerNode* node;

    while (g_video_object_list_650e24) {
        node = g_video_object_list_650e24;
        g_video_object_list_650e24 = node->next;
        DeleteVideoObject((HVOBJECT)node->handle);
        free(node);
    }
    g_dword_650e28 = 0;
    g_dword_650e2c = 0;
    g_dword_650e30 = 0;
    g_video_objects_ready_650e20 = 0;
    g_flag_5ff5e8 = 1;
}

// FUNCTION: WIZ8 0x00428b80
int ReturnZero(void)
{
    return 0;
}

// FUNCTION: WIZ8 0x00427a60
char* GetVideoConfigFileName(void)
{
    return "3DVideo.CFG";
}

unsigned short* g_pointer_table_6ed440[0x400];
unsigned char g_flags_6ed040[0x400];
bool g_flag_650de4;
bool g_flag_5ff538;
unsigned char g_flag_6ef440;

void ShutdownHandler(void);
bool SetModuleSubdirectory(const char* subdirectory);
bool g_shutdown_started_650db5;
bool g_teardown_done_650db4;
char g_shutdown_message_6505ac[0x100];
extern void ShutdownScreenStack(int flag);
extern void ShutdownGameData(void);
extern void ShutdownDisplayList(void);
extern bool ShutdownWizardryVideoSurfaceManager(void);
extern void ShutdownWizardryVideoObjectManager(void);
extern void ShutdownRenderer(void);
extern int ReturnZero(void);



/* Stores the byte 0x004086D0's environment selection consults. */
// FUNCTION: WIZ8 0x004086c0
void Function4086C0(int enable)
{
    g_flag_5ff651 = (unsigned char)enable;
}

/* Reports the byte at 0x00603C39; the only reader is 0x00421BB0. */
// FUNCTION: WIZ8 0x004229b0
unsigned char Function4229B0(void)
{
    return g_fullscreen_603c39;
}

/* Three latches, each set once and never cleared here. */
// FUNCTION: WIZ8 0x004229d0
void Function4229D0(void)
{
    g_flag_659711 = 1;
}

// FUNCTION: WIZ8 0x004277d0
void Function4277D0(void)
{
    g_flag_65970f = 1;
}

// FUNCTION: WIZ8 0x0042bc00
void Function42BC00(void)
{
    g_flag_6598a8 = 1;
}


/* Takes the default unless the caller names a size. Nothing yet establishes
   what consumes the value, so both it and its holder keep address-derived
   names. */
// FUNCTION: WIZ8 0x004098d0
unsigned char Function4098D0(int size)
{
    g_dword_5ff64c = 0x1f5800;
    if (size != 0) {
        g_dword_5ff64c = size;
    }
    return 1;
}

/* Sets the size above to 0x000C8000 and clears its companion. */
// FUNCTION: WIZ8 0x00479010
void Function479010(void)
{
    Function4098D0(0xc8000);
    g_dword_65a104 = 0;
}

/* Three more single-global writes, each in a different unit. The first stores a
   full dword and hands the same value back - it materializes the 1 in eax and
   stores through it - so its global is not the byte flag its one-bit use
   suggests, and it is not the void setter the other two are. */
// FUNCTION: WIZ8 0x00443a50
int Function443A50(void)
{
    g_status_685170.next_trigger_id_2356 = 1;
    return 1;
}

// FUNCTION: WIZ8 0x00482740
void Function482740(int value)
{
    g_dword_687595 = value;
}

// FUNCTION: WIZ8 0x005588e0
void Function5588E0(unsigned char value)
{
    g_byte_68de44 = value;
}

/* Clears the flag InitializeVideoSurfaceState raises. Both the window
   procedure's teardown and the shutdown handler reach it. */
// FUNCTION: WIZ8 0x00404bc0
void ShutdownVideoSurfaceState(void)
{
    g_flag_650e04 = false;
}

/* Clears the flag the 0x004086D0 environment selection leaves set. */
// FUNCTION: WIZ8 0x00408850
void Function408850(void)
{
    g_flag_650e50 = 0;
}

/* Empty in the shipped build: a single ret. BringUpEngine still calls it. */
// FUNCTION: WIZ8 0x004023a0
void NoOp(void)
{
}

/* Clears the pointer table, then walks it releasing each entry. The walk can
   never see a live entry because the clear precedes it, but both are in the
   original and the compiler kept them, so both are reproduced. */
// FUNCTION: WIZ8 0x00404b00
void Function404B00(void)
{
    unsigned short** table;
    unsigned char* flags;
    unsigned short* entry;
    int remaining;

    memset(g_pointer_table_6ed440, 0, sizeof(g_pointer_table_6ed440));
    table = g_pointer_table_6ed440;
    flags = g_flags_6ed040;
    remaining = 0x400;
    do {
        entry = *table;
        *flags = 0;
        if (entry) {
            *entry = 0xffff;
            *table = 0;
        }
        ++flags;
        ++table;
        --remaining;
    } while (remaining);
    g_flag_650de4 = true;
    g_flag_5ff538 = true;
    g_flag_6ef440 = 0;
}

extern void Function4E3340(void);
/* These are retained SGP globals, named by the vendored declaration surface. */
extern "C" HINSTANCE ghInstance;
extern "C" unsigned char gfApplicationActive;

/* The caller shifts the result right by ten and stores kilobytes. */
// FUNCTION: WIZ8 0x00404bd0
unsigned int QueryAvailableMemory(void)
{
    MEMORYSTATUS status;

    status.dwLength = sizeof(status);
    GlobalMemoryStatus(&status);
    return status.dwAvailPhys;
}


/* An eight-byte node: a code and a payload word. 0x00407D30 builds a head node
   pointing at a copy of the caller's, which is the only structure either
   establishes. */
struct W8BindingNode {
    unsigned short code;                  /* 0x00 */
    unsigned char unknown_02[2];
    void* payload;                        /* 0x04 */
};

int g_dword_5ff5f0;
int g_dword_5ff5f4;
int g_dword_5ff5f8;
int g_dword_5ff5fc;
int g_dword_5ff600;
int g_dword_5ff604;
int g_dword_5ff608;
int g_dword_5ff60c;
unsigned char g_flag_650e38;
W8BindingNode* g_binding_head_6eb704;
unsigned char g_block_6eb6a0[0x64];

extern void GetDefaultScreenMode(unsigned short* height,
                                 unsigned short* width,
                                 unsigned char* depth);
extern void Function427A30(const char* path);
extern void Function422970(int enable);

extern unsigned char Function409C50(void);

unsigned char g_flag_5ff652;
void* g_pointer_5ff648;
unsigned char g_block_6e4120[0x980];
unsigned char g_block_6e4aa0[0x6c00];
int g_dword_650e4c;
extern char* g_sound_provider_650e54;
HPROVIDER g_provider_650e58;
H3DPOBJECT g_listener_650e5c;
unsigned char g_buffer_7dc000[1];

/* Walks the Miles 3D providers for the one whose name matches the configured
   string, opens it and its listener, and records whether the provider exposes
   EAX environment selection. Any failure leaves the subsystem closed and still
   reports success, so audio never blocks bring-up. */
// FUNCTION: WIZ8 0x004086d0
bool Function4086D0(void)
{
    HPROENUM next;
    HPROVIDER provider;
    C8* name;
    S32 attribute;

    if (g_flag_650e50) {
        g_flag_650e50 = 0;
    }
    memset(g_block_6e4120, 0, sizeof(g_block_6e4120));
    if (g_flag_5ff651 && Function409C50()) {
        g_flag_650e50 = 1;
    }
    g_pointer_5ff648 = g_buffer_7dc000;
    memset(g_block_6e4aa0, 0, sizeof(g_block_6e4aa0));
    g_dword_650e4c = 0;
    g_dword_5ff64c = 0x1f5800;
    if (g_sound_provider_650e54 && g_provider_650e58 == 0) {
        next = 0;
        provider = 0;
        if (g_flag_650e50 && g_sound_provider_650e54) {
            do {
                do {
                    if (AIL_enumerate_3D_providers(&next, &provider, &name) == 0) {
                        return true;
                    }
                } while (provider == 0);
            } while (strcmp(g_sound_provider_650e54, name) != 0);
            if (AIL_open_3D_provider(provider) == 0) {
                g_provider_650e58 = provider;
                g_listener_650e5c = AIL_open_3D_listener(provider);
                if (g_listener_650e5c == 0) {
                    AIL_close_3D_provider(g_provider_650e58);
                    return true;
                }
                if (g_flag_650e50) {
                    AIL_set_3D_position(g_listener_650e5c, 0, 0, 0);
                }
                AIL_3D_provider_attribute(g_provider_650e58, "EAX environment selection",
                                          &attribute);
                if (attribute != -1) {
                    g_flag_5ff652 = 1;
                }
            }
        }
    }
    return true;
}

/* Builds the default key binding table: a head node carrying the count and a
   0x1f8-byte array of 252 key codes. Neither allocation is checked, and the
   codes are transcribed from the canonical encoding rather than by hand.

   The cursor form is what the original uses: it gives the same 517 instructions
   and the same tail, where an indexed write collapses to 269. What remains is a
   one-byte-per-entry phase difference. The original pairs each entry as bump
   then store through [eax]; this pairs it as store through [eax+2] then bump,
   which is the same semantics and the same instruction count but one byte
   longer, and it pushes the batched cdecl cleanup from just after the first
   entry to the middle of the table. Post-increment, pre-increment, indexing,
   and dropping the second cursor local all compile to one of these two shapes,
   so the phase is not reachable by rewriting the loop body. */
// FUNCTION: WIZ8 0x00407ec0
W8BindingNode* Function407EC0(void)
{
    W8BindingNode* node;
    unsigned short* codes;
    unsigned short* cursor;

    node = (W8BindingNode*)malloc(8);
    node->code = 0xfc;
    codes = (unsigned short*)malloc(0x1f8);
    node->payload = codes;
    cursor = codes;
    *cursor++ = 0x41;
    *cursor++ = 0x42;
    *cursor++ = 0x43;
    *cursor++ = 0x44;
    *cursor++ = 0x45;
    *cursor++ = 0x46;
    *cursor++ = 0x47;
    *cursor++ = 0x48;
    *cursor++ = 0x49;
    *cursor++ = 0x4a;
    *cursor++ = 0x4b;
    *cursor++ = 0x4c;
    *cursor++ = 0x4d;
    *cursor++ = 0x4e;
    *cursor++ = 0x4f;
    *cursor++ = 0x50;
    *cursor++ = 0x51;
    *cursor++ = 0x52;
    *cursor++ = 0x53;
    *cursor++ = 0x54;
    *cursor++ = 0x55;
    *cursor++ = 0x56;
    *cursor++ = 0x57;
    *cursor++ = 0x58;
    *cursor++ = 0x59;
    *cursor++ = 0x5a;
    *cursor++ = 0x61;
    *cursor++ = 0x62;
    *cursor++ = 0x63;
    *cursor++ = 0x64;
    *cursor++ = 0x65;
    *cursor++ = 0x66;
    *cursor++ = 0x67;
    *cursor++ = 0x68;
    *cursor++ = 0x69;
    *cursor++ = 0x6a;
    *cursor++ = 0x6b;
    *cursor++ = 0x6c;
    *cursor++ = 0x6d;
    *cursor++ = 0x6e;
    *cursor++ = 0x6f;
    *cursor++ = 0x70;
    *cursor++ = 0x71;
    *cursor++ = 0x72;
    *cursor++ = 0x73;
    *cursor++ = 0x74;
    *cursor++ = 0x75;
    *cursor++ = 0x76;
    *cursor++ = 0x77;
    *cursor++ = 0x78;
    *cursor++ = 0x79;
    *cursor++ = 0x7a;
    *cursor++ = 0x30;
    *cursor++ = 0x31;
    *cursor++ = 0x32;
    *cursor++ = 0x33;
    *cursor++ = 0x34;
    *cursor++ = 0x35;
    *cursor++ = 0x36;
    *cursor++ = 0x37;
    *cursor++ = 0x38;
    *cursor++ = 0x39;
    *cursor++ = 0x21;
    *cursor++ = 0x40;
    *cursor++ = 0x23;
    *cursor++ = 0x24;
    *cursor++ = 0x25;
    *cursor++ = 0x5e;
    *cursor++ = 0x26;
    *cursor++ = 0x2a;
    *cursor++ = 0x28;
    *cursor++ = 0x29;
    *cursor++ = 0x2d;
    *cursor++ = 0x5f;
    *cursor++ = 0x2b;
    *cursor++ = 0x3d;
    *cursor++ = 0x7c;
    *cursor++ = 0x5c;
    *cursor++ = 0x7b;
    *cursor++ = 0x7d;
    *cursor++ = 0x5b;
    *cursor++ = 0x5d;
    *cursor++ = 0x3a;
    *cursor++ = 0x3b;
    *cursor++ = 0x22;
    *cursor++ = 0x27;
    *cursor++ = 0x3c;
    *cursor++ = 0x3e;
    *cursor++ = 0x2c;
    *cursor++ = 0x2e;
    *cursor++ = 0x3f;
    *cursor++ = 0x2f;
    *cursor++ = 0x20;
    *cursor++ = 0xc1;
    *cursor++ = 0xc0;
    *cursor++ = 0xc1;
    *cursor++ = 0xc4;
    *cursor++ = 0xc3;
    *cursor++ = 0xc5;
    *cursor++ = 0xc7;
    *cursor++ = 0xc9;
    *cursor++ = 0xc8;
    *cursor++ = 0xca;
    *cursor++ = 0xcb;
    *cursor++ = 0xcd;
    *cursor++ = 0xcc;
    *cursor++ = 0xce;
    *cursor++ = 0xcf;
    *cursor++ = 0xd1;
    *cursor++ = 0xd3;
    *cursor++ = 0xd2;
    *cursor++ = 0xd4;
    *cursor++ = 0xd6;
    *cursor++ = 0xd5;
    *cursor++ = 0xd8;
    *cursor++ = 0xda;
    *cursor++ = 0xd9;
    *cursor++ = 0xdb;
    *cursor++ = 0xdc;
    *cursor++ = 0xdd;
    *cursor++ = 0xe1;
    *cursor++ = 0xe0;
    *cursor++ = 0xe2;
    *cursor++ = 0xe4;
    *cursor++ = 0xe3;
    *cursor++ = 0xe5;
    *cursor++ = 0xe7;
    *cursor++ = 0xe9;
    *cursor++ = 0xe8;
    *cursor++ = 0xea;
    *cursor++ = 0xeb;
    *cursor++ = 0xed;
    *cursor++ = 0xec;
    *cursor++ = 0xee;
    *cursor++ = 0xef;
    *cursor++ = 0xf1;
    *cursor++ = 0xf3;
    *cursor++ = 0xf2;
    *cursor++ = 0xf4;
    *cursor++ = 0xf6;
    *cursor++ = 0xf5;
    *cursor++ = 0xf8;
    *cursor++ = 0xfa;
    *cursor++ = 0xf9;
    *cursor++ = 0xfb;
    *cursor++ = 0xfc;
    *cursor++ = 0xfe;
    *cursor++ = 0xff;
    *cursor++ = 0xdf;
    *cursor++ = 0xfff0;
    *cursor++ = 0xfff1;
    *cursor++ = 0xfff2;
    *cursor++ = 0xfff3;
    *cursor++ = 0xfff4;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0xbf;
    *cursor++ = 0xa1;
    return node;
}

/* Publishes the three values 0x00422AF0 reports, narrowed to their field
   widths, then - when given a source node - allocates a head and a copy of it.
   Either allocation failing abandons the whole thing, leaking the first. */
// FUNCTION: WIZ8 0x00407d30
bool Function407D30(unsigned short code, W8BindingNode* source)
{
    unsigned short first;
    unsigned short second;
    unsigned char third;
    W8BindingNode* copy;

    g_dword_5ff5f0 = -1;
    g_dword_5ff5f4 = -15;
    g_dword_5ff5f8 = 0;
    GetDefaultScreenMode(&first, &second, &third);
    g_dword_5ff600 = 0;
    g_dword_5ff604 = 0;
    g_dword_5ff608 = first;
    g_dword_5ff60c = second;
    g_dword_5ff5fc = third;
    g_flag_650e38 = 0;
    if (!source) {
        return false;
    }
    g_binding_head_6eb704 = (W8BindingNode*)malloc(8);
    if (!g_binding_head_6eb704) {
        return false;
    }
    copy = (W8BindingNode*)malloc(8);
    if (!copy) {
        return false;
    }
    g_binding_head_6eb704->payload = copy;
    g_binding_head_6eb704->code = code;
    copy->code = source->code;
    copy->payload = source->payload;
    memset(g_block_6eb6a0, 0, sizeof(g_block_6eb6a0));
    return true;
}

/* Appends the subdirectory to the working directory and prepends the result to
   PATH, so plug-in DLLs load from the shipped subdirectory. Every string call
   here is inlined by VC6, which is why the body is mostly rep movs. */
// FUNCTION: WIZ8 0x00405740
bool SetModuleSubdirectory(const char* subdirectory)
{
    char path[520];
    CHAR environment[520];
    unsigned int length;

    if (!subdirectory) {
        return false;
    }
    if (strlen(subdirectory) == 0) {
        return false;
    }
    _getcwd(path, 0x208);
    length = strlen(path);
    if (path[length != 0 ? length - 1 : 0] != '\\') {
        strcat(path, "\\");
    }
    strcat(path, subdirectory);
    if (GetEnvironmentVariableA("PATH", environment, 0x208) == 0) {
        return false;
    }
    strcat(environment, ";");
    strcat(environment, path);
    SetEnvironmentVariableA("PATH", environment);
    return true;
}

/* Registered with atexit as BringUpEngine's first act. Guarded twice: a once
   flag so a second exit does nothing, and a separate teardown flag so the long
   release sequence runs at most once. The engine flag BringUpEngine sets on
   success decides how much of it applies. Any message left in the buffer is
   shown before handing off. */
// FUNCTION: WIZ8 0x004017f0
void ShutdownHandler(void)
{
    unsigned char engine_up;

    if (g_shutdown_started_650db5) {
        return;
    }
    g_shutdown_started_650db5 = true;
    gfProgramIsRunning = 0;
    Function408850();
    if (g_flag_6505a9) {
        ShutdownScreenStack(1);
    }
    if (!g_teardown_done_650db4) {
        engine_up = g_flag_6505a9;
        g_teardown_done_650db4 = true;
        if (engine_up) {
            ShutdownGameData();
        }
        ShutdownButtonSystem();
        ShutdownDisplayList();
        Function408850();
        DestroyEnglishTransTable();
        ShutdownFontManager();
        ShutdownClockManager();
        ShutdownWizardryVideoSurfaceManager();
        ShutdownWizardryVideoObjectManager();
        ShutdownRenderer();
        ShutdownInputManager();
        NoOp();
        NoOp();
        ShutdownVideoSurfaceState();
        NoOp();
    }
    ShowCursor(TRUE);
    if (strlen(g_shutdown_message_6505ac) != 0) {
        MessageBoxA(NULL, g_shutdown_message_6505ac, "Error", MB_ICONHAND);
    }
    ReturnZero();
}

/* The window init sequence. Registers the window class, points module loading
   at the DLL subdirectory, then runs each bring-up gate in order and abandons
   the whole sequence the moment one reports failure. Like the smaller gates it
   returns the flag it raises at the end. */
// FUNCTION: WIZ8 0x00401570
bool BringUpEngine(void* instance, int show_command)
{
    FontTranslationTable* font_table;

    atexit(ShutdownHandler);
    RegisterWindowClass("Wizardry8", "Wizardry8key");
    SetModuleSubdirectory("DLL");
    GetRuntimeSettings();
    Function404B00();
    if (!InitializeVideoSurfaceState()) {
        return false;
    }
    if (!Function5B1740()) {
        return false;
    }
    NoOp();
    if (!InitializeInputManager()) {
        return false;
    }
    if (!InitializeRenderer(instance, show_command, WindowProc4011E0)) {
        return false;
    }
    if (!InitializeWizardryVideoObjectManager()) {
        return false;
    }
    if (!InitializeWizardryVideoSurfaceManager()) {
        return false;
    }
    InitializeClockManager();
    font_table = CreateEnglishTransTable();
    if (!font_table) {
        return false;
    }
    if (!InitializeWiz8FontManager(8, font_table)) {
        return false;
    }
    free(font_table);
    if (!Function4086D0()) {
        return false;
    }
    InitializeRandom();
    if (!InitializeGameData()) {
        return false;
    }
    g_mswheel_roll_message = RegisterWindowMessageA("MSWHEEL_ROLLMSG");
    g_flag_6505a9 = true;
    return true;
}

/* A running instance is found by class and title both spelled "Wizardry 8"; it
   is raised and this one exits. Otherwise the video configuration file gates
   startup: absent, 3DSetup.EXE is spawned to write it and the check repeats,
   and still absent ends the run. The message loop ticks a frame whenever no
   message is waiting and the application is active, and waits otherwise. */
/* The shipped body allocates a 0xC8 frame and jumps straight to returning true.
   The CD check it guards - a sprintf of the insert-CD message and a MessageBoxA
   - sits between that jump and its target and is unreachable, which is the
   shape of a patch rather than of compiler output: the jump overwrites the
   first instruction after the prologue. All three GOG builds carry it
   identically, the retail image is protected so its addresses do not line up,
   and the demo has no insert-CD string at all, so the corpus holds no unpatched
   reference to recover the original check from.

   This therefore models the shipped behaviour rather than the shipped bytes,
   and is recorded structurally-strong: matching 87 bytes of retained-but-
   unreachable code is not something C can express, since the compiler would
   delete it. */
// FUNCTION: WIZ8 0x0042b830
bool CheckCdPresent(void)
{
    return true;
}

// FUNCTION: WIZ8 0x00401670
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    MSG message;
    HWND existing;

    existing = FindWindowExA(NULL, NULL, "Wizardry 8", "Wizardry 8");
    if (existing) {
        SetForegroundWindow(existing);
        ShowWindow(existing, 9);
        return 0;
    }
    ghInstance = hInstance;
    ProcessCommandLine(lpCmdLine);
    giStartMem = QueryAvailableMemory() >> 10;
    if (!FileExists(GetVideoConfigFileName())) {
        _spawnl(0, "3DSetup.EXE", "3DSetup.EXE", GetVideoConfigFileName(), NULL);
    }
    if (!FileExists(GetVideoConfigFileName())) {
        return 0;
    }
    if (!CheckCdPresent()) {
        return 0;
    }
    ShowCursor(FALSE);
    if (!BringUpEngine(hInstance, nShowCmd)) {
        return 0;
    }
    gfApplicationActive = 1;
    gfProgramIsRunning = 1;
    do {
        if (PeekMessageA(&message, NULL, 0, 0, 0)) {
            if (GetMessageA(&message, NULL, 0, 0) == 0) {
                return message.wParam;
            }
            TranslateMessage(&message);
            DispatchMessageA(&message);
        } else if (gfApplicationActive == 0) {
            WaitMessage();
        } else {
            Function4E3340();
            gfSGPInputReceived = 0;
        }
    } while (gfProgramIsRunning);
    PostQuitMessage(0);
    return message.wParam;
}
