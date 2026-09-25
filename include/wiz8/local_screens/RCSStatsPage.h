#pragma once

#include "wiz8/local_code/Controls.h"
#include "wiz8/local_code/RangeControl.h"
#include "wiz8/local_code/TextControl.h"
#include "input.h"

struct W8Region;

/* Camp stats page (page 1) and skills page (page 2) machinery. The original
   file was "Local Screens\RCSStatsPage.cpp" in the official demo; retail keeps
   no path string, so the unit's hull comes from the demo function order
   between the mipeEdit.cpp and CGSSpellsPage.cpp anchors. The scrollbar and
   the three toggle buttons drive the stats page's condition/equipment effect
   list: button 0 selects beneficial rows, button 1 detrimental rows and
   button 2 switches the list to equipped items. */

// VTABLE: WIZ8 0x005ef530
class W8CampStatsRange : public W8CampRangeListener {
public:
    W8CampStatsRange();
    ~W8CampStatsRange();
    virtual void OnRangeChanged(W8RangeControl* control) override;
};

// VTABLE: WIZ8 0x005ef53c Controls
// VTABLE: WIZ8 0x005ef534 W8TextControl::Listener
class W8CampStatsControls : public Controls, public W8TextControl::Listener {
public:
    W8CampStatsControls();
    virtual ~W8CampStatsControls();
    virtual void OnPrimary(W8TextControl* control) override;
    W8TextControl* m_buttons[3];
};

static_assert(sizeof(W8CampStatsRange) == 8, "W8CampStatsRange_size");
static_assert(sizeof(W8CampStatsControls) == 0x5c, "W8CampStatsControls_size");
/* Retail secondary vftable 0x005ef534 places W8TextControl::Listener at +0x4c. */
W8_ASSERT_BASE_END(W8CampStatsControls, W8TextControl::Listener, m_buttons, 0x4c);

/* 0x005C48B0: camp page-1 renderer - attributes, traits and the effect list
   trigger; 0x005C5D80: camp page-2 renderer - the five skill categories. Both
   dispatch from the page switch in ReviewCharacterScreen.cpp's 0x005A42A0. */
void DrawCampStatsPage(void);
void DrawCampSkillsPage(void);

/* Rebuilds the stats page's effect list from the reviewed character's
   conditions, enchantments and equipped items, then refilters it. Called when
   the reviewed character changes and when the stats page is entered. */
void RebuildCampEffectList(void);
void FilterCampEffectList(void);

/* Lazily creates and enables the five per-category skill regions of camp page
   2; DisableCampSkillRegions releases them on camp leave. */
void CreateCampSkillRegions(void);
void DisableCampSkillRegions(void);
unsigned char CampSkillListRegionHandler(const InputAtom* event, W8Region* region);

extern unsigned int g_camp_stats_range_region_set_0069c51c;
extern unsigned int g_camp_stats_controls_region_set_0069c520;
extern unsigned int g_camp_skill_regions_0069c528;
