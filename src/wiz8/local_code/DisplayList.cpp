#include "wiz8/gameplay_boundaries.h"

#include <stdlib.h>
#include <string.h>

/*
 * Teardown for the display list headed at 0x00650E94, called by the main menu
 * entry on the way in.
 *
 * The node layout is not guessed. The destruction path frees a pointer at
 * +0x38, follows a link at +0x44, and finishes by clearing 0x4c bytes with a
 * rep stosd of 0x13 dwords, which fixes the size. The template at 0x005FF7D0
 * that the tail resets is then exactly 0x4c bytes wide and writes at the same
 * offsets - a pointer slot at +0x38 and a link at +0x44 - so it is a node, and
 * the two together give the field widths without any appeal to a caller.
 *
 * The pair of shorts at +0x08 and the pair at +0x0c are set to 0x8001 and
 * 0x7fff, which is -32767 and 32767. 0x0040B900 hit-tests the cursor against
 * them as left, top, right, bottom, so they are the node's rectangle and the
 * template is a catch-all that every position falls inside.
 */

extern "C" {

typedef struct W8DisplayNode {
    /* 0x00: the insert widens this with a zero extension before comparing it
       against the identifier counter, so it is unsigned. */
    unsigned short id;
    /* 0x02: the insert orders the list on this, descending, with a signed
       comparison. The template's -1 therefore sorts last. */
    signed char sort_key;
    unsigned char pad_03;
    /* 0x04: the reset writes all four bytes at once while every test reads
       only the low one, so the member is wider than the flags it carries.
       0x10 selects a node for destruction, 0x80 wants the help hidden. */
    union {
        unsigned int all;
        unsigned char low;
    } flags;
    short left;                     /* 0x08: 0x0040B900 hit-tests the cursor */
    short top;                      /* 0x0a */
    short right;                    /* 0x0c */
    short bottom;                   /* 0x0e */
    short cursor_x;                 /* 0x10: cursor position captured on a hit */
    short cursor_y;                 /* 0x12 */
    short offset_x;                 /* 0x14: cursor_x - left */
    short offset_y;                 /* 0x16: cursor_y - top */
    short word_18;                  /* 0x18: taken from the global at 0x00650E84 */
    short cursor_shape;             /* 0x1a: passed to 0x005A1140; -2 means none */
    /* 0x1c and 0x20: two callbacks, both invoked as (node, code). 0x0040B900
       sends state changes to the first and input events to the second. */
    void (*on_state)(struct W8DisplayNode*, int);
    void (*on_input)(struct W8DisplayNode*, unsigned int);
    int dword_24;
    int dword_28;
    int dword_2c;
    int dword_30;
    short word_34;
    short pad_36;                   /* 0x36: never written by the reset */
    void* buffer;                   /* 0x38: freed on destruction */
    int dword_3c;                   /* 0x3c: -1 in the template */
    int dword_40;                   /* 0x40: never written by the reset */
    struct W8DisplayNode* next;     /* 0x44 */
    struct W8DisplayNode* prev;     /* 0x48: 0x0040B830 unlinks through both directions */
} W8DisplayNode;                    /* 0x4c */



void Function40B830(W8DisplayNode* node);
extern void HideRegionHelp(void);
extern void Function5A1140(short shape);

short g_word_5ff7c8;
W8DisplayNode* g_display_ptr_650e6c;
W8DisplayNode* g_display_ptr_650e70;
unsigned int g_time_650e74;

/* The buffer is not released with a direct call to free: the body loads a
   function pointer from 0x005EB224 once, before the loop, and calls through it
   for every node. */
void (*g_deallocator_5eb224)(void*) = free;

W8DisplayNode g_display_template_5ff7d0;
W8DisplayNode* g_display_head_650e94;
W8DisplayNode* g_display_ptr_650e98;
W8DisplayNode* g_display_ptr_650e9c;
unsigned char g_display_flag_650ea0;
unsigned char g_display_flag_650e90;
unsigned short g_display_id_6e4100;

int g_dword_650e78;
int g_dword_650e7c;
short g_word_650e80;
short g_word_650e82;
short g_word_650e84;
unsigned short g_word_650e86;
unsigned char g_byte_650e88;
unsigned char g_byte_650e89;
unsigned char g_byte_650e8a;
W8DisplayNode* g_display_ptr_650e8c;

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
// FUNCTION: WIZ8 0x0040B720
void Function40B720(W8DisplayNode* node)
{
    W8DisplayNode* scan;
    W8DisplayNode* cursor;
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
                if (scan->id == candidate) {
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
    node->id = (short)assigned;
    node->next = 0;
    node->prev = 0;

    cursor = g_display_head_650e94;
    if (cursor != 0) {
        W8DisplayNode* ahead;
        W8DisplayNode* pick;

        found = 0;
        ahead = cursor->next;
        for (;;) {
            pick = ahead;
            if (pick == 0 || found) {
                break;
            }
            if (cursor->sort_key <= node->sort_key) {
                found = 1;
                pick = cursor;
            }
            ahead = pick->next;
            cursor = pick;
        }
        if (node->sort_key < cursor->sort_key) {
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
// FUNCTION: WIZ8 0x0040B830
void Function40B830(W8DisplayNode* node)
{
    W8DisplayNode* scan;
    int found;

    if (g_display_head_650e94 == 0) {
        return;
    }
    found = 0;
    scan = g_display_head_650e94;
    while (scan != 0 && !found) {
        if (scan->id == node->id) {
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

// FUNCTION: WIZ8 0x0040B290
int Function40B290(void)
{
    W8DisplayNode* node;
    void (*deallocate)(void*);

    node = g_display_head_650e94;
    deallocate = g_deallocator_5eb224;
    while (node != 0) {
        if ((node->flags.low & 0x10) != 0) {
            if (node == 0) {
                break;
            }
            if (node->buffer != 0) {
                if ((node->flags.low & 0x80) != 0) {
                    HideRegionHelp();
                }
                (*deallocate)(node->buffer);
            }
            node->buffer = 0;
            Function40B830(node);
            if (g_display_ptr_650e98 == node) {
                g_display_ptr_650e98 = 0;
            }
            if (g_display_ptr_650e9c == node) {
                g_display_ptr_650e9c = 0;
            }
            g_display_flag_650ea0 = 1;
            if (g_display_flag_650e90 != 0 && g_display_id_6e4100 == node->id) {
                g_display_flag_650e90 = 0;
            }
            memset(node, 0, sizeof(*node));
            node = g_display_head_650e94;
        } else {
            node = node->next;
            g_display_head_650e94 = node;
        }
    }

    g_display_template_5ff7d0.left = (short)0x8001;
    g_display_template_5ff7d0.top = (short)0x8001;
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
    g_display_template_5ff7d0.id = 0;
    g_display_template_5ff7d0.sort_key = -1;
    g_display_template_5ff7d0.flags.all = 0x40;
    g_display_template_5ff7d0.right = 0x7fff;
    g_display_template_5ff7d0.bottom = 0x7fff;
    g_display_template_5ff7d0.cursor_x = 0;
    g_display_template_5ff7d0.cursor_y = 0;
    g_display_template_5ff7d0.offset_x = 0;
    g_display_template_5ff7d0.offset_y = 0;
    g_display_template_5ff7d0.word_18 = 0;
    g_display_template_5ff7d0.cursor_shape = 0;
    g_display_template_5ff7d0.dword_24 = 0;
    g_display_template_5ff7d0.dword_28 = 0;
    g_display_template_5ff7d0.dword_2c = 0;
    g_display_template_5ff7d0.dword_30 = 0;
    g_display_template_5ff7d0.on_state = 0;
    g_display_template_5ff7d0.on_input = 0;
    g_display_template_5ff7d0.word_34 = 0;
    g_display_template_5ff7d0.buffer = 0;
    g_display_template_5ff7d0.dword_3c = -1;
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
// FUNCTION: WIZ8 0x0040B450
void Function40B450(void)
{
    W8DisplayNode* node;
    void (*deallocate)(void*);

    node = g_display_head_650e94;
    g_byte_650e88 = 0;
    g_byte_650e89 = 0;
    deallocate = g_deallocator_5eb224;
    while (node != 0) {
        if ((node->flags.low & 0x10) != 0) {
            g_display_head_650e94 = node;
            if (node->buffer != 0) {
                if ((node->flags.low & 0x80) != 0) {
                    HideRegionHelp();
                }
                (*deallocate)(node->buffer);
            }
            node->buffer = 0;
            Function40B830(node);
            if (g_display_ptr_650e98 == node) {
                g_display_ptr_650e98 = 0;
            }
            if (g_display_ptr_650e9c == node) {
                g_display_ptr_650e9c = 0;
            }
            g_display_flag_650ea0 = 1;
            if (g_display_flag_650e90 != 0 && g_display_id_6e4100 == node->id) {
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
// FUNCTION: WIZ8 0x0040B900
void Function40B900(void)
{
    W8DisplayNode* node;
    W8DisplayNode* child;
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
            if ((g_display_ptr_650e9c->flags.all & 0x840) != 0
                && g_display_ptr_650e9c->left <= g_word_650e80
                && g_display_ptr_650e9c->top <= g_word_650e82
                && g_word_650e80 <= g_display_ptr_650e9c->right
                && g_word_650e82 <= g_display_ptr_650e9c->bottom) {
                hit = 1;
                break;
            }
        }
    } else {
        g_display_ptr_650e9c = g_display_ptr_650e8c;
        hit = 1;
    }

    if (g_display_ptr_650e98 != 0) {
        g_display_ptr_650e98->flags.all = g_display_ptr_650e98->flags.all & 0xfffffffe;
        if (g_display_ptr_650e98 != g_display_ptr_650e9c) {
            if (g_display_ptr_650e98->buffer != 0) {
                g_display_ptr_650e98->flags.all = g_display_ptr_650e98->flags.all & 0xfffffeff;
                g_display_ptr_650e98->flags.all = g_display_ptr_650e98->flags.all & 0xfffffbff;
                HideRegionHelp();
            }
            g_display_ptr_650e9c->cursor_shape = g_word_5ff7c8;
            if ((g_display_ptr_650e98->flags.all & 4) != 0
                && (g_display_ptr_650e98->flags.all & 0x40) != 0) {
                (*g_display_ptr_650e98->on_state)(g_display_ptr_650e98, 0x40);
            }
        }
    }

    if (!hit) {
        g_display_ptr_650e98 = 0;
        return;
    }

    node = g_display_ptr_650e9c;
    if (g_display_ptr_650e9c != g_display_ptr_650e98) {
        if ((g_display_ptr_650e9c->flags.all & 4) != 0) {
            if (g_display_ptr_650e9c->buffer != 0
                && (g_display_ptr_650e9c->flags.all & 0x400) == 0) {
                g_display_ptr_650e9c->cursor_shape = g_word_5ff7c8;
                g_display_ptr_650e9c->flags.all = g_display_ptr_650e9c->flags.all & 0xfffffeff;
                g_display_ptr_650e9c->flags.all = g_display_ptr_650e9c->flags.all | 0x400;
                HideRegionHelp();
            }
            if ((g_display_ptr_650e9c->flags.low & 0x40) != 0) {
                (*g_display_ptr_650e9c->on_state)(g_display_ptr_650e9c, 0x80);
            }
        }
        if ((g_display_ptr_650e9c->flags.all & 0x40) == 0
            || (g_display_ptr_650e9c->flags.all & 2) == 0
            || g_display_ptr_650e9c->cursor_shape == -2) {
            child = g_display_ptr_650e9c->next;
            node = g_display_ptr_650e9c;
            while (child != 0 && !child_hit) {
                child_hit = 0;
                if ((child->flags.all & 0x40) != 0
                    && child->left <= g_word_650e80
                    && child->top <= g_word_650e82
                    && g_word_650e80 <= child->right
                    && g_word_650e82 <= child->bottom
                    && (child->flags.all & 2) != 0) {
                    child_hit = 1;
                    if (child->cursor_shape != -2) {
                        Function5A1140(child->cursor_shape);
                        node = g_display_ptr_650e9c;
                    }
                }
                child = child->next;
            }
        } else {
            Function5A1140(g_display_ptr_650e9c->cursor_shape);
            node = g_display_ptr_650e9c;
        }
    }

    if (g_display_flag_650e90 != 0 && g_display_id_6e4100 != node->id) {
        if ((node->flags.low & 0x40) == 0) {
            g_display_ptr_650e98 = node;
            return;
        }
        if ((g_word_650e86 & 0x10) != 0) {
            g_display_flag_650e90 = 0;
        }
        if ((g_word_650e86 & 4) != 0) {
            g_display_flag_650e90 = 0;
        }
        node->flags.all = node->flags.all | 1;
        g_display_ptr_650e9c->cursor_x = g_word_650e80;
        g_display_ptr_650e9c->cursor_y = g_word_650e82;
        g_display_ptr_650e9c->offset_x = g_word_650e80 - g_display_ptr_650e9c->left;
        g_display_ptr_650e9c->offset_y = g_word_650e82 - g_display_ptr_650e9c->top;
        if ((g_display_ptr_650e9c->flags.low & 4) != 0 && (g_word_650e86 & 1) != 0) {
            (*g_display_ptr_650e9c->on_state)(g_display_ptr_650e9c, 2);
        }
        g_word_650e86 = g_word_650e86 & 0xfffe;
        g_display_ptr_650e98 = g_display_ptr_650e9c;
        return;
    }

    node->flags.all = node->flags.all | 1;
    g_display_ptr_650e9c->cursor_x = g_word_650e80;
    g_display_ptr_650e9c->cursor_y = g_word_650e82;
    g_display_ptr_650e9c->offset_x = g_word_650e80 - g_display_ptr_650e9c->left;
    g_display_ptr_650e9c->offset_y = g_word_650e82 - g_display_ptr_650e9c->top;
    g_display_ptr_650e9c->word_18 = g_word_650e84;
    if ((g_display_ptr_650e9c->flags.all & 0x40) != 0
        && (g_display_ptr_650e9c->flags.all & 4) != 0
        && (g_word_650e86 & 1) != 0) {
        (*g_display_ptr_650e9c->on_state)(g_display_ptr_650e9c, 2);
    }
    mask = g_word_650e86 & 0xfffe;
    if ((g_display_ptr_650e9c->flags.all & 8) == 0
        || (g_word_650e86 & 0x7e) == 0
        || (g_display_ptr_650e9c->flags.all & 0x40) == 0) {
        g_word_650e86 = mask & 0xff81;
        g_display_ptr_650e98 = g_display_ptr_650e9c;
        return;
    }

    events = 0;
    if ((g_word_650e86 & 2) != 0) {
        g_display_flag_650e90 = 1;
        g_display_id_6e4100 = g_display_ptr_650e9c->id;
        events = 4;
    }
    if ((g_word_650e86 & 4) != 0) {
        events = events | 8;
        g_display_flag_650e90 = 0;
    }
    if ((g_word_650e86 & 8) != 0) {
        g_display_flag_650e90 = 1;
        g_display_id_6e4100 = g_display_ptr_650e9c->id;
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
    if ((g_display_ptr_650e9c->flags.all & 0x80) != 0) {
        g_display_ptr_650e9c->flags.all = g_display_ptr_650e9c->flags.all & 0xffffff7f;
        g_display_ptr_650e9c->flags.all = g_display_ptr_650e9c->flags.all & 0xfffffeff;
        g_display_ptr_650e9c->cursor_shape = g_word_5ff7c8;
        g_display_ptr_650e9c->flags.all = g_display_ptr_650e9c->flags.all & 0xfffffbff;
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
    (*g_display_ptr_650e9c->on_input)(g_display_ptr_650e9c, events);
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
// FUNCTION: WIZ8 0x0040BFC0
int Function40BFC0(W8DisplayNode* node)
{
    W8DisplayNode* scan;
    int found;

    found = 0;
    scan = g_display_head_650e94;
    if (scan != 0) {
        while (scan != 0 && !found) {
            if (scan->id == node->id) {
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
