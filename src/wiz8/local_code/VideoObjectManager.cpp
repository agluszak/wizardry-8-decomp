#include "wiz8/engine_code/Video2.h"
#include "wiz8/wiz8_windows.h"
#include "wiz8/sr_api.h"
#include "wiz8/video_object_catalog.h"
#include "wiz8/utility.h"

#include <stdlib.h>
#include <string.h>
#include "vobject.h"
#include "vobject_blitters.h"
#include "vsurface.h"

/*
 * Local Code\VideoObjectManager.cpp, named by the three assertions this body
 * embeds at lines 45, 62 and 221.
 *
 * Two tables are indexed here and both are established by this body alone. The
 * slot table is eight bytes per entry: a first-frame index at +0x00 and a
 * signed vertical offset at +0x04, which is read with movsx. The frame table is
 * 0x3c bytes per entry - the decompiler shows the index scaled by 0xf dwords -
 * whose first 0x30 bytes are the path it loads from, then a mode selector, a
 * loaded flag and the surface it draws. The path is what fixes the base at
 * 0x0062C430: reading the blit alone suggests 0x0062C460, because the first
 * fields it touches are the mode and the surface.
 */

static_assert(sizeof(W8VideoObjectSlot) == 8, "W8VideoObjectSlot_size_must_be_8");
static_assert(sizeof(W8VideoFrame) == 0x3c, "W8VideoFrame_size_must_be_0x3c");

/* The retail catalog's omitted fields - the loaded flag, its alignment bytes
   and the handle - are zero in the data image; only the path and mode are
   explicit. clang's -Wmissing-field-initializers fires once per omitted field
   per row, so the table is wrapped rather than spelling 1600 zero literals
   that the original source did not write. */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
