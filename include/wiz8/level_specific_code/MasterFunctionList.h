#pragma once

#include "wiz8/vector.h"

struct W8MonsterInfo;
struct Trigger;

/* One level-specific per-frame entry point, registered by the level's own
   master function and run every world update. */
typedef void (*W8MasterFunction)(int);

/* The registered master functions, and the flag a callback sets to ask the
   dispatcher to drop it after this run. */
extern W8GrowableVector<W8MasterFunction>* g_master_functions_006834d8;
extern unsigned char g_flag_006834dc;

void ClearValue6834D4(void);
int NormalizeMasterFunctionValue004D9700(int value);
/* Run every registered master function once with argument zero, dropping the
   ones that set the removal flag while it runs. */
void RunMasterFunctions004D8E40(void);

void Function4D6C50(int level);
unsigned char Function4D9080(W8MonsterInfo* monster_info, int arg_2, int arg_3);

/* Master-function helpers installed by the per-level setup. */
void Function4D9740(void);
void Function4D9B40(void);
void Function4D9D30(void);
void Function4DA670(void);
void Function4DB200(void);
void Function4DBAB0(void);
void Function4DBE70(void);
void Function4DC8D0(void);
void Function4DCB50(void);
void Function4DEB40(void);
void Function4DF870(void);
void Function4E0510(void);
void Function4E06D0(void);
unsigned char Function521060(int a, int b, int c, int d, int e);

