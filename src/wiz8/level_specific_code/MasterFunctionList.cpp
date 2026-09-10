#include "wiz8/level_specific_code/MasterFunctionList.h"
#include "wiz8/fact_state.h"
#include "wiz8/engine_code/Trigger.h"
#include "wiz8/sr_api.h"
#include "wiz8/screen_state.h"
#include "wiz8/local_screens/MGSSpellCasting.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/utility.h"

extern "C" int g_value_6834d4;
extern "C" {
// GLOBAL: WIZ8 0x006834d4
int g_value_6834d4;
}

// GLOBAL: WIZ8 0x006834d8
W8GrowableVector<W8MasterFunction>* g_master_functions_006834d8;
// GLOBAL: WIZ8 0x006834dc
unsigned char g_flag_006834dc;

/* Run every registered master function once with argument zero, dropping the
   ones that set the removal flag while it runs. */
// FUNCTION: WIZ8 0x004D8E40
void RunMasterFunctions004D8E40(void)
{
    int count = g_master_functions_006834d8->GetCount();

    for (int index = 0; index < count; ++index) {
        (*g_master_functions_006834d8->GetAt(index))(0);
        if (g_flag_006834dc != 0) {
            g_master_functions_006834d8->RemoveAt(index);
            --count;
            --index;
        }
    }
}

/* Predicate installed in the master-function callback table. */
// FUNCTION: WIZ8 0x004D95F0
unsigned char IsMasterFunctionTypeEight004D95F0(int type)
{
    return type == 8;
}

// FUNCTION: WIZ8 0x004D96F0
void ClearValue6834D4(void)
{
    g_value_6834d4 = 0;
}

/* Master-function values 0x10 and 0x26 select the same path once either of
   the two enabling facts has been set. */
// FUNCTION: WIZ8 0x004D9700
int NormalizeMasterFunctionValue004D9700(int value)
{
    if (value == 0x10 || value == 0x26) {
        if (GetFact(0x5b) == 0 && GetFact(0x1ce) == 0) {
            return 0x26;
        }
        value = 0x10;
    }
    return value;
}


#define MASTER_FUNCTION_CPP \
    "C:\\Projects\\Wizardry 8\\Level Specific Code\\MasterFunctionList.cpp"

// GLOBAL: WIZ8 0x006834DD
unsigned char g_flag_006834dd;
// GLOBAL: WIZ8 0x006109F0
unsigned char g_flag_6109f0;
// GLOBAL: WIZ8 0x006834E0
int g_value_6834e0;
// GLOBAL: WIZ8 0x00652DA5
unsigned char g_flag_652da5;

