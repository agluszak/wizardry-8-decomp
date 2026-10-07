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
W8VideoFrame g_video_frames[1658] = {
    {"Data\\Cursors\\2D-Cursors.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Large\\Lhummf.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lhummn.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lhummm.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lhumm1.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lhumm2.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lhumm3.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lhumff.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lhumfn.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lhumfm.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lhumf1.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lhumf2.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lhumf3.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lelfmf.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lelfmn.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lelfmm.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lelfff.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lelffn.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lelffm.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Ldwarfmf.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Ldwarfmn.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Ldwarfmm.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Ldwarfff.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Ldwarffn.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Ldwarffm.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lgnomemn.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lgnomemm.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lgnomeff.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lgnomefn.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lhobmf.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lhobmn.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lhobff.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lhobfn.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lfairymf.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lfairymm.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lfairyff.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lfairyfn.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Llizmf.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Llizmn.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Llizff.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Llizfn.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Ldracmf.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Ldracmn.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Ldracff.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Ldracfn.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lfelpmf.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lfelpmn.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lfelpff.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lfelpfn.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lrawmn.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lrawmm.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lrawff.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lrawfn.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lmookmf.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lmookmn.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lmookff.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lmookfn.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lninmf.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lninff.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lmook.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Ltrynm1.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Ltrynm2.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Ltrynm3.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Ltrang.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lumpani.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lsexus.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lurq.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\LRFS-81.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lmadras.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lsparkle.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lmyles.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\LVi.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Ldrazic.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Ltantris.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lrodan.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lglumph.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lsaxx.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lhumm4.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lhumf4.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lelfm1.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Large\\Lelff1.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Medium\\Mhummf.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Mhummn.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Mhummm.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\mhumm1.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\mhumm2.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\mhumm3.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Mhumff.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Mhumfn.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Mhumfm.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\mhumf1.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\mhumf2.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\mhumf3.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Melfmf.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Melfmn.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Melfmm.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Melfff.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Melffn.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Melffm.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Mdwarfmf.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Mdwarfmn.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Mdwarfmm.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Mdwarfff.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Mdwarffn.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Mdwarffm.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Mgnomemn.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Mgnomemm.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Mgnomeff.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Mgnomefn.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Mhobmf.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Mhobmn.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Mhobff.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Mhobfn.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Mfairymf.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Mfairymm.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Mfairyff.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Mfairyfn.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Mlizmf.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Mlizmn.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Mlizff.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Mlizfn.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Mdracmf.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Mdracmn.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Mdracff.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Mdracfn.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Mfelpmf.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Mfelpmn.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Mfelpff.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Mfelpfn.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Mrawmn.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Mrawmm.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Mrawff.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Mrawfn.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Mmookmf.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Mmookmn.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Mmookff.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Mmookfn.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Mninmf.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Mninff.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\mmook.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\mtrynm1.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\mtrynm2.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\mtrynm3.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Mtrang.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Mumpani.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\asexus.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\aurq.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\aRFS-81.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\amadras.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\asparkle.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\amyles.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\aVi.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\adrazic.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\atantris.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\arodan.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\aglumph.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\asaxx.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\mhumm4.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\mhumf4.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Melfm1.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\Melff1.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Small\\Shummf.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Shummn.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Shummm.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\shumm1.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\shumm2.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\shumm3.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Shumff.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Shumfn.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Shumfm.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\shumf1.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\shumf2.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\shumf3.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Selfmf.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Selfmn.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Selfmm.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Selfff.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Selffn.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Selffm.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Sdwarfmf.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Sdwarfmn.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Sdwarfmm.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Sdwarfff.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Sdwarffn.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Sdwarffm.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Sgnomemn.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Sgnomemm.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Sgnomeff.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Sgnomefn.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Shobmf.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Shobmn.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Shobff.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Shobfn.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Sfairymf.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Sfairymm.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Sfairyff.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Sfairyfn.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Slizmf.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Slizmn.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Slizff.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Slizfn.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Sdracmf.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Sdracmn.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Sdracff.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Sdracfn.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Sfelpmf.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Sfelpmn.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Sfelpff.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Sfelpfn.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Srawmn.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Srawmm.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Srawff.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Srawfn.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Smookmf.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Smookmn.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Smookff.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Smookfn.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Sninmf.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Sninff.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\smook.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\strynm1.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\strynm2.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\strynm3.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\strang.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\sumpani.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\ssexus.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Small\\surq.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Small\\sRFS-81.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Small\\smadras.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Small\\ssparkle.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Small\\smyles.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Small\\sVi.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Small\\sdrazic.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Small\\stantris.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Small\\srodan.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Small\\sglumph.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Small\\ssaxx.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Small\\shumm4.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\shumf4.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Selfm1.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\Selff1.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\SSkull.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\SSkull_drac.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\SSkull_felp.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\SSkull_liz.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\SSkull_mook.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\SSkull_raw.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\SSkull_trang.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\SSkull_tryn.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\SSkull_ump.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\SSkull_rap.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Small\\SSkull_and.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Portraits\\Medium\\MSkull.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\MSkull_drac.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\MSkull_felp.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\MSkull_liz.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\MSkull_mook.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\MSkull_raw.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\MSkull_trang.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\MSkull_tryn.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\MSkull_ump.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\MSkull_rap.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Portraits\\Medium\\MSkull_and.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\option2G.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\option2H.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\option2N.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\party2G.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\party2H.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\party2N.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\search2G.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\search2H.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\search2N.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\camp2G.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\camp2H.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\camp2N.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\bottom.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\bottomB.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\roof.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\layout.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\leftcorner.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\rightcorner.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\leftplug.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\rightplug.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\pback.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\CharHighlight.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\CharSelect.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\dragons.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\Inside.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\Outside.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\Outside2.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\lefttop.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\leftside1.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\leftside2.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\leftside3.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\leftside4.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\righttop.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\rightside1.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\rightside2.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\rightside3.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\rightside4.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\HealthBar.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\StaminaBar.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\MagicBar.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\HealthBarWide.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\StaminaBarWide.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\MagicBarWide.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\GoldBar.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\TextUpArrowOff.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\TextUpArrowOn.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\TextUpArrowHigh.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\TextDownArrowOff.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\TextDownArrowOn.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\TextDownArrowHigh.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\SliderBar.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\SliderNut.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\EdgeHighlight.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\EdgeSelect.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\CornerSelect.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\condition_highlight.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\NameSelect.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\WeaponSelect.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\StatsSelect.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\StatsSelectWide.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\HandLeft.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\HandRight.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\FHandLeft.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\FHandRight.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\PawLeft.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\PawRight.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\ClawLeft.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\ClawRight.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\RHandLeft.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\RHandRight.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\popup_edge.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\popup_divider.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\PopupHighlight.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\Weapon2Select.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\LeftWingTip.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\RightWingTip.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\LeftStrip.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\RightStrip.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\BottomStrip.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\FormationJewels.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\FormationJewelsH.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Flics\\FormationSymbol.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\Compass.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\ViewCone.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\main_bottom_bar.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\main_submenu_bar.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\main_column_info_normal.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\main_columns_hit.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\main_columns_normal.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\small_formation_back.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\main_radar_back.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\main_roof.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\main_scroll.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\main_textbox_back.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\main_textbox_tabs.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\icons_submenu.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\icons_portrait.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\icons_portrait_shield.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\icons_portrait_jewels.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\progressbar.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\confirmation_buttons.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\undo_button.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\damage_splat_anim.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\damage_splat_anim2.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\death_splat_anim.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\partymovement_back.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\partymovement_buttons.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\partymovement_man.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\partymovement_bars.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\item_bottompanel.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\item_inventorypool.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Party Formation\\small_formation_cage.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Party Formation\\small_form_cage_high.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Party Formation\\small_formation_cone.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Party Formation\\small_formation_symbol.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Party Formation\\small_formation_jewels.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Party Formation\\large_formation_panel.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Party Formation\\large_formation_symbol.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Party Formation\\large_formation_jewels.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Radar Compass\\compass_rim.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Radar Compass\\compass_rim_high.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Radar Compass\\formation_dot_large.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Radar Compass\\formation_dot_small.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Radar Compass\\formation_jewels.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\CombatPortraitPanel.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\leveling_button.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\cond_icon_rim_wide.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\paused_background.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Spells\\Bitmaps\\default_2D_anim.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Spells\\Bitmaps\\fire_positive.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Spells\\Bitmaps\\fire_negative.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Spells\\Bitmaps\\water_positive.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Spells\\Bitmaps\\water_negative.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Spells\\Bitmaps\\air_positive.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Spells\\Bitmaps\\air_negative.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Spells\\Bitmaps\\earth_positive.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Spells\\Bitmaps\\earth_negative.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Spells\\Bitmaps\\mental_positive.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Spells\\Bitmaps\\mental_negative.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Spells\\Bitmaps\\divine_positive.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Spells\\Bitmaps\\divine_negative.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Icons\\Conditions\\Drained.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Icons\\Conditions\\Diseased.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Icons\\Conditions\\Irritated.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Icons\\Conditions\\Nauseated.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Icons\\Conditions\\Slowed.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Icons\\Conditions\\Afraid.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Icons\\Conditions\\Poisoned.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Icons\\Conditions\\Silenced.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Icons\\Conditions\\Hexed.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Icons\\Conditions\\Infatuated.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Icons\\Conditions\\Insane.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Icons\\Conditions\\Blind.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Icons\\Conditions\\Turncoat.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Icons\\Conditions\\Webbed.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Icons\\Conditions\\Asleep.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Icons\\Conditions\\Paralyzed.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Icons\\Conditions\\Unconscious.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Icons\\Conditions\\Dead.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Icons\\Conditions\\Missing.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Icons\\Enchantments\\Dracon_Breath.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Icons\\Enchantments\\Guardian_Angel.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Icons\\Enchantments\\Razor_Cloak.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Icons\\Enchantments\\Eye4Eye.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Icons\\Enchantments\\Haste.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Icons\\Enchantments\\Super_Man.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Icons\\Enchantments\\Body_Stone.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Icons\\TravelingSpells\\armor_plate.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Icons\\TravelingSpells\\chameleon.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Icons\\TravelingSpells\\detect_secret.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Icons\\TravelingSpells\\enchanted_blade.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Icons\\TravelingSpells\\light.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Icons\\TravelingSpells\\magic_screen.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Icons\\TravelingSpells\\missile_shield.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Icons\\TravelingSpells\\shadow_hound.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Icons\\TravelingSpells\\x_ray.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Icons\\PartySpells\\Armor_Melt.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Icons\\PartySpells\\Acid_Cloud.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Icons\\PartySpells\\Toxic_Cloud.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Icons\\PartySpells\\Fire_Storm.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Icons\\PartySpells\\Death_Cloud.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Icons\\PartySpells\\Draining_Cloud.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Icons\\PartySpells\\Bless.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Icons\\PartySpells\\Element_Shield.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Icons\\PartySpells\\Soul_Shield.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Icons\\PartySpells\\Ring_Of_Fire.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Options\\intro_bg.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Options\\intro_bg_menu.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Options\\intro_menu.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Options\\opt_menu.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Options\\intro_bg_text.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Options\\intro_bg_credits.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Options\\introtext_normal.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Options\\introtext_depressed.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Options\\introtext_highlight.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Options\\introtext_unavailable.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Options\\options_base.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Options\\options_buttons.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Options\\options_computer.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Options\\options_controls.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Options\\options_submenu.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Options\\options_pagingbase.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Options\\options_navigation_arrows.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Options\\options_slider.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Options\\save_load_defaultscreen.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Options\\save_load_back.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Options\\save_load_buttons.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Options\\save_load_gamelists.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Party Generation\\back.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Party Generation\\listboxes.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Party Generation\\detailbox.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Party Generation\\bottombuttons.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Party Generation\\listbuttons.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Party Generation\\scrollbuttons.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Party Generation\\portraitborder.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Party Generation\\portraithighlight.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Party Generation\\options_back.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Party Generation\\import_addon.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Char Generation\\CG_Profession.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Char Generation\\CG_Personality.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Char Generation\\CG_BottomButtons.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Char Generation\\CG_Pieces.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Char Generation\\CG_Skills.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Char Generation\\CG_Magic.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Char Generation\\CG_Buttons.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Char Generation\\CG_Icons_Profession.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Char Generation\\CG_Icons_Race.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Char Generation\\CG_Icons_Gender.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Char Generation\\CG_Icons_Base.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\CommonCorner.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\BottomButtonBar.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\ReviewPageButtons.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\ReviewItemButtons.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\dismiss_button.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\ReviewItemPage.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\ReviewItemHighlights.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\PortraitExit.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\SmallPortraitHighlight.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\ReviewGoldIcon.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\BoxesOnly.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\ItemUsableBackground.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\ItemUnidentifiedOverlay.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\AC_Ovals.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\Leveling_bar.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\InventorySwapButtons.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\InventoryFilterButtons.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\commod_button_bar.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\commod_buttons.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\commod_back.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\reviewscreen_popups.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\InvCursedHighlight.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\statue_human_m.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\statue_human_f.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\statue_elf_m.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\statue_elf_f.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\statue_dwarf_m.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\statue_dwarf_f.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\statue_gnome_m.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\statue_gnome_f.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\statue_hobbit_m.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\statue_hobbit_f.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\statue_fairy_m.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\statue_fairy_f.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\statue_lizard_m.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\statue_lizard_f.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\statue_dracon_m.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\statue_dracon_f.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\statue_felpurr_m.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\statue_felpurr_f.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\statue_rawulf_m.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\statue_rawulf_f.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\statue_mook_m.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\statue_mook_f.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\statue_Trynnie.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\statue_Trang.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\statue_Umpani.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\statue_Rapax.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\statue_Android.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\ReviewMagicPage.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\ReviewSkillsPage.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\ReviewStatsPage.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\ReviewSliderbar.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\ReviewSkillsIcons.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\ReviewFXFilters.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\SecondaryOccupied.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\ButtonBackgroundOff.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\ButtonBackgroundOn.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Review\\ButtonHighlight.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Fonts\\PaletteRed.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Fonts\\PaletteGreen.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Fonts\\PalettePurple.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Fonts\\PaletteBlue.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Fonts\\PaletteOrange.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Fonts\\PaletteYellow.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Fonts\\PalettePink.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Fonts\\PaletteBrown.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Fonts\\PaletteWhite.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Fonts\\PaletteRust.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Fonts\\PaletteBronze.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Fonts\\PaletteGray.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Fonts\\PaletteBeige.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Fonts\\PaletteOptGreen.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Fonts\\PaletteOptWhite.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Flics\\Realms\\Fire.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Flics\\Realms\\Water.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Flics\\Realms\\Air.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Flics\\Realms\\Earth.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Flics\\Realms\\Mental.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Flics\\Realms\\Divine.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Automap\\map_border.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Automap\\map_cursors.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Automap\\map_exitbutton.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Automap\\map_layerbuttons.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Automap\\map_markerbuttons.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Automap\\map_movementbuttons.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Automap\\map_notebuttons.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Automap\\map_zoombuttons.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Spell Casting\\MagicBottomNew.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Spell Casting\\RealmHighlightNew.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Spell Casting\\PowerButtons.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Spell Casting\\fire_realm.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Spell Casting\\water_realm.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Spell Casting\\air_realm.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Spell Casting\\earth_realm.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Spell Casting\\mental_realm.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Spell Casting\\divine_realm.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Spell Casting\\MagicBottom.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Spell Casting\\ReviewSpellCast.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Spell Casting\\SmallPowerBalls.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Spell Casting\\SmallPowerRings.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Spell Casting\\LargePowerBalls.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Spell Casting\\LargePowerRings.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Spell Casting\\CastCancelButtons.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Spell Casting\\RealmHighlight.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\NPC Interaction\\Npc_bottompanel.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\NPC Interaction\\Npc_buttons.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\NPC Interaction\\Npc_itemfilters.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\NPC Interaction\\Npc_gold.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Locks And Traps\\LockPickMockup.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Locks And Traps\\lockpick_tumblers.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Locks And Traps\\lockpick_bottompanel.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Locks And Traps\\lockpick_bashbutton.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Locks And Traps\\traps_bottompanel.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Locks And Traps\\traps_deviceicons.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Locks And Traps\\traps_devicehighlight.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Locks And Traps\\traps_buttonglow.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Locks And Traps\\inspecting_bar.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Main Interface\\basic_fill_texture.pcx", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Journal\\journal_base.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Journal\\journal_parchment.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Journal\\journal_whitepal.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Journal\\journal_redpal.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Journal\\Faction_ratings_button.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Level Load\\Arnika.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Level Load\\Ascension Peak.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Level Load\\Bayjin.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Level Load\\Bayjin Shallows.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Level Load\\Cosmic Circle.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Level Load\\Marten's Bluff.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Level Load\\Marten's Underground.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Level Load\\Mine Tunnels.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Level Load\\LowerMonastery.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Level Load\\UpperMonastery.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Level Load\\MtGigas Water Caves.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Level Load\\MtGigas Caves.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Level Load\\MtGigas Upper Caves.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Level Load\\Umpani Base Camp.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Level Load\\MtGigas Peak.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Level Load\\Wilderness Clearing.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Level Load\\Rapax Castle Cellar.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Level Load\\Rapax Castle Main Level.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Level Load\\Upper Rapax Castle.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Level Load\\Rapax Courtyard.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Level Load\\Rapax Rift.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Level Load\\Sea Caves.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Level Load\\Swamp.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Level Load\\Trynton.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Level Load\\Trynton Upper Branches.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Level Load\\Arnika-Trynton Road.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Level Load\\South East Wilderness.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Level Load\\Mountain Wilderness.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Level Load\\Northern Wilderness.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Level Load\\Arnika Road.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Level Load\\SavantTower.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Level Load\\Rattkin Tree.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Level Load\\Wilderness Clearing2.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Level Load\\levelload_gears.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Level Load\\levelload_textbar.sti", W8_VIDEO_STORAGE_OBJECT},
    {"Data\\Level Load\\DeathScreen.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Level Load\\Camping Screen.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Level Load\\DarkLord.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Level Load\\Cosmic ending.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Level Load\\BlowOut.sti", W8_VIDEO_STORAGE_SURFACE},
    {"Data\\Options\\Falcon.sti", W8_VIDEO_STORAGE_OBJECT},
};
#pragma clang diagnostic pop

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
// GLOBAL: WIZ8 0x006448c8
W8VideoObjectSlot g_video_slots[494] = {
    {0, 0},    {0, 1},    {0, 2},    {0, 7},    {0, 12},   {0, 13},   {0, 14},   {0, 18},
    {0, 22},   {0, 23},   {0, 24},   {0, 29},   {0, 30},   {0, 31},   {0, 35},   {0, 36},
    {0, 37},   {1, 0},    {81, 0},   {161, 0},  {241, 0},  {242, 0},  {243, 0},  {244, 0},
    {245, 0},  {246, 0},  {247, 0},  {248, 0},  {249, 0},  {250, 0},  {251, 0},  {252, 0},
    {253, 0},  {254, 0},  {255, 0},  {256, 0},  {257, 0},  {258, 0},  {259, 0},  {260, 0},
    {261, 0},  {262, 0},  {277, 0},  {275, 0},  {276, 0},  {279, 0},  {280, 0},  {281, 0},
    {282, 0},  {283, 0},  {284, 0},  {285, 0},  {286, 0},  {265, 0},  {264, 0},  {263, 0},
    {268, 0},  {267, 0},  {266, 0},  {271, 0},  {270, 0},  {269, 0},  {274, 0},  {273, 0},
    {272, 0},  {278, 0},  {287, 0},  {288, 0},  {289, 0},  {290, 0},  {291, 0},  {292, 0},
    {293, 0},  {294, 0},  {295, 0},  {296, 0},  {297, 0},  {298, 0},  {299, 0},  {300, 0},
    {301, 0},  {302, 0},  {303, 0},  {304, 0},  {305, 0},  {306, 0},  {307, 0},  {308, 0},
    {309, 0},  {310, 0},  {311, 0},  {312, 0},  {313, 0},  {314, 0},  {315, 0},  {316, 0},
    {317, 0},  {318, 0},  {319, 0},  {320, 0},  {321, 0},  {322, 0},  {323, 0},  {324, 0},
    {325, 0},  {326, 0},  {327, 0},  {328, 0},  {329, 0},  {330, 0},  {331, 0},  {332, 0},
    {333, 0},  {334, 0},  {335, 0},  {336, 0},  {337, 0},  {338, 0},  {339, 0},  {340, 0},
    {341, 0},  {345, 0},  {346, 0},  {344, 0},  {342, 0},  {343, 0},  {347, 0},  {348, 0},
    {349, 0},  {350, 0},  {351, 0},  {352, 0},  {353, 0},  {354, 0},  {355, 0},  {356, 0},
    {357, 0},  {358, 0},  {359, 0},  {360, 0},  {361, 0},  {362, 0},  {363, 0},  {364, 0},
    {365, 0},  {366, 0},  {367, 0},  {368, 0},  {369, 0},  {370, 0},  {371, 0},  {372, 0},
    {373, 0},  {374, 0},  {375, 0},  {376, 0},  {377, 0},  {378, 0},  {379, 0},  {380, 0},
    {381, 0},  {382, 0},  {383, 0},  {384, 0},  {385, 0},  {386, 0},  {387, 0},  {388, 0},
    {389, 0},  {390, 0},  {391, 0},  {392, 0},  {393, 0},  {394, 0},  {395, 0},  {396, 0},
    {397, 0},  {398, 0},  {399, 0},  {400, 0},  {401, 0},  {402, 0},  {403, 0},  {404, 0},
    {405, 0},  {406, 0},  {407, 0},  {408, 0},  {409, 0},  {410, 0},  {411, 0},  {412, 0},
    {413, 0},  {414, 0},  {415, 0},  {416, 0},  {417, 0},  {418, 0},  {419, 0},  {420, 0},
    {421, 0},  {422, 0},  {423, 0},  {424, 0},  {425, 0},  {426, 0},  {427, 0},  {428, 0},
    {429, 0},  {430, 0},  {431, 0},  {432, 0},  {433, 0},  {434, 0},  {435, 0},  {436, 0},
    {437, 0},  {438, 0},  {439, 0},  {440, 0},  {441, 0},  {442, 0},  {443, 0},  {444, 0},
    {445, 0},  {446, 0},  {447, 0},  {448, 0},  {449, 0},  {450, 0},  {451, 0},  {452, 0},
    {453, 0},  {454, 0},  {455, 0},  {456, 0},  {457, 0},  {458, 0},  {459, 0},  {460, 0},
    {461, 0},  {462, 0},  {463, 0},  {464, 0},  {465, 0},  {466, 0},  {467, 0},  {468, 0},
    {469, 0},  {470, 0},  {471, 0},  {472, 0},  {473, 0},  {474, 0},  {475, 0},  {476, 0},
    {477, 0},  {478, 0},  {479, 0},  {480, 0},  {481, 0},  {482, 0},  {483, 0},  {484, 0},
    {485, 0},  {486, 0},  {487, 0},  {488, 0},  {489, 0},  {490, 0},  {491, 0},  {492, 0},
    {493, 0},  {494, 0},  {495, 0},  {496, 0},  {497, 0},  {498, 0},  {499, 0},  {500, 0},
    {501, 0},  {502, 0},  {503, 0},  {504, 0},  {505, 0},  {506, 0},  {507, 0},  {508, 0},
    {509, 0},  {510, 0},  {511, 0},  {512, 0},  {513, 0},  {514, 0},  {515, 0},  {516, 0},
    {517, 0},  {518, 0},  {519, 0},  {520, 0},  {521, 0},  {522, 0},  {523, 0},  {524, 0},
    {525, 0},  {526, 0},  {527, 0},  {528, 0},  {529, 0},  {530, 0},  {531, 0},  {532, 0},
    {533, 0},  {534, 0},  {535, 0},  {536, 0},  {537, 0},  {538, 0},  {539, 0},  {540, 0},
    {541, 0},  {542, 0},  {543, 0},  {544, 0},  {545, 0},  {546, 0},  {547, 0},  {548, 0},
    {549, 0},  {550, 0},  {572, 0},  {573, 0},  {573, 1},  {573, 2},  {573, 3},  {573, 4},
    {574, 0},  {574, 1},  {574, 2},  {574, 3},  {575, 0},  {575, 1},  {575, 2},  {575, 3},
    {575, 4},  {575, 5},  {575, 6},  {575, 7},  {576, 0},  {576, 1},  {576, 2},  {576, 3},
    {576, 4},  {576, 5},  {576, 6},  {576, 7},  {576, 8},  {576, 9},  {576, 10}, {576, 11},
    {577, 0},  {577, 1},  {577, 2},  {577, 3},  {577, 4},  {577, 5},  {577, 6},  {577, 7},
    {577, 8},  {577, 9},  {577, 10}, {577, 11}, {577, 12}, {577, 13}, {577, 14}, {577, 15},
    {577, 16}, {577, 17}, {577, 18}, {577, 19}, {578, 0},  {578, 1},  {578, 2},  {578, 3},
    {578, 4},  {578, 5},  {578, 6},  {578, 7},  {579, 0},  {579, 1},  {579, 2},  {579, 3},
    {579, 4},  {579, 5},  {579, 6},  {579, 7},  {579, 8},  {579, 9},  {579, 10}, {579, 11},
    {580, 0},  {581, 0},  {582, 0},  {583, 0},  {584, 0},  {585, 0},  {586, 0},  {587, 0},
    {588, 0},  {589, 0},  {590, 0},  {591, 0},  {591, 8},  {592, 0},  {593, 0},  {593, 8},
    {594, 0},  {595, 0},  {595, 1},  {595, 2},  {595, 3},  {595, 4},  {595, 5},  {596, 0},
    {596, 1},  {597, 0},  {598, 0},  {599, 0},  {600, 0},  {601, 0},  {602, 0},  {603, 0},
    {604, 0},  {605, 0},  {606, 0},  {607, 0},  {608, 0},  {609, 0},  {610, 0},  {611, 0},
    {612, 0},  {613, 0},  {614, 0},  {615, 0},  {616, 0},  {617, 0},  {618, 0},  {619, 0},
    {620, 0},  {621, 0},  {622, 0},  {623, 0},  {624, 0},  {625, 0},  {626, 0},  {627, 0},
    {628, 0},  {629, 0},  {630, 0},  {631, 0},  {632, 0},  {633, 0},  {634, 0},  {635, 0},
    {636, 0},  {637, 0},  {638, 0},  {639, 0},  {640, 0},  {641, 0},  {642, 0},  {643, 0},
    {644, 0},  {645, 0},  {646, 0},  {647, 0},  {648, 0},  {649, 0},  {650, 0},  {651, 0},
    {652, 0},  {653, 0},  {654, 0},  {655, 0},  {656, 0},  {551, 0},  {566, 0},  {567, 0},
    {568, 0},  {569, 0},  {570, 0},  {571, 0},
};
#pragma clang diagnostic pop

/* The two loaders consume the released SGP object and surface request records.
   Their 0x6c and 0x70 sizes account exactly for this function's 0xe0-byte pair
   of stack objects. */

#define VIDEO_OBJECT_MANAGER_CPP "C:\\Projects\\Wizardry 8\\Local Code\\VideoObjectManager.cpp"

// FUNCTION: WIZ8 0x00549250
void ReleaseLoadedVideoFrames(void)
{
    char released;

    if (!gfVideoObjectsInit) {
        srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP, 0x9c, 0);
    }
    for (W8VideoFrame* frame = g_video_frames; frame < g_video_frames + 1658; ++frame) {
        if (frame->loaded) {
            if (frame->storage_kind == W8_VIDEO_STORAGE_OBJECT) {
                released = DeleteVideoObjectFromIndex(frame->handle);
            } else {
                released = DeleteVideoSurfaceFromIndex(frame->handle);
            }
            if (!released) {
                srAssertFail("fReturnCode", VIDEO_OBJECT_MANAGER_CPP, 0x85, 0);
            }
            frame->handle = 0;
            frame->loaded = false;
        }
    }
}

