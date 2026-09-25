/* Local Screens\mipeEdit.cpp — prop field editor of the debug MIPE panel.

   Editing is driven by a table of 0x18-byte W8MipeEditField descriptors:
   digits select a row (or append to the selected row's numeric value), arrows
   scroll the table or bump the selected value, Enter commits the edited bits
   back into the prop's trigger action data and Escape abandons the edit. */

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "wiz8/engine_code/3dapi.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/float_constants.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/local_screens/mipe.h"
#include "wiz8/sr_api.h"

#define MIPE_EDIT_CPP "C:\\Projects\\Wizardry 8\\Local Screens\\mipeEdit.cpp"

#define NUM_VAR_SETS 1

/* Decimal-place counter while typing a fraction into a type-1 field: -1 until
   '.' starts the fractional digits. */
// GLOBAL: WIZ8 0x0064e000
static signed char g_mipe_edit_decimal_0064e000 = -1;

/* Row labels for the prop field editor, one per W8MipeEditField label_index. */
// GLOBAL: WIZ8 0x0064e004
static wchar_t g_mipe_prop_labels_0064e004[][0x80] = {
    L"Open",   L"Lockable", L"Locked",   L"AutoLocking", L"One Way",
    L"Secret", L"Found",    L"Jammable", L"Jammed",      L"Key",
};

/* Key names for the type-7 "Key" row, indexed by option_base + value. */
// GLOBAL: WIZ8 0x0064ea04
static wchar_t g_mipe_key_names_0064ea04[][0x80] = {
    L"Any Key",
    L"Master Key",
    L"Gold Key",
};

/* The door-prop field table: nine bit flags packed into flags_008/flags_009
   plus the key id written to item_00a by CommitMipeEditFields. */
// GLOBAL: WIZ8 0x0064ed08
static W8MipeEditField g_mipe_prop_fields_0064ed08[10] = {
    {5, 0, 0, 0, 0, 0.0f, 1}, {5, 1, 0, 0, 0, 0.0f, 1}, {5, 2, 0, 0, 0, 0.0f, 1},
    {5, 3, 0, 0, 0, 0.0f, 1}, {5, 4, 0, 0, 0, 0.0f, 1}, {5, 5, 0, 0, 0, 0.0f, 1},
    {5, 6, 0, 0, 0, 0.0f, 1}, {5, 7, 0, 0, 0, 0.0f, 1}, {5, 8, 0, 0, 0, 0.0f, 1},
    {7, 9, 3, 0, 0, 0.0f, 0},
};

/* Field table per variable set; only set 0 (prop trigger flags) exists. */
// GLOBAL: WIZ8 0x0064edf8
static W8MipeEditField* g_mipe_var_set_fields_0064edf8[NUM_VAR_SETS] = {
    g_mipe_prop_fields_0064ed08,
};

// GLOBAL: WIZ8 0x0064edfc
static int g_mipe_prop_field_count_0064edfc = 10;

/* Running digit accumulator while typing a numeric field value. */
// GLOBAL: WIZ8 0x0069c510
static float g_mipe_edit_accum_0069c510;

static void DrawMipeEditFieldRow(W8MipeEditField* field, unsigned int palette, char row);
static void CommitMipeEditFields(W8TriggerActionData* data, signed char bVarSet);

/* Redraws the up-to-seven visible editor rows starting at
   g_mipe_table_base_0068f120; the selected row gets palette 6, the rest 15. */
#define MIPE_REDRAW_EDIT_FIELDS()                                                                  \
    do {                                                                                           \
        W8MipeState* state_ = g_mipe_state_0068f100;                                               \
        char shown_ = static_cast<char>(state_->edit_field_count -                                 \
                                        static_cast<char>(g_mipe_table_base_0068f120));            \
        char row_;                                                                                 \
        if (shown_ >= 8) {                                                                         \
            shown_ = 7;                                                                            \
        }                                                                                          \
        ResetEditorStatusLine(-1);                                                                 \
        for (row_ = static_cast<char>(g_mipe_table_base_0068f120);                                 \
             row_ < g_mipe_table_base_0068f120 + shown_; ++row_) {                                 \
            DrawMipeEditFieldRow(&state_->edit_fields[static_cast<int>(row_)],                     \
                                 row_ != state_->edit_selection ? 15 : 6, row_);                   \
        }                                                                                          \
    } while (0)

/* Key handler for the prop field editor: digits pick or edit a row, arrows
   scroll/adjust, Backspace and printable keys edit type-6 text, Enter commits
   to the prop's trigger action data and Escape cancels the selection. */
