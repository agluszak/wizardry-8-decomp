#include "wiz8/gameplay_boundaries.h"

/* 0x00683F78 is a fixed three-dword owner for one eight-byte runtime row per
   item database record.  The element constructor at 0x004023A0 is empty, but
   the original still uses typed array-new and therefore carries an array
   cookie; retaining an actual C++ row preserves that ownership model. */
struct W8ItemRuntimeRow {
    unsigned int value_00;
    unsigned int value_04;

    W8ItemRuntimeRow() {}
};

struct W8ItemRuntimeCatalog {
    W8ItemRuntimeRow* rows;
    int count;
    int used;
};

typedef char W8ItemRuntimeRow_must_be_8[
    sizeof(W8ItemRuntimeRow) == 8 ? 1 : -1];
typedef char W8ItemRuntimeCatalog_must_be_0x0c[
    sizeof(W8ItemRuntimeCatalog) == 0x0c ? 1 : -1];

W8ItemRuntimeCatalog g_item_runtime_catalog_683f78;

// FUNCTION: WIZ8 0x0055cdc0
void InitializeItemRuntimeCatalog(W8ItemRuntimeCatalog* catalog, int count)
{
    catalog->count = count;
    catalog->rows = new W8ItemRuntimeRow[count];
    catalog->used = 0;
}

// FUNCTION: WIZ8 0x0051b560
void Function51B560(void)
{
    InitializeItemRuntimeCatalog(&g_item_runtime_catalog_683f78,
                                 g_item_record_count);
}