// GLOBAL: WIZ8 0x0062c430
W8VideoFrame g_video_frames_62c430[1658] = {
    {"Data\\Cursors\\2D-Cursors.sti", 0},
    {"Data\\Portraits\\Large\\Lhummf.sti", 1},
    {"Data\\Portraits\\Large\\Lhummn.sti", 1},
    {"Data\\Portraits\\Large\\Lhummm.sti", 1},
    {"Data\\Portraits\\Large\\Lhumm1.sti", 1},
    {"Data\\Portraits\\Large\\Lhumm2.sti", 1},
    {"Data\\Portraits\\Large\\Lhumm3.sti", 1},
    {"Data\\Portraits\\Large\\Lhumff.sti", 1},
    {"Data\\Portraits\\Large\\Lhumfn.sti", 1},
    {"Data\\Portraits\\Large\\Lhumfm.sti", 1},
    {"Data\\Portraits\\Large\\Lhumf1.sti", 1},
    {"Data\\Portraits\\Large\\Lhumf2.sti", 1},
    {"Data\\Portraits\\Large\\Lhumf3.sti", 1},
    {"Data\\Portraits\\Large\\Lelfmf.sti", 1},
    {"Data\\Portraits\\Large\\Lelfmn.sti", 1},
    {"Data\\Portraits\\Large\\Lelfmm.sti", 1},
    {"Data\\Portraits\\Large\\Lelfff.sti", 1},
    {"Data\\Portraits\\Large\\Lelffn.sti", 1},
    {"Data\\Portraits\\Large\\Lelffm.sti", 1},
    {"Data\\Portraits\\Large\\Ldwarfmf.sti", 1},
    {"Data\\Portraits\\Large\\Ldwarfmn.sti", 1},
    {"Data\\Portraits\\Large\\Ldwarfmm.sti", 1},
    {"Data\\Portraits\\Large\\Ldwarfff.sti", 1},
    {"Data\\Portraits\\Large\\Ldwarffn.sti", 1},
    {"Data\\Portraits\\Large\\Ldwarffm.sti", 1},
    {"Data\\Portraits\\Large\\Lgnomemn.sti", 1},
    {"Data\\Portraits\\Large\\Lgnomemm.sti", 1},
    {"Data\\Portraits\\Large\\Lgnomeff.sti", 1},
    {"Data\\Portraits\\Large\\Lgnomefn.sti", 1},
    {"Data\\Portraits\\Large\\Lhobmf.sti", 1},
    {"Data\\Portraits\\Large\\Lhobmn.sti", 1},
    {"Data\\Portraits\\Large\\Lhobff.sti", 1},
    {"Data\\Portraits\\Large\\Lhobfn.sti", 1},
    {"Data\\Portraits\\Large\\Lfairymf.sti", 1},
    {"Data\\Portraits\\Large\\Lfairymm.sti", 1},
    {"Data\\Portraits\\Large\\Lfairyff.sti", 1},
    {"Data\\Portraits\\Large\\Lfairyfn.sti", 1},
    {"Data\\Portraits\\Large\\Llizmf.sti", 1},
    {"Data\\Portraits\\Large\\Llizmn.sti", 1},
    {"Data\\Portraits\\Large\\Llizff.sti", 1},
    {"Data\\Portraits\\Large\\Llizfn.sti", 1},
    {"Data\\Portraits\\Large\\Ldracmf.sti", 1},
    {"Data\\Portraits\\Large\\Ldracmn.sti", 1},
    {"Data\\Portraits\\Large\\Ldracff.sti", 1},
    {"Data\\Portraits\\Large\\Ldracfn.sti", 1},
    {"Data\\Portraits\\Large\\Lfelpmf.sti", 1},
    {"Data\\Portraits\\Large\\Lfelpmn.sti", 1},
    {"Data\\Portraits\\Large\\Lfelpff.sti", 1},
    {"Data\\Portraits\\Large\\Lfelpfn.sti", 1},
    {"Data\\Portraits\\Large\\Lrawmn.sti", 1},
    {"Data\\Portraits\\Large\\Lrawmm.sti", 1},
    {"Data\\Portraits\\Large\\Lrawff.sti", 1},
    {"Data\\Portraits\\Large\\Lrawfn.sti", 1},
    {"Data\\Portraits\\Large\\Lmookmf.sti", 1},
    {"Data\\Portraits\\Large\\Lmookmn.sti", 1},
    {"Data\\Portraits\\Large\\Lmookff.sti", 1},
    {"Data\\Portraits\\Large\\Lmookfn.sti", 1},
    {"Data\\Portraits\\Large\\Lninmf.sti", 1},
    {"Data\\Portraits\\Large\\Lninff.sti", 1},
    {"Data\\Portraits\\Large\\Lmook.sti", 1},
    {"Data\\Portraits\\Large\\Ltrynm1.sti", 1},
    {"Data\\Portraits\\Large\\Ltrynm2.sti", 1},
    {"Data\\Portraits\\Large\\Ltrynm3.sti", 1},
    {"Data\\Portraits\\Large\\Ltrang.sti", 1},
    {"Data\\Portraits\\Large\\Lumpani.sti", 1},
    {"Data\\Portraits\\Large\\Lsexus.sti", 1},
    {"Data\\Portraits\\Large\\Lurq.sti", 1},
    {"Data\\Portraits\\Large\\LRFS-81.sti", 1},
    {"Data\\Portraits\\Large\\Lmadras.sti", 1},
    {"Data\\Portraits\\Large\\Lsparkle.sti", 1},
    {"Data\\Portraits\\Large\\Lmyles.sti", 1},
    {"Data\\Portraits\\Large\\LVi.sti", 1},
    {"Data\\Portraits\\Large\\Ldrazic.sti", 1},
    {"Data\\Portraits\\Large\\Ltantris.sti", 1},
    {"Data\\Portraits\\Large\\Lrodan.sti", 1},
    {"Data\\Portraits\\Large\\Lglumph.sti", 1},
    {"Data\\Portraits\\Large\\Lsaxx.sti", 1},
    {"Data\\Portraits\\Large\\Lhumm4.sti", 1},
    {"Data\\Portraits\\Large\\Lhumf4.sti", 1},
    {"Data\\Portraits\\Large\\Lelfm1.sti", 1},
    {"Data\\Portraits\\Large\\Lelff1.sti", 1},
    {"Data\\Portraits\\Medium\\Mhummf.sti", 0},
    {"Data\\Portraits\\Medium\\Mhummn.sti", 0},
    {"Data\\Portraits\\Medium\\Mhummm.sti", 0},
    {"Data\\Portraits\\Medium\\mhumm1.sti", 0},
    {"Data\\Portraits\\Medium\\mhumm2.sti", 0},
    {"Data\\Portraits\\Medium\\mhumm3.sti", 0},
    {"Data\\Portraits\\Medium\\Mhumff.sti", 0},
    {"Data\\Portraits\\Medium\\Mhumfn.sti", 0},
    {"Data\\Portraits\\Medium\\Mhumfm.sti", 0},
    {"Data\\Portraits\\Medium\\mhumf1.sti", 0},
    {"Data\\Portraits\\Medium\\mhumf2.sti", 0},
    {"Data\\Portraits\\Medium\\mhumf3.sti", 0},
    {"Data\\Portraits\\Medium\\Melfmf.sti", 0},
    {"Data\\Portraits\\Medium\\Melfmn.sti", 0},
    {"Data\\Portraits\\Medium\\Melfmm.sti", 0},
    {"Data\\Portraits\\Medium\\Melfff.sti", 0},
    {"Data\\Portraits\\Medium\\Melffn.sti", 0},
    {"Data\\Portraits\\Medium\\Melffm.sti", 0},
    {"Data\\Portraits\\Medium\\Mdwarfmf.sti", 0},
    {"Data\\Portraits\\Medium\\Mdwarfmn.sti", 0},
    {"Data\\Portraits\\Medium\\Mdwarfmm.sti", 0},
    {"Data\\Portraits\\Medium\\Mdwarfff.sti", 0},
    {"Data\\Portraits\\Medium\\Mdwarffn.sti", 0},
    {"Data\\Portraits\\Medium\\Mdwarffm.sti", 0},
    {"Data\\Portraits\\Medium\\Mgnomemn.sti", 0},
    {"Data\\Portraits\\Medium\\Mgnomemm.sti", 0},
    {"Data\\Portraits\\Medium\\Mgnomeff.sti", 0},
    {"Data\\Portraits\\Medium\\Mgnomefn.sti", 0},
    {"Data\\Portraits\\Medium\\Mhobmf.sti", 0},
    {"Data\\Portraits\\Medium\\Mhobmn.sti", 0},
    {"Data\\Portraits\\Medium\\Mhobff.sti", 0},
    {"Data\\Portraits\\Medium\\Mhobfn.sti", 0},
    {"Data\\Portraits\\Medium\\Mfairymf.sti", 0},
    {"Data\\Portraits\\Medium\\Mfairymm.sti", 0},
    {"Data\\Portraits\\Medium\\Mfairyff.sti", 0},
    {"Data\\Portraits\\Medium\\Mfairyfn.sti", 0},
    {"Data\\Portraits\\Medium\\Mlizmf.sti", 0},
    {"Data\\Portraits\\Medium\\Mlizmn.sti", 0},
    {"Data\\Portraits\\Medium\\Mlizff.sti", 0},
    {"Data\\Portraits\\Medium\\Mlizfn.sti", 0},
    {"Data\\Portraits\\Medium\\Mdracmf.sti", 0},
    {"Data\\Portraits\\Medium\\Mdracmn.sti", 0},
    {"Data\\Portraits\\Medium\\Mdracff.sti", 0},
    {"Data\\Portraits\\Medium\\Mdracfn.sti", 0},
    {"Data\\Portraits\\Medium\\Mfelpmf.sti", 0},
    {"Data\\Portraits\\Medium\\Mfelpmn.sti", 0},
    {"Data\\Portraits\\Medium\\Mfelpff.sti", 0},
    {"Data\\Portraits\\Medium\\Mfelpfn.sti", 0},
    {"Data\\Portraits\\Medium\\Mrawmn.sti", 0},
    {"Data\\Portraits\\Medium\\Mrawmm.sti", 0},
    {"Data\\Portraits\\Medium\\Mrawff.sti", 0},
    {"Data\\Portraits\\Medium\\Mrawfn.sti", 0},
    {"Data\\Portraits\\Medium\\Mmookmf.sti", 0},
    {"Data\\Portraits\\Medium\\Mmookmn.sti", 0},
    {"Data\\Portraits\\Medium\\Mmookff.sti", 0},
    {"Data\\Portraits\\Medium\\Mmookfn.sti", 0},
    {"Data\\Portraits\\Medium\\Mninmf.sti", 0},
    {"Data\\Portraits\\Medium\\Mninff.sti", 0},
    {"Data\\Portraits\\Medium\\mmook.sti", 0},
    {"Data\\Portraits\\Medium\\mtrynm1.sti", 0},
    {"Data\\Portraits\\Medium\\mtrynm2.sti", 0},
    {"Data\\Portraits\\Medium\\mtrynm3.sti", 0},
    {"Data\\Portraits\\Medium\\Mtrang.sti", 0},
    {"Data\\Portraits\\Medium\\Mumpani.sti", 0},
    {"Data\\Portraits\\Medium\\asexus.sti", 0},
    {"Data\\Portraits\\Medium\\aurq.sti", 0},
    {"Data\\Portraits\\Medium\\aRFS-81.sti", 0},
    {"Data\\Portraits\\Medium\\amadras.sti", 0},
    {"Data\\Portraits\\Medium\\asparkle.sti", 0},
    {"Data\\Portraits\\Medium\\amyles.sti", 0},
    {"Data\\Portraits\\Medium\\aVi.sti", 0},
    {"Data\\Portraits\\Medium\\adrazic.sti", 0},
    {"Data\\Portraits\\Medium\\atantris.sti", 0},
    {"Data\\Portraits\\Medium\\arodan.sti", 0},
    {"Data\\Portraits\\Medium\\aglumph.sti", 0},
    {"Data\\Portraits\\Medium\\asaxx.sti", 0},
    {"Data\\Portraits\\Medium\\mhumm4.sti", 0},
    {"Data\\Portraits\\Medium\\mhumf4.sti", 0},
    {"Data\\Portraits\\Medium\\Melfm1.sti", 0},
    {"Data\\Portraits\\Medium\\Melff1.sti", 0},
    {"Data\\Portraits\\Small\\Shummf.sti", 1},
    {"Data\\Portraits\\Small\\Shummn.sti", 1},
    {"Data\\Portraits\\Small\\Shummm.sti", 1},
    {"Data\\Portraits\\Small\\shumm1.sti", 1},
    {"Data\\Portraits\\Small\\shumm2.sti", 1},
    {"Data\\Portraits\\Small\\shumm3.sti", 1},
    {"Data\\Portraits\\Small\\Shumff.sti", 1},
    {"Data\\Portraits\\Small\\Shumfn.sti", 1},
    {"Data\\Portraits\\Small\\Shumfm.sti", 1},
    {"Data\\Portraits\\Small\\shumf1.sti", 1},
    {"Data\\Portraits\\Small\\shumf2.sti", 1},
    {"Data\\Portraits\\Small\\shumf3.sti", 1},
    {"Data\\Portraits\\Small\\Selfmf.sti", 1},
    {"Data\\Portraits\\Small\\Selfmn.sti", 1},
    {"Data\\Portraits\\Small\\Selfmm.sti", 1},
    {"Data\\Portraits\\Small\\Selfff.sti", 1},
    {"Data\\Portraits\\Small\\Selffn.sti", 1},
    {"Data\\Portraits\\Small\\Selffm.sti", 1},
    {"Data\\Portraits\\Small\\Sdwarfmf.sti", 1},
    {"Data\\Portraits\\Small\\Sdwarfmn.sti", 1},
    {"Data\\Portraits\\Small\\Sdwarfmm.sti", 1},
    {"Data\\Portraits\\Small\\Sdwarfff.sti", 1},
    {"Data\\Portraits\\Small\\Sdwarffn.sti", 1},
    {"Data\\Portraits\\Small\\Sdwarffm.sti", 1},
    {"Data\\Portraits\\Small\\Sgnomemn.sti", 1},
    {"Data\\Portraits\\Small\\Sgnomemm.sti", 1},
    {"Data\\Portraits\\Small\\Sgnomeff.sti", 1},
    {"Data\\Portraits\\Small\\Sgnomefn.sti", 1},
    {"Data\\Portraits\\Small\\Shobmf.sti", 1},
    {"Data\\Portraits\\Small\\Shobmn.sti", 1},
    {"Data\\Portraits\\Small\\Shobff.sti", 1},
    {"Data\\Portraits\\Small\\Shobfn.sti", 1},
    {"Data\\Portraits\\Small\\Sfairymf.sti", 1},
    {"Data\\Portraits\\Small\\Sfairymm.sti", 1},
    {"Data\\Portraits\\Small\\Sfairyff.sti", 1},
    {"Data\\Portraits\\Small\\Sfairyfn.sti", 1},
    {"Data\\Portraits\\Small\\Slizmf.sti", 1},
    {"Data\\Portraits\\Small\\Slizmn.sti", 1},
    {"Data\\Portraits\\Small\\Slizff.sti", 1},
    {"Data\\Portraits\\Small\\Slizfn.sti", 1},
    {"Data\\Portraits\\Small\\Sdracmf.sti", 1},
    {"Data\\Portraits\\Small\\Sdracmn.sti", 1},
    {"Data\\Portraits\\Small\\Sdracff.sti", 1},
    {"Data\\Portraits\\Small\\Sdracfn.sti", 1},
    {"Data\\Portraits\\Small\\Sfelpmf.sti", 1},
    {"Data\\Portraits\\Small\\Sfelpmn.sti", 1},
    {"Data\\Portraits\\Small\\Sfelpff.sti", 1},
    {"Data\\Portraits\\Small\\Sfelpfn.sti", 1},
    {"Data\\Portraits\\Small\\Srawmn.sti", 1},
    {"Data\\Portraits\\Small\\Srawmm.sti", 1},
    {"Data\\Portraits\\Small\\Srawff.sti", 1},
    {"Data\\Portraits\\Small\\Srawfn.sti", 1},
    {"Data\\Portraits\\Small\\Smookmf.sti", 1},
    {"Data\\Portraits\\Small\\Smookmn.sti", 1},
    {"Data\\Portraits\\Small\\Smookff.sti", 1},
    {"Data\\Portraits\\Small\\Smookfn.sti", 1},
    {"Data\\Portraits\\Small\\Sninmf.sti", 1},
    {"Data\\Portraits\\Small\\Sninff.sti", 1},
    {"Data\\Portraits\\Small\\smook.sti", 1},
    {"Data\\Portraits\\Small\\strynm1.sti", 1},
    {"Data\\Portraits\\Small\\strynm2.sti", 1},
    {"Data\\Portraits\\Small\\strynm3.sti", 1},
    {"Data\\Portraits\\Small\\strang.sti", 1},
    {"Data\\Portraits\\Small\\sumpani.sti", 1},
    {"Data\\Portraits\\Small\\ssexus.sti", 0},
    {"Data\\Portraits\\Small\\surq.sti", 0},
    {"Data\\Portraits\\Small\\sRFS-81.sti", 0},
    {"Data\\Portraits\\Small\\smadras.sti", 0},
    {"Data\\Portraits\\Small\\ssparkle.sti", 0},
    {"Data\\Portraits\\Small\\smyles.sti", 0},
    {"Data\\Portraits\\Small\\sVi.sti", 0},
    {"Data\\Portraits\\Small\\sdrazic.sti", 0},
    {"Data\\Portraits\\Small\\stantris.sti", 0},
    {"Data\\Portraits\\Small\\srodan.sti", 0},
    {"Data\\Portraits\\Small\\sglumph.sti", 0},
    {"Data\\Portraits\\Small\\ssaxx.sti", 0},
    {"Data\\Portraits\\Small\\shumm4.sti", 1},
    {"Data\\Portraits\\Small\\shumf4.sti", 1},
    {"Data\\Portraits\\Small\\Selfm1.sti", 1},
    {"Data\\Portraits\\Small\\Selff1.sti", 1},
    {"Data\\Portraits\\Small\\SSkull.sti", 1},
    {"Data\\Portraits\\Small\\SSkull_drac.sti", 1},
    {"Data\\Portraits\\Small\\SSkull_felp.sti", 1},
    {"Data\\Portraits\\Small\\SSkull_liz.sti", 1},
    {"Data\\Portraits\\Small\\SSkull_mook.sti", 1},
    {"Data\\Portraits\\Small\\SSkull_raw.sti", 1},
    {"Data\\Portraits\\Small\\SSkull_trang.sti", 1},
    {"Data\\Portraits\\Small\\SSkull_tryn.sti", 1},
    {"Data\\Portraits\\Small\\SSkull_ump.sti", 1},
    {"Data\\Portraits\\Small\\SSkull_rap.sti", 1},
    {"Data\\Portraits\\Small\\SSkull_and.sti", 1},
    {"Data\\Portraits\\Medium\\MSkull.sti", 0},
    {"Data\\Portraits\\Medium\\MSkull_drac.sti", 0},
    {"Data\\Portraits\\Medium\\MSkull_felp.sti", 0},
    {"Data\\Portraits\\Medium\\MSkull_liz.sti", 0},
    {"Data\\Portraits\\Medium\\MSkull_mook.sti", 0},
    {"Data\\Portraits\\Medium\\MSkull_raw.sti", 0},
    {"Data\\Portraits\\Medium\\MSkull_trang.sti", 0},
    {"Data\\Portraits\\Medium\\MSkull_tryn.sti", 0},
    {"Data\\Portraits\\Medium\\MSkull_ump.sti", 0},
    {"Data\\Portraits\\Medium\\MSkull_rap.sti", 0},
    {"Data\\Portraits\\Medium\\MSkull_and.sti", 0},
    {"Data\\Main Interface\\option2G.sti", 0},
    {"Data\\Main Interface\\option2H.sti", 0},
    {"Data\\Main Interface\\option2N.sti", 0},
    {"Data\\Main Interface\\party2G.sti", 0},
    {"Data\\Main Interface\\party2H.sti", 0},
    {"Data\\Main Interface\\party2N.sti", 0},
    {"Data\\Main Interface\\search2G.sti", 0},
    {"Data\\Main Interface\\search2H.sti", 0},
    {"Data\\Main Interface\\search2N.sti", 0},
    {"Data\\Main Interface\\camp2G.sti", 0},
    {"Data\\Main Interface\\camp2H.sti", 0},
    {"Data\\Main Interface\\camp2N.sti", 0},
    {"Data\\Main Interface\\bottom.sti", 0},
    {"Data\\Main Interface\\bottomB.sti", 0},
    {"Data\\Main Interface\\roof.sti", 0},
    {"Data\\Main Interface\\layout.sti", 0},
    {"Data\\Main Interface\\leftcorner.sti", 0},
    {"Data\\Main Interface\\rightcorner.sti", 0},
    {"Data\\Main Interface\\leftplug.sti", 0},
    {"Data\\Main Interface\\rightplug.sti", 0},
    {"Data\\Main Interface\\pback.sti", 0},
    {"Data\\Main Interface\\CharHighlight.sti", 0},
    {"Data\\Main Interface\\CharSelect.sti", 0},
    {"Data\\Main Interface\\dragons.sti", 0},
    {"Data\\Main Interface\\Inside.sti", 0},
    {"Data\\Main Interface\\Outside.sti", 0},
    {"Data\\Main Interface\\Outside2.sti", 0},
    {"Data\\Main Interface\\lefttop.sti", 0},
    {"Data\\Main Interface\\leftside1.sti", 0},
    {"Data\\Main Interface\\leftside2.sti", 0},
    {"Data\\Main Interface\\leftside3.sti", 0},
    {"Data\\Main Interface\\leftside4.sti", 0},
    {"Data\\Main Interface\\righttop.sti", 0},
    {"Data\\Main Interface\\rightside1.sti", 0},
    {"Data\\Main Interface\\rightside2.sti", 0},
    {"Data\\Main Interface\\rightside3.sti", 0},
    {"Data\\Main Interface\\rightside4.sti", 0},
    {"Data\\Main Interface\\HealthBar.sti", 0},
    {"Data\\Main Interface\\StaminaBar.sti", 0},
    {"Data\\Main Interface\\MagicBar.sti", 0},
    {"Data\\Main Interface\\HealthBarWide.sti", 0},
    {"Data\\Main Interface\\StaminaBarWide.sti", 0},
    {"Data\\Main Interface\\MagicBarWide.sti", 0},
    {"Data\\Main Interface\\GoldBar.sti", 0},
    {"Data\\Main Interface\\TextUpArrowOff.sti", 0},
    {"Data\\Main Interface\\TextUpArrowOn.sti", 0},
    {"Data\\Main Interface\\TextUpArrowHigh.sti", 0},
    {"Data\\Main Interface\\TextDownArrowOff.sti", 0},
    {"Data\\Main Interface\\TextDownArrowOn.sti", 0},
    {"Data\\Main Interface\\TextDownArrowHigh.sti", 0},
    {"Data\\Main Interface\\SliderBar.sti", 0},
    {"Data\\Main Interface\\SliderNut.sti", 0},
    {"Data\\Main Interface\\EdgeHighlight.sti", 0},
    {"Data\\Main Interface\\EdgeSelect.sti", 0},
    {"Data\\Main Interface\\CornerSelect.sti", 0},
    {"Data\\Main Interface\\condition_highlight.sti", 0},
    {"Data\\Main Interface\\NameSelect.sti", 0},
    {"Data\\Main Interface\\WeaponSelect.sti", 0},
    {"Data\\Main Interface\\StatsSelect.sti", 0},
    {"Data\\Main Interface\\StatsSelectWide.sti", 0},
    {"Data\\Main Interface\\HandLeft.sti", 0},
    {"Data\\Main Interface\\HandRight.sti", 0},
    {"Data\\Main Interface\\FHandLeft.sti", 0},
    {"Data\\Main Interface\\FHandRight.sti", 0},
    {"Data\\Main Interface\\PawLeft.sti", 0},
    {"Data\\Main Interface\\PawRight.sti", 0},
    {"Data\\Main Interface\\ClawLeft.sti", 0},
    {"Data\\Main Interface\\ClawRight.sti", 0},
    {"Data\\Main Interface\\RHandLeft.sti", 0},
    {"Data\\Main Interface\\RHandRight.sti", 0},
    {"Data\\Main Interface\\popup_edge.sti", 0},
    {"Data\\Main Interface\\popup_divider.sti", 0},
    {"Data\\Main Interface\\PopupHighlight.sti", 0},
    {"Data\\Main Interface\\Weapon2Select.sti", 0},
    {"Data\\Main Interface\\LeftWingTip.sti", 0},
    {"Data\\Main Interface\\RightWingTip.sti", 0},
    {"Data\\Main Interface\\LeftStrip.sti", 0},
    {"Data\\Main Interface\\RightStrip.sti", 0},
    {"Data\\Main Interface\\BottomStrip.sti", 0},
    {"Data\\Main Interface\\FormationJewels.sti", 0},
    {"Data\\Main Interface\\FormationJewelsH.sti", 0},
    {"Data\\Flics\\FormationSymbol.sti", 0},
    {"Data\\Main Interface\\Compass.sti", 0},
    {"Data\\Main Interface\\ViewCone.sti", 0},
    {"Data\\Main Interface\\main_bottom_bar.sti", 0},
    {"Data\\Main Interface\\main_submenu_bar.sti", 0},
    {"Data\\Main Interface\\main_column_info_normal.sti", 0},
    {"Data\\Main Interface\\main_columns_hit.sti", 0},
    {"Data\\Main Interface\\main_columns_normal.sti", 0},
    {"Data\\Main Interface\\small_formation_back.sti", 0},
    {"Data\\Main Interface\\main_radar_back.sti", 0},
    {"Data\\Main Interface\\main_roof.sti", 0},
    {"Data\\Main Interface\\main_scroll.sti", 0},
    {"Data\\Main Interface\\main_textbox_back.sti", 0},
    {"Data\\Main Interface\\main_textbox_tabs.sti", 0},
    {"Data\\Main Interface\\icons_submenu.sti", 0},
    {"Data\\Main Interface\\icons_portrait.sti", 0},
    {"Data\\Main Interface\\icons_portrait_shield.sti", 0},
    {"Data\\Main Interface\\icons_portrait_jewels.sti", 0},
    {"Data\\Main Interface\\progressbar.sti", 0},
    {"Data\\Main Interface\\confirmation_buttons.sti", 0},
    {"Data\\Main Interface\\undo_button.sti", 0},
    {"Data\\Main Interface\\damage_splat_anim.sti", 0},
    {"Data\\Main Interface\\damage_splat_anim2.sti", 0},
    {"Data\\Main Interface\\death_splat_anim.sti", 0},
    {"Data\\Main Interface\\partymovement_back.sti", 0},
    {"Data\\Main Interface\\partymovement_buttons.sti", 0},
    {"Data\\Main Interface\\partymovement_man.sti", 0},
    {"Data\\Main Interface\\partymovement_bars.sti", 0},
    {"Data\\Main Interface\\item_bottompanel.sti", 0},
    {"Data\\Main Interface\\item_inventorypool.sti", 0},
    {"Data\\Party Formation\\small_formation_cage.sti", 0},
    {"Data\\Party Formation\\small_form_cage_high.sti", 0},
    {"Data\\Party Formation\\small_formation_cone.sti", 0},
    {"Data\\Party Formation\\small_formation_symbol.sti", 0},
    {"Data\\Party Formation\\small_formation_jewels.sti", 0},
    {"Data\\Party Formation\\large_formation_panel.sti", 0},
    {"Data\\Party Formation\\large_formation_symbol.sti", 0},
    {"Data\\Party Formation\\large_formation_jewels.sti", 0},
    {"Data\\Radar Compass\\compass_rim.sti", 0},
    {"Data\\Radar Compass\\compass_rim_high.sti", 0},
    {"Data\\Radar Compass\\formation_dot_large.sti", 0},
    {"Data\\Radar Compass\\formation_dot_small.sti", 0},
    {"Data\\Radar Compass\\formation_jewels.sti", 0},
    {"Data\\Main Interface\\CombatPortraitPanel.sti", 0},
    {"Data\\Main Interface\\leveling_button.sti", 0},
    {"Data\\Main Interface\\cond_icon_rim_wide.sti", 0},
    {"Data\\Main Interface\\paused_background.sti", 0},
    {"Data\\Spells\\Bitmaps\\default_2D_anim.sti", 0},
    {"Data\\Spells\\Bitmaps\\fire_positive.sti", 0},
    {"Data\\Spells\\Bitmaps\\fire_negative.sti", 0},
    {"Data\\Spells\\Bitmaps\\water_positive.sti", 0},
    {"Data\\Spells\\Bitmaps\\water_negative.sti", 0},
    {"Data\\Spells\\Bitmaps\\air_positive.sti", 0},
    {"Data\\Spells\\Bitmaps\\air_negative.sti", 0},
    {"Data\\Spells\\Bitmaps\\earth_positive.sti", 0},
    {"Data\\Spells\\Bitmaps\\earth_negative.sti", 0},
    {"Data\\Spells\\Bitmaps\\mental_positive.sti", 0},
    {"Data\\Spells\\Bitmaps\\mental_negative.sti", 0},
    {"Data\\Spells\\Bitmaps\\divine_positive.sti", 0},
    {"Data\\Spells\\Bitmaps\\divine_negative.sti", 0},
    {"Data\\Icons\\Conditions\\Drained.sti", 0},
    {"Data\\Icons\\Conditions\\Diseased.sti", 0},
    {"Data\\Icons\\Conditions\\Irritated.sti", 0},
    {"Data\\Icons\\Conditions\\Nauseated.sti", 0},
    {"Data\\Icons\\Conditions\\Slowed.sti", 0},
    {"Data\\Icons\\Conditions\\Afraid.sti", 0},
    {"Data\\Icons\\Conditions\\Poisoned.sti", 0},
    {"Data\\Icons\\Conditions\\Silenced.sti", 0},
    {"Data\\Icons\\Conditions\\Hexed.sti", 0},
    {"Data\\Icons\\Conditions\\Infatuated.sti", 0},
    {"Data\\Icons\\Conditions\\Insane.sti", 0},
    {"Data\\Icons\\Conditions\\Blind.sti", 0},
    {"Data\\Icons\\Conditions\\Turncoat.sti", 0},
    {"Data\\Icons\\Conditions\\Webbed.sti", 0},
    {"Data\\Icons\\Conditions\\Asleep.sti", 0},
    {"Data\\Icons\\Conditions\\Paralyzed.sti", 0},
    {"Data\\Icons\\Conditions\\Unconscious.sti", 0},
    {"Data\\Icons\\Conditions\\Dead.sti", 0},
    {"Data\\Icons\\Conditions\\Missing.sti", 0},
    {"Data\\Icons\\Enchantments\\Dracon_Breath.sti", 0},
    {"Data\\Icons\\Enchantments\\Guardian_Angel.sti", 0},
    {"Data\\Icons\\Enchantments\\Razor_Cloak.sti", 0},
    {"Data\\Icons\\Enchantments\\Eye4Eye.sti", 0},
    {"Data\\Icons\\Enchantments\\Haste.sti", 0},
    {"Data\\Icons\\Enchantments\\Super_Man.sti", 0},
    {"Data\\Icons\\Enchantments\\Body_Stone.sti", 0},
    {"Data\\Icons\\TravelingSpells\\armor_plate.sti", 0},
    {"Data\\Icons\\TravelingSpells\\chameleon.sti", 0},
    {"Data\\Icons\\TravelingSpells\\detect_secret.sti", 0},
    {"Data\\Icons\\TravelingSpells\\enchanted_blade.sti", 0},
    {"Data\\Icons\\TravelingSpells\\light.sti", 0},
    {"Data\\Icons\\TravelingSpells\\magic_screen.sti", 0},
    {"Data\\Icons\\TravelingSpells\\missile_shield.sti", 0},
    {"Data\\Icons\\TravelingSpells\\shadow_hound.sti", 0},
    {"Data\\Icons\\TravelingSpells\\x_ray.sti", 0},
    {"Data\\Icons\\PartySpells\\Armor_Melt.sti", 0},
    {"Data\\Icons\\PartySpells\\Acid_Cloud.sti", 0},
    {"Data\\Icons\\PartySpells\\Toxic_Cloud.sti", 0},
    {"Data\\Icons\\PartySpells\\Fire_Storm.sti", 0},
    {"Data\\Icons\\PartySpells\\Death_Cloud.sti", 0},
    {"Data\\Icons\\PartySpells\\Draining_Cloud.sti", 0},
    {"Data\\Icons\\PartySpells\\Bless.sti", 0},
    {"Data\\Icons\\PartySpells\\Element_Shield.sti", 0},
    {"Data\\Icons\\PartySpells\\Soul_Shield.sti", 0},
    {"Data\\Icons\\PartySpells\\Ring_Of_Fire.sti", 0},
    {"Data\\Options\\intro_bg.sti", 1},
    {"Data\\Options\\intro_bg_menu.sti", 1},
    {"Data\\Options\\intro_menu.sti", 0},
    {"Data\\Options\\opt_menu.sti", 0},
    {"Data\\Options\\intro_bg_text.sti", 1},
    {"Data\\Options\\intro_bg_credits.sti", 1},
    {"Data\\Options\\introtext_normal.sti", 0},
    {"Data\\Options\\introtext_depressed.sti", 0},
    {"Data\\Options\\introtext_highlight.sti", 0},
    {"Data\\Options\\introtext_unavailable.sti", 0},
    {"Data\\Options\\options_base.sti", 0},
    {"Data\\Options\\options_buttons.sti", 0},
    {"Data\\Options\\options_computer.sti", 0},
    {"Data\\Options\\options_controls.sti", 0},
    {"Data\\Options\\options_submenu.sti", 0},
    {"Data\\Options\\options_pagingbase.sti", 0},
    {"Data\\Options\\options_navigation_arrows.sti", 0},
    {"Data\\Options\\options_slider.sti", 0},
    {"Data\\Options\\save_load_defaultscreen.sti", 0},
    {"Data\\Options\\save_load_back.sti", 0},
    {"Data\\Options\\save_load_buttons.sti", 0},
    {"Data\\Options\\save_load_gamelists.sti", 0},
    {"Data\\Party Generation\\back.sti", 0},
    {"Data\\Party Generation\\listboxes.sti", 0},
    {"Data\\Party Generation\\detailbox.sti", 0},
    {"Data\\Party Generation\\bottombuttons.sti", 0},
    {"Data\\Party Generation\\listbuttons.sti", 0},
    {"Data\\Party Generation\\scrollbuttons.sti", 0},
    {"Data\\Party Generation\\portraitborder.sti", 0},
    {"Data\\Party Generation\\portraithighlight.sti", 0},
    {"Data\\Party Generation\\options_back.sti", 0},
    {"Data\\Party Generation\\import_addon.sti", 0},
    {"Data\\Char Generation\\CG_Profession.sti", 0},
    {"Data\\Char Generation\\CG_Personality.sti", 0},
    {"Data\\Char Generation\\CG_BottomButtons.sti", 0},
    {"Data\\Char Generation\\CG_Pieces.sti", 0},
    {"Data\\Char Generation\\CG_Skills.sti", 0},
    {"Data\\Char Generation\\CG_Magic.sti", 0},
    {"Data\\Char Generation\\CG_Buttons.sti", 0},
    {"Data\\Char Generation\\CG_Icons_Profession.sti", 0},
    {"Data\\Char Generation\\CG_Icons_Race.sti", 0},
    {"Data\\Char Generation\\CG_Icons_Gender.sti", 0},
    {"Data\\Char Generation\\CG_Icons_Base.sti", 0},
    {"Data\\Review\\CommonCorner.sti", 0},
    {"Data\\Review\\BottomButtonBar.sti", 0},
    {"Data\\Review\\ReviewPageButtons.sti", 0},
    {"Data\\Review\\ReviewItemButtons.sti", 0},
    {"Data\\Review\\dismiss_button.sti", 0},
    {"Data\\Review\\ReviewItemPage.sti", 0},
    {"Data\\Review\\ReviewItemHighlights.sti", 0},
    {"Data\\Review\\PortraitExit.sti", 0},
    {"Data\\Review\\SmallPortraitHighlight.sti", 0},
    {"Data\\Review\\ReviewGoldIcon.sti", 0},
    {"Data\\Review\\BoxesOnly.sti", 0},
    {"Data\\Review\\ItemUsableBackground.sti", 0},
    {"Data\\Review\\ItemUnidentifiedOverlay.sti", 0},
    {"Data\\Review\\AC_Ovals.sti", 0},
    {"Data\\Review\\Leveling_bar.sti", 0},
    {"Data\\Review\\InventorySwapButtons.sti", 0},
    {"Data\\Review\\InventoryFilterButtons.sti", 0},
    {"Data\\Review\\commod_button_bar.sti", 0},
    {"Data\\Review\\commod_buttons.sti", 0},
    {"Data\\Review\\commod_back.sti", 0},
    {"Data\\Review\\reviewscreen_popups.sti", 0},
    {"Data\\Review\\InvCursedHighlight.sti", 0},
    {"Data\\Review\\statue_human_m.sti", 0},
    {"Data\\Review\\statue_human_f.sti", 0},
    {"Data\\Review\\statue_elf_m.sti", 0},
    {"Data\\Review\\statue_elf_f.sti", 0},
    {"Data\\Review\\statue_dwarf_m.sti", 0},
    {"Data\\Review\\statue_dwarf_f.sti", 0},
    {"Data\\Review\\statue_gnome_m.sti", 0},
    {"Data\\Review\\statue_gnome_f.sti", 0},
    {"Data\\Review\\statue_hobbit_m.sti", 0},
    {"Data\\Review\\statue_hobbit_f.sti", 0},
    {"Data\\Review\\statue_fairy_m.sti", 0},
    {"Data\\Review\\statue_fairy_f.sti", 0},
    {"Data\\Review\\statue_lizard_m.sti", 0},
    {"Data\\Review\\statue_lizard_f.sti", 0},
    {"Data\\Review\\statue_dracon_m.sti", 0},
    {"Data\\Review\\statue_dracon_f.sti", 0},
    {"Data\\Review\\statue_felpurr_m.sti", 0},
    {"Data\\Review\\statue_felpurr_f.sti", 0},
    {"Data\\Review\\statue_rawulf_m.sti", 0},
    {"Data\\Review\\statue_rawulf_f.sti", 0},
    {"Data\\Review\\statue_mook_m.sti", 0},
    {"Data\\Review\\statue_mook_f.sti", 0},
    {"Data\\Review\\statue_Trynnie.sti", 0},
    {"Data\\Review\\statue_Trang.sti", 0},
    {"Data\\Review\\statue_Umpani.sti", 0},
    {"Data\\Review\\statue_Rapax.sti", 0},
    {"Data\\Review\\statue_Android.sti", 0},
    {"Data\\Review\\ReviewMagicPage.sti", 0},
    {"Data\\Review\\ReviewSkillsPage.sti", 0},
    {"Data\\Review\\ReviewStatsPage.sti", 0},
    {"Data\\Review\\ReviewSliderbar.sti", 0},
    {"Data\\Review\\ReviewSkillsIcons.sti", 0},
    {"Data\\Review\\ReviewFXFilters.sti", 0},
    {"Data\\Review\\SecondaryOccupied.sti", 0},
    {"Data\\Review\\ButtonBackgroundOff.sti", 0},
    {"Data\\Review\\ButtonBackgroundOn.sti", 0},
    {"Data\\Review\\ButtonHighlight.sti", 0},
    {"Data\\Fonts\\PaletteRed.sti", 0},
    {"Data\\Fonts\\PaletteGreen.sti", 0},
    {"Data\\Fonts\\PalettePurple.sti", 0},
    {"Data\\Fonts\\PaletteBlue.sti", 0},
    {"Data\\Fonts\\PaletteOrange.sti", 0},
    {"Data\\Fonts\\PaletteYellow.sti", 0},
    {"Data\\Fonts\\PalettePink.sti", 0},
    {"Data\\Fonts\\PaletteBrown.sti", 0},
    {"Data\\Fonts\\PaletteWhite.sti", 0},
    {"Data\\Fonts\\PaletteRust.sti", 0},
    {"Data\\Fonts\\PaletteBronze.sti", 0},
    {"Data\\Fonts\\PaletteGray.sti", 0},
    {"Data\\Fonts\\PaletteBeige.sti", 0},
    {"Data\\Fonts\\PaletteOptGreen.sti", 0},
    {"Data\\Fonts\\PaletteOptWhite.sti", 0},
    {"Data\\Flics\\Realms\\Fire.sti", 0},
    {"Data\\Flics\\Realms\\Water.sti", 0},
    {"Data\\Flics\\Realms\\Air.sti", 0},
    {"Data\\Flics\\Realms\\Earth.sti", 0},
    {"Data\\Flics\\Realms\\Mental.sti", 0},
    {"Data\\Flics\\Realms\\Divine.sti", 0},
    {"Data\\Automap\\map_border.sti", 0},
    {"Data\\Automap\\map_cursors.sti", 0},
    {"Data\\Automap\\map_exitbutton.sti", 0},
    {"Data\\Automap\\map_layerbuttons.sti", 0},
    {"Data\\Automap\\map_markerbuttons.sti", 0},
    {"Data\\Automap\\map_movementbuttons.sti", 0},
    {"Data\\Automap\\map_notebuttons.sti", 0},
    {"Data\\Automap\\map_zoombuttons.sti", 0},
    {"Data\\Spell Casting\\MagicBottomNew.sti", 0},
    {"Data\\Spell Casting\\RealmHighlightNew.sti", 0},
    {"Data\\Spell Casting\\PowerButtons.sti", 0},
    {"Data\\Spell Casting\\fire_realm.sti", 0},
    {"Data\\Spell Casting\\water_realm.sti", 0},
    {"Data\\Spell Casting\\air_realm.sti", 0},
    {"Data\\Spell Casting\\earth_realm.sti", 0},
    {"Data\\Spell Casting\\mental_realm.sti", 0},
    {"Data\\Spell Casting\\divine_realm.sti", 0},
    {"Data\\Spell Casting\\MagicBottom.sti", 0},
    {"Data\\Spell Casting\\ReviewSpellCast.sti", 0},
    {"Data\\Spell Casting\\SmallPowerBalls.sti", 0},
    {"Data\\Spell Casting\\SmallPowerRings.sti", 0},
    {"Data\\Spell Casting\\LargePowerBalls.sti", 0},
    {"Data\\Spell Casting\\LargePowerRings.sti", 0},
    {"Data\\Spell Casting\\CastCancelButtons.sti", 0},
    {"Data\\Spell Casting\\RealmHighlight.sti", 0},
    {"Data\\NPC Interaction\\Npc_bottompanel.sti", 0},
    {"Data\\NPC Interaction\\Npc_buttons.sti", 0},
    {"Data\\NPC Interaction\\Npc_itemfilters.sti", 0},
    {"Data\\NPC Interaction\\Npc_gold.sti", 0},
    {"Data\\Locks And Traps\\LockPickMockup.sti", 0},
    {"Data\\Locks And Traps\\lockpick_tumblers.sti", 0},
    {"Data\\Locks And Traps\\lockpick_bottompanel.sti", 0},
    {"Data\\Locks And Traps\\lockpick_bashbutton.sti", 0},
    {"Data\\Locks And Traps\\traps_bottompanel.sti", 0},
    {"Data\\Locks And Traps\\traps_deviceicons.sti", 0},
    {"Data\\Locks And Traps\\traps_devicehighlight.sti", 0},
    {"Data\\Locks And Traps\\traps_buttonglow.sti", 0},
    {"Data\\Locks And Traps\\inspecting_bar.sti", 0},
    {"Data\\Main Interface\\basic_fill_texture.pcx", 1},
    {"Data\\Journal\\journal_base.sti", 0},
    {"Data\\Journal\\journal_parchment.sti", 0},
    {"Data\\Journal\\journal_whitepal.sti", 0},
    {"Data\\Journal\\journal_redpal.sti", 0},
    {"Data\\Journal\\Faction_ratings_button.sti", 0},
    {"Data\\Level Load\\Arnika.sti", 1},
    {"Data\\Level Load\\Ascension Peak.sti", 1},
    {"Data\\Level Load\\Bayjin.sti", 1},
    {"Data\\Level Load\\Bayjin Shallows.sti", 1},
    {"Data\\Level Load\\Cosmic Circle.sti", 1},
    {"Data\\Level Load\\Marten's Bluff.sti", 1},
    {"Data\\Level Load\\Marten's Underground.sti", 1},
    {"Data\\Level Load\\Mine Tunnels.sti", 1},
    {"Data\\Level Load\\LowerMonastery.sti", 1},
    {"Data\\Level Load\\UpperMonastery.sti", 1},
    {"Data\\Level Load\\MtGigas Water Caves.sti", 1},
    {"Data\\Level Load\\MtGigas Caves.sti", 1},
    {"Data\\Level Load\\MtGigas Upper Caves.sti", 1},
    {"Data\\Level Load\\Umpani Base Camp.sti", 1},
    {"Data\\Level Load\\MtGigas Peak.sti", 1},
    {"Data\\Level Load\\Wilderness Clearing.sti", 1},
    {"Data\\Level Load\\Rapax Castle Cellar.sti", 1},
    {"Data\\Level Load\\Rapax Castle Main Level.sti", 1},
    {"Data\\Level Load\\Upper Rapax Castle.sti", 1},
    {"Data\\Level Load\\Rapax Courtyard.sti", 1},
    {"Data\\Level Load\\Rapax Rift.sti", 1},
    {"Data\\Level Load\\Sea Caves.sti", 1},
    {"Data\\Level Load\\Swamp.sti", 1},
    {"Data\\Level Load\\Trynton.sti", 1},
    {"Data\\Level Load\\Trynton Upper Branches.sti", 1},
    {"Data\\Level Load\\Arnika-Trynton Road.sti", 1},
    {"Data\\Level Load\\South East Wilderness.sti", 1},
    {"Data\\Level Load\\Mountain Wilderness.sti", 1},
    {"Data\\Level Load\\Northern Wilderness.sti", 1},
    {"Data\\Level Load\\Arnika Road.sti", 1},
    {"Data\\Level Load\\SavantTower.sti", 1},
    {"Data\\Level Load\\Rattkin Tree.sti", 1},
    {"Data\\Level Load\\Wilderness Clearing2.sti", 1},
    {"Data\\Level Load\\levelload_gears.sti", 0},
    {"Data\\Level Load\\levelload_textbar.sti", 0},
    {"Data\\Level Load\\DeathScreen.sti", 1},
    {"Data\\Level Load\\Camping Screen.sti", 1},
    {"Data\\Level Load\\DarkLord.sti", 1},
    {"Data\\Level Load\\Cosmic ending.sti", 1},
    {"Data\\Level Load\\BlowOut.sti", 1},
    {"Data\\Options\\Falcon.sti", 0},
};
#pragma clang diagnostic pop

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
// GLOBAL: WIZ8 0x006448c8
W8VideoObjectSlot g_video_slots_6448c8[494] = {
    {0, 0},
    {0, 1},
    {0, 2},
    {0, 7},
    {0, 12},
    {0, 13},
    {0, 14},
    {0, 18},
    {0, 22},
    {0, 23},
    {0, 24},
    {0, 29},
    {0, 30},
    {0, 31},
    {0, 35},
    {0, 36},
    {0, 37},
    {1, 0},
    {81, 0},
    {161, 0},
    {241, 0},
    {242, 0},
    {243, 0},
    {244, 0},
    {245, 0},
    {246, 0},
    {247, 0},
    {248, 0},
    {249, 0},
    {250, 0},
    {251, 0},
    {252, 0},
    {253, 0},
    {254, 0},
    {255, 0},
    {256, 0},
    {257, 0},
    {258, 0},
    {259, 0},
    {260, 0},
    {261, 0},
    {262, 0},
    {277, 0},
    {275, 0},
    {276, 0},
    {279, 0},
    {280, 0},
    {281, 0},
    {282, 0},
    {283, 0},
    {284, 0},
    {285, 0},
    {286, 0},
    {265, 0},
    {264, 0},
    {263, 0},
    {268, 0},
    {267, 0},
    {266, 0},
    {271, 0},
    {270, 0},
    {269, 0},
    {274, 0},
    {273, 0},
    {272, 0},
    {278, 0},
    {287, 0},
    {288, 0},
    {289, 0},
    {290, 0},
    {291, 0},
    {292, 0},
    {293, 0},
    {294, 0},
    {295, 0},
    {296, 0},
    {297, 0},
    {298, 0},
    {299, 0},
    {300, 0},
    {301, 0},
    {302, 0},
    {303, 0},
    {304, 0},
    {305, 0},
    {306, 0},
    {307, 0},
    {308, 0},
    {309, 0},
    {310, 0},
    {311, 0},
    {312, 0},
    {313, 0},
    {314, 0},
    {315, 0},
    {316, 0},
    {317, 0},
    {318, 0},
    {319, 0},
    {320, 0},
    {321, 0},
    {322, 0},
    {323, 0},
    {324, 0},
    {325, 0},
    {326, 0},
    {327, 0},
    {328, 0},
    {329, 0},
    {330, 0},
    {331, 0},
    {332, 0},
    {333, 0},
    {334, 0},
    {335, 0},
    {336, 0},
    {337, 0},
    {338, 0},
    {339, 0},
    {340, 0},
    {341, 0},
    {345, 0},
    {346, 0},
    {344, 0},
    {342, 0},
    {343, 0},
    {347, 0},
    {348, 0},
    {349, 0},
    {350, 0},
    {351, 0},
    {352, 0},
    {353, 0},
    {354, 0},
    {355, 0},
    {356, 0},
    {357, 0},
    {358, 0},
    {359, 0},
    {360, 0},
    {361, 0},
    {362, 0},
    {363, 0},
    {364, 0},
    {365, 0},
    {366, 0},
    {367, 0},
    {368, 0},
    {369, 0},
    {370, 0},
    {371, 0},
    {372, 0},
    {373, 0},
    {374, 0},
    {375, 0},
    {376, 0},
    {377, 0},
    {378, 0},
    {379, 0},
    {380, 0},
    {381, 0},
    {382, 0},
    {383, 0},
    {384, 0},
    {385, 0},
    {386, 0},
    {387, 0},
    {388, 0},
    {389, 0},
    {390, 0},
    {391, 0},
    {392, 0},
    {393, 0},
    {394, 0},
    {395, 0},
    {396, 0},
    {397, 0},
    {398, 0},
    {399, 0},
    {400, 0},
    {401, 0},
    {402, 0},
    {403, 0},
    {404, 0},
    {405, 0},
    {406, 0},
    {407, 0},
    {408, 0},
    {409, 0},
    {410, 0},
    {411, 0},
    {412, 0},
    {413, 0},
    {414, 0},
    {415, 0},
    {416, 0},
    {417, 0},
    {418, 0},
    {419, 0},
    {420, 0},
    {421, 0},
    {422, 0},
    {423, 0},
    {424, 0},
    {425, 0},
    {426, 0},
    {427, 0},
    {428, 0},
    {429, 0},
    {430, 0},
    {431, 0},
    {432, 0},
    {433, 0},
    {434, 0},
    {435, 0},
    {436, 0},
    {437, 0},
    {438, 0},
    {439, 0},
    {440, 0},
    {441, 0},
    {442, 0},
    {443, 0},
    {444, 0},
    {445, 0},
    {446, 0},
    {447, 0},
    {448, 0},
    {449, 0},
    {450, 0},
    {451, 0},
    {452, 0},
    {453, 0},
    {454, 0},
    {455, 0},
    {456, 0},
    {457, 0},
    {458, 0},
    {459, 0},
    {460, 0},
    {461, 0},
    {462, 0},
    {463, 0},
    {464, 0},
    {465, 0},
    {466, 0},
    {467, 0},
    {468, 0},
    {469, 0},
    {470, 0},
    {471, 0},
    {472, 0},
    {473, 0},
    {474, 0},
    {475, 0},
    {476, 0},
    {477, 0},
    {478, 0},
    {479, 0},
    {480, 0},
    {481, 0},
    {482, 0},
    {483, 0},
    {484, 0},
    {485, 0},
    {486, 0},
    {487, 0},
    {488, 0},
    {489, 0},
    {490, 0},
    {491, 0},
    {492, 0},
    {493, 0},
    {494, 0},
    {495, 0},
    {496, 0},
    {497, 0},
    {498, 0},
    {499, 0},
    {500, 0},
    {501, 0},
    {502, 0},
    {503, 0},
    {504, 0},
    {505, 0},
    {506, 0},
    {507, 0},
    {508, 0},
    {509, 0},
    {510, 0},
    {511, 0},
    {512, 0},
    {513, 0},
    {514, 0},
    {515, 0},
    {516, 0},
    {517, 0},
    {518, 0},
    {519, 0},
    {520, 0},
    {521, 0},
    {522, 0},
    {523, 0},
    {524, 0},
    {525, 0},
    {526, 0},
    {527, 0},
    {528, 0},
    {529, 0},
    {530, 0},
    {531, 0},
    {532, 0},
    {533, 0},
    {534, 0},
    {535, 0},
    {536, 0},
    {537, 0},
    {538, 0},
    {539, 0},
    {540, 0},
    {541, 0},
    {542, 0},
    {543, 0},
    {544, 0},
    {545, 0},
    {546, 0},
    {547, 0},
    {548, 0},
    {549, 0},
    {550, 0},
    {572, 0},
    {573, 0},
    {573, 1},
    {573, 2},
    {573, 3},
    {573, 4},
    {574, 0},
    {574, 1},
    {574, 2},
    {574, 3},
    {575, 0},
    {575, 1},
    {575, 2},
    {575, 3},
    {575, 4},
    {575, 5},
    {575, 6},
    {575, 7},
    {576, 0},
    {576, 1},
    {576, 2},
    {576, 3},
    {576, 4},
    {576, 5},
    {576, 6},
    {576, 7},
    {576, 8},
    {576, 9},
    {576, 10},
    {576, 11},
    {577, 0},
    {577, 1},
    {577, 2},
    {577, 3},
    {577, 4},
    {577, 5},
    {577, 6},
    {577, 7},
    {577, 8},
    {577, 9},
    {577, 10},
    {577, 11},
    {577, 12},
    {577, 13},
    {577, 14},
    {577, 15},
    {577, 16},
    {577, 17},
    {577, 18},
    {577, 19},
    {578, 0},
    {578, 1},
    {578, 2},
    {578, 3},
    {578, 4},
    {578, 5},
    {578, 6},
    {578, 7},
    {579, 0},
    {579, 1},
    {579, 2},
    {579, 3},
    {579, 4},
    {579, 5},
    {579, 6},
    {579, 7},
    {579, 8},
    {579, 9},
    {579, 10},
    {579, 11},
    {580, 0},
    {581, 0},
    {582, 0},
    {583, 0},
    {584, 0},
    {585, 0},
    {586, 0},
    {587, 0},
    {588, 0},
    {589, 0},
    {590, 0},
    {591, 0},
    {591, 8},
    {592, 0},
    {593, 0},
    {593, 8},
    {594, 0},
    {595, 0},
    {595, 1},
    {595, 2},
    {595, 3},
    {595, 4},
    {595, 5},
    {596, 0},
    {596, 1},
    {597, 0},
    {598, 0},
    {599, 0},
    {600, 0},
    {601, 0},
    {602, 0},
    {603, 0},
    {604, 0},
    {605, 0},
    {606, 0},
    {607, 0},
    {608, 0},
    {609, 0},
    {610, 0},
    {611, 0},
    {612, 0},
    {613, 0},
    {614, 0},
    {615, 0},
    {616, 0},
    {617, 0},
    {618, 0},
    {619, 0},
    {620, 0},
    {621, 0},
    {622, 0},
    {623, 0},
    {624, 0},
    {625, 0},
    {626, 0},
    {627, 0},
    {628, 0},
    {629, 0},
    {630, 0},
    {631, 0},
    {632, 0},
    {633, 0},
    {634, 0},
    {635, 0},
    {636, 0},
    {637, 0},
    {638, 0},
    {639, 0},
    {640, 0},
    {641, 0},
    {642, 0},
    {643, 0},
    {644, 0},
    {645, 0},
    {646, 0},
    {647, 0},
    {648, 0},
    {649, 0},
    {650, 0},
    {651, 0},
    {652, 0},
    {653, 0},
    {654, 0},
    {655, 0},
    {656, 0},
    {551, 0},
    {566, 0},
    {567, 0},
    {568, 0},
    {569, 0},
    {570, 0},
    {571, 0},
};
#pragma clang diagnostic pop


