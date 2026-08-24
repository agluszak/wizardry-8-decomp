#pragma once

/* Local Code\Configuration.cpp owns the persisted 0xa4-byte configuration
   block at 0x006850C8. */

#pragma pack(push, 1)
struct W8GameSettings {
    unsigned char field_000;
    unsigned char field_001;
    unsigned char unknown_002[0x4];
    int field_006;
    unsigned char field_00a;
    unsigned char field_00b;
    unsigned char field_00c;
    int field_00d;
    int field_011;
    int field_015;
    int field_019;
    int field_01d;
    int field_021;
    int field_025;
    unsigned char field_029;
    unsigned char field_02a;
    unsigned char field_02b;
    unsigned char field_02c;
    unsigned char unknown_02d[0x1];
    unsigned char field_02e;
    unsigned char field_02f;
    unsigned char field_030;
    unsigned char field_031;
    unsigned char field_032;
    unsigned char field_033;
    unsigned char field_034;
    unsigned char field_035;
    unsigned char field_036;
    int field_037;
    unsigned char field_03b;
    int field_03c;
    unsigned char field_040;
    unsigned char field_041;
    unsigned char field_042;
    unsigned char field_043;
    unsigned char unknown_044[0x1];
    unsigned char field_045;
    unsigned char unknown_046[0x1];
    unsigned char field_047;
    unsigned char field_048;
    unsigned char field_049;
    unsigned char field_04a;
    unsigned char field_04b;
    unsigned char field_04c;
    unsigned char field_04d;
    unsigned char field_04e;
    unsigned char field_04f;
    unsigned char field_050;
    unsigned char unknown_051[0x53];
};
#pragma pack(pop)

static_assert(sizeof(W8GameSettings) == 0xa4,
              "W8GameSettings_must_be_0xa4");

extern "C" {
extern W8GameSettings g_settings_6850c8;
}
