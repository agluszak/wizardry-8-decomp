#pragma once

#include "wiz8/local_code/RangeControl.h"

class W8DialogBase;
struct W8Character;

/* The three listeners own different range controls. Their callbacks update
   the item, spell-realm and skill scroll positions respectively. */
// VTABLE: WIZ8 0x005eed08
class W8CampItemRange : public W8RangeListener {
public:
    W8CampItemRange();
    ~W8CampItemRange() { delete m_range; }
    virtual void OnRangeChanged(W8RangeControl* control) override;
    W8RangeControl* m_range;
};

// VTABLE: WIZ8 0x005ef298
class W8CampSpellRange : public W8RangeListener {
public:
    explicit W8CampSpellRange(int realm);
    ~W8CampSpellRange();
    virtual void OnRangeChanged(W8RangeControl* control) override;
    W8RangeControl* m_range;
    int m_realm;
};

// VTABLE: WIZ8 0x005ef530
class W8CampSkillRange : public W8RangeListener {
public:
    W8CampSkillRange();
    ~W8CampSkillRange();
    virtual void OnRangeChanged(W8RangeControl* control) override;
    W8RangeControl* m_range;
};

// VTABLE: WIZ8 0x005ef53c Controls
// VTABLE: WIZ8 0x005ef534 W8TextControl::Listener
class W8CampSkillControls : public Controls, public W8TextControl::Listener {
public:
    W8CampSkillControls();
    virtual ~W8CampSkillControls();
    virtual void OnPrimary(W8TextControl* control) override;
    W8TextControl* m_buttons[3];
};

// VTABLE: WIZ8 0x005ef278 Controls
// VTABLE: WIZ8 0x005ef270 W8TextControl::Listener
class W8CampCharacterInfo : public Controls, public W8TextControl::Listener {
public:
    W8CampCharacterInfo();
    virtual void SetEnabled(unsigned char enabled) override;
    virtual void Redraw() override;
    virtual void OnPrimary(W8TextControl* control) override;
    void SetCombatView(unsigned char enabled);
    unsigned char m_combat_view;
    unsigned char m_pad_051[3];
    W8TextControl* m_button_054;
    W8TextControl* m_button_058;
    W8HelpTextControl* m_values[4];
};

static_assert(sizeof(W8CampItemRange) == 8, "W8CampItemRange_size");
static_assert(sizeof(W8CampSpellRange) == 12, "W8CampSpellRange_size");
static_assert(sizeof(W8CampSkillRange) == 8, "W8CampSkillRange_size");
static_assert(sizeof(W8CampSkillControls) == 0x5c, "W8CampSkillControls_size");
static_assert(sizeof(W8CampCharacterInfo) == 0x6c, "W8CampCharacterInfo_size");

/* malloc(0xd54) in Camp entry owns the record. Suspension destroys this UI;
   the screen-state stack retains the arguments needed to recreate it. */
struct W8CampScreenState0069C0F4 {
    wchar_t caption[120];
    int page;                            /* 0x0f0 */
    unsigned int hover_region;
    unsigned int redraw_flags;
    unsigned int item_redraw_flags;
    unsigned char unknown_100[0x3c0];
    int spell_scroll[6];                 /* 0x4c0 */
    unsigned char unknown_4d8[4];
    unsigned char realm_flags[6];       /* 0x4dc */
    unsigned char unknown_4e2[2];
    int item_scroll;
    unsigned char unknown_4e8[0x7d4];
    W8CampItemRange* item_range;         /* 0xcbc */
    W8CampSpellRange* spell_ranges[6];
    W8CampSkillRange* skill_range;       /* 0xcd8 */
    W8CampSkillControls* skill_controls;
    unsigned int item_timer;            /* 0xce0 */
    unsigned char item_timer_active;
    unsigned char item_timer_expired;
    unsigned char unknown_ce6[2];
    unsigned int animation_timer;
    int animation_frames[6];
    int input_mode;                     /* 0xd04 */
    unsigned char skill_flag;
    unsigned char unknown_d09[3];
    int skill_scroll;
    unsigned char unknown_d10[0x1c];
    int skill_list_scroll;
    void* skill_stack;
    int field_d34;
    unsigned char unknown_d38[7];
    unsigned char entry_mode;
    unsigned char unknown_d40[4];
    W8DialogBase* dialog;
    unsigned char item_mode;
    unsigned char unknown_d49[3];
    W8CampCharacterInfo* character_info;
    unsigned char flag_d50;
    unsigned char unknown_d51[3];
};
static_assert(sizeof(W8CampScreenState0069C0F4) == 0xd54, "W8CampScreenState_size");

extern W8CampScreenState0069C0F4* g_camp_screen_0069c0f4;
extern int g_rcs_mode_0064cbe8;
extern W8Character* g_value_0069c0f8;
extern int g_camp_entry_parameter_0069c0fc;
extern W8Character* g_camp_character_0069c100;
extern unsigned char g_camp_character_pending_0069c104;
extern unsigned int g_camp_item_region_set_0069c108;
extern unsigned int g_camp_spell_region_sets_0069c40c[6];
extern unsigned int g_camp_skill_region_set_0069c51c;

void CreateRcsLevelUpPanel(void);
void DestroyRcsLevelUpPanel(void);
void CreateRcsDismissPanel(void);
void DestroyRcsDismissPanel(void);
void DrawRcsText(const wchar_t* text, int left, int top, int width, unsigned int layout_mode);

void CampScreenInitializeRegions(void);
void Function5B7230(void);

extern int g_effect_005ee6ec;
extern int g_effect_argument_005ed8cc;
