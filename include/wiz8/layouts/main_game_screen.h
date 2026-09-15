#pragma once

#include "wiz8/geometry.h"

class srClass;
class stModelInstance2D;

#include <cstddef>

/* One animated cursor resource the main-game / camp screens can install. The
   object is an stTextureAnim; size and hotspot are the unsigned shorts
   ApplyCurrentCursor feeds to the mouse-cursor surface helpers. */
struct W8MainGameResourceSlot {
    srClass* object;
    unsigned int frame_count;
    unsigned short size_x;
    unsigned short size_y;
    unsigned short hotspot_x;
    unsigned short hotspot_y;
    int image_id;
};
static_assert(sizeof(W8MainGameResourceSlot) == 0x14, "W8MainGameResourceSlot_size");

#pragma pack(push, 1)
/* The dormant typed-dialogue input state hung off the level block: a plain
   heap object, not an srClass derivative. */
struct W8DialogueTextState {
    wchar_t* text;                 /* 0x00: owned editable buffer */
    wchar_t* first_line_prefix;    /* 0x04: owned, measured before line one */
    void (*completion_callback)(); /* 0x08: invoked before the input closes */
    unsigned int* line_offsets;    /* 0x0c: owned offsets into text */
    unsigned int line_count;       /* 0x10 */
    unsigned int text_capacity;    /* 0x14: characters, grown by 0x400 */
    unsigned int line_capacity;    /* 0x18: entries, grown by 0x20 */
    int notice_channel;            /* 0x1c: passed to the completed notice */
    short text_box;                /* 0x20: text box whose scroll is replaced */
    short unknown_22;              /* 0x22: never consumed by the retail cluster */
    unsigned int wrap_width;       /* 0x24: pixel budget and notice width */
    unsigned int cursor;           /* 0x28: insertion point in text */
    unsigned char unknown_2c;      /* 0x2c: never consumed by the retail cluster */
    unsigned char dirty;           /* 0x2d: cursor/text redraw pending */
    unsigned char unknown_2e[2];
    unsigned int saved_scroll_line; /* 0x30: restored when input closes */
};
static_assert(sizeof(W8DialogueTextState) == 0x34, "W8DialogueTextState_size");
static_assert(offsetof(W8DialogueTextState, saved_scroll_line) == 0x30,
              "W8DialogueTextState_saved_scroll_line");