/* The two loaders consume the released SGP object and surface request records.
   Their 0x6c and 0x70 sizes account exactly for this function's 0xe0-byte pair
   of stack objects. */

#define VIDEO_OBJECT_MANAGER_CPP "C:\\Projects\\Wizardry 8\\Local Code\\VideoObjectManager.cpp"

// FUNCTION: WIZ8 0x00549250
void ReleaseLoadedVideoFrames(void)
{
    W8VideoFrame* frame;
    char released;

    if (!gfVideoObjectsInit) {
        srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP, 0x9c, 0);
    }
    frame = g_video_frames_62c430;
    do {
        if (frame->loaded) {
            if (frame->mode == 0) {
                released = DeleteVideoObjectFromIndex(frame->handle);
            } else {
                released = DeleteVideoSurfaceFromIndex(frame->handle);
            }
            if (!released) {
                srAssertFail("fReturnCode", VIDEO_OBJECT_MANAGER_CPP, 0x85, 0);
            }
            frame->handle = 0;
            frame->loaded = 0;
        }
        ++frame;
    } while (&frame->handle < (unsigned int*)(g_video_frames_62c430 + 1658));
}

// FUNCTION: WIZ8 0x00548f90
void DrawCatalogImage(int target, int object, int frame, short image,
                      int left, int top, int mode, int flags)
{
    W8VideoObjectSlot* slot;
    short row;
    unsigned int surface;
    char ok;

    if (!gfVideoObjectsInit) {
        srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP, 0x2d, 0);
    }
    EnsureCatalogFrameLoaded(object, frame);
    /* The slot address is held; the frame index is not. The original recomputes
       first_frame + frame for each of the two frame reads rather than keeping
       it, and the vertical offset is added to the caller's row in sixteen bits -
       both are shorts and the original adds them as such. */
    slot = &g_video_slots_6448c8[object];
    row = slot->y_offset + image;
    surface = g_video_frames_62c430[slot->first_frame + frame].handle;
    if (!gfVideoObjectsInit) {
        srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP, 0xdd, 0);
    }
    if (g_video_frames_62c430[slot->first_frame + frame].mode == 0) {
        ok = BltVideoObjectFromIndex(
            target, surface, row, left, top, mode, (blt_fx*)flags);
    } else {
        ok = BltVideoSurface(target, surface, row, left, top, mode, 0);
    }
    if (!ok) {
        srAssertFail("fReturnCode", VIDEO_OBJECT_MANAGER_CPP, 0x3e, 0);
    }
}


