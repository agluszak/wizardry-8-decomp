#pragma once

/* Shared declaration owner; implementation remains in Local Code\Controls.cpp. */

/* Same four-int left/top/right/bottom layout as W8ScreenRect and the same
   storage width as SGPRect (iLeft/iTop/iRight/iBottom). No recovered call
   site passes a Controls rect into an SGP or UtilityFunctions ScreenRect API
   without unpacking to scalars or copying fields, so shared source identity
   remains unproven; keep separate declarations. */
struct W8ControlsRect {
    int left;   /* 0x00 */
    int top;    /* 0x04 */
    int right;  /* 0x08 */
    int bottom; /* 0x0c */
};
