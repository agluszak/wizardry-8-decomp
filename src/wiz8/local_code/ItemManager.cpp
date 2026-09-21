#include "wiz8/local_code/ItemManager.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/3d.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/layouts/character.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/UtilityFunctions.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/Item.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/item_spawning.h"
#include "wiz8/item_tables.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_code/NPCManager.h"
#include "wiz8/local_code/GameplayDatabase.h"
#include "wiz8/layouts/screen_state.h"
#include "wiz8/local_code/Gameloop.h"
#include "wiz8/sr_api.h"
#include "wiz8/utility.h"
#include "random.h"
#include "wiz8/engine_code/GDProp.h"
#include "wiz8/float_constants.h"
#include "wiz8/engine_code/ReadLevel.h"
#include "wiz8/engine_code/stModelInstance.h"
#include "surrender/srCore.h"
#include "FileMan.h"

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/GameTimeAccumulator0043A910.h"
#include "wiz8/engine_code/PolyPick.h"
#include "wiz8/geometry.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/xstatus.h"
#include "wiz8/cursor.h"
#include "soundman.h"
#include "wiz8/dialog_code/DialogBase.h"
#include "wiz8/dialog_code/DialogFactoryDialogs.h"

/* 0x0068EDCC: the level runtime block, which also carries the interface
   selection the item manager resets. */
#define ITEM_MANAGER_CPP "C:\\Projects\\Wizardry 8\\Local Code\\ItemManager.cpp"

/* 0x005ED7B0: 1/360, the half-degree step random item angles are built
   from. */
// GLOBAL: WIZ8 0x005ed7b0
extern const double g_double_005ed7b0 = 1.0 / 360.0;

/* 0x005ED7B8: camera distance inside which inactive world items are activated. */
// GLOBAL: WIZ8 0x005ed7b8
float g_float_005ed7b8 = 20000.0f;

/* 0x005ED7A8: pi, the amplitude cursor-driven throw angles are scaled from. */
// GLOBAL: WIZ8 0x005ed7a8
extern const double g_double_005ed7a8 = 3.141592653589793;

/* 0x005EBF48: the screen-y coefficient in the drop-item yaw. */
// GLOBAL: WIZ8 0x005ebf48
extern const float g_float_005ebf48 = 85.0f;

/* 0x005EBF4C: the screen-z coefficient in the drop-item pitch. */
// GLOBAL: WIZ8 0x005ebf4c
extern const float g_float_005ebf4c = 71.0f;

/* 0x005ED7C0: the vertical scale of the drop-item direction. */
// GLOBAL: WIZ8 0x005ed7c0
extern const double g_double_005ed7c0 = -2500.0;

/* 0x0064A1CD: when set, skip activating world items that already carry flag
   bit 0. */
// GLOBAL: WIZ8 0x0064a1cd
unsigned char g_byte_0064a1cd = 1;

// FUNCTION: WIZ8 0x004f69f0
bool InitializeItemManagerState()
{
    g_status_685170.next_world_item_id_2352 = 1;
    gXStatus.item_manager_pending = 0;
    if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
        g_level_block->selected_item = -1;
    }
    if (gXStatus.plsItemList != 0) {
        PListClear(gXStatus.plsItemList);
        return gXStatus.plsItemList != 0;
    }
    gXStatus.plsItemList = PLCreate();
    return gXStatus.plsItemList != 0;
}

// FUNCTION: WIZ8 0x004f8130
bool ItemHasFlags(W8WorldItem* item, unsigned int mask)
{
    if (item == 0) {
        srAssertFail("pItemInfo", ITEM_MANAGER_CPP, 998, 0);
    }
    return (item->flags & mask) != 0;
}

// FUNCTION: WIZ8 0x004f8170
void SetItemFlags(W8WorldItem* item, unsigned int mask, bool enabled)
{
    if (item == 0) {
        srAssertFail("pItemInfo", ITEM_MANAGER_CPP, 1004, 0);
    }
    if (enabled) {
        item->flags |= mask;
    } else {
        item->flags &= ~mask;
    }
}

// FUNCTION: WIZ8 0x004f81c0
void SetItemAndEntityFlags(W8WorldItem* item, unsigned int mask, bool enabled)
{
    if (item == 0) {
        srAssertFail("pItemInfo", ITEM_MANAGER_CPP, 1019, 0);
    }
    if (enabled) {
        item->entity_flags |= mask;
    } else {
        item->entity_flags &= ~mask;
    }
    if (item->p3D != 0) {
        static_cast<W8ItemRep*>(item->p3D->m_pRep)->SetFlags(mask, enabled);
    }
}

// FUNCTION: WIZ8 0x004f8300
int ItemInfoGetNumInGroup(W8WorldItem* item)
{
    if (item == 0) {
        srAssertFail("pItemInfo", ITEM_MANAGER_CPP, 1115,
                     "Bad ITEM_STRUCT in ItemInfoGetNumInGroup");
    }
    item = item->next;
    int count = 1;
    while (item != 0) {
        item = item->next;
        ++count;
    }
    return count;
}

// FUNCTION: WIZ8 0x004f8340
void ItemInfoAddToGroup(W8WorldItem* group, W8WorldItem* item)
{
    W8WorldItem* tail;

    if (group == 0) {
        srAssertFail("pItemInfo", ITEM_MANAGER_CPP, 1130, "Bad ITEM_STRUCT in ItemInfoAddToGroup");
    }
    tail = group;
    while (tail->next != 0) {
        tail = tail->next;
    }
    tail->next = item;
    item->next = 0;
}

struct W8ItemLevelScaleRange {
    unsigned int minimum_party_level;
    unsigned int minimum_item_value;
    unsigned int maximum_item_value;
};

static const W8ItemLevelScaleRange g_item_level_scale_ranges[7] = {
    {1, 0, 500},      {6, 50, 1000},    {11, 100, 3000},     {16, 300, 5000},
    {21, 600, 10000}, {26, 800, 20000}, {31, 1000, 1000000},
};

// FUNCTION: WIZ8 0x004f88a0
int FindItemTableByName(const char* name)
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-compare"
    /* Retail compiled this comparison with VC6's mixed-sign operands; the
   signedness is part of the recovered body and changing it would change
   the compare and branch. Suppress only this diagnostic here. */
    int index;

    for (index = 0; index < (int)gXStatus.uiItemTablesInDatabase; ++index) {
        if (_stricmp(name, g_item_tables[index]->name) == 0) {
            return index;
        }
    }
    return -1;
