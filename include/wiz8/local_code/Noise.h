#ifndef WIZ8_LOCAL_CODE_NOISE_H
#define WIZ8_LOCAL_CODE_NOISE_H

template <class T> class srVector3T;

/* Local Code\Noise.cpp. The TU's only anchor is 0x004F0E80; its two gap
   neighbours 0x004F1100/0x004F1150 call it but stay unrecovered. */
void AlertMonsterGroupsToNoise004F0E80(const srVector3T<float>* position, int radius, int flag);

#endif
