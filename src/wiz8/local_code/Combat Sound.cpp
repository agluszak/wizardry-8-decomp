#include "wiz8/virtual_file.h"
#include "FileMan.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

char* g_weapon_attack_sounds_68dd90[38];
char* g_material_impact_sounds_68d850[28][12];

static unsigned char ReadHitSoundLine(int handle, char* line,
                                      unsigned int capacity)
{
    unsigned int length = 0;
    unsigned int done;
    char value;

    while (length + 1 < capacity) {
        done = 0;
        if (!ReadVirtualFile(handle, &value, 1, &done) || done == 0) {
            line[length] = '\0';
            return length != 0;
        }
        if (value == '\n') {
            break;
        }
        if (value != '\r') {
            line[length++] = value;
        }
    }
    line[length] = '\0';
    return 1;
}

static void TrimHitSoundLine(char* line)
{
    char* comment = strchr(line, '*');
    size_t length;

    if (comment) {
        *comment = '\0';
    }
    length = strlen(line);
    while (length && isspace((unsigned char)line[length - 1])) {
        line[--length] = '\0';
    }
}

static char* DuplicateHitSound(const char* source)
{
    char* copy = static_cast<char*>(malloc(strlen(source) + 1));
    if (copy) {
        strcpy(copy, source);
    }
    return copy;
}

/* Local Code\\Combat Sound.cpp reads the attack list followed by twelve
   material columns of up to twenty-eight impact sounds.  A hash line advances
   the material column; an asterisk starts an inline comment. */
// FUNCTION: WIZ8 0x00549b00
extern "C" unsigned char LoadHitSoundDatabase(void)
{
    char path[] = "Data\\Databases\\HitSounds.txt";
    char line[256];
    int handle;
    int row = 0;
    int column = -1;

    memset(g_weapon_attack_sounds_68dd90, 0,
           sizeof(g_weapon_attack_sounds_68dd90));
    memset(g_material_impact_sounds_68d850, 0,
           sizeof(g_material_impact_sounds_68d850));
    handle = FileOpen(path, 0x41, 0);
    if (!handle) {
        return 0;
    }
    while (row < 38 && ReadHitSoundLine(handle, line, sizeof(line))) {
        char* marker;
        TrimHitSoundLine(line);
        marker = strchr(line, '#');
        if (marker) {
            *marker = '\0';
            TrimHitSoundLine(line);
        }
        if (line[0]) {
            g_weapon_attack_sounds_68dd90[row++] = DuplicateHitSound(line);
        }
    }
    if (row != 38) {
        CloseVirtualFile(handle);
        return 0;
    }
    row = 0;
    while (ReadHitSoundLine(handle, line, sizeof(line))) {
        TrimHitSoundLine(line);
        if (strchr(line, '#')) {
            ++column;
            row = 0;
            continue;
        }
        if (!line[0]) {
            continue;
        }
        if (column < 0 || column >= 12 || row >= 28) {
            CloseVirtualFile(handle);
            return 0;
        }
        g_material_impact_sounds_68d850[row++][column] =
            DuplicateHitSound(line);
    }
    CloseVirtualFile(handle);
    // Several impact materials intentionally provide one catch-all sound rather than one
    // entry per weapon class.  The retail loader accepts EOF after any valid final entry.
    return 1;
}

/* Free the two string tables populated by LoadHitSoundDatabase. */
// FUNCTION: WIZ8 0x00549e50
extern "C" void ReleaseHitSoundDatabase(void)
{
    int row;
    int column;

    for (row = 0; row < 38; ++row) {
        if (g_weapon_attack_sounds_68dd90[row]) {
            free(g_weapon_attack_sounds_68dd90[row]);
            g_weapon_attack_sounds_68dd90[row] = 0;
        }
    }
    for (column = 0; column < 12; ++column) {
        for (row = 0; row < 28; ++row) {
            if (g_material_impact_sounds_68d850[row][column]) {
                free(g_material_impact_sounds_68d850[row][column]);
                g_material_impact_sounds_68d850[row][column] = 0;
            }
        }
    }
}
