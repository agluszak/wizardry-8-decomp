#ifndef WIZ8_LOCAL_CODE_MONSTER_GENERATOR_H
#define WIZ8_LOCAL_CODE_MONSTER_GENERATOR_H

#include "wiz8/geometry.h"

#pragma pack(push, 1)
/* What a generator hangs off itself. Both are released through slot zero of
   their own vtable with the deleting flag set, so both are polymorphic; nothing
   else about either is established. */
class W8MonsterGeneratorNode {
public:
    virtual ~W8MonsterGeneratorNode();
};

typedef struct W8MonsterGenerator {
    unsigned int flags;                   /* 0x00: bit 2 is cleared on teardown */
    unsigned char flag_04;                /* 0x04 */
    unsigned char unknown_05;
    unsigned short value_06;              /* 0x06 */
    unsigned short value_08;              /* 0x08 */
    unsigned char unknown_0a[2];
    /* 0x0c..0x14: three dwords, saved individually and handed to
       GenerateEncounter as a block. */
    int state_0c;
    int state_10;
    int state_14;
    W8MonsterGeneratorNode* node_18;      /* 0x18 */
    int value_1c;                         /* 0x1c */
    /* 0x20: m_pTimer, named by the MonGen.cpp:535 assertion, whose message also
       gives the owning class and method - "MonGen::Reset() out of memory
       allocating m_pTimer". */
    W8MonsterGeneratorNode* m_pTimer;
    char name[32];                        /* 0x24 */
    unsigned char flag_44;                /* 0x44: written to the save after the name */

#ifdef __cplusplus
    /* Named by the assertion message above. Rearms the generator's timer,
       creating it on first use, with a delay jittered around the configured
       interval. */
    void Reset();
    /* Arms or disarms the generator, loading its marker on the way in. */
    void SetActive(unsigned char active, W8MonsterGeneratorNode* node);
    /* The save pair. Both are __thiscall in the image. */
    void Save(int handle);
    unsigned char Load(int handle);
    /* Moves the generator, notifying the scene when it is armed. */
    void SetState(const srVector3T<float>* state);
    /* Loads the marker unconditionally, then applies the armed state. */
    void Reload(int unused, unsigned char active);
    ~W8MonsterGenerator();
#endif
} W8MonsterGenerator;
#pragma pack(pop)

W8MonsterGenerator* FindMonGenByName(const char* name);

#endif

