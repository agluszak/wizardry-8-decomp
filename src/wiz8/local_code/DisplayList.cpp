#include "wiz8/local_code/DisplayList.h"
#include "wiz8/regions.h"
#include "wiz8/screen_state.h"
#include "wiz8/utility.h"
#include "Types.h"
#include "mousesystem.h"
#include "timer.h"

#include <stdlib.h>
#include <string.h>

extern "C" {
void Function40B830(MOUSE_REGION* node);
extern void HideRegionHelp(void);
extern void Function5A1140(short shape);
extern void SetHelpBoxText(void* text);
extern void PlaceHelpBox(int x, int y);
extern int g_help_box_width;
extern int g_help_box_height;
extern unsigned char g_flag_5ff7ca;

short g_word_5ff7c8;
MOUSE_REGION* g_display_ptr_650e6c;
MOUSE_REGION* g_display_ptr_650e70;
unsigned int g_time_650e74;

/* The buffer is not released with a direct call to free: the body loads a
   function pointer from 0x005EB224 once, before the loop, and calls through it
   for every node. */
void (*g_deallocator_5eb224)(void*) = free;

MOUSE_REGION g_display_template_5ff7d0;
MOUSE_REGION* g_display_head_650e94;
MOUSE_REGION* g_display_ptr_650e98;
MOUSE_REGION* g_display_ptr_650e9c;
unsigned char g_display_flag_650ea0;
unsigned char g_display_flag_650e90;
unsigned short g_display_id_6e4100;
unsigned char g_fast_help_render_enabled_5ff824 = 1;
int g_fast_help_last_clock_650e68;

int g_dword_650e78;
int g_dword_650e7c;
short g_word_650e80;
short g_word_650e82;
short g_word_650e84;
unsigned short g_word_650e86;
unsigned char g_byte_650e88;
unsigned char g_byte_650e89;
unsigned char g_byte_650e8a;
MOUSE_REGION* g_display_ptr_650e8c;

/* This is the pinned SGP RenderFastHelp body selected by its Wizardry build.
   Wizardry's video tooltip calls are represented by the already recovered
   game-owned help-box boundary, but the timer and mouse-region rules remain
   the source-owned SGP implementation. */
// LIBRARY: WIZ8 0x0040c0b0
// RenderFastHelp
void Function40C0B0(void)
{
    int current_clock;
    int elapsed;
    int x;
    int y;

    if (!g_fast_help_render_enabled_5ff824) {
        return;
    }
    current_clock = GetClock();
    elapsed = current_clock - g_fast_help_last_clock_650e68;
    if (elapsed < 0) {
        elapsed += 0x7fffffff;
    }
    g_fast_help_last_clock_650e68 = current_clock;

    if (g_display_ptr_650e9c == 0 || g_display_ptr_650e9c->FastHelpText == 0 ||
        !g_flag_5ff7ca) {
        return;
    }
    if (g_display_ptr_650e9c->FastHelpTimer == 0) {
        if ((g_display_ptr_650e9c->uiFlags &
             (MSYS_ALLOW_DISABLED_FASTHELP | MSYS_REGION_ENABLED)) == 0) {
            return;
        }
        if ((g_display_ptr_650e9c->uiFlags & MSYS_MOUSE_IN_AREA) == 0) {
            g_display_ptr_650e9c->uiFlags &=
                ~(MSYS_FASTHELP | MSYS_FASTHELP_RESET);
            ReleaseScreenTransitionObjects();
            return;
        }
        g_display_ptr_650e9c->uiFlags |= MSYS_FASTHELP;
        SetHelpBoxText(g_display_ptr_650e9c->FastHelpText);
        x = g_display_ptr_650e9c->RegionTopLeftX + 10;
        if (x < 0) x = 0;
        if (x + g_help_box_width >= 640) x = 636 - g_help_box_width;
        y = g_display_ptr_650e9c->RegionTopLeftY - g_help_box_height * 3 / 4;
        if (y < 0) y = 0;
        if (y + g_help_box_height >= 480) y = 465 - g_help_box_height;
        PlaceHelpBox(x, y);
        return;
    }
    if ((g_display_ptr_650e9c->uiFlags &
         (MSYS_ALLOW_DISABLED_FASTHELP | MSYS_REGION_ENABLED)) != 0 &&
        (g_display_ptr_650e9c->uiFlags & MSYS_MOUSE_IN_AREA) != 0 &&
        g_display_ptr_650e9c->ButtonState == 0) {
        if (elapsed > 0) {
            g_display_ptr_650e9c->FastHelpTimer -= (short)elapsed;
        }
        if (g_display_ptr_650e9c->FastHelpTimer < 0) {
            g_display_ptr_650e9c->FastHelpTimer = 0;
        }
    }
}

/*
 * Inserts a node into the display list, ordered on the key at +0x02 with
 * higher keys first, after giving it an identifier from the counter at
 * 0x00650E7C.
 *
 * The identifier search is only entered once that counter would pass
 * 0x0FFFFFFF, or once the flag at 0x00650E78 records that it already has. In
 * that branch the original does not advance its scan pointer: it reads the
 * list head into a register before the loop and compares that one node's id on
 * every iteration, so the search only terminates if the head happens to carry
 * the identifier being tried. This is reproduced rather than repaired. It is
 * what the shipped code does, it is unreachable until the counter wraps, and
 * silently inserting the advance would make the body describe a program that
 * was never built.
 *
 * The identifier written into the node is the counter's value from before the
 * increment, so the search updates the global and never the node.
 */
// FUNCTION: WIZ8 0x0040b720
void Function40B720(MOUSE_REGION* node)
{
    MOUSE_REGION* scan;
    MOUSE_REGION* cursor;
    int assigned;
    int candidate;
    int found;

    if (node->next != 0 || node->prev != 0) {
        Function40B830(node);
    }
    assigned = g_dword_650e7c;
    candidate = 1;
    g_dword_650e7c = assigned + 1;
    if (g_dword_650e7c >= 0xfffffff || g_dword_650e78 != 0) {
        scan = g_display_head_650e94;
        g_dword_650e78 = 1;
        for (;;) {
            found = 0;
            if (scan == 0) {
                break;
            }
            while (!found) {
                if (scan->IDNumber == candidate) {
                    found = 1;
                }
            }
            if (candidate >= 0xfffffff) {
                if (found) {
                    candidate = 0xfffffff;
                }
                break;
            }
            candidate = candidate + 1;
        }
        g_dword_650e7c = candidate;
    }
    node->IDNumber = (short)assigned;
    node->next = 0;
    node->prev = 0;

    cursor = g_display_head_650e94;
    if (cursor != 0) {
        MOUSE_REGION* ahead;
        MOUSE_REGION* pick;

        found = 0;
        ahead = cursor->next;
        for (;;) {
            pick = ahead;
            if (pick == 0 || found) {
                break;
            }
            if (cursor->PriorityLevel <= node->PriorityLevel) {
                found = 1;
                pick = cursor;
            }
            ahead = pick->next;
            cursor = pick;
        }
        if (node->PriorityLevel < cursor->PriorityLevel) {
            node->next = cursor->next;
            cursor->next = node;
            node->prev = cursor;
            if (node->next == 0) {
                return;
            }
            node->next->prev = node;
            return;
        }
        node->next = cursor;
        node->prev = cursor->prev;
        cursor->prev = node;
        if (node->prev != 0) {
            node->prev->next = node;
        }
        if (g_display_head_650e94 != cursor) {
            return;
        }
    }
    g_display_head_650e94 = node;
}

/*
 * Unlinks a node from the display list. The search that guards it compares
 * identifiers rather than pointers, so a node is unlinked when one carrying
 * its id is on the list, not only when that exact node is.
 */
// FUNCTION: WIZ8 0x0040b830
void Function40B830(MOUSE_REGION* node)
{
    MOUSE_REGION* scan;
    int found;

    if (g_display_head_650e94 == 0) {
        return;
    }
    found = 0;
    scan = g_display_head_650e94;
    while (scan != 0 && !found) {
        if (scan->IDNumber == node->IDNumber) {
            found = 1;
        }
        scan = scan->next;
    }
    if (!found) {
        return;
    }

    if (g_display_head_650e94 == node) {
        g_display_head_650e94 = node->next;
        if (g_display_head_650e94 != 0) {
            g_display_head_650e94->prev = 0;
        }
        node->prev = 0;
        node->next = 0;
    } else {
        if (node->prev != 0) {
            node->prev->next = node->next;
        }
        if (node->next != 0) {
            node->next->prev = node->prev;
        }
        node->next = 0;
        node->prev = 0;
    }

    if (g_byte_650e8a != 0 && g_display_ptr_650e8c == node) {
        g_byte_650e8a = 0;
        g_display_ptr_650e8c = 0;
    }
    if (g_display_head_650e94 == &g_display_template_5ff7d0) {
        g_dword_650e78 = 0;
        g_dword_650e7c = 1;
        return;
    }
    if (g_display_head_650e94 == 0) {
        g_dword_650e7c = 0;
        g_dword_650e78 = 0;
    }
}

// FUNCTION: WIZ8 0x0040b290
int Function40B290(void)
{
    MOUSE_REGION* node;
    void (*deallocate)(void*);

    node = g_display_head_650e94;
    deallocate = g_deallocator_5eb224;
    while (node != 0) {
        if ((node->uiFlags & 0x10) != 0) {
            if (node == 0) {
                break;
            }
            if (node->FastHelpText != 0) {
                if ((node->uiFlags & 0x80) != 0) {
                    HideRegionHelp();
                }
                (*deallocate)(node->FastHelpText);
            }
            node->FastHelpText = 0;
            Function40B830(node);
            if (g_display_ptr_650e98 == node) {
                g_display_ptr_650e98 = 0;
            }
            if (g_display_ptr_650e9c == node) {
                g_display_ptr_650e9c = 0;
            }
            g_display_flag_650ea0 = 1;
            if (g_display_flag_650e90 != 0 && g_display_id_6e4100 == node->IDNumber) {
                g_display_flag_650e90 = 0;
            }
            memset(node, 0, sizeof(*node));
            node = g_display_head_650e94;
        } else {
            node = node->next;
            g_display_head_650e94 = node;
        }
    }

    g_display_template_5ff7d0.RegionTopLeftX = (short)0x8001;
    g_display_template_5ff7d0.RegionTopLeftY = (short)0x8001;
    g_dword_650e7c = 0;
    g_dword_650e78 = 0;
    g_word_650e80 = 0;
    g_word_650e82 = 0;
    g_word_650e84 = 0;
    g_word_650e86 = 0;
    g_display_ptr_650e98 = 0;
    g_byte_650e88 = 1;
    g_byte_650e89 = 0;
    g_byte_650e8a = 0;
    g_display_ptr_650e8c = 0;
    g_display_template_5ff7d0.IDNumber = 0;
    g_display_template_5ff7d0.PriorityLevel = -1;
    g_display_template_5ff7d0.uiFlags = 0x40;
    g_display_template_5ff7d0.RegionBottomRightX = 0x7fff;
    g_display_template_5ff7d0.RegionBottomRightY = 0x7fff;
    g_display_template_5ff7d0.MouseXPos = 0;
    g_display_template_5ff7d0.MouseYPos = 0;
    g_display_template_5ff7d0.RelativeXPos = 0;
    g_display_template_5ff7d0.RelativeYPos = 0;
    g_display_template_5ff7d0.ButtonState = 0;
    g_display_template_5ff7d0.Cursor = 0;
    g_display_template_5ff7d0.UserData[0] = 0;
    g_display_template_5ff7d0.UserData[1] = 0;
    g_display_template_5ff7d0.UserData[2] = 0;
    g_display_template_5ff7d0.UserData[3] = 0;
    g_display_template_5ff7d0.MovementCallback = 0;
    g_display_template_5ff7d0.ButtonCallback = 0;
    g_display_template_5ff7d0.FastHelpTimer = 0;
    g_display_template_5ff7d0.FastHelpText = 0;
    g_display_template_5ff7d0.FastHelpRect = -1;
    g_display_template_5ff7d0.next = 0;
    g_display_template_5ff7d0.prev = 0;
    Function40B720(&g_display_template_5ff7d0);
    g_byte_650e89 = 1;
    return 1;
}

/*
 * The other teardown. It differs from 0x0040B290 in leaving the template and
 * the counters alone and in not advancing the head past the nodes it skips,
 * and the image carries the destruction block inline in both, so it is written
 * out twice here rather than shared behind a helper VC6 would not have inlined.
 */
// FUNCTION: WIZ8 0x0040b450
void ShutdownDisplayList(void)
{
    MOUSE_REGION* node;
    void (*deallocate)(void*);

    node = g_display_head_650e94;
    g_byte_650e88 = 0;
    g_byte_650e89 = 0;
    deallocate = g_deallocator_5eb224;
    while (node != 0) {
        if ((node->uiFlags & 0x10) != 0) {
            g_display_head_650e94 = node;
            if (node->FastHelpText != 0) {
                if ((node->uiFlags & 0x80) != 0) {
                    HideRegionHelp();
                }
                (*deallocate)(node->FastHelpText);
            }
            node->FastHelpText = 0;
            Function40B830(node);
            if (g_display_ptr_650e98 == node) {
                g_display_ptr_650e98 = 0;
            }
            if (g_display_ptr_650e9c == node) {
                g_display_ptr_650e9c = 0;
            }
            g_display_flag_650ea0 = 1;
            if (g_display_flag_650e90 != 0 && g_display_id_6e4100 == node->IDNumber) {
                g_display_flag_650e90 = 0;
            }
            memset(node, 0, sizeof(*node));
            node = g_display_head_650e94;
        } else {
            node = node->next;
        }
    }
    g_display_head_650e94 = node;

}

/*
 * Routes the cursor to the display list: finds the node under it, tells the
 * one it just left that it lost the cursor, and turns the pending event mask
 * at 0x00650E86 into a call on the node's input callback.
 *
 * The search runs the list in order and takes the first node whose bounds
 * contain the cursor, which is why the insert sorts on +0x02 - the key decides
 * who wins an overlap. When the capture flag at 0x00650E8A is set the search is
 * skipped entirely and the captured node at 0x00650E8C takes the cursor
 * wherever it is.
 *
 * Two quirks are preserved rather than tidied. The cursor shape is written to
 * the newly hit node while the code is otherwise servicing the node being
 * left, and the double-click window is compared with a 400 unit tolerance
 * against a clock the caller never resets.
 */
// FUNCTION: WIZ8 0x0040b900
void Function40B900(void)
{
    MOUSE_REGION* node;
    MOUSE_REGION* child;
    int hit;
    int child_hit;
    unsigned short mask;
    unsigned int events;
    unsigned int now;

    child_hit = 0;
    hit = 0;
    g_display_ptr_650e9c = g_display_head_650e94;
    if (g_byte_650e8a == 0) {
        for (; g_display_ptr_650e9c != 0; g_display_ptr_650e9c = g_display_ptr_650e9c->next) {
            if ((g_display_ptr_650e9c->uiFlags & 0x840) != 0
                && g_display_ptr_650e9c->RegionTopLeftX <= g_word_650e80
                && g_display_ptr_650e9c->RegionTopLeftY <= g_word_650e82
                && g_word_650e80 <= g_display_ptr_650e9c->RegionBottomRightX
                && g_word_650e82 <= g_display_ptr_650e9c->RegionBottomRightY) {
                hit = 1;
                break;
            }
        }
    } else {
        g_display_ptr_650e9c = g_display_ptr_650e8c;
        hit = 1;
    }

    if (g_display_ptr_650e98 != 0) {
        g_display_ptr_650e98->uiFlags = g_display_ptr_650e98->uiFlags & 0xfffffffe;
        if (g_display_ptr_650e98 != g_display_ptr_650e9c) {
            if (g_display_ptr_650e98->FastHelpText != 0) {
                g_display_ptr_650e98->uiFlags = g_display_ptr_650e98->uiFlags & 0xfffffeff;
                g_display_ptr_650e98->uiFlags = g_display_ptr_650e98->uiFlags & 0xfffffbff;
                HideRegionHelp();
            }
            g_display_ptr_650e9c->Cursor = g_word_5ff7c8;
            if ((g_display_ptr_650e98->uiFlags & 4) != 0
                && (g_display_ptr_650e98->uiFlags & 0x40) != 0) {
                (*g_display_ptr_650e98->MovementCallback)(g_display_ptr_650e98, 0x40);
            }
        }
    }

    if (!hit) {
        g_display_ptr_650e98 = 0;
        return;
    }

    node = g_display_ptr_650e9c;
    if (g_display_ptr_650e9c != g_display_ptr_650e98) {
        if ((g_display_ptr_650e9c->uiFlags & 4) != 0) {
            if (g_display_ptr_650e9c->FastHelpText != 0
                && (g_display_ptr_650e9c->uiFlags & 0x400) == 0) {
                g_display_ptr_650e9c->Cursor = g_word_5ff7c8;
                g_display_ptr_650e9c->uiFlags = g_display_ptr_650e9c->uiFlags & 0xfffffeff;
                g_display_ptr_650e9c->uiFlags = g_display_ptr_650e9c->uiFlags | 0x400;
                HideRegionHelp();
            }
            if ((g_display_ptr_650e9c->uiFlags & 0x40) != 0) {
                (*g_display_ptr_650e9c->MovementCallback)(g_display_ptr_650e9c, 0x80);
            }
        }
        if ((g_display_ptr_650e9c->uiFlags & 0x40) == 0
            || (g_display_ptr_650e9c->uiFlags & 2) == 0
            || g_display_ptr_650e9c->Cursor == -2) {
            child = g_display_ptr_650e9c->next;
            node = g_display_ptr_650e9c;
            while (child != 0 && !child_hit) {
                child_hit = 0;
                if ((child->uiFlags & 0x40) != 0
                    && child->RegionTopLeftX <= g_word_650e80
                    && child->RegionTopLeftY <= g_word_650e82
                    && g_word_650e80 <= child->RegionBottomRightX
                    && g_word_650e82 <= child->RegionBottomRightY
                    && (child->uiFlags & 2) != 0) {
                    child_hit = 1;
                    if (child->Cursor != -2) {
                        Function5A1140(child->Cursor);
                        node = g_display_ptr_650e9c;
                    }
                }
                child = child->next;
            }
        } else {
            Function5A1140(g_display_ptr_650e9c->Cursor);
            node = g_display_ptr_650e9c;
        }
    }

    if (g_display_flag_650e90 != 0 && g_display_id_6e4100 != node->IDNumber) {
        if ((node->uiFlags & 0x40) == 0) {
            g_display_ptr_650e98 = node;
            return;
        }
        if ((g_word_650e86 & 0x10) != 0) {
            g_display_flag_650e90 = 0;
        }
        if ((g_word_650e86 & 4) != 0) {
            g_display_flag_650e90 = 0;
        }
        node->uiFlags = node->uiFlags | 1;
        g_display_ptr_650e9c->MouseXPos = g_word_650e80;
        g_display_ptr_650e9c->MouseYPos = g_word_650e82;
        g_display_ptr_650e9c->RelativeXPos = g_word_650e80 - g_display_ptr_650e9c->RegionTopLeftX;
        g_display_ptr_650e9c->RelativeYPos = g_word_650e82 - g_display_ptr_650e9c->RegionTopLeftY;
        if ((g_display_ptr_650e9c->uiFlags & 4) != 0 && (g_word_650e86 & 1) != 0) {
            (*g_display_ptr_650e9c->MovementCallback)(g_display_ptr_650e9c, 2);
        }
        g_word_650e86 = g_word_650e86 & 0xfffe;
        g_display_ptr_650e98 = g_display_ptr_650e9c;
        return;
    }

    node->uiFlags = node->uiFlags | 1;
    g_display_ptr_650e9c->MouseXPos = g_word_650e80;
    g_display_ptr_650e9c->MouseYPos = g_word_650e82;
    g_display_ptr_650e9c->RelativeXPos = g_word_650e80 - g_display_ptr_650e9c->RegionTopLeftX;
    g_display_ptr_650e9c->RelativeYPos = g_word_650e82 - g_display_ptr_650e9c->RegionTopLeftY;
    g_display_ptr_650e9c->ButtonState = g_word_650e84;
    if ((g_display_ptr_650e9c->uiFlags & 0x40) != 0
        && (g_display_ptr_650e9c->uiFlags & 4) != 0
        && (g_word_650e86 & 1) != 0) {
        (*g_display_ptr_650e9c->MovementCallback)(g_display_ptr_650e9c, 2);
    }
    mask = g_word_650e86 & 0xfffe;
    if ((g_display_ptr_650e9c->uiFlags & 8) == 0
        || (g_word_650e86 & 0x7e) == 0
        || (g_display_ptr_650e9c->uiFlags & 0x40) == 0) {
        g_word_650e86 = mask & 0xff81;
        g_display_ptr_650e98 = g_display_ptr_650e9c;
        return;
    }

    events = 0;
    if ((g_word_650e86 & 2) != 0) {
        g_display_flag_650e90 = 1;
        g_display_id_6e4100 = g_display_ptr_650e9c->IDNumber;
        events = 4;
    }
    if ((g_word_650e86 & 4) != 0) {
        events = events | 8;
        g_display_flag_650e90 = 0;
    }
    if ((g_word_650e86 & 8) != 0) {
        g_display_flag_650e90 = 1;
        g_display_id_6e4100 = g_display_ptr_650e9c->IDNumber;
        events = events | 0x10;
    }
    if ((g_word_650e86 & 0x10) != 0) {
        events = events | 0x20;
        g_display_flag_650e90 = 0;
    }
    if ((g_word_650e86 & 0x20) != 0) {
        events = events | 0x100;
    }
    if ((g_word_650e86 & 0x40) != 0) {
        events = events | 0x200;
    }
    if (events == 0) {
        g_word_650e86 = mask & 0xff81;
        g_display_ptr_650e98 = g_display_ptr_650e9c;
        return;
    }

    g_word_650e86 = mask;
    if ((g_display_ptr_650e9c->uiFlags & 0x80) != 0) {
        g_display_ptr_650e9c->uiFlags = g_display_ptr_650e9c->uiFlags & 0xffffff7f;
        g_display_ptr_650e9c->uiFlags = g_display_ptr_650e9c->uiFlags & 0xfffffeff;
        g_display_ptr_650e9c->Cursor = g_word_5ff7c8;
        g_display_ptr_650e9c->uiFlags = g_display_ptr_650e9c->uiFlags & 0xfffffbff;
        HideRegionHelp();
    }
    if (events == 4) {
        now = GetClock();
        if (g_display_ptr_650e6c == g_display_ptr_650e9c
            && g_display_ptr_650e70 == g_display_ptr_650e9c
            && now <= g_time_650e74 + 400) {
            events = 0x404;
            g_display_ptr_650e6c = 0;
            g_display_ptr_650e70 = 0;
            g_time_650e74 = 0;
        } else {
            g_display_ptr_650e6c = g_display_ptr_650e9c;
            g_time_650e74 = GetClock();
        }
    } else if (events == 8) {
        now = GetClock();
        g_display_ptr_650e70 = g_display_ptr_650e9c;
        if (g_display_ptr_650e6c != g_display_ptr_650e9c || g_time_650e74 + 400 < now) {
            g_display_ptr_650e6c = 0;
            g_display_ptr_650e70 = 0;
            g_time_650e74 = 0;
        }
    }
    (*g_display_ptr_650e9c->ButtonCallback)(g_display_ptr_650e9c, events);
    g_word_650e86 = g_word_650e86 & 0xff81;
    g_display_ptr_650e98 = g_display_ptr_650e9c;
}

/*
 * Takes the cursor capture for a node, which is what makes 0x0040B900 skip its
 * search and send everything to 0x00650E8C.
 *
 * The result is three-valued rather than a success flag: 2 if no node carrying
 * this identifier is on the list, 1 if the capture was already held, and 0 if
 * this call took it. Only the last case writes anything, so a second capture
 * does not displace the first.
 */
// FUNCTION: WIZ8 0x0040bfc0
int Function40BFC0(MOUSE_REGION* node)
{
    MOUSE_REGION* scan;
    int found;

    found = 0;
    scan = g_display_head_650e94;
    if (scan != 0) {
        while (scan != 0 && !found) {
            if (scan->IDNumber == node->IDNumber) {
                found = 1;
            }
            scan = scan->next;
        }
        if (found) {
            if (g_byte_650e8a == 1) {
                return 1;
            }
            g_byte_650e8a = 1;
            g_display_ptr_650e8c = node;
            return 0;
        }
    }
    return 2;
}

}
