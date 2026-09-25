#ifndef WIZ8_LOCAL_CODE_NOISE_H
#define WIZ8_LOCAL_CODE_NOISE_H

template <class T> class srVector3T;

/* Local Code\Noise.cpp. */
void AlertMonsterGroupsToNoise(const srVector3T<float>* position, int radius, int flag);
void AlertWorldNoise(void);
void AlertCombatNoise(char large_radius);
/* radius - 25000 * hops - cost: whether a heard noise is still loud enough to
   be worth walking to once the path cost and hop count are paid for. */
int NoiseHearingMargin(int radius, int range, int hops);

#endif
