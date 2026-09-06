extern "C" int g_value_60dfac;

/* Address quarantine 004b6bd1-004b6f2f; bounds come from adjacent
   assertion-backed original translation-unit intervals. */

// FUNCTION: WIZ8 0x004B6D10
int IncrementValue60DFAC(void)
{
    g_value_60dfac = g_value_60dfac + 1;
    return g_value_60dfac;
}
// FUNCTION: WIZ8 0x004B6D20
void SetValue60DFAC(void)
{
    g_value_60dfac = 1;
}
