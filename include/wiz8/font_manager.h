#pragma once

#include "Font.h"

/* Wizardry uses SGP's font-manager representation: FontObjs holds the
   0xFC-byte objects 0x00406180 builds, FontDefault selects the print font,
   and FontDestBuffer/FontDestRegion/FontDestWrap select the print target.
   Retail's stateless accessors (StringPixLength, GetFontHeight,
   GetFontObject, GetFontObjectPalette16BPP, SetFont, SetFontDestBuffer,
   BltIsClipped) are byte-identical to the oracle (see
   build/reports/sgp/harness.csv) and link from Font.c; only the modified
   palette setter, the file-backed loader, and the two surface-aware
   printers stay first-party. The entry points' declarations stay in the
   pinned Font.h, whose signatures match. */

FontTranslationTable* CreateDefaultFontTranslationTable(void);
unsigned char InitializeWiz8FontManager(
    unsigned short code, FontTranslationTable* source);

struct W8FontLoadRequest;
struct W8ImageRecord;
struct W8StiHeader;

void* Function402B90(int target, unsigned int* pitch);
void Function402C30(int target);
void* Function406180(W8FontLoadRequest* request);
void* Function40F850(char* path, unsigned int mode);
unsigned char Function415130(void* record, unsigned int mode);
unsigned char Function415250(
    W8ImageRecord* record, unsigned int mode, int file, W8StiHeader* header);
unsigned char Function4153F0(
    W8ImageRecord* record, unsigned int mode, int file, W8StiHeader* header);
unsigned char Function410580(void* image, void* metrics);
unsigned short* Function410190(int table);
unsigned char Function40F9F0(void* resource);
unsigned char Function40FA10(void* resource, unsigned char mask);
void Function410620(unsigned short* pixels, int count);
void Function410670(unsigned short* pixels, int count);
void Function4106C0(unsigned short* pixels, int count);
void Function410700(unsigned short* pixels, int count);
unsigned int Function4104B0(int value);
