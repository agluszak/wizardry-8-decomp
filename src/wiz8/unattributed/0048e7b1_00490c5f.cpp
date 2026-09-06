extern "C" {
extern int g_value_65ba5c;
extern unsigned char g_flag_6850fb;
}

/* Address quarantine 0048e7b1-00490c5f; bounds come from adjacent
   assertion-backed original translation-unit intervals. */

// FUNCTION: WIZ8 0x0048ED00
int GetValue65BA5C(void)
{
    return g_value_65ba5c;
}
// FUNCTION: WIZ8 0x0048FE80
bool IsFlag6850FBSet(void)
{
    return g_flag_6850fb != 0xff;
}
