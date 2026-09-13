#ifndef WIZ8_LOCAL_CODE_NOISE_H
#define WIZ8_LOCAL_CODE_NOISE_H

template <class T> class srVector3T;

/* Local Code\Noise.cpp. */
void AlertMonsterGroupsToNoise004F0E80(const srVector3T<float>* position, int radius, int flag);
void AlertWorldNoise004F1100(void);
void AlertCombatNoise004F1150(char large_radius);

#endif
