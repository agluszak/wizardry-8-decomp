#pragma once

/* The six radar blip classes. Remembered monsters that cannot be rendered use
   the gray class; items and missiles use the final two. */
enum W8RadarBlipClass {
    W8_RADAR_BLIP_NEUTRAL = 0,
    W8_RADAR_BLIP_HOSTILE = 1,
    W8_RADAR_BLIP_FRIENDLY = 2,
    W8_RADAR_BLIP_LAST_SEEN = 3,
    W8_RADAR_BLIP_ITEM = 4,
    W8_RADAR_BLIP_MISSILE = 5,
    W8_RADAR_BLIP_CLASS_COUNT = 6
};

void EnableRadarMap(bool enable);
void EnsureRadarMapOverlay(void);
void ReleaseRadarMap(void);
void RefreshRadarMap(void);
void UpdateRadarBlips(void);
void ToggleRadarMapZoom(void);
void ZoomRadarMapIn(void);
void ZoomRadarMapOut(void);