#pragma clang diagnostic pop
}

static __forceinline W8WorldItem* CreateTableItem(unsigned int item_id,
                                                  const srVector3T<float>* position)
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-compare"
    /* Retail tests the item id against the -1 sentinel with VC6's mixed-sign
   compare; the caller-side id domain keeps the unsigned parameter. */
    W8ItemInstance item;
    W8ItemInstance* item_pointer;
    W8WorldItem* result;

    if (item_id == -1) {
#pragma clang diagnostic pop
        item_pointer = 0;
    } else {
        ReplaceOrCreateItem(&item, item_id, 0, 0, 0);
        item_pointer = &item;
    }
    result = CreateWorldItem(item_pointer, position, 3, 0);
    if (result == 0) {
        srAssertFail("pItemInfo", "C:\\Projects\\Wizardry 8\\Local Code\\ItemManager.cpp", 0x18e,
                     0);
    }
    return result;
}

// FUNCTION: WIZ8 0x004f88f0
int GenerateItemsFromTable(W8GrowableVector<W8WorldItem*>* output_items, unsigned int table_id,
                           unsigned int maximum_items)
{
    unsigned int party_level = GetAveragePartyLevel();
    srVector3T<float> position;
    W8GrowableVector<int> candidates;
    W8ItemTableRecord* table;
    W8ItemTableEntry* entry;
    W8ItemDatabaseRecord* item_record;
    const W8ItemLevelScaleRange* range;
    unsigned int item_value;
    unsigned int total_weight;
    unsigned int selected_count;
    unsigned int random_value;
    int entry_index;
    int position_index;

    table_id &= 0xffff;
    for (entry_index = 0; entry_index < 40; ++entry_index) {
        table = g_item_tables[table_id];
        if (table->entries[entry_index].selector_00 != 0) {
            item_record = &g_item_records[table->entries[entry_index].item_id];
            if (table->entries[entry_index].weight == 0) {
                output_items->Add(CreateTableItem(
                    g_item_tables[table_id]->entries[entry_index].item_id, &position));
            } else if (table->level_scaled == 0) {
                candidates.Add(entry_index);
            } else {
                item_value = item_record->value;
                for (range = g_item_level_scale_ranges; range < g_item_level_scale_ranges + 7;
                     ++range) {
                    if (range->minimum_party_level <= party_level &&
                        range->minimum_item_value <= item_value &&
                        item_value < range->maximum_item_value) {
                        candidates.Add(entry_index);
                        break;
                    }
                }
            }
        }
    }

    if (candidates.GetCount() < (int)maximum_items && g_item_tables[table_id]->level_scaled != 0) {
        candidates.Clear();
        for (entry_index = 0; entry_index < 40; ++entry_index) {
            if (g_item_tables[table_id]->entries[entry_index].selector_00 != 0) {
                candidates.Add(entry_index);
            }
        }
    }

    total_weight = 0;
    for (entry_index = 0; entry_index < candidates.GetCount(); ++entry_index) {
        total_weight += g_item_tables[table_id]->entries[*candidates.GetAt(entry_index)].weight;
    }

    selected_count = 0;
    while (selected_count < maximum_items) {
        if (candidates.GetCount() == 0) {
            break;
        }
        position_index = Random(candidates.GetCount());
        entry_index = *candidates.GetAt(position_index);
        table = g_item_tables[table_id];
        entry = &table->entries[entry_index];
        random_value = Random(total_weight);
        if (random_value <= entry->weight) {
            output_items->Add(CreateTableItem(entry->item_id, &position));
            candidates.RemoveAt(position_index);
            ++selected_count;
        }
    }

    return output_items->GetCount();
}

/* 0x00689B54: the cursor the iterator below resumes from. */

// GLOBAL: WIZ8 0x00689b54
int g_world_item_cursor;

/* Bit 0x20 of the item record's flag word, which is the only bit
   ItemInfoIsWorldPersistent reads. */
enum { W8_ITEM_FLAG_PERSISTENT = 0x20 };
/* Bit 1 of the world item's own flag word. */
enum { W8_WORLD_ITEM_FLAG_02 = 2 };

/* Free a whole group of world items, following the link that chains them. */
// FUNCTION: WIZ8 0x004f6cc0
void FreeWorldItemGroup(W8WorldItem* item)
{
    W8WorldItem* next;

    while (item != 0) {
        next = item->next;
        free(item);
        item = next;
    }
}

/* Look an item record up by its internal name. The name sits at 0x8d in the
   record; a record whose name is empty is matched against the display name
   converted down from wide instead. */
// FUNCTION: WIZ8 0x004f8220
int FindItemRecordByName(const char* name)
{
    int index;
    const char* internal_name;

    for (index = 0; index < (int)gXStatus.uiItemsInDatabase; ++index) {
        internal_name = g_item_records[index].internal_name;
        if (internal_name[0] == 0) {
            if (_stricmp(ConvertWideStringToString(g_item_records[index].display_name), name) ==
                0) {
                return index;
            }
        } else if (_stricmp(internal_name, name) == 0) {
            return index;
        }
    }
    return -1;
}

/* Walk the world item list, resuming where the last call left off. Restarting
   rewinds; running off the end answers nothing without rewinding. */
// FUNCTION: WIZ8 0x004f82b0
W8WorldItem* GetNextWorldItem(char restart)
{
    int index;

    if (restart) {
        g_world_item_cursor = 0;
    }
    index = g_world_item_cursor;
    if ((unsigned int)index < PLLength(gXStatus.plsItemList)) {
        ++g_world_item_cursor;
        return (W8WorldItem*)PLGet(gXStatus.plsItemList, index);
    }
    return 0;
}

/* Take one item out of the group chained onto another. Unlinking the head
   promotes whatever followed it; unlinking anything else just closes the gap.
   Answers the group's head afterwards. */
// FUNCTION: WIZ8 0x004f83a0
W8WorldItem* ItemInfoRemoveFromGroup(W8WorldItem* head, W8WorldItem* item)
{
    W8WorldItem* previous;
    W8WorldItem* scan;

    if (head == 0) {
        srAssertFail("pItemInfo", ITEM_MANAGER_CPP, 1142,
                     "Bad ITEM_STRUCT in ItemInfoRemoveFromGroup");
    }

    if (head == item) {
        return item;
    }

    scan = head;
    do {
        previous = scan;
        if (previous == 0) {
            break;
        }
        scan = previous->next;
        if (scan == 0) {
            break;
        }
    } while (scan != item);

    if (scan == item) {
        if (item->next != 0) {
            previous->next = item->next;
            item->next = 0;
            return head;
        }
        previous->next = 0;
    }
    item->next = 0;
    return head;
}

