extern "C" {
extern int g_value_69da68;
extern unsigned char g_flag_69da6c;
extern unsigned char g_table_650434[][8];
}

/* Address quarantine 005e2cc1-005e37ff; bounds come from adjacent
   assertion-backed original translation-unit intervals. */

// FUNCTION: WIZ8 0x005E35F0
void ClearValue69DA68(void)
{
    g_value_69da68 = 0;
}
// FUNCTION: WIZ8 0x005E3600
unsigned char GetFlag69DA6C(void)
{
    return g_flag_69da6c;
}
// FUNCTION: WIZ8 0x005E3730
unsigned char GetTable650434Entry(int row, int column)
{
    return g_table_650434[row][column];
}
