#ifdef WIZ8_BOOL_TEST_INVALID
int invalid = ; // The wrapper must propagate actual clang failures.
#endif

typedef unsigned char BOOLEAN;

extern "C" void* memset(void* destination, int value, unsigned int size);

void mutate(unsigned char& value);
void mutate_pointer(unsigned char* value);

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

    unsigned char and_ready = 1;
    and_ready &= 1;
    unsigned char or_ready = 0;
    or_ready |= 1;
    unsigned char xor_ready = 1;
    xor_ready ^= 1;

    unsigned char address_ready = 0;
    mutate_pointer(&address_ready);
    unsigned char alias_ready = 1;
    unsigned char& alias = alias_ready;
    mutate(alias);
}

struct CombatState {
    unsigned char execution_active_000;
};

bool run_combat(CombatState& state)
{
    state.execution_active_000 = 0;
    bool inactive = state.execution_active_000 == 0;
    state.execution_active_000 = 1;
    if (state.execution_active_000 && state.execution_active_000 != 0) {
        return inactive;
    }
    return false;
}

struct OneFillState {
    unsigned char initialized;
};

void fill_one(OneFillState* state)
{
    memset(state, 1, sizeof *state);
}

struct NonDomainFillState {
    unsigned char non_domain_ready;
};
struct PartialFillState {
    unsigned char partial_ready;
    unsigned char tail;
};
struct AggregateState {
    unsigned char aggregate_ready;
};
void mutate_record(AggregateState* state);

void unknown_producers(NonDomainFillState* non_domain, PartialFillState* partial,
                       AggregateState& destination, const AggregateState& source)
{
    memset(non_domain, 2, sizeof *non_domain);
    memset(partial, 0, sizeof *partial - 1);
    destination = source;
    mutate_record(&destination);
    // Neither a partial/non-domain fill nor an unknown aggregate/alias producer
    // supplies a direct boolean-domain anchor. This is not full alias analysis.
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