// FUNCTION: WIZ8 0x00548f90
void DrawCatalogImage(UINT32 target, int object, int frame, short image, int left, int top,
                      UINT32 mode, blt_fx* effects)
{
    W8VideoObjectSlot* slot;
    short row;
    unsigned int surface;
    BOOLEAN ok;

    if (!gfVideoObjectsInit) {
        srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP, 0x2d, 0);
    }
    EnsureCatalogFrameLoaded(object, frame);
    slot = &g_video_slots[object];
    row = slot->y_offset + image;
    surface = g_video_frames[slot->first_frame + frame].handle;
    if (!gfVideoObjectsInit) {
        srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP, 0xdd, 0);
    }
    if (g_video_frames[slot->first_frame + frame].storage_kind == W8_VIDEO_STORAGE_OBJECT) {
        ok = BltVideoObjectFromIndex(target, surface, row, left, top, mode, effects);
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
    BOOLEAN loaded_ok;

    /* Two nested checks, both in the original: the assertion does not return,
       so the inner one is reachable only when it is compiled out. */
    if (!gfVideoObjectsInit) {
        srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP, 0x4c, 0);
        if (!gfVideoObjectsInit) {
            srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP, 0xd4, 0);
        }
    }
    record = &g_video_frames[g_video_slots[object].first_frame + frame];
    if (!record->loaded) {
        if (!gfVideoObjectsInit) {
            srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP, 0xdd, 0);
        }
        if (record->storage_kind == W8_VIDEO_STORAGE_OBJECT) {
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
                                      record->path, record->storage_kind));
        }
        record->handle = handle;
        record->loaded = true;
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
        srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP, 0xaf, 0);
    }
    palette = static_cast<unsigned short*>(malloc(0x200));
    if (!palette) {
        return 0;
    }
    EnsureCatalogFrameLoaded(object, frame);
    if (!CopyVideoObjectPalette16BPP(GetCatalogVideoObjectHandle(object, frame), palette)) {
        return 0;
    }
    return palette;
}

