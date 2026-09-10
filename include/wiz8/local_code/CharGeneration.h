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
    unsigned char unknown_1b9[3];
    /* 0x1bc: the per-skill counterpart of attribute_baselines_048. The
       level-up reset zeroes it, and a profession change refunds whatever each
       entry holds before the new profession's skill points are assigned. */
    int skill_baselines_1bc[0x29];
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
void Function556EB0(W8Character*, W8CharacterCreationState*);
void Function557060(W8Character*, W8CharacterCreationState*, int);
void Function5571C0(W8Character*, W8CharacterCreationState*, int);
void Function5571E0(W8Character*, W8CharacterCreationState*, int);
void Function557200(W8Character*, W8CharacterCreationState*);
void DetermineEligibleProfessions(
    W8Character*, W8CharacterCreationState*, unsigned char*);
void Function557580(W8Character*, W8CharacterCreationState*, unsigned char);
void Function557730(W8Character*, W8CharacterCreationState*);
void Function557800(W8Character*, W8CharacterCreationState*);
void Function557890(W8Character*, W8CharacterCreationState*);
void Function5579E0(W8Character*, W8CharacterCreationState*, int, int);
void Function557AE0(W8Character*, W8CharacterCreationState*);
void Function557B20(W8Character*, W8CharacterCreationState*);
void Function557BC0(W8Character*, W8CharacterCreationState*, unsigned int, int);
void Function557C90(W8Character*, W8CharacterCreationState*, int);
void Function557D20(W8Character*, W8CharacterCreationState*, int);
void Function557D80(W8Character*, W8CharacterCreationState*);
void Function557EB0(W8Character*, W8CharacterCreationState*);
void Function557F90(W8Character*, W8CharacterCreationState*);
void Function558070(W8Character*, W8CharacterCreationState*);
int Function558180(W8Character*, W8CharacterCreationState*);
int Function558330(W8Character*, W8CharacterCreationState*);
void Function5584E0(W8Character*, W8CharacterCreationState*, unsigned int spell);
void Function558560(W8Character*, W8CharacterCreationState*, unsigned int spell);
void Function5585D0(W8Character*, W8CharacterCreationState*);
