#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"

#define TARGETING_CPP "C:\\Projects\\Wizardry 8\\Local Code\\Targeting.cpp"

/* BAD_INDEX is -1: the canonical assertions read "!= BAD_INDEX" where the
   bodies compare against -1. */
#define BAD_INDEX (-1)

// FUNCTION: WIZ8 0x0053BEA0
unsigned char TargetSourceIsCharacter(const W8TargetSource* source, int allow_indirect)
{
    if (source->iType == 1) {
        if (source->iChar == BAD_INDEX) {
            srAssertFail("pSource->iChar != BAD_INDEX", TARGETING_CPP, 0xce3, 0);
        }
        return 1;
    }
    if (allow_indirect == 1 && source->iType == 3 && source->iChar != BAD_INDEX) {
        if (!source->fBackfire && !source->fReflection) {
            srAssertFail("pSource->fBackfire || pSource->fReflection", TARGETING_CPP, 0xceb, 0);
        }
        return 1;
    }
    return 0;
}

// FUNCTION: WIZ8 0x0053BF10
unsigned char TargetSourceIsMonster(const W8TargetSource* source, int allow_indirect)
{
    if (source->iType == 2) {
        if (source->iMonsterID == BAD_INDEX) {
            srAssertFail("pSource->iMonsterID != BAD_INDEX", TARGETING_CPP, 0xcf8, 0);
        }
        return 1;
    }
    if (allow_indirect == 1 && source->iType == 3 && source->iMonsterID != BAD_INDEX) {
        if (!source->fBackfire && !source->fReflection) {
            srAssertFail("pSource->fBackfire || pSource->fReflection", TARGETING_CPP, 0xd00, 0);
        }
        return 1;
    }
    return 0;
}
