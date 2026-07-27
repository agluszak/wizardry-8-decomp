#include "wiz8/gameplay_boundaries.h"

/*
 * Marks a rectangle dirty on the current page.
 *
 * The screen is tracked as an 80x60 grid of eight-pixel cells - 0x50 cells per
 * row, which is 640/8 - and this walks the cells a rectangle covers, handing
 * each to 0x004259B0. A page already marked whole is skipped outright, and a
 * rectangle that turns out to cover the whole 640x480 marks it whole.
 *
 * Nothing here is named beyond that. The unit is not established and the flag
 * bits the caller passes are only known by which bits they set.
 */

extern "C" {

extern int g_page_index_6596e4;
extern unsigned char g_page_whole_6596e8[];

extern void Function4259B0(int cell, unsigned int flags);

// FUNCTION: WIZ8 0x00422D50
void Function422D50(int left, int top, int right, int bottom, int flags)
{
    unsigned char cell_flags;
    unsigned int clipped_left;
    unsigned int clipped_right;
    unsigned int x;

    cell_flags = 0;
    if (g_page_whole_6596e8[g_page_index_6596e4] == 0) {
        clipped_right = 0x280;
        /* The low clamp is a conditional expression because the original is
           branchless there and branches on the high one, and it is written
           <= 0 rather than < 1: the two are the same test and VC6 encodes them
           differently, setle against setl. */
        clipped_left = left <= 0 ? 0 : left;
        if ((int)clipped_left > 0x27f) {
            clipped_left = 0x280;
        }
        top = top <= 0 ? 0 : top;
        if (top > 0x1df) {
            top = 0x1e0;
        }
        /* Rounded up to the next cell boundary, with C's truncating division so
           a negative right edge collapses rather than wrapping. */
        right = ((right + 7) / 8) * 8;
        if (right <= 0 || right < 0x280) {
            clipped_right = right <= 0 ? 0 : right;
        }
        bottom = ((bottom + 7) / 8) * 8;
        if (bottom <= 0 || bottom < 0x1e0) {
            bottom = bottom <= 0 ? 0 : bottom;
        } else {
            bottom = 0x1e0;
        }
        if ((int)(clipped_right - clipped_left) > 0 && bottom - top > 0) {
            if (clipped_right - clipped_left == 0x280 && bottom - top == 0x1e0) {
                g_page_whole_6596e8[g_page_index_6596e4] = 1;
            }
            if (flags & 4) {
                cell_flags = 0x80;
            }
            if (flags & 1) {
                cell_flags = cell_flags | 2;
            }
            for (; top < bottom; top = top + 8) {
                if ((int)clipped_left < (int)clipped_right) {
                    x = clipped_left;
                    do {
                        Function4259B0((int)x / 8 + (top / 8) * 0x50, cell_flags);
                        x = x + 8;
                    } while ((int)x < (int)clipped_right);
                }
            }
        }
    }
}

}