struct W8LevelRuntimeBlock {
    unsigned char unknown_000[0xf0];
    unsigned char flag_0f0; /* 0x0f0 */
    unsigned char unknown_0f1[3];
    unsigned int redraw_flags; /* 0x0f4 */
    unsigned char unknown_0f8[4];
    int value_0fc;                             /* 0x0fc */
    int camera_mode_100;                       /* 0x100 */
    unsigned int hover_region;                 /* 0x104 */
    unsigned char flag_108;                    /* 0x108 */
    unsigned char portrait_refresh_pending[8]; /* 0x109 */
    unsigned char unknown_111[3];
    int portrait_refresh_image[8]; /* 0x114 */
    int portrait_refresh_mode[8];  /* 0x134 */
    unsigned char pick_changed_154;
    unsigned char action_panel_visible;
    unsigned char formation_board_visible; /* 0x156: formation board shown */
    unsigned char radar_map_visible;
    unsigned char unknown_158;
    unsigned char text_scroll_drag_idle; /* 0x159: cleared while thumb is dragged */
    unsigned char unknown_15a[0x12];
    int highlight_override;             /* 0x16c */
    int party_slots_170[7];             /* 0x170: positional roles unresolved */
    int formation_highlight_party_slot; /* 0x18c */
    int held_item_display_190;          /* 0x190 */
    int value_194;
    int value_198;
    int value_19c;
    int text_content_region;
    int dialogue_content_region;
    unsigned int text_lines[12]; /* 0x1a8 */
    int text_slots_1d8[4];
    int text_slots_1e8[4];
    unsigned char dialogue_text_input_open;
    unsigned char unknown_1f9[3];
    /* GOG retail retains the complete editor consumer path, but has no writer
       that raises this gate and no allocation/store producer for the pointer.
       The input is therefore dormant in this build rather than an inferred
       NPC-dialogue feature. */
    W8DialogueTextState* dialogue_text_input;
    int portrait_overlay_party_slot;    /* 0x200: -1 while untracked */
    int party_slot_204;                 /* 0x204: positional role unresolved */
    int party_slot_208;                 /* 0x208: positional role unresolved */
    int condition_highlight_party_slot; /* 0x20c: -1 while untracked */
    unsigned char flag_210;             /* 0x210 */
    unsigned char unknown_211[3];
    unsigned int clock_214; /* 0x214 */
    unsigned char flag_218; /* 0x218 */
    unsigned char unknown_219[7];
    int dialogue_x_220;
    unsigned int dialogue_y_224; /* ClearSurfaceRect's unsigned top/bottom */
    unsigned int dialogue_height_228;
    unsigned char unknown_22c[0xc];
    int dialogue_width_238;
    int value_23c; /* 0x23c */
    /* The dialogue highlight sprite. The unrecovered dialogue-box draw at
       0x00563FC0 lazily creates it from catalog object 0x72 through
       CreateSpriteFromSurface - the retail assertion spells it
       gpMGSV->pHighlightGraphic - and it is released through
       ReleaseObject004257F0 whenever mode 6 ends or the tracked party slots
       change. */
    stModelInstance2D* highlight_graphic; /* 0x240 */
    unsigned int world_update_flags;      /* 0x244 */
    unsigned int world_render_flags;      /* 0x248 */
    unsigned char unknown_24c;
    unsigned char flag_24d;
    unsigned char unknown_24e[2];
    unsigned int character_update_timer; /* 0x250 */
    unsigned int world_update_timer;     /* 0x254 */
    unsigned int countdown_258;          /* 0x258 */
    unsigned int countdown_25c;          /* 0x25c */
    unsigned char transition_active;     /* 0x260 */
    unsigned char transition_pending;    /* 0x261 */
    unsigned char unknown_262[2];
    int highlighted_item;
    int selected_item;
    unsigned int countdown_26c; /* 0x26c */
    unsigned char flag_270;
    unsigned char flag_271;
    unsigned char flag_272;
    unsigned char unknown_273;
    unsigned int tick_274; /* 0x274 */
    int value_278;         /* 0x278 */
    int pending_level;
    int pending_entry_id;
    int value_284; /* 0x284 */
    int value_288; /* 0x288 */
    int value_28c; /* 0x28c */
    unsigned char unknown_290[0x10];
    /* 0x2a0..0x2a8: the formation board's three stModelInstance2D-family
       sprites - the board art with slot markers baked in, the rotating compass
       needle tracking party_facing against party_heading, and a lazily
       created overlay.  All are produced by CreateSpriteFromSurface in MGSFormation.cpp
       and released through ReleaseObject004257F0. */
    stModelInstance2D* formation_board_sprite;
    stModelInstance2D* formation_compass_sprite;
    stModelInstance2D* formation_overlay_sprite;
    int value_2ac; /* 0x2ac */
    int value_2b0; /* 0x2b0 */
    int value_2b4; /* 0x2b4 */
    unsigned char unknown_2b8[8];
    unsigned char refresh_combat_panel;
    unsigned char unknown_2c1[3];
    unsigned int combat_panel_timer;
    unsigned char refresh_party_panel;
    unsigned char unknown_2c9;
    short combat_end_notification;
    int text_box_left;
    int text_box_top;
    int text_box_right;
    int text_box_bottom;
    int move_budget_2dc;
    int move_budget_2e0;
    unsigned char unknown_2e4[4];
    int value_2e8;
    unsigned short* palette_2ec; /* 0x2ec */
    int selection_kind;
    int value_2f4; /* 0x2f4 */
    unsigned char selection_settled;
    unsigned char unknown_2f9[3];
    unsigned int tooltip_since;
    unsigned char tooltip_pending;
    unsigned char unknown_301[3];
    int tooltip_subject;
    int tooltip_kind;
    unsigned int countdown_30c; /* 0x30c */
    int combat_slot;            /* 0x310 */
    unsigned char flag_314;
    unsigned char unknown_315[3];
    int hover_combat_slot; /* 0x318 */
    unsigned char flag_31c;
    unsigned char unknown_31d[3];
    unsigned int countdown_320;
    unsigned char flag_324;
    unsigned char formation_board_alternate; /* 0x325: highlighted board art while hovered */
    unsigned char flag_326;
    unsigned char flag_327;
    unsigned char flag_328;
    unsigned char unknown_329[3];
    unsigned int countdown_32c;
};
#pragma pack(pop)

