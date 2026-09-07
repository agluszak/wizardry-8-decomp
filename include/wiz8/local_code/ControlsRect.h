#pragma once


/* Shared declaration owner; implementation remains in Local Code\Controls.cpp. */

/* Same four-int layout as W8ScreenRect. Shared source identity is unproven;
   retain separate declarations until evidence connects their uses. */
struct W8ControlsRect {
    int left;                               /* 0x00 */
    int top;                                /* 0x04 */
    int right;                              /* 0x08 */
    int bottom;                             /* 0x0c */
};
