#pragma once

#if defined(SURRENDER_BUILD)
#define SR_DYNAMIC_LIBRARY_API __declspec(dllexport)
#elif defined(_MSC_VER) && !defined(WIZ8_CLANG_LINT)
#define SR_DYNAMIC_LIBRARY_API __declspec(dllimport)
#else
#define SR_DYNAMIC_LIBRARY_API
#endif

class SR_DYNAMIC_LIBRARY_API srDynamicLibrary {
public:
    // SYNTHETIC: SURRENDER 0x10045890
    // srDynamicLibrary::operator=(srDynamicLibrary const &)

    enum Compatibility {
        COMPATIBILITY_0 = 0,
        COMPATIBILITY_1 = 1,
        COMPATIBILITY_2 = 2
    };

    static Compatibility checkCompatibility(const char* name);
    static int free(void* library);
    static void* getFunction(
        void* library, const char* function_name);
    static unsigned long getVersion(const char* name);
    static void* load(const char* name);
    static int testDependencies(const char* name);
};

static_assert(sizeof(srDynamicLibrary) == 0x01,
              "srDynamicLibrary_must_be_stateless");

#undef SR_DYNAMIC_LIBRARY_API
