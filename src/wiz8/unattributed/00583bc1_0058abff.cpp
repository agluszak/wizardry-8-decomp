extern "C" {
extern float g_float_64b914;
extern int g_value_68f2b0;
extern int g_value_68f2c4;
}

/* Address quarantine 00583bc1-0058abff; bounds come from adjacent
   assertion-backed original translation-unit intervals. */

// FUNCTION: WIZ8 0x00585300
void SetFloat64B914(float value)
{
    g_float_64b914 = value;
}
// FUNCTION: WIZ8 0x00585310
float GetFloat64B914(void)
{
    return g_float_64b914;
}
// FUNCTION: WIZ8 0x00587C10
void SetValue68F2B0(int value)
{
    g_value_68f2b0 = value;
}
// FUNCTION: WIZ8 0x0058A870
void SetValue68F2C4(int value)
{
    g_value_68f2c4 = value;
}