/* The next item in a group. */
// FUNCTION: WIZ8 0x004f8410
W8WorldItem* ItemInfoGroupGetNext(W8WorldItem* item)
{
    if (item == 0) {
        srAssertFail("pItemInfo", ITEM_MANAGER_CPP, 1178,
                     "Bad ITEM_STRUCT in ItemInfoGroupGetNext");
    }
    return item->next;
}

/* Whether the item's record marks it as one the world keeps. */
// FUNCTION: WIZ8 0x004f91e0
bool ItemInfoIsWorldPersistent(const W8WorldItem* item)
{
    if (item == 0) {
        return false;
    }
    return (g_item_records[item->item.item_id].flags_041 & W8_ITEM_FLAG_PERSISTENT) != 0;
}

/* Copy a world item's carried item out onto the heap. */
// FUNCTION: WIZ8 0x004f9210
W8ItemInstance* CopyWorldItemInstance(const W8WorldItem* item)
{
    W8ItemInstance* copy = (W8ItemInstance*)malloc(0xc);

    if (copy == 0) {
        return 0;
    }
    copy->item_id = item->item.item_id;
    copy->stack_count = item->item.stack_count;
    copy->uses_or_charges = item->item.uses_or_charges;
    copy->identified = item->item.identified;
    copy->unknown_07[0] = item->item.unknown_07[0];
    copy->unknown_07[1] = item->item.unknown_07[1];
    copy->unknown_07[2] = item->item.unknown_07[2];
    copy->bind_announced = item->item.bind_announced;
    copy->bound = item->item.bound;
    return copy;
}

/* Raise or lower bit one of the world item's own flag word. */
// FUNCTION: WIZ8 0x004f94a0
void SetWorldItemFlag02(W8WorldItem* item, char enabled)
{
    if (enabled) {
        item->flags |= W8_WORLD_ITEM_FLAG_02;
    } else {
        item->flags &= ~W8_WORLD_ITEM_FLAG_02;
    }
}

/* The world item at one list position. Both the bound and the fetch are
   asserted, and the second reports the list it failed on. */
// FUNCTION: WIZ8 0x004f7fe0
W8WorldItem* ItemInfo(unsigned int item_list_index)
{
    W8WorldItem* item;

    if (item_list_index >= PLLength(gXStatus.plsItemList)) {
        srAssertFail("uiItemListIndex < (UINT32) PLLength(gXStatus.plsItemList)", ITEM_MANAGER_CPP,
                     961, 0);
    }
    item = (W8WorldItem*)PLGet(gXStatus.plsItemList, item_list_index);
    if (item == 0) {
        srAssertFail("pItemInfo != NULL", ITEM_MANAGER_CPP, 965,
                     FormatString("ItemInfo: ERROR - PLGet failed, index %d, pList %d",
                                  item_list_index, gXStatus.plsItemList));
    }
    return item;
}

/* Where in the world item list one runtime id sits. Not finding it is a data
   error rather than a -1. */
// FUNCTION: WIZ8 0x004f8060
unsigned int ItemIndex(int runtime_id)
{
    unsigned int index;

    for (index = 0; index < PLLength(gXStatus.plsItemList); ++index) {
        if (ItemInfo(index)->runtime_id == runtime_id) {
            return index;
        }
    }
    srAssertFail("FALSE", ITEM_MANAGER_CPP, 992,
                 FormatString("ItemIndex: ERROR - ItemID %d not found", runtime_id));
    return 0;
}

/* 0x0068EDCC: the level runtime block, which also carries the interface
   selection the item manager resets. */

/* Flatten one item's whole group into a vector, the item itself first and then
   everything chained onto it. A failed append drops that entry and the walk
   continues. */
// FUNCTION: WIZ8 0x004f8440
int ItemInfoMakeGroupList(W8WorldItem* item, W8GrowableVector<W8WorldItem*>* out)
{
    W8WorldItem* next;

    if (item == 0) {
        srAssertFail("pItemInfo", ITEM_MANAGER_CPP, 1185,
                     "Bad ITEM_STRUCT in ItemInfoMakeGroupList");
    }

    out->Add(item);

    for (next = item->next; next != 0; next = next->next) {
        out->Add(next);
    }
    return out->count;
}

/* 0x00617D34: the generic 3D model names ActivateItem falls back to when a
   record's internal_name is blank, indexed by unidentified_name_index. */
