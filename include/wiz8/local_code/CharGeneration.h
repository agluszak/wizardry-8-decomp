#pragma once

struct W8Character;

/* CharGeneration.cpp's transient editing state, embedded by the Character
   screen and passed through its four pages and commit helpers. */
struct W8CharacterCreationState {
    int attribute_points_remaining;       /* 0x000 */
    int attribute_points_total;           /* 0x004 */
    int attribute_values_008[7];          /* 0x008 */
    int attribute_step_limit;             /* 0x024 */
    int attribute_limits_028[7];          /* 0x028 */
    unsigned char attributes_complete;    /* 0x044 */
    unsigned char unknown_045[3];
    int attribute_baselines_048[7];       /* 0x048 */
    int skill_points_remaining;           /* 0x064 */
    int skill_points_total;               /* 0x068 */
    int skill_points_spent[0x29];         /* 0x06c */
    int skill_step_limit;                 /* 0x110 */
    int skill_limits[0x29];               /* 0x114 */
    unsigned char skills_complete;        /* 0x1b8 */
    unsigned char unknown_1b9[0xa7];
    int spell_points_remaining;           /* 0x260 */
    int spell_points_total;               /* 0x264 */
    int magic_skill_bonus;                /* 0x268 */
    unsigned char spells_complete;        /* 0x26c */
    unsigned char unknown_26d[3];
};
static_assert(sizeof(W8CharacterCreationState) == 0x270,
              "W8CharacterCreationState_size");

void Function556DC0(W8Character*, W8CharacterCreationState*);
void Function556CC0(W8Character*, W8CharacterCreationState*);
void Function557580(W8Character*, W8CharacterCreationState*, unsigned char);
void Function557BC0(W8Character*, W8CharacterCreationState*, unsigned int, int);
void Function557F90(W8Character*, W8CharacterCreationState*);
void Function558180(W8Character*, W8CharacterCreationState*);
void Function5584E0(W8Character*, W8CharacterCreationState*, unsigned int spell);
void Function558560(W8Character*, W8CharacterCreationState*, unsigned int spell);
void Function5585D0(W8Character*, W8CharacterCreationState*);