/* Loads one frame's surface the first time it is drawn. The path comes out of
   the frame record itself, and the mode picks which loader receives it. A
   failure does not return - it formats the path and the mode into the
   assertion's message. */
// FUNCTION: WIZ8 0x00549090
void EnsureCatalogFrameLoaded(int object, int frame)
{
    VOBJECT_DESC request_a;
    VSURFACE_DESC request_b;
    W8VideoFrame* record;
    unsigned int handle;
    char loaded_ok;

    /* Two nested checks, both in the original: the assertion does not return,
       so the inner one is reachable only when it is compiled out. */
    if (!gfVideoObjectsInit) {
        srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP, 0x4c, 0);
        if (!gfVideoObjectsInit) {
            srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP, 0xd4, 0);
        }
    }
    record = &g_video_frames_62c430[g_video_slots_6448c8[object].first_frame + frame];
    if (record->loaded == 0) {
        if (!gfVideoObjectsInit) {
            srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP, 0xdd, 0);
        }
        if (record->mode == 0) {
            request_a.fCreateFlags = VOBJECT_CREATE_FROMFILE;
            strcpy(request_a.ImageFile, record->path);
            loaded_ok = AddVideoObject(&request_a, &handle);
        } else {
            request_b.fCreateFlags = VSURFACE_CREATE_FROMFILE;
            strcpy(request_b.ImageFile, record->path);
            loaded_ok = AddVideoSurface(&request_b, &handle);
        }
        if (loaded_ok == 0) {
            if (!gfVideoObjectsInit) {
                srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP, 0xdd, 0);
            }
            srAssertFail("fReturnCode", VIDEO_OBJECT_MANAGER_CPP, 0x68,
                         FormatString("LoadVideoObject: ERROR - Add %s failed, type %d",
                                      record->path, record->mode));
        }
        record->handle = handle;
        record->loaded = 1;
    }
}