// GLOBAL: WIZ8 0x00617d34
static const char g_item_model_fallback_names[145][0x1e] = {"Dagger",
                                                            "Long Sword",
                                                            "Bipennis",
                                                            "Battle Axe",
                                                            "Flail",
                                                            "Mace",
                                                            "Hammer",
                                                            "Staff",
                                                            "Halberd",
                                                            "Spear",
                                                            "Bo Stick",
                                                            "Bow",
                                                            "Crossbow",
                                                            "Sling",
                                                            "Great Sword",
                                                            "Rapier",
                                                            "Katana",
                                                            "Long Staff",
                                                            "Wand",
                                                            "Magic Stave",
                                                            "Spellbook",
                                                            "Shuriken",
                                                            "Arrows",
                                                            "Bullets",
                                                            "Kite Shield",
                                                            "Basnet",
                                                            "Plate Torso",
                                                            "Plate Leggings",
                                                            "Gauntlets",
                                                            "Sollerets",
                                                            "Amulet",
                                                            "Ring",
                                                            "Blue Potion",
                                                            "Scroll",
                                                            "Powder",
                                                            "Key",
                                                            "Locket",
                                                            "Great Bow",
                                                            "Round Shield",
                                                            "Wizard Cone",
                                                            "Leather Helm",
                                                            "Cuirass",
                                                            "Leather Top",
                                                            "Greaves",
                                                            "Leather Leggings",
                                                            "Gloves",
                                                            "Sandals",
                                                            "Buskins",
                                                            "Boots",
                                                            "Book",
                                                            "Ankh",
                                                            "Ninjato",
                                                            "War Hammer",
                                                            "Flamberge",
                                                            "Bullwhip",
                                                            "Sai",
                                                            "Nunchuka",
                                                            "Glaive",
                                                            "Black Sword",
                                                            "Fire Sword",
                                                            "Upper Robes",
                                                            "Lower Robes",
                                                            "Halter",
                                                            "Skirt",
                                                            "Skullcap",
                                                            "Feathered Cap",
                                                            "Mitre",
                                                            "Breast Plate",
                                                            "Kabuto",
                                                            "Upper Toseido",
                                                            "Lower Toseido",
                                                            "Burgonet",
                                                            "Fur Leggings",
                                                            "Cloak",
                                                            "Mantis Gloves",
                                                            "Mantis Boots",
                                                            "Garland",
                                                            "Cat 'O Nine Tails",
                                                            "Silver Cross",
                                                            "Ruby Talisman",
                                                            "Purple Amulet",
                                                            "Bracelet",
                                                            "Necklace",
                                                            "Quarrels",
                                                            "Jade Figure",
                                                            "Chain Coif",
                                                            "Upper Chain",
                                                            "Lower Chain",
                                                            "Mail Mittens",
                                                            "Chain Hosen",
                                                            "Lance",
                                                            "Armet",
                                                            "Ninja Cowl",
                                                            "Upper Ninja Garb",
                                                            "Lower Ninja Garb",
                                                            "Tabi Boots",
                                                            "Heaume",
                                                            "Box Helm",
                                                            "Diamond Ring",
                                                            "Bag",
                                                            "Red Potion",
                                                            "Green Potion",
                                                            "Purple Potion",
                                                            "Yellow Potion",
                                                            "Power Glove",
                                                            "T'Rang Staff",
                                                            "Bronze Gauntlets",
                                                            "Comm Link",
                                                            "Light Sword",
                                                            "Magic Bag",
                                                            "Phaser",
                                                            "ID Card",
                                                            "Darts",
                                                            "Powder Shot",
                                                            "Musket",
                                                            "Coin",
                                                            "Stone",
                                                            "Stick",
                                                            "Short Sword",
                                                            "UNDEFINED",
                                                            "Bagpipes",
                                                            "Lyre",
                                                            "Lute",
                                                            "Horn",
                                                            "Map",
                                                            "Bracelet",
                                                            "Shuriken",
                                                            "UNDEFINED",
                                                            "Wand",
                                                            "Yellow Potion",
                                                            "Medium Shield",
                                                            "UNDEFINED",
                                                            "UNDEFINED",
                                                            "UNDEFINED",
                                                            "UNDEFINED",
                                                            "UNDEFINED",
                                                            "Saxophone",
                                                            "Violin",
                                                            "Drum",
                                                            "Bullroarer",
                                                            "bomb",
                                                            "emptybottle",
                                                            "stix",
                                                            "rocket",
                                                            "rocketlauncher"};

/* Put one spawned item into the world: load its 3D model from
   Data\Items3D\Bitmaps, drop it at the item's position with a random yaw,
   attach and register it, then scale it by whether the sun reaches it. */
// FUNCTION: WIZ8 0x004f6cf0
void ActivateItem(W8WorldItem* item)
{
    srVector3T<float> position;
    srVector3T<float> sun_position;
    W8ReadLevelInfo info;
    char zItemName[0x3c];
    char zItemFullPath[0x38];
    const char* name;
    srRegistry::ClassNode* node;
    srNode* sun;
    stModelInstance* mesh;

    if (item == 0) {
        srAssertFail("pItemInfo != NULL", ITEM_MANAGER_CPP, 0x1d4, 0);
    }
    if (item->fActive != 0) {
        srAssertFail("!pItemInfo->fActive", ITEM_MANAGER_CPP, 0x1d5, 0);
    }

    info.world = GetWorld();
    info.hFile = 0;
    info.bitmap_folder = "Data\\Items3D\\Bitmaps";

    name = g_item_records[item->item.item_id].internal_name;
    if (strlen(name) == 0) {
        name =
            g_item_model_fallback_names[g_item_records[item->item.item_id].unidentified_name_index];
    }
    strcpy(zItemName, name);
    if (strstr(zItemName, ".ITM") != 0) {
        sprintf(zItemFullPath, "%s\\%s", "Data\\Items3D", zItemName);
    } else {
        sprintf(zItemFullPath, "%s\\%s.ITM", "Data\\Items3D", zItemName);
    }
    if (!FileExists(zItemFullPath)) {
        strcpy(zItemName, "questionmark");
        sprintf(zItemFullPath, "%s\\%s.ITM", "Data\\Items3D", zItemName);
        if (!FileExists(zItemFullPath)) {
            srAssertFail("FileExists(zItemFullPath)", ITEM_MANAGER_CPP, 0x1ed,
                         FormatString("ActivateItem: ERROR - missing ITM file %s, item %d",
                                      zItemFullPath, item->item.item_id));
        }
    }

    if (LoadItemFromFile(&info, zItemName, &item->p3D, 0) == 0) {
        srAssertFail("fSuccess", ITEM_MANAGER_CPP, 0x1f2,
                     FormatString("ActivateItem: ERROR - ItemRead %s failed", zItemName));
    }
    if (ItemHasFlags(item, 4)) {
        item->p3D->LightRadarBlip();
    }
    position = item->position;
    item->p3D->SetLocation0049F720(&position);
    item->p3D->SetYaw(
        static_cast<float>(Random(0x168) * 2 * g_camera_pi_005ec2a0 * g_double_005ed7b0));
    static_cast<W8ItemRep*>(item->p3D->m_pRep)->flags |= item->entity_flags;
    item->p3D->AttachMesh0049F900(GetWorld());
    AddItemToWorld0046E5C0(GetWorld(), item->p3D);
    item->fActive = 1;
    ++gXStatus.item_manager_pending;

    node = srCore.getRegistry()->getClassNode(0x1000);
    if (node == 0) {
        node = srCore.getRegistry()->registerClass(srNode::sGetClassName(),
                                                   srClass::sGetClassNode(), 0x1000, 1);
    }
    sun = static_cast<srNode*>(srCore.getRegistry()->find(node, "SUN", 0));
    if (sun != 0) {
        sun_position = sun->getLocation();
        mesh = static_cast<stModelInstance*>(item->p3D->GetMesh());
        if (mesh != 0) {
            if (g_octree_6598a4->HasLineOfSight(&position, &sun_position, 1)) {
                mesh->scale_194 = 1.0f;
            } else {
                mesh->scale_194 = 0.0f;
            }
        }
    }
}

/* Take one item out of the world. Its three assertions name the two fields
   they guard - fActive and p3D - and the item keeps its last position and
   entity flags so it can be put back. */
