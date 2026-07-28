/*
 * Engine Code\Missile.cpp.
 *
 * What a missile is fired from and where it comes out of. A missile holds a
 * launcher record at 0x1dc; the record carries a small table of emitters at
 * 0xd8 and an index into it at 0xa4, and the accessors below read the chosen
 * emitter, count how many the record has, and reach the emitter's own value.
 */

#include "wiz8/engine_code/Emitter.h"

class W8Missile {
public:
    virtual ~W8Missile();

    W8EmitterHost* GetLauncher();    /* 0x004A45E0 */
    W8Emitter* GetActiveEmitter();/* 0x004A4570 */
    float GetActiveEmitterValue();       /* 0x004A45C0 */
    char GetEmitterCount();              /* 0x004A4590 */
    void Release();                      /* 0x004A4100 */
    virtual void* Method34();
    virtual char Method14();
    void ApplyLauncherSetting98();       /* 0x004A4110 */
    void ResetLauncherCounters(int arg_1, int arg_2);   /* 0x004A4140 */

    unsigned char unknown_004[0x1d8];
    W8EmitterHost* launcher;             /* 0x1dc */
};

extern void ReleaseMissile(W8Missile* missile);                          /* 0x004A7470 */
extern void Function4A15D0(void* node, unsigned char value);
extern void Function4A72F0(int arg_1, int arg_2);

/* The launcher a missile was fired from. */
// FUNCTION: WIZ8 0x004A45E0
W8EmitterHost* W8Missile::GetLauncher()
{
    return this->launcher;
}

/* The emitter the launcher is currently firing from. */
// FUNCTION: WIZ8 0x004A4570
W8Emitter* W8Missile::GetActiveEmitter()
{
    return this->launcher->emitters[this->launcher->emitter_index];
}

/* That emitter's own value. */
// FUNCTION: WIZ8 0x004A45C0
float W8Missile::GetActiveEmitterValue()
{
    return this->launcher->emitters[this->launcher->emitter_index]->value_08;
}

/* How many emitters the launcher has, counted by testing each for null rather
   than read from a stored count. */
// FUNCTION: WIZ8 0x004A4590
char W8Missile::GetEmitterCount()
{
    char count = this->launcher->emitters[0] != 0;

    if (this->launcher->emitters[1] != 0) {
        ++count;
    }
    return count;
}

/* Thirteen-byte forwarder onto the release path. */
// FUNCTION: WIZ8 0x004A4100
void W8Missile::Release()
{
    ReleaseMissile(this);
}

/* Hand the launcher's setting at 0x98 to whatever the missile's own virtual
   accessor answers with. */
// FUNCTION: WIZ8 0x004A4110
void W8Missile::ApplyLauncherSetting98()
{
    Function4A15D0(Method34(), this->launcher->setting_98);
}

/* Reset the launcher's two counters at 0x94 and 0x95. The second is one less
   than the missile's own virtual answer, and the launcher pointer is taken
   before the virtual call rather than after. */
// FUNCTION: WIZ8 0x004A4140
void W8Missile::ResetLauncherCounters(int arg_1, int arg_2)
{
    W8EmitterHost* launcher_before;

    *(unsigned char*)((char*)this->launcher + 0x94) = 0;
    launcher_before = this->launcher;
    *(char*)((char*)launcher_before + 0x95) = Method14() - 1;
    Function4A72F0(arg_1, arg_2);
}
