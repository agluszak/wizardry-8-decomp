#include "wiz8/gameplay_boundaries.h"

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
 * 0x7fff, which is -32767 and 32767: a rectangle opened as wide as it goes,
 * not an empty bounding box, so they are read as a clip rectangle rather than
 * as accumulating extents.
 */

extern "C" {

typedef struct W8DisplayNode {
    short id;                       /* 0x00: matched against the word at 0x006E4100 */
    unsigned char byte_02;          /* 0x02: 0xff in the template */
    unsigned char pad_03;
    /* 0x04: the reset writes all four bytes at once while every test reads
       only the low one, so the member is wider than the flags it carries.
       0x10 selects a node for destruction, 0x80 wants the help hidden. */
    union {
        unsigned int all;
        unsigned char low;
    } flags;
    short clip_left;                /* 0x08 */
    short clip_top;                 /* 0x0a */
    short clip_right;               /* 0x0c */
    short clip_bottom;              /* 0x0e */
    short word_10;
    short word_12;
    short word_14;
    short word_16;
    short word_18;
    short word_1a;
    int dword_1c;
    int dword_20;
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
    int dword_48;
} W8DisplayNode;                    /* 0x4c */

extern void Function40B720(W8DisplayNode* node);
extern void Function40B830(W8DisplayNode* node);
extern void HideRegionHelp(void);

/* The buffer is not released with a direct call to free: the body loads a
   function pointer from 0x005EB224 once, before the loop, and calls through it
   for every node. */
extern void (*g_deallocator_5eb224)(void*);

extern W8DisplayNode g_display_template_5ff7d0;
extern W8DisplayNode* g_display_head_650e94;
extern W8DisplayNode* g_display_ptr_650e98;
extern W8DisplayNode* g_display_ptr_650e9c;
extern unsigned char g_display_flag_650ea0;
extern unsigned char g_display_flag_650e90;
extern short g_display_id_6e4100;

extern int g_dword_650e78;
extern int g_dword_650e7c;
extern short g_word_650e80;
extern short g_word_650e82;
extern short g_word_650e84;
extern short g_word_650e86;
extern unsigned char g_byte_650e88;
extern unsigned char g_byte_650e89;
extern unsigned char g_byte_650e8a;
extern int g_dword_650e8c;

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

    g_display_template_5ff7d0.clip_left = (short)0x8001;
    g_display_template_5ff7d0.clip_top = (short)0x8001;
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
    g_dword_650e8c = 0;
    g_display_template_5ff7d0.id = 0;
    g_display_template_5ff7d0.byte_02 = 0xff;
    g_display_template_5ff7d0.flags.all = 0x40;
    g_display_template_5ff7d0.clip_right = 0x7fff;
    g_display_template_5ff7d0.clip_bottom = 0x7fff;
    g_display_template_5ff7d0.word_10 = 0;
    g_display_template_5ff7d0.word_12 = 0;
    g_display_template_5ff7d0.word_14 = 0;
    g_display_template_5ff7d0.word_16 = 0;
    g_display_template_5ff7d0.word_18 = 0;
    g_display_template_5ff7d0.word_1a = 0;
    g_display_template_5ff7d0.dword_24 = 0;
    g_display_template_5ff7d0.dword_28 = 0;
    g_display_template_5ff7d0.dword_2c = 0;
    g_display_template_5ff7d0.dword_30 = 0;
    g_display_template_5ff7d0.dword_1c = 0;
    g_display_template_5ff7d0.dword_20 = 0;
    g_display_template_5ff7d0.word_34 = 0;
    g_display_template_5ff7d0.buffer = 0;
    g_display_template_5ff7d0.dword_3c = -1;
    g_display_template_5ff7d0.next = 0;
    g_display_template_5ff7d0.dword_48 = 0;
    Function40B720(&g_display_template_5ff7d0);
    g_byte_650e89 = 1;
    return 1;
}

}