// FUNCTION: WIZ8 0x004f70d0
void DeactivateWorldItem(W8WorldItem* item)
{
    srVector3T<float> position;

    if (item == 0) {
        srAssertFail("pItemInfo != NULL", ITEM_MANAGER_CPP, 554, 0);
    }
    if (item->fActive == 0) {
        srAssertFail("pItemInfo->fActive", ITEM_MANAGER_CPP, 555, 0);
    }
    if (item->p3D == 0) {
        srAssertFail("pItemInfo->p3D != NULL", ITEM_MANAGER_CPP, 556, 0);
    }

    if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0 &&
        g_level_block->selected_item == item->runtime_id) {
        g_level_block->selected_item = -1;
    }

    item->p3D->m_pRep->GetLocation004B8890(&position);
    item->position = position;
    item->entity_flags = static_cast<W8ItemRep*>(item->p3D->m_pRep)->flags;

    item->p3D->DetachMesh0049FA30(GetWorld());
    RemoveItemFromWorld0046E5E0(GetWorld(), item->p3D);
    delete item->p3D;
    item->p3D = 0;
    item->fActive = 0;
    --gXStatus.item_manager_pending;
}

/* Highlight or un-highlight a world item by runtime id: the rep's highlight
   bit tracks the SetHighlight call so the marker and the flag stay in step. */
// FUNCTION: WIZ8 0x004F71E0
void SetWorldItemHighlight(int runtime_id, char on)
{
    W8WorldItem* item = ItemInfo(ItemIndex(runtime_id));
    W8Item* world_item = item->p3D;
    if (world_item == 0) {
        srAssertFail("pItem", ITEM_MANAGER_CPP, 627, 0);
    }
    W8ItemRep* rep = static_cast<W8ItemRep*>(world_item->m_pRep);
    if (on != 0) {
        rep->flags |= 0x10;
        world_item->SetHighlight(true);
        return;
    }
    rep->flags &= ~0x10;
    world_item->SetHighlight(false);
}

/* The runtime id of the nearest active world item the cursor is hovering over
   and whose screen distance stays inside `max_distance`, or -1. The cursor
   coordinates are carried but unused - the hover test is IsSelected. */
// FUNCTION: WIZ8 0x004f7370
int PickNearestItemUnderCursor004F7370(int cursor_x, int cursor_y, float max_distance)
{
    int result;
    float best_distance;
    unsigned int index;

    result = -1;
    best_distance = 999999.0f;
    for (index = 0; index < PLLength(gXStatus.plsItemList); ++index) {
        W8WorldItem* item = ItemInfo(index);
        float distance;

        if (item->fActive == 0) {
            continue;
        }
        if (static_cast<W8ItemRep*>(item->p3D->m_pRep)->flags & 4) {
            continue;
        }
        if (!item->p3D->IsSelected()) {
            continue;
        }
        distance = ItemDistanceToCamera004BE7C0(GetWorld(), item->p3D);
        if (distance < max_distance && distance < best_distance) {
            result = item->runtime_id;
            best_distance = distance;
        }
    }
    return result;
}

/* Walk every world item: repair invalid sectors, advance falling ones, then
   activate inactive items near the camera and deactivate active ones that have
   drifted beyond the far range. */
// FUNCTION: WIZ8 0x004f7480
void UpdateNearbyWorldItems(void)
{
    srVector3T<float> camera;
    unsigned int index;
    unsigned int count;

    WorldGetCameraLocation(GetWorld(), &camera);
    count = PLLength(gXStatus.plsItemList);
    for (index = 0; index < count; ++index) {
        W8WorldItem* item = ItemInfo(index);

        if (item->sector_id < -1) {
            item->sector_id = -1;
            SettleWorldItem(item);
        }
        if (ItemHasFlags(item, 2)) {
            AdvanceFallingWorldItem(item);
        }
        if (item->fActive == 0) {
            if (DistanceBetweenPoints004BE6D0(&item->position, &camera) < g_float_005ed7b8) {
                if (g_byte_0064a1cd != 0) {
                    if (item == 0) {
                        srAssertFail("pItemInfo", ITEM_MANAGER_CPP, 0x3e6, 0);
                    }
                    if (ItemHasFlags(item, 1)) {
                        goto next_item;
                    }
                }
                ActivateItem(item);
            }
        } else {
            srVector3T<float> location;

            item->p3D->m_pRep->GetLocation004B8890(&location);
            if (DistanceBetweenPoints004BE6D0(&location, &camera) > g_float_005ec360) {
                DeactivateWorldItem(item);
            }
        }
    next_item:
        count = PLLength(gXStatus.plsItemList);
    }
}

/* Toss the cursor-held item into the world. The tracked cursor position is
   turned into a 2500-unit direction through the camera rotation, traced to
   the first obstruction, shortened by the item radius, settled onto the
   ground and lifted back up. When the spot is free the held instance becomes
   a world item; otherwise the refusal sound plays. */
// FUNCTION: WIZ8 0x004F7610
void DropHeldItem(int arg_1)
{
    srVector3T<float> cursor;
    GetCursorScaledPosition004282F0(&cursor);
    cursor.y = (cursor.y - g_float_005ebc7c) * g_float_005ec390;
    cursor.z = (cursor.z - g_float_005ebc7c) * g_float_005ec390;

    double amplitude = g_double_005ed7a8 * g_float_005ebcf8;
    double pitch = amplitude * g_float_005ebc28 + amplitude * cursor.z * g_float_005ebf4c;
    double base = cos(pitch) * g_double_005ec030;
    double yaw = amplitude * cursor.y * g_float_005ebf48;

    srVector3T<float> direction;
    direction.y = static_cast<float>(sin(pitch) * g_double_005ed7c0);
    direction.x = static_cast<float>(base * sin(yaw));
    direction.z = static_cast<float>(base * cos(yaw));

    srMatrix3T<float> rotation;
    WorldGetCameraRotation(g_world, &rotation);
    direction = rotation.Transform(direction);

    srVector3T<float> camera;
    GetCameraPosition(&camera);
    direction += camera;
    g_octree_6598a4->TraceLineOfSight(&camera, &direction, 1, -3, -3, 1, 0);

    srVector3T<float> delta = direction - camera;
    float distance_squared = delta.LengthSquared();
    float distance = static_cast<float>(sqrt(distance_squared)) - g_float_005ec3f8;
    if (distance < g_float_005ebb34) {
        distance = g_float_005ebb34;
    }
    if (distance_squared != static_cast<float>(g_zero_005ebb40)) {
        distance /= static_cast<float>(sqrt(distance_squared));
        delta *= distance;
    }

    srVector3T<float> position = camera + delta;
    position.y = g_octree_6598a4->SettleToGround(&position, 0, 1, 250.0f) + g_float_005ec3f8;
    if (FindNearbyFreePosition00451800(250.0f, &position, 1, 1) != 0) {
        W8WorldItem* item = CreateWorldItem(&g_status_685170.item_in_hand_235b, &position, 3, 1);
        if (item == 0) {
            // Retail passes the NULL item pointer as the assert message.
            // reinterpret-ok: pointer-valued assert message argument.
            const char* failed_item = reinterpret_cast<const char*>(item);
            srAssertFail("pItemInfo != NULL", ITEM_MANAGER_CPP, 0x339, failed_item);
        }
        return;
    }
    SoundPlay("Data\\Sound\\Misc\\CantDrop.WAV", 0);
}

