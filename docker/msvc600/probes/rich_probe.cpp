extern "C" int probe_c_linkage(int value) {
    return value * 33 + 17;
}

template <typename T>
static T probe_template(T left, T right) {
    return left < right ? right : left;
}

int main(int argc, char **) {
    return probe_template(probe_c_linkage(argc), 42);
}
