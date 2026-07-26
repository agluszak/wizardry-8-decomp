#ifndef WIZ8_RECOVERED_WIZLIBS_H
#define WIZ8_RECOVERED_WIZLIBS_H

/*
 * The executable embeds six initialized SLF paths: Data, Sound,
 * MonsterSound, Music, Monsters, and Levels. The product-private source file
 * that defined their LibraryInitHeader array is not released with the SGP
 * source, so preserve only the count and declaration required by this unit.
 */
#define NUMBER_OF_LIBRARIES 6
extern LibraryInitHeader gGameLibaries[NUMBER_OF_LIBRARIES];

#endif