/* Destroy callback the item picker installs: the items still held by the
   dialog come back as a group chain. A two-item group returns the picked
   extra to the world at the group's position; a lone item is simply dropped.
   Either way the group's own world entry is found by runtime id, pulled out
   of its sector, deactivated, freed and removed from the list. */
// FUNCTION: WIZ8 0x004f7c50
void OnItemPickerDialogDestroyed004F7C50(W8DialogBase* dialog)
{
    W8WorldItem* group;
    W8WorldItem* item;
    unsigned int index;

    if (dialog == 0) {
        return;
    }
    group = static_cast<W8TriggerItemPickerDialog*>(dialog)->ReturnItemsToGroup();
    if (ItemInfoGetNumInGroup(group) == 2) {
        item = ItemInfoGroupGetNext(group);
        item->position = group->position;
        if (item->p3D == 0) {
            ActivateItem(item);
        }
        PLAdoptAppend(gXStatus.plsItemList, item);
        group->next = 0;
    } else if (ItemInfoGetNumInGroup(group) != 1) {
        return;
    }
    index = ItemIndex(group->runtime_id);
    item = ItemInfo(index);
    if (item->sector_id > -1) {
        RemoveItemFromSector(item->sector_id, item);
    }
    if (item->fActive != 0) {
        DeactivateWorldItem(item);
    }
    FreeWorldItemGroup(item);
    PLRemoveAt(gXStatus.plsItemList, index);
}

/* Interact with one world item by runtime id. Records flagged 0x20 open the
   trigger item-picker instead of being taken; otherwise the item's trigger
   runs, the instance copies into the cursor hand, and the world entry is
   stripped back out of its sector and the list. */
// FUNCTION: WIZ8 0x004f7910
unsigned char InteractWithWorldItem004F7910(int runtime_id)
{
    unsigned char result = 1;
    int index = ItemIndex(runtime_id);
    W8WorldItem* item = ItemInfo(index);

    if (item == 0) {
        srAssertFail("pItemInfo != NULL", ITEM_MANAGER_CPP, 857, 0);
    }
    if (g_item_records[item->item.item_id].flags_041 & 0x20) {
        W8TriggerItemPickerDialog* dialog = new W8TriggerItemPickerDialog;
        if (dialog != 0) {
            dialog->SetItemGroup(item);
            dialog->m_destroy_callback = OnItemPickerDialogDestroyed004F7C50;
        }
        g_modal_owner_0068edd0 = dialog;
        return 1;
    }
    if (item->p3D->trigger_018 != 0) {
        result = RunItemTrigger004A0070(item->p3D);
        if (result == 0) {
            return result;
        }
    }
    if ((static_cast<W8ItemRep*>(item->p3D->m_pRep)->flags & 4) == 0) {
        CopyItemInstance(&g_status_685170.item_in_hand_235b, &item->item, 0, 1);
    }
    index = ItemIndex(runtime_id);
    item = ItemInfo(index);
    if (item->sector_id > -1) {
        RemoveItemFromSector(item->sector_id, item);
    }
    if (item->fActive != 0) {
        DeactivateWorldItem(item);
    }
    FreeWorldItemGroup(item);
    void* status = PLRemoveAt(gXStatus.plsItemList, index);
    if (status == 0) {
        srAssertFail("fStatus", ITEM_MANAGER_CPP, 893, 0);
    }
    return result;
}

/* Push every live world item back to its sector and free the whole list. The
   list is reduced from its head until empty and then destroyed; only the
   destroy failure keeps the global pointing at it. */
// FUNCTION: WIZ8 0x004f6a50
unsigned char ReleaseItemLists(void)
{
    unsigned int count;

    if (gXStatus.plsItemList == 0) {
        srAssertFail("gXStatus.plsItemList != NULL", ITEM_MANAGER_CPP, 0x126, 0);
    }
    count = PLLength(gXStatus.plsItemList);
    while ((int)count >= 1) {
        W8WorldItem* item;

        count = PLLength(gXStatus.plsItemList);
        if (count == 0) {
            srAssertFail("uiItemListIndex < (UINT32) PLLength(gXStatus.plsItemList)",
                         ITEM_MANAGER_CPP, 0x3c1, 0);
        }
        item = static_cast<W8WorldItem*>(PLGet(gXStatus.plsItemList, 0));
        if (item == 0) {
            srAssertFail("pItemInfo != NULL", ITEM_MANAGER_CPP, 0x3c5,
                         FormatString("ItemInfo: ERROR - PLGet failed, index %d, pList %d", 0,
                                      gXStatus.plsItemList));
        }
        if (item->sector_id >= 0) {
            RemoveItemFromSector(item->sector_id, item);
        }
        if (item->fActive != 0) {
            DeactivateWorldItem(item);
        }
        while (item != 0) {
            W8WorldItem* next = item->next;

            free(item);
            item = next;
        }
        if (PLRemoveAt(gXStatus.plsItemList, 0) == 0) {
            return 0;
        }
        count = PLLength(gXStatus.plsItemList);
    }
    if (PLDestroy(gXStatus.plsItemList) == 0) {
        return 0;
    }
    gXStatus.plsItemList = 0;
    return 1;
}

/* Whether one world item is close enough to a point to be reached, and in
   sight of the party's eye. The distance is compared before the trace, so a
   far item is never traced to. */