static_assert(sizeof(W8LevelRuntimeBlock) == 0x330, "W8LevelRuntimeBlock_must_be_0x330");
static_assert(offsetof(W8LevelRuntimeBlock, flag_0f0) == 0x0f0, "W8LevelRuntimeBlock_flag_0f0");
static_assert(offsetof(W8LevelRuntimeBlock, value_0fc) == 0x0fc, "W8LevelRuntimeBlock_value_0fc");
static_assert(offsetof(W8LevelRuntimeBlock, portrait_refresh_pending) == 0x109,
              "W8LevelRuntimeBlock_portrait_refresh_pending");
static_assert(offsetof(W8LevelRuntimeBlock, portrait_refresh_image) == 0x114,
              "W8LevelRuntimeBlock_portrait_refresh_image");
static_assert(offsetof(W8LevelRuntimeBlock, portrait_refresh_mode) == 0x134,
              "W8LevelRuntimeBlock_portrait_refresh_mode");
static_assert(offsetof(W8LevelRuntimeBlock, party_slots_170) == 0x170,
              "W8LevelRuntimeBlock_party_slots_170");
static_assert(offsetof(W8LevelRuntimeBlock, formation_highlight_party_slot) == 0x18c,
              "W8LevelRuntimeBlock_formation_highlight_party_slot");
static_assert(offsetof(W8LevelRuntimeBlock, value_194) == 0x194, "W8LevelRuntimeBlock_value_194");
static_assert(offsetof(W8LevelRuntimeBlock, value_198) == 0x198, "W8LevelRuntimeBlock_value_198");
static_assert(offsetof(W8LevelRuntimeBlock, portrait_overlay_party_slot) == 0x200,
              "W8LevelRuntimeBlock_portrait_overlay_party_slot");
static_assert(offsetof(W8LevelRuntimeBlock, party_slot_204) == 0x204,
              "W8LevelRuntimeBlock_party_slot_204");
static_assert(offsetof(W8LevelRuntimeBlock, party_slot_208) == 0x208,
              "W8LevelRuntimeBlock_party_slot_208");
static_assert(offsetof(W8LevelRuntimeBlock, condition_highlight_party_slot) == 0x20c,
              "W8LevelRuntimeBlock_condition_highlight_party_slot");
static_assert(offsetof(W8LevelRuntimeBlock, flag_210) == 0x210, "W8LevelRuntimeBlock_flag_210");
static_assert(offsetof(W8LevelRuntimeBlock, clock_214) == 0x214, "W8LevelRuntimeBlock_clock_214");
static_assert(offsetof(W8LevelRuntimeBlock, countdown_258) == 0x258,
              "W8LevelRuntimeBlock_countdown_258");
static_assert(offsetof(W8LevelRuntimeBlock, countdown_26c) == 0x26c,
              "W8LevelRuntimeBlock_countdown_26c");
static_assert(offsetof(W8LevelRuntimeBlock, palette_2ec) == 0x2ec,
              "W8LevelRuntimeBlock_palette_2ec");
static_assert(offsetof(W8LevelRuntimeBlock, countdown_32c) == 0x32c,
              "W8LevelRuntimeBlock_countdown_32c");