/* Copies the loaded frame's released-SGP palette into an owned 256-entry
   table.  The allocation is intentionally retained when the source API says
   the object has no palette, matching the shipped failure path. */
// FUNCTION: WIZ8 0x005492e0
unsigned short* CopyCatalogImagePalette16BPP(int object, int frame)
{
    unsigned short* palette;

    if (!gfVideoObjectsInit) {
        srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP,
                     0xaf, 0);
    }
    palette = (unsigned short*)malloc(0x200);
    if (!palette) {
        return 0;
    }
    EnsureCatalogFrameLoaded(object, frame);
    if (!gfVideoObjectsInit) {
        srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP,
                     0xe6, 0);
    }
    if (!CopyVideoObjectPalette16BPP(
            g_video_frames_62c430[
                g_video_slots_6448c8[object].first_frame + frame].handle,
            palette)) {
        return 0;
    }
    return palette;
}

// FUNCTION: WIZ8 0x00549390
unsigned int GetCatalogVideoObjectHandle(int object, int frame)
{
    if (!gfVideoObjectsInit) {
        srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP,
                     0xe6, 0);
    }
    EnsureCatalogFrameLoaded(object, frame);
    return g_video_frames_62c430[
        g_video_slots_6448c8[object].first_frame + frame].handle;
}