// FUNCTION: WIZ8 0x004f8560
bool IsWorldItemWithinReach(W8Item* owner, const srVector3T<float>* from, float radius)
{
    srVector3T<float> position;
    srVector3T<float> lower;
    srVector3T<float> upper;
    srVector3T<float> eye;

    owner->m_pRep->GetLocation004B8890(&position);
    GetCameraPosition(&eye);

    srVector3T<float> delta(position.x - from->x, position.y - from->y, position.z - from->z);
    if (delta.LengthSquared() < radius * radius) {
        owner->GetCachedLocalBounds(&lower, &upper);
        lower.x += position.x;
        lower.y += position.y;
        lower.z += position.z;
        upper.x += position.x;
        upper.y += position.y;
        upper.z += position.z;
        if (ShowTargetMarker(&eye, &lower, &upper)) {
            return 1;
        }
    }
    return 0;
}

/* plsItemList index that last satisfied AnyWorldItemVisible. */
// GLOBAL: WIZ8 0x00618E70
static int g_last_visible_world_item_00618e70 = -1;

/* Any live world item visible to the camera within g_double_005ec030, resuming
   the scan at the last match. */
// FUNCTION: WIZ8 0x004f8650
bool AnyWorldItemVisible(void)
{
    srVector3T<float> camera;
    srVector3T<float> position;
    srVector3T<float> eye;
    srVector3T<float> lower;
    srVector3T<float> upper;
    int count;
    int index;

    if (g_world == 0 || g_world->camera == 0) {
        return 0;
    }
    GetCameraPosition(&camera);
    count = PLLength(gXStatus.plsItemList);
    if (0 <= g_last_visible_world_item_00618e70 && g_last_visible_world_item_00618e70 < count) {
        W8WorldItem* item = static_cast<W8WorldItem*>(
            PLGet(gXStatus.plsItemList, g_last_visible_world_item_00618e70));
        if (item->p3D != 0) {
            item->p3D->m_pRep->GetLocation004B8890(&position);
            GetCameraPosition(&eye);
            srVector3T<float> delta(position.x - camera.x, position.y - camera.y,
                                    position.z - camera.z);
            if (delta.Length() < static_cast<float>(g_double_005ec030)) {
                item->p3D->GetCachedLocalBounds(&lower, &upper);
                lower.x += position.x;
                lower.y += position.y;
                lower.z += position.z;
                upper.x += position.x;
                upper.y += position.y;
                upper.z += position.z;
                if (ShowTargetMarker(&eye, &lower, &upper) != 0) {
                    return 1;
                }
            }
        }
    }
    for (index = 0; index < count; ++index) {
        W8WorldItem* item = static_cast<W8WorldItem*>(PLGet(gXStatus.plsItemList, index));

        if (item->p3D != 0) {
            item->p3D->m_pRep->GetLocation004B8890(&position);
            GetCameraPosition(&eye);
            srVector3T<float> delta(position.x - camera.x, position.y - camera.y,
                                    position.z - camera.z);
            if (delta.Length() < static_cast<float>(g_double_005ec030)) {
                item->p3D->GetCachedLocalBounds(&lower, &upper);
                lower.x += position.x;
                lower.y += position.y;
                lower.z += position.z;
                upper.x += position.x;
                upper.y += position.y;
                upper.z += position.z;
                if (ShowTargetMarker(&eye, &lower, &upper) != 0) {
                    g_last_visible_world_item_00618e70 = index;
                    return 1;
                }
            }
        }
    }
    return 0;
}

/* Advance one falling world item (flag bit 1) toward the ground for this
   frame. Clears the falling flag once it has landed or the settle misses. */
// FUNCTION: WIZ8 0x004f9240
unsigned char AdvanceFallingWorldItem(W8WorldItem* item)
{
    srVector3T<float> probe;
    unsigned char hit;
    float ground;
    float previous_y;
    float dt;
    int sector;

    if (!ItemHasFlags(item, 2)) {
        return 0;
    }

    previous_y = item->position.y;
    probe.x = item->position.x;
    probe.z = item->position.z;
    probe.y = previous_y + g_world_scale_005ebc40;
    dt = g_game_time_accumulator_6598bc->GetValue28();
    if (g_camera_snap_epsilon_005ebc2c < item->vertical_velocity_35) {
        probe.y = dt * item->vertical_velocity_35 + probe.y;
    }

    ground = g_octree_6598a4->SettleToGround(&probe, &hit, 1, 250.0f);
    if (hit == 0 || fabs(ground - previous_y) < g_camera_snap_epsilon_005ebc2c) {
        item->flags &= ~2u;
        item->vertical_velocity_35 = 0.0f;
        return 0;
    }

    sector = g_octree_6598a4->current_prop;
    if (sector != item->sector_id) {
        if (item->sector_id >= 0) {
            RemoveItemFromSector(item->sector_id, item);
        }
        if (sector >= 0) {
            AddItemToSector(sector, item);
        }
        item->sector_id = sector;
    }

    if (previous_y <= ground) {
        item->vertical_velocity_35 = (ground - previous_y) / dt;
        probe.y = ground;
    } else {
        item->vertical_velocity_35 =
            dt * g_navigator_gravity_00603acc * g_float_005ebc7c + item->vertical_velocity_35;
        probe.y = probe.y - dt * item->vertical_velocity_35;
        if (probe.y < ground) {
            probe.y = ground;
        }
    }

    if (item->p3D != 0) {
        item->p3D->SetLocation0049F720(&probe);
    }
    item->position = probe;
    return 1;
}

/* Drop one item onto the ground below where it is. The search starts one world
   unit up so an item already resting does not settle into the floor; landing
   moves it between sectors and clears its saved-marker flag. */
// FUNCTION: WIZ8 0x004f93d0
unsigned char SettleWorldItem(W8WorldItem* item)
{
    srVector3T<float> start;
    unsigned char hit;
    int sector;

    start.x = item->position.x;
    start.z = item->position.z;
    start.y = item->position.y + g_world_scale_005ebc40;

    item->flags &= ~2u;
    item->vertical_velocity_35 = 0.0f;

    g_octree_6598a4->SettleToGround(&start, &hit, 1, 250.0f);
    if (hit == 0) {
        return 0;
    }

    sector = g_octree_6598a4->current_prop;
    if (sector != item->sector_id) {
        if (item->sector_id >= 0) {
            RemoveItemFromSector(item->sector_id, item);
        }
        if (sector >= 0) {
            AddItemToSector(sector, item);
        }
        item->sector_id = sector;
    }
    if (item->p3D != 0) {
        item->p3D->SetLocation0049F720(&start);
    }
    item->position = start;
    return 1;
}