// FUNCTION: WIZ8 0x00549390
unsigned int GetCatalogVideoObjectHandle(int object, int frame)
{
    if (!gfVideoObjectsInit) {
        srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP, 0xe6, 0);
    }
    EnsureCatalogFrameLoaded(object, frame);
    return g_video_frames[g_video_slots[object].first_frame + frame].handle;
}

// FUNCTION: WIZ8 0x005493e0
short GetCatalogVideoObjectYOffset(int object)
{
    if (!gfVideoObjectsInit) {
        srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP, 0xf2, 0);
    }
    return g_video_slots[object].y_offset;
}

// FUNCTION: WIZ8 0x00549420
HVOBJECT GetCatalogVideoObject(int object, int frame, int* y_offset_out)
{
    W8VideoFrame* record;
    HVOBJECT video_object;

    if (!gfVideoObjectsInit) {
        srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP, 0xfd, 0);
    }
    EnsureCatalogFrameLoaded(object, frame);
    if (!gfVideoObjectsInit) {
        srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP, 0xdd, 0);
    }
    record = &g_video_frames[g_video_slots[object].first_frame + frame];
    if (record->storage_kind == W8_VIDEO_STORAGE_OBJECT) {
        if (y_offset_out != 0) {
            if (!gfVideoObjectsInit) {
                srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP, 0xf2, 0);
            }
            *y_offset_out = g_video_slots[object].y_offset;
        }
        GetVideoObject(&video_object, record->handle);
        return video_object;
    }
    return 0;
}

