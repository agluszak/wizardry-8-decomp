#pragma once

/* Local Screens\MGSRadarMap.cpp: the main-game radar overlay. RefreshRadarMap
   rebuilds the frame/map/compass sprites and restocks the eighteen sector
   blip pools; UpdateRadarBlips re-places world items, monsters and missiles
   through PlaceRadarBlip. The zoom trio picks between the detail and wide
   range presets. */
void EnableRadarMap(char enable); /* 0x005A20E0 */
void EnsureRadarMapOverlay(void); /* 0x005A2140 */
void ReleaseRadarMap(void);       /* 0x005A23E0 */
void RefreshRadarMap(void);       /* 0x005A24A0 */
void UpdateRadarBlips(void);      /* 0x005A2800 */
void ToggleRadarMapZoom(void);    /* 0x005A3360 */
void ZoomRadarMapIn(void);        /* 0x005A3410 */
void ZoomRadarMapOut(void);       /* 0x005A3470 */