// FUNCTION: WIZ8 0x005493e0
short GetCatalogVideoObjectYOffset(int object)
{
    if (!gfVideoObjectsInit) {
        srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP,
                     0xf2, 0);
    }
    return g_video_slots_6448c8[object].y_offset;
}

/* Mark the complete rectangle covered by one catalog image. ETRLE-backed
   objects supply subimage dimensions through the canonical SGP object API;
   surface-backed objects expose the dimensions on the canonical SGP surface
   record returned by GetVideoSurface. */
// FUNCTION: WIZ8 0x005494f0
void InvalidateCatalogImageRect(int object, int frame, int image,
                                int left, int top, int flags)
{
    W8VideoObjectSlot* slot;
    W8VideoFrame* record;
    short subimage;
    unsigned short width = 0;
    unsigned short height = 0;

    EnsureCatalogFrameLoaded(object, frame);
    slot = &g_video_slots_6448c8[object];
    subimage = slot->y_offset + image;
    record = &g_video_frames_62c430[slot->first_frame + frame];
    if (!gfVideoObjectsInit) {
        srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP,
                     0xdd, 0);
    }
    if (record->mode == 0) {
        GetVideoObjectETRLESubregionProperties(
            record->handle, subimage, &width, &height);
    }
    else {
        HVSURFACE surface;
        if (GetVideoSurface(&surface, record->handle)) {
            height = surface->usHeight;
            width = surface->usWidth;
        }
    }

    if (width != 0 && height != 0) {
        InvalidateRegion(
            left, top, left + width, top + height, flags);
    }
}