/* Mark the complete rectangle covered by one catalog image. ETRLE-backed
   objects supply subimage dimensions through the canonical SGP object API;
   surface-backed objects expose the dimensions on the canonical SGP surface
   record returned by GetVideoSurface. */
// FUNCTION: WIZ8 0x005494f0
void InvalidateCatalogImageRect(int object, int frame, int image, int left, int top, int flags)
{
    W8VideoObjectSlot* slot;
    W8VideoFrame* record;
    short subimage;
    unsigned short width = 0;
    unsigned short height = 0;

    EnsureCatalogFrameLoaded(object, frame);
    slot = &g_video_slots[object];
    subimage = slot->y_offset + image;
    record = &g_video_frames[slot->first_frame + frame];
    if (!gfVideoObjectsInit) {
        srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP, 0xdd, 0);
    }
    if (record->storage_kind == W8_VIDEO_STORAGE_OBJECT) {
        GetVideoObjectETRLESubregionProperties(record->handle, subimage, &width, &height);
    } else {
        HVSURFACE surface;
        if (GetVideoSurface(&surface, record->handle)) {
            height = surface->usHeight;
            width = surface->usWidth;
        }
    }

    if (width != 0 && height != 0) {
        InvalidateRegion(left, top, left + width, top + height, flags);
    }
}