// FUNCTION: WIZ8 0x005c3880
void HandleMipeEditPropKey(unsigned short key)
{
    W8MipeState* state = g_mipe_state_0068f100;
    W8MipeEditField* fields = state->edit_fields;
    W8MipeEditField* field;
    Trigger* trigger;
    size_t len;

    if (key == 8) {
        if (state->edit_selection != -1) {
            field = &fields[static_cast<int>(state->edit_selection)];
            if (field->type == 6 && field->text != 0 && static_cast<int>(wcslen(field->text)) > 0) {
                field->text[wcslen(field->text) - 1] = 0;
            }
            MIPE_REDRAW_EDIT_FIELDS();
        }
        return;
    }
    if (((key > 0x40 && key < 0x5b) || (key > 0x2f && key < 0x3a) || key == 0x20) &&
        state->edit_selection != -1) {
        field = &fields[static_cast<int>(state->edit_selection)];
        if (field->type == 6) {
            if (field->text == 0) {
                field->text = static_cast<wchar_t*>(malloc(0x100));
                field->text[0] = 0;
            }
            len = wcslen(field->text);
            if (static_cast<int>(len) < 0x7e) {
                if (key != 0x20 && g_shift_held_006f0530 == 0 && (key < 0x30 || key > 0x39)) {
                    key += 0x20;
                }
                field->text[len] = key;
                field->text[len + 1] = 0;
            }
            MIPE_REDRAW_EDIT_FIELDS();
        }
        return;
    }
    switch (key) {
    case 0xd:
        trigger = state->prop->GetValue18();
        if (trigger != 0) {
            CommitMipeEditFields(trigger->m_pActionData, 0);
        }
        state->edit_selection = -1;
        MIPE_REDRAW_EDIT_FIELDS();
        break;
    case 0x1b:
        state->edit_selection = -1;
        MIPE_REDRAW_EDIT_FIELDS();
        break;
    case 0x26:
        if (state->edit_selection == -1) {
            if (g_mipe_table_base_0068f120 != 0) {
                --g_mipe_table_base_0068f120;
            }
        } else {
            field = &fields[static_cast<int>(state->edit_selection)];
            if (field->type == 7) {
                if (field->value > 0) {
                    --field->value;
                }
            } else if (field->type == 5) {
                field->value = field->value == 0;
            }
        }
        MIPE_REDRAW_EDIT_FIELDS();
        break;
    case 0x28:
        if (state->edit_selection == -1) {
            if (g_mipe_table_base_0068f120 < state->edit_field_count - 1) {
                ++g_mipe_table_base_0068f120;
            }
        } else {
            field = &fields[static_cast<int>(state->edit_selection)];
            if (field->type == 7) {
                if (field->value < field->option_count - 1) {
                    ++field->value;
                }
            } else if (field->type == 5) {
                field->value = field->value == 0;
            }
        }
        MIPE_REDRAW_EDIT_FIELDS();
        break;
    case 0x2e:
        if (g_mipe_edit_decimal_0064e000 == -1) {
            g_mipe_edit_decimal_0064e000 = 0;
            MIPE_REDRAW_EDIT_FIELDS();
        }
        break;
    case 0x30:
    case 0x31:
    case 0x32:
    case 0x33:
    case 0x34:
    case 0x35:
    case 0x36:
    case 0x37:
    case 0x38:
    case 0x39:
        if (state->edit_selection == -1) {
            state->edit_selection = static_cast<signed char>(key - 0x30);
            g_mipe_edit_accum_0069c510 = 0.0f;
            field = &fields[static_cast<int>(state->edit_selection)];
            g_mipe_edit_decimal_0064e000 = -1;
            if (field->type == 6 && field->text != 0) {
                field->text[0] = 0;
            }
        } else {
            field = &fields[static_cast<int>(state->edit_selection)];
            if (field->type == 4) {
                g_mipe_edit_accum_0069c510 =
                    g_mipe_edit_accum_0069c510 * g_float_005ebc88 + (key - 0x30);
                field->value = static_cast<int>(g_mipe_edit_accum_0069c510);
            } else if (field->type == 1) {
                g_mipe_edit_accum_0069c510 =
                    g_mipe_edit_accum_0069c510 * g_float_005ebc88 + (key - 0x30);
                if (g_mipe_edit_decimal_0064e000 < 0) {
                    field->float_value = g_mipe_edit_accum_0069c510;
                } else {
                    ++g_mipe_edit_decimal_0064e000;
                    field->float_value = static_cast<float>(
                        g_mipe_edit_accum_0069c510 / pow(10.0, g_mipe_edit_decimal_0064e000));
                }
            }
        }
        MIPE_REDRAW_EDIT_FIELDS();
        break;
    }
}