/* Rebuild every world item's carried instance from its own item id, which
   re-rolls whatever the record decides rather than keeping what was there. */
// FUNCTION: WIZ8 0x004f94c0
void RebuildAllWorldItemInstances(void)
{
    unsigned int index;
    W8WorldItem* item;

    for (index = 0; index < PLLength(gXStatus.plsItemList); ++index) {
        item = ItemInfo(index);
        ReplaceOrCreateItem(&item->item, item->item.item_id, 0, 0, 0);
    }
}

// FUNCTION: WIZ8 0x004f6b90
W8WorldItem* CreateWorldItem(W8ItemInstance* item, const srVector3T<float>* position,
                             int entity_flags, unsigned char add_to_world)
{
    W8WorldItem* result = (W8WorldItem*)malloc(sizeof(W8WorldItem));

    if (result == 0) {
        return 0;
    }

    memset(result, 0, sizeof(W8WorldItem));
    EmptyItemRecord(&result->item, 0, 1);
    result->runtime_id = g_status_685170.next_world_item_id_2352++;
    result->fActive = 0;
    result->p3D = 0;
    result->position = *position;
    result->sector_id = -1;
    SettleWorldItem(result);
    result->entity_flags = entity_flags;

    if (item != 0) {
        CopyItemInstance(&result->item, item, 0, 1);
    }
    if (add_to_world && PLAdoptAppend(gXStatus.plsItemList, result) == -1) {
        return 0;
    }
    return result;
}

// FUNCTION: WIZ8 0x004f6c50
W8WorldItem* SpawnItem(int item_id, const srVector3T<float>* position, int entity_flags,
                       unsigned char add_to_world)
{
    W8ItemInstance local_item;
    W8ItemInstance* item;
    W8WorldItem* result;

    if (item_id == -1) {
        item = 0;
    } else {
        ReplaceOrCreateItem(&local_item, item_id, 0, 0, 0);
        item = &local_item;
    }

    result = CreateWorldItem(item, position, entity_flags, add_to_world);
    if (result == 0) {
        srAssertFail("pItemInfo", "C:\\Projects\\Wizardry 8\\Local Code\\ItemManager.cpp", 0x18e,
                     0);
    }
    return result;
}

/* Monster loot drop, driven by the death switch in MonsterManager. Non-NPC
   records fire their treasure slots (count gate, Random(100) chance, dice for
   the drop count; type 1 routes the id to GenerateItemsFromTable once per
   roll) and roll the gold dice. NPC-bound monsters empty their state item
   list instead: every zero-weight slot drops outright, then a weighted pick
   rolls item_count_dice_10e items out of what remains; the NPC's gold field
   pays out directly. One drop is adopted into plsItemList at the death
   position; two or more are chained onto a container spawned with
   SpawnItem - 0x23c by default, or value when the caller names one. */
// FUNCTION: WIZ8 0x004f8cb0
void DropMonsterLoot(W8MonsterInfo* monster_info, int value)
{
    W8GrowableVector<W8WorldItem*> items(5);
    if (monster_info != 0) {
        W8MonsterRecord* record = GetMonsterDataForInfo(monster_info);
        srVector3T<float> position = monster_info->monster->GetPosition();
        W8WorldItem* item;
        int gold = 0;
        int slot;
        int roll;
        int index;
        unsigned int remaining;
        int total_weight;
        unsigned int take;
        unsigned int taken;
        int pick;
        if ((record->flags_0d0 & 1) == 0) {
            for (slot = 0; slot < 8; ++slot) {
                W8MonsterTreasureEntry* entry = &record->treasure_1c3.slots[slot];
                if (entry->count != 0 && Random(100) <= entry->chance) {
                    int rolls = RollDice(&entry->dice);
                    for (roll = 0; roll < rolls; ++roll) {
                        if (entry->type == 0) {
                            item = SpawnItem(entry->item_id, &position, 3, 0);
                            if (item != 0) {
                                items.Add(item);
                            }
                        } else if (entry->type == 1) {
                            GenerateItemsFromTable(&items, entry->item_id, rolls);
                        }
                    }
                }
            }
            gold = RollDice(&record->treasure_1c3.gold_dice);
        } else {
            W8NpcState* npc = GetNpcStateForMonsterInfo(monster_info, 1);
            if (npc == 0) {
                srAssertFail("pNPC", ITEM_MANAGER_CPP, 1459,
                             FormatString("Monster %S marked as NPC with no NPC data", record));
            }
            for (index = 0; index < 40; ++index) {
                if (npc->item_ids_30[index] != -1 && npc->item_weights_115[index] == 0) {
                    item = SpawnItem(npc->item_ids_30[index], &position, 3, 0);
                    if (item != 0) {
                        items.Add(item);
                    }
                    npc->item_ids_30[index] = -1;
                }
            }
            remaining = 0;
            for (index = 0; index < 40; ++index) {
                if (npc->item_ids_30[index] != -1) {
                    ++remaining;
                }
            }
            if (remaining != 0) {
                total_weight = 0;
                for (index = 0; index < 40; ++index) {
                    if (npc->item_ids_30[index] != -1) {
                        total_weight += static_cast<signed char>(npc->item_weights_115[index]);
                    }
                }
                take = RollDice(&npc->item_count_dice_10e);
                if (remaining < take) {
                    take = remaining;
                }
                taken = 0;
                while (taken < take) {
                    pick = Random(40);
                    if (npc->item_ids_30[pick] != -1 &&
                        static_cast<short>(Random(total_weight)) <=
                            static_cast<short>(
                                static_cast<signed char>(npc->item_weights_115[pick]))) {
                        item = SpawnItem(npc->item_ids_30[pick], &position, 3, 0);
                        if (item != 0) {
                            items.Add(item);
                        }
                        ++taken;
                        npc->item_ids_30[pick] = -1;
                    }
                }
            }
            gold = npc->gold_80;
        }
        if (gold != 0) {
            AddPartyGold(gold, 1);
        }
        if (items.GetCount() <= 1) {
            item = *items.GetAt(0);
            PLAdoptAppend(gXStatus.plsItemList, item);
            item->position = position;
        } else {
            W8WorldItem* container;
            int item_index;
            if (value == -1) {
                container = SpawnItem(0x23c, &position, 2, 1);
            } else {
                container = SpawnItem(value, &position, 3, 1);
            }
            for (item_index = 0; item_index < items.GetCount(); ++item_index) {
                ItemInfoAddToGroup(container, *items.GetAt(item_index));
            }
        }
    }
}