/* Draw a catalog video object, then mark the area it covered. The subimage
   index is truncated to a short for the draw and passed whole to the mark,
   and the seventh reaches the mark only as whether it equals two. */
// FUNCTION: WIZ8 0x00549600
void DrawCatalogImageAndInvalidate(UINT32 target, int object, int frame, int image, int left,
                                   int top, UINT32 mode, blt_fx* effects)
{
    DrawCatalogImage(target, object, frame, static_cast<short>(image), left, top, mode, effects);
    InvalidateCatalogImageRect(object, frame, image, left, top, mode == VO_BLT_SRCTRANSPARENCY);
}

/* Loads the selected catalog frame and returns the dimensions of one of its
   ETRLE subimages. Surface-backed records have no ETRLE table, so the retail
   body intentionally leaves the caller's outputs untouched for them. */
// FUNCTION: WIZ8 0x00549660
void GetCatalogImageSize(int object, int frame, int image, short* width, short* height)
{
    W8VideoObjectSlot* slot;
    W8VideoFrame* record;
    short subimage;

    EnsureCatalogFrameLoaded(object, frame);
    slot = &g_video_slots[object];
    subimage = slot->y_offset + image;
    record = &g_video_frames[slot->first_frame + frame];
    if (!gfVideoObjectsInit) {
        srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP, 0xdd, 0);
    }
    if (record->storage_kind == W8_VIDEO_STORAGE_OBJECT) {
        GetVideoObjectETRLESubregionProperties(record->handle, subimage, (unsigned short*)width,
                                               (unsigned short*)height);
    }
}