/* Install the level's trigger callbacks and master-function helpers. */
// FUNCTION: WIZ8 0x004D6C50
void Function4D6C50(int level)
{
    Trigger* pTrigger;

    if (g_master_functions_006834d8 == 0) {
        g_master_functions_006834d8 = new W8GrowableVector<W8MasterFunction>(5);
    }
    else {
        while (g_master_functions_006834d8->GetCount() > 0) {
            g_master_functions_006834d8->RemoveAt(0);
        }
    }
    for (int index = g_master_functions_006834d8->GetCount() - 1;
         index >= 0;
         --index) {
        void* entry = reinterpret_cast<void*>(g_master_functions_006834d8->RemoveAt(index)); /* reinterpret-ok: function entry stored as data */
        ::operator delete(entry);
    }
    g_flag_006834dd = 0;
    g_flag_6109f0 = 1;
    g_value_6834e0 = level;
    g_flag_652da5 = 0;
    switch (level) {
    case 0:
        pTrigger = FindTriggerByName("ChaosMolori");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x74, "Missing trigger 'ChaosMolori'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4E2340;
        pTrigger = FindTriggerByName("Maddmook");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x78, "Missing trigger 'Maddmook'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4E2360;
        pTrigger = FindTriggerByName("CMbox");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x7c, "Missing trigger 'CMbox'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4E2420;
        pTrigger = FindTriggerByName("AstralDominae");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x80, "Missing trigger 'AstralDominae'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4E23C0;
        pTrigger = FindTriggerByName("BallSlot");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x84, "Missing trigger 'BallSlot'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4E26F0;
        pTrigger = FindTriggerByName("Flightrecordertrigger");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x88, "Missing trigger 'Flightrecordertrigger'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4E2760;
        pTrigger = FindTriggerByName("ULLspawn");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x8c, "Missing trigger 'ULLspawn'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = (Trigger::ActivationCallback)ScreenLifecycleSuccess;
        pTrigger = FindTriggerByName("Mookholo");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x90, "Missing trigger 'Mookholo'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4E0DC0;
        pTrigger = FindTriggerByName("MookFrontDoor");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x9c, "Missing trigger 'MookFrontDoor'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4E1040;
        pTrigger = FindTriggerByName("YellowButton");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0xa0, "Missing trigger 'YellowButton'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4E10A0;
        pTrigger = FindTriggerByName("Vaultalarmdoor");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0xa4, "Missing trigger 'Vaultalarmdoor'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4E1120;
        pTrigger = FindTriggerByName("Exitbutton");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0xa8, "Missing trigger 'Exitbutton'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4E1180;
        pTrigger = FindTriggerByName("GenVault-2-door");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0xac, "Missing trigger 'GenVault-2-door'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4E1340;
        pTrigger = FindTriggerByName("ARN11");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0xb8, "Missing trigger 'ARN11'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4D9AC0;
        pTrigger = FindTriggerByName("RedButton");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0xbd, "Missing trigger 'RedButton'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4E1740;
        pTrigger = FindTriggerByName("El1-TopButtons");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0xc1, "Missing trigger 'El1-TopButtons'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4E1930;
        pTrigger = FindTriggerByName("El1-BottomButtons");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0xc5, "Missing trigger 'El1-BottomButtons'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4E1970;
        pTrigger = FindTriggerByName("GreenButton");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0xca, "Missing trigger 'GreenButton'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4E1DC0;
        pTrigger = FindTriggerByName("Elevator-02");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0xce, "Missing trigger 'Elevator-02'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4E1F80;
        pTrigger = FindTriggerByName("LazerScanner");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0xd8, "Missing trigger 'LazerScanner'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4E0880;
        pTrigger = FindTriggerByName("ScannerDoor");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0xdc, "Missing trigger 'ScannerDoor'! It's not in the LVL file!");
        *reinterpret_cast<unsigned int*>((unsigned char*)pTrigger + 0xbc) = 0; /* reinterpret-ok: unresolved base-class field */
        pTrigger->activation_callback_360 = Function4E0A80;
        Function4E06D0();
        return;
    case 1:
        pTrigger = FindTriggerByName("RampUp");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x116, "Missing trigger 'RampUp'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DFE60;
        pTrigger = FindTriggerByName("ChaosATrigger");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x11a, "Missing trigger 'ChaosATrigger'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DFFF0;
        pTrigger = FindTriggerByName("ChaosBTrigger");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x11e, "Missing trigger 'ChaosBTrigger'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DFF90;
        pTrigger = FindTriggerByName("LifeATrigger");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x122, "Missing trigger 'LifeATrigger'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4E0110;
        pTrigger = FindTriggerByName("LifeBTrigger");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x126, "Missing trigger 'LifeBTrigger'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4E00B0;
        pTrigger = FindTriggerByName("KnowATrigger");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x12a, "Missing trigger 'KnowATrigger'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4E0230;
        pTrigger = FindTriggerByName("KnowBTrigger");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x12e, "Missing trigger 'KnowBTrigger'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4E01D0;
        pTrigger = FindTriggerByName("RampUp");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x132, "Missing trigger 'RampUp'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4E02F0;
        pTrigger = FindTriggerByName("Path1Camera");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x136, "Missing trigger 'Path1Camera'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4E0390;
        pTrigger = FindTriggerByName("Shaker");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x5ee, FormatString("Missing trigger '%s'! It's not in the LVL file!", "Shaker"));
        pTrigger->activation_callback_360 = Function4E04F0;
        Function4DF870();
        return;
    case 4:
        Function4D9B40();
        pTrigger = FindTriggerByName("CC_TRIGGERPLANE1HEDRA");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x33a, "Missing trigger 'CC_TRIGGERPLANE1HEDRA'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4D9AD0;
        break;
    case 5:
        Function4DEB40();
        pTrigger = FindTriggerByName("MR109");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x144, "Missing trigger 'MR109'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DF4A0;
        pTrigger = FindTriggerByName("MR110");
        if (pTrigger != 0) {
        pTrigger->activation_callback_360 = (Trigger::ActivationCallback)Function5A1140;
    }
        pTrigger = FindTriggerByName("MR111");
        if (pTrigger != 0) {
        pTrigger->activation_callback_360 = (Trigger::ActivationCallback)Function5A1140;
    }
        pTrigger = FindTriggerByName("MR112");
        if (pTrigger != 0) {
        pTrigger->activation_callback_360 = (Trigger::ActivationCallback)Function5A1140;
    }
        pTrigger = FindTriggerByName("F-Handlock");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x154, "Missing trigger 'F-Handlock'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DEDB0;
        pTrigger = FindTriggerByName("ButtonGigas");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x158, "Missing trigger 'ButtonGigas'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DF540;
        pTrigger = FindTriggerByName("ButtonTrang");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x15c, "Missing trigger 'ButtonTrang'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DF560;
        pTrigger = FindTriggerByName("ButtonRift");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x160, "Missing trigger 'ButtonRift'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DF580;
        pTrigger = FindTriggerByName("ButtonMaten");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x164, "Missing trigger 'ButtonMaten'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DF5A0;
        pTrigger = FindTriggerByName("WireTrigger");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x168, "Missing trigger 'WireTrigger'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DF710;
        pTrigger = FindTriggerByName("ButtonGigas");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x16c, "Missing trigger 'ButtonGigas'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DF540;
        pTrigger = FindTriggerByName("Controller");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x170, "Missing trigger 'Controller'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DF120;
        pTrigger = FindTriggerByName("Dial-A");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x174, "Missing trigger 'Dial-A'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DEE10;
        pTrigger = FindTriggerByName("Dial-B");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x178, "Missing trigger 'Dial-B'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DEE50;
        pTrigger = FindTriggerByName("Dial-C");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x17c, "Missing trigger 'Dial-C'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DEEA0;
        pTrigger = FindTriggerByName("Gas-Switch");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x180, "Missing trigger 'Gas-Switch'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DEEF0;
        pTrigger = FindTriggerByName("J-Doorcontroller");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x184, "Missing trigger 'J-Doorcontroller'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DEFB0;
        pTrigger = FindTriggerByName("MartenBook");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x188, "Missing trigger 'trigger16254'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DF7E0;
        return;
    case 6:
        Function4DCB50();
        pTrigger = FindTriggerByName("Arrowtraptrigger");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x192, "Missing trigger 'Arrowtraptrigger'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = TriggerArrowTrap;
        pTrigger = FindTriggerByName("Spikeballtrigger");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x196, "Missing trigger 'Spikeballtrigger'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DD3E0;
        pTrigger = FindTriggerByName("DoorBolt");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x19a, "Missing trigger 'DoorBolt'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DDD30;
        pTrigger = FindTriggerByName("DummyLever");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x19e, "Missing trigger 'DummyLever'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DDD50;
        pTrigger = FindTriggerByName("Dummy");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x1a2, "Missing trigger 'Dummy'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DDD80;
        pTrigger = FindTriggerByName("PerfumeBox");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x1a6, "Missing trigger 'PerfumeBox'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DDDC0;
        pTrigger = FindTriggerByName("StoneIdol");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x1aa, "Missing trigger 'StoneIdol'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DE520;
        pTrigger = FindTriggerByName("BlueFlowers");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x1ae, "Missing trigger 'BlueFlowers'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DE620;
        pTrigger = FindTriggerByName("SquisherControls");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x1b2, "Missing trigger 'SquisherControls'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DDF20;
        pTrigger = FindTriggerByName("DoorControls");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x1b6, "Missing trigger 'DoorControls'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DDEB0;
        return;
    case 8:
        Function4DC8D0();
        pTrigger = FindTriggerByName("roach_trigger");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x1c0, "Missing trigger 'roach_trigger'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DC910;
        pTrigger = FindTriggerByName("spider_trigger");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x1c4, "Missing trigger 'spider_trigger'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DCA20;
        pTrigger = FindTriggerByName("Bartrigger");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x1c8, "Missing trigger 'Bartrigger'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DCAF0;
        pTrigger = FindTriggerByName("Coffinlide");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x1cc, "Missing trigger 'Coffinlide'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DCA60;
        pTrigger = FindTriggerByName("Coffinlidg");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x1d0, "Missing trigger 'Coffinlidg'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DCAC0;
        pTrigger = FindTriggerByName("wheel_star");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x5ee, FormatString("Missing trigger '%s'! It's not in the LVL file!", "wheel_star"));
        pTrigger->activation_callback_360 = Function4DCB10;
        return;
    case 9:
        pTrigger = FindTriggerByName("bell_button");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x1d8, "Missing trigger 'bell_button'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DC7A0;
        pTrigger = FindTriggerByName("micro_door2");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x1dc, "Missing trigger 'micro_door2'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DC880;
        return;
    case 0xc:
        Function4DBAB0();
        pTrigger = FindTriggerByName("_VOC_EWAXXLIFT1");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x219, "Missing trigger '_VOC_EWAXXLIFT1'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DBB50;
        pTrigger = FindTriggerByName("_VOC_EWAXXLIFT2");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x21d, "Missing trigger '_VOC_EWAXXLIFT2'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DBB90;
        pTrigger = FindTriggerByName("PRESSUREPLATE");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x221, "Missing trigger '_VOC_EWAXXLIFT2'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DBBD0;
        pTrigger = FindTriggerByName("mudWallTrigger");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x225, "Missing trigger 'mudWallTrigger'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DBE30;
        return;
    case 0xd:
        Function4DB200();
        pTrigger = FindTriggerByName("_VOC_EWAXXTRAIN");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x22e, "Missing trigger '_VOC_EWAXXTRAIN'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DB380;
        pTrigger = FindTriggerByName("redwire");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x232, "Missing trigger 'redwire'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DB420;
        pTrigger = FindTriggerByName("bluewire");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x236, "Missing trigger 'bluewire'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DB460;
        pTrigger = FindTriggerByName("yellowwire");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x23a, "Missing trigger 'yellowwire'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DB4A0;
        pTrigger = FindTriggerByName("_VOC_EWAXXLIFT3");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x23e, "Missing trigger '_VOC_EWAXXLIFT3'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DB650;
        pTrigger = FindTriggerByName("_VOC_EWAXXTOPDOOR1");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x242, "Missing trigger '_VOC_EWAXXTOPDOOR1'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DB690;
        pTrigger = FindTriggerByName("_VOC_EWAXXOFFICER1");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x246, "Missing trigger '_VOC_EWAXXTOPDOOR1'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DB6D0;
        pTrigger = FindTriggerByName("_VOC_EWAXXOFFICER2");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x24a, "Missing trigger '_VOC_EWAXXTOPDOOR1'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DB770;
        pTrigger = FindTriggerByName("triggerPlaneLaserAlarm");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x24e, "Missing trigger 'triggerPlaneLaserAlarm'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DB810;
        pTrigger = FindTriggerByName("triggerPlaneLaserAlarm01");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x252, "Missing trigger 'triggerPlaneLaserAlarm01'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DB810;
        pTrigger = FindTriggerByName("accessHatch");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x256, "Missing trigger 'accessHatch'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DBA90;
        pTrigger = FindTriggerByName("wiringMalfunction");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x25a, "Missing trigger 'wiringMalfunction'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DBA70;
        return;
    case 0xe:
        Function4DBE70();
        pTrigger = FindTriggerByName("_VOC_EWAXXLIFT1");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x1ff, "Missing trigger '_VOC_EWAXXLIFT1'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DBB50;
        pTrigger = FindTriggerByName("crank");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x203, "Missing trigger 'crank'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DBEC0;
        pTrigger = FindTriggerByName("Security Button");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x207, "Missing trigger 'Security Button'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DC390;
        pTrigger = FindTriggerByName("VOC_EWAXXSENTRYtrig");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x20b, "Missing trigger 'VOC_EWAXXSENTRYtrig'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DC600;
        pTrigger = FindTriggerByName("ewaxxdoortrigger03");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x20f, "Missing trigger 'ewaxxdoortrigger03'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DC640;
        pTrigger = FindTriggerByName("dummytrigger");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x5ee, FormatString("Missing trigger '%s'! It's not in the LVL file!", "dummytrigger"));
        pTrigger->activation_callback_360 = Function4DC670;
        return;
    case 0xf:
        pTrigger = FindTriggerByName("_VOC_EWAXXCANNON1");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x1e2, "Missing trigger '_VOC_EWAXXCANNON1'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DC6B0;
        pTrigger = FindTriggerByName("EwaxxLanding");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x1ea, "Missing trigger 'EwaxxLanding'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DC6E0;
        pTrigger = FindTriggerByName("catchCord");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x1ee, "Missing trigger 'catchCord'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DC710;
        pTrigger = FindTriggerByName("painActivatorTrigger");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x1f2, "Missing trigger 'painActivatorTrigger'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DC770;
        pTrigger = FindTriggerByName("_VOC_EWAXXTOPDOOR2");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x1f6, "Missing trigger '_VOC_EWAXXTOPDOOR2'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DC730;
        return;
    case 0x10:
        if (GetFact(0x1ce) == 0) {
            Function4D9740();
            SetFact(0x1ce, 0, 0);
        }
        pTrigger = FindTriggerByName("prisondoor06");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x308, "Missing trigger 'prisondoor06'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DA610;
        pTrigger = FindTriggerByName("prisondoor04");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x30c, "Missing trigger 'prisondoor04'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DA630;
        pTrigger = FindTriggerByName("prisondoor03");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x310, "Missing trigger 'prisondoor03'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DA650;
        return;
    case 0x12:
        pTrigger = FindTriggerByName("AltarBox");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x29c, "Missing trigger 'AltarBox'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DA3C0;
        pTrigger = FindTriggerByName("platformtrigger");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x2a0, "Missing trigger 'platformtrigger'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DA460;
        pTrigger = FindTriggerByName("platformtrigger01");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x2a4, "Missing trigger 'platformtrigger01'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DA590;
        pTrigger = FindTriggerByName("platformtrigger02");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x2a8, "Missing trigger 'platformtrigger02'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DA5D0;
        return;
    case 0x13:
        pTrigger = FindTriggerByName("AirBox");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x290, "Missing trigger 'AirBox'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DA110;
        pTrigger = FindTriggerByName("DoorDone");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x294, "Missing trigger 'DoorDone'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DA2E0;
        return;
    case 0x15:
        pTrigger = FindTriggerByName("Fireantspawn");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x27a, "Missing trigger 'Fireantspawn'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DB120;
        pTrigger = FindTriggerByName("Sexspawn");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x27e, "Missing trigger 'Sexspawn'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DAFD0;
        pTrigger = FindTriggerByName("Hotstuff");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x282, "Missing trigger 'Hotstuff'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DB090;
        pTrigger = FindTriggerByName("Gate");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x286, "Missing trigger 'Gate'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DB160;
        pTrigger = FindTriggerByName("AshLock");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x5ee, FormatString("Missing trigger '%s'! It's not in the LVL file!", "AshLock"));
        pTrigger->activation_callback_360 = Function4DB180;
        pTrigger = FindTriggerByName("TimeDorado");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x5ee, FormatString("Missing trigger '%s'! It's not in the LVL file!", "TimeDorado"));
        pTrigger->activation_callback_360 = Function4DB1C0;
        return;
    case 0x16:
        g_flag_652da5 = 1;
        g_byte_652da6 = Function521060(0x254,0,0,0,0);
        pTrigger = FindTriggerByName("HigardiChest01");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x2b1, "Missing trigger 'HigardiChest01'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DAE30;
        pTrigger = FindTriggerByName("HigardiChest02");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x2b5, "Missing trigger 'HigardiChest02'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DAE70;
        pTrigger = FindTriggerByName("HigardiChest03");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x2b9, "Missing trigger 'HigardiChest03'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DAEB0;
        pTrigger = FindTriggerByName("HigardiChest04");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x2bd, "Missing trigger 'HigardiChest04'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DAEF0;
        pTrigger = FindTriggerByName("HigardiChest05");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x2c1, "Missing trigger 'HigardiChest05'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DAF30;
        pTrigger = FindTriggerByName("doortomb");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x5ee, FormatString("Missing trigger '%s'! It's not in the LVL file!", "doortomb"));
        pTrigger->activation_callback_360 = Function4DAF70;
        return;
    case 0x18:
        pTrigger = FindTriggerByName("gas_trig_plane01");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x2ca, "Missing trigger 'gas_trig_plane01'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DA9B0;
        pTrigger = FindTriggerByName("gas_trig_plane02");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x2ce, "Missing trigger 'gas_trig_plane02'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DA9B0;
        pTrigger = FindTriggerByName("gas_trig_plane03");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x2d2, "Missing trigger 'gas_trig_plane03'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DA9B0;
        pTrigger = FindTriggerByName("gas_trig_plane04");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x2d6, "Missing trigger 'gas_trig_plane04'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DA9B0;
        pTrigger = FindTriggerByName("gas_trig_plane05");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x2da, "Missing trigger 'gas_trig_plane05'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DA9B0;
        pTrigger = FindTriggerByName("gas_trig_plane06");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x2de, "Missing trigger 'gas_trig_plane06'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DA9B0;
        pTrigger = FindTriggerByName("gas_trig_plane07");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x2e2, "Missing trigger 'gas_trig_plane07'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DA9B0;
        pTrigger = FindTriggerByName("oil_pool");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x2e6, "Missing trigger 'oil_pool'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DA960;
        pTrigger = FindTriggerByName("onelid");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x2ea, "Missing trigger 'onelid'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DADF0;
        pTrigger = FindTriggerByName("fire_trig_plane01");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x5ee, FormatString("Missing trigger '%s'! It's not in the LVL file!", "fire_trig_plane01"));
        pTrigger->activation_callback_360 = Function4DAA10;
        pTrigger = FindTriggerByName("fire_trig_plane02");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x5ee, FormatString("Missing trigger '%s'! It's not in the LVL file!", "fire_trig_plane02"));
        pTrigger->activation_callback_360 = Function4DAA10;
        pTrigger = FindTriggerByName("fire_trig_plane03");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x5ee, FormatString("Missing trigger '%s'! It's not in the LVL file!", "fire_trig_plane03"));
        pTrigger->activation_callback_360 = Function4DAA10;
        pTrigger = FindTriggerByName("fire_trig_plane04");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x5ee, FormatString("Missing trigger '%s'! It's not in the LVL file!", "fire_trig_plane04"));
        pTrigger->activation_callback_360 = Function4DAA10;
        pTrigger = FindTriggerByName("fire_trig_plane05");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x5ee, FormatString("Missing trigger '%s'! It's not in the LVL file!", "fire_trig_plane05"));
        pTrigger->activation_callback_360 = Function4DAA10;
        pTrigger = FindTriggerByName("fire_trig_plane06");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x5ee, FormatString("Missing trigger '%s'! It's not in the LVL file!", "fire_trig_plane06"));
        pTrigger->activation_callback_360 = Function4DAA10;
        pTrigger = FindTriggerByName("fire_trig_plane07");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x5ee, FormatString("Missing trigger '%s'! It's not in the LVL file!", "fire_trig_plane07"));
        pTrigger->activation_callback_360 = Function4DAA10;
        return;
    case 0x19:
        pTrigger = FindTriggerByName("Fount_randomFX");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x2fa, "Missing trigger 'Fount_randomFX'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4DA740;
        Function4DA670();
        return;
    case 0x1a:
        pTrigger = FindTriggerByName("GoodaVine_A");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x31a, "Missing trigger 'GoodaVine_A'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4D9D70;
        pTrigger = FindTriggerByName("GoodaVine_B");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x31e, "Missing trigger 'GoodaVine_B'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4D9DA0;
        pTrigger = FindTriggerByName("Meat_Maker");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x322, "Missing trigger 'Meat maker'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4D9E00;
        pTrigger = FindTriggerByName("Meat_Box");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x326, "Missing trigger 'Meat_Box! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4D9E60;
        pTrigger = FindTriggerByName("Give_Zulu");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x32a, "Missing trigger 'Give Zulu'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4D9DD0;
        pTrigger = FindTriggerByName("URN_Trigger_01");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x5ee, FormatString("Missing trigger '%s'! It's not in the LVL file!", "URN_Trigger_01"));
        pTrigger->activation_callback_360 = Function4DA060;
        pTrigger = FindTriggerByName("URN_Trigger_02");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x5ee, FormatString("Missing trigger '%s'! It's not in the LVL file!", "URN_Trigger_02"));
        pTrigger->activation_callback_360 = Function4DA060;
        pTrigger = FindTriggerByName("URN_Trigger_03");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x5ee, FormatString("Missing trigger '%s'! It's not in the LVL file!", "URN_Trigger_03"));
        pTrigger->activation_callback_360 = Function4DA060;
        pTrigger = FindTriggerByName("URN_Trigger_04");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x5ee, FormatString("Missing trigger '%s'! It's not in the LVL file!", "URN_Trigger_04"));
        pTrigger->activation_callback_360 = Function4DA060;
        Function4D9D30();
        return;
    case 0x1b:
        pTrigger = FindTriggerByName("Liche");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0xe7, "Missing trigger 'Liche'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4E05F0;
        return;
    case 0x24:
        Function4E0510();
        pTrigger = FindTriggerByName("Triangle");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0xf2, "Missing trigger 'Triangle'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4E0560;
        pTrigger = FindTriggerByName("Circle");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0xf6, "Missing trigger 'Circle'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4E0560;
        pTrigger = FindTriggerByName("Square");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0xfa, "Missing trigger 'Square'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4E0560;
        pTrigger = FindTriggerByName("Star");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0xfe, "Missing trigger 'Star'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4E0560;
        pTrigger = FindTriggerByName("ButtonBlocker");
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP,
            0x102, "Missing trigger 'ButtonBlocker'! It's not in the LVL file!");
        pTrigger->activation_callback_360 = Function4E05A0;
        return;
    }
        return;
    }
