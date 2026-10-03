typedef unsigned char BOOLEAN;

extern "C" void* memset(void* destination, int value, unsigned int size);

void mutate(unsigned char& value);

BOOLEAN IsVendorEnabled()
{
    return 1;
}

unsigned char OpenRendererWindow()
{
    return 1;
}

unsigned char HasEntries(int count)
{
    return count > 0;
}

unsigned char Both(bool left, bool right)
{
    return static_cast<unsigned char>(left) & static_cast<unsigned char>(right);
}

unsigned char HasLineOfSightToBounds0046FD70(const void* origin, const void* minimum,
                                             const void* maximum);

unsigned char IsVisibleToPlayer(bool use_bounds)
{
    if (use_bounds) {
        return HasLineOfSightToBounds0046FD70(0, 0, 0);
    }
    return true;
}

struct State {
    State();

    unsigned char m_dirty;
    unsigned char mask;
};

State::State() : m_dirty(0), mask(0) {}

struct SelectionState {
    unsigned char selection_settled;
};

void update_selection(SelectionState& state, bool done)
{
    unsigned char settled = 0;
    if (done) {
        settled = 1;
    }
    state.selection_settled = settled;
    if (state.selection_settled) {
    }
}

struct RuntimeBlock {
    unsigned char transition_active;
    unsigned char byte_count;
};

void clear_runtime(RuntimeBlock* state)
{
    memset(state, 0, sizeof(RuntimeBlock));
    if (!state->transition_active) {
    }
}

void update(State& state, bool left, bool right)
{
    state.m_dirty = 1;
    if (state.m_dirty) {
    }

    state.mask = 1;
    state.mask |= 2;

    unsigned char left_ready = 0;
    unsigned char right_ready = 1;
    unsigned char both_ready = left_ready & right_ready;
    if (both_ready && Both(left, right)) {
    }

    unsigned char escaped_ready = 0;
    mutate(escaped_ready);

    unsigned char item_count = 1;
    if (item_count) {
    }

    unsigned char fCombatMode = 0;
    unsigned char talking = 1;
    if (fCombatMode && talking) {
    }

    unsigned char bRepType = 0;
    if (bRepType) {
    }

    unsigned char wire_ready = 0; // bool-byte-ok: serialized protocol byte
    (void)wire_ready;
}

struct WireRecord {
    unsigned char monster_bound;
};

struct ReferencedRecord {
    unsigned char has_group;
};

struct NestedWireField {
    unsigned char nested_active;
};

struct NestedWireRecord {
    NestedWireField nested;
};

struct ObservedRecord {
    unsigned char display_active;
};

void read_bytes(void* destination, unsigned int size);
void mutate_record(ReferencedRecord& record);
void inspect_record(const ObservedRecord* record);

void aggregate_escapes(WireRecord* wire, ReferencedRecord& referenced,
                       NestedWireRecord* nested, ObservedRecord* observed)
{
    memset(wire, 0, sizeof(*wire));
    read_bytes(wire, sizeof(*wire));
    if (wire->monster_bound) {}

    referenced.has_group = 0;
    mutate_record(referenced);
    if (referenced.has_group) {}

    memset(nested, 0, sizeof(*nested));
    nested->nested.nested_active = 1;
    read_bytes(nested, sizeof(*nested));
    if (nested->nested.nested_active) {}

    memset(observed, 0, sizeof(*observed));
    inspect_record(observed);
    if (observed->display_active) {}
}

void SetPanelVisible(unsigned char visible)
{
    if (visible) {
    }
}

void SetPanelLayered(unsigned char layered)
{
    if (layered) {
    }
}

void SetCallbackEnabled(unsigned char enabled)
{
    if (enabled) {
    }
}

void (*g_enable_callback)(unsigned char) = SetCallbackEnabled;

unsigned char g_menu_active = 0;

int parameter_and_numeric_uses()
{
    SetPanelVisible(1);
    SetPanelVisible(0);
    SetPanelLayered(2);
    SetCallbackEnabled(1);
    g_menu_active = 1;
    return 3 + g_menu_active;
}

struct HandedRecord {
    unsigned char handed_ready;
};

void settle_handed(HandedRecord* record)
{
    record->handed_ready = 1;
}

void handed_record_use(HandedRecord* record)
{
    record->handed_ready = 0;
    settle_handed(record);
    if (record->handed_ready) {
    }
}
