#pragma once

/* W8NpcInteractionState::dialogue_layout - which NPC dialogue layout is up. The
   layout-1 caption is "MAGIC" (Charm/Mindread/Use Item services) and the
   layout-5 caption "TRADE"; both spellings come from the StringData.DAT
   captions each open routine loads. Layout 6 owns no panels: every close path
   only parks it back on NONE, and no open path is recovered. */
enum W8NpcDialogueLayout {
    W8_DIALOGUE_LAYOUT_UNSET = -1, /* spell view has no dialogue to reopen */
    W8_DIALOGUE_LAYOUT_NONE = 0,
    W8_DIALOGUE_LAYOUT_SERVICES = 1,
    W8_DIALOGUE_LAYOUT_TOPIC_MENU = 2,
    W8_DIALOGUE_LAYOUT_TRANSCRIPT = 3,
    W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX = 4,
    W8_DIALOGUE_LAYOUT_TRADE = 5,
    W8_DIALOGUE_LAYOUT_BARE = 6
};
