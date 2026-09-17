#include "wiz8/level_specific_code/Rift1.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/fact_state.h"
#include "wiz8/level_specific_code/MasterFunctionList.h"
#include "wiz8/location_variables.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "surrender/srMath.h"

#define RIFT1_CPP "C:\\Projects\\Wizardry 8\\Level Specific Code\\Rift1.cpp"

/* Level Specific Code\Rift1.cpp (level 0x15).

   Attribution evidence: 0x004DAFD0 passes this file's path string to
   MonsterGetIndexByLocationID. The level-0x15 block in
   InitializeLevelMasterFunctions004D6C50 registers the surrounding cluster
   (Fireantspawn, Sexspawn, Hotstuff, Gate, AshLock, TimeDorado). */

/* "Sexspawn": on the first activation after fact 0x15f, mark the
   Al-Sedexus swap, remove the original Al-Sedexus (species 0xd9) and spawn
   the replacement (species 0x22b) at the Sexspawn entity if it is not already
   in the level. Returns false unless the swap runs, so the trigger stays
   armed until it does. */
// FUNCTION: WIZ8 0x004DAFD0
bool Rift1Sexspawn004DAFD0(Trigger* pTrigger)
{
    srVector3T<float> position;
    W8MonsterInfo* monster_info;

    if (GetFact(0x15f) != 0) {
        if (GetLocationVarIDByName("AlSedexusSwapped") == -1) {
            CreateLocationVar("AlSedexusSwapped", 1);
            monster_info = FindMonsterInfoBySpecies(0xd9);
            if (monster_info != 0) {
                RemoveMonster(
                    MonsterGetIndexByLocationID(0x42, RIFT1_CPP, monster_info->location_id, 1), 1);
            }
            monster_info = FindMonsterInfoBySpecies(0x22b);
            if (monster_info == 0 && FindEntityByName("Sexspawn", &position, 0, 0)) {
                SpawnMonsters(0x22b, 1, &position, 0, 1, 0, 0);
            }
            return true;
        }
    }
    return false;
}

/* "Hotstuff": until fact 0x326, spawn the lava lord (0x175) at the Hotstuff
   entity, give it the MoveLavalord movement script and begin the scripted
   world action. */
// FUNCTION: WIZ8 0x004DB090
bool Rift1Hotstuff004DB090(Trigger* pTrigger)
{
    srVector3T<float> position;
    W8MonsterGroup* group;
    W8Monster* monster;

    if (GetFact(0x326) == 0) {
        if (FindEntityByName("Hotstuff", &position, 0, 0)) {
            group = SpawnMonsters(0x175, 1, &position, 0, 1, 0, 0);
            if (group != 0) {
                monster = GetMonsterByLocationID(IListGetAt(group->monsters, 0));
                if (monster != 0 && monster->SetScript004C7F10("MoveLavalord.MSF", 1) != 0) {
                    BeginScriptedWorldAction();
                }
            }
        }
    }
    return true;
}

/* "Fireantspawn": spawn a hostile group of six fire ants (0x12b) at the
   Fireantspawn entity. */
// FUNCTION: WIZ8 0x004DB120
bool Rift1Fireantspawn004DB120(Trigger* pTrigger)
{
    srVector3T<float> position;

    if (FindEntityByName("Fireantspawn", &position, 0, 0)) {
        SpawnMonsters(0x12b, 6, &position, 1, 0, 0, 0);
    }
    return true;
}

/* "Gate": raise fact 0x1c4. */
// FUNCTION: WIZ8 0x004DB160
bool Rift1Gate004DB160(Trigger* pTrigger)
{
    SetFact(0x1c4, 1, 0);
    return true;
}

/* "AshLock": spawn one hostile monster 0x222 at the NP_Hotstuff3 entity. */
// FUNCTION: WIZ8 0x004DB180
bool Rift1AshLock004DB180(Trigger* pTrigger)
{
    srVector3T<float> position;

    if (FindEntityByName("NP_Hotstuff3", &position, 0, 0)) {
        SpawnMonsters(0x222, 1, &position, 1, 1, 0, 0);
    }
    return true;
}

/* "TimeDorado": give every existing monster group of species 0xfc a fresh
   member. */
// FUNCTION: WIZ8 0x004DB1C0
bool Rift1TimeDorado004DB1C0(Trigger* pTrigger)
{
    W8MonsterGroup* previous;

    previous = FindNextExistingMonsterByID(0xfc, 0);
    while (previous != 0) {
        GiveBirthToMonster(previous);
        previous = FindNextExistingMonsterByID(0xfc, previous);
    }
    return true;
}