/* The image's own offset inside its frame, read from the video object's ETRLE
   table; only uncompressed frames carry one. */
// FUNCTION: WIZ8 0x00549700
void GetCatalogImagePosition(int object, int frame, int image, short* x, short* y)
{
    W8VideoObjectSlot* slot;
    W8VideoFrame* record;
    HVOBJECT video_object;
    short subimage;

    EnsureCatalogFrameLoaded(object, frame);
    slot = &g_video_slots[object];
    subimage = slot->y_offset + image;
    record = &g_video_frames[slot->first_frame + frame];
    if (!gfVideoObjectsInit) {
        srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP, 0xdd, 0);
    }
    if (record->storage_kind == W8_VIDEO_STORAGE_OBJECT) {
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
unsigned char BlitCatalogSurfaceRectTo16BPP(UINT32 target, int left, int top, int right, int bottom,
                                            int object, int source_x, int source_y)
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
        srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP, 0x19d, 0);
    }
    EnsureCatalogFrameLoaded(object, 0);
    source_handle = g_video_frames[g_video_slots[object].first_frame].handle;
    GetVideoSurface(&source_surface, source_handle);
    target_pixels = LockVideoSurface(target, &target_pitch);
    source_pixels = LockVideoSurface(source_handle, &source_pitch);
    Blt8BPPDataSubTo16BPPBuffer(reinterpret_cast<unsigned short*>(target_pixels), target_pitch,
                                source_surface, source_pixels, source_pitch, left, top,
                                &source_rect);
    UnLockVideoSurface(target);
    UnLockVideoSurface(source_handle);
    return 1;
}

