#pragma once

/* Address-qualified engine object constructed beside the canonical GDCamera
   owner in GameData.cpp. Its constructor proves the complete 0x34-byte
   derived layout; no original class or member names are available. */
class W8ObjectBase00439550 {
public:
    W8ObjectBase00439550();              /* 0x00439550 */
    virtual ~W8ObjectBase00439550();

protected:
    unsigned char unknown_004[0x10];
};

class W8Object0043A910 : public W8ObjectBase00439550 {
public:
    W8Object0043A910();                  /* 0x0043A910 */
    virtual ~W8Object0043A910() override;
    float GetValue28() const { return m_value_28; }
    float GetValue30() const { return m_value_30; }

private:
    int m_ticks_14;
    int m_ticks_18;
    float m_rate_1c;
    float m_scale_20;
    float m_scale_24;
    float m_value_28;
    float m_value_2c;
    float m_value_30;
};

static_assert(sizeof(W8ObjectBase00439550) == 0x14,
              "W8ObjectBase00439550_must_be_0x14");
static_assert(sizeof(W8Object0043A910) == 0x34,
              "W8Object0043A910_must_be_0x34");
