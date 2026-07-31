/* Address quarantine 005bc811-005c433f; bounds come from adjacent
   assertion-backed original translation-unit intervals.

   This unit declares the three functions it calls itself rather than reaching
   for quarantine_common.h: nothing here needs a type, and that header is a
   mechanism the repository drains rather than grows. */

void RequestScreenTransition(void);
void SetPendingScreenState(int value);
void SetValue64D8AC(unsigned long value);

// GLOBAL: WIZ8 0x006875B4
unsigned char g_flag_6875b4;
// GLOBAL: WIZ8 0x0068DE50
int g_value_68de50;

/* Lifecycle record 2's frame close-out, and the only slot that record fills:
   its other four are the shared do-nothing filler. Every path asks for a screen
   transition, records one of four codes and queues screen 0, so the record is a
   pure router - it selects which of the four the transition reports and then
   leaves. Nothing here names the four codes or the two globals that pick them.

   The retail body carries four copies of the call tail, each loading its code
   through EAX rather than pushing it. That is VC6 duplicating one tail, not the
   original writing four: the tail is written once here and the four copies come
   back, where writing the four calls out literally emits direct pushes and four
   instructions too few. */
// FUNCTION: WIZ8 0x005c3800
void Screen2Finish(void)
{
    unsigned long code;

    RequestScreenTransition();
    if (!g_flag_6875b4) {
        code = 4;
    }
    else {
        switch (g_value_68de50) {
        case 1:
            code = 2;
            break;
        case 2:
            code = 3;
            break;
        default:
            code = 1;
            break;
        }
    }
    SetValue64D8AC(code);
    SetPendingScreenState(0);
}
