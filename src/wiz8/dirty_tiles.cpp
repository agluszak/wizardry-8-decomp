#include "wiz8/gameplay_boundaries.h"
#include "wiz8/render_state.h"
#include "wiz8/surface2d.h"
#include "wiz8/wiz8_windows.h"
#include "surrender/srGERD.h"
#include "DirectDraw Calls.h"

class srNode;

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

extern unsigned int g_index_6596e4;
extern unsigned char g_flags_6596e8[2];
unsigned char g_flag_65970d;
unsigned char g_flag_6596ea;
/* The initial full-screen invalidation runs before any 2D node occupies the
   tile table.  Preserve that complete empty-slot path here; the non-empty path
   remains owned by the 2D-node recovery rather than pretending a partial
   release/recursive invalidation is complete. */
void Function4259B0(int cell, unsigned int flags)
{
    int left;
    int top;
    int right;
    int bottom;

    if (g_surface_nodes_654adc[cell]) {
        return;
    }
    g_block_652ddc[cell] |= static_cast<unsigned char>(flags | 0x40);
    ++g_dword_6596d8;
    top = (cell / 0x50) * 8;
    bottom = top + 8;
    left = (cell % 0x50) * 8;
    right = left + 8;
    if (g_flag_65970d
        && ((g_viewport_left_6595e8 <= left && left <= g_viewport_right_6595f0)
            || (g_viewport_left_6595e8 <= right && right <= g_viewport_right_6595f0))
        && ((g_viewport_top_6595ec <= top && top <= g_viewport_bottom_6595f4)
            || (g_viewport_top_6595ec <= bottom && bottom <= g_viewport_bottom_6595f4))) {
        g_block_652ddc[cell] |= 3;
        g_flag_6596ea = 1;
    }
}

// FUNCTION: WIZ8 0x00422D50
void MarkScreenRectDirty(int left, int top, int right, int bottom, int flags)
{
    unsigned char cell_flags;
    unsigned int clipped_left;
    unsigned int clipped_right;
    unsigned int x;

    cell_flags = 0;
    if (g_flags_6596e8[g_index_6596e4] == 0) {
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
                g_flags_6596e8[g_index_6596e4] = 1;
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

/* Coalesces dirty 8x8 cells into rectangular texture updates. Retail keeps the
   software surface locked for the complete batch and clears only the uploaded
   bit, preserving the lower per-cell state for the page lifecycle. */
// FUNCTION: WIZ8 0x00425B40
void Function425B40(void)
{
    DDSURFACEDESC description;

    if (g_dword_6596d8 == 0) {
        return;
    }
    DDLockSurface(g_primary_surface_6596a8, 0, &description, 0, 0);
    for (int row = 0; row != 60; ++row) {
        int column = 0;
        while (column < 80) {
            int cell = row * 80 + column;
            if ((g_block_652ddc[cell] & 0x40) == 0) {
                ++column;
                continue;
            }

            int width = 0;
            while (column + width < 80
                   && (g_block_652ddc[cell + width] & 0x40) != 0) {
                ++width;
            }
            int height = 0;
            while (row + height < 60
                   && (g_block_652ddc[cell + height * 80] & 0x40) != 0) {
                ++height;
            }

            g_surface_node_659664->updateRectangle(
                g_gerd_659634, description.lpSurface, description.lPitch,
                column * 8, row * 8,
                (column + width) * 8, (row + height) * 8);
            for (int y = 0; y != height; ++y) {
                for (int x = 0; x != width; ++x) {
                    g_block_652ddc[cell + y * 80 + x] &= 0x3f;
                }
            }
            column += width;
        }
    }
    DDUnlockSurface(g_primary_surface_6596a8, 0);
    g_dword_6596d8 = 0;
}

}