/* Draw a catalog video object, then mark the area it covered. The vertical
   argument is truncated to a short for the draw and passed whole to the mark,
   and the seventh reaches the mark only as whether it equals two. */
// FUNCTION: WIZ8 0x00549600
void DrawCatalogImageAndInvalidate(int target, int object, int frame, int image,
                                   int left, int top, int mode, int flags)
{
    DrawCatalogImage(
        target, object, frame, (short)image, left, top, mode, flags);
    InvalidateCatalogImageRect(object, frame, image, left, top, mode == 2);
}

/* Loads the selected catalog frame and returns the dimensions of one of its
   ETRLE subimages. Surface-backed records have no ETRLE table, so the retail
   body intentionally leaves the caller's outputs untouched for them. */
// FUNCTION: WIZ8 0x00549660
void GetCatalogImageSize(int object, int frame, int image,
                         short* width, short* height)
{
    W8VideoObjectSlot* slot;
    W8VideoFrame* record;
    short subimage;

    EnsureCatalogFrameLoaded(object, frame);
    slot = &g_video_slots_6448c8[object];
    subimage = slot->y_offset + image;
    record = &g_video_frames_62c430[slot->first_frame + frame];
    if (!gfVideoObjectsInit) {
        srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP,
                     0xdd, 0);
    }
    if (record->mode == 0) {
        GetVideoObjectETRLESubregionProperties(
            record->handle, subimage,
            (unsigned short*)width, (unsigned short*)height);
    }
}

