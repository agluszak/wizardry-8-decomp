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

State::State()
    : m_dirty(0)
    , mask(0)
{
}

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

    unsigned char wire_ready = 0; // bool-byte-ok: serialized protocol byte
    (void)wire_ready;
}
