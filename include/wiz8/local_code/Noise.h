#ifndef WIZ8_LOCAL_CODE_NOISE_H
#define WIZ8_LOCAL_CODE_NOISE_H

template <class T> class srVector3T;

/* Local Code\Noise.cpp. */
void AlertMonsterGroupsToNoise004F0E80(const srVector3T<float>* position, int radius, int flag);
void AlertWorldNoise004F1100(void);
void AlertCombatNoise004F1150(char large_radius);
/* radius - 25000 * hops - cost: whether a heard noise is still loud enough to
   be worth walking to once the path cost and hop count are paid for. */
int Function4F0E50(int radius, int cost, int hops); /* 0x004F0E50 */

#endif
