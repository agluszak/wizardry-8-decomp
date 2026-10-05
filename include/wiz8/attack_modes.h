#pragma once

/* The paired labels at 0x0061e9a8 select retail StringData labels 1311..1328.
   Each mode indexes a bit in the packed weapon/monster attack mask. NONE
   leaves the attack mode unselected; StartCharacterAttack chooses one then. */
enum W8AttackMode {
    W8_ATTACK_MODE_SWING = 0,
    W8_ATTACK_MODE_THRUST = 1,
    W8_ATTACK_MODE_BASH = 2,
    W8_ATTACK_MODE_BERSERK = 3,
    W8_ATTACK_MODE_THROW = 4,
    W8_ATTACK_MODE_PUNCH = 5,
    W8_ATTACK_MODE_KICK = 6,
    W8_ATTACK_MODE_LASH = 7,
    W8_ATTACK_MODE_SHOOT = 8,
    W8_ATTACK_MODE_NONE = -1,
    W8_ATTACK_MODE_COUNT = 9
};

/* Secondary physical-hit effects, in the order of the retail label table at
   0x0061e9cc (StringData 1330..1345). These index the sixteen packed chance
   bytes on item, monster-attack, missile and spell-effect records. */
enum W8AttackEffect {
    W8_ATTACK_EFFECT_SLEEP = 0,
    W8_ATTACK_EFFECT_PARALYZE = 1,
    W8_ATTACK_EFFECT_POISON = 2,
    W8_ATTACK_EFFECT_HEX = 3,
    W8_ATTACK_EFFECT_DISEASE = 4,
    W8_ATTACK_EFFECT_KILL = 5,
    W8_ATTACK_EFFECT_KNOCK_OUT = 6,
    W8_ATTACK_EFFECT_BLIND = 7,
    W8_ATTACK_EFFECT_FRIGHTEN = 8,
    W8_ATTACK_EFFECT_SWALLOW = 9,
    W8_ATTACK_EFFECT_POSSESS = 10,
    W8_ATTACK_EFFECT_DRAIN_HP = 11,
    W8_ATTACK_EFFECT_DRAIN_STAMINA = 12,
    W8_ATTACK_EFFECT_DRAIN_SP = 13,
    W8_ATTACK_EFFECT_NAUSEATE = 14,
    W8_ATTACK_EFFECT_INSANE = 15,
    W8_ATTACK_EFFECT_COUNT = 16
};