/* Lock the pixel buffer of a surface-backed catalog frame. ETRLE video-object
   entries have no lockable surface and return null. */
// FUNCTION: WIZ8 0x005498a0
void* LockCatalogFrameSurface(unsigned int object, unsigned int frame, long* pitch)
{
    unsigned int surface;

    if (!gfVideoObjectsInit) {
        srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP, 0x1b9, 0);
    }
    EnsureCatalogFrameLoaded(object, frame);
    surface = g_video_frames[g_video_slots[object].first_frame + frame].handle;
    if (!gfVideoObjectsInit) {
        srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP, 0xdd, 0);
    }
    if (g_video_frames[g_video_slots[object].first_frame + frame].storage_kind ==
        W8_VIDEO_STORAGE_OBJECT) {
        return 0;
    }
    // reinterpret-ok: retail forwards the same 32-bit pitch word to the SGP lock API
    return LockVideoSurface(surface, reinterpret_cast<UINT32*>(pitch));
}

/* Release a surface-backed catalog frame locked by LockCatalogFrameSurface. Video
   objects need no corresponding SGP surface unlock. */
// FUNCTION: WIZ8 0x00549950
void UnlockCatalogFrameSurface(unsigned int object, unsigned int frame)
{
    unsigned int surface;

    EnsureCatalogFrameLoaded(object, frame);
    surface = g_video_frames[g_video_slots[object].first_frame + frame].handle;
    if (!gfVideoObjectsInit) {
        srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP, 0xdd, 0);
    }
    if (g_video_frames[g_video_slots[object].first_frame + frame].storage_kind !=
        W8_VIDEO_STORAGE_OBJECT) {
        UnLockVideoSurface(surface);
    }
}