/* The image's own offset inside its frame, read from the video object's ETRLE
   table; only uncompressed frames carry one. */
// FUNCTION: WIZ8 0x00549700
void GetCatalogImagePosition00549700(
    int object, int frame, int image, short* x, short* y)
{
    W8VideoObjectSlot* slot;
    W8VideoFrame* record;
    HVOBJECT video_object;
    short subimage;

    EnsureCatalogFrameLoaded(object, frame);
    slot = &g_video_slots_6448c8[object];
    subimage = slot->y_offset + image;
    record = &g_video_frames_62c430[slot->first_frame + frame];
    if (!gfVideoObjectsInit) {
        srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP,
                     0xdd, 0);
    }
    if (record->mode == 0) {
        GetVideoObject(&video_object, record->handle);
        *x = video_object->pETRLEObject[subimage].sOffsetX;
        *y = video_object->pETRLEObject[subimage].sOffsetY;
    }
}

/* Copy a rectangle from one catalog-owned 8-bit surface into a 16-bit target.
   The source rectangle has the destination's extent and begins at the two
   caller-provided source coordinates. Both surfaces remain locked for exactly
   the pinned SGP conversion call. */
// FUNCTION: WIZ8 0x005497c0
unsigned char BlitCatalogSurfaceRectTo16BPP(
    int target, int left, int top, int right, int bottom, int object,
    int source_x, int source_y)
{
    SGPRect source_rect;
    HVSURFACE source_surface;
    unsigned int target_pitch;
    unsigned int source_pitch;
    unsigned int source_handle;
    unsigned char* target_pixels;
    unsigned char* source_pixels;

    source_rect.iLeft = source_x;
    source_rect.iTop = source_y;
    source_rect.iRight = source_x + right - left;
    source_rect.iBottom = source_y + bottom - top;

    if (!gfVideoObjectsInit) {
        srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP,
                     0x19d, 0);
    }
    EnsureCatalogFrameLoaded(object, 0);
    source_handle =
        g_video_frames_62c430[g_video_slots_6448c8[object].first_frame].handle;
    GetVideoSurface(&source_surface, source_handle);
    target_pixels = LockVideoSurface(target, &target_pitch);
    source_pixels = LockVideoSurface(source_handle, &source_pitch);
    Blt8BPPDataSubTo16BPPBuffer(
        reinterpret_cast<unsigned short*>(target_pixels), target_pitch,
        source_surface, source_pixels, source_pitch, left, top, &source_rect);
    UnLockVideoSurface(target);
    UnLockVideoSurface(source_handle);
    return 1;
}