/* Renders one editor row: "index) label: value" with the value format chosen
   by the field type. */
// FUNCTION: WIZ8 0x005c41d0
static void DrawMipeEditFieldRow(W8MipeEditField* field, unsigned int palette, char row)
{
    if (field->type == 1) {
        if (g_mipe_edit_decimal_0064e000 == 0) {
            ShowNoticef(palette, L"%d) %s: %g.", static_cast<int>(row),
                        g_mipe_prop_labels_0064e004[field->label_index], field->float_value);
        } else {
            ShowNoticef(palette, L"%d) %s: %g", static_cast<int>(row),
                        g_mipe_prop_labels_0064e004[field->label_index], field->float_value);
        }
    } else if (field->type == 2 || field->type == 4) {
        ShowNoticef(palette, L"%d) %s: %d", static_cast<int>(row),
                    g_mipe_prop_labels_0064e004[field->label_index], field->value);
    } else if (field->type == 5) {
        if (field->value != 0) {
            ShowNoticef(palette, L"%d) %s: true", static_cast<int>(row),
                        g_mipe_prop_labels_0064e004[field->label_index]);
        } else {
            ShowNoticef(palette, L"%d) %s: false", static_cast<int>(row),
                        g_mipe_prop_labels_0064e004[field->label_index]);
        }
    } else if (field->type == 6) {
        ShowNoticef(palette, L"%d) %s: %s", static_cast<int>(row),
                    g_mipe_prop_labels_0064e004[field->label_index], field->text);
    } else if (field->type == 7) {
        ShowNoticef(palette, L"%d) %s: %s", static_cast<int>(row),
                    g_mipe_prop_labels_0064e004[field->label_index],
                    g_mipe_key_names_0064ea04[field->option_base + field->value]);
    }
}

/* Packs the edited fields of variable set bVarSet back into a trigger's action
   data: bits 0-7 of flags_008, bit 0 of flags_009 and the key id item_00a. */
// FUNCTION: WIZ8 0x005c4340
static void CommitMipeEditFields(W8TriggerActionData* data, signed char bVarSet)
{
    W8MipeEditField* fields;

    if (bVarSet >= NUM_VAR_SETS) {
        srAssertFail("bVarSet < NUM_VAR_SETS", MIPE_EDIT_CPP, 0x20a, 0);
    }
    fields = g_mipe_var_set_fields_0064edf8[static_cast<int>(bVarSet)];
    if (bVarSet == 0) {
        static_cast<W8DoorTriggerActionData*>(data)->flags_008 =
            (static_cast<W8DoorTriggerActionData*>(data)->flags_008 & ~1) | (fields[0].value & 1);
        static_cast<W8DoorTriggerActionData*>(data)->flags_008 =
            (static_cast<W8DoorTriggerActionData*>(data)->flags_008 & ~2) |
            ((fields[1].value & 1) << 1);
        static_cast<W8DoorTriggerActionData*>(data)->flags_008 =
            (static_cast<W8DoorTriggerActionData*>(data)->flags_008 & ~4) |
            ((fields[2].value & 1) << 2);
        static_cast<W8DoorTriggerActionData*>(data)->flags_008 =
            (static_cast<W8DoorTriggerActionData*>(data)->flags_008 & ~8) |
            ((fields[3].value & 1) << 3);
        static_cast<W8DoorTriggerActionData*>(data)->flags_008 =
            (static_cast<W8DoorTriggerActionData*>(data)->flags_008 & ~0x10) |
            ((fields[4].value & 1) << 4);
        static_cast<W8DoorTriggerActionData*>(data)->flags_008 =
            (static_cast<W8DoorTriggerActionData*>(data)->flags_008 & ~0x20) |
            ((fields[5].value & 1) << 5);
        static_cast<W8DoorTriggerActionData*>(data)->flags_008 =
            (static_cast<W8DoorTriggerActionData*>(data)->flags_008 & ~0x40) |
            ((fields[6].value & 1) << 6);
        static_cast<W8DoorTriggerActionData*>(data)->flags_008 =
            (fields[7].value << 7) |
            (static_cast<W8DoorTriggerActionData*>(data)->flags_008 & 0x7f);
        static_cast<W8DoorTriggerActionData*>(data)->flags_009 =
            (static_cast<W8DoorTriggerActionData*>(data)->flags_009 & ~1) | (fields[8].value & 1);
        static_cast<W8DoorTriggerActionData*>(data)->item_00a =
            static_cast<signed char>(fields[9].value);
    }
}