/* The level callbacks installed into Trigger::activation_callback_360. */
unsigned char Function4D9AC0(Trigger* trigger);
unsigned char Function4D9AD0(Trigger* trigger);
unsigned char Function4D9D70(Trigger* trigger);
unsigned char Function4D9DA0(Trigger* trigger);
unsigned char Function4D9DD0(Trigger* trigger);
unsigned char Function4D9E00(Trigger* trigger);
unsigned char Function4D9E60(Trigger* trigger);
unsigned char Function4DA060(Trigger* trigger);
unsigned char Function4DA110(Trigger* trigger);
unsigned char Function4DA2E0(Trigger* trigger);
unsigned char Function4DA3C0(Trigger* trigger);
unsigned char Function4DA460(Trigger* trigger);
unsigned char Function4DA590(Trigger* trigger);
unsigned char Function4DA5D0(Trigger* trigger);
unsigned char Function4DA610(Trigger* trigger);
unsigned char Function4DA630(Trigger* trigger);
unsigned char Function4DA650(Trigger* trigger);
unsigned char Function4DA740(Trigger* trigger);
unsigned char Function4DA960(Trigger* trigger);
unsigned char Function4DA9B0(Trigger* trigger);
unsigned char Function4DAA10(Trigger* trigger);
unsigned char Function4DADF0(Trigger* trigger);
unsigned char Function4DAE30(Trigger* trigger);
unsigned char Function4DAE70(Trigger* trigger);
unsigned char Function4DAEB0(Trigger* trigger);
unsigned char Function4DAEF0(Trigger* trigger);
unsigned char Function4DAF30(Trigger* trigger);
unsigned char Function4DAF70(Trigger* trigger);
unsigned char Function4DAFD0(Trigger* trigger);
unsigned char Function4DB090(Trigger* trigger);
unsigned char Function4DB120(Trigger* trigger);
unsigned char Function4DB160(Trigger* trigger);
unsigned char Function4DB180(Trigger* trigger);
unsigned char Function4DB1C0(Trigger* trigger);
unsigned char Function4DB380(Trigger* trigger);
unsigned char Function4DB420(Trigger* trigger);
unsigned char Function4DB460(Trigger* trigger);
unsigned char Function4DB4A0(Trigger* trigger);
unsigned char Function4DB650(Trigger* trigger);
unsigned char Function4DB690(Trigger* trigger);
unsigned char Function4DB6D0(Trigger* trigger);
unsigned char Function4DB770(Trigger* trigger);
unsigned char Function4DB810(Trigger* trigger);
unsigned char Function4DBA70(Trigger* trigger);
unsigned char Function4DBA90(Trigger* trigger);
unsigned char Function4DBB50(Trigger* trigger);
unsigned char Function4DBB90(Trigger* trigger);
unsigned char Function4DBBD0(Trigger* trigger);
unsigned char Function4DBE30(Trigger* trigger);
unsigned char Function4DBEC0(Trigger* trigger);
unsigned char Function4DC390(Trigger* trigger);
unsigned char Function4DC600(Trigger* trigger);
unsigned char Function4DC640(Trigger* trigger);
unsigned char Function4DC670(Trigger* trigger);
unsigned char Function4DC6B0(Trigger* trigger);
unsigned char Function4DC6E0(Trigger* trigger);
unsigned char Function4DC710(Trigger* trigger);
unsigned char Function4DC730(Trigger* trigger);
unsigned char Function4DC770(Trigger* trigger);
unsigned char Function4DC7A0(Trigger* trigger);
unsigned char Function4DC880(Trigger* trigger);
unsigned char Function4DC910(Trigger* trigger);
unsigned char Function4DCA20(Trigger* trigger);
unsigned char Function4DCA60(Trigger* trigger);
unsigned char Function4DCAC0(Trigger* trigger);
unsigned char Function4DCAF0(Trigger* trigger);
unsigned char Function4DCB10(Trigger* trigger);
unsigned char Function4DD3E0(Trigger* trigger);
unsigned char Function4DDD30(Trigger* trigger);
unsigned char Function4DDD50(Trigger* trigger);
unsigned char Function4DDD80(Trigger* trigger);
unsigned char Function4DDDC0(Trigger* trigger);
unsigned char Function4DDEB0(Trigger* trigger);
unsigned char Function4DDF20(Trigger* trigger);
unsigned char Function4DE520(Trigger* trigger);
unsigned char Function4DE620(Trigger* trigger);
unsigned char Function4DEDB0(Trigger* trigger);
unsigned char Function4DEE10(Trigger* trigger);
unsigned char Function4DEE50(Trigger* trigger);
unsigned char Function4DEEA0(Trigger* trigger);
unsigned char Function4DEEF0(Trigger* trigger);
unsigned char Function4DEFB0(Trigger* trigger);
unsigned char Function4DF120(Trigger* trigger);
unsigned char Function4DF4A0(Trigger* trigger);
unsigned char Function4DF540(Trigger* trigger);
unsigned char Function4DF560(Trigger* trigger);
unsigned char Function4DF580(Trigger* trigger);
unsigned char Function4DF5A0(Trigger* trigger);
unsigned char Function4DF710(Trigger* trigger);
unsigned char Function4DF7E0(Trigger* trigger);
unsigned char Function4DFE60(Trigger* trigger);
unsigned char Function4DFF90(Trigger* trigger);
unsigned char Function4DFFF0(Trigger* trigger);
unsigned char Function4E00B0(Trigger* trigger);
unsigned char Function4E0110(Trigger* trigger);
unsigned char Function4E01D0(Trigger* trigger);
unsigned char Function4E0230(Trigger* trigger);
unsigned char Function4E02F0(Trigger* trigger);
unsigned char Function4E0390(Trigger* trigger);
unsigned char Function4E04F0(Trigger* trigger);
unsigned char Function4E0560(Trigger* trigger);
unsigned char Function4E05A0(Trigger* trigger);
unsigned char Function4E05F0(Trigger* trigger);
unsigned char Function4E0880(Trigger* trigger);
unsigned char Function4E0A80(Trigger* trigger);
unsigned char Function4E0DC0(Trigger* trigger);
unsigned char Function4E1040(Trigger* trigger);
unsigned char Function4E10A0(Trigger* trigger);
unsigned char Function4E1120(Trigger* trigger);
unsigned char Function4E1180(Trigger* trigger);
unsigned char Function4E1340(Trigger* trigger);
unsigned char Function4E1740(Trigger* trigger);
unsigned char Function4E1930(Trigger* trigger);
unsigned char Function4E1970(Trigger* trigger);
unsigned char Function4E1DC0(Trigger* trigger);
unsigned char Function4E1F80(Trigger* trigger);
unsigned char Function4E2340(Trigger* trigger);
unsigned char Function4E2360(Trigger* trigger);
unsigned char Function4E23C0(Trigger* trigger);
unsigned char Function4E2420(Trigger* trigger);
unsigned char Function4E26F0(Trigger* trigger);
unsigned char Function4E2760(Trigger* trigger);
unsigned char TriggerArrowTrap(Trigger* trigger);
